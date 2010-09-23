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
 * Temperature Table
 */
const int temper_table[][2] =  {
	/* ADC, Temperature (C/10) */
	{ 1715,		(-100)	},
	{ 1691,		(-90)	},
	{ 1669,		(-80)	},
	{ 1648,		(-70)	},
	{ 1631,		(-60)	},
	{ 1617,		(-50)	},
	{ 1591,		(-40)	},
	{ 1565,		(-30)	},
	{ 1539,		(-20)	},
	{ 1513,		(-10)	},
	{ 1489,		0	},
	{ 1465,		10	},
	{ 1441,		20	},
	{ 1417,		30	},
	{ 1394,		40	},
	{ 1371,		50	},
	{ 1359,		60	},
	{ 1340,		70	},
	{ 1321,		80	},
	{ 1302,		90	},
	{ 1288,		100	},
	{ 1266,		110	},
	{ 1245,		120	},
	{ 1225,		130	},
	{ 1206,		140	},
	{ 1188,		150	},
	{ 1164,		160	},
	{ 1141,		170	},
	{ 1119,		180	},
	{ 1098,		190	},
	{ 1078,		200	},
	{ 1052,		210	},
	{ 1027,		220	},
	{ 1003,		230	},
	{  980,		240	},
	{  958,		250	},
	{  933,		260	},
	{  909,		270	},
	{  887,		280	},
	{  866,		290	},
	{  845,		300	},
	{  821,		310	},
	{  799,		320	},
	{  778,		330	},
	{  758,		340	},
	{  739,		350	},
	{  715,		360	},
	{  692,		370	},
	{  671,		380	},
	{  653,		390	},
	{  637,		400	},
	{  616,		410	},
	{  596,		420	},
	{  577,		430	},
	{  558,		440	},
	{  539,		450	},
	{  519,		460	},
	{  502,		470	},
	{  486,		480	},
	{  470,		490	},
	{  454,		500	},
	{  440,		510	},
	{  426,		520	},
	{  412,		530	},
	{  398,		540	},
	{  385,		550	},
	{  371,		560	},
	{  357,		570	},
	{  343,		580	},
	{  329,		590	},
	{  316,		600	},
	{  300,		610	},
	{  285,		620	},
	{  270,		630	},
	{  366,		640	},
	{  264,		650	},
	{  253,		650	},
	{  244,		650	},
	{  235,		650	},
	{  227,		650	},
	{  219,		700	},
};

#define TEMP_IDX_ZERO_CELSIUS	10
#define TEMP_EVENT_HIGH_BLOCK	temper_table[TEMP_IDX_ZERO_CELSIUS+65][0]
#define TEMP_EVENT_HIGH_RECOVER	temper_table[TEMP_IDX_ZERO_CELSIUS+45][0]
#define TEMP_IDLE_HIGH_BLOCK	temper_table[TEMP_IDX_ZERO_CELSIUS+45][0]
#define TEMP_IDLE_HIGH_RECOVER	temper_table[TEMP_IDX_ZERO_CELSIUS+40][0]
#define TEMP_LOW_BLOCK		temper_table[TEMP_IDX_ZERO_CELSIUS][0]
#define TEMP_LOW_RECOVER	temper_table[TEMP_IDX_ZERO_CELSIUS+2][0]

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

enum charger_type_t{
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC,
	CHARGER_DISCHARGE
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

#define TOTAL_CHARGING_TIME	(6*60*60*1000)	/* 6 hours */
#define TOTAL_RECHARGING_TIME	  (90*60*1000)	/* 1.5 hours */

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

#define FULL_CHARGE_COND_VOLTAGE	4000
#define RECHARGE_COND_VOLTAGE		4130
#define RECHARGE_COND_TIME		(30*1000)	/* 30 seconds */
