/*  linux/arch/arm/mach-s5pv210/cpu-freq.c
 *
 *  Copyright (C) 2010 Samsung Electronics Co., Ltd.
 *
 *  CPU frequency scaling for S5PV210/S5PC110
 *  Based on cpu-sa1110.c and s5pc11x-cpufreq.c
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
#include <linux/suspend.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <asm/system.h>

#include <mach/map.h>
#include <mach/cpu-freq-v210.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

#include <plat/cpu-freq.h>
#include <plat/pll.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>
#include <plat/pm.h>

static struct clk *mpu_clk;
static struct regulator *arm_regulator;
static struct regulator *internal_regulator;

struct s3c_cpufreq_freqs s3c_freqs;

static unsigned long previous_arm_volt;

static unsigned int backup_dmc0_reg;
static unsigned int backup_dmc1_reg;
static unsigned int backup_freq_level;
static unsigned int mpll_freq; /* in MHz */
static unsigned int apll_freq_max; /* in MHz */
static DEFINE_MUTEX(set_freq_lock);

/* frequency */
static struct cpufreq_frequency_table freq_table[] = {
	{L0, 1000*1000},
	{L1, 800*1000},
	{L2, 400*1000},
	{L3, 200*1000},
	{L4, 100*1000},
	{0, CPUFREQ_TABLE_END},
};

struct s5pv210_dvs_conf {
	unsigned long       arm_volt;   /* uV */
	unsigned long       int_volt;   /* uV */
};

const unsigned long arm_volt_max = 1350000;
const unsigned long int_volt_max = 1250000;

static struct s5pv210_dvs_conf dvs_conf[] = {
	[L0] = {
		.arm_volt   = 1250000,
		.int_volt   = 1100000,
	},
	[L1] = {
		.arm_volt   = 1200000,
		.int_volt   = 1100000,
	},
	[L2] = {
		.arm_volt   = 1050000,
		.int_volt   = 1100000,
	},
	[L3] = {
		.arm_volt   = 950000,
		.int_volt   = 1100000,
	},
	[L4] = {
		.arm_volt   = 950000,
		.int_volt   = 1000000,
	},
};

static u32 clkdiv_val[5][11] = {
	/*{ APLL, A2M, HCLK_MSYS, PCLK_MSYS,
	 * HCLK_DSYS, PCLK_DSYS, HCLK_PSYS, PCLK_PSYS, ONEDRAM,
	 * MFC, G3D }
	 */
	/* L0 : [1000/200/200/100][166/83][133/66][200/200] */
	{0, 4, 4, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L1 : [800/200/200/100][166/83][133/66][200/200] */
	{0, 3, 3, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L2 : [400/200/200/100][166/83][133/66][200/200] */
	{1, 3, 1, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L3 : [200/200/200/100][166/83][133/66][200/200] */
	{3, 3, 0, 1, 3, 1, 4, 1, 3, 0, 0},
	/* L4 : [100/100/100/100][83/83][66/66][100/100] */
	{7, 7, 0, 0, 7, 0, 9, 0, 7, 0, 0},
};

static struct s3c_freq clk_info[] = {
	[L0] = {	/* L0: 1GHz */
		.fclk       = 1000000,
		.armclk     = 1000000,
		.hclk_tns   = 0,
		.hclk       = 133000,
		.pclk       = 66000,
		.hclk_msys  = 200000,
		.pclk_msys  = 100000,
		.hclk_dsys  = 166750,
		.pclk_dsys  = 83375,
	},
	[L1] = {	/* L1: 800MHz */
		.fclk       = 800000,
		.armclk     = 800000,
		.hclk_tns   = 0,
		.hclk       = 133000,
		.pclk       = 66000,
		.hclk_msys  = 200000,
		.pclk_msys  = 100000,
		.hclk_dsys  = 166750,
		.pclk_dsys  = 83375,
	},
	[L2] = {	/* L2: 400MHz */
		.fclk       = 800000,
		.armclk     = 400000,
		.hclk_tns   = 0,
		.hclk       = 133000,
		.pclk       = 66000,
		.hclk_msys  = 200000,
		.pclk_msys  = 100000,
		.hclk_dsys  = 166750,
		.pclk_dsys  = 83375,
	},
	[L3] = {	/* L3: 200MHz */
		.fclk       = 800000,
		.armclk     = 200000,
		.hclk_tns   = 0,
		.hclk       = 133000,
		.pclk       = 66000,
		.hclk_msys  = 200000,
		.pclk_msys  = 100000,
		.hclk_dsys  = 166750,
		.pclk_dsys  = 83375,
	},
	[L4] = {	/* L4: 100MHz */
		.fclk       = 800000,
		.armclk     = 100000,
		.hclk_tns   = 0,
		.hclk       = 66000,
		.pclk       = 66000,
		.hclk_msys  = 100000,
		.pclk_msys  = 100000,
		.hclk_dsys  = 83375,
		.pclk_dsys  = 83375,
	},
};

static int s5pv210_cpufreq_verify_speed(struct cpufreq_policy *policy)
{
	if (policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, freq_table);
}

static unsigned int s5pv210_cpufreq_getspeed(unsigned int cpu)
{
	unsigned long rate;

	if (cpu)
		return 0;

	rate = clk_get_rate(mpu_clk) / 1000;

	return rate;
}

static void wait4div_gxd(void)
{
	unsigned int reg;
	/* Wait for MFC, G3D div changing */
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & (S5P_CLKDIV_STAT0_G3D | S5P_CLKDIV_STAT0_MFC));
	/* Wait for G2D div changing */
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT1);
	} while  (reg & (S5P_CLKDIV_STAT1_G2D));
}

static void wait4src_gxd(void)
{
	unsigned int reg;
	/* Wait for MFC, G3D, G2D mux changing */
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT1);
	} while (reg & (S5P_CLKMUX_STAT1_G3D | S5P_CLKMUX_STAT1_MFC
				| S5P_CLKMUX_STAT1_G2D));
}

static void s5pv210_cpufreq_clksrcs_APLL2MPLL(unsigned int index,
		unsigned int bus_speed_changing)
{
	unsigned int reg;

	/*
	 * 1. Temporarily change divider for MFC and G3D
	 * SCLKA2M(200/1=200)->(200/4=50)MHz
	 */
	reg = __raw_readl(S5P_CLK_DIV2);
	reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
	reg |= (0x3 << S5P_CLKDIV2_G3D_SHIFT) |
		(0x3 << S5P_CLKDIV2_MFC_SHIFT);
	reg &= ~S5P_CLKDIV2_G2D_MASK;
	reg |= (0x2 << S5P_CLKDIV2_G2D_SHIFT);

	__raw_writel(reg, S5P_CLK_DIV2);

	wait4div_gxd();

	/*
	 * 2. Change SCLKA2M(200MHz) to SCLKMPLL in MFC_MUX, G3D MUX
	 * (100/4=50)->(667/4=166)MHz
	 */
	reg = __raw_readl(S5P_CLK_SRC2);
	reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
	reg |= (0x1 << S5P_CLKSRC2_G3D_SHIFT) |
		(0x1 << S5P_CLKSRC2_MFC_SHIFT);
	reg &= ~S5P_CLKSRC2_G2D_MASK;
	reg |= (0x1 << S5P_CLKSRC2_G2D_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC2);

	wait4src_gxd();

	/* 3. msys: SCLKAPLL -> SCLKMPLL */
	reg = __raw_readl(S5P_CLK_SRC0);
	reg &= ~(S5P_CLKSRC0_MUX200_MASK);
	reg |= (0x1 << S5P_CLKSRC0_MUX200_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC0);

	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT0);
	} while (reg & S5P_CLKMUX_STAT0_MUX200);
}

static void s5pv210_cpufreq_clksrcs_MPLL2APLL(unsigned int index,
		unsigned int bus_speed_changing)
{
	unsigned int reg;

	/*
	 * 1. Set Lock time = 30us*24MHz = 02cf
	 */
	__raw_writel(0x2cf, S5P_APLL_LOCK);

	/*
	 * 2. Turn on APLL
	 * 2-1. Set PMS values
	 */
	if (index == L0)
		/* APLL FOUT becomes 1000 Mhz */
		__raw_writel(PLL45XX_APLL_VAL_1000, S5P_APLL_CON);
	else
		/* APLL FOUT becomes 800 Mhz */
		__raw_writel(PLL45XX_APLL_VAL_800, S5P_APLL_CON);
	/* 2-2. Wait until the PLL is locked */
	do {
		reg = __raw_readl(S5P_APLL_CON);
	} while (!(reg & (0x1 << 29)));

	/*
	 * 3. Change source clock from SCLKMPLL(667MHz)
	 * to SCLKA2M(200MHz) in MFC_MUX and G3D_MUX
	 * (667/4=166)->(200/4=50)MHz
	 */
	reg = __raw_readl(S5P_CLK_SRC2);
	reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
	reg |= (0 << S5P_CLKSRC2_G3D_SHIFT) | (0 << S5P_CLKSRC2_MFC_SHIFT);
	reg &= ~S5P_CLKSRC2_G2D_MASK;
	reg |= 0x1 << S5P_CLKSRC2_G2D_SHIFT;
	__raw_writel(reg, S5P_CLK_SRC2);

	wait4src_gxd();

	/*
	 * 4. Change divider for MFC and G3D
	 * (200/4=50)->(200/1=200)MHz
	 */
	reg = __raw_readl(S5P_CLK_DIV2);
	reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
	reg |= (clkdiv_val[index][10] << S5P_CLKDIV2_G3D_SHIFT) |
		(clkdiv_val[index][9] << S5P_CLKDIV2_MFC_SHIFT);
	reg &= ~S5P_CLKDIV2_G2D_MASK;
	reg |= 0x2 << S5P_CLKDIV2_G2D_SHIFT;
	__raw_writel(reg, S5P_CLK_DIV2);

	wait4div_gxd();

	/* 5. Change MPLL to APLL in MSYS_MUX */
	reg = __raw_readl(S5P_CLK_SRC0);
	reg &= ~(S5P_CLKSRC0_MUX200_MASK);
	reg |= (0x0 << S5P_CLKSRC0_MUX200_SHIFT); /* SCLKMPLL -> SCLKAPLL   */
	__raw_writel(reg, S5P_CLK_SRC0);

	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT0);
	} while (reg & S5P_CLKMUX_STAT0_MUX200);
}

static int no_cpufreq_access;
/*
 * s5pv210_cpufreq_target: relation has an additional symantics other than
 * the standard
 * [0x30]:
 *	1: disable further access to target until being re-enabled.
 *	2: re-enable access to target */
static int s5pv210_cpufreq_target(struct cpufreq_policy *policy,
		unsigned int target_freq,
		unsigned int relation)
{
	static bool first_run = true;
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index, reg, arm_volt, int_volt;
	unsigned int pll_changing = 0;
	unsigned int bus_speed_changing = 0;

	mutex_lock(&set_freq_lock);

	cpufreq_debug_printk(CPUFREQ_DEBUG_DRIVER, KERN_INFO,
			"cpufreq: Entering for %dkHz\n", target_freq);

	if ((relation & ENABLE_FURTHER_CPUFREQ) &&
			(relation & DISABLE_FURTHER_CPUFREQ)) {
		/* Invalidate both if both marked */
		relation &= ~ENABLE_FURTHER_CPUFREQ;
		relation &= ~DISABLE_FURTHER_CPUFREQ;
		pr_err("%s:%d denied marking \"FURTHER_CPUFREQ\""
				" as both marked.\n",
				__FILE__, __LINE__);
	}
	if (relation & ENABLE_FURTHER_CPUFREQ)
		no_cpufreq_access = 0;
	if (no_cpufreq_access == 1) {
#ifdef CONFIG_PM_VERBOSE
		pr_err("%s:%d denied access to %s as it is disabled"
			       " temporarily\n", __FILE__, __LINE__, __func__);
#endif
		ret = -EINVAL;
		goto out;
	}
	if (relation & DISABLE_FURTHER_CPUFREQ)
		no_cpufreq_access = 1;
	relation &= ~MASK_FURTHER_CPUFREQ;

	s3c_freqs.freqs.old = s5pv210_cpufreq_getspeed(0);

	if (cpufreq_frequency_table_target(policy, freq_table,
				target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}

	arm_clk = freq_table[index].frequency;

	s3c_freqs.freqs.new = arm_clk;
	s3c_freqs.freqs.cpu = 0;

	/*
	 * Run this function unconditionally until s3c_freqs.freqs.new
	 * and s3c_freqs.freqs.old are both set by this function.
	 */
	if (s3c_freqs.freqs.new == s3c_freqs.freqs.old && !first_run)
		goto out;

	arm_volt = dvs_conf[index].arm_volt;
	int_volt = dvs_conf[index].int_volt;

	/* New clock information update */
	memcpy(&s3c_freqs.new, &clk_info[index],
			sizeof(struct s3c_freq));

	if (s3c_freqs.freqs.new >= s3c_freqs.freqs.old) {
		/* Voltage up code: increase ARM first */
		if (!IS_ERR_OR_NULL(arm_regulator) &&
				!IS_ERR_OR_NULL(internal_regulator)) {
			ret = regulator_set_voltage(arm_regulator,
						    arm_volt, arm_volt_max);
			if (ret)
				goto out;
			ret = regulator_set_voltage(internal_regulator,
						    int_volt, int_volt_max);
			if (ret)
				goto out;
		}
	}
	cpufreq_notify_transition(&s3c_freqs.freqs, CPUFREQ_PRECHANGE);

	if (s3c_freqs.new.fclk != s3c_freqs.old.fclk || first_run)
		pll_changing = 1;

	if (s3c_freqs.new.hclk_msys != s3c_freqs.old.hclk_msys || first_run)
		bus_speed_changing = 1;

	/*
	 * If ONEDRAM(DMC0)'s clock is getting slower, DMC0's
	 * refresh counter should decrease before slowing down
	 * DMC0 clock. We assume that DMC0's source clock never
	 * changes. This is a temporary setting for the transition.
	 * Stable setting is done at the end of this function.
	 */
	reg = (__raw_readl(S5P_CLK_DIV6) & S5P_CLKDIV6_ONEDRAM_MASK)
		>> S5P_CLKDIV6_ONEDRAM_SHIFT;
	if (clkdiv_val[index][8] > reg) {
		reg = backup_dmc0_reg * (reg + 1) / (clkdiv_val[index][8] + 1);
		WARN_ON(reg > 0xFFFF);
		reg &= 0xFFFF;
		__raw_writel(reg, S5P_VA_DMC0 + 0x30);
	}

	/*
	 * If hclk_msys (for DMC1) is getting slower, DMC1's
	 * refresh counter should decrease before slowing down
	 * hclk_msys in order to get rid of glitches in the
	 * transition. This is temporary setting for the transition.
	 * Stable setting is done at the end of this function.
	 *
	 * Besides, we need to consider the case when PLL speed changes,
	 * where the DMC1's source clock hclk_msys is changed from ARMCLK
	 * to MPLL temporarily. DMC1 needs to be ready for this
	 * transition as well.
	 */
	if (s3c_freqs.new.hclk_msys < s3c_freqs.old.hclk_msys || first_run) {
		/*
		 * hclk_msys is up to 12bit. (200000)
		 * reg is 16bit. so no overflow, yet.
		 *
		 * May need to use div64.h later with larger hclk_msys or
		 * DMCx refresh counter. But, we have bugs in do_div and
		 * that should be fixed before.
		 */
		reg = backup_dmc1_reg * s3c_freqs.new.hclk_msys;
		reg /= clk_info[backup_freq_level].hclk_msys;

		/*
		 * When ARM_CLK is absed on APLL->MPLL,
		 * hclk_msys becomes hclk_msys *= MPLL/APLL;
		 *
		 * Based on the worst case scenario, we use MPLL/APLL_MAX
		 * assuming that MPLL clock speed does not change.
		 *
		 * Multiplied first in order to reduce rounding error.
		 * because reg has 15b length, using 64b should be enough to
		 * prevent overflow.
		 */
		if (pll_changing) {
			reg *= mpll_freq;
			reg /= apll_freq_max;
		}
		WARN_ON(reg > 0xFFFF);
		__raw_writel(reg & 0xFFFF, S5P_VA_DMC1 + 0x30);
	}

	/*
	 * APLL should be changed in this level
	 * APLL -> MPLL(for stable transition) -> APLL
	 * Some clock source's clock API  are not prepared. Do not use clock API
	 * in below code.
	 */
	if (pll_changing)
		s5pv210_cpufreq_clksrcs_APLL2MPLL(index, bus_speed_changing);

	/* ARM MCS value changed */
	if (index <= L2) {
		reg = __raw_readl(S5P_ARM_MCS_CON);
		reg &= ~0x3;
		reg |= 0x1;
		__raw_writel(reg, S5P_ARM_MCS_CON);
	}

	reg = __raw_readl(S5P_CLK_DIV0);

	reg &= ~(S5P_CLKDIV0_APLL_MASK | S5P_CLKDIV0_A2M_MASK
			| S5P_CLKDIV0_HCLK200_MASK | S5P_CLKDIV0_PCLK100_MASK
			| S5P_CLKDIV0_HCLK166_MASK | S5P_CLKDIV0_PCLK83_MASK
			| S5P_CLKDIV0_HCLK133_MASK | S5P_CLKDIV0_PCLK66_MASK);

	reg |= ((clkdiv_val[index][0]<<S5P_CLKDIV0_APLL_SHIFT)
			| (clkdiv_val[index][1] << S5P_CLKDIV0_A2M_SHIFT)
			| (clkdiv_val[index][2] << S5P_CLKDIV0_HCLK200_SHIFT)
			| (clkdiv_val[index][3] << S5P_CLKDIV0_PCLK100_SHIFT)
			| (clkdiv_val[index][4] << S5P_CLKDIV0_HCLK166_SHIFT)
			| (clkdiv_val[index][5] << S5P_CLKDIV0_PCLK83_SHIFT)
			| (clkdiv_val[index][6] << S5P_CLKDIV0_HCLK133_SHIFT)
			| (clkdiv_val[index][7] << S5P_CLKDIV0_PCLK66_SHIFT));

	__raw_writel(reg, S5P_CLK_DIV0);

	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & 0xff);

	/* ARM MCS value changed */
	if (index > L2) {
		reg = __raw_readl(S5P_ARM_MCS_CON);
		reg &= ~0x3;
		reg |= 0x3;
		__raw_writel(reg, S5P_ARM_MCS_CON);
	}

	if (pll_changing)
		s5pv210_cpufreq_clksrcs_MPLL2APLL(index, bus_speed_changing);

	/*
	 * Adjust DMC0 refresh ratio according to the rate of DMC0
	 * The DIV value of DMC0 clock changes and SRC value is not controlled.
	 * We assume that no one changes SRC value of DMC0 clock, either.
	 */
	reg = __raw_readl(S5P_CLK_DIV6);
	reg &= ~S5P_CLKDIV6_ONEDRAM_MASK;
	reg |= (clkdiv_val[index][8] << S5P_CLKDIV6_ONEDRAM_SHIFT);
	/* ONEDRAM(DMC0) Clock Divider Ratio: 7+1 for L4, 3+1 for Others */
	__raw_writel(reg, S5P_CLK_DIV6);
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT1);
	} while (reg & (1 << 15));

	/*
	 * If DMC0 clock gets slower (by orginal clock speed / n),
	 * then, the refresh rate should decrease
	 * (by original refresh count / n) (n: divider)
	 */
	reg = backup_dmc0_reg * (clkdiv_val[backup_freq_level][8] + 1)
		/ (clkdiv_val[index][8] + 1);
	__raw_writel(reg & 0xFFFF, S5P_VA_DMC0 + 0x30);

	/*
	 * Adjust DMC1 refresh ratio according to the rate of hclk_msys
	 * (L0~L3: 200 <-> L4: 100)
	 * If DMC1 clock gets slower (by original clock speed * n),
	 * then, the refresh rate should decrease
	 * (by original refresh count * n) (n : clock rate)
	 */
	reg = backup_dmc1_reg * clk_info[index].hclk_msys;
	reg /= clk_info[backup_freq_level].hclk_msys;
	__raw_writel(reg & 0xFFFF, S5P_VA_DMC1 + 0x30);
	cpufreq_notify_transition(&s3c_freqs.freqs, CPUFREQ_POSTCHANGE);

	if (s3c_freqs.freqs.new < s3c_freqs.freqs.old) {
		/* Voltage down: decrease INT first.*/
		if (!IS_ERR_OR_NULL(arm_regulator) &&
				!IS_ERR_OR_NULL(internal_regulator)) {
			regulator_set_voltage(internal_regulator,
					int_volt, int_volt_max);
			regulator_set_voltage(arm_regulator,
					arm_volt, arm_volt_max);
		}
	}

	memcpy(&s3c_freqs.old, &s3c_freqs.new, sizeof(struct s3c_freq));
	cpufreq_debug_printk(CPUFREQ_DEBUG_DRIVER, KERN_INFO,
			"cpufreq: Performance changed[L%d]\n", index);
	previous_arm_volt = dvs_conf[index].arm_volt;

	if (first_run)
		first_run = false;
out:
	mutex_unlock(&set_freq_lock);
	return ret;
}

#ifdef CONFIG_PM
static int s5pv210_cpufreq_suspend(struct cpufreq_policy *policy,
		pm_message_t pmsg)
{
	return 0;
}

static int s5pv210_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret = 0;
	u32 rate;
	int level = CPUFREQ_TABLE_END;
	int i = 0;

	/* Clock information update with wakeup value */
	rate = clk_get_rate(mpu_clk);

	while (freq_table[i].frequency != CPUFREQ_TABLE_END) {
		if (freq_table[i].frequency * 1000 == rate) {
			level = freq_table[i].index;
			break;
		}
		i++;
	}

	if (level == CPUFREQ_TABLE_END) { /* Not found */
		pr_err("[%s:%d] clock speed does not match: "
				"%d. Using L1 of 800MHz.\n",
				__FILE__, __LINE__, rate);
		level = L1;
	}

	memcpy(&s3c_freqs.old, &clk_info[level],
			sizeof(struct s3c_freq));
	previous_arm_volt = dvs_conf[level].arm_volt;

	return ret;
}
#endif

static int __init s5pv210_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	u32 rate ;
	int i, level = CPUFREQ_TABLE_END;
	struct clk *mpll_clk;

	pr_info("S5PV210 CPUFREQ Initialising...\n");

	no_cpufreq_access = 0;

	mpu_clk = clk_get(NULL, "armclk");
	if (IS_ERR(mpu_clk)) {
		pr_err("S5PV210 CPUFREQ cannot get armclk\n");
		return PTR_ERR(mpu_clk);
	}

	if (policy->cpu != 0) {
		pr_err("S5PV210 CPUFREQ cannot get proper cpu(%d)\n",
				policy->cpu);
		return -EINVAL;
	}
	policy->cur = policy->min = policy->max = s5pv210_cpufreq_getspeed(0);

	cpufreq_frequency_table_get_attr(freq_table, policy->cpu);

	policy->cpuinfo.transition_latency = 40000;	/* 1us */

	rate = clk_get_rate(mpu_clk);
	i = 0;

	while (freq_table[i].frequency != CPUFREQ_TABLE_END) {
		if (freq_table[i].frequency * 1000 == rate) {
			level = freq_table[i].index;
			break;
		}
		i++;
	}

	if (level == CPUFREQ_TABLE_END) { /* Not found */
		pr_err("[%s:%d] clock speed does not match: "
				"%d. Using L1 of 800MHz.\n",
				__FILE__, __LINE__, rate);
		level = L1;
	}

	backup_dmc0_reg = __raw_readl(S5P_VA_DMC0 + 0x30) & 0xFFFF;
	backup_dmc1_reg = __raw_readl(S5P_VA_DMC1 + 0x30) & 0xFFFF;
	backup_freq_level = level;
	mpll_clk = clk_get(NULL, "mout_mpll");
	mpll_freq = clk_get_rate(mpll_clk) / 1000 / 1000; /* in MHz */
	clk_put(mpll_clk);
	i = 0;
	do {
		int index = freq_table[i].index;
		if (apll_freq_max < clk_info[index].fclk)
			apll_freq_max = clk_info[index].fclk;
		i++;
	} while (freq_table[i].frequency != CPUFREQ_TABLE_END);
	apll_freq_max /= 1000; /* in MHz */

	memcpy(&s3c_freqs.old, &clk_info[level],
			sizeof(struct s3c_freq));
	previous_arm_volt = dvs_conf[level].arm_volt;

	return cpufreq_frequency_table_cpuinfo(policy, freq_table);
}

static int s5pv210_cpufreq_notifier_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	int ret;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		ret = cpufreq_driver_target(cpufreq_cpu_get(0), SLEEP_FREQ,
				DISABLE_FURTHER_CPUFREQ);
		if (ret < 0)
			return NOTIFY_BAD;
		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		cpufreq_driver_target(cpufreq_cpu_get(0), SLEEP_FREQ,
				ENABLE_FURTHER_CPUFREQ);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct cpufreq_driver s5pv210_cpufreq_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5pv210_cpufreq_verify_speed,
	.target		= s5pv210_cpufreq_target,
	.get		= s5pv210_cpufreq_getspeed,
	.init		= s5pv210_cpufreq_driver_init,
	.name		= "s5pv210",
#ifdef CONFIG_PM
	.suspend	= s5pv210_cpufreq_suspend,
	.resume		= s5pv210_cpufreq_resume,
#endif
};

static struct notifier_block s5pv210_cpufreq_notifier = {
	.notifier_call = s5pv210_cpufreq_notifier_event,
};

static int __init s5pv210_cpufreq_probe(struct platform_device *pdev)
{
	struct s5pv210_cpufreq_data *pdata = dev_get_platdata(&pdev->dev);
	int i, j;

	if (pdata && pdata->size) {
		for (i = 0; i < pdata->size; i++) {
			j = 0;
			while (freq_table[j].frequency != CPUFREQ_TABLE_END) {
				if (freq_table[j].frequency == pdata->volt[i].freq) {
					dvs_conf[j].arm_volt = pdata->volt[i].varm;
					dvs_conf[j].int_volt = pdata->volt[i].vint;
					break;
				}
				j++;
			}
		}
	}

#ifdef CONFIG_REGULATOR
	arm_regulator = regulator_get_exclusive(NULL, "vddarm");
	if (IS_ERR(arm_regulator)) {
		pr_err("failed to get regulater resource vddarm\n");
		goto error;
	}
	internal_regulator = regulator_get_exclusive(NULL, "vddint");
	if (IS_ERR(internal_regulator)) {
		pr_err("failed to get regulater resource vddint\n");
		goto error;
	}
	goto finish;
error:
	pr_warn("Cannot get vddarm or vddint. CPUFREQ Will not"
		       " change the voltage.\n");
finish:
#endif
	register_pm_notifier(&s5pv210_cpufreq_notifier);

	return cpufreq_register_driver(&s5pv210_cpufreq_driver);
}

static struct platform_driver s5pv210_cpufreq_drv = {
	.probe		= s5pv210_cpufreq_probe,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s5pv210-cpufreq",
	},
};

static int __init s5pv210_cpufreq_init(void)
{
	int ret;

	ret = platform_driver_register(&s5pv210_cpufreq_drv);
	if (!ret)
		pr_info("%s: S5PV210 cpu-freq driver\n", __func__);

	return ret;
}

late_initcall(s5pv210_cpufreq_init);
