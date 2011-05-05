/*
 *  STMicroelectronics kr3dm acceleration sensor driver
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kr3dm.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include "kr3dm_reg.h"

#define kr3dm_dbgmsg(str, args...) pr_debug("%s: " str, __func__, ##args)

/* The default settings when sensor is on is for all 3 axis to be enabled
 * and output data rate set to 400Hz.  Output is via a ioctl read call.
 * The ioctl blocks on data_ready completion.
 * The sensor generates an interrupt when the output is ready and the
 * irq handler atomically sets the completion and wakes any
 * blocked reader.
 */
#define DEFAULT_POWER_ON_SETTING (ODR400 | ENABLE_ALL_AXES)
#define READ_REPEAT_SHIFT	3
#define READ_REPEAT		(1 << READ_REPEAT_SHIFT)

static const struct odr_delay {
	u8 odr; /* odr reg setting */
	s64 delay_ns; /* odr in ns */
} odr_delay_table[] = {
	{  ODR400,    2500000LL << READ_REPEAT_SHIFT }, /* 400Hz */
	{  ODR100,   10000000LL << READ_REPEAT_SHIFT }, /* 100Hz */
	{   ODR50,   20000000LL << READ_REPEAT_SHIFT }, /*  50Hz */
	{   ODR10,  100000000LL << READ_REPEAT_SHIFT }, /*  10Hz */
	{    ODR5,  200000000LL << READ_REPEAT_SHIFT }, /*   5Hz */
	{    ODR2,  500000000LL << READ_REPEAT_SHIFT }, /*   2Hz */
	{    ODR1, 1000000000LL << READ_REPEAT_SHIFT }, /*   1Hz */
	{ ODRHALF, 2000000000LL << READ_REPEAT_SHIFT }, /* 0.5Hz */
};

/* KR3DM acceleration data */
struct kr3dm_acc {
	s8 x;
	s8 y;
	s8 z;
};

struct kr3dm_data {
	struct i2c_client *client;
	struct miscdevice kr3dm_device;
	struct kr3dm_platform_data *pdata;
	int irq;
	u8 ctrl_reg1_shadow;
	struct completion data_ready;
	atomic_t opened; /* opened implies enabled */
	struct mutex read_lock;
	struct mutex write_lock;
};

static void kr3dm_disable_irq(struct kr3dm_data *kr3dm)
{
	disable_irq(kr3dm->irq);
	if (try_wait_for_completion(&kr3dm->data_ready)) {
		/* we actually got the interrupt before we could disable it
		 * so we need to enable again to undo our disable and the
		 * one done in the irq_handler
		 */
		enable_irq(kr3dm->irq);
	}
}

static irqreturn_t kr3dm_irq_handler(int irq, void *data)
{
	struct kr3dm_data *kr3dm = data;
	disable_irq_nosync(irq);
	complete(&kr3dm->data_ready);
	return IRQ_HANDLED;
}

static int kr3dm_wait_for_data_ready(struct kr3dm_data *kr3dm)
{
	int err;

	if (gpio_get_value(kr3dm->pdata->gpio_acc_int))
		return 0;

	enable_irq(kr3dm->irq);

	err = wait_for_completion_timeout(&kr3dm->data_ready, 5*HZ);
	if (err > 0)
		return 0;

	kr3dm_disable_irq(kr3dm);

	if (err == 0) {
		pr_err("kr3dm: wait timed out\n");
		return -ETIMEDOUT;
	}

	pr_err("kr3dm: wait restart\n");
	return err;
}

/* Read X,Y and Z-axis acceleration data.  Blocks until there is
 * something to read, based on interrupt from chip.
 */
static int kr3dm_read_accel_xyz(struct kr3dm_data *kr3dm,
				struct kr3dm_acc *acc)
{
	int err;
	s8 reg = OUT_X | AC; /* read from OUT_X to OUT_Z by auto-inc */
	s8 acc_data[5];

	err = kr3dm_wait_for_data_ready(kr3dm);
	if (err)
		return err;

	/* OUT_X, OUT_Y, and OUT_Z are single byte registers at
	 * address 0x29, 0x2B, and 0x2D respectively, with 1 dummy
	 * register in between.  Rather than doing 3 separate i2c reads,
	 * we do one multi-byte read and just use the bytes we want.
	 */
	err = i2c_smbus_read_i2c_block_data(kr3dm->client, reg,
					    sizeof(acc_data), acc_data);
	if (err != sizeof(acc_data)) {
		pr_err("%s : failed to read 5 bytes for getting x/y/z\n",
		       __func__);
		return -EIO;
	}

	if (kr3dm->pdata->rotation) {
		acc->x = acc_data[0] * kr3dm->pdata->rotation[0] +
				acc_data[2] * kr3dm->pdata->rotation[1] +
				acc_data[4] * kr3dm->pdata->rotation[2];
		acc->y = acc_data[0] * kr3dm->pdata->rotation[3] +
				acc_data[2] * kr3dm->pdata->rotation[4] +
				acc_data[4] * kr3dm->pdata->rotation[5];
		acc->z = acc_data[0] * kr3dm->pdata->rotation[6] +
				acc_data[2] * kr3dm->pdata->rotation[7] +
				acc_data[4] * kr3dm->pdata->rotation[8];
	} else {
		acc->x = acc_data[0];
		acc->y = acc_data[2];
		acc->z = acc_data[4];
	}

	return 0;
}

/*  open command for KR3DM device file  */
static int kr3dm_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct kr3dm_data *kr3dm = container_of(file->private_data,
						struct kr3dm_data,
						kr3dm_device);

	if (atomic_xchg(&kr3dm->opened, 1)) {
		pr_err("kr3dm_open() called when already open\n");
		return -EBUSY;
	} else {
		file->private_data = kr3dm;
		kr3dm->ctrl_reg1_shadow = DEFAULT_POWER_ON_SETTING;
		err = i2c_smbus_write_byte_data(kr3dm->client, CTRL_REG1,
						DEFAULT_POWER_ON_SETTING);
		if (err) {
			pr_err("kr3dm_open() i2c write ctrl_reg1 failed\n");
			atomic_set(&kr3dm->opened, 0);
		}
	}

	return err;
}

/*  release command for KR3DM device file */
static int kr3dm_close(struct inode *inode, struct file *file)
{
	int err;
	struct kr3dm_data *kr3dm = file->private_data;

	err = i2c_smbus_write_byte_data(kr3dm->client, CTRL_REG1, PM_OFF);
	atomic_set(&kr3dm->opened, 0);
	kr3dm->ctrl_reg1_shadow = PM_OFF;

	return err;
}

static s64 kr3dm_get_delay(struct kr3dm_data *kr3dm)
{
	int i;
	u8 odr;
	s64 delay = -1;

	odr = kr3dm->ctrl_reg1_shadow & ODR_MASK;
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (odr == odr_delay_table[i].odr) {
			delay = odr_delay_table[i].delay_ns;
			break;
		}
	}
	return delay;
}

static int kr3dm_set_delay(struct kr3dm_data *kr3dm, s64 delay_ns)
{
	int odr_value = ODRHALF;
	int res = 0;
	int i;
	/* round to the nearest delay that is less than
	 * the requested value (next highest freq)
	 */
	kr3dm_dbgmsg(" passed %lldns\n", delay_ns);
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (delay_ns < odr_delay_table[i].delay_ns)
			break;
	}
	if (i > 0)
		i--;
	kr3dm_dbgmsg("matched rate %lldns, odr = 0x%x\n",
			odr_delay_table[i].delay_ns,
			odr_delay_table[i].odr);
	odr_value = odr_delay_table[i].odr;
	delay_ns = odr_delay_table[i].delay_ns;
	mutex_lock(&kr3dm->write_lock);
	kr3dm_dbgmsg("old = %lldns, new = %lldns\n",
		     kr3dm_get_delay(kr3dm), delay_ns);
	if (odr_value != (kr3dm->ctrl_reg1_shadow & ODR_MASK)) {
		u8 ctrl = (kr3dm->ctrl_reg1_shadow & ~ODR_MASK);
		ctrl |= odr_value;
		kr3dm->ctrl_reg1_shadow = ctrl;
		res = i2c_smbus_write_byte_data(kr3dm->client, CTRL_REG1, ctrl);
		kr3dm_dbgmsg("writing odr value 0x%x\n", odr_value);
	}
	mutex_unlock(&kr3dm->write_lock);
	return res;
}

/*  ioctl command for KR3DM device file */
static int kr3dm_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct kr3dm_data *kr3dm = file->private_data;
	s64 delay_ns;
	struct kr3dm_acc data;
	int i;
	struct kr3dm_acceldata sum = { 0, };

	/* cmd mapping */
	switch (cmd) {
	case KR3DM_IOCTL_SET_DELAY:
		if (copy_from_user(&delay_ns, (void __user *)arg,
					sizeof(delay_ns)))
			return -EFAULT;
		err = kr3dm_set_delay(kr3dm, delay_ns);
		break;
	case KR3DM_IOCTL_GET_DELAY:
		delay_ns = kr3dm_get_delay(kr3dm);
		if (put_user(delay_ns, (s64 __user *)arg))
			return -EFAULT;
		break;
	case KR3DM_IOCTL_READ_ACCEL_XYZ:
		mutex_lock(&kr3dm->read_lock);
		for (i = 0; i < READ_REPEAT; i++) {
			err = kr3dm_read_accel_xyz(kr3dm, &data);
			if (err)
				break;
			sum.x += data.x;
			sum.y += data.y;
			sum.z += data.z;
		}
		mutex_unlock(&kr3dm->read_lock);
		if (err)
			return err;
		if (copy_to_user((void __user *)arg, &sum, sizeof(sum)))
			return -EFAULT;
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int kr3dm_suspend(struct device *dev)
{
	int res = 0;
	struct kr3dm_data *kr3dm  = dev_get_drvdata(dev);

	if (atomic_read(&kr3dm->opened))
		res = i2c_smbus_write_byte_data(kr3dm->client,
						CTRL_REG1, PM_OFF);

	return res;
}

static int kr3dm_resume(struct device *dev)
{
	int res = 0;
	struct kr3dm_data *kr3dm = dev_get_drvdata(dev);

	if (atomic_read(&kr3dm->opened))
		res = i2c_smbus_write_byte_data(kr3dm->client, CTRL_REG1,
						kr3dm->ctrl_reg1_shadow);

	return res;
}


static const struct dev_pm_ops kr3dm_pm_ops = {
	.suspend = kr3dm_suspend,
	.resume = kr3dm_resume,
};

static const struct file_operations kr3dm_fops = {
	.owner = THIS_MODULE,
	.open = kr3dm_open,
	.release = kr3dm_close,
	.ioctl = kr3dm_ioctl,
};

static int kr3dm_setup_irq(struct kr3dm_data *kr3dm)
{
	int rc = -EIO;
	struct kr3dm_platform_data *pdata = kr3dm->pdata;
	int irq;

	rc = gpio_request(pdata->gpio_acc_int, "gpio_acc_int");
	if (rc < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
			__func__, pdata->gpio_acc_int, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->gpio_acc_int);
	if (rc < 0) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
			__func__, pdata->gpio_acc_int, rc);
		goto err_gpio_direction_input;
	}

	/* configure INT1 to deliver data ready interrupt */
	rc = i2c_smbus_write_byte_data(kr3dm->client, CTRL_REG3, I1_CFG_DR);
	if (rc) {
		pr_err("%s: CTRL_REG3 write failed with error %d\n",
			__func__, rc);
		goto err_i2c_write_failed;
	}

	irq = gpio_to_irq(pdata->gpio_acc_int);

	/* trigger high so we don't miss initial interrupt if it
	 * is already pending
	 */
	rc = request_irq(irq, kr3dm_irq_handler, IRQF_TRIGGER_HIGH,
			 "acc_int", kr3dm);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			pdata->gpio_acc_int, rc);
		goto err_request_irq;
	}

	/* start with interrupt disabled until the driver is enabled */
	kr3dm->irq = irq;
	kr3dm_disable_irq(kr3dm);

	goto done;

err_request_irq:
err_i2c_write_failed:
err_gpio_direction_input:
	gpio_free(pdata->gpio_acc_int);
done:
	return rc;
}

static int kr3dm_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct kr3dm_data *kr3dm;
	int err;
	struct kr3dm_platform_data *pdata = client->dev.platform_data;

	if (!pdata) {
		pr_err("%s: missing pdata!\n", __func__);
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
				     I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		err = -ENODEV;
		goto exit;
	}

	kr3dm = kzalloc(sizeof(struct kr3dm_data), GFP_KERNEL);
	if (kr3dm == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	kr3dm->client = client;
	kr3dm->pdata = pdata;
	i2c_set_clientdata(client, kr3dm);

	init_completion(&kr3dm->data_ready);
	mutex_init(&kr3dm->read_lock);
	mutex_init(&kr3dm->write_lock);
	atomic_set(&kr3dm->opened, 0);

	err = kr3dm_setup_irq(kr3dm);
	if (err) {
		pr_err("%s: could not setup irq\n", __func__);
		goto err_setup_irq;
	}

	/* sensor HAL expects to find /dev/accelerometer */
	kr3dm->kr3dm_device.minor = MISC_DYNAMIC_MINOR;
	kr3dm->kr3dm_device.name = "accelerometer";
	kr3dm->kr3dm_device.fops = &kr3dm_fops;

	err = misc_register(&kr3dm->kr3dm_device);
	if (err) {
		pr_err("%s: misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	return 0;

err_misc_register:
	free_irq(kr3dm->irq, kr3dm);
	gpio_free(kr3dm->pdata->gpio_acc_int);
err_setup_irq:
	mutex_destroy(&kr3dm->read_lock);
	mutex_destroy(&kr3dm->write_lock);
	kfree(kr3dm);
exit:
	return err;
}

static int kr3dm_remove(struct i2c_client *client)
{
	struct kr3dm_data *kr3dm = i2c_get_clientdata(client);

	misc_deregister(&kr3dm->kr3dm_device);
	free_irq(kr3dm->irq, kr3dm);
	gpio_free(kr3dm->pdata->gpio_acc_int);
	mutex_destroy(&kr3dm->read_lock);
	mutex_destroy(&kr3dm->write_lock);
	kfree(kr3dm);

	return 0;
}

static const struct i2c_device_id kr3dm_id[] = {
	{ "kr3dm", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, kr3dm_id);

static struct i2c_driver kr3dm_driver = {
	.probe = kr3dm_probe,
	.remove = __devexit_p(kr3dm_remove),
	.id_table = kr3dm_id,
	.driver = {
		.pm = &kr3dm_pm_ops,
		.owner = THIS_MODULE,
		.name = "kr3dm",
	},
};

static int __init kr3dm_init(void)
{
	return i2c_add_driver(&kr3dm_driver);
}

static void __exit kr3dm_exit(void)
{
	i2c_del_driver(&kr3dm_driver);
}

module_init(kr3dm_init);
module_exit(kr3dm_exit);

MODULE_DESCRIPTION("kr3dm accelerometer driver");
MODULE_AUTHOR("Tim SK Lee Samsung Electronics <tim.sk.lee@samsung.com>");
MODULE_LICENSE("GPL");
