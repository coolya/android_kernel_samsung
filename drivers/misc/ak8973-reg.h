/* linux/drivers/misc/ak8973-reg.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __AK8973_REG__
#define __AK8983_REG__

/* Compass device dependent definition */
#define AK8973_MODE_MEASURE		0x00	/* Starts measurement. */
#define AK8973_MODE_E2P_READ		0x02	/* E2P access mode (read). */
#define AK8973_MODE_POWERDOWN		0x03	/* Power down mode */

/* Rx buffer size. i.e ST,TMPS,H1X,H1Y,H1Z*/
#define SENSOR_DATA_SIZE		5

/* Read/Write buffer size.*/
#define RWBUF_SIZE			16

/* AK8973 register address */
#define AK8973_REG_ST			0xC0
#define AK8973_REG_TMPS			0xC1
#define AK8973_REG_H1X			0xC2
#define AK8973_REG_H1Y			0xC3
#define AK8973_REG_H1Z			0xC4

#define AK8973_REG_MS1			0xE0
#define AK8973_REG_HXDA			0xE1
#define AK8973_REG_HYDA			0xE2
#define AK8973_REG_HZDA			0xE3
#define AK8973_REG_HXGA			0xE4
#define AK8973_REG_HYGA			0xE5
#define AK8973_REG_HZGA			0xE6

#define AK8973_EEP_ETS			0x62
#define AK8973_EEP_EVIR			0x63
#define AK8973_EEP_EIHE			0x64
#define AK8973_EEP_ETST			0x65
#define AK8973_EEP_EHXGA		0x66
#define AK8973_EEP_EHYGA		0x67
#define AK8973_EEP_EHZGA		0x68

#endif /* __AK8983_REG__ */
