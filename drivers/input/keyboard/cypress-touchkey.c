/*
 * Copyright 2006-2010, Cypress Semiconductor Corporation.
 * Copyright (C) 2010, Samsung Electronics Co. Ltd. All Rights Reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/input/cypress-touchkey.h>
#include <linux/firmware.h>

#define SCANCODE_MASK		0x07
#define UPDOWN_EVENT_MASK	0x08
#define ESD_STATE_MASK		0x10

#define BACKLIGHT_ON		0x10
#define BACKLIGHT_OFF		0x20

#define OLD_BACKLIGHT_ON	0x1
#define OLD_BACKLIGHT_OFF	0x2

#define DEVICE_NAME "cypress-touchkey"

#define FW_SIZE 8192

struct cypress_touchkey_devdata {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct touchkey_platform_data *pdata;
	struct early_suspend early_suspend;
	u8 backlight_on;
	u8 backlight_off;
	bool is_dead;
	bool is_powering_on;
	bool has_legacy_keycode;
};

static int i2c_touchkey_read_byte(struct cypress_touchkey_devdata *devdata,
					u8 *val)
{
	int ret;
	int retry = 5;

	while (true) {
		ret = i2c_smbus_read_byte(devdata->client);
		if (ret >= 0) {
			*val = ret;
			return 0;
		}

		dev_err(&devdata->client->dev, "i2c read error\n");
		if (!retry--)
			break;
		msleep(10);
	}

	return ret;
}

static int i2c_touchkey_write_byte(struct cypress_touchkey_devdata *devdata,
					u8 val)
{
	int ret;
	int retry = 2;

	while (true) {
		ret = i2c_smbus_write_byte(devdata->client, val);
		if (!ret)
			return 0;

		dev_err(&devdata->client->dev, "i2c write error\n");
		if (!retry--)
			break;
		msleep(10);
	}

	return ret;
}

static void all_keys_up(struct cypress_touchkey_devdata *devdata)
{
	int i;

	for (i = 0; i < devdata->pdata->keycode_cnt; i++)
		input_report_key(devdata->input_dev,
						devdata->pdata->keycode[i], 0);

	input_sync(devdata->input_dev);
}

static int recovery_routine(struct cypress_touchkey_devdata *devdata)
{
	int ret = -1;
	int retry = 10;
	u8 data;
	int irq_eint;

	if (unlikely(devdata->is_dead)) {
		dev_err(&devdata->client->dev, "%s: Device is already dead, "
				"skipping recovery\n", __func__);
		return -ENODEV;
	}

	irq_eint = devdata->client->irq;

	all_keys_up(devdata);

	disable_irq_nosync(irq_eint);
	while (retry--) {
		devdata->pdata->touchkey_onoff(TOUCHKEY_OFF);
		devdata->pdata->touchkey_onoff(TOUCHKEY_ON);
		ret = i2c_touchkey_read_byte(devdata, &data);
		if (!ret) {
			enable_irq(irq_eint);
			goto out;
		}
		dev_err(&devdata->client->dev, "%s: i2c transfer error retry = "
				"%d\n", __func__, retry);
	}
	devdata->is_dead = true;
	devdata->pdata->touchkey_onoff(TOUCHKEY_OFF);
	dev_err(&devdata->client->dev, "%s: touchkey died\n", __func__);
out:
	return ret;
}

static irqreturn_t touchkey_interrupt_thread(int irq, void *touchkey_devdata)
{
	u8 data;
	int i;
	int ret;
	int scancode;
	struct cypress_touchkey_devdata *devdata = touchkey_devdata;

	ret = i2c_touchkey_read_byte(devdata, &data);
	if (ret || (data & ESD_STATE_MASK)) {
		ret = recovery_routine(devdata);
		if (ret) {
			dev_err(&devdata->client->dev, "%s: touchkey recovery "
					"failed!\n", __func__);
			goto err;
		}
	}

	if (devdata->has_legacy_keycode) {
		scancode = (data & SCANCODE_MASK) - 1;
		if (scancode < 0 || scancode >= devdata->pdata->keycode_cnt) {
			dev_err(&devdata->client->dev, "%s: scancode is out of "
				"range\n", __func__);
			goto err;
		}
		input_report_key(devdata->input_dev,
			devdata->pdata->keycode[scancode],
			!(data & UPDOWN_EVENT_MASK));
	} else {
		for (i = 0; i < devdata->pdata->keycode_cnt; i++)
			input_report_key(devdata->input_dev,
				devdata->pdata->keycode[i],
				!!(data & (1U << i)));
	}

	input_sync(devdata->input_dev);
err:
	return IRQ_HANDLED;
}

static irqreturn_t touchkey_interrupt_handler(int irq, void *touchkey_devdata)
{
	struct cypress_touchkey_devdata *devdata = touchkey_devdata;

	if (devdata->is_powering_on) {
		dev_dbg(&devdata->client->dev, "%s: ignoring spurious boot "
					"interrupt\n", __func__);
		return IRQ_HANDLED;
	}

	return IRQ_WAKE_THREAD;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress_touchkey_early_suspend(struct early_suspend *h)
{
	struct cypress_touchkey_devdata *devdata =
		container_of(h, struct cypress_touchkey_devdata, early_suspend);

	devdata->is_powering_on = true;

	if (unlikely(devdata->is_dead))
		return;

	disable_irq(devdata->client->irq);
	devdata->pdata->touchkey_onoff(TOUCHKEY_OFF);

	all_keys_up(devdata);
}

static void cypress_touchkey_early_resume(struct early_suspend *h)
{
	struct cypress_touchkey_devdata *devdata =
		container_of(h, struct cypress_touchkey_devdata, early_suspend);

	devdata->pdata->touchkey_onoff(TOUCHKEY_ON);
	if (i2c_touchkey_write_byte(devdata, devdata->backlight_on)) {
		devdata->is_dead = true;
		devdata->pdata->touchkey_onoff(TOUCHKEY_OFF);
		dev_err(&devdata->client->dev, "%s: touch keypad not responding"
				" to commands, disabling\n", __func__);
		return;
	}
	devdata->is_dead = false;
	enable_irq(devdata->client->irq);
	devdata->is_powering_on = false;
}
#endif

static void set_device_params(struct cypress_touchkey_devdata *devdata,
								u8 *data)
{
	if (data[1] < 0xc4 && (data[1] > 0x8 ||
				(data[1] == 0x8 && data[2] >= 0x9))) {
		devdata->backlight_on = BACKLIGHT_ON;
		devdata->backlight_off = BACKLIGHT_OFF;
	} else {
		devdata->backlight_on = OLD_BACKLIGHT_ON;
		devdata->backlight_off = OLD_BACKLIGHT_OFF;
	}

	devdata->has_legacy_keycode = data[1] >= 0xc4 || data[1] < 0x9 ||
					(data[1] == 0x9 && data[2] < 0x9);
}

static int update_firmware(struct cypress_touchkey_devdata *devdata)
{
	int ret;
	const struct firmware *fw = NULL;
	struct device *dev = &devdata->input_dev->dev;
	int retries = 10;

	dev_info(dev, "%s: Updating " DEVICE_NAME " firmware\n", __func__);

	if (!devdata->pdata->fw_name) {
		dev_err(dev, "%s: Device firmware name is not set\n", __func__);
		return -EINVAL;
	}

	ret = request_firmware(&fw, devdata->pdata->fw_name, dev);
	if (ret) {
		dev_err(dev, "%s: Can't open firmware file from %s\n", __func__,
						devdata->pdata->fw_name);
		return ret;
	}

	if (fw->size != FW_SIZE) {
		dev_err(dev, "%s: Firmware file size invalid\n", __func__);
		return -EINVAL;
	}

	disable_irq(devdata->client->irq);
	/* Lock the i2c bus since the firmware updater accesses it */
	i2c_lock_adapter(devdata->client->adapter);
	while (retries--) {
		ret = touchkey_flash_firmware(devdata->pdata, fw->data);
		if (!ret)
			break;
	}
	if (ret)
		dev_err(dev, "%s: Firmware update failed\n", __func__);
	i2c_unlock_adapter(devdata->client->adapter);
	enable_irq(devdata->client->irq);

	release_firmware(fw);
	return ret;
}

static int cypress_touchkey_open(struct input_dev *input_dev)
{
	struct device *dev = &input_dev->dev;
	struct cypress_touchkey_devdata *devdata = dev_get_drvdata(dev);
	u8 data[3];
	int ret;

	ret = update_firmware(devdata);
	if (ret)
		goto done;

	ret = i2c_master_recv(devdata->client, data, sizeof(data));
	if (ret < sizeof(data)) {
		if (ret >= 0)
			ret = -EIO;
		dev_err(dev, "%s: error reading hardware version\n", __func__);
		goto done;
	}

	dev_info(dev, "%s: hardware rev1 = %#02x, rev2 = %#02x\n", __func__,
				data[1], data[2]);
	set_device_params(devdata, data);

	devdata->pdata->touchkey_onoff(TOUCHKEY_OFF);
	devdata->pdata->touchkey_onoff(TOUCHKEY_ON);

	ret = i2c_touchkey_write_byte(devdata, devdata->backlight_on);
	if (ret) {
		dev_err(dev, "%s: touch keypad backlight on failed\n",
				__func__);
		goto done;
	}

done:
	input_dev->open = NULL;
	return 0;
}

static int cypress_touchkey_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct input_dev *input_dev;
	struct cypress_touchkey_devdata *devdata;
	u8 data[3];
	int err;
	int cnt;

	if (!dev->platform_data) {
		dev_err(dev, "%s: Platform data is NULL\n", __func__);
		return -EINVAL;
	}

	devdata = kzalloc(sizeof(*devdata), GFP_KERNEL);
	if (devdata == NULL) {
		dev_err(dev, "%s: failed to create our state\n", __func__);
		return -ENODEV;
	}

	devdata->client = client;
	i2c_set_clientdata(client, devdata);

	devdata->pdata = client->dev.platform_data;
	if (!devdata->pdata->keycode) {
		dev_err(dev, "%s: Invalid platform data\n", __func__);
		err = -EINVAL;
		goto err_null_keycodes;
	}

	strlcpy(devdata->client->name, DEVICE_NAME, I2C_NAME_SIZE);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		goto err_input_alloc_dev;
	}

	devdata->input_dev = input_dev;
	dev_set_drvdata(&input_dev->dev, devdata);
	input_dev->name = DEVICE_NAME;
	input_dev->id.bustype = BUS_HOST;

	for (cnt = 0; cnt < devdata->pdata->keycode_cnt; cnt++)
		input_set_capability(input_dev, EV_KEY,
					devdata->pdata->keycode[cnt]);

	devdata->is_powering_on = true;

	devdata->pdata->touchkey_onoff(TOUCHKEY_ON);

	err = i2c_master_recv(client, data, sizeof(data));
	if (err < sizeof(data)) {
		if (err >= 0)
			err = -EIO;
		dev_err(dev, "%s: error reading hardware version\n", __func__);
		goto err_read;
	}

	dev_info(dev, "%s: hardware rev1 = %#02x, rev2 = %#02x\n", __func__,
				data[1], data[2]);

	if (data[1] != 0xa || data[2] < 0x9)
		input_dev->open = cypress_touchkey_open;

	err = input_register_device(input_dev);
	if (err)
		goto err_input_reg_dev;

	set_device_params(devdata, data);

	err = i2c_touchkey_write_byte(devdata, devdata->backlight_on);
	if (err) {
		dev_err(dev, "%s: touch keypad backlight on failed\n",
				__func__);
		/* The device may not be responding because of bad firmware
		 * Allow the firmware to be reflashed if it needs to be
		 */
		if (!input_dev->open)
			goto err_backlight_on;
	}

	err = request_threaded_irq(client->irq, touchkey_interrupt_handler,
				touchkey_interrupt_thread, IRQF_TRIGGER_FALLING,
				DEVICE_NAME, devdata);
	if (err) {
		dev_err(dev, "%s: Can't allocate irq.\n", __func__);
		goto err_req_irq;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	devdata->early_suspend.suspend = cypress_touchkey_early_suspend;
	devdata->early_suspend.resume = cypress_touchkey_early_resume;
#endif
	register_early_suspend(&devdata->early_suspend);

	devdata->is_powering_on = false;

	return 0;

err_req_irq:
err_backlight_on:
	input_unregister_device(input_dev);
	goto touchkey_off;
err_input_reg_dev:
err_read:
	input_free_device(input_dev);
touchkey_off:
	devdata->pdata->touchkey_onoff(TOUCHKEY_OFF);
err_input_alloc_dev:
err_null_keycodes:
	kfree(devdata);
	return err;
}

static int __devexit i2c_touchkey_remove(struct i2c_client *client)
{
	struct cypress_touchkey_devdata *devdata = i2c_get_clientdata(client);

	unregister_early_suspend(&devdata->early_suspend);
	/* If the device is dead IRQs are disabled, we need to rebalance them */
	if (unlikely(devdata->is_dead))
		enable_irq(client->irq);
	else
		devdata->pdata->touchkey_onoff(TOUCHKEY_OFF);
	free_irq(client->irq, devdata);
	all_keys_up(devdata);
	input_unregister_device(devdata->input_dev);
	kfree(devdata);
	return 0;
}

static const struct i2c_device_id cypress_touchkey_id[] = {
	{ CYPRESS_TOUCHKEY_DEV_NAME, 0 },
};

MODULE_DEVICE_TABLE(i2c, cypress_touchkey_id);

struct i2c_driver touchkey_i2c_driver = {
	.driver = {
		.name = "cypress_touchkey_driver",
	},
	.id_table = cypress_touchkey_id,
	.probe = cypress_touchkey_probe,
	.remove = __devexit_p(i2c_touchkey_remove),
};

static int __init touchkey_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&touchkey_i2c_driver);
	if (ret)
		pr_err("%s: cypress touch keypad registration failed. (%d)\n",
				__func__, ret);

	return ret;
}

static void __exit touchkey_exit(void)
{
	i2c_del_driver(&touchkey_i2c_driver);
}

late_initcall(touchkey_init);
module_exit(touchkey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("@@@");
MODULE_DESCRIPTION("cypress touch keypad");
