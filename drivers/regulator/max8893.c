/*
 * max8893.c
 *
 * based on max8660.c
 *
 * Copyright (C) 2009 Wolfram Sang, Pengutronix e.K.
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/max8893.h>
#include <linux/slab.h>

#define MAX8893_STEP		(100000)

enum {
	MAX8893_ONOFF		= 0x00,
	MAX8893_VOLTAGE_BASE	= 0x04,
};

struct max8893 {
	struct i2c_client *client;
	struct mutex lock;
	struct regulator_dev *rdev[];
};

struct max8893_desc {
	struct regulator_desc desc;
	int min_uV;
	u8 mask;
};

static struct max8893_desc *rdev_get_max8893_desc(struct regulator_dev *rdev)
{
	return container_of(rdev->desc, struct max8893_desc, desc);
}

static int max8893_onoff(struct regulator_dev *rdev, bool enable)
{
	struct max8893 *max8893 = rdev_get_drvdata(rdev);
	struct max8893_desc *desc = rdev_get_max8893_desc(rdev);
	int ret;
	u8 val;

	mutex_lock(&max8893->lock);
	ret = i2c_smbus_read_byte_data(max8893->client, MAX8893_ONOFF);
	if (ret < 0) {
		dev_err(&rdev->dev, "%s: i2c read failed\n", __func__);
		goto err;
	}
	val = ret;

	if (enable)
		val |= desc->mask;
	else
		val &= ~desc->mask;

	ret = i2c_smbus_write_byte_data(max8893->client, MAX8893_ONOFF, val);
	if (ret) {
		dev_err(&rdev->dev, "%s: i2c write failed\n", __func__);
		goto err;
	}
	mutex_unlock(&max8893->lock);
err:
	return ret;
}

static int max8893_enable(struct regulator_dev *rdev)
{
	return max8893_onoff(rdev, true);
}

static int max8893_disable(struct regulator_dev *rdev)
{
	return max8893_onoff(rdev, false);
}

static int max8893_is_enabled(struct regulator_dev *rdev)
{
	struct max8893 *max8893 = rdev_get_drvdata(rdev);
	struct max8893_desc *desc = rdev_get_max8893_desc(rdev);
	int val;

	val = i2c_smbus_read_byte_data(max8893->client, MAX8893_ONOFF);
	if (val < 0) {
		dev_err(&rdev->dev, "%s: i2c read failed\n", __func__);
		return val;
	}

	return !!(val & desc->mask);
}

static int max8893_list(struct regulator_dev *rdev, unsigned selector)
{
	struct max8893_desc *desc = rdev_get_max8893_desc(rdev);
	if (selector >= desc->desc.n_voltages)
		return -EINVAL;
	return desc->min_uV + selector * MAX8893_STEP;
}

static int max8893_set(struct regulator_dev *rdev, int min_uV, int max_uV)
{
	struct max8893 *max8893 = rdev_get_drvdata(rdev);
	struct max8893_desc *desc = rdev_get_max8893_desc(rdev);
	int id = rdev_get_id(rdev);
	int rmax_uV = desc->min_uV + (desc->desc.n_voltages - 1) * MAX8893_STEP;
	u8 selector;
	int ret;

	if (min_uV > rmax_uV)
		return -EINVAL;
	if (max_uV < desc->min_uV)
		return -EINVAL;

	if (min_uV < desc->min_uV)
		min_uV = desc->min_uV;
	if (max_uV > rmax_uV)
		max_uV = rmax_uV;

	selector = DIV_ROUND_UP(min_uV - desc->min_uV, MAX8893_STEP);

	ret = max8893_list(rdev, selector);
	if (ret < 0 || ret > max_uV)
		return -EINVAL;

	ret = i2c_smbus_write_byte_data(max8893->client,
					MAX8893_VOLTAGE_BASE + id, selector);
	if (ret) {
		dev_err(&rdev->dev, "%s: i2c write failed\n", __func__);
		return ret;
	}

	return 0;
}

static int max8893_get(struct regulator_dev *rdev)
{
	struct max8893 *max8893 = rdev_get_drvdata(rdev);
	struct max8893_desc *desc = rdev_get_max8893_desc(rdev);
	int id = rdev_get_id(rdev);
	int selector;

	selector = i2c_smbus_read_byte_data(max8893->client,
						MAX8893_VOLTAGE_BASE + id);
	if (selector < 0) {
		dev_err(&rdev->dev, "%s: i2c read failed\n", __func__);
		return selector;
	}
	return desc->min_uV + selector * MAX8893_STEP;
}

static struct regulator_ops max8893_ops = {
	.enable = max8893_enable,
	.disable = max8893_disable,
	.is_enabled = max8893_is_enabled,
	.list_voltage = max8893_list,
	.set_voltage = max8893_set,
	.get_voltage = max8893_get,
};

static struct max8893_desc max8893_reg[] = {
	{
		.desc = {
			.name = "BUCK",
			.id = MAX8893_BUCK,
			.ops = &max8893_ops,
			.type = REGULATOR_VOLTAGE,
			.n_voltages = 0x10 + 1,
			.owner = THIS_MODULE,
		},
		.mask = BIT(7),
		.min_uV = 800000,
	},
	{
		.desc = {
			.name = "LDO1",
			.id = MAX8893_LDO1,
			.ops = &max8893_ops,
			.type = REGULATOR_VOLTAGE,
			.n_voltages = 0x11 + 1,
			.owner = THIS_MODULE,
		},
		.mask = BIT(5),
		.min_uV = 1600000,
	},
	{
		.desc = {
			.name = "LDO2",
			.id = MAX8893_LDO2,
			.ops = &max8893_ops,
			.type = REGULATOR_VOLTAGE,
			.n_voltages = 0x15 + 1,
			.owner = THIS_MODULE,
		},
		.mask = BIT(4),
		.min_uV = 1200000,
	},
	{
		.desc = {
			.name = "LDO3",
			.id = MAX8893_LDO3,
			.ops = &max8893_ops,
			.type = REGULATOR_VOLTAGE,
			.n_voltages = 0x11 + 1,
			.owner = THIS_MODULE,
		},
		.mask = BIT(3),
		.min_uV = 1600000,
	},
	{
		.desc = {
			.name = "LDO4",
			.id = MAX8893_LDO4,
			.ops = &max8893_ops,
			.type = REGULATOR_VOLTAGE,
			.n_voltages = 0x19 + 1,
			.owner = THIS_MODULE,
		},
		.mask = BIT(2),
		.min_uV = 800000,
	},
	{
		.desc = {
			.name = "LDO5",
			.id = MAX8893_LDO5,
			.ops = &max8893_ops,
			.type = REGULATOR_VOLTAGE,
			.n_voltages = 0x19 + 1,
			.owner = THIS_MODULE,
		},
		.mask = BIT(1),
		.min_uV = 800000,
	},
};


static int __devinit max8893_probe(struct i2c_client *client,
				   const struct i2c_device_id *i2c_id)
{
	struct regulator_dev **rdev;
	struct max8893_platform_data *pdata = client->dev.platform_data;
	struct max8893 *max8893;
	int i, id, ret = -EINVAL;

	if (pdata->num_subdevs > MAX8893_END) {
		dev_err(&client->dev, "Too many regulators found!\n");
		goto out;
	}

	max8893 = kzalloc(sizeof(struct max8893) +
			sizeof(struct regulator_dev *) * MAX8893_END,
			GFP_KERNEL);
	if (!max8893) {
		ret = -ENOMEM;
		goto out;
	}

	mutex_init(&max8893->lock);
	max8893->client = client;
	rdev = max8893->rdev;

	/* Finally register devices */
	for (i = 0; i < pdata->num_subdevs; i++) {

		id = pdata->subdevs[i].id;

		rdev[i] = regulator_register(&max8893_reg[id].desc,
					     &client->dev,
					     pdata->subdevs[i].initdata,
					     max8893);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(&client->dev, "failed to register %s\n",
				max8893_reg[id].desc.name);
			goto err_unregister;
		}
	}

	i2c_set_clientdata(client, max8893);
	dev_info(&client->dev, "Maxim 8893 regulator driver loaded\n");
	return 0;

err_unregister:
	while (--i >= 0)
		regulator_unregister(rdev[i]);

	kfree(max8893);
out:
	return ret;
}

static int __devexit max8893_remove(struct i2c_client *client)
{
	struct max8893 *max8893 = i2c_get_clientdata(client);
	struct regulator_dev **rdev = max8893->rdev;
	int i;

	for (i = 0; i < MAX8893_END; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
	mutex_destroy(&max8893->lock);
	kfree(max8893);

	return 0;
}

static const struct i2c_device_id max8893_id[] = {
	{ "max8893", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max8893_id);

static struct i2c_driver max8893_driver = {
	.probe = max8893_probe,
	.remove = __devexit_p(max8893_remove),
	.driver		= {
		.name	= "max8893",
		.owner	= THIS_MODULE,
	},
	.id_table	= max8893_id,
};

static int __init max8893_init(void)
{
	return i2c_add_driver(&max8893_driver);
}
subsys_initcall(max8893_init);

static void __exit max8893_exit(void)
{
	i2c_del_driver(&max8893_driver);
}
module_exit(max8893_exit);

