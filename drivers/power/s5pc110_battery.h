/*
 * linux/drivers/power/s3c6410_battery.h
 *
 * Battery measurement code for S3C6410 platform.
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define DRIVER_NAME	"sec-battery"

/*
 * Battery Table
 */
#define BATT_CAL		2447	/* 3.60V */

#define BATT_MAXIMUM		406	/* 4.176V */
#define BATT_FULL		353	/* 4.10V  */
#define BATT_SAFE_RECHARGE	353	/* 4.10V */
#define BATT_ALMOST_FULL	188	/* 3.8641V */
#define BATT_HIGH		112	/* 3.7554V */
#define BATT_MED		66	/* 3.6907V */
#define BATT_LOW		43	/* 3.6566V */
#define BATT_CRITICAL		8	/* 3.6037V */
#define BATT_MINIMUM		(-28)	/* 3.554V */
#define BATT_OFF		(-128)	/* 3.4029V */

/*
 * ADC channel
 */
enum adc_channel_type{
	S3C_ADC_VOLTAGE = 0,
	S3C_ADC_CHG_CURRENT = 2,
	S3C_ADC_EAR = 3,
	S3C_ADC_TEMPERATURE = 6,
	S3C_ADC_V_F,
	ENDOFADC
};

enum {
	BATT_VOL = 0,
	BATT_VOL_ADC,
	BATT_VOL_ADC_CAL,
	BATT_TEMP,
	BATT_TEMP_ADC,
	BATT_TEMP_ADC_CAL,
	BATT_VOL_ADC_AVER,
	BATT_CHARGING_SOURCE,
	BATT_VIBRATOR,
	BATT_CAMERA,
	BATT_MP3,
	BATT_VIDEO,
	BATT_VOICE_CALL_2G,
	BATT_VOICE_CALL_3G,
	BATT_DATA_CALL,
	BATT_DEV_STATE,
	BATT_COMPENSATION,
	BATT_BOOTING,
	BATT_FG_SOC,
	BATT_RESET_SOC,
};

enum {
    CHARGING_MODE_BOOTING,
    BATT_TEMP_CHECK,
    BATT_FULL_CHECK,
};

#define TOTAL_CHARGING_TIME	(6*60*60)	/* 6 hours */
#define TOTAL_RECHARGING_TIME	  (90*60)	/* 1.5 hours */

#define COMPENSATE_VIBRATOR		19
#define COMPENSATE_CAMERA		25
#define COMPENSATE_MP3			17
#define COMPENSATE_VIDEO		28
#define COMPENSATE_VOICE_CALL_2G	13
#define COMPENSATE_VOICE_CALL_3G	14
#define COMPENSATE_DATA_CALL		25
#define COMPENSATE_LCD			0
#define COMPENSATE_TA			0
#define COMPENSATE_CAM_FALSH		0
#define COMPENSATE_BOOTING		52

#define SOC_LB_FOR_POWER_OFF		27

#define RECHARGE_COND_VOLTAGE		4130000
#define RECHARGE_COND_TIME		(30*1000)	/* 30 seconds */
