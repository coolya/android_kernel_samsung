/* linux/arch/arm/plat-s5p/systimer-s5p.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * S5P System Timer
 *
 * Based on linux/arch/arm/plat-samsung/time.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/mach-types.h>

#include <asm/irq.h>
#include <asm/mach/time.h>

#include <mach/map.h>
#include <mach/regs-irq.h>
#include <mach/tick.h>

#include <plat/regs-systimer.h>
#include <plat/clock.h>
#include <plat/cpu.h>

#include <mach/regs-clock.h>

static unsigned long timer_startval;
static unsigned long timer_usec_ticks;
static unsigned long timer_icnt;

#define TICK_MAX		(0xffffffff)
#define TIMER_USEC_SHIFT	16

static unsigned int systimer_write_done(unsigned int value)
{
	unsigned int cnt;
	unsigned int tmp_reg;

	cnt = 1000;

	do {
		cnt--;

		if (__raw_readl(S5P_SYSTIMER_INT_CSTAT) & value) {
			tmp_reg = __raw_readl(S5P_SYSTIMER_INT_CSTAT);
			tmp_reg |= value;
			__raw_writel(tmp_reg , S5P_SYSTIMER_INT_CSTAT);

			return 0;
		}

	} while (cnt > 0);

	printk(KERN_ERR "%s : %d : Timer Expired\n", __func__, value);

	return -ETIME;
}

static unsigned int s5p_systimer_write(void __iomem *reg_offset,
				       unsigned int value)
{
	unsigned int int_cstat;
	unsigned int ret = 0;

	int_cstat = __raw_readl(S5P_SYSTIMER_INT_CSTAT);

	if (reg_offset == S5P_SYSTIMER_TCON) {
		__raw_writel(value, reg_offset);

		if (int_cstat & S5P_SYSTIMER_INT_TWIE)
			ret = systimer_write_done(S5P_SYSTIMER_INT_TCON);

	} else if (reg_offset == S5P_SYSTIMER_ICNTB) {
		__raw_writel(value, reg_offset);

		if (int_cstat & S5P_SYSTIMER_INT_IWIE)
			ret = systimer_write_done(S5P_SYSTIMER_INT_ICNTB);

	} else if (reg_offset == S5P_SYSTIMER_TFCNTB) {
		__raw_writel(value, reg_offset);

		if (int_cstat & S5P_SYSTIMER_INT_TFWIE)
			ret = systimer_write_done(S5P_SYSTIMER_INT_TFCNTB);

	} else if (reg_offset == S5P_SYSTIMER_TICNTB) {
		__raw_writel(value, reg_offset);

		if (int_cstat & S5P_SYSTIMER_INT_TIWIE)
			ret = systimer_write_done(S5P_SYSTIMER_INT_TICNTB);
	} else {
		__raw_writel(value, reg_offset);
	}

	return ret;
}

/*
 * S5P has system timer to use as OS tick Timer.
 * System Timer provides two distincive feature. Accurate timer which provides
 * exact 1ms time tick at any power mode except sleep mode.
 * interrupt interval without stopping reference tick timer.
 */

/*
 * timer_mask_usec_ticks
 *
 * given a clock and divisor, make the value to pass into timer_ticks_to_usec
 * to scale the ticks into usecs
 */
static inline unsigned long timer_mask_usec_ticks(unsigned long scaler,
						  unsigned long pclk)
{
	unsigned long den = pclk / 1000;

	return ((1000 << TIMER_USEC_SHIFT) * scaler + (den >> 1)) / den;
}

/*
 * timer_ticks_to_usec
 *
 * convert timer ticks to usec.
 */
static inline unsigned long timer_ticks_to_usec(unsigned long ticks)
{
	unsigned long res;

	res = ticks * timer_usec_ticks;
	res += 1 << (TIMER_USEC_SHIFT - 4);	/* round up slightly */

	return res >> TIMER_USEC_SHIFT;
}

/*
 * Returns microsecond  since last clock interrupt.  Note that interrupts
 * will have been disabled by do_gettimeoffset()
 * IRQs are disabled before entering here from do_gettimeofday()
 */
static unsigned long s5p_gettimeoffset(void)
{
	unsigned long tdone;
	unsigned long tval;
	unsigned long clk_tick_totcnt;

	clk_tick_totcnt = (timer_icnt + 1) * timer_startval;

	/* work out how many ticks have gone since last timer interrupt */
	tval = __raw_readl(S5P_SYSTIMER_ICNTO) * timer_startval;
	tval += __raw_readl(S5P_SYSTIMER_TICNTO);

	tdone = clk_tick_totcnt - tval;

	/* check to see if there is an interrupt pending */
	if (s5p_ostimer_pending()) {
		/* re-read the timer, and try and fix up for the missed
		 * interrupt. Note, the interrupt may go off before the
		 * timer has re-loaded from wrapping.
		 */

		tval = __raw_readl(S5P_SYSTIMER_ICNTO) * timer_startval;
		tval += __raw_readl(S5P_SYSTIMER_TICNTO);

		tdone = clk_tick_totcnt - tval;

		if (tval != 0)
			tdone += clk_tick_totcnt;
	}

	return timer_ticks_to_usec(tdone);
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t s5p_systimer_interrupt(int irq, void *dev_id)
{
	unsigned int temp_cstat;

	temp_cstat = __raw_readl(S5P_SYSTIMER_INT_CSTAT);
	temp_cstat |= S5P_SYSTIMER_INT_INTCNT;
	s5p_systimer_write(S5P_SYSTIMER_INT_CSTAT, temp_cstat);

	timer_tick();

	return IRQ_HANDLED;
}

static struct irqaction s5p_systimer_irq = {
	.name		= "S5P System Timer",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= s5p_systimer_interrupt,
};

/*
 * Set up timer interrupt, and return the current time in seconds.
 */
static void s5p_systimer_setup(void)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcfg;
	unsigned long int_csata;

	/* clock configuration setting and enable */
	unsigned long pclk;
	struct clk *clk;

	clk = clk_get(NULL, "systimer");
	if (IS_ERR(clk))
		panic("failed to get clock for system timer");

	clk_enable(clk);

	pclk = clk_get_rate(clk);

	tcfg = __raw_readl(S5P_SYSTIMER_TCFG);
	tcfg |= S5P_SYSTIMER_SWRST;
	s5p_systimer_write(S5P_SYSTIMER_TCFG, tcfg);

	tcnt = TICK_MAX;  /* default value for tcnt */

	/* initialize system timer clock */
	tcfg = __raw_readl(S5P_SYSTIMER_TCFG);

	tcfg &= ~S5P_SYSTIMER_TCLK_MASK;
	tcfg |= S5P_SYSTIMER_TCLK_PCLK;

	s5p_systimer_write(S5P_SYSTIMER_TCFG, tcfg);

	/* TCFG must not be changed at run-time.
	 * If you want to change TCFG, stop timer(TCON[0] = 0)
	 */

	s5p_systimer_write(S5P_SYSTIMER_TCON, 0);

	/* read the current timer configuration bits */
	tcon = __raw_readl(S5P_SYSTIMER_TCON);
	tcfg = __raw_readl(S5P_SYSTIMER_TCFG);

	/* configure clock tick */
	timer_usec_ticks = timer_mask_usec_ticks(S5P_SYSTIMER_PRESCALER, pclk);

	tcfg &= ~S5P_SYSTIMER_TCLK_MASK;
	tcfg |= S5P_SYSTIMER_TCLK_PCLK;
	tcfg &= ~S5P_SYSTIMER_PRESCALER_MASK;
	tcfg |= S5P_SYSTIMER_PRESCALER - 1;

	tcnt = ((pclk / S5P_SYSTIMER_PRESCALER) / S5P_SYSTIMER_TARGET_HZ) - 1;

	/* check to see if timer is within 16bit range... */
	if (tcnt > TICK_MAX) {
		panic("setup_timer: HZ is too small, cannot configure timer!");
		return;
	}

	s5p_systimer_write(S5P_SYSTIMER_TCFG, tcfg);

	timer_startval = tcnt;
	s5p_systimer_write(S5P_SYSTIMER_TICNTB, tcnt);

	/* set Interrupt tick value */
	timer_icnt = (S5P_SYSTIMER_TARGET_HZ / HZ) - 1;
	s5p_systimer_write(S5P_SYSTIMER_ICNTB, timer_icnt);

	tcon = (S5P_SYSTIMER_INT_AUTO | S5P_SYSTIMER_START
		| S5P_SYSTIMER_INT_START);
	s5p_systimer_write(S5P_SYSTIMER_TCON, tcon);

	/* Interrupt Start and Enable */
	int_csata = __raw_readl(S5P_SYSTIMER_INT_CSTAT);
	int_csata |= (S5P_SYSTIMER_INT_ICNTEIE | S5P_SYSTIMER_INT_INTENABLE);
	s5p_systimer_write(S5P_SYSTIMER_INT_CSTAT, int_csata);
}

static void __init s5p_systimer_init(void)
{
	s5p_systimer_setup();
	setup_irq(IRQ_SYSTIMER, &s5p_systimer_irq);
}

struct sys_timer s5p_systimer = {
	.init		= s5p_systimer_init,
	.offset		= s5p_gettimeoffset,
	.resume		= s5p_systimer_setup
};
