/*
 * linux/drivers/power/s3c6410_battery.c
 *
 * Battery measurement code for S3C6410 platform.
 *
 * based on palmtx_battery.c
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <asm/mach-types.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mfd/max8998.h>
#include <linux/mfd/max8998-private.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <mach/battery.h>
#include <mach/gpio-herring.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>
#include <mach/adc.h>
#include <plat/gpio-cfg.h>
#include "s5pc110_battery.h"

#define POLLING_INTERVAL	1000
#define ADC_TOTAL_COUNT		10
#define ADC_DATA_ARR_SIZE	6

#define OFFSET_VIBRATOR_ON		(0x1 << 0)
#define OFFSET_CAMERA_ON		(0x1 << 1)
#define OFFSET_MP3_PLAY			(0x1 << 2)
#define OFFSET_VIDEO_PLAY		(0x1 << 3)
#define OFFSET_VOICE_CALL_2G		(0x1 << 4)
#define OFFSET_VOICE_CALL_3G		(0x1 << 5)
#define OFFSET_DATA_CALL		(0x1 << 6)
#define OFFSET_LCD_ON			(0x1 << 7)
#define OFFSET_TA_ATTACHED		(0x1 << 8)
#define OFFSET_CAM_FLASH		(0x1 << 9)
#define OFFSET_BOOTING			(0x1 << 10)

struct battery_info {
	u32 batt_temp;		/* Battery Temperature (C) from ADC */
	u32 batt_temp_adc;	/* Battery Temperature ADC value */
	u32 charging_source:2;	/* 0: no cable, 1:usb, 2:AC */
	int charging_enabled;	/* 0: Disable, 1: Enable */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 batt_is_full;	/* 0 : Not full 1: Full */
	u32 batt_is_recharging; /* 0 : Not recharging 1: Recharging */
	u32 charging_status;
	int charger_online:1;
};

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_TOTAL_COUNT];
	int index;
};

struct chg_data {
	struct device		*dev;
	struct max8998_dev	*iodev;
	struct delayed_work	bat_work;
	struct max8998_charger_data *pdata;

	struct power_supply	psy_bat;
	struct power_supply	psy_usb;
	struct power_supply	psy_ac;

	struct wake_lock	vbus_wake_lock;
	struct adc_sample_info	adc_sample[ENDOFADC];
	struct battery_info	bat_info;
	struct mutex		mutex;

	charging_device_type	current_device_type;
	enum charger_type_t	cable_status;
	int			force_update;
	int			present;
	unsigned int		polling_interval;
	unsigned int		device_state;
};

static char *supply_list[] = {
	"battery",
};

static enum power_supply_property max8998_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property s3c_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static void s3c_bat_update_charging_status(struct chg_data *chg)
{
	enum charger_type_t charger;

	charger = chg->bat_info.charging_source;

	switch (charger) {
	case CHARGER_BATTERY:
		chg->bat_info.charging_status =
			POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case CHARGER_USB:
	case CHARGER_AC:
		chg->bat_info.charging_status =
			chg->bat_info.batt_is_full ?
			POWER_SUPPLY_STATUS_FULL : POWER_SUPPLY_STATUS_CHARGING;
		break;
	case CHARGER_DISCHARGE:
		chg->bat_info.charging_status =
			POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	default:
		chg->bat_info.charging_status =
			POWER_SUPPLY_STATUS_UNKNOWN;
	}

}

static int s3c_bat_get_property(struct power_supply *bat_ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct chg_data *chg = container_of(bat_ps,
				struct chg_data, psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chg->bat_info.charging_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = chg->bat_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = chg->present;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = chg->bat_info.batt_temp;
		pr_info("psp = %d\n", chg->bat_info.batt_temp);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* battery is always online */
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_CAPACITY:
		if (chg->pdata && chg->pdata->psy_fuelgauge &&
			chg->pdata->psy_fuelgauge->get_property &&
			chg->pdata->psy_fuelgauge->get_property(chg->pdata->psy_fuelgauge,
				psp, (union power_supply_propval *)&val->intval) < 0)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int s3c_usb_get_property(struct power_supply *ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct chg_data *chg = container_of(ps, struct chg_data, psy_usb);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the USB charger is in use */
	val->intval = (chg->cable_status == CHARGER_USB);

	return 0;
}

static int s3c_ac_get_property(struct power_supply *ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct chg_data *chg = container_of(ps, struct chg_data, psy_ac);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the AC charger is in use */
	val->intval = (chg->cable_status == CHARGER_AC);

	return 0;
}

static int s3c_bat_get_adc_data(enum adc_channel_type adc_ch)
{
	int adc_arr[ADC_DATA_ARR_SIZE];
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;

	for (i = 0; i < ADC_DATA_ARR_SIZE; i++) {
		adc_arr[i] = s3c_adc_get_adc_data(adc_ch);
		if (i != 0) {
			if (adc_arr[i] > adc_max)
				adc_max = adc_arr[i];
			else if (adc_arr[i] < adc_min)
				adc_min = adc_arr[i];
		} else {
			adc_max = adc_arr[0];
			adc_min = adc_arr[0];
		}
		adc_total += adc_arr[i];
	}

	return (adc_total - adc_max - adc_min) / (ADC_DATA_ARR_SIZE - 2);
}

static unsigned long calculate_average_adc(enum adc_channel_type channel,
			int adc, struct chg_data *chg)
{
	unsigned int cnt = 0;
	int total_adc = 0;
	int average_adc = 0;
	int index = 0;

	cnt = chg->adc_sample[channel].cnt;
	total_adc = chg->adc_sample[channel].total_adc;

	if (adc <= 0) {
		pr_err("%s : invalid adc : %d\n", __func__, adc);
		adc = chg->adc_sample[channel].average_adc;
	}

	if (cnt < ADC_TOTAL_COUNT) {
		chg->adc_sample[channel].adc_arr[cnt] = adc;
		chg->adc_sample[channel].index = cnt;
		chg->adc_sample[channel].cnt = ++cnt;

		total_adc += adc;
		average_adc = total_adc / cnt;
	} else {
		index = chg->adc_sample[channel].index;
		if (++index >= ADC_TOTAL_COUNT)
			index = 0;

		total_adc = total_adc - chg->adc_sample[channel].adc_arr[index]
				+ adc;
		average_adc = total_adc / ADC_TOTAL_COUNT;

		chg->adc_sample[channel].adc_arr[index] = adc;
		chg->adc_sample[channel].index = index;
	}

	chg->adc_sample[channel].total_adc = total_adc;
	chg->adc_sample[channel].average_adc = average_adc;

	chg->bat_info.batt_temp_adc = average_adc;

	return average_adc;
}

static unsigned long s3c_read_temp(struct chg_data *chg)
{
	int adc = 0;

	adc = s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);

	return calculate_average_adc(S3C_ADC_TEMPERATURE, adc, chg);
}

static int s3c_get_bat_temp(struct chg_data *chg)
{
	int temp = 0;
	int array_size = 0;
	int i = 0;
	int temp_adc = s3c_read_temp(chg);
	int health = chg->bat_info.batt_health;

	if (temp_adc <= TEMP_IDLE_HIGH_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
		    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			chg->bat_info.batt_health =
					POWER_SUPPLY_HEALTH_OVERHEAT;
	} else if (temp_adc >= TEMP_IDLE_HIGH_RECOVER &&
		   temp_adc <= TEMP_LOW_RECOVER) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
		    health == POWER_SUPPLY_HEALTH_FREEZE)
			chg->bat_info.batt_health =
						POWER_SUPPLY_HEALTH_GOOD;
	} else if (temp_adc >= TEMP_LOW_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_FREEZE &&
		    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			chg->bat_info.batt_health =
						POWER_SUPPLY_HEALTH_FREEZE;
	}

	array_size = ARRAY_SIZE(temper_table);

	if (temp_adc >= temper_table[0][0])
		temp = temper_table[0][1];
	else if (temp_adc <= temper_table[array_size - 1][0])
		temp = temper_table[array_size - 1][1];

	for (i = 0; i < array_size - 1; i++)
		if (temper_table[i][0] > temp_adc &&
		    temper_table[i+1][0] <= temp_adc)
			temp = temper_table[i+1][1];

	chg->bat_info.batt_temp = temp;

	pr_debug("%s : temp = %d, adc = %d\n", __func__, temp, temp_adc);

	return temp;
}

static unsigned char max8998_charging_status(struct chg_data *chg)
{
	u8 data = 0;
	int ret;

	ret = max8998_read_reg(chg->iodev, MAX8998_REG_STATUS2, &data);

	if (ret < 0) {
		pr_err("max8998_read_reg error\n");
		return ret;
	}

	chg->current_device_type = (data & MAX8998_MASK_VDCIN) ?
					PM_CHARGER_TA : PM_CHARGER_NULL;

	return chg->current_device_type;
}

static int max8998_charging_control(struct chg_data *chg,
						unsigned int dev_type)
{
	int ret;

	if (dev_type == PM_CHARGER_TA) {
		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR1,
			(0 << MAX8998_SHIFT_TOPOFF), MAX8998_MASK_TOPOFF);
		if (ret < 0)
			goto err;

		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR1,
			(5 << MAX8998_SHIFT_ICHG), MAX8998_MASK_ICHG);
		if (ret < 0)
			goto err;

		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
			(2 << MAX8998_SHIFT_ESAFEOUT), MAX8998_MASK_ESAFEOUT);
		if (ret < 0)
			goto err;

		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
			(0 << MAX8998_SHIFT_CHGEN), MAX8998_MASK_CHGEN);
		if (ret < 0)
			goto err;

		pr_debug("%s : TA charging enable\n", __func__);
	} else if (dev_type == PM_CHARGER_USB_INSERT) {
		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR1,
			(3 << MAX8998_SHIFT_TOPOFF), MAX8998_MASK_TOPOFF);
		if (ret < 0)
			goto err;

		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR1,
			(2 << MAX8998_SHIFT_ICHG), MAX8998_MASK_ICHG);
		if (ret < 0)
			goto err;

		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
			(1 << MAX8998_SHIFT_ESAFEOUT), MAX8998_MASK_ESAFEOUT);
		if (ret < 0)
			goto err;

		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
			(0 << MAX8998_SHIFT_CHGEN), MAX8998_MASK_CHGEN);
		if (ret < 0)
			goto err;

		pr_debug("%s : USB charging enable\n", __func__);
	} else {
		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
			(1 << MAX8998_SHIFT_CHGEN), MAX8998_MASK_CHGEN);
		if (ret < 0)
			goto err;

		pr_debug("%s : charging disable\n", __func__);
	}
	return 0;
err:
	pr_err("max8998_read_reg error\n");
	return ret;
}

static void s3c_bat_status_update(struct chg_data *chg)
{
	static int old_temp = -1;
	static int old_is_full = -1;
	static int old_health = -1;

	if (old_temp != chg->bat_info.batt_temp ||
	    old_is_full != chg->bat_info.batt_is_full ||
	    old_health != chg->bat_info.batt_health ||
	    chg->force_update) {
		chg->force_update = 0;
		power_supply_changed(&chg->psy_bat);
		pr_info("%s : call power_supply_changed\n", __func__);
	}

	old_temp = chg->bat_info.batt_temp;
	old_is_full = chg->bat_info.batt_is_full;
	old_health = chg->bat_info.batt_health;
}

static int s3c_cable_status_update(struct chg_data *chg)
{
	static enum charger_type_t old_cable_status = CHARGER_BATTERY;
	int ret;

	if (max8998_charging_status(chg)) {
		if (chg->bat_info.batt_health !=
					POWER_SUPPLY_HEALTH_GOOD) {
			pr_info("%s : Unhealth battery state\n", __func__);
			chg->cable_status = CHARGER_DISCHARGE;
			ret = max8998_charging_control(chg, PM_CHARGER_DEFAULT);
			if (ret < 0)
				goto err;
			chg->bat_info.batt_is_recharging = 0;
			chg->bat_info.charging_enabled = 0;
			chg->bat_info.charger_online = 1;
			goto __end__;
		} else {
			/* TODO : check USB or TA */
			chg->cable_status = CHARGER_AC;
		}

		/* TODO : check recharging, full chg, over time */
		ret = max8998_charging_control(chg, chg->current_device_type);
		if (ret < 0)
			goto err;

		chg->bat_info.charging_enabled = 1;
		chg->bat_info.charger_online = 1;
	} else {
		chg->cable_status = CHARGER_BATTERY;
		ret = max8998_charging_control(chg, PM_CHARGER_DEFAULT);
		if (ret < 0)
			goto err;

		chg->bat_info.batt_is_recharging = 0;
		chg->bat_info.charging_enabled = 0;
		chg->bat_info.charger_online = 0;
	}

	if (old_cable_status != chg->cable_status) {
		old_cable_status = chg->cable_status;
		chg->bat_info.charging_source = chg->cable_status;

		chg->bat_info.charging_status =
				chg->bat_info.batt_is_full ?
						POWER_SUPPLY_STATUS_FULL :
						POWER_SUPPLY_STATUS_CHARGING;

		power_supply_changed(&chg->psy_bat);
	}

	if (chg->current_device_type == PM_CHARGER_NULL) {
		/* give userspace some time to see the uevent and update */
		if ((chg->cable_status != CHARGER_AC) &&
		    (chg->cable_status != CHARGER_USB))
			wake_lock_timeout(&chg->vbus_wake_lock, HZ / 2);
	} else {
		wake_lock(&chg->vbus_wake_lock);
	}

__end__:
	return 0;
err:
	return ret;
}

static void s3c_bat_work(struct work_struct *work)
{
	struct chg_data *chg =
		container_of(work, struct chg_data, bat_work.work);
	int ret;

	mutex_lock(&chg->mutex);

	s3c_get_bat_temp(chg);
	s3c_bat_status_update(chg);

	ret = s3c_cable_status_update(chg);
	if (ret < 0)
		goto err;

	s3c_bat_update_charging_status(chg);

	mutex_unlock(&chg->mutex);

	schedule_delayed_work(&chg->bat_work,
		msecs_to_jiffies(chg->polling_interval));

	return;
err:
	pr_err("battery workqueue fail\n");
}

static __devinit int max8998_charger_probe(struct platform_device *pdev)
{
	struct max8998_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max8998_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct chg_data *chg;
	int ret = 0;

	pr_info("%s : MAX8998 Charger Driver Loading\n", __func__);

	chg = kzalloc(sizeof(struct chg_data), GFP_KERNEL);
	if (!chg)
		return -ENOMEM;

	chg->iodev = iodev;
	chg->pdata = pdata->charger;

	chg->psy_bat.name = "battery",
	chg->psy_bat.type = POWER_SUPPLY_TYPE_BATTERY,
	chg->psy_bat.properties = max8998_battery_props,
	chg->psy_bat.num_properties = ARRAY_SIZE(max8998_battery_props),
	chg->psy_bat.get_property = s3c_bat_get_property,

	chg->psy_usb.name = "usb",
	chg->psy_usb.type = POWER_SUPPLY_TYPE_USB,
	chg->psy_usb.supplied_to = supply_list,
	chg->psy_usb.num_supplicants = ARRAY_SIZE(supply_list),
	chg->psy_usb.properties = s3c_power_properties,
	chg->psy_usb.num_properties = ARRAY_SIZE(s3c_power_properties),
	chg->psy_usb.get_property = s3c_usb_get_property,

	chg->psy_ac.name = "ac",
	chg->psy_ac.type = POWER_SUPPLY_TYPE_MAINS,
	chg->psy_ac.supplied_to = supply_list,
	chg->psy_ac.num_supplicants = ARRAY_SIZE(supply_list),
	chg->psy_ac.properties = s3c_power_properties,
	chg->psy_ac.num_properties = ARRAY_SIZE(s3c_power_properties),
	chg->psy_ac.get_property = s3c_ac_get_property,

	chg->present = 1;
	chg->polling_interval = POLLING_INTERVAL;
	chg->bat_info.batt_temp = 0;
	chg->bat_info.batt_temp_adc = 0;
	chg->bat_info.charging_source = CHARGER_BATTERY;
	chg->bat_info.charging_enabled = 0;
	chg->bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
	chg->bat_info.batt_is_full = 0;
	chg->bat_info.batt_is_recharging = 0;
	chg->bat_info.charging_status = 0;

	chg->current_device_type = PM_CHARGER_NULL;
	chg->cable_status = CHARGER_BATTERY;

	mutex_init(&chg->mutex);

	platform_set_drvdata(pdev, chg);

	ret = max8998_update_reg(iodev, MAX8998_REG_CHGR1, /* disable */
		(0x3 << MAX8998_SHIFT_RSTR), MAX8998_MASK_RSTR);
	if (ret < 0)
		goto err_kfree;

	ret = max8998_update_reg(iodev, MAX8998_REG_CHGR2, /* 6 Hr */
		(0x2 << MAX8998_SHIFT_FT), MAX8998_MASK_FT);
	if (ret < 0)
		goto err_kfree;

	ret = max8998_update_reg(iodev, MAX8998_REG_CHGR2, /* 4.2V */
		(0x0 << MAX8998_SHIFT_BATTSL), MAX8998_MASK_BATTSL);
	if (ret < 0)
		goto err_kfree;

	ret = max8998_update_reg(iodev, MAX8998_REG_CHGR2, /* 105c */
		(0x0 << MAX8998_SHIFT_TMP), MAX8998_MASK_TMP);
	if (ret < 0)
		goto err_kfree;

	wake_lock_init(&chg->vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");

	INIT_DELAYED_WORK(&chg->bat_work, s3c_bat_work);

	/* init power supplier framework */
	ret = power_supply_register(&pdev->dev, &chg->psy_bat);
	if (ret) {
		pr_err("Failed to register power supply psy_bat\n");
		goto err_wake_lock;
	}

	ret = power_supply_register(&pdev->dev, &chg->psy_usb);
	if (ret) {
		pr_err("Failed to register power supply psy_usb\n");
		goto err_supply_unreg_bat;
	}

	ret = power_supply_register(&pdev->dev, &chg->psy_ac);
	if (ret) {
		pr_err("Failed to register power supply psy_ac\n");
		goto err_supply_unreg_usb;
	}

	pr_info("%s : pmic interrupt registered\n", __func__);

	schedule_delayed_work(&chg->bat_work,
		msecs_to_jiffies(chg->polling_interval));

	return 0;

err_supply_unreg_usb:
	power_supply_unregister(&chg->psy_usb);
err_supply_unreg_bat:
	power_supply_unregister(&chg->psy_bat);
err_wake_lock:
	wake_lock_destroy(&chg->vbus_wake_lock);
err_kfree:
	mutex_destroy(&chg->mutex);
	kfree(chg);
	return ret;
}

static int __devexit max8998_charger_remove(struct platform_device *pdev)
{
	struct chg_data *chg = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&chg->bat_work);
	power_supply_unregister(&chg->psy_bat);
	power_supply_unregister(&chg->psy_usb);
	power_supply_unregister(&chg->psy_ac);

	wake_lock_destroy(&chg->vbus_wake_lock);
	mutex_destroy(&chg->mutex);
	kfree(chg);

	return 0;
}

static struct platform_driver max8998_charger_driver = {
	.driver = {
		.name = "max8998-charger",
		.owner = THIS_MODULE,
	},
	.probe = max8998_charger_probe,
	.remove = __devexit_p(max8998_charger_remove),
};

static int __init max8998_charger_init(void)
{
	return platform_driver_register(&max8998_charger_driver);
}

static void __exit max8998_charger_exit(void)
{
	platform_driver_register(&max8998_charger_driver);
}

module_init(max8998_charger_init);
module_exit(max8998_charger_exit);

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("S3C6410 battery driver");
MODULE_LICENSE("GPL");
