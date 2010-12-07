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
#include <linux/android_alarm.h>
#include "s5pc110_battery.h"
#include <linux/mfd/max8998.h>

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

#define FAST_POLL			(1 * 60)
#define SLOW_POLL			(10 * 60)

#define DISCONNECT_BAT_FULL		0x1
#define DISCONNECT_TEMP_OVERHEAT	0x2
#define DISCONNECT_TEMP_FREEZE		0x4
#define DISCONNECT_OVER_TIME		0x8

#define ATTACH_USB	1
#define ATTACH_TA	2

#define HIGH_BLOCK_TEMP			500
#define HIGH_RECOVER_TEMP		420
#define LOW_BLOCK_TEMP			0
#define LOW_RECOVER_TEMP		20

struct battery_info {
	u32 batt_temp;		/* Battery Temperature (C) from ADC */
	u32 batt_temp_adc;	/* Battery Temperature ADC value */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 dis_reason;
	u32 batt_vcell;
	u32 batt_soc;
	u32 charging_status;
	bool batt_is_full;      /* 0 : Not full 1: Full */
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
	struct work_struct	bat_work;
	struct max8998_charger_data *pdata;

	struct power_supply	psy_bat;
	struct power_supply	psy_usb;
	struct power_supply	psy_ac;
	struct alarm		alarm;
	struct workqueue_struct *monitor_wqueue;
	struct wake_lock	vbus_wake_lock;
	struct wake_lock	work_wake_lock;
	struct adc_sample_info	adc_sample[ENDOFADC];
	struct battery_info	bat_info;
	struct mutex		mutex;

	enum cable_type_t	cable_status;
	bool			charging;
	bool			set_charge_timeout;
	int			present;
	int			timestamp;
	int			set_batt_full;
	unsigned long		discharging_time;
	unsigned int		polling_interval;
	int                     slow_poll;
	ktime_t                 last_poll;
	struct max8998_charger_callbacks callbacks;
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
	POWER_SUPPLY_PROP_TECHNOLOGY,
};

static enum power_supply_property s3c_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static void max8998_set_cable(struct max8998_charger_callbacks *ptr,
	enum cable_type_t status)
{
	struct chg_data *chg = container_of(ptr, struct chg_data, callbacks);
	chg->cable_status = status;
	power_supply_changed(&chg->psy_ac);
	power_supply_changed(&chg->psy_usb);
	wake_lock(&chg->work_wake_lock);
	queue_work(chg->monitor_wqueue, &chg->bat_work);
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
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* battery is always online */
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if (chg->pdata && chg->pdata->psy_fuelgauge &&
			chg->pdata->psy_fuelgauge->get_property &&
			chg->pdata->psy_fuelgauge->get_property(
				chg->pdata->psy_fuelgauge, psp, val) < 0)
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (chg->bat_info.batt_is_full)
			val->intval = 100;
		else if (chg->pdata && chg->pdata->psy_fuelgauge &&
			chg->pdata->psy_fuelgauge->get_property &&
			chg->pdata->psy_fuelgauge->get_property(
				chg->pdata->psy_fuelgauge, psp, val) < 0)
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
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

	/* Set enable=1 only if the USB charger is connected */
	val->intval = (chg->cable_status == CABLE_TYPE_USB);

	return 0;
}

static int s3c_ac_get_property(struct power_supply *ps,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct chg_data *chg = container_of(ps, struct chg_data, psy_ac);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	/* Set enable=1 only if the AC charger is connected */
	val->intval = (chg->cable_status == CABLE_TYPE_AC);

	return 0;
}

static int s3c_bat_get_adc_data(enum adc_channel_type adc_ch)
{
	int adc_data;
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;

	for (i = 0; i < ADC_DATA_ARR_SIZE; i++) {
		adc_data = s3c_adc_get_adc_data(adc_ch);

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
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
	int temp_adc = s3c_read_temp(chg);
	int health = chg->bat_info.batt_health;
	int left_side = 0;
	int right_side = chg->pdata->adc_array_size - 1;
	int mid;

	while (left_side <= right_side) {
		mid = (left_side + right_side) / 2 ;
		if (mid == 0 || mid == chg->pdata->adc_array_size - 1 ||
				(chg->pdata->adc_table[mid].adc_value <= temp_adc &&
				chg->pdata->adc_table[mid+1].adc_value > temp_adc)) {
			temp = chg->pdata->adc_table[mid].temperature;
			break;
		} else if (temp_adc - chg->pdata->adc_table[mid].adc_value > 0)
			left_side = mid + 1;
		else
			right_side = mid - 1;
	}

	chg->bat_info.batt_temp = temp;
	if (temp >= HIGH_BLOCK_TEMP) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
		    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			chg->bat_info.batt_health =
					POWER_SUPPLY_HEALTH_OVERHEAT;
	} else if (temp <= HIGH_RECOVER_TEMP && temp >= LOW_RECOVER_TEMP) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
		    health == POWER_SUPPLY_HEALTH_COLD)
			chg->bat_info.batt_health =
					POWER_SUPPLY_HEALTH_GOOD;
	} else if (temp <= LOW_BLOCK_TEMP) {
		if (health != POWER_SUPPLY_HEALTH_COLD &&
		    health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			chg->bat_info.batt_health =
				POWER_SUPPLY_HEALTH_COLD;
	}

	pr_debug("%s : temp = %d, adc = %d\n", __func__, temp, temp_adc);

	return temp;
}

static void s3c_bat_discharge_reason(struct chg_data *chg)
{
	int discharge_reason;
	ktime_t ktime;
	struct timespec cur_time;
	union power_supply_propval value;

	if (chg->pdata && chg->pdata->psy_fuelgauge &&
			 chg->pdata->psy_fuelgauge->get_property) {
		chg->pdata->psy_fuelgauge->get_property(chg->pdata->psy_fuelgauge,
		POWER_SUPPLY_PROP_VOLTAGE_NOW, &value);
		chg->bat_info.batt_vcell = value.intval;

		chg->pdata->psy_fuelgauge->get_property(chg->pdata->psy_fuelgauge,
		POWER_SUPPLY_PROP_CAPACITY, &value);
		chg->bat_info.batt_soc = value.intval;
	}

	discharge_reason = chg->bat_info.dis_reason & 0xf;

	if (discharge_reason == DISCONNECT_BAT_FULL &&
			chg->bat_info.batt_vcell < RECHARGE_COND_VOLTAGE)
		chg->bat_info.dis_reason &= ~DISCONNECT_BAT_FULL;

	if (discharge_reason == DISCONNECT_TEMP_OVERHEAT &&
			chg->bat_info.batt_temp <=
			HIGH_RECOVER_TEMP)
		chg->bat_info.dis_reason &= ~DISCONNECT_TEMP_OVERHEAT;

	if (discharge_reason == DISCONNECT_TEMP_FREEZE &&
			chg->bat_info.batt_temp >=
			LOW_RECOVER_TEMP)
		chg->bat_info.dis_reason &= ~DISCONNECT_TEMP_FREEZE;

	if (discharge_reason == DISCONNECT_OVER_TIME &&
			chg->bat_info.batt_vcell < RECHARGE_COND_VOLTAGE)
		chg->bat_info.dis_reason &= ~DISCONNECT_OVER_TIME;

	if (chg->set_batt_full)
		chg->bat_info.dis_reason |= DISCONNECT_BAT_FULL;

	if (chg->bat_info.batt_health != POWER_SUPPLY_HEALTH_GOOD)
		chg->bat_info.dis_reason |= chg->bat_info.batt_health ==
			POWER_SUPPLY_HEALTH_OVERHEAT ?
			DISCONNECT_TEMP_OVERHEAT : DISCONNECT_TEMP_FREEZE;

	ktime = alarm_get_elapsed_realtime();
	cur_time = ktime_to_timespec(ktime);

	if (chg->discharging_time &&
			cur_time.tv_sec > chg->discharging_time) {
		chg->set_charge_timeout = true;
		chg->bat_info.dis_reason |= DISCONNECT_OVER_TIME;
	}

	pr_debug("%s : Current Voltage : %d\n			\
		Current time : %ld  discharging_time : %ld\n	\
		discharging reason : %d\n",			\
		__func__, chg->bat_info.batt_vcell, cur_time.tv_sec,
		chg->discharging_time, chg->bat_info.dis_reason);
}

static bool max8998_check_vdcin(struct chg_data *chg)
{
	u8 data = 0;
	int ret;

	ret = max8998_read_reg(chg->iodev, MAX8998_REG_STATUS2, &data);

	if (ret < 0) {
		pr_err("max8998_read_reg error\n");
		return ret;
	}

	return data & MAX8998_MASK_VDCIN;
}

static int max8998_charging_control(struct chg_data *chg)
{
	int ret;

	if (!chg->charging) {
		/* disable charging */
		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
			(1 << MAX8998_SHIFT_CHGEN), MAX8998_MASK_CHGEN);
		if (ret < 0)
			goto err;

		pr_debug("%s : charging disabled", __func__);
	} else {
		/* enable charging */
		if (chg->cable_status == CABLE_TYPE_AC) {
			/* ac */
			ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR1,
				(2 << MAX8998_SHIFT_TOPOFF), MAX8998_MASK_TOPOFF);
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

			pr_debug("%s : TA charging enabled", __func__);

		} else {
			/* usb */
			ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR1,
				(6 << MAX8998_SHIFT_TOPOFF), MAX8998_MASK_TOPOFF);
			if (ret < 0)
				goto err;

			ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR1,
				(2 << MAX8998_SHIFT_ICHG), MAX8998_MASK_ICHG);
			if (ret < 0)
				goto err;

			ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
				(3 << MAX8998_SHIFT_ESAFEOUT), MAX8998_MASK_ESAFEOUT);
			if (ret < 0)
				goto err;

			pr_debug("%s : USB charging enabled", __func__);
		}

		ret = max8998_update_reg(chg->iodev, MAX8998_REG_CHGR2,
			(0 << MAX8998_SHIFT_CHGEN), MAX8998_MASK_CHGEN);
		if (ret < 0)
			goto err;
	}

	return 0;
err:
	pr_err("max8998_read_reg error\n");
	return ret;
}

static int s3c_cable_status_update(struct chg_data *chg)
{
	int ret;
	ktime_t ktime;
	struct timespec cur_time;

	/* if max8998 has detected vdcin */
	if (max8998_check_vdcin(chg)) {
		if (chg->bat_info.dis_reason) {
			pr_info("%s : battery status discharging : %d\n",
				__func__, chg->bat_info.dis_reason);
			/* have vdcin, but cannot charge */
			chg->charging = false;
			ret = max8998_charging_control(chg);
			if (ret < 0)
				goto err;
			chg->bat_info.charging_status =
				chg->bat_info.batt_is_full ?
				POWER_SUPPLY_STATUS_FULL :
				POWER_SUPPLY_STATUS_NOT_CHARGING;
			chg->discharging_time = 0;
			chg->set_batt_full = 0;
			goto update;
		} else if (chg->discharging_time == 0) {
			ktime = alarm_get_elapsed_realtime();
			cur_time = ktime_to_timespec(ktime);
			chg->discharging_time =
				chg->bat_info.batt_is_full ||
				chg->set_charge_timeout ?
				cur_time.tv_sec + TOTAL_RECHARGING_TIME :
				cur_time.tv_sec + TOTAL_CHARGING_TIME;
		}

		/* able to charge */
		chg->charging = true;
		ret = max8998_charging_control(chg);
		if (ret < 0)
			goto err;

		chg->bat_info.charging_status = chg->bat_info.batt_is_full ?
			POWER_SUPPLY_STATUS_FULL : POWER_SUPPLY_STATUS_CHARGING;

	} else {
		/* no vdc in, not able to charge */
		chg->charging = false;
		ret = max8998_charging_control(chg);
		if (ret < 0)
			goto err;

		chg->bat_info.charging_status = POWER_SUPPLY_STATUS_DISCHARGING;

		chg->bat_info.batt_is_full = false;
		chg->set_charge_timeout = false;
		chg->set_batt_full = 0;
		chg->bat_info.dis_reason = 0;
		chg->discharging_time = 0;
	}

update:
	if (chg->cable_status == CABLE_TYPE_USB)
		wake_lock(&chg->vbus_wake_lock);
	else
		wake_lock_timeout(&chg->vbus_wake_lock, HZ / 2);

	return 0;
err:
	return ret;
}

static void s3c_program_alarm(struct chg_data *chg, int seconds)
{
	ktime_t low_interval = ktime_set(seconds - 10, 0);
	ktime_t slack = ktime_set(20, 0);
	ktime_t next;

	next = ktime_add(chg->last_poll, low_interval);
	alarm_start_range(&chg->alarm, next, ktime_add(next, slack));
}

static void s3c_bat_work(struct work_struct *work)
{
	struct chg_data *chg =
		container_of(work, struct chg_data, bat_work);
	int ret;
	struct timespec ts;
	unsigned long flags;
	mutex_lock(&chg->mutex);

	s3c_get_bat_temp(chg);
	s3c_bat_discharge_reason(chg);

	ret = s3c_cable_status_update(chg);
	if (ret < 0)
		goto err;

	mutex_unlock(&chg->mutex);

	power_supply_changed(&chg->psy_bat);

	chg->last_poll = alarm_get_elapsed_realtime();
	ts = ktime_to_timespec(chg->last_poll);
	chg->timestamp = ts.tv_sec;

	/* prevent suspend before starting the alarm */
	local_irq_save(flags);
	wake_unlock(&chg->work_wake_lock);
	s3c_program_alarm(chg, FAST_POLL);
	local_irq_restore(flags);
	return;
err:
	mutex_unlock(&chg->mutex);
	wake_unlock(&chg->work_wake_lock);
	pr_err("battery workqueue fail\n");
}

static void s3c_battery_alarm(struct alarm *alarm)
{
	struct chg_data *chg =
			container_of(alarm, struct chg_data, alarm);

	wake_lock(&chg->work_wake_lock);
	queue_work(chg->monitor_wqueue, &chg->bat_work);
}

static irqreturn_t max8998_int_work_func(int irq, void *max8998_chg)
{
	int ret;
	u8 data = 0;
	struct chg_data *chg;

	chg = max8998_chg;

	ret = max8998_read_reg(chg->iodev, MAX8998_REG_IRQ1, &data);
	if (ret < 0)
		goto err;

	ret = max8998_read_reg(chg->iodev, MAX8998_REG_IRQ3, &data);
	if (ret < 0)
		goto err;

	if ((data & 0x4) || (ret != 0)) {
		pr_info("%s : pmic interrupt\n", __func__);
		chg->set_batt_full = 1;
		chg->bat_info.batt_is_full = true;
	}

	wake_lock(&chg->work_wake_lock);
	queue_work(chg->monitor_wqueue, &chg->bat_work);

	return IRQ_HANDLED;
err:
	pr_err("%s : pmic read error\n", __func__);
	return IRQ_HANDLED;
}

static __devinit int max8998_charger_probe(struct platform_device *pdev)
{
	struct max8998_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max8998_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct chg_data *chg;
	int ret = 0;

	pr_info("%s : MAX8998 Charger Driver Loading\n", __func__);

	chg = kzalloc(sizeof(*chg), GFP_KERNEL);
	if (!chg)
		return -ENOMEM;

	chg->iodev = iodev;
	chg->pdata = pdata->charger;

	if (!chg->pdata || !chg->pdata->adc_table) {
		pr_err("%s : No platform data & adc_table supplied\n", __func__);
		ret = -EINVAL;
		goto err_bat_table;
	}

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
	chg->bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
	chg->bat_info.batt_is_full = false;
	chg->set_charge_timeout = false;

	chg->cable_status = CABLE_TYPE_NONE;

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

	pr_info("%s : pmic interrupt registered\n", __func__);
	ret = max8998_write_reg(iodev, MAX8998_REG_IRQM1,
		~(MAX8998_MASK_DCINR | MAX8998_MASK_DCINF));
	if (ret < 0)
		goto err_kfree;

	ret = max8998_write_reg(iodev, MAX8998_REG_IRQM2, 0xFF);
	if (ret < 0)
		goto err_kfree;

	ret = max8998_write_reg(iodev, MAX8998_REG_IRQM3, ~0x4);
	if (ret < 0)
		goto err_kfree;

	ret = max8998_write_reg(iodev, MAX8998_REG_IRQM4, 0xFF);
	if (ret < 0)
		goto err_kfree;

	wake_lock_init(&chg->vbus_wake_lock, WAKE_LOCK_SUSPEND,
		"vbus_present");
	wake_lock_init(&chg->work_wake_lock, WAKE_LOCK_SUSPEND,
		"max8998-charger");

	INIT_WORK(&chg->bat_work, s3c_bat_work);

	chg->monitor_wqueue =
		create_freezeable_workqueue(dev_name(&pdev->dev));
	if (!chg->monitor_wqueue) {
		pr_err("Failed to create freezeable workqueue\n");
		ret = -ENOMEM;
		goto err_wake_lock;
	}

	chg->last_poll = alarm_get_elapsed_realtime();
	alarm_init(&chg->alarm, ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
		s3c_battery_alarm);

	/* init power supplier framework */
	ret = power_supply_register(&pdev->dev, &chg->psy_bat);
	if (ret) {
		pr_err("Failed to register power supply psy_bat\n");
		goto err_wqueue;
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

	ret = request_threaded_irq(iodev->i2c_client->irq, NULL,
			max8998_int_work_func,
			IRQF_TRIGGER_FALLING, "max8998-charger", chg);
	if (ret) {
		pr_err("%s : Failed to request pmic irq\n", __func__);
		goto err_supply_unreg_ac;
	}

	ret = enable_irq_wake(iodev->i2c_client->irq);
	if (ret) {
		pr_err("Failed to enable pmic irq wake\n");
		goto err_irq;
	}

	chg->callbacks.set_cable = max8998_set_cable;
	if (chg->pdata->register_callbacks)
		chg->pdata->register_callbacks(&chg->callbacks);

	wake_lock(&chg->work_wake_lock);
	queue_work(chg->monitor_wqueue, &chg->bat_work);

	return 0;

err_irq:
	free_irq(iodev->i2c_client->irq, NULL);
err_supply_unreg_ac:
	power_supply_unregister(&chg->psy_ac);
err_supply_unreg_usb:
	power_supply_unregister(&chg->psy_usb);
err_supply_unreg_bat:
	power_supply_unregister(&chg->psy_bat);
err_wqueue:
	destroy_workqueue(chg->monitor_wqueue);
	cancel_work_sync(&chg->bat_work);
	alarm_cancel(&chg->alarm);
err_wake_lock:
	wake_lock_destroy(&chg->work_wake_lock);
	wake_lock_destroy(&chg->vbus_wake_lock);
err_kfree:
	mutex_destroy(&chg->mutex);
err_bat_table:
	kfree(chg);
	return ret;
}

static int __devexit max8998_charger_remove(struct platform_device *pdev)
{
	struct chg_data *chg = platform_get_drvdata(pdev);

	alarm_cancel(&chg->alarm);
	free_irq(chg->iodev->i2c_client->irq, NULL);
	flush_workqueue(chg->monitor_wqueue);
	destroy_workqueue(chg->monitor_wqueue);
	power_supply_unregister(&chg->psy_bat);
	power_supply_unregister(&chg->psy_usb);
	power_supply_unregister(&chg->psy_ac);

	wake_lock_destroy(&chg->vbus_wake_lock);
	mutex_destroy(&chg->mutex);
	kfree(chg);

	return 0;
}

static int max8998_charger_suspend(struct device *dev)
{

	struct chg_data *chg = dev_get_drvdata(dev);
	if (!chg->charging) {
		s3c_program_alarm(chg, SLOW_POLL);
		chg->slow_poll = 1;
	}

	return 0;
}

static void max8998_charger_resume(struct device *dev)
{

	struct chg_data *chg = dev_get_drvdata(dev);
	/* We might be on a slow sample cycle.  If we're
	 * resuming we should resample the battery state
	 * if it's been over a minute since we last did
	 * so, and move back to sampling every minute until
	 * we suspend again.
	 */
	if (chg->slow_poll) {
		s3c_program_alarm(chg, FAST_POLL);
		chg->slow_poll = 0;
	}
}

static const struct dev_pm_ops max8998_charger_pm_ops = {
	.prepare        = max8998_charger_suspend,
	.complete       = max8998_charger_resume,
};

static struct platform_driver max8998_charger_driver = {
	.driver = {
		.name = "max8998-charger",
		.owner = THIS_MODULE,
		.pm = &max8998_charger_pm_ops,
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

late_initcall(max8998_charger_init);
module_exit(max8998_charger_exit);

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("S3C6410 battery driver");
MODULE_LICENSE("GPL");
