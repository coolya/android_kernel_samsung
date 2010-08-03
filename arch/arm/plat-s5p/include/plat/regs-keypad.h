/* linux/arch/arm/plat-s5p/include/plat/regs-keypad.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Register definition file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_KEYPAD_H
#define __ASM_ARCH_REGS_KEYPAD_H

/*
 * Keypad Interface
 */
#define S3C_KEYPADREG(x)	(x)

#define S3C_KEYIFCON		S3C_KEYPADREG(0x00)
#define S3C_KEYIFSTSCLR		S3C_KEYPADREG(0x04)
#define S3C_KEYIFCOL		S3C_KEYPADREG(0x08)
#define S3C_KEYIFROW		S3C_KEYPADREG(0x0C)
#define S3C_KEYIFFC		S3C_KEYPADREG(0x10)

#define KEYCOL_DMASK            (0xff)
#if defined(CONFIG_KEYPAD_S3C_MSM)
#define KEYROW_DMASK		(0x3fff) /*msm interface for s5pv210 */
#else
#if defined(CONFIG_ARCH_S5PV210)
#define KEYROW_DMASK		(0x3fff)
#else
#define KEYROW_DMASK		(0xff)
#endif
#endif
#define	INT_F_EN		(1<<0)	/*falling edge(key-pressed) interuppt enable*/
#define	INT_R_EN		(1<<1)	/*rising edge(key-released) interuppt enable*/
#define	DF_EN			(1<<2)	/*debouncing filter enable*/
#define	FC_EN			(1<<3)	/*filter clock enable*/
#define	KEYIFCON_INIT		(KEYIFCON_CLEAR | INT_F_EN|INT_R_EN|DF_EN|FC_EN)
#if defined(CONFIG_ARCH_S5PV210)
#define KEYIFSTSCLR_CLEAR	(0x3fffffff)
#define KEYIFCOL_RESET	((0X0<<8) | (0X0<<9))
#else
#define KEYIFSTSCLR_CLEAR	(0xffff)
#endif

#endif /* __ASM_ARCH_REGS_KEYPAD_H */


