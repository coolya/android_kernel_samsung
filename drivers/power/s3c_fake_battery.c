/*
 * linux/drivers/power/s3c_fake_battery.c
 *
 * Battery measurement code for S3C platform.
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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>

#define DRIVER_NAME	"sec-fake-battery"

static struct wake_lock vbus_wake_lock;

/* Prototypes */
extern int s3c_adc_get_adc_data(int channel);

static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf);
static ssize_t s3c_bat_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count);

#define FAKE_BAT_LEVEL	80

static struct device *dev;
static int s3c_battery_initial;
static int force_update;

static char *status_text[] = {
	[POWER_SUPPLY_STATUS_UNKNOWN] =		"Unknown",
	[POWER_SUPPLY_STATUS_CHARGING] =	"Charging",
	[POWER_SUPPLY_STATUS_DISCHARGING] =	"Discharging",
	[POWER_SUPPLY_STATUS_NOT_CHARGING] =	"Not Charging",
	[POWER_SUPPLY_STATUS_FULL] =		"Full",
};

typedef enum {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC,
	CHARGER_DISCHARGE
} charger_type_t;

struct battery_info {
	u32 batt_id;		/* Battery ID from ADC */
	u32 batt_vol;		/* Battery voltage from ADC */
	u32 batt_vol_adc;	/* Battery ADC value */
	u32 batt_vol_adc_cal;	/* Battery ADC value (calibrated)*/
	u32 batt_temp;		/* Battery Temperature (C) from ADC */
	u32 batt_temp_adc;	/* Battery Temperature ADC value */
	u32 batt_temp_adc_cal;	/* Battery Temperature ADC value (calibrated) */
	u32 batt_current;	/* Battery current from ADC */
	u32 level;		/* formula */
	u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
	u32 charging_enabled;	/* 0: Disable, 1: Enable */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 batt_is_full;       /* 0 : Not full 1: Full */
};

/* lock to protect the battery info */
static DEFINE_MUTEX(work_lock);

struct s3c_battery_info {
	int present;
	int polling;
	unsigned long polling_interval;

	struct battery_info bat_info;
};
static struct s3c_battery_info s3c_bat_info;

static int s3c_get_bat_level(struct power_supply *bat_ps)
{
	return FAKE_BAT_LEVEL;
}

static int s3c_get_bat_vol(struct power_supply *bat_ps)
{
	int bat_vol = 0;

	return bat_vol;
}

static u32 s3c_get_bat_health(void)
{
	return s3c_bat_info.bat_info.batt_health;
}

static int s3c_get_bat_temp(struct power_supply *bat_ps)
{
	int temp = 0;

	return temp;
}

static int s3c_bat_get_charging_status(void)
{
	charger_type_t charger = CHARGER_BATTERY; 
	int ret = 0;
        
	charger = s3c_bat_info.bat_info.charging_source;
        
	switch (charger) {
	case CHARGER_BATTERY:
		ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case CHARGER_USB:
	case CHARGER_AC:
		if (s3c_bat_info.bat_info.level == 100 
			&& s3c_bat_info.bat_info.batt_is_full) {
			ret = POWER_SUPPLY_STATUS_FULL;
		} else {
			ret = POWER_SUPPLY_STATUS_CHARGING;
		}
		break;
	case CHARGER_DISCHARGE:
		ret = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	default:
		ret = POWER_SUPPLY_STATUS_UNKNOWN;
	}
	dev_dbg(dev, "%s : %s\n", __func__, status_text[ret]);

	return ret;
}

static int s3c_bat_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	dev_dbg(bat_ps->dev, "%s : psp = %d\n", __func__, psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s3c_bat_get_charging_status();
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s3c_get_bat_health();
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s3c_bat_info.present;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = s3c_bat_info.bat_info.level;
		dev_dbg(dev, "%s : level = %d\n", __func__, 
				val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = s3c_bat_info.bat_info.batt_temp;
		dev_dbg(bat_ps->dev, "%s : temp = %d\n", __func__, 
				val->intval);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int s3c_power_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp, 
		union power_supply_propval *val)
{
	charger_type_t charger;
	
	dev_dbg(bat_ps->dev, "%s : psp = %d\n", __func__, psp);

	charger = s3c_bat_info.bat_info.charging_source;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (bat_ps->type == POWER_SUPPLY_TYPE_MAINS)
			val->intval = (charger == CHARGER_AC ? 1 : 0);
		else if (bat_ps->type == POWER_SUPPLY_TYPE_USB)
			val->intval = (charger == CHARGER_USB ? 1 : 0);
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

#define SEC_BATTERY_ATTR(_name)								\
{											\
	.attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO, .owner = THIS_MODULE },	\
	.show = s3c_bat_show_property,							\
	.store = s3c_bat_store,								\
}

static struct device_attribute s3c_battery_attrs[] = {
	SEC_BATTERY_ATTR(batt_vol),
	SEC_BATTERY_ATTR(batt_vol_adc),
	SEC_BATTERY_ATTR(batt_vol_adc_cal),
	SEC_BATTERY_ATTR(batt_temp),
	SEC_BATTERY_ATTR(batt_temp_adc),
	SEC_BATTERY_ATTR(batt_temp_adc_cal),
};

enum {
	BATT_VOL = 0,
	BATT_VOL_ADC,
	BATT_VOL_ADC_CAL,
	BATT_TEMP,
	BATT_TEMP_ADC,
	BATT_TEMP_ADC_CAL,
};

static int s3c_bat_create_attrs(struct device * dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(s3c_battery_attrs); i++) {
		rc = device_create_file(dev, &s3c_battery_attrs[i]);
		if (rc)
		goto s3c_attrs_failed;
	}
	goto succeed;

s3c_attrs_failed:
	while (i--)
	device_remove_file(dev, &s3c_battery_attrs[i]);
succeed:
	return rc;
}

static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - s3c_battery_attrs;

	switch (off) {
	case BATT_VOL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       s3c_bat_info.bat_info.batt_vol);
	break;
	case BATT_VOL_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       s3c_bat_info.bat_info.batt_vol_adc);
		break;
	case BATT_VOL_ADC_CAL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       s3c_bat_info.bat_info.batt_vol_adc_cal);
		break;
	case BATT_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       s3c_bat_info.bat_info.batt_temp);
		break;
	case BATT_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       s3c_bat_info.bat_info.batt_temp_adc);
		break;	
	case BATT_TEMP_ADC_CAL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       s3c_bat_info.bat_info.batt_temp_adc_cal);
		break;
	default:
		i = -EINVAL;
	}       

	return i;
}

static ssize_t s3c_bat_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - s3c_battery_attrs;

	switch (off) {
	case BATT_VOL_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_vol_adc_cal = x;
			ret = count;
		}
		dev_info(dev, "%s : batt_vol_adc_cal = %d\n", __func__, x);
		break;
	case BATT_TEMP_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_temp_adc_cal = x;
			ret = count;
		}
		dev_info(dev, "%s : batt_temp_adc_cal = %d\n", __func__, x);
		break;
	default:
		ret = -EINVAL;
	}       

	return ret;
}

static enum power_supply_property s3c_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property s3c_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

static struct power_supply s3c_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = s3c_battery_properties,
		.num_properties = ARRAY_SIZE(s3c_battery_properties),
		.get_property = s3c_bat_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
};

static int s3c_cable_status_update(int status)
{
	int ret = 0;
	charger_type_t source = CHARGER_BATTERY;

	dev_dbg(dev, "%s\n", __func__);

	if(!s3c_battery_initial)
		return -EPERM;

	switch(status) {
	case CHARGER_BATTERY:
		dev_dbg(dev, "%s : cable NOT PRESENT\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
		break;
	case CHARGER_USB:
		dev_dbg(dev, "%s : cable USB\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_USB;
		break;
	case CHARGER_AC:
		dev_dbg(dev, "%s : cable AC\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_AC;
		break;
	case CHARGER_DISCHARGE:
		dev_dbg(dev, "%s : Discharge\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_DISCHARGE;
		break;
	default:
		dev_err(dev, "%s : Nat supported status\n", __func__);
		ret = -EINVAL;
	}
	source = s3c_bat_info.bat_info.charging_source;

	if (source == CHARGER_USB || source == CHARGER_AC) {
		wake_lock(&vbus_wake_lock);
	} else {
		/* give userspace some time to see the uevent and update
		 * LED state or whatnot...
		 */
		wake_lock_timeout(&vbus_wake_lock, HZ / 2);
	}

	/* if the power source changes, all power supplies may change state */
	power_supply_changed(&s3c_power_supplies[CHARGER_BATTERY]);
	/*
	power_supply_changed(&s3c_power_supplies[CHARGER_USB]);
	power_supply_changed(&s3c_power_supplies[CHARGER_AC]);
	*/
	dev_dbg(dev, "%s : call power_supply_changed\n", __func__);
	return ret;
}

static void s3c_bat_status_update(struct power_supply *bat_ps)
{
	int old_level, old_temp, old_is_full;
	dev_dbg(dev, "%s ++\n", __func__);

	if(!s3c_battery_initial)
		return;

	mutex_lock(&work_lock);
	old_temp = s3c_bat_info.bat_info.batt_temp;
	old_level = s3c_bat_info.bat_info.level; 
	old_is_full = s3c_bat_info.bat_info.batt_is_full;
	s3c_bat_info.bat_info.batt_temp = s3c_get_bat_temp(bat_ps);
	s3c_bat_info.bat_info.level = s3c_get_bat_level(bat_ps);
	s3c_bat_info.bat_info.batt_vol = s3c_get_bat_vol(bat_ps);

	if (old_level != s3c_bat_info.bat_info.level 
			|| old_temp != s3c_bat_info.bat_info.batt_temp
			|| old_is_full != s3c_bat_info.bat_info.batt_is_full
			|| force_update) {
		force_update = 0;
		power_supply_changed(bat_ps);
		dev_dbg(dev, "%s : call power_supply_changed\n", __func__);
	}

	mutex_unlock(&work_lock);
	dev_dbg(dev, "%s --\n", __func__);
}

void s3c_cable_check_status(int flag)
{
    charger_type_t status = 0;

    if (flag == 0)  // Battery
		status = CHARGER_BATTERY;
    else    // USB
		status = CHARGER_USB;
    s3c_cable_status_update(status);
}
EXPORT_SYMBOL(s3c_cable_check_status);

#ifdef CONFIG_PM
static int s3c_bat_suspend(struct platform_device *pdev, 
		pm_message_t state)
{
	dev_info(dev, "%s\n", __func__);

	return 0;
}

static int s3c_bat_resume(struct platform_device *pdev)
{
	dev_info(dev, "%s\n", __func__);

	return 0;
}
#else
#define s3c_bat_suspend NULL
#define s3c_bat_resume NULL
#endif /* CONFIG_PM */

static int __devinit s3c_bat_probe(struct platform_device *pdev)
{
	int i;
	int ret = 0;

	dev = &pdev->dev;
	dev_info(dev, "%s\n", __func__);

	s3c_bat_info.present = 1;

	s3c_bat_info.bat_info.batt_id = 0;
	s3c_bat_info.bat_info.batt_vol = 0;
	s3c_bat_info.bat_info.batt_vol_adc = 0;
	s3c_bat_info.bat_info.batt_vol_adc_cal = 0;
	s3c_bat_info.bat_info.batt_temp = 0;
	s3c_bat_info.bat_info.batt_temp_adc = 0;
	s3c_bat_info.bat_info.batt_temp_adc_cal = 0;
	s3c_bat_info.bat_info.batt_current = 0;
	s3c_bat_info.bat_info.level = 0;
	s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
	s3c_bat_info.bat_info.charging_enabled = 0;
	s3c_bat_info.bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		ret = power_supply_register(&pdev->dev, 
				&s3c_power_supplies[i]);
		if (ret) {
			dev_err(dev, "Failed to register"
					"power supply %d,%d\n", i, ret);
			goto __end__;
		}
	}

	/* create sec detail attributes */
	s3c_bat_create_attrs(s3c_power_supplies[CHARGER_BATTERY].dev);

	s3c_battery_initial = 1;
	force_update = 0;

	s3c_bat_status_update(
			&s3c_power_supplies[CHARGER_BATTERY]);

__end__:
	return ret;
}

static int __devexit s3c_bat_remove(struct platform_device *pdev)
{
	int i;
	dev_info(dev, "%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		power_supply_unregister(&s3c_power_supplies[i]);
	}
 
	return 0;
}

static struct platform_driver s3c_bat_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.probe		= s3c_bat_probe,
	.remove		= __devexit_p(s3c_bat_remove),
	.suspend	= s3c_bat_suspend,
	.resume		= s3c_bat_resume,
};

/* Initailize GPIO */
static void s3c_bat_init_hw(void)
{
}

static int __init s3c_bat_init(void)
{
	pr_info("%s\n", __func__);

	s3c_bat_init_hw();

	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");

	return platform_driver_register(&s3c_bat_driver);
}

static void __exit s3c_bat_exit(void)
{
	pr_info("%s\n", __func__);
	platform_driver_unregister(&s3c_bat_driver);
}

module_init(s3c_bat_init);
module_exit(s3c_bat_exit);

MODULE_AUTHOR("HuiSung Kang <hs1218.kang@samsung.com>");
MODULE_DESCRIPTION("S3C battery driver for SMDK Board");
MODULE_LICENSE("GPL");
