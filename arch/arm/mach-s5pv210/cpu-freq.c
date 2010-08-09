/*  linux/arch/arm/mach-s5pv210/cpu-freq.c
 *
 *  Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *
 *  CPU frequency scaling for S5PC110
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <asm/system.h>

#include <plat/pll.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>

#include <mach/map.h>
#include <mach/cpu-freq-v210.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

#define DBG(fmt...)
//#define DBG printk

static struct clk *mpu_clk;
static struct regulator *arm_regulator;
static struct regulator *internal_regulator;

struct s5pv210_cpufreq_freqs s5pv210_freqs;

static enum perf_level bootup_level;

unsigned int S5PV210_FREQ_TAB = 1;
static spinlock_t cpufreq_lock;

static char cpufreq_governor_name[CPUFREQ_NAME_LEN] = "conservative";// default governor
static char userspace_governor[CPUFREQ_NAME_LEN] = "userspace";
static char conservative_governor[CPUFREQ_NAME_LEN] = "conservative";
static char ondemand_governor[CPUFREQ_NAME_LEN] = "ondemand";

/* Based on MAX8698C 
 * RAMP time : 10mV/us
 **/
#define PMIC_RAMP_UP	10

static unsigned long previous_arm_volt;

/* If you don't need to wait exact ramp up delay
 * you can use fixed delay time
 **/
//#define USE_FIXED_RAMP_UP_DELAY

/* frequency */
static struct cpufreq_frequency_table s5pv210_freq_table_max[] = {
#if USE_1DOT2GHZ
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{0, CPUFREQ_TABLE_END},
#else
	{L0, 1000*1000},
	{L1, 800*1000},
	{0, CPUFREQ_TABLE_END},
#endif
};

static struct cpufreq_frequency_table s5pv210_freq_table_normal[] = {
#if USE_1DOT2GHZ
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{L3, 400*1000},
	{L4, 200*1000},
	{L5, 100*1000},
	{0, CPUFREQ_TABLE_END},
#else
	{L0, 1000*1000},
	{L1, 800*1000},
	{L2, 400*1000},
	{L3, 200*1000},
	{L4, 100*1000},
	{0, CPUFREQ_TABLE_END},
#endif
};

static struct cpufreq_frequency_table s5pv210_freq_table_restrict_1[] = {
#if USE_1DOT2GHZ
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{L3, 400*1000},
	{L4, 200*1000},
	{0, CPUFREQ_TABLE_END},
#else
	{L0, 1000*1000},
	{L1, 800*1000},
	{L2, 400*1000},
	{L3, 200*1000},
	{0, CPUFREQ_TABLE_END},
#endif
};

static struct cpufreq_frequency_table s5pv210_freq_table_restrict_2[] = {
#if USE_1DOT2GHZ
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{L3, 400*1000},
	{0, CPUFREQ_TABLE_END},
#else
	{L0, 1000*1000},
	{L1, 800*1000},
	{L2, 400*1000},
	{0, CPUFREQ_TABLE_END},
#endif
};

static struct cpufreq_frequency_table s5pv210_freq_table_suspend[] = {
#if USE_1DOT2GHZ
	{L2, 800*1000},
#else
	{L1, 800*1000},
#endif
	{0, CPUFREQ_TABLE_END},
};

static struct cpufreq_frequency_table *s5pv210_freq_table[] = {
	s5pv210_freq_table_max,
	s5pv210_freq_table_normal,
	s5pv210_freq_table_restrict_1,
	s5pv210_freq_table_restrict_2,
	s5pv210_freq_table_suspend,
};

static struct s5pv210_dvs_conf smdkc110_dvs_confs[] = {
#if USE_1DOT2GHZ
	{
		.lvl		= L0,
		.arm_volt	= 1300000,
		.int_volt	= 1150000,
	}, {
		.lvl		= L1,
		.arm_volt	= 1250000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L2,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L3,
		.arm_volt	= 1050000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L4,
		.arm_volt	= 950000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L5,
		.arm_volt	= 950000,
		.int_volt	= 1000000,
	},
#else
	{
		.lvl		= L0,
		.arm_volt	= 1250000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L1,
		.arm_volt	= 1200000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L2,
		.arm_volt	= 1050000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L3,
		.arm_volt	= 950000,
		.int_volt	= 1100000,
	}, {
		.lvl		= L4,
		.arm_volt	= 950000,
		.int_volt	= 1000000,
	},
#endif
};


static u32 clkdiv_val[][11] = {
/*{ APLL, A2M, HCLK_MSYS, PCLK_MSYS,
 *	HCLK_DSYS, PCLK_DSYS, HCLK_PSYS, PCLK_PSYS,
 *		ONEDRAM, MFC, G3D }
 */
#if USE_1DOT2GHZ
	/* L0 : [1200/200/100][166/83][133/66][200/200] */
	{0, 5, 5, 1, 3, 1, 4, 1, 3, 0, 0},
#endif
	/* L0 : [1000/200/100][166/83][133/66][200/200] */
	{0, 4, 4, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L1 : [800/200/100][166/83][133/66][200/200] */
	{0, 3, 3, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L2 : [400/200/100][166/83][133/66][200/200] */
	{1, 3, 1, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L3 : [200/200/100][166/83][133/66][200/100] */
	{3, 3, 0, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L4 : [100/100/100][83/83][66/66][100/100] */
	{7, 7, 0, 0, 7, 0, 9, 0, 7, 0, 0},
};

static struct s5pv210_domain_freq s5pv210_clk_info[] = {
	{
#if USE_1DOT2GHZ
		.apll_out 	= 1200000,
		.armclk 	= 1200000,
		.hclk_msys 	= 200000,
		.pclk_msys 	= 100000,
		.hclk_dsys 	= 166000,
		.pclk_dsys 	= 83000,
		.hclk_psys	= 133000,
		.pclk_psys	= 66000,
	}, {
#endif
		.apll_out 	= 1000000,
		.armclk 	= 1000000,
		.hclk_msys 	= 200000,
		.pclk_msys 	= 100000,
		.hclk_dsys 	= 166000,
		.pclk_dsys 	= 83000,
		.hclk_psys	= 133000,
		.pclk_psys	= 66000,
	}, {
		.apll_out 	= 800000,
		.armclk 	= 800000,
		.hclk_msys 	= 200000,
		.pclk_msys 	= 100000,
		.hclk_dsys 	= 166000,
		.pclk_dsys 	= 83000,
		.hclk_psys	= 133000,
		.pclk_psys	= 66000,
	}, {
		.apll_out 	= 800000,
		.armclk 	= 400000,
		.hclk_msys 	= 200000,
		.pclk_msys 	= 100000,
		.hclk_dsys 	= 166000,
		.pclk_dsys 	= 83000,
		.hclk_psys	= 133000,
		.pclk_psys	= 66000,
	}, {
		.apll_out 	= 800000,
		.armclk 	= 200000,
		.hclk_msys 	= 200000,
		.pclk_msys 	= 100000,
		.hclk_dsys 	= 166000,
		.pclk_dsys 	= 83000,
		.hclk_psys	= 133000,
		.pclk_psys	= 66000,
	}, {
		.apll_out 	= 800000,
		.armclk 	= 100000,
		.hclk_msys 	= 100000,
		.pclk_msys 	= 100000,
		.hclk_dsys 	= 83000,
		.pclk_dsys 	= 83000,
		.hclk_psys	= 66000,
		.pclk_psys	= 66000,
	},
};

int is_userspace_gov(void)
{
        int ret = 0;
        if(!strnicmp(cpufreq_governor_name, userspace_governor, CPUFREQ_NAME_LEN)) {
                ret = 1;
        }
        return ret;
}

int is_conservative_gov(void)
{
        int ret = 0;
        if(!strnicmp(cpufreq_governor_name, conservative_governor, CPUFREQ_NAME_LEN)) {
                ret = 1;
        }
        return ret;
}

int is_ondemand_gov(void)
{
        int ret = 0;
        if(!strnicmp(cpufreq_governor_name, ondemand_governor, CPUFREQ_NAME_LEN)) {
                ret = 1;
        }
        return ret;
}

/*return performance level */
static int get_dvfs_perf_level(enum freq_level_states freq_level, unsigned int *perf_level)
{
	unsigned int freq=0, index = 0;
	struct cpufreq_frequency_table *freq_tab = s5pv210_freq_table[S5PV210_FREQ_TAB];
	switch(freq_level)
	{
	case LEV_1200MHZ:
		freq = 1200 * 1000;
		break;
	case LEV_1000MHZ:
		freq = 1000 * 1000;
		break;
	case LEV_800MHZ:
		freq = 800 * 1000;
		break;
	case LEV_400MHZ:
		freq = 400 * 1000;
		break;
	case LEV_200MHZ:
		freq = 200 * 1000;
		break;
	case LEV_100MHZ:
		freq = 100 * 1000;
		break;
	default:
		printk(KERN_ERR "Invalid freq level\n");
		return -EINVAL;
	}
	while((freq_tab[index].frequency != CPUFREQ_TABLE_END) &&
		(freq_tab[index].frequency != freq)) {
		index++;
	}
	if(freq_tab[index].frequency == CPUFREQ_TABLE_END)
	{
		printk(KERN_ERR "Invalid freq table\n");
		return -EINVAL;	
	}

	*perf_level = freq_tab[index].index;
	return 0;
}


// jump to 400mhz
int dvfs_change_quick = 0;
static void set_dvfs_perf_level(unsigned int perf_level) 
{
	//spin_lock(&cpufreq_lock); check if lock is needed
	//desired frequency level
	dvfs_change_quick = s5pv210_freq_table[S5PV210_FREQ_TAB][perf_level].frequency;
	//spin_unlock(&cpufreq_lock); check if lock is needed
}

//Jump to the given frequency level 
void set_dvfs_target_level(enum freq_level_states freq_level) 
{
	unsigned int ret = 0, perf_level=0;

	if(!is_conservative_gov())
		return;

	ret = get_dvfs_perf_level(freq_level, &perf_level);
	if(ret)
		return;

	set_dvfs_perf_level(perf_level);
	return;
}


void s5pv210_set_cpufreq_level(unsigned int mode)
{
	unsigned int index = 0;
	struct cpufreq_frequency_table *freq_tab;
	switch (mode) {
	case MAXIMUM_TABLE:
		S5PV210_FREQ_TAB = 0;
		break;
	case NORMAL_TABLE:
		S5PV210_FREQ_TAB = 1;
		break;
	case RESTRICT_TABLE_1:
		S5PV210_FREQ_TAB = 2;
		break;
	case RESTRICT_TABLE_2:
		S5PV210_FREQ_TAB = 3;
		break;
	case SUSPEND_TABLE:
		S5PV210_FREQ_TAB = 4;
		break;
	default:
		printk(KERN_ERR "Can't find corresponding cpufreq table %d \n", mode);
		S5PV210_FREQ_TAB = 1;
		break;
	}

	if(!is_conservative_gov())
		return;

	freq_tab = s5pv210_freq_table[S5PV210_FREQ_TAB];
	/*jump to the lower level*/
	while(freq_tab[index].frequency != CPUFREQ_TABLE_END)
		index++;
	set_dvfs_perf_level(index-1);
	return;
}

int s5pv210_verify_speed(struct cpufreq_policy *policy)
{

	if (policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, s5pv210_freq_table[S5PV210_FREQ_TAB]);
}

unsigned int s5pv210_getspeed(unsigned int cpu)
{
	unsigned long rate;

	if (cpu)
		return 0;

	rate = clk_get_rate(mpu_clk) / KHZ_T;

	return rate;
}

extern bool gbTransitionLogEnable;
extern void print_clocks(void);
static int s5pv210_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index, reg, arm_volt, int_volt;
	unsigned int pll_changing = 0;
	unsigned int bus_speed_changing = 0;
	int ramp_req_delay, ramp_real_delay;
	struct timeval start, end;

	//DBG("%s: called for target freq=%d\n",__func__,target_freq);

	s5pv210_freqs.freqs.old = s5pv210_getspeed(0);

	if(policy != NULL) {
		if(policy -> governor) {
			if (strnicmp(cpufreq_governor_name, policy->governor->name, CPUFREQ_NAME_LEN)) {
				strcpy(cpufreq_governor_name, policy->governor->name);
			}
		}
	}

	if (cpufreq_frequency_table_target(policy, s5pv210_freq_table[S5PV210_FREQ_TAB],
		target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}
	

	arm_clk = s5pv210_freq_table[S5PV210_FREQ_TAB][index].frequency;
	index = s5pv210_freq_table[S5PV210_FREQ_TAB][index].index;

	//DBG("%s: called for target freq=%d\n",__func__,arm_clk);

	s5pv210_freqs.freqs.new = arm_clk;
	s5pv210_freqs.freqs.cpu = 0;

	if (s5pv210_freqs.freqs.new == s5pv210_freqs.freqs.old)
		return 0;

	DBG("%s: called for target freq=%d old freq=%d\n",__func__,arm_clk,s5pv210_freqs.freqs.old);

	arm_volt = smdkc110_dvs_confs[index].arm_volt;
	int_volt = smdkc110_dvs_confs[index].int_volt;

	/* iNew clock inforamtion update */
	memcpy(&s5pv210_freqs.new, &s5pv210_clk_info[index],
					sizeof(struct s5pv210_domain_freq));

	cpufreq_notify_transition(&s5pv210_freqs.freqs, CPUFREQ_PRECHANGE);

	if (s5pv210_freqs.freqs.new > s5pv210_freqs.freqs.old) {
		/* Voltage up */

		DBG("Voltage increase \n");
		regulator_set_voltage(arm_regulator, arm_volt,
				arm_volt);
		regulator_set_voltage(internal_regulator, int_volt,
				int_volt);
#if defined(USE_FIXED_RAMP_UP_DELAY)
		ramp_req_delay = (((arm_volt - previous_arm_volt) / 1000) / PMIC_RAMP_UP);
		udelay(ramp_req_delay);
#else
		do_gettimeofday(&start);
#endif
	}

	/* Check if there need to change PLL */
	if (s5pv210_freqs.new.apll_out != s5pv210_freqs.old.apll_out)
		pll_changing = 1;

	/* Check if there need to change System bus clock */
	if (s5pv210_freqs.new.hclk_msys != s5pv210_freqs.old.hclk_msys)
		bus_speed_changing = 1;

	if (bus_speed_changing) {
		/* Reconfigure DRAM refresh counter value for minimum
		 * temporary clock while changing divider.
		 * expected clock is 83Mhz : 7.8usec/(1/83Mhz) = 0x287
		 **/
		DBG("Bus speed changing...\n");
		if (pll_changing)
			__raw_writel(0x287, S5P_VA_DMC1 + 0x30);
		else
			__raw_writel(0x30c, S5P_VA_DMC1 + 0x30);

		__raw_writel(0x287, S5P_VA_DMC0 + 0x30);

	}

/* APLL should be changed in this level
 * APLL -> MPLL(for stable transition) -> APLL
 * Some clock source's clock API  are not prepared. Do not use clock API
 * in below code.
 */
	if (pll_changing) {
		DBG("Pll speed changing...\n");
		/* 1. Temporary Change divider for MFC and G3D 
		 * SCLKA2M(200/1=200)->(200/4=50)Mhz
		 **/
		reg = __raw_readl(S5P_CLK_DIV2);
		reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
		reg |= 	(3 << S5P_CLKDIV2_G3D_SHIFT) |
			(3 << S5P_CLKDIV2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV2);

		/* For MFC, G3D dividing */
		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT0);
		} while (reg & ((1 << 16) | (1 << 17)));
		
		/* 2. Change SCLKA2M(200Mhz)to SCLKMPLL in MFC_MUX, G3D MUX 
		 * (200/4=50)->(667/4=166)Mhz
		 **/
		reg = __raw_readl(S5P_CLK_SRC2);
		reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
		reg |= 	(1 << S5P_CLKSRC2_G3D_SHIFT) |
			(1 << S5P_CLKSRC2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC2);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & ((1 << 7) | (1 << 3)));

		/* 3. DMC1 refresh count for 133Mhz if (index == L4) is
		 * true refresh counter is already programed in upper
		 * code. 0x287@83Mhz
		 **/
		if (!bus_speed_changing)
			__raw_writel(0x40e, S5P_VA_DMC1 + 0x30);

		/* 4. SCLKAPLL -> SCLKMPLL */
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &= ~(S5P_CLKSRC0_MUX200_MASK);
		reg |= (0x1 << S5P_CLKSRC0_MUX200_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC0);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT0);
		} while (reg & (0x1 << 18));

#if defined(CONFIG_S5PC110_H_TYPE)
		/* DMC0 source clock : SCLKA2M -> SCLKMPLL */
		__raw_writel(0x50e, S5P_VA_DMC0 + 0x30);

		reg = __raw_readl(S5P_CLK_DIV6);
		reg &= ~(S5P_CLKDIV6_ONEDRAM_MASK);
		reg |= (0x3 << S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV6);

		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (reg & (1 << 15));

		reg = __raw_readl(S5P_CLK_SRC6);
		reg &= ~(S5P_CLKSRC6_ONEDRAM_MASK);
		reg |= (0x1 << S5P_CLKSRC6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC6);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & (1 << 11));
#endif
	}

	/* ARM MCS value changed */
#if USE_1DOT2GHZ
	if (index < L4) { // not 200 , 100MHZ
#else
	if (index < L3) { // not 200 , 100MHZ
#endif
		reg = __raw_readl(S5P_ARM_MCS_CON);
		reg &= ~0x3;
		reg |= 0x1;
		__raw_writel(reg, S5P_ARM_MCS_CON);
	}

#if !defined(USE_FIXED_RAMP_UP_DELAY)
	if (s5pv210_freqs.freqs.new > s5pv210_freqs.freqs.old) {


		do_gettimeofday(&end);

		/* Based on 10mV/usec ramp up speed
		 * Requried ramp up delay time (usec) for this voltage change
		 **/
		ramp_req_delay = (((arm_volt - previous_arm_volt) / 1000) / PMIC_RAMP_UP);

		ramp_real_delay = ramp_req_delay -
			((end.tv_sec - start.tv_sec) * USEC_PER_SEC +
			(end.tv_usec - start.tv_usec));

		if (ramp_real_delay > 0) {
			udelay(ramp_real_delay);
		} else {
			DBG(KERN_INFO "Ramp up delay already consumed [%d]\n", ramp_real_delay);
		}
	}
#endif
	/* 5. Change divider */
	reg = __raw_readl(S5P_CLK_DIV0);

	reg &= ~(S5P_CLKDIV0_APLL_MASK | S5P_CLKDIV0_A2M_MASK |
		S5P_CLKDIV0_HCLK200_MASK | S5P_CLKDIV0_PCLK100_MASK |
		S5P_CLKDIV0_HCLK166_MASK | S5P_CLKDIV0_PCLK83_MASK |
		S5P_CLKDIV0_HCLK133_MASK | S5P_CLKDIV0_PCLK66_MASK);

	reg |= ((clkdiv_val[index][0] << S5P_CLKDIV0_APLL_SHIFT) |
		(clkdiv_val[index][1] << S5P_CLKDIV0_A2M_SHIFT) |
		(clkdiv_val[index][2] << S5P_CLKDIV0_HCLK200_SHIFT) |
		(clkdiv_val[index][3] << S5P_CLKDIV0_PCLK100_SHIFT) |
		(clkdiv_val[index][4] << S5P_CLKDIV0_HCLK166_SHIFT) |
		(clkdiv_val[index][5] << S5P_CLKDIV0_PCLK83_SHIFT) |
		(clkdiv_val[index][6] << S5P_CLKDIV0_HCLK133_SHIFT) |
		(clkdiv_val[index][7] << S5P_CLKDIV0_PCLK66_SHIFT));

	__raw_writel(reg, S5P_CLK_DIV0);

	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & 0xff);

	/* ARM MCS value changed */
#if USE_1DOT2GHZ
	if (index >= L4) {// for 100, 200MHZ
#else
	if (index >= L3) {// for 100, 200MHZ
#endif
		reg = __raw_readl(S5P_ARM_MCS_CON);
		reg &= ~0x3;
		reg |= 0x3;
		__raw_writel(reg, S5P_ARM_MCS_CON);
	}

	if (pll_changing) {
		/* 7. Set Lock time = 30us*24Mhz = 0x2cf */
		__raw_writel(0x2cf, S5P_APLL_LOCK);

		/* 8. Turn on APLL
		 * 8-1. Set PMS values
		 * 8-3. Wait untile the PLL is locked
		 **/
#if USE_1DOT2GHZ
		if (index == L0)
			__raw_writel(APLL_VAL_1200, S5P_APLL_CON);
		else if (index == L1)
			__raw_writel(APLL_VAL_1000, S5P_APLL_CON);
		else //L2
			__raw_writel(APLL_VAL_800, S5P_APLL_CON);
		
#else
		if (index == L0)
			__raw_writel(APLL_VAL_1000, S5P_APLL_CON);
		else // L1
			__raw_writel(APLL_VAL_800, S5P_APLL_CON);
#endif

		do {
			reg = __raw_readl(S5P_APLL_CON);
		} while (!(reg & (0x1 << 29)));

		/* 9. Change souce clock from SCLKMPLL(667Mhz)
		 * to SCLKA2M(200Mhz) in MFC_MUX and G3D MUX 
		 * (667/4=166)->(200/4=50)Mhz
		 **/
		reg = __raw_readl(S5P_CLK_SRC2);
		reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
		reg |= 	(0 << S5P_CLKSRC2_G3D_SHIFT) |
			(0 << S5P_CLKSRC2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC2);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & ((1 << 7) | (1 << 3)));

		/* 10. Change divider for MFC and G3D 
		 * (200/4=50)->(200/1=200)Mhz
		 **/
		reg = __raw_readl(S5P_CLK_DIV2);
		reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
		reg |= 	(clkdiv_val[index][10] << S5P_CLKDIV2_G3D_SHIFT) |
			(clkdiv_val[index][9] << S5P_CLKDIV2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV2);

		/* For MFC, G3D dividing */
		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT0);
		} while (reg & ((1 << 16) | (1 << 17)));
	
		/* 11. Change MPLL to APLL in MSYS_MUX */
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &= ~(S5P_CLKSRC0_MUX200_MASK);
		reg |= (0x0 << S5P_CLKSRC0_MUX200_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC0);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT0);
		} while (reg & (0x1 << 18));

		/* 12. DMC1 refresh counter
		 * L4 : DMC1 = 100Mhz 7.8us/(1/100) = 0x30c
		 * Others : DMC1 = 200Mhz 7.8us/(1/200) = 0x618
		 **/
		if (!bus_speed_changing)
			__raw_writel(0x618, S5P_VA_DMC1 + 0x30);

#if defined(CONFIG_S5PC110_H_TYPE)
		/* DMC0 source clock : SCLKMPLL -> SCLKA2M */
		reg = __raw_readl(S5P_CLK_SRC6);
		reg &= ~(S5P_CLKSRC6_ONEDRAM_MASK);
		reg |= (0x0 << S5P_CLKSRC6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC6);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & (1 << 11));

		reg = __raw_readl(S5P_CLK_DIV6);
		reg &= ~(S5P_CLKDIV6_ONEDRAM_MASK);
		reg |= (0x0 << S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV6);

		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (reg & (1 << 15));
		__raw_writel(0x618, S5P_VA_DMC0 + 0x30);
#endif
				DBG("PLL change done...\n");
	}


/* L4 level need to change memory bus speed, hence onedram clock divier and
 * memory refresh parameter should be changed
 * Only care L3 <-> L4 transition
 */
	if (bus_speed_changing) {
		reg = __raw_readl(S5P_CLK_DIV6);
		reg &= ~S5P_CLKDIV6_ONEDRAM_MASK;
		reg |= (clkdiv_val[index][8] << S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV6);

		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (reg & (1 << 15));

		/* Reconfigure DRAM refresh counter value */
#if USE_1DOT2GHZ
		if (index != L5) {
#else
		if (index != L4) {
#endif
			/* DMC0 : 166Mhz
			 * DMC1 : 200Mhz
			 **/
			__raw_writel(0x618, S5P_VA_DMC1 + 0x30);
#if !defined(CONFIG_S5PC110_H_TYPE)
			__raw_writel(0x50e, S5P_VA_DMC0 + 0x30);
#else
			__raw_writel(0x618, S5P_VA_DMC0 + 0x30);
#endif
		} else {
			/* DMC0 : 83Mhz
			 * DMC1 : 100Mhz
			 **/
			__raw_writel(0x30c, S5P_VA_DMC1 + 0x30);
			__raw_writel(0x287, S5P_VA_DMC0 + 0x30);
		}
	}

	if (s5pv210_freqs.freqs.new < s5pv210_freqs.freqs.old) {
		/* Voltage down */
		regulator_set_voltage(arm_regulator, arm_volt,
				arm_volt);
		regulator_set_voltage(internal_regulator, int_volt,
				int_volt);
	}

	cpufreq_notify_transition(&s5pv210_freqs.freqs, CPUFREQ_POSTCHANGE);

	memcpy(&s5pv210_freqs.old, &s5pv210_freqs.new, sizeof(struct s5pv210_domain_freq));

	mpu_clk->rate = s5pv210_freqs.freqs.new * KHZ_T;

	//printk(KERN_INFO "Perf changed[L%d]\n", index);
#ifdef CONFIG_CPU_FREQ_LOG
	if(gbTransitionLogEnable == true)
	{
		DBG("Perf changed[L%d]\n",index);
		printk("[DVFS Transition]...\n");
		print_clocks();
	}
#endif

	previous_arm_volt = smdkc110_dvs_confs[index].arm_volt;
out:
	return ret;
}

#ifdef CONFIG_PM
static int s5pv210_cpufreq_suspend(struct cpufreq_policy *policy,
			pm_message_t pmsg)
{
	int ret = 0;

	return ret;
}

static int s5pv210_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret = 0;
	/* Clock inforamtion update with wakeup value */
	memcpy(&s5pv210_freqs.old, &s5pv210_clk_info[bootup_level],
			sizeof(struct s5pv210_domain_freq));
	previous_arm_volt = smdkc110_dvs_confs[bootup_level].arm_volt;
	return ret;
}
#endif

static int __init s5pv210_cpu_init(struct cpufreq_policy *policy)
{
	u32 rate;
	DBG("%s: called. \n");
#ifdef CLK_OUT_PROBING
	//reg = __raw_readl(S5P_CLK_OUT);
	//reg &= ~(0x1f << 12 | 0xf << 20);
	//reg |= (0xf << 12 | 0x1 << 20);
	//__raw_writel(reg, S5P_CLK_OUT);
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);
#if defined(CONFIG_REGULATOR)
	arm_regulator = regulator_get(NULL, "vddarm");
	if (IS_ERR(arm_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vddarm");
		return PTR_ERR(arm_regulator);
	}
	internal_regulator = regulator_get(NULL, "vddint");
	if (IS_ERR(internal_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vddint");
		return PTR_ERR(internal_regulator);
	}

#endif

	S5PV210_FREQ_TAB = 1;

	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5pv210_getspeed(0);

	cpufreq_frequency_table_get_attr(s5pv210_freq_table[S5PV210_FREQ_TAB], policy->cpu);

	policy->cpuinfo.transition_latency = 40000;

	rate = clk_get_rate(mpu_clk);

	switch (rate) {
	case 1000000000:	/* 1GHz */
		bootup_level = L0;
		break;
	case 800000000:		/* 800MHz */
#if USE_1DOT2GHZ
		bootup_level = L2;
#else
		bootup_level = L1;
#endif
		break;
	default:
		printk(KERN_ERR "[%s] cannot find matching clock"
				"[%s] rate [%d]\n"
				, __FUNCTION__, MPU_CLK, rate);
		bootup_level = L1;
		break;
	}

	printk("S5PV210_FREQ_TAB=%d bootup_level=%d\n",S5PV210_FREQ_TAB,bootup_level);

	memcpy(&s5pv210_freqs.old, &s5pv210_clk_info[bootup_level],
			sizeof(struct s5pv210_domain_freq));
	
	previous_arm_volt = smdkc110_dvs_confs[bootup_level].arm_volt;

	spin_lock_init(&cpufreq_lock);

	return cpufreq_frequency_table_cpuinfo(policy, s5pv210_freq_table[S5PV210_FREQ_TAB]);
}

static struct cpufreq_driver s5pv210_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5pv210_verify_speed,
	.target		= s5pv210_target,
	.get		= s5pv210_getspeed,
	.init		= s5pv210_cpu_init,
	.name		= "s5pv210",
#ifdef CONFIG_PM
	.suspend	= s5pv210_cpufreq_suspend,
	.resume		= s5pv210_cpufreq_resume,
#endif
};

static int __init s5pv210_cpufreq_init(void)
{
	return cpufreq_register_driver(&s5pv210_driver);
}

late_initcall(s5pv210_cpufreq_init);
