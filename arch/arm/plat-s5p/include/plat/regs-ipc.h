/* linux/arch/arm/plat-s5p/include/plat/regs-ipc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Register definition file for IPC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _REGS_IPC_H
#define _REGS_IPC_H

#define S3C_IPCREG(x)	(x)

#define IPC_2D_ENABLE	0x10000
#define IPC_HOR_SCALING_ENABLE	0x8000

/*************************************************************************
 * Registers
 ************************************************************************/
#define S3C_IPC_ENABLE				S3C_IPCREG(0x00)
#define S3C_IPC_SRESET				S3C_IPCREG(0x04)
#define S3C_IPC_SHADOW_UPDATE			S3C_IPCREG(0x08)
#define S3C_IPC_FIELD_ID			S3C_IPCREG(0x0c)
#define S3C_IPC_MODE				S3C_IPCREG(0x10)
#define S3C_IPC_PEL_RATE_CTRL			S3C_IPCREG(0x1C)
#define S3C_IPC_ENDIAN_MODE			S3C_IPCREG(0x3C)
#define S3C_IPC_SRC_WIDTH			S3C_IPCREG(0x4C)
#define S3C_IPC_SRC_HEIGHT			S3C_IPCREG(0x50)
#define S3C_IPC_DST_WIDTH			S3C_IPCREG(0x5C)
#define S3C_IPC_DST_HEIGHT			S3C_IPCREG(0x60)
#define S3C_IPC_H_RATIO				S3C_IPCREG(0x64)
#define S3C_IPC_V_RATIO				S3C_IPCREG(0x68)

#define S3C_IPC_POLY8_Y0_LL			S3C_IPCREG(0x6C)
#define S3C_IPC_POLY8_Y0_LH			S3C_IPCREG(0x70)
#define S3C_IPC_POLY8_Y0_HL			S3C_IPCREG(0x74)
#define S3C_IPC_POLY8_Y0_HH			S3C_IPCREG(0x78)
#define S3C_IPC_POLY8_Y1_LL			S3C_IPCREG(0x7C)
#define S3C_IPC_POLY8_Y1_LH			S3C_IPCREG(0x80)
#define S3C_IPC_POLY8_Y1_HL			S3C_IPCREG(0x84)
#define S3C_IPC_POLY8_Y1_HH			S3C_IPCREG(0x88)
#define S3C_IPC_POLY8_Y2_LL			S3C_IPCREG(0x8C)
#define S3C_IPC_POLY8_Y2_LH			S3C_IPCREG(0x90)
#define S3C_IPC_POLY8_Y2_HL			S3C_IPCREG(0x94)
#define S3C_IPC_POLY8_Y2_HH			S3C_IPCREG(0x98)
#define S3C_IPC_POLY8_Y3_LL			S3C_IPCREG(0x9C)
#define S3C_IPC_POLY8_Y3_LH			S3C_IPCREG(0xA0)
#define S3C_IPC_POLY8_Y3_HL			S3C_IPCREG(0xA4)
#define S3C_IPC_POLY8_Y3_HH			S3C_IPCREG(0xA8)
#define S3C_IPC_POLY4_Y0_LL			S3C_IPCREG(0xEC)
#define S3C_IPC_POLY4_Y0_LH			S3C_IPCREG(0xF0)
#define S3C_IPC_POLY4_Y0_HL			S3C_IPCREG(0xF4)
#define S3C_IPC_POLY4_Y0_HH			S3C_IPCREG(0xF8)
#define S3C_IPC_POLY4_Y1_LL			S3C_IPCREG(0xFC)
#define S3C_IPC_POLY4_Y1_LH			S3C_IPCREG(0x100)
#define S3C_IPC_POLY4_Y1_HL			S3C_IPCREG(0x104)
#define S3C_IPC_POLY4_Y1_HH			S3C_IPCREG(0x108)
#define S3C_IPC_POLY4_Y2_LL			S3C_IPCREG(0x10C)
#define S3C_IPC_POLY4_Y2_LH			S3C_IPCREG(0x110)
#define S3C_IPC_POLY4_Y2_HL			S3C_IPCREG(0x114)
#define S3C_IPC_POLY4_Y2_HH			S3C_IPCREG(0x118)
#define S3C_IPC_POLY4_Y3_LL			S3C_IPCREG(0x11C)
#define S3C_IPC_POLY4_Y3_LH			S3C_IPCREG(0x120)
#define S3C_IPC_POLY4_Y3_HL			S3C_IPCREG(0x124)
#define S3C_IPC_POLY4_Y3_HH			S3C_IPCREG(0x128)
#define S3C_IPC_POLY4_C0_LL			S3C_IPCREG(0x12C)
#define S3C_IPC_POLY4_C0_LH			S3C_IPCREG(0x130)
#define S3C_IPC_POLY4_C0_HL			S3C_IPCREG(0x134)
#define S3C_IPC_POLY4_C0_HH			S3C_IPCREG(0x138)
#define S3C_IPC_POLY4_C1_LL			S3C_IPCREG(0x13C)
#define S3C_IPC_POLY4_C1_LH			S3C_IPCREG(0x140)
#define S3C_IPC_POLY4_C1_HL			S3C_IPCREG(0x144)
#define S3C_IPC_POLY4_C1_HH			S3C_IPCREG(0x148)
#define S3C_IPC_BYPASS				S3C_IPCREG(0x200)
#define S3C_IPC_PP_SATURATION			S3C_IPCREG(0x20C)
#define S3C_IPC_PP_SHARPNESS			S3C_IPCREG(0x210)
#define S3C_IPC_PP_LINE_EQ0			S3C_IPCREG(0x218)
#define S3C_IPC_PP_LINE_EQ1			S3C_IPCREG(0x21C)
#define S3C_IPC_PP_LINE_EQ2			S3C_IPCREG(0x220)
#define S3C_IPC_PP_LINE_EQ3			S3C_IPCREG(0x224)
#define S3C_IPC_PP_LINE_EQ4			S3C_IPCREG(0x228)
#define S3C_IPC_PP_LINE_EQ5			S3C_IPCREG(0x22C)
#define S3C_IPC_PP_LINE_EQ6			S3C_IPCREG(0x230)
#define S3C_IPC_PP_LINE_EQ7			S3C_IPCREG(0x234)
#define S3C_IPC_PP_BRIGHT_OFFSET		S3C_IPCREG(0x238)
#define S3C_IPC_VERSION_INFO			S3C_IPCREG(0x3FC)

/* Bit Definitions
 ************************************************************************/
/* ENABLE/DISABLE CONTROL */
#define S3C_IPC_ON		(1 << 0)
#define S3C_IPC_OFF		(0 << 0)

/* SOFTWARE RESET */
#define S3C_IPC_SRESET_ENABLE		(1 << 0)
#define S3C_IPC_SRESET_MASK		(1 << 0)

/* SHADOW UPDATE ENABLE CONTROL */
#define S3C_IPC_SHADOW_UPDATE_ENABLE		(1 << 0)
#define S3C_IPC_SHADOW_UPDATE_DISABLE		(0 << 0)

/* OPERATION MODE CONTROL */
#define S3C_IPC_2D_ENABLE		(1 << 0)
#define S3C_IPC_2D_DISABLE		(0 << 0)
#define S3C_IPC_2D_MASK			(1 << 1)

/* VERTICAL SCALER PIXEL RATE CONTROL */
#define S3C_IPC_PEL_RATE_SET		(0 << 0)

/* HORIZONTAL ZOOM RATIO */
#define S3C_IPC_H_RATIO_MASK		(0x7fff << 0)
#define S3C_IPC_V_RATIO_MASK		(0x7fff << 0)

/* POST PROCESSING IMAGE BYPASS MODE CONTROL */
#define S3C_IPC_PP_BYPASS_ENABLE		(0 << 0)
#define S3C_IPC_PP_BYPASS_DISABLE		(1 << 0)
#define S3C_IPC_PP_BYPASS_MASK			(1 << 0)

/* BRIGHTNESS AND CONTRAST CONTROL */
#define S3C_IPC_PP_LINE_CONTRAST_MASK		(0xff << 0)
#define S3C_IPC_PP_LINE_BRIGTHNESS_MASK		(0xffff << 8)

/*************************************************************************/
/* Macro part 							*/
/*************************************************************************/
#define S3C_IPC_FIELD_ID_SELECTION(x)		((x) << 6)
#define S3C_IPC_FIELD_ID_AUTO_TOGGLING(x)	((x) << 2)
#define S3C_IPC_2D_CTRL(x)			((x) << 1)
#define S3C_IPC_SRC_WIDTH_SET(x)		((x) & 0x7ff << 0)
#define S3C_IPC_SRC_HEIGHT_SET(x)		((x) & 0x3ff << 0)
#define S3C_IPC_DST_WIDTH_SET(x)		((x) & 0x7ff << 0)
#define S3C_IPC_DST_HEIGHT_SET(x)		((x) & 0x3ff << 0)
#define S3C_IPC_PP_SATURATION_SET(x)		((x) & 0xff << 0)
#define S3C_IPC_PP_TH_HNOISE_SET(x)		((x) & 0xff << 8)
#define S3C_IPC_PP_SHARPNESS_SET(x)		((x) & 0x3 << 8)
#define S3C_IPC_PP_BRIGHT_OFFSET_SET(x)		((x) & 0x1ff << 8)
#define S3C_IPC_PP_LINE_CONTRAST(x)\
	(((x) & S3C_IPC_PP_LINE_CONTRAST_MASK) << 0)
#define S3C_IPC_PP_LINE_BRIGHT(x)\
	(((x) & S3C_IPC_PP_LINE_BRIGTHNESS_MASK) << 8)

#endif /* _REGS_IPC_H */

