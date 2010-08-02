/* linux/arch/arm/plat-s5p/include/plat/regs-csis.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com/
 *
 * S5PV210 - Register definition file for MIPI-CSI2 Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _REGS_CSIS_H
#define _REGS_CSIS_H

#define S3C_CSISREG(x)	(x)

/*************************************************************************
 * Registers
 ************************************************************************/
#define S3C_CSIS_CONTROL			S3C_CSISREG(0x00)
#define S3C_CSIS_DPHYCTRL			S3C_CSISREG(0x04)
#define S3C_CSIS_CONFIG				S3C_CSISREG(0x08)
#define S3C_CSIS_DPHYSTS			S3C_CSISREG(0x0c)
#define S3C_CSIS_INTMSK				S3C_CSISREG(0x10)
#define S3C_CSIS_INTSRC				S3C_CSISREG(0x14)
#define S3C_CSIS_RESOL				S3C_CSISREG(0x2c)

/*************************************************************************
 * Bit Definitions
 ************************************************************************/
/* Control Register */
#define S3C_CSIS_CONTROL_DPDN_DEFAULT		(0 << 31)
#define S3C_CSIS_CONTROL_DPDN_SWAP		(1 << 31)
#define S3C_CSIS_CONTROL_ALIGN_32BIT		(1 << 20)
#define S3C_CSIS_CONTROL_ALIGN_24BIT		(0 << 20)
#define S3C_CSIS_CONTROL_ALIGN_MASK		(1 << 20)
#define S3C_CSIS_CONTROL_UPDATE_SHADOW		(1 << 16)
#define S3C_CSIS_CONTROL_WCLK_PCLK		(0 << 8)
#define S3C_CSIS_CONTROL_WCLK_EXTCLK		(1 << 8)
#define S3C_CSIS_CONTROL_WCLK_MASK		(1 << 8)
#define S3C_CSIS_CONTROL_RESET			(1 << 4)
#define S3C_CSIS_CONTROL_DISABLE		(0 << 0)
#define S3C_CSIS_CONTROL_ENABLE			(1 << 0)

/* D-PHY Control Register */
#define S3C_CSIS_DPHYCTRL_HS_SETTLE_MASK	(0x1f << 27)
#define S3C_CSIS_DPHYCTRL_HS_SETTLE_SHIFT	(27)
#define S3C_CSIS_DPHYCTRL_DISABLE		(0xf << 0)
#define S3C_CSIS_DPHYCTRL_ENABLE		(0xf << 0)

/* Configuration Register */
#define S3C_CSIS_CONFIG_FORMAT_SHIFT		(2)
#define S3C_CSIS_CONFIG_FORMAT_MASK		(0x1f << 2)
#define S3C_CSIS_CONFIG_NR_LANE_1		(0 << 0)
#define S3C_CSIS_CONFIG_NR_LANE_2		(1 << 0)
#define S3C_CSIS_CONFIG_NR_LANE_MASK		(1 << 0)

/* D-PHY Status Register */
#define S3C_CSIS_DPHYSTS_STOPSTATE_LANE1	(1 << 5)
#define S3C_CSIS_DPHYSTS_STOPSTATE_LANE0	(1 << 4)
#define S3C_CSIS_DPHYSTS_STOPSTATE_CLOCK	(1 << 0)

/* Interrupt Mask Register */
#define S3C_CSIS_INTMSK_EVEN_BEFORE_DISABLE	(0 << 31)
#define S3C_CSIS_INTMSK_EVEN_BEFORE_ENABLE	(1 << 31)
#define S3C_CSIS_INTMSK_EVEN_AFTER_DISABLE	(0 << 30)
#define S3C_CSIS_INTMSK_EVEN_AFTER_ENABLE	(1 << 30)
#define S3C_CSIS_INTMSK_ODD_BEFORE_DISABLE	(0 << 29)
#define S3C_CSIS_INTMSK_ODD_BEFORE_ENABLE	(1 << 29)
#define S3C_CSIS_INTMSK_ODD_AFTER_DISABLE	(0 << 28)
#define S3C_CSIS_INTMSK_ODD_AFTER_ENABLE	(1 << 28)
#define S3C_CSIS_INTMSK_ERR_SOT_HS_DISABLE	(0 << 12)
#define S3C_CSIS_INTMSK_ERR_SOT_HS_ENABLE	(1 << 12)
#define S3C_CSIS_INTMSK_ERR_ESC_DISABLE		(0 << 8)
#define S3C_CSIS_INTMSK_ERR_ESC_ENABLE		(1 << 8)
#define S3C_CSIS_INTMSK_ERR_CTRL_DISABLE	(0 << 4)
#define S3C_CSIS_INTMSK_ERR_CTRL_ENABLE		(1 << 4)
#define S3C_CSIS_INTMSK_ERR_ECC_DISABLE		(0 << 2)
#define S3C_CSIS_INTMSK_ERR_ECC_ENABLE		(1 << 2)
#define S3C_CSIS_INTMSK_ERR_CRC_DISABLE		(0 << 1)
#define S3C_CSIS_INTMSK_ERR_CRC_ENABLE		(1 << 1)
#define S3C_CSIS_INTMSK_ERR_ID_DISABLE		(0 << 0)
#define S3C_CSIS_INTMSK_ERR_ID_ENABLE		(1 << 0)

/* Interrupt Source Register */
#define S3C_CSIS_INTSRC_EVEN_BEFORE		(1 << 31)
#define S3C_CSIS_INTSRC_EVEN_AFTER		(1 << 30)
#define S3C_CSIS_INTSRC_ODD_BEFORE		(1 << 29)
#define S3C_CSIS_INTSRC_ODD_AFTER		(1 << 28)
#define S3C_CSIS_INTSRC_ERR_SOT_HS_LANE1	(1 << 13)
#define S3C_CSIS_INTSRC_ERR_SOT_HS_LANE0	(1 << 12)
#define S3C_CSIS_INTSRC_ERR_ESC_LANE1		(1 << 9)
#define S3C_CSIS_INTSRC_ERR_ESC_LANE0		(1 << 8)
#define S3C_CSIS_INTSRC_ERR_CTRL1		(1 << 5)
#define S3C_CSIS_INTSRC_ERR_CTRL0		(1 << 4)
#define S3C_CSIS_INTSRC_ERR_ECC			(1 << 2)
#define S3C_CSIS_INTSRC_ERR_CRC			(1 << 1)
#define S3C_CSIS_INTSRC_ERR_ID			(1 << 0)

/* Resolution Register */
#define S3C_CSIS_RESOL_HOR_SHIFT		(16)
#define S3C_CSIS_RESOL_VER_SHIFT		(0)

#endif /* _REGS_CSIS_H */
