/* linux/arch/arm/plat-s5p/include/plat/regs-systimer.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * S5P System Timer Driver Header information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_PLAT_REGS_SYSTIMER_H
#define __ASM_PLAT_REGS_SYSTIMER_H __FILE__

#define S5P_SYSTIMERREG(x)		(S5P_VA_SYSTIMER + (x))

#define S5P_SYSTIMER_TCFG		S5P_SYSTIMERREG(0x00)
#define S5P_SYSTIMER_TCON		S5P_SYSTIMERREG(0x04)
#define S5P_SYSTIMER_TICNTB		S5P_SYSTIMERREG(0x08)
#define S5P_SYSTIMER_TICNTO		S5P_SYSTIMERREG(0x0c)
#define S5P_SYSTIMER_TFCNTB		S5P_SYSTIMERREG(0x10)
#define S5P_SYSTIMER_ICNTB		S5P_SYSTIMERREG(0x18)
#define S5P_SYSTIMER_ICNTO		S5P_SYSTIMERREG(0x1c)
#define S5P_SYSTIMER_INT_CSTAT		S5P_SYSTIMERREG(0x20)

/* Value for TCFG */

#define S5P_SYSTIMER_SWRST		(1<<16)

#define S5P_SYSTIMER_DIV_GEN		(0<<15)
#define S5P_SYSTIMER_DIV_RTC		(1<<15)

#define S5P_SYSTIMER_TICK_INT		(0<<14)
#define S5P_SYSTIMER_TICK_FRA		(1<<14)

#define S5P_SYSTIMER_TCLK_MASK		(3<<12)
#define S5P_SYSTIMER_TCLK_XXTI		(0<<12)
#define S5P_SYSTIMER_TCLK_RTC		(1<<12)
#define S5P_SYSTIMER_TCLK_USB		(2<<12)
#define S5P_SYSTIMER_TCLK_PCLK		(3<<12)

#define S5P_SYSTIMER_DIV_MASK		(7<<8)
#define S5P_SYSTIMER_DIV_1		(0<<8)
#define S5P_SYSTIMER_DIV_2		(1<<8)
#define S5P_SYSTIMER_DIV_4		(2<<8)
#define S5P_SYSTIMER_DIV_8		(3<<8)
#define S5P_SYSTIMER_DIV_16		(4<<8)

#define S5P_SYSTIMER_TARGET_HZ		200
#define S5P_SYSTIMER_PRESCALER		5
#define S5P_SYSTIMER_PRESCALER_MASK	(0x3f<<0)

/* value for TCON */

#define S5P_SYSTIMER_INT_AUTO		(1<<5)
#define S5P_SYSTIMER_INT_IMM		(1<<4)
#define S5P_SYSTIMER_INT_START		(1<<3)
#define S5P_SYSTIMER_AUTO_RELOAD	(1<<2)
#define S5P_SYSTIMER_IMM_UPDATE		(1<<1)
#define S5P_SYSTIMER_START		(1<<0)

/* Value for INT_CSTAT */

#define S5P_SYSTIMER_INT_TWIE		(1<<10)
#define S5P_SYSTIMER_INT_IWIE		(1<<9)
#define S5P_SYSTIMER_INT_TFWIE		(1<<8)
#define S5P_SYSTIMER_INT_TIWIE		(1<<7)
#define S5P_SYSTIMER_INT_ICNTEIE	(1<<6)
#define S5P_SYSTIMER_INT_TCON		(1<<5)
#define S5P_SYSTIMER_INT_ICNTB		(1<<4)
#define S5P_SYSTIMER_INT_TFCNTB		(1<<3)
#define S5P_SYSTIMER_INT_TICNTB		(1<<2)
#define S5P_SYSTIMER_INT_INTCNT		(1<<1)
#define S5P_SYSTIMER_INT_INTENABLE	(1<<0)

#endif /* __ASM_PLAT_REGS_SYSTIMER_H */
