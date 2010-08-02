/* linux/arch/arm/mach-s5pv210/include/mach/pm-core.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Based on arch/arm/plat-s3c24xx/include/plat/pm-core.h,
 * Copyright 2008-2009 Simtec Electronics Ben Dooks <ben@simtec.co.uk>
 * S5P series driver for power management
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

static inline void s3c_pm_debug_init_uart(void)
{
}

static inline void s3c_pm_arch_prepare_irqs(void)
{
	__raw_writel(s3c_irqwake_intmask, S5P_WAKEUP_MASK);
	__raw_writel(s3c_irqwake_eintmask, S5P_EINT_WAKEUP_MASK);

	/* ack any outstanding external interrupts before we go to sleep */

}

static inline void s3c_pm_arch_stop_clocks(void)
{
}

static void s3c_pm_show_resume_irqs(int start, unsigned long which,
				    unsigned long mask);

static inline void s3c_pm_arch_show_resume_irqs(void)
{
}

/* make these defines, we currently do not have any need to change
 * the IRQ wake controls depending on the CPU we are running on */

/* 2009.04.27 by icarus : don't use this definition, use extern var */
#if 0
#define s3c_irqwake_eintallow	(0xFFFFFFFF)
#define s3c_irqwake_intallow	(0)
#endif

static inline void s3c_pm_arch_update_uart(void __iomem *regs,
					   struct pm_uart_save *save)
{
}
