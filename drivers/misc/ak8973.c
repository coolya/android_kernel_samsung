/*
 * ak8973.c - ak8973 compass driver
 *
 * Copyright (C) 2008-2009 HTC Corporation.
 * Author: viral wang <viralwang@gmail.com>
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
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

#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c/ak8973.h>
#include <linux/completion.h>
#include "ak8973-reg.h"

#define AK8973DRV_DATA_DBG 0

struct akm8973_data {
	struct i2c_client *this_client;
	struct akm8973_platform_data *pdata;
	struct mutex lock;
	struct miscdevice akmd_device;
	int irq;
	struct completion data_ready;
	wait_queue_head_t state_wq;
};

static s32 akm8973_ecs_set_mode_power_down(struct akm8973_data *akm)
{
	s32 ret;

	ret = i2c_smbus_write_byte_data(akm->this_client,
			AK8973_REG_MS1, AK8973_MODE_POWERDOWN);
	if (ret < 0)
		return ret;

	return i2c_smbus_read_byte_data(akm->this_client, AK8973_REG_TMPS);
}

static int akm8973_ecs_set_mode(struct akm8973_data *akm, char mode)
{
	s32 ret;

	switch (mode) {
	case AK8973_MODE_MEASURE:
		ret = i2c_smbus_write_byte_data(akm->this_client,
				AK8973_REG_MS1, AK8973_MODE_MEASURE);
		break;
	case AK8973_MODE_E2P_READ:
		ret = i2c_smbus_write_byte_data(akm->this_client,
				AK8973_REG_MS1, AK8973_MODE_E2P_READ);
		break;
	case AK8973_MODE_POWERDOWN:
		ret = akm8973_ecs_set_mode_power_down(akm);
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0)
		return ret;

	/* Wait at least 300us after changing mode. */
	udelay(300);

	return 0;
}

static void akm8973_reset(struct akm8973_data *akm)
{
	gpio_set_value(akm->pdata->reset_line, akm->pdata->reset_asserted);
	msleep(2);
	gpio_set_value(akm->pdata->reset_line, !akm->pdata->reset_asserted);
}

static int akmd_copy_in(unsigned int cmd, void __user *argp,
			void *buf, size_t buf_size)
{
	if (!(cmd & IOC_IN))
		return 0;
	if (_IOC_SIZE(cmd) > buf_size)
		return -EINVAL;
	if (copy_from_user(buf, argp, _IOC_SIZE(cmd)))
		return -EFAULT;
	return 0;
}

static int akmd_copy_out(unsigned int cmd, void __user *argp,
			 void *buf, size_t buf_size)
{
	if (!(cmd & IOC_OUT))
		return 0;
	if (_IOC_SIZE(cmd) > buf_size)
		return -EINVAL;
	if (copy_to_user(argp, buf, _IOC_SIZE(cmd)))
		return -EFAULT;
	return 0;
}

static void akm8973_disable_irq(struct akm8973_data *akm)
{
	disable_irq(akm->irq);
	if (try_wait_for_completion(&akm->data_ready)) {
		/* we actually got the interrupt before we could disable it
		 * so we need to enable again to undo our disable since the
		 * irq_handler already disabled it
		 */
		enable_irq(akm->irq);
	}
}

static irqreturn_t akm8973_irq_handler(int irq, void *data)
{
	struct akm8973_data *akm = data;
	disable_irq_nosync(irq);
	complete(&akm->data_ready);
	return IRQ_HANDLED;
}

static int akm8973_wait_for_data_ready(struct akm8973_data *akm)
{
	int data_ready = gpio_get_value(akm->pdata->gpio_data_ready_int);
	int err;

	if (data_ready)
		return 0;

	enable_irq(akm->irq);

	err = wait_for_completion_timeout(&akm->data_ready, 5*HZ);
	if (err > 0)
		return 0;

	akm8973_disable_irq(akm);

	if (err == 0) {
		pr_err("akm: wait timed out\n");
		return -ETIMEDOUT;
	}

	pr_err("akm: wait restart\n");
	return err;
}

static int akmd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct akm8973_data *akm = container_of(file->private_data,
			struct akm8973_data, akmd_device);
	int ret;
	union {
		char raw[RWBUF_SIZE];
		int status;
		char mode;
		u8 data[5];
	} rwbuf;

	ret = akmd_copy_in(cmd, argp, rwbuf.raw, sizeof(rwbuf));
	if (ret)
		return ret;

	switch (cmd) {
	case ECS_IOCTL_WRITE:
		if ((rwbuf.raw[0] < 2) || (rwbuf.raw[0] > (RWBUF_SIZE - 1)))
			return -EINVAL;
		if (copy_from_user(&rwbuf.raw[2], argp+2, rwbuf.raw[0]-1))
			return -EFAULT;

		ret = i2c_smbus_write_i2c_block_data(akm->this_client,
						     rwbuf.raw[1],
						     rwbuf.raw[0] - 1,
						     &rwbuf.raw[2]);
		break;
	case ECS_IOCTL_READ:
		if ((rwbuf.raw[0] < 1) || (rwbuf.raw[0] > (RWBUF_SIZE - 1)))
			return -EINVAL;

		ret = i2c_smbus_read_i2c_block_data(akm->this_client,
						    rwbuf.raw[1],
						    rwbuf.raw[0],
						    &rwbuf.raw[1]);
		if (ret < 0)
			return ret;
		if (copy_to_user(argp, rwbuf.raw+1, rwbuf.raw[0]))
			return -EFAULT;
		return 0;
	case ECS_IOCTL_RESET:
		akm8973_reset(akm);
		break;
	case ECS_IOCTL_SET_MODE:
		ret = akm8973_ecs_set_mode(akm, rwbuf.mode);
		break;
	case ECS_IOCTL_GETDATA:
		ret = akm8973_wait_for_data_ready(akm);
		if (ret)
			return ret;
		ret = i2c_smbus_read_i2c_block_data(akm->this_client,
						    AK8973_REG_ST,
						    sizeof(rwbuf.data),
						    rwbuf.data);
		if (ret != sizeof(rwbuf.data)) {
			pr_err("%s : failed to read %d bytes of mag data\n",
			       __func__, sizeof(rwbuf.data));
			return -EIO;
		}
		break;
	default:
		return -ENOTTY;
	}

	if (ret < 0)
		return ret;

	return akmd_copy_out(cmd, argp, rwbuf.raw, sizeof(rwbuf));
}

static const struct file_operations akmd_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.ioctl = akmd_ioctl,
};

static int akm8973_setup_irq(struct akm8973_data *akm)
{
	int rc = -EIO;
	struct akm8973_platform_data *pdata = akm->pdata;
	int irq;

	rc = gpio_request(pdata->gpio_data_ready_int, "gpio_akm_int");
	if (rc < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
			__func__, pdata->gpio_data_ready_int, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->gpio_data_ready_int);
	if (rc < 0) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
			__func__, pdata->gpio_data_ready_int, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(pdata->gpio_data_ready_int);

	/* trigger high so we don't miss initial interrupt if it
	 * is already pending
	 */
	rc = request_irq(irq, akm8973_irq_handler, IRQF_TRIGGER_HIGH,
			 "akm_int", akm);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			pdata->gpio_data_ready_int, rc);
		goto err_request_irq;
	}

	/* start with interrupt disabled until the driver is enabled */
	akm->irq = irq;
	akm8973_disable_irq(akm);

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->gpio_data_ready_int);
done:
	return rc;
}

int akm8973_probe(struct i2c_client *client,
		const struct i2c_device_id *devid)
{
	struct akm8973_data *akm;
	int err;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit_platform_data_null;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C check failed, exiting.\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	akm = kzalloc(sizeof(struct akm8973_data), GFP_KERNEL);
	if (!akm) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	akm->pdata = client->dev.platform_data;
	mutex_init(&akm->lock);
	init_completion(&akm->data_ready);

	i2c_set_clientdata(client, akm);
	akm->this_client = client;

	err = gpio_request(akm->pdata->reset_line, "AK8973 Reset Line");
	if (err < 0)
		goto exit_reset_gpio_request_failed;
	gpio_direction_output(akm->pdata->reset_line,
			!akm->pdata->reset_asserted);
	akm8973_reset(akm);

	err = akm8973_ecs_set_mode_power_down(akm);
	if (err < 0)
		goto exit_set_mode_power_down_failed;

	err = akm8973_setup_irq(akm);
	if (err) {
		pr_err("%s: could not setup irq\n", __func__);
		goto exit_setup_irq;
	}

	akm->akmd_device.minor = MISC_DYNAMIC_MINOR;
	akm->akmd_device.name = "akm8973";
	akm->akmd_device.fops = &akmd_fops;

	err = misc_register(&akm->akmd_device);
	if (err)
		goto exit_akmd_device_register_failed;

	init_waitqueue_head(&akm->state_wq);

	return 0;

exit_akmd_device_register_failed:
	free_irq(akm->irq, akm);
	gpio_free(akm->pdata->gpio_data_ready_int);
exit_setup_irq:
exit_set_mode_power_down_failed:
	gpio_direction_input(akm->pdata->reset_line);
	gpio_free(akm->pdata->reset_line);
exit_reset_gpio_request_failed:
	mutex_destroy(&akm->lock);
	kfree(akm);
exit_alloc_data_failed:
exit_check_functionality_failed:
exit_platform_data_null:
	return err;
}

static int __devexit akm8973_remove(struct i2c_client *client)
{
	struct akm8973_data *akm = i2c_get_clientdata(client);

	misc_deregister(&akm->akmd_device);
	gpio_direction_input(akm->pdata->reset_line);
	gpio_free(akm->pdata->reset_line);
	free_irq(akm->irq, akm);
	gpio_free(akm->pdata->gpio_data_ready_int);
	mutex_destroy(&akm->lock);
	kfree(akm);
	return 0;
}

static const struct i2c_device_id akm8973_id[] = {
	{AKM8973_I2C_NAME, 0 },
	{ }
};

static struct i2c_driver akm8973_driver = {
	.probe		= akm8973_probe,
	.remove		= akm8973_remove,
	.id_table	= akm8973_id,
	.driver = {
		.name = AKM8973_I2C_NAME,
	},
};

static int __init akm8973_init(void)
{
	return i2c_add_driver(&akm8973_driver);
}

static void __exit akm8973_exit(void)
{
	i2c_del_driver(&akm8973_driver);
}

module_init(akm8973_init);
module_exit(akm8973_exit);

MODULE_DESCRIPTION("AKM8973 compass driver");
MODULE_LICENSE("GPL");
