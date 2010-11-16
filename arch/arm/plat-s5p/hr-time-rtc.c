/*
 * linux/arch/arm/plat-s5p/hr-time-rtc.c
 *
 * S5P Timers
 *
 * Copyright (c) 2006 Samsung Electronics
 *
 *
 * S5P (and compatible) HRT support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/io.h>

#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach-types.h>
#include <mach/map.h>
#include <plat/regs-timer.h>
#include <plat/regs-rtc.h>
#include <plat/regs-systimer.h>
#include <mach/regs-irq.h>
#include <mach/regs-clock.h>
#include <mach/tick.h>

#include <plat/clock.h>
#include <plat/cpu.h>

static unsigned long long time_stamp;
static unsigned long long s5p_sched_timer_overflows;
static unsigned long long old_overflows;
static cycle_t last_ticks;

/* Sched timer interrupt is not processed right after
 * timer counter expired
 */
static unsigned int pending_irq;
#define USE_SYSTIMER_IRQ

/* sched_timer_running
 * 0 : sched timer stopped or not initialized
 * 1 : sched timer started
 */
static unsigned int sched_timer_running;

void __iomem *rtc_base = S5P_VA_RTC;
static struct clk *clk_event;
static struct clk *clk_sched;
static int tick_timer_mode;	/* 0: oneshot, 1: autoreload */

#define RTC_CLOCK	(32768)
#define RTC_DEFAULT_TICK	((RTC_CLOCK / HZ) - 1)
/*
 * Helper functions
 * s5p_systimer_read() : Read from System timer register
 * s5p_systimer_write(): Write to System timer register
 *
 */
static unsigned int s5p_systimer_read(unsigned int *reg_offset)
{
	return __raw_readl(reg_offset);
}

static unsigned int s5p_systimer_write(unsigned int *reg_offset,
					unsigned int value)
{
	unsigned int temp_regs;

	__raw_writel(value, reg_offset);

	if (reg_offset == S5P_SYSTIMER_TCON) {
		while (!(__raw_readl(S5P_SYSTIMER_INT_CSTAT) &
			S5P_SYSTIMER_INT_TCON))
			;
		temp_regs = __raw_readl(S5P_SYSTIMER_INT_CSTAT);
		temp_regs |= S5P_SYSTIMER_INT_TCON;
		__raw_writel(temp_regs, S5P_SYSTIMER_INT_CSTAT);

	} else if (reg_offset == S5P_SYSTIMER_ICNTB) {
		while (!(__raw_readl(S5P_SYSTIMER_INT_CSTAT) &
			S5P_SYSTIMER_INT_ICNTB))
			;
		temp_regs = __raw_readl(S5P_SYSTIMER_INT_CSTAT);
		temp_regs |= S5P_SYSTIMER_INT_ICNTB;
		__raw_writel(temp_regs, S5P_SYSTIMER_INT_CSTAT);

	} else if (reg_offset == S5P_SYSTIMER_TICNTB) {
		while (!(__raw_readl(S5P_SYSTIMER_INT_CSTAT) &
			S5P_SYSTIMER_INT_TICNTB))
			;
		temp_regs = __raw_readl(S5P_SYSTIMER_INT_CSTAT);
		temp_regs |= S5P_SYSTIMER_INT_TICNTB;
		__raw_writel(temp_regs, S5P_SYSTIMER_INT_CSTAT);
	}

	return 0;
}

unsigned int get_rtc_cnt(void)
{
	unsigned int ticcnt, current_cnt, rtccnt;

	ticcnt = __raw_readl(rtc_base + S3C2410_TICNT);
	current_cnt = __raw_readl(rtc_base + S3C2410_CURTICCNT);
	rtccnt = ticcnt - current_cnt;

	return rtccnt;
}

static void s5p_tick_timer_setup(void);

static void s5p_tick_timer_start(unsigned long load_val,
					int autoreset)
{
	unsigned int tmp;

	tmp = __raw_readl(rtc_base + S3C2410_RTCCON) &
		~(S3C_RTCCON_TICEN);
	__raw_writel(tmp, rtc_base + S3C2410_RTCCON);

	__raw_writel(load_val, rtc_base + S3C2410_TICNT);

	tmp |= S3C_RTCCON_TICEN;

	__raw_writel(tmp, rtc_base + S3C2410_RTCCON);
}

static inline void s5p_tick_timer_stop(void)
{
	unsigned int tmp;

	tmp = __raw_readl(rtc_base + S3C2410_RTCCON) &
		~(S3C_RTCCON_TICEN);

	__raw_writel(tmp, rtc_base + S3C2410_RTCCON);
}

static void s5p_sched_timer_start(unsigned long load_val,
					int autoreset)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcfg;

	tcnt = TICK_MAX;  /* default value for tcnt */

	/* initialize system timer clock */
	tcfg = s5p_systimer_read(S5P_SYSTIMER_TCFG);

	tcfg &= ~S5P_SYSTIMER_TCLK_MASK;
	tcfg |= S5P_SYSTIMER_TCLK_USB;

	s5p_systimer_write(S5P_SYSTIMER_TCFG, tcfg);

	/* TCFG must not be changed at run-time.
	 * If you want to change TCFG, stop timer(TCON[0] = 0)
	 */
	s5p_systimer_write(S5P_SYSTIMER_TCON, 0);

	/* read the current timer configuration bits */
	tcon = s5p_systimer_read(S5P_SYSTIMER_TCON);
	tcfg = s5p_systimer_read(S5P_SYSTIMER_TCFG);

	tcfg &= ~S5P_SYSTIMER_TCLK_MASK;
	tcfg |= S5P_SYSTIMER_TCLK_USB;
	tcfg &= ~S5P_SYSTIMER_PRESCALER_MASK;

	/* check to see if timer is within 16bit range... */
	if (tcnt > TICK_MAX) {
		panic("setup_timer: cannot configure timer!");
		return;
	}

	s5p_systimer_write(S5P_SYSTIMER_TCFG, tcfg);

	s5p_systimer_write(S5P_SYSTIMER_TICNTB, tcnt);

#if !defined(USE_SYSTIMER_IRQ)
	/* set timer con */
	tcon =  (S5P_SYSTIMER_START | S5P_SYSTIMER_AUTO_RELOAD);
	s5p_systimer_write(S5P_SYSTIMER_TCON, tcon);
#else
	/* set timer con */
	tcon =  S5P_SYSTIMER_INT_AUTO | S5P_SYSTIMER_START |
		S5P_SYSTIMER_AUTO_RELOAD;
	s5p_systimer_write(S5P_SYSTIMER_TCON, tcon);

	tcon |= S5P_SYSTIMER_INT_START;
	s5p_systimer_write(S5P_SYSTIMER_TCON, tcon);

	/* Interrupt Start and Enable */
	s5p_systimer_write(S5P_SYSTIMER_INT_CSTAT,
			   (S5P_SYSTIMER_INT_ICNTEIE |
			    S5P_SYSTIMER_INT_INTENABLE));
#endif
	sched_timer_running = 1;
}

/*
 * RTC tick : count down to zero, interrupt, reload
 */
static int s5p_tick_set_next_event(unsigned long cycles,
				   struct clock_event_device *evt)
{
	/* printk(KERN_INFO "%d\n", cycles); */
	if  (cycles == 0)	/* Should be larger than 0 */
		cycles = 1;
	s5p_tick_timer_start(cycles, 0);
	return 0;
}

static void s5p_tick_set_mode(enum clock_event_mode mode,
			      struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		tick_timer_mode = 1;
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		s5p_tick_timer_stop();
		tick_timer_mode = 0;
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		/* Sched timer stopped */
		sched_timer_running = 0;

		/* Reset sched_clock variables after sleep/wakeup */
		last_ticks = 0;
		s5p_sched_timer_overflows = 0;
		old_overflows = 0;
		pending_irq = 0;
		break;
	case CLOCK_EVT_MODE_RESUME:
		s5p_tick_timer_setup();
		s5p_sched_timer_start(~0, 1);
		break;
	}
}

static struct clock_event_device clockevent_tick_timer = {
	.name		= "S5PC110 event timer",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.shift		= 32,
	.set_next_event	= s5p_tick_set_next_event,
	.set_mode	= s5p_tick_set_mode,
};

static irqreturn_t s5p_tick_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &clockevent_tick_timer;

	__raw_writel(S3C_INTP_TIC, rtc_base + S3C_INTP);
	/* In case of oneshot mode */
	if (tick_timer_mode == 0)
		s5p_tick_timer_stop();

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction s5p_tick_timer_irq = {
	.name		= "rtc-tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= s5p_tick_timer_interrupt,
};

static void  s5p_init_dynamic_tick_timer(unsigned long rate)
{
	tick_timer_mode = 1;

	s5p_tick_timer_stop();

	s5p_tick_timer_start((rate / HZ) - 1, 1);

	clockevent_tick_timer.mult = div_sc(rate, NSEC_PER_SEC,
					    clockevent_tick_timer.shift);
	clockevent_tick_timer.max_delta_ns =
		clockevent_delta2ns(-1, &clockevent_tick_timer);
	clockevent_tick_timer.min_delta_ns =
		clockevent_delta2ns(1, &clockevent_tick_timer);

	clockevent_tick_timer.cpumask = cpumask_of(0);
	clockevents_register_device(&clockevent_tick_timer);

	printk(KERN_INFO "mult[%lu]\n",
			(long unsigned int)clockevent_tick_timer.mult);
	printk(KERN_INFO "max_delta_ns[%lu]\n",
			(long unsigned int)clockevent_tick_timer.max_delta_ns);
	printk(KERN_INFO "min_delta_ns[%lu]\n",
			(long unsigned int)clockevent_tick_timer.min_delta_ns);
	printk(KERN_INFO "rate[%lu]\n",
			(long unsigned int)rate);
	printk(KERN_INFO "HZ[%d]\n", HZ);
}

/*
 * ---------------------------------------------------------------------------
 * SYSTEM TIMER ... free running 32-bit clock source and scheduler clock
 * ---------------------------------------------------------------------------
 */
irqreturn_t s5p_sched_timer_interrupt(int irq, void *dev_id)
{
	unsigned int temp_cstat;

	temp_cstat = s5p_systimer_read(S5P_SYSTIMER_INT_CSTAT);
	temp_cstat |= S5P_SYSTIMER_INT_INTCNT;

	s5p_systimer_write(S5P_SYSTIMER_INT_CSTAT, temp_cstat);

	if (unlikely(pending_irq))
		pending_irq = 0;
	else
		s5p_sched_timer_overflows++;

	return IRQ_HANDLED;
}

struct irqaction s5p_systimer_irq = {
	.name		= "System timer",
	.flags		= IRQF_DISABLED ,
	.handler	= s5p_sched_timer_interrupt,
};


static cycle_t s5p_sched_timer_read(struct clocksource *cs)
{

	return (cycle_t)~__raw_readl(S5P_SYSTIMER_TICNTO);
}

struct clocksource clocksource_s5p = {
	.name		= "clock_source_systimer",
	.rating		= 300,
	.read		= s5p_sched_timer_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.shift		= 20,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void s5p_init_clocksource(unsigned long rate)
{
	static char err[] __initdata = KERN_ERR
			"%s: can't register clocksource!\n";

	clocksource_s5p.mult
		= clocksource_khz2mult(rate/1000, clocksource_s5p.shift);

	s5p_sched_timer_start(~0, 1);

	if (clocksource_register(&clocksource_s5p))
		printk(err, clocksource_s5p.name);
}

/*
 * Returns current time from boot in nsecs. It's OK for this to wrap
 * around for now, as it's just a relative time stamp.
 */
unsigned long long sched_clock(void)
{
	unsigned long irq_flags;
	cycle_t ticks, elapsed_ticks = 0;
	unsigned long long increment = 0;
	unsigned int overflow_cnt = 0;

	local_irq_save(irq_flags);

	if (likely(sched_timer_running)) {
		overflow_cnt = (s5p_sched_timer_overflows - old_overflows);
		ticks = s5p_sched_timer_read(&clocksource_s5p);

		if (overflow_cnt) {
			increment = (overflow_cnt - 1) *
			(clocksource_cyc2ns(clocksource_s5p.read(&clocksource_s5p),
				clocksource_s5p.mult, clocksource_s5p.shift));
			elapsed_ticks =
				(clocksource_s5p.mask - last_ticks) + ticks;
		} else {
			if (unlikely(last_ticks > ticks)) {
				pending_irq = 1;
				elapsed_ticks =
				(clocksource_s5p.mask - last_ticks) + ticks;
				s5p_sched_timer_overflows++;
			} else {
				elapsed_ticks = (ticks - last_ticks);
			}
		}

		time_stamp += (clocksource_cyc2ns(elapsed_ticks,
					clocksource_s5p.mult,
					clocksource_s5p.shift) + increment);

		old_overflows = s5p_sched_timer_overflows;
		last_ticks = ticks;
	}
	local_irq_restore(irq_flags);

	return time_stamp;
}

/*
 *  Event/Sched Timer initialization
 */
static void s5p_timer_setup(void)
{
	unsigned long rate;
	unsigned int tmp;

	/* Setup event timer using XrtcXTI */
	if (clk_event == NULL)
		clk_event = clk_get(NULL, "xrtcxti");

	if (IS_ERR(clk_event))
		panic("failed to get clock for event timer");

	rate = clk_get_rate(clk_event);

	tmp = readl(rtc_base + S3C2410_RTCCON) &
		~(S3C_RTCCON_TICEN);

	/* We only support 32768 Hz : [7:4] = 0x0 */
	writel(tmp & ~0xf0, rtc_base + S3C2410_RTCCON);

	s5p_init_dynamic_tick_timer(rate);

	/* Setup sched-timer using XusbXTI */
	if (clk_sched == NULL)
		clk_sched = clk_get(NULL, "xusbxti");
	if (IS_ERR(clk_sched))
		panic("failed to get clock for sched-timer");
	rate = clk_get_rate(clk_sched);

	s5p_init_clocksource(rate);
}

static void s5p_tick_timer_setup(void)
{
	unsigned long rate;

	rate = clk_get_rate(clk_event);
	s5p_tick_timer_start((rate / HZ) - 1, 1);
}

static void __init s5p_timer_init(void)
{

	/* clock configuration setting and enable */
	struct clk *clk_systimer;
	struct clk *clk_rtc;

	/* Initialize variables before starting each timers */
	last_ticks = 0;
	s5p_sched_timer_overflows = 0;
	old_overflows = 0;
	time_stamp = 0;
	sched_timer_running = 0;
	pending_irq = 0;

	/* Setup system timer */
	clk_systimer = clk_get(NULL, "systimer");
	if (IS_ERR(clk_systimer))
		panic("failed to get clock[%s] for system timer", "systimer");

	clk_enable(clk_systimer);
	clk_put(clk_systimer);

	/* Setup rtc timer */
	clk_rtc = clk_get(NULL, "rtc");
	if (IS_ERR(clk_rtc))
		panic("failed to get clock[%s] for system timer", "rtc");

	clk_enable(clk_rtc);
	clk_put(clk_rtc);

	s5p_timer_setup();
	setup_irq(IRQ_RTC_TIC, &s5p_tick_timer_irq);
#if defined(USE_SYSTIMER_IRQ)
	setup_irq(IRQ_SYSTIMER, &s5p_systimer_irq);
#endif
}

struct sys_timer s5p_systimer = {
	.init		= s5p_timer_init,
};

