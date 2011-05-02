/* modem_ctl.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 * Copyright (C) 2010 Kolja Dummann (k.dummann@gmail.com)
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

/* supports modem delta update */
#include "modem_ctl_recovery.h"

/* Defines the primitives for writing and reading from and to the oneDRAM.
 *  All these primitives are used by the functions of file operation.
 */
static u32 onedram_checksum(u32 org, u8 *data, u32 len)
{
	u32 temp = org;
	u32 i;

	for (i = 0; i < len; i++)
		temp += *(data + i);

	return temp;
}

static inline u32 read_semaphore(struct modemctl *mc)
{
	return ioread32(mc->mmio + OFF_SEM) & 1;
}

static void return_semaphore(struct modemctl *mc)
{
	iowrite32(0, mc->mmio + OFF_SEM);
}

static u32 get_mailbox_ab(struct modemctl *mc)
{
	return ioread32(mc->mmio + OFF_MBOX_BP);
}

static void set_mailbox_ba(struct modemctl *mc, u32 data)
{
	iowrite32(data, mc->mmio + OFF_MBOX_AP);
}

static void write_single_data(struct modemctl *mc, int offset, int data)
{
	*(u32 *)(mc->mmio + offset) = data;
}

static int read_multiple_data(struct modemctl *mc, int offset, char *buf,
			    size_t size)
{
	if (!read_semaphore(mc)) {
		pr_err("Semaphore is held by modem!");
		return -EINVAL;
	}

	if (!buf) {
		pr_err("Invalid buffer!");
		return -EINVAL;
	}

	memcpy(buf, mc->mmio + offset, size);

	return size;
}

static int dpram_write_from_user(struct modemctl *mc, int addr,
				const char __user *data, size_t size)
{
	if (!read_semaphore(mc)) {
		pr_err("Semaphore is held by modem!");
		return -EINVAL;
	}

	if (copy_from_user(mc->mmio + addr, data, size) < 0) {
		pr_err("[%s:%d] Copy from user failed\n", __func__, __LINE__);
		return -EINVAL;
	}

	return 0;
}


static int modem_pwr_status(struct modemctl *mc)
{
	pr_debug("%s\n", __func__);
	return gpio_get_value(mc->gpio_phone_active);
}


static int dpram_modem_pwroff(struct modemctl *mc)
{
	pr_debug("%s\n", __func__);

	gpio_set_value(mc->gpio_phone_on, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);
	mdelay(100);

	return 0;
}

static int dpram_modem_reset(struct modemctl *mc)
{
	pr_debug("%s\n", __func__);

	gpio_set_value(mc->gpio_phone_on, 1);
	msleep(50);
	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(100);
	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(500);
	gpio_set_value(mc->gpio_phone_on, 0);

	return 0;
}

static int dpram_modem_pwron(struct modemctl *mc)
{
	int err = -1;
	int msec;

	pr_debug("%s\n", __func__);

	return_semaphore(mc);

	dpram_modem_reset(mc);

	for (msec = 0; msec < 10000; msec++) {
		if (modem_pwr_status(mc)) {
			err = 0;
			break;
		}
		msleep(1);
	}

	return err;
}

static int acquire_semaphore(struct modemctl *mc)
{
	int retrycnt = 20;

	while (!read_semaphore(mc)) {
		set_mailbox_ba(mc, DPRAM_BOOT_SEM_REQ);
		dpram_modem_reset(mc);

		msleep(100);
		if (!(retrycnt--)) {
			pr_debug("failed to get semaphore!");
			return -1;
		}
	}

	pr_debug("We have Semaphore!");
	dpram_modem_pwroff(mc);
	set_mailbox_ba(mc, 0x0);
	return 0;
}

static int dpram_write_delta(struct modemctl *mc,
					char __user *firmware, int size)
{
	int ret = 0;

	pr_debug("%s\n", __func__);
	acquire_semaphore(mc);
	/* write the sizeof firmware */
	write_single_data(mc, DPRAM_FIRMWARE_SIZE_ADDR, size);
	/* write the data of firmware */
	dpram_write_from_user(mc, DPRAM_FIRMWARE_ADDR, firmware, size);
	return ret;
}

static int dpram_write_full(struct modemctl *mc,
					char __user *firmware, int size)
{
	struct onedram_head_t onedram;
	u32 data_offset = ONEDRAM_DL_DATA_OFFSET;
	u32 head_offset = ONEDRAM_DL_HEADER_OFFSET;
	u32 checksum;

	pr_debug("%s\n", __func__);

	acquire_semaphore(mc);

	/* write full data of firmware */
	dpram_write_from_user(mc, data_offset, firmware, size);
	/* check checksum */
	checksum = onedram_checksum(0, (u8 *)(mc->mmio + data_offset), size);

	onedram.signature	= ONEDRAM_DL_SIGNATURE;
	onedram.is_boot_update	= 0;
	onedram.is_nv_update	= 0;
	onedram.length		= size;
	onedram.checksum	= checksum;

	/* write header info */
	memcpy(mc->mmio + head_offset, (u8 *)&onedram, sizeof(onedram));

	return 0;
}

static int dpram_update_delta(struct modemctl *mc)
{
	int err = 0;
	int msec = 0;
	u32 val;

	pr_debug("%s\n", __func__);

	acquire_semaphore(mc);
	/* write boot magic */
	write_single_data(mc, DPRAM_BOOT_MAGIC_ADDR,
				DPRAM_BOOT_MAGIC_RECOVERY_FOTA);
	write_single_data(mc, DPRAM_BOOT_TYPE_ADDR,
				DPRAM_BOOT_TYPE_DPRAM_DELTA);
	/* At this point modem is powered off.  So power on modem */
	err = dpram_modem_pwron(mc);
	if (err < 0) {
		pr_err("modem_reset() fail : %d", modem_pwr_status(mc));
		return err;
	}
	/* clear mailboxBA */
	set_mailbox_ba(mc, 0xFFFFFFFF);
	/* wait for job sync message */
	while (true) {
		val = get_mailbox_ab(mc);
		if ((val & STATUS_JOB_MAGIC_M) == STATUS_JOB_MAGIC_CODE) {
			err = 0;
			break;
		}
		msleep(1);
		if (++msec > 20000) {
			err = -2;
			pr_err("Failed to sync with modem (%x)", val);
			return err;
		}
		if ((msec % 1000) == 0)
			pr_info("Waiting for sync message... 0x%08x (pwr:%s)", \
			val, modem_pwr_status(mc) ? "ON" : "OFF");
	}
	if (err == 0) {
		pr_info("Modem ready to start the firmware update");
		/* let modem start the job */
		set_mailbox_ba(mc, STATUS_JOB_MAGIC_CODE);
		/* If we have the semaphore, toss it to modem. */
		return_semaphore(mc);
	}

	return err;

}

static int dpram_update_full(struct modemctl *mc)
{
	int err;

	pr_debug("%s\n", __func__);

	err = dpram_modem_pwron(mc);
	if (err < 0) {
		pr_err("dpram_modem_pwron() fail : %d", modem_pwr_status(mc));
		return -1;
	}
	/* clear mailboxBA */
	set_mailbox_ba(mc, 0xFFFFFFFF);

	return 0;
}

static int dpram_process_modem_update(struct modemctl *mc,
		struct dpram_firmware *pfw)
{
	int ret = 0;

	if (pfw->is_delta) {
		mc->is_modem_delta_update = 1;

		if (dpram_write_delta(mc, pfw->firmware, pfw->size) < 0) {
			pr_err("firmware write failed\n");
			ret = -1;
		} else if (dpram_update_delta(mc) < 0) {
			pr_err("Firmware update failed\n");
			ret = -1;
		}
	} else {
		if (dpram_write_full(mc, pfw->firmware, pfw->size) < 0) {
			pr_err("firmware full write failed\n");
			ret = -1;
		} else if (dpram_update_full(mc) < 0) {
			pr_err("Firmware full update failed\n");
			ret = -1;
		}
	}

	return ret;
}

static int dpram_chk_delta_update(struct modemctl *mc,
				int __user *pct, char __user *msg)
{
	u32 status;
	int percent = 0;
	int err = 0;
	char buf[DPRAM_MODEM_MSG_SIZE];
	int debugprint = false;
	int wait = 0;
	/* check mailboxAB for the modem status */
	status = get_mailbox_ab(mc);

	debugprint = (mc->dpram_prev_status != status);
	if (debugprint)
		pr_info("Job status : 0x%08x (pwr:%s)\n", status, \
			modem_pwr_status(mc) ? "ON" : "OFF");

	if ((status & STATUS_JOB_MAGIC_M) != STATUS_JOB_MAGIC_CODE) {
		if (debugprint)
			pr_info("Job not accepted yet\n");
		err = 1;
		percent = 0;
		strncpy(buf, "Job not accepted yet", DPRAM_MODEM_MSG_SIZE);
		goto out;
	}
	if (status & STATUS_JOB_STARTED_M) {
		return_semaphore(mc);
		percent = status & STATUS_JOB_PROGRESS_M;
		if (debugprint)
			pr_info("Job progress pct=%d\n", percent);
		err = 3;
	} else {
		percent = 0;
		if (debugprint)
			pr_info("Job NOT started yet...\n");
		err = 2;
	}
	if (status & STATUS_JOB_ENDED_M) {
		percent = status & STATUS_JOB_PROGRESS_M;
		/* wait till we have semaphore */
		pr_info("Wait for semaphore");
		while (true) {
			msleep(10);
			if (read_semaphore(mc)) {
				pr_info("We have semaphore");
				break;
			}
			if (wait++ > 1000) {
				pr_info("Proceeding without semaphore");
				break;
			}
		}
		read_multiple_data(mc, DPRAM_MODEM_STRING_MSG_ADDR, buf,
							DPRAM_MODEM_MSG_SIZE);
		if (status & STATUS_JOB_ERROR_M) {
			err = -1;
			pr_err("Job ended with error msg : %s\n", buf);
		} else if (status & STATUS_JOB_COMPLETE_M) {
			err = 0;
			pr_info("Job completed successfully : %s\n", buf);
		}
	}
out:
	mc->dpram_prev_status = status;
	if (put_user(percent, pct) < 0)
		pr_err("[%s:%d] Copy to user failed\n", __func__, __LINE__);
	if (copy_to_user((void *)msg, (void *)buf, DPRAM_MODEM_MSG_SIZE) < 0)
		pr_err("[%s:%d] Copy to user failed\n", __func__, __LINE__);
	return err;
}

static int dpram_chk_full_update(struct modemctl *mc,
					int __user *pct, char __user *msg)
{
	int err;
	u32 status = 0;
	u32 phone_active = 0;
	bool is_reboot = 0;

	err = 3;
	mc->dpram_prev_status = 0xFFFFFFFF;
	mc->dpram_prev_phone_active = 0xFFFFFFFF;

retry:
	phone_active = modem_pwr_status(mc);
	status = get_mailbox_ab(mc);

	pr_debug("PHONE %d Mailbox 0x%x\n", phone_active, status);
	if ((mc->dpram_prev_phone_active != phone_active) ||
			(mc->dpram_prev_status != status)) {
		mc->dpram_prev_phone_active = phone_active;
		mc->dpram_prev_status = status;
	}

	if (!phone_active) {
		if (status == ONEDRAM_DL_COMPLETE) {
			pr_info("*OK* ONEDRAM_DL_COMPLETE\n");
			err = 0;
		} else if (status == ONEDRAM_DL_DONE_AND_RESET) {
			pr_info("*OK* ONEDRAM_DONE_AND_RESET\n");
			dpram_modem_pwron(mc);
			goto retry;
		}
	}

	if (status == ONEDRAM_DL_CHECKSUM_ERR) {
		pr_info("*ERROR* ONEDRAM_DL_CHECKSUM_ERR\n");
		is_reboot = 1;
	} else if (status == ONEDRAM_DL_ERASE_WRITE_ERR) {
		pr_info("*ERROR* ONEDRAM_DL_ERASE_WRITE_ERR\n");
		is_reboot = 1;
	} else if (status == ONEDRAM_DL_BOOT_UPDATE_ERR) {
		pr_info("*ERROR* ONEDRAM_DL_BOOT_UPDATE_ERR\n");
		is_reboot = 1;
	} else if (status == ONEDRAM_DL_REWRITE_FAIL_ERR) {
		pr_info("*ERROR* ONEDRAM_DL_REWRITE_FAIL_ERR\n");
		is_reboot = 1;
	} else if (status == ONEDRAM_DL_LENGTH_CH_FAIL) {
		pr_info("*ERROR* ONEDRAM_DL_LENGTH_CH_FAIL\n");
		is_reboot = 1;
	} else {
		if (status != mc->dpram_prev_status)
			pr_info("*ERROR* %d, 0x%x\n", phone_active, status);
	}

	if (is_reboot) {
		pr_info("system reboot necessary\n");
		err = -1;
	}

	if (err <= 0) {
		pr_info("Update done.\n");
		acquire_semaphore(mc);
		write_single_data(mc, 0, 0xffffffff);
	}

	return err;
}

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
	if (modem_request_mmio(mc) == 0) {
		int ret = wait_event_interruptible_timeout(
			mc->wq, mmio_owner_p(mc), 5 * HZ);
		if (ret <= 0) {
			modem_release_mmio(mc, 0);
			if (ret == 0) {
				pr_err("modem_acquire_mmio() TIMEOUT\n");
				return -ENODEV;
			} else {
				return -ERESTARTSYS;
			}
		}
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
			wait_event_timeout(mc->wq, mc->ramdump_size != 0, 10 * HZ);
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
	int ret;

	pr_info("[MODEM] modem_start() %s\n",
		ramdump ? "ramdump" : "normal");

	if (mc->status != MODEM_POWER_ON) {
		pr_err("[MODEM] modem not powered on\n");
		return -EINVAL;
	}

#ifdef CONFIG_MODEM_HAS_CRAPPY_BOOTLOADER

	/* we do this as the BP bootloader from the SGS is a little bit
	   crapy it does not send the magic data MODEM_MSG_SBL_DONE when
	   it has finished loading. so we wait some amount of time */

	pr_info("[MODEM] we have a crappy bootloader an wait for it");

	//waiting 1500 ms should be enough, maybe we can decrease this but unsure
	msleep(1500);

#else
	if (!mc->is_cdma_modem &&
			readl(mc->mmio + OFF_MBOX_BP) != MODEM_MSG_SBL_DONE) {
		pr_err("[MODEM] bootloader not ready\n");
		return -EIO;
	}
#endif

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

		ret = wait_event_timeout(mc->wq, mc->status == MODEM_DUMPING, 25 * HZ);
		if (ret == 0)
			return -ENODEV;
	} else {
		if (mc->is_cdma_modem)
			mc->status = MODEM_RUNNING;
		else {
			mc->status = MODEM_BOOTING_NORMAL;
			writel(MODEM_CMD_BINARY_LOAD, mc->mmio + OFF_MBOX_AP);

			ret = wait_event_timeout(mc->wq,
						modem_running(mc), 25 * HZ);
			if (ret == 0)
				return -ENODEV;
		}
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

	if (mc->is_cdma_modem) {
		gpio_set_value(mc->gpio_phone_on, 1);
		msleep(50);

		/* ensure cp_reset pin set to low */
		gpio_set_value(mc->gpio_cp_reset, 0);
		msleep(100);

		gpio_set_value(mc->gpio_cp_reset, 1);
		msleep(500);

	} else {
		/* ensure cp_reset pin set to low */
		gpio_set_value(mc->gpio_cp_reset, 0);
		msleep(100);

		gpio_set_value(mc->gpio_cp_reset, 0);
		msleep(100);

		gpio_set_value(mc->gpio_cp_reset, 1);

		/* Follow RESET timming delay not Power-On timming,
		because CP_RST & PHONE_ON have been set high already. */
		msleep(100); /*wait modem stable */
	}

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
	struct dpram_firmware fw;
	struct stat_info *pst;
	int ret = 0;

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

	/* CDMA modem update in recovery mode */
	case IOCTL_MODEM_FW_UPDATE:
		pr_info("IOCTL_MODEM_FW_UPDATE\n");
		if (arg == NULL) {
			pr_err("No firmware");
			break;
		}

		if (copy_from_user((void *)&fw, (void *)arg, sizeof(fw)) < 0) {
			pr_err("copy from user failed!");
			ret = -EINVAL;
		} else if (dpram_process_modem_update(mc, &fw) < 0) {
			pr_err("firmware write failed\n");
			ret = -EIO;
		}
		break;
	case IOCTL_MODEM_CHK_STAT:
		pst = (struct stat_info *)arg;
		if (mc->is_modem_delta_update)
			ret = dpram_chk_delta_update(mc, &(pst->pct), pst->msg);
		else
			ret = dpram_chk_full_update(mc, &(pst->pct), pst->msg);
		break;
	case IOCTL_MODEM_PWROFF:
		pr_info("IOCTL_MODEM_PWROFF\n");
		dpram_modem_pwroff(mc);
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
	int value = 0;

	value = gpio_get_value(((struct modemctl *)_mc)->gpio_phone_active);
	pr_info("[MODEM] bp_irq() PHONE_ACTIVE_PIN=%d\n", value);
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
				modem_update_state(mc);
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
	mc->gpio_phone_on = pdata->gpio_phone_on;
	mc->is_cdma_modem = pdata->is_cdma_modem;

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

