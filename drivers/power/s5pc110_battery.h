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
	{ 1697,		(-90)	},
	{ 1678,		(-80)	},
	{ 1659,		(-70)	},
	{ 1641,		(-60)	},
	{ 1623,		(-50)	},
	{ 1604,		(-40)	},
	{ 1585,		(-30)	},
	{ 1567,		(-20)	},
	{ 1544,		(-10)	},
	{ 1533,		0	},
	{ 1514,		10	},
	{ 1498,		20	},
	{ 1474,		30	},
	{ 1450,		40	},
	{ 1427,		50	},
	{ 1405,		60	},
	{ 1384,		70	},
	{ 1362,		80	},
	{ 1341,		90	},
	{ 1320,		100	},
	{ 1297,		110	},
	{ 1274,		120	},
	{ 1251,		130	},
	{ 1228,		140	},
	{ 1206,		150	},
	{ 1182,		160	},
	{ 1158,		170	},
	{ 1134,		180	},
	{ 1110,		190	},
	{ 1086,		200	},
	{ 1062,		210	},
	{ 1038,		220	},
	{ 1014,		230	},
	{  990,		240	},
	{  966,		250	},
	{  942,		260	},
	{  918,		270	},
	{  894,		280	},
	{  870,		290	},
	{  847,		300	},
	{  825,		310	},
	{  803,		320	},
	{  782,		330	},
	{  760,		340	},
	{  739,		350	},
	{  717,		360	},
	{  695,		370	},
	{  674,		380	},
	{  649,		390	},
	{  642,		400	},
	{  618,		410	},
	{  600,		420	},
	{  580,		430	},
	{  559,		440	},
	{  539,		450	},
	{  526,		460	},
	{  513,		470	},
	{  495,		480	},
	{  477,		490	},
	{  459,		500	},
	{  444,		510	},
	{  430,		520	},
	{  415,		530	},
	{  401,		540	},
	{  386,		550	},
	{  374,		560	},
	{  362,		570	},
	{  350,		580	},
	{  339,		590	},
	{  327,		600	},
	{  314,		610	},
	{  300,		620	},
	{  287,		630	},
	{  274,		640	},
	{  261,		650	},
	{  253,		660	},
	{  245,		670	},
	{  238,		680	},
	{  230,		690	},
	{  222,		700	},
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
