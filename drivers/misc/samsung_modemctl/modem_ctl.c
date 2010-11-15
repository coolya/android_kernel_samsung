/* modem_ctl.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include "modem_ctl.h"
#include "modem_ctl_p.h"

/* The modem_ctl portion of this driver handles modem lifecycle
 * transitions (OFF -> ON -> RUNNING -> ABNORMAL), the firmware
 * download mechanism (via /dev/modem_ctl), and interrupts from
 * the modem (direct and via onedram mailbox interrupt).
 *
 * It also handles tracking the ownership of the onedram "semaphore"
 * which governs which processor (AP or BP) has access to the 16MB
 * shared memory region.  The modem_mmio_{acquire,release,request}
 * primitives are used by modem_io.c to obtain access to the shared
 * memory region when necessary to do io.
 *
 * Further, modem_update_state() and modem_handle_io() are called
 * when we gain control over the shared memory region (to update
 * fifo state info) and when there may be io to process, respectively.
 *
 */

#define WAIT_TIMEOUT                (HZ*5)

void modem_request_sem(struct modemctl *mc)
{
	writel(MB_COMMAND | MB_VALID | MBC_REQ_SEM,
	       mc->mmio + OFF_MBOX_AP);
}

static inline int mmio_sem(struct modemctl *mc)
{
	return readl(mc->mmio + OFF_SEM) & 1;
}

int modem_request_mmio(struct modemctl *mc)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&mc->lock, flags);
	mc->mmio_req_count++;
	ret = mc->mmio_owner;
	if (!ret) {
		if (mmio_sem(mc) == 1) {
			/* surprise! we already have control */
			ret = mc->mmio_owner = 1;
			wake_up(&mc->wq);
			modem_update_state(mc);
			MODEM_COUNT(mc,request_no_wait);
		} else {
			/* ask the modem for mmio access */
			if (modem_running(mc))
				modem_request_sem(mc);
			MODEM_COUNT(mc,request_wait);
		}
	} else {
		MODEM_COUNT(mc,request_no_wait);
	}
	/* TODO: timer to retry? */
	spin_unlock_irqrestore(&mc->lock, flags);
	return ret;
}

void modem_release_mmio(struct modemctl *mc, unsigned bits)
{
	unsigned long flags;
	spin_lock_irqsave(&mc->lock, flags);
	mc->mmio_req_count--;
	mc->mmio_signal_bits |= bits;
	if ((mc->mmio_req_count == 0) && modem_running(mc)) {
		if (mc->mmio_bp_request) {
			mc->mmio_bp_request = 0;
			writel(0, mc->mmio + OFF_SEM);
			writel(MB_COMMAND | MB_VALID | MBC_RES_SEM,
			       mc->mmio + OFF_MBOX_AP);
			MODEM_COUNT(mc,release_bp_waiting);
		} else if (mc->mmio_signal_bits) {
			writel(0, mc->mmio + OFF_SEM);
			writel(MB_VALID | mc->mmio_signal_bits,
			       mc->mmio + OFF_MBOX_AP);
			MODEM_COUNT(mc,release_bp_signaled);
		} else {
			MODEM_COUNT(mc,release_no_action);
		}
		mc->mmio_owner = 0;
		mc->mmio_signal_bits = 0;
	}
	spin_unlock_irqrestore(&mc->lock, flags);
}

static int mmio_owner_p(struct modemctl *mc)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&mc->lock, flags);
	ret = mc->mmio_owner || modem_offline(mc);
	spin_unlock_irqrestore(&mc->lock, flags);
	return ret;
}

int modem_acquire_mmio(struct modemctl *mc)
{
	if (modem_request_mmio(mc) == 0)
		if (wait_event_interruptible(mc->wq, mmio_owner_p(mc))) {
			modem_release_mmio(mc, 0);
			return -ERESTARTSYS;
		}
	if (!modem_running(mc)) {
		modem_release_mmio(mc, 0);
		return -ENODEV;
	}
	return 0;
}

static int modemctl_open(struct inode *inode, struct file *filp)
{
	struct modemctl *mc = to_modemctl(filp->private_data);
	filp->private_data = mc;

	if (mc->open_count)
		return -EBUSY;

	mc->open_count++;
	return 0;
}

static int modemctl_release(struct inode *inode, struct file *filp)
{
	struct modemctl *mc = filp->private_data;

	mc->open_count = 0;
	filp->private_data = NULL;
	return 0;
}

static ssize_t modemctl_read(struct file *filp, char __user *buf,
			     size_t count, loff_t *ppos)
{
	struct modemctl *mc = filp->private_data;
	loff_t pos;
	int ret;

	mutex_lock(&mc->ctl_lock);
	pos = mc->ramdump_pos;
	if (mc->status != MODEM_DUMPING) {
		pr_err("[MODEM] not in ramdump mode\n");
		ret = -ENODEV;
		goto done;
	}
	if (pos < 0) {
		ret = -EINVAL;
		goto done;
	}
	if (pos >= mc->ramdump_size) {
		pr_err("[MODEM] ramdump EOF\n");
		ret = 0;
		goto done;
	}
	if (count > mc->ramdump_size - pos)
		count = mc->ramdump_size - pos;

	ret = copy_to_user(buf, mc->mmio + pos, count);
	if (ret) {
		ret = -EFAULT;
		goto done;
	}
	pos += count;
	ret = count;

	if (pos == mc->ramdump_size) {
		if (mc->ramdump_size == RAMDUMP_LARGE_SIZE) {
			mc->ramdump_size = 0;
			pr_info("[MODEM] requesting more ram\n");
			writel(0, mc->mmio + OFF_SEM);
			writel(MODEM_CMD_RAMDUMP_MORE, mc->mmio + OFF_MBOX_AP);
			wait_event(mc->wq, mc->ramdump_size != 0); /* TODO: timeout */
		} else {
			pr_info("[MODEM] no more ram to dump\n");
			mc->ramdump_size = 0;
		}
		mc->ramdump_pos = 0;
	} else {
		mc->ramdump_pos = pos;
	}
	
done:
	mutex_unlock(&mc->ctl_lock);
	return ret;

}

static ssize_t modemctl_write(struct file *filp, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	struct modemctl *mc = filp->private_data;
	u32 owner;
	char *data;
	loff_t pos = *ppos;
	unsigned long ret;

	mutex_lock(&mc->ctl_lock);
	data = (char __force *)mc->mmio + pos;
	owner = mmio_sem(mc);

	if (mc->status != MODEM_POWER_ON) {
		pr_err("modemctl_write: modem not powered on\n");
		ret = -EINVAL;
		goto done;
	}

	if (!owner) {
		pr_err("modemctl_write: doesn't own semaphore\n");
		ret = -EIO;
		goto done;
	}

	if (pos < 0) {
		ret = -EINVAL;
		goto done;
	}

	if (pos >= mc->mmsize) {
		ret = -EINVAL;
		goto done;
	}

	if (count > mc->mmsize - pos)
		count = mc->mmsize - pos;

	ret = copy_from_user(data, buf, count);
	if (ret) {
		ret = -EFAULT;
		goto done;
	}
	*ppos = pos + count;
	ret = count;

done:
	mutex_unlock(&mc->ctl_lock);
	return ret;
}


static int modem_start(struct modemctl *mc, int ramdump)
{
	pr_info("[MODEM] modem_start() %s\n",
		ramdump ? "ramdump" : "normal");

	if (mc->status != MODEM_POWER_ON) {
		pr_err("[MODEM] modem not powered on\n");
		return -EINVAL;
	}

	if (readl(mc->mmio + OFF_MBOX_BP) != MODEM_MSG_SBL_DONE) {
		pr_err("[MODEM] bootloader not ready\n");
		return -EIO;
	}

	if (mmio_sem(mc) != 1) {
		pr_err("[MODEM] we do not own the semaphore\n");
		return -EIO;
	}

	writel(0, mc->mmio + OFF_SEM);
	if (ramdump) {
		mc->status = MODEM_BOOTING_RAMDUMP;
		mc->ramdump_size = 0;
		mc->ramdump_pos = 0;
		writel(MODEM_CMD_RAMDUMP_START, mc->mmio + OFF_MBOX_AP);

		/* TODO: timeout and fail */
		wait_event(mc->wq, mc->status == MODEM_DUMPING);
	} else {
		mc->status = MODEM_BOOTING_NORMAL;
		writel(MODEM_CMD_BINARY_LOAD, mc->mmio + OFF_MBOX_AP);

		/* TODO: timeout and fail */
		wait_event(mc->wq, modem_running(mc));
	}


	pr_info("[MODEM] modem_start() DONE\n");
	return 0;
}

static int modem_reset(struct modemctl *mc)
{
	pr_info("[MODEM] modem_reset()\n");

	/* ensure phone active pin irq type */
	set_irq_type(mc->gpio_phone_active, IRQ_TYPE_EDGE_BOTH);

	/* ensure pda active pin set to low */
	gpio_set_value(mc->gpio_pda_active, 0);

	/* read inbound mbox to clear pending IRQ */
	(void) readl(mc->mmio + OFF_MBOX_BP);

	/* write outbound mbox to assert outbound IRQ */
	writel(0, mc->mmio + OFF_MBOX_AP);

	/* ensure cp_reset pin set to low */
	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 1);

	/* Follow RESET timming delay not Power-On timming,
	   because CP_RST & PHONE_ON have been set high already. */
	msleep(100); /*wait modem stable */

	gpio_set_value(mc->gpio_pda_active, 1);

	mc->status = MODEM_POWER_ON;

	return 0;
}

static int modem_off(struct modemctl *mc)
{
	pr_info("[MODEM] modem_off()\n");
	gpio_set_value(mc->gpio_cp_reset, 0);
	mc->status = MODEM_OFF;
	return 0;
}

static long modemctl_ioctl(struct file *filp,
			   unsigned int cmd, unsigned long arg)
{
	struct modemctl *mc = filp->private_data;
	int ret;

	mutex_lock(&mc->ctl_lock);
	switch (cmd) {
	case IOCTL_MODEM_RESET:
		ret = modem_reset(mc);
		MODEM_COUNT(mc,resets);
		break;
	case IOCTL_MODEM_START:
		ret = modem_start(mc, 0);
		break;
	case IOCTL_MODEM_RAMDUMP:
		ret = modem_start(mc, 1);
		break;
	case IOCTL_MODEM_OFF:
		ret = modem_off(mc);
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&mc->ctl_lock);
	pr_info("modemctl_ioctl() %d\n", ret);
	return ret;
}

static const struct file_operations modemctl_fops = {
	.owner =		THIS_MODULE,
	.open =			modemctl_open,
	.release =		modemctl_release,
	.read =			modemctl_read,
	.write =		modemctl_write,
	.unlocked_ioctl =	modemctl_ioctl,
};

static irqreturn_t modemctl_bp_irq_handler(int irq, void *_mc)
{
	pr_info("[MODEM] bp_irq()\n");
	return IRQ_HANDLED;

}

static void modemctl_handle_offline(struct modemctl *mc, unsigned cmd)
{
	switch (mc->status) {
	case MODEM_BOOTING_NORMAL:
		if (cmd == MODEM_MSG_BINARY_DONE) {
			pr_info("[MODEM] binary load done\n");
			mc->status = MODEM_RUNNING;
			wake_up(&mc->wq);
		}
		break;
	case MODEM_BOOTING_RAMDUMP:
	case MODEM_DUMPING:
		if (cmd == MODEM_MSG_RAMDUMP_LARGE) {
			mc->status = MODEM_DUMPING;
			mc->ramdump_size = RAMDUMP_LARGE_SIZE;
			wake_up(&mc->wq);
			pr_info("[MODEM] ramdump - %d bytes available\n",
				mc->ramdump_size);
		} else if (cmd == MODEM_MSG_RAMDUMP_SMALL) {
			mc->status = MODEM_DUMPING;
			mc->ramdump_size = RAMDUMP_SMALL_SIZE;
			wake_up(&mc->wq);
			pr_info("[MODEM] ramdump - %d bytes available\n",
				mc->ramdump_size);
		} else {
			pr_err("[MODEM] unknown msg %08x in ramdump mode\n", cmd);
		}
		break;
	}
}

static irqreturn_t modemctl_mbox_irq_handler(int irq, void *_mc)
{
	struct modemctl *mc = _mc;
	unsigned cmd;
	unsigned long flags;

	cmd = readl(mc->mmio + OFF_MBOX_BP);

	if (unlikely(mc->status != MODEM_RUNNING)) {
		modemctl_handle_offline(mc, cmd);
		return IRQ_HANDLED;
	}

	if (!(cmd & MB_VALID)) {
		if (cmd == MODEM_MSG_LOGDUMP_DONE) {
			pr_info("modem: logdump done!\n");
			mc->logdump_data = 1;
			wake_up(&mc->wq);
		} else {
			pr_info("modem: what is %08x\n",cmd);
		}
		return IRQ_HANDLED;
	}

	spin_lock_irqsave(&mc->lock, flags);

	if (cmd & MB_COMMAND) {
		switch (cmd & 15) {
		case MBC_REQ_SEM:
			if (mmio_sem(mc) == 0) {
				/* Sometimes the modem may ask for the
				 * sem when it already owns it.  Humor
				 * it and ack that request.
				 */
				writel(MB_COMMAND | MB_VALID | MBC_RES_SEM,
				       mc->mmio + OFF_MBOX_AP);
				MODEM_COUNT(mc,bp_req_confused);
			} else if (mc->mmio_req_count == 0) {
				/* No references? Give it to the modem. */
				mc->mmio_owner = 0;
				writel(0, mc->mmio + OFF_SEM);
				writel(MB_COMMAND | MB_VALID | MBC_RES_SEM,
				       mc->mmio + OFF_MBOX_AP);
				MODEM_COUNT(mc,bp_req_instant);
				goto done;
			} else {
				/* Busy now, remember the modem needs it. */
				mc->mmio_bp_request = 1;
				MODEM_COUNT(mc,bp_req_delayed);
				break;
			}
		case MBC_RES_SEM:
			break;
		case MBC_PHONE_START:
			/* TODO: should we avoid sending any other messages
			 * to the modem until this message is received and
			 * acknowledged?
			 */
			writel(MB_COMMAND | MB_VALID |
			       MBC_INIT_END | CP_BOOT_AIRPLANE | AP_OS_ANDROID,
			       mc->mmio + OFF_MBOX_AP);

			/* TODO: probably unsafe to send this back-to-back
			 * with the INIT_END message.
			 */
			/* if somebody is waiting for mmio access... */
			if (mc->mmio_req_count)
				modem_request_sem(mc);
			break;
		case MBC_RESET:
			pr_err("$$$ MODEM RESET $$$\n");
			mc->status = MODEM_CRASHED;
			wake_up(&mc->wq);
			break;
		case MBC_ERR_DISPLAY: {
			char buf[SIZ_ERROR_MSG + 1];
			int i;
			pr_err("$$$ MODEM ERROR $$$\n");
			mc->status = MODEM_CRASHED;
			wake_up(&mc->wq);
			memcpy(buf, mc->mmio + OFF_ERROR_MSG, SIZ_ERROR_MSG);
			for (i = 0; i < SIZ_ERROR_MSG; i++)
				if ((buf[i] < 0x20) || (buf[1] > 0x7e))
					buf[i] = 0x20;
			buf[i] = 0;
			i--;
			while ((i > 0) && (buf[i] == 0x20))
				buf[i--] = 0;
			pr_err("$$$ %s $$$\n", buf);
			break;
		}
		case MBC_SUSPEND:
			break;
		case MBC_RESUME:
			break;
		}
	}

	/* On *any* interrupt from the modem it may have given
	 * us ownership of the mmio hw semaphore.  If that
	 * happens, we should claim the semaphore if we have
	 * threads waiting for it and we should process any
	 * messages that the modem has enqueued in its fifos
	 * by calling modem_handle_io().
	 */
	if (mmio_sem(mc) == 1) {
		if (!mc->mmio_owner) {
			modem_update_state(mc);
			if (mc->mmio_req_count) {
				mc->mmio_owner = 1;
				wake_up(&mc->wq);
			}
		}

		modem_handle_io(mc);

		/* If we have a signal to send and we're not
		 * hanging on to the mmio hw semaphore, give
		 * it back to the modem and send the signal.
		 * Otherwise this will happen when we give up
		 * the mmio hw sem in modem_release_mmio().
		 */
		if (mc->mmio_signal_bits && !mc->mmio_owner) {
			writel(0, mc->mmio + OFF_SEM);
			writel(MB_VALID | mc->mmio_signal_bits,
			       mc->mmio + OFF_MBOX_AP);
			mc->mmio_signal_bits = 0;
		}
	}
done:
	spin_unlock_irqrestore(&mc->lock, flags);
	return IRQ_HANDLED;
}

void modem_force_crash(struct modemctl *mc)
{
	unsigned long int flags;
	pr_info("modem_force_crash() BOOM!\n");
	spin_lock_irqsave(&mc->lock, flags);
	mc->status = MODEM_CRASHED;
	wake_up(&mc->wq);
	spin_unlock_irqrestore(&mc->lock, flags);
}

static int __devinit modemctl_probe(struct platform_device *pdev)
{
	int r = -ENOMEM;
	struct modemctl *mc;
	struct modemctl_data *pdata;
	struct resource *res;

	pdata = pdev->dev.platform_data;

	mc = kzalloc(sizeof(*mc), GFP_KERNEL);
	if (!mc)
		return -ENOMEM;

	init_waitqueue_head(&mc->wq);
	spin_lock_init(&mc->lock);
	mutex_init(&mc->ctl_lock);

	mc->irq_bp = platform_get_irq_byname(pdev, "active");
	mc->irq_mbox = platform_get_irq_byname(pdev, "onedram");

	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		goto err_free;
	mc->mmbase = res->start;
	mc->mmsize = resource_size(res);

	mc->mmio = ioremap_nocache(mc->mmbase, mc->mmsize);
	if (!mc->mmio)
		goto err_free;

	platform_set_drvdata(pdev, mc);

	mc->dev.name = "modem_ctl";
	mc->dev.minor = MISC_DYNAMIC_MINOR;
	mc->dev.fops = &modemctl_fops;
	r = misc_register(&mc->dev);
	if (r)
		goto err_ioremap;

	/* hide control registers from userspace */
	mc->mmsize -= 0x800;
	mc->status = MODEM_OFF;

	wake_lock_init(&mc->ip_tx_wakelock,
		       WAKE_LOCK_SUSPEND, "modem_ip_tx");
	wake_lock_init(&mc->ip_rx_wakelock,
		       WAKE_LOCK_SUSPEND, "modem_ip_rx");

	modem_io_init(mc, mc->mmio);

	r = request_irq(mc->irq_bp, modemctl_bp_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"modemctl_bp", mc);
	if (r)
		goto err_ioremap;

	r = request_irq(mc->irq_mbox, modemctl_mbox_irq_handler,
			IRQF_TRIGGER_LOW, "modemctl_mbox", mc);
	if (r)
		goto err_irq_bp;

	enable_irq_wake(mc->irq_bp);
	enable_irq_wake(mc->irq_mbox);

	modem_debugfs_init(mc);

	return 0;

err_irq_mbox:
	free_irq(mc->irq_mbox, mc);
err_irq_bp:
	free_irq(mc->irq_bp, mc);
err_ioremap:
	iounmap(mc->mmio);
err_free:
	kfree(mc);
	return r;
}

static int modemctl_suspend(struct device *pdev)
{
	struct modemctl *mc = dev_get_drvdata(pdev);
	gpio_set_value(mc->gpio_pda_active, 0);
	return 0;
}

static int modemctl_resume(struct device *pdev)
{
	struct modemctl *mc = dev_get_drvdata(pdev);
	gpio_set_value(mc->gpio_pda_active, 1);
	return 0;
}

static const struct dev_pm_ops modemctl_pm_ops = {
	.suspend    = modemctl_suspend,
	.resume     = modemctl_resume,
};

static struct platform_driver modemctl_driver = {
	.probe = modemctl_probe,
	.driver = {
		.name = "modemctl",
		.pm   = &modemctl_pm_ops,
	},
};

static int __init modemctl_init(void)
{
	return platform_driver_register(&modemctl_driver);
}

module_init(modemctl_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung Modem Control Driver");

