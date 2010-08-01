/* arch/arm/mach-s5pv210/include/mach/cpu-freq-v210.h
 *
 *  Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpufreq.h>

//extern void s5pc110_lock_power_domain(unsigned int nToken);

#define MAXIMUM_FREQ 1000000
#define USE_FREQ_TABLE
//#undef USE_DVS
#define USE_DVS
#define VERY_HI_RATE  800*1000*1000
#define APLL_GEN_CLK  800*1000
#define KHZ_T		1000

#define MPU_CLK		"armclk"

#define INDX_ERROR  65535

enum perf_level {
	L0,
	L1,
	L2,
	L3,
	L4,
	L5,
	L6,
	L7,
};

enum freq_level_states {
	LEV_1200MHZ,
	LEV_1000MHZ,
	LEV_800MHZ,
	LEV_400MHZ,
	LEV_200MHZ,
	LEV_100MHZ,
};

extern unsigned int s5pc11x_cpufreq_index;
extern unsigned int S5PC11X_FREQ_TAB;
extern unsigned int S5PC11X_MAXFREQLEVEL;

extern unsigned int s5pc11x_target_frq(unsigned int pred_freq, int flag);
extern int s5pc110_pm_target(unsigned int target_freq);
extern int is_conservative_gov(void);
extern int is_userspace_gov(void);
extern void set_dvfs_target_level(enum freq_level_states freq_level);
extern int set_voltage(enum perf_level p_lv);
extern int set_voltage_dvs(enum perf_level p_lv);

extern int s5pc110_dvfs_lock_high_hclk(unsigned int dToken);
extern int s5pc110_dvfs_unlock_high_hclk(unsigned int dToken);

#define NUMBER_OF_LOCKTOKEN 9

#define DVFS_LOCK_TOKEN_1	 0
#define DVFS_LOCK_TOKEN_2	 1
#define DVFS_LOCK_TOKEN_3	 2
#define DVFS_LOCK_TOKEN_4	 3
#define DVFS_LOCK_TOKEN_5	 4
#define DVFS_LOCK_TOKEN_6	 5
#define DVFS_LOCK_TOKEN_7	 6
#define DVFS_LOCK_TOKEN_8	 7
#define DVFS_LOCK_TOKEN_9	 8

void s5pc110_lock_dvfs_high_level(unsigned int nToken, enum freq_level_states freq_level);
void s5pc110_unlock_dvfs_high_level(unsigned int nToken);

#define CLK_OUT_PROBING	//TP80 on SMDKC100 board


#define CLK_DIV0_MASK	((0x7<<0)|(0x7<<8)|(0x7<<12))	// APLL,HCLK_MSYS,PCLK_MSYS mask value 


/*
 * APLL M,P,S value for target frequency
 **/
#define APLL_VAL_1664	(1<<31)|(417<<16)|(3<<8)|(0)
#define APLL_VAL_1332	(1<<31)|(444<<16)|(4<<8)|(0)
#define APLL_VAL_1000	(1<<31)|(125<<16)|(3<<8)|(1)
#define APLL_VAL_800	(1<<31)|(100<<16)|(3<<8)|(1)

struct s5pv210_domain_freq {
	unsigned long	apll_out;
	unsigned long	armclk;
	unsigned long	hclk_msys;
	unsigned long	pclk_msys;
	unsigned long	hclk_dsys;
	unsigned long	pclk_dsys;
	unsigned long	hclk_psys;
	unsigned long	pclk_psys;
};

struct s5pv210_cpufreq_freqs {
	struct cpufreq_freqs	freqs;
	struct s5pv210_domain_freq	old;
	struct s5pv210_domain_freq	new;
};

struct s5pv210_dvs_conf {
	const unsigned long	lvl;		/* DVFS level : L0,L1,L2,L3.*/
	unsigned long		arm_volt;	/* uV */
	unsigned long		int_volt;	/* uV */
};

extern void s5pv210_set_cpufreq_level(unsigned int flag);
