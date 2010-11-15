/*
 *  STMicroelectronics kr3dm acceleration sensor driver
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */


/* kr3dm i2c slave address & etc */
#define KR3DM_I2C_ADDR          0x09
/* kr3dm registers */
#define WHO_AM_I		0x0F
#define CTRL_REG1		0x20    /* power control reg */
#define CTRL_REG2		0x21    /* power control reg */
#define CTRL_REG3		0x22    /* power control reg */
#define CTRL_REG4		0x23    /* interrupt control reg */
#define CTRL_REG5		0x24    /* interrupt control reg */
#define STATUS_REG		0x27
#define AXISDATA_REG		0x28
#define OUT_X			0x29
#define OUT_Y			0x2B
#define OUT_Z			0x2D
#define INT1_CFG		0x30
#define INT1_SOURCE		0x31
#define INT1_THS		0x32
#define INT1_DURATION		0x33
#define INT2_CFG		0x34
#define INT2_SOURCE		0x35
#define INT2_THS		0x36
#define INT2_DURATION		0x37

#define KR3DM_G_2G		0x00
#define KR3DM_G_4G		0x10
#define KR3DM_G_8G		0x30

/* CTRL_REG1 */
/* ctrl 1: pm2 pm1 pm0 dr1 dr0 x-enable y-enable z-enable */
#define PM_OFF			0x00
#define ENABLE_ALL_AXES		0x07

#define ODRHALF		0x40  /* 0.5Hz output data rate */
#define ODR1		0x60  /* 1Hz output data rate */
#define ODR2		0x80  /* 2Hz output data rate */
#define ODR5		0xA0  /* 5Hz output data rate */
#define ODR10		0xC0  /* 10Hz output data rate */
#define ODR50		0x20  /* 50Hz output data rate */
#define ODR100		0x28  /* 100Hz output data rate */
#define ODR400		0x30  /* 400Hz output data rate */

#define ODR_MASK        0xf8

#define CTRL_REG1_PM2		(1 << 7)
#define CTRL_REG1_PM1		(1 << 6)
#define CTRL_REG1_PM0		(1 << 5)
#define CTRL_REG1_DR1		(1 << 4)
#define CTRL_REG1_DR0		(1 << 3)
#define CTRL_REG1_Zen		(1 << 2)
#define CTRL_REG1_Yen		(1 << 1)
#define CTRL_REG1_Xen		(1 << 0)

#define PM_down			0x00
#define PM_Normal		(CTRL_REG1_PM0)
#define PM_Low05		(CTRL_REG1_PM1)
#define PM_Low1			(CTRL_REG1_PM1|CTRL_REG1_PM0)
#define PM_Low2			(CTRL_REG1_PM2)
#define PM_Low5			(CTRL_REG1_PM2|CTRL_REG1_PM0)
#define PM_Low10		(CTRL_REG1_PM2|CTRL_REG1_PM1)

/* CTRL_REG2 */
#define CTRL_REG2_BOOT		(1 << 7)
#define CTRL_REG2_HPM1		(1 << 6)
#define CTRL_REG2_HPM0		(1 << 5)
#define CTRL_REG2_FDS		(1 << 4)
#define CTRL_REG2_HPen2		(1 << 3)
#define CTRL_REG2_HPen1		(1 << 2)
#define CTRL_REG2_HPCF1		(1 << 1)
#define CTRL_REG2_HPCF0		(1 << 0)

#define HPM_Normal		(CTRL_REG2_HPM1)
#define HPM_Filter		(CTRL_REG2_HPM0)

#define HPCF_ft8		0x00
#define HPCF_ft4		(CTRL_REG2_HPCF0)
#define HPCF_ft2		(CTRL_REG2_HPCF1)
#define HPCF_ft1		(CTRL_REG2_HPCF1|CTRL_REG2_HPCF0)

/* CTRL_REG3 */
#define CTRL_REG3_IHL		(1 << 7)
#define CTRL_REG3_PP_OD		(1 << 6)
#define CTRL_REG3_LIR2		(1 << 5)
#define CTRL_REG3_I2_CFG1	(1 << 4)
#define ICTRL_REG3_2_CFG0	(1 << 3)
#define CTRL_REG3_LIR1		(1 << 2)
#define CTRL_REG3_I1_CFG1	(1 << 1)
#define CTRL_REG3_I1_CFG0	(1 << 0)

/* Interrupt 1 (2) source */
#define I1_CFG_SC		(0x00)
/* Interrupt 1 source OR Interrupt 2 source */
#define I1_CFG_OR		(CTRL_REG3_I1_CFG0)
/* Data Ready */
#define I1_CFG_DR		(CTRL_REG3_I1_CFG1)
/* Boot running */
#define I1_CFG_BR		(CTRL_REG3_I1_CFG1|CTRL_REG3_I1_CFG0)

 /* Interrupt 1 (2) source */
#define I2_CFG_SC		(0x00)
/* Interrupt 1 source OR Interrupt 2 source */
#define I2_CFG_OR		(CTRL_REG3_I2_CFG0)
/* Data Ready */
#define I2_CFG_DR		(CTRL_REG3_I2_CFG1)
/* Boot running */
#define I2_CFG_BR		(CTRL_REG3_I2_CFG1|CTRL_REG3_I2_CFG0)

/* CTRL_REG4 */
#define CTRL_REG4_FS1		(1 << 5)
#define CTRL_REG4_FS0		(1 << 4)
#define CTRL_REG4_STsign	(1 << 3)
#define CTRL_REG4_ST		(1 << 1)
#define CTRL_REG4_SIM		(1 << 0)

#define FS2g			0x00
#define FS4g			(CTRL_REG4_FS0)
#define FS8g			(CTRL_REG4_FS1|CTRL_REG4_FS0)

/* CTRL_REG5 */
#define CTRL_REG5_TurnOn1	(1 << 1)
#define CTRL_REG5_TurnOn0	(1 << 0)

/* STATUS_REG */
#define ZYXOR			(1 << 7)
#define ZOR			(1 << 6)
#define YOR			(1 << 5)
#define XOR			(1 << 4)
#define ZYXDA			(1 << 3)
#define ZDA			(1 << 2)
#define YDA			(1 << 1)
#define XDA			(1 << 0)

/* INT1/2_CFG */
#define INT_CFG_AOI		(1 << 7)
#define INT_CFG_6D		(1 << 6)
#define INT_CFG_ZHIE		(1 << 5)
#define INT_CFG_ZLIE		(1 << 4)
#define INT_CFG_YHIE		(1 << 3)
#define INT_CFG_YLIE		(1 << 2)
#define INT_CFG_XHIE		(1 << 1)
#define INT_CFG_XLIE		(1 << 0)

/* INT1/2_SRC */
#define IA			(1 << 6)
#define ZH			(1 << 5)
#define ZL			(1 << 4)
#define YH			(1 << 3)
#define YL			(1 << 2)
#define XH			(1 << 1)
#define XL			(1 << 0)

/* Register Auto-increase */
#define AC			(1 << 7)
