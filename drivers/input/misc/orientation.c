/*
 * Copyright (c) 2010 Yamaha Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#define SENSOR_NAME "orientation_sensor"
#define SENSOR_DEFAULT_DELAY            (200)
#define REL_STATUS                      (REL_RX)
#define REL_WAKE                        (REL_RY)
#define REL_CONTROL_REPORT              (REL_RZ)

struct orientation_data {
	struct mutex mutex;
	int enabled;
	int delay;
};

static struct platform_device *orientation_pdev;
static struct input_dev *this_data;

/* Sysfs interface */
static ssize_t
orientation_delay_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct orientation_data *data = input_get_drvdata(input_data);
	int delay;

	mutex_lock(&data->mutex);
	delay = data->delay;
	mutex_unlock(&data->mutex);

	return sprintf(buf, "%d\n", delay);
}

static ssize_t
orientation_delay_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct orientation_data *data = input_get_drvdata(input_data);
	long value;
	int err;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0 || value < 0)
		return count;

	if (SENSOR_DEFAULT_DELAY < value)
		value = SENSOR_DEFAULT_DELAY;

	mutex_lock(&data->mutex);
	data->delay = value;
	input_report_rel(input_data, REL_CONTROL_REPORT,
				(data->enabled << 16) | value);
	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t
orientation_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct orientation_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", data->enabled);
}

static ssize_t
orientation_enable_store(struct device *dev, struct device_attribute *attr,
						const char *buf,  size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct orientation_data *data = input_get_drvdata(input_data);
	long value;
	int err;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;

	if (value != 0 && value != 1)
		return count;

	mutex_lock(&data->mutex);
	data->enabled = value;
	input_report_rel(input_data, REL_CONTROL_REPORT,
				(value << 16) | data->delay);
	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t
orientation_wake_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	static int cnt = 1;

	input_report_rel(input_data, REL_WAKE, cnt++);

	return count;
}

static ssize_t
orientation_data_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	unsigned long flags;
	int x, y, z;

	spin_lock_irqsave(&input_data->event_lock, flags);

	x = input_data->abs[REL_X];
	y = input_data->abs[REL_Y];
	z = input_data->abs[REL_Z];

	spin_unlock_irqrestore(&input_data->event_lock, flags);

	return sprintf(buf, "%d %d %d\n", x, y, z);
}

static ssize_t
orientation_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	unsigned long flags;
	int status;

	spin_lock_irqsave(&input_data->event_lock, flags);

	status = input_data->abs[REL_STATUS];

	spin_unlock_irqrestore(&input_data->event_lock, flags);

	return sprintf(buf, "%d\n", status);
}

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
			orientation_delay_show, orientation_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
			orientation_enable_show, orientation_enable_store);
static DEVICE_ATTR(wake, S_IWUSR|S_IWGRP,
			NULL, orientation_wake_store);
static DEVICE_ATTR(data, S_IRUGO, orientation_data_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, orientation_status_show, NULL);

static struct attribute *orientation_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_wake.attr,
	&dev_attr_data.attr,
	&dev_attr_status.attr,
	NULL
};

static struct attribute_group orientation_attribute_group = {
	.attrs = orientation_attributes
};

static int
orientation_probe(struct platform_device *pdev)
{
	struct orientation_data *data = NULL;
	struct input_dev *input_data = NULL;
	int err;

	data = kzalloc(sizeof(struct orientation_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto err;
	}

	data->enabled = 0;
	data->delay = SENSOR_DEFAULT_DELAY;

	input_data = input_allocate_device();
	if (!input_data) {
		err = -ENOMEM;
		pr_err("%s: Failed to allocate input_data device\n", __func__);
		goto err_input_allocate_device;
	}

	set_bit(EV_REL, input_data->evbit);
	input_set_capability(input_data, EV_REL, REL_X);
	input_set_capability(input_data, EV_REL, REL_Y);
	input_set_capability(input_data, EV_REL, REL_Z);
	/* sattus */
	input_set_capability(input_data, EV_REL, REL_STATUS);
	/* wake */
	input_set_capability(input_data, EV_REL, REL_WAKE);
	/* enabled/delay */
	input_set_capability(input_data, EV_REL, REL_CONTROL_REPORT);
	input_data->name = SENSOR_NAME;

	err = input_register_device(input_data);
	if (err) {
		pr_err("%s: Unable to register input_data device: %s\n",
					__func__, input_data->name);
		input_free_device(input_data);
		goto err_input_register_device;
	}
	input_set_drvdata(input_data, data);

	err = sysfs_create_group(&input_data->dev.kobj,
				&orientation_attribute_group);
	if (err) {
		pr_err("%s: sysfs_create_group failed(%s)\n",
				__func__, input_data->name);
		goto err_sys_create_group;
	}
	mutex_init(&data->mutex);
	this_data = input_data;

	return 0;

err_sys_create_group:
	input_unregister_device(input_data);
err_input_register_device:
err_input_allocate_device:
	kfree(data);
err:
	return err;
}

static int
orientation_remove(struct platform_device *pdev)
{
	struct orientation_data *data;

	data = input_get_drvdata(this_data);
	sysfs_remove_group(&this_data->dev.kobj, &orientation_attribute_group);
	input_unregister_device(this_data);
	kfree(data);

	return 0;
}

static struct platform_driver orientation_driver = {
	.probe = orientation_probe,
	.remove = orientation_remove,
	.driver = {
		.name = SENSOR_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init orientation_init(void)
{
	orientation_pdev
		= platform_device_register_simple(SENSOR_NAME, 0, NULL, 0);
	if (IS_ERR(orientation_pdev))
		return -1;
	return platform_driver_register(&orientation_driver);
}
module_init(orientation_init);

static void __exit orientation_exit(void)
{
	platform_driver_unregister(&orientation_driver);
	platform_device_unregister(orientation_pdev);
}
module_exit(orientation_exit);

MODULE_AUTHOR("Yamaha Corporation");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
