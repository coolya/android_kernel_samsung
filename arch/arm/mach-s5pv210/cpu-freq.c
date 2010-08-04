/*  linux/arch/arm/mach-s5pv210/cpu-freq.c
 *
 *  Copyright (C) 2010 Samsung Electronics Co., Ltd.
 *
 *  CPUFreq driver support for S5PC110/S5PV210
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

#include <asm/system.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <mach/regs-clock.h>
#include <mach/cpu-freq-v210.h>
#include <plat/pll.h>
#include <plat/clock.h>
#include <plat/s5p-clock.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#define ENABLE_DVFS_LOCK_HIGH 1
#define USE_DVS
#define GPIO_BASED_DVS

#define DBG(fmt...)
//#define DBG(fmt...) printk(fmt)
#define CLK_DIV_CHANGE_BY_STEP 0
#define MAX_DVFS_LEVEL  7

#define	S5P_ONENAND_IF_BASE		0xB0600000

#define	ONENANDCLK_ON		0
#define	ONENANDCLK_OFF		1

unsigned int dvfs_change_direction;
#define CLIP_LEVEL(a, b) (a > b ? b : a)

unsigned int MAXFREQ_LEVEL_SUPPORTED = 4;
unsigned int S5PC11X_MAXFREQLEVEL = 4;
unsigned int S5PC11X_FREQ_TAB;
static spinlock_t g_dvfslock = SPIN_LOCK_UNLOCKED;
static unsigned int s5pc11x_cpufreq_level = 3;
unsigned int s5pc11x_cpufreq_index = 0;

static char cpufreq_governor_name[CPUFREQ_NAME_LEN] = "conservative";// default governor
static char userspace_governor[CPUFREQ_NAME_LEN] = "userspace";
static char conservative_governor[CPUFREQ_NAME_LEN] = "conservative";
int s5pc11x_clk_dsys_psys_change(int index);
int s5pc11x_armclk_set_rate(struct clk *clk, unsigned long rate);

unsigned int prevIndex = 0;

static struct clk * mpu_clk;
#ifdef CONFIG_CPU_FREQ_LOG
static void inform_dvfs_clock_status(struct work_struct *work);
static DECLARE_DELAYED_WORK(dvfs_info_print_work, inform_dvfs_clock_status);
#endif
#if ENABLE_DVFS_LOCK_HIGH
unsigned int g_dvfs_high_lock_token = 0;
static DEFINE_MUTEX(dvfs_high_lock);
unsigned int g_dvfs_high_lock_limit = 4;
unsigned int g_dvfslockval[NUMBER_OF_LOCKTOKEN];
bool g_dvfs_fix_lock_limit = false; // global variable to avoid up frequency scaling 

#endif //ENABLE_DVFS_LOCK_HIGH

/* frequency */
static struct cpufreq_frequency_table s5pc110_freq_table_1GHZ[] = {
	{L0, 1000*1000},
	{L1, 800*1000},
	{L2, 400*1000},
	{L3, 200*1000},
	{L4, 100*1000},
	{0, CPUFREQ_TABLE_END},
};

/*Assigning different index for fast scaling up*/
static unsigned char transition_state_1GHZ[][2] = {
        {1, 0},
        {2, 0},
        {3, 1},
        {4, 2},
        {4, 3},
};

/* frequency */
static struct cpufreq_frequency_table s5pc110_freq_table_1d2GHZ[] = {
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{L3, 400*1000},
	{L4, 200*1000},
	{L5, 100*1000},
	{0, CPUFREQ_TABLE_END},
};

/*Assigning different index for fast scaling up*/
static unsigned char transition_state_1d2GHZ[][2] = {
        {1, 0},
        {2, 0},
        {3, 1},
        {4, 2},
        {5, 3},
        {5, 4},        
};


static unsigned char (*transition_state[2])[2] = {
        transition_state_1GHZ,
        transition_state_1d2GHZ,
};

static struct cpufreq_frequency_table *s5pc110_freq_table[] = {
        s5pc110_freq_table_1GHZ,
        s5pc110_freq_table_1d2GHZ,
};

static unsigned int s5pc110_thres_table_1GHZ[][2] = {
      	{30, 70},
        {30, 70},
        {30, 70},
        {30, 70},
        {30, 70},
};

static unsigned int s5pc110_thres_table_1d2GHZ[][2] = {
      	{30, 70},
        {30, 70},
        {30, 70},
        {30, 70},
        {30, 70},
        {30, 70},
};

static unsigned int  (*s5pc110_thres_table[2])[2] = {
	s5pc110_thres_table_1GHZ,
	s5pc110_thres_table_1d2GHZ,
};


struct S5PC110_clk_info {
	u32	armclk;
	u32 	apllout;
	u32 	mpllout;
	u32	apll_mps;
	u32	mpll_mps;
	u32	msys_div0;
	u32	psys_dsys_div0;
	u32	div2val;
	u32	dmc0_div6;
};


struct S5PC110_clk_info clk_info[] = {
#if USE_1DOT2GHZ
{
	// APLL:1200,ARMCLK:1200,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	.armclk		=	1200* MHZ,
	.apllout	=	1200* MHZ,
	.apll_mps	=	((150<<16)|(3<<8)|1),
	.msys_div0	=	(0|(5<<4)|(5<<8)|(1<<12)),
	.mpllout	=	667* MHZ,
	.mpll_mps	=	((667<<16)|(12<<8)|(1)),
	.psys_dsys_div0 =	((3<<16)|(1<<20)|(4<<24)|(1<<28)),
	.div2val	=	((3<<0)|(3<<4)|(3<<8)),
	.dmc0_div6 	=	(3<<28),
},
#endif// A extra entry for 1200MHZ level 
{
	// APLL:1000,ARMCLK:1000,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	.armclk		=	1000* MHZ,
	.apllout	=	1000* MHZ,
	.apll_mps	=	((125<<16)|(3<<8)|1),
	.msys_div0	=	(0|(4<<4)|(4<<8)|(1<<12)),
	.mpllout	=	667* MHZ,
	.mpll_mps	=	((667<<16)|(12<<8)|(1)),
	.psys_dsys_div0 =	((3<<16)|(1<<20)|(4<<24)|(1<<28)),
	.div2val	=	((3<<0)|(3<<4)|(3<<8)),
	.dmc0_div6 	=	(3<<28),
},
{
	// APLL:800,ARMCLK:800,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	.armclk		=	800* MHZ,
	.apllout	=	800* MHZ,
	.apll_mps	=	((100<<16)|(3<<8)|1),
	.msys_div0	=	(0|(3<<4)|(3<<8)|(1<<12)),
	.mpllout	=	667* MHZ,
	.mpll_mps	=	((667<<16)|(12<<8)|(1)),
	.psys_dsys_div0 =	((3<<16)|(1<<20)|(4<<24)|(1<<28)),
	.div2val	=	((3<<0)|(3<<4)|(3<<8)),
	.dmc0_div6 	=	(3<<28),
},
{
	// APLL:800,ARMCLK:400,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	.armclk		=	400* MHZ,
	.apllout	=	800* MHZ,
	.apll_mps	=	((100<<16)|(3<<8)|1),
	.msys_div0	=	(1|(3<<4)|(1<<8)|(1<<12)),
	.mpllout	=	667* MHZ,
	.mpll_mps	=	((667<<16)|(12<<8)|(1)),
	.psys_dsys_div0 =	((3<<16)|(1<<20)|(4<<24)|(1<<28)),
	.div2val	=	((3<<0)|(3<<4)|(3<<8)),
	.dmc0_div6 	=	(3<<28),
},
{
	// APLL:800,ARMCLK:200,HCLK_MSYS:200,MPLL:667,HCLK_DSYS:166,HCLK_PSYS:133,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	.armclk		=	200* MHZ,
	.apllout	=	800* MHZ,
	.apll_mps	=	((100<<16)|(3<<8)|1),
	.msys_div0	=	(3|(3<<4)|(0<<8)|(1<<12)),
	.mpllout	=	667* MHZ,
	.mpll_mps	=	((667<<16)|(12<<8)|(1)),
	.psys_dsys_div0 =	((3<<16)|(1<<20)|(4<<24)|(1<<28)),
	.div2val	=	((3<<0)|(3<<4)|(3<<8)),
	.dmc0_div6 	=	(3<<28),
},
{
	// APLL:800,ARMCLK:100,HCLK_MSYS:100,MPLL:667,HCLK_DSYS:83,HCLK_PSYS:66,PCLK_MSYS:100,PCLK_DSYS:83,PCLK_PSYS:66
	.armclk		=	100* MHZ,
	.apllout	=	800* MHZ,
	.apll_mps	=	((100<<16)|(3<<8)|1),
	.msys_div0	=	(7|(3<<4)|(0<<8)|(0<<12)),
	.mpllout	=	667* MHZ,
	.mpll_mps	=	((667<<16)|(12<<8)|(1)),
	.psys_dsys_div0 =	((7<<16)|(0<<20)|(9<<24)|(0<<28)),
	.div2val	=	((3<<0)|(3<<4)|(3<<8)),
	.dmc0_div6 	=	(7<<28),
}
};

unsigned long onenand_if_base = 0x00;

static u32 s5p_cpu_clk_tab_size(void)
{
	return ARRAY_SIZE(clk_info);
}

static int s5pc11x_clk_set_withapllchange(unsigned int target_freq,
                                unsigned int index )
{
	u32 val, reg;
	unsigned int mask;	

	/*change the apll*/
	
	//////////////////////////////////////////////////
	/* APLL should be changed in this level
	 * APLL -> MPLL(for stable transition) -> APLL
	 * Some clock source's clock API  are not prepared. Do not use clock API
	 * in below code.
	 */

	if (dvfs_change_direction == 0) {
		__raw_writel(0x40e, S5P_VA_DMC1 + 0x30);
	}

	reg = __raw_readl(S5P_CLK_DIV2);
	DBG("before apll transition DIV2=%x\n",reg);
	reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
	reg |= clk_info[index].div2val ;
	__raw_writel(reg, S5P_CLK_DIV2);	
	DBG("during apll transition DIV2=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & ((1<<16)|(1<<17)));

	/* Change APLL to MPLL in MFC_MUX and G3D MUX*/	
	reg = __raw_readl(S5P_CLK_SRC2);
	DBG("before apll transition SRC2=%x\n",reg);
	reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
	reg |= (1<<S5P_CLKSRC2_G3D_SHIFT) | (1<<S5P_CLKSRC2_MFC_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC2);
	DBG("during apll transition SRC2=%x\n",reg);	
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT1);
	} while (reg & ((1<<7)|(1<<3)));

	if (dvfs_change_direction == 1) {
		__raw_writel(0x40e, S5P_VA_DMC1 + 0x30);
	}

	/* HCLKMSYS change: SCLKAPLL -> SCLKMPLL */
	reg = __raw_readl(S5P_CLK_SRC0);
	DBG("before apll transition SRC0=%x\n",reg);	
	reg &= ~(S5P_CLKSRC0_MUX200_MASK);
	reg |= (0x1 << S5P_CLKSRC0_MUX200_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC0);
	DBG("durint apll transition SRC0=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT0);
	} while (reg & (0x1<<18));	
		
	//////////////////////////////////////////
		
	/*set apll divider value*/	
	val = __raw_readl(S5P_CLK_DIV0);
	val = val & (~(0xffff));
	val = val | clk_info[index].msys_div0;
	__raw_writel(val, S5P_CLK_DIV0);

	mask = S5P_CLK_DIV_STAT0_DIV_APLL | S5P_CLK_DIV_STAT0_DIV_A2M | S5P_CLK_DIV_STAT0_DIV_HCLK_MSYS | S5P_CLK_DIV_STAT0_DIV_PCLK_MSYS;
	do {
		val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
	} while (val);
	DBG("\n DIV0 = %x\n",__raw_readl(S5P_CLK_DIV0));

	/*set apll_con M P S value*/
	val = (0 << 31) | clk_info[index].apll_mps;

	__raw_writel(val, S5P_APLL_CON);
	__raw_writel(val | (1 << 31), S5P_APLL_CON);
	while(!(__raw_readl(S5P_APLL_CON) & (1 << 29)));

	////////////////////////////////////
	
	/*  HCLKMSYS change: SCLKMPLL -> SCLKAPLL */
	reg = __raw_readl(S5P_CLK_SRC0);
	reg &= ~(S5P_CLKSRC0_MUX200_MASK);
	reg |= (0x0 << S5P_CLKSRC0_MUX200_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC0);
	DBG("after apll transition SRC0=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT0);
	} while (reg & (0x1<<18));

	if (dvfs_change_direction == 1) {
		__raw_writel(0x618, S5P_VA_DMC1 + 0x30);
	}

	/* Change MPLL to APLL in MFC_MUX and G3D MUX */
	reg = __raw_readl(S5P_CLK_SRC2);
	reg &= ~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
	reg |= (0<<S5P_CLKSRC2_G3D_SHIFT) | (0<<S5P_CLKSRC2_MFC_SHIFT);
	__raw_writel(reg, S5P_CLK_SRC2);
	DBG("after apll transition SRC2=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_MUX_STAT1);
	} while (reg & ((1<<7)|(1<<3)));

	reg = __raw_readl(S5P_CLK_DIV2);
	reg &= ~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
	reg |= (0x0<<S5P_CLKDIV2_G3D_SHIFT)|(0x0<<S5P_CLKDIV2_MFC_SHIFT);
	__raw_writel(reg, S5P_CLK_DIV2);
	DBG("after apll transition DIV2=%x\n",reg);
	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & ((1<<16)|(1<<17)));

	if (dvfs_change_direction == 0) {
		__raw_writel(0x618, S5P_VA_DMC1 + 0x30);
	}

	////////////////////////////////////

	clk_fout_apll.rate = target_freq ;
	DBG("S5P_APLL_CON = %x, S5P_CLK_DIV0=%x\n",__raw_readl(S5P_APLL_CON),__raw_readl(S5P_CLK_DIV0));

	return 0;
}

int s5pc11x_armclk_set_rate(struct clk *clk, unsigned long rate)
{
	int cur_freq;
	unsigned int mask;

	u32 val;
	u32 size;
        int index;

        index = s5pc11x_cpufreq_index;

	size = s5p_cpu_clk_tab_size();

	if(index >= size)
	{
		printk("=DVFS ERR index(%d) > size(%d)\n", index, size);
		return 1;
	}

	if ( onenand_if_base == 0x00 )
		onenand_if_base = ioremap(S5P_ONENAND_IF_BASE, 4096);

	/* validate target frequency */ 
	if(clk_info[index].armclk != rate)
	{
		DBG("=DVFS ERR target_freq (%d) != cpu_tab_freq (%d)\n", clk_info[index].armclk, rate);
		return 0;
	}

	cur_freq = clk_get_rate(clk);

	/*check if change in DMC0 divider*/
	if(clk_info[prevIndex].dmc0_div6 != clk_info[index].dmc0_div6)
	{
		u32	onenand_if_ctrl = 0x00;

		if(clk_info[index].dmc0_div6 == (3<<28)) { // for 200mhz/166mhz
			__raw_writel(0x618, S5P_VA_DMC1 + 0x30);
			__raw_writel(0x50e, S5P_VA_DMC0 + 0x30);
#if	0

			onenand_clk_ctrl(ONENANDCLK_ON);
			onenand_if_ctrl = __raw_readl(onenand_if_base + 0x100);
			__raw_writel(onenand_if_ctrl | 1 << 2 , onenand_if_base + 0x100);
			printk( "%s %s Line : %d	200M Hz onenand if ctrl : %08x\n", __FILE__, __FUNCTION__, __LINE__, __raw_readl(onenand_if_base + 0x100));
			onenand_clk_ctrl(ONENANDCLK_OFF);
#endif
		} else {					// for 100mhz/83mhz
			__raw_writel(0x30c, S5P_VA_DMC1 + 0x30);
			__raw_writel(0x287, S5P_VA_DMC0 + 0x30);

#if	0			
			onenand_clk_ctrl(ONENANDCLK_ON);

			onenand_if_ctrl = __raw_readl(onenand_if_base + 0x100);
			__raw_writel(onenand_if_ctrl & ~(1 << 2), onenand_if_base + 0x100);

//			printk( "%s %s Line : %d	100M Hz onenand if ctrl : %08x\n", __FILE__, __FUNCTION__, __LINE__, __raw_readl(onenand_if_base + 0x100));
			onenand_clk_ctrl(ONENANDCLK_OFF);
#endif
		}

		val = __raw_readl(S5P_CLK_DIV6);
		val &= ~(S5P_CLKDIV6_ONEDRAM_MASK);
		val |= (clk_info[index].dmc0_div6);
		__raw_writel(val, S5P_CLK_DIV6);
		do {
			val = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (val & ((1<<15)));
	}

	/* check if change in apll */
	if(clk_info[prevIndex].apllout != clk_info[index].apllout)
	{
		DBG("changing apll\n");
		s5pc11x_clk_set_withapllchange(rate, index);
		return 0;	
	}
	DBG("apll target frequency = %d, index=%d\n",clk_info[index].apllout,index);

	/*return if current frequency is same as target frequency*/
	if(cur_freq == clk_info[index].armclk)
		return 0;

	// change clock divider
	mask = (~(S5P_CLKDIV0_APLL_MASK)) & (~(S5P_CLKDIV0_A2M_MASK)) & 
			(~(S5P_CLKDIV0_HCLK200_MASK)) & (~(S5P_CLKDIV0_PCLK100_MASK));
	val = __raw_readl(S5P_CLK_DIV0) & mask;
	val |= clk_info[index].msys_div0;
	
	__raw_writel(val, S5P_CLK_DIV0);

	mask = S5P_CLK_DIV_STAT0_DIV_APLL | S5P_CLK_DIV_STAT0_DIV_A2M | S5P_CLK_DIV_STAT0_DIV_HCLK_MSYS | S5P_CLK_DIV_STAT0_DIV_PCLK_MSYS;
	do {
		val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
	} while (val);
	DBG("\n DIV0 = %x\n",__raw_readl(S5P_CLK_DIV0));



       return 0;
}


/*Change MPll rate*/
int s5pc11x_clk_set_mpll(unsigned int index )
{
	int cur_freq;
	u32 val, mask;
	u32 size;
	u32 mpll_target_freq = 0;
	u32 vsel = 0;
	struct clk *xtal_clk;
        u32 xtal;

	printk("mpll changed.\n");
	size = s5p_cpu_clk_tab_size();

	if(index >= size)
	{
		printk("=DVFS ERR index(%d) > size(%d)\n", index, size);
		return 1;
	}


	mpll_target_freq = clk_info[index].armclk;
	
	cur_freq = clk_fout_mpll.rate;

	DBG("Current mpll frequency = %d\n",cur_freq);
	DBG("target mpll frequency = %d\n",mpll_target_freq);


	/* current frquency is same as target frequency */
	if(cur_freq == mpll_target_freq)
	{
		return 0;
	}

	/* Set mpll_out=fin */ //workaround for evt0
	val = __raw_readl(S5P_CLK_SRC0);
	val = val & (~S5P_CLKSRC0_MPLL_MASK);
	__raw_writel(val, S5P_CLK_SRC0);
	
	/*stop mpll*/
	val  = __raw_readl(S5P_MPLL_CON);
        val = val & (~(0x1 << 31));
        __raw_writel(val, S5P_MPLL_CON);


	/*set mpll divider value*/	
	val = __raw_readl(S5P_CLK_DIV0);
	val = val & (~(0xffff<<16));
	val = val | clk_info[index].psys_dsys_div0;

	__raw_writel(val, S5P_CLK_DIV0);

	/*set mpll_con*/
	xtal_clk = clk_get(NULL, "xtal");
        BUG_ON(IS_ERR(xtal_clk));

        xtal = clk_get_rate(xtal_clk);
        clk_put(xtal_clk);
	DBG("xtal = %d\n",xtal);


	vsel = 0;

	val = (0 << 31) | (0 << 28) | (vsel << 27) | clk_info[index].mpll_mps; 

	__raw_writel(val, S5P_MPLL_CON);
	__raw_writel(val | (1 << 31), S5P_MPLL_CON);
	while(!(__raw_readl(S5P_MPLL_CON) & (1 << 29)));

	/* Set mpll_out=fout */ //workaround for evt0
	val = __raw_readl(S5P_CLK_SRC0);
	val = val | (0x1 << S5P_CLKSRC0_MPLL_SHIFT);
	__raw_writel(val, S5P_CLK_SRC0);


	clk_fout_mpll.rate = mpll_target_freq ;

	mask = S5P_CLK_DIV_STAT0_DIV_PCLK_PSYS | S5P_CLK_DIV_STAT0_DIV_HCLK_PSYS |  S5P_CLK_DIV_STAT0_DIV_PCLK_DSYS | S5P_CLK_DIV_STAT0_DIV_HCLK_DSYS;
	do {
		val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
	} while (val);
	
	DBG("S5P_MPLL_CON = %x, S5P_CLK_DIV0=%x, S5P_CLK_SRC0=%x, \n",__raw_readl(S5P_MPLL_CON),__raw_readl(S5P_CLK_DIV0),__raw_readl(S5P_CLK_SRC0));

	return 0;
}

int s5pc11x_clk_dsys_psys_change(int index) {
	unsigned int val, mask;
	u32 size;

	size = s5p_cpu_clk_tab_size();

	if(index >= size)
	{
		printk("index(%d) > size(%d)\n", index, size);
		return 1;
	}

	/* check if change in mpll */
        if(clk_info[prevIndex].mpllout != clk_info[index].mpllout)
        {
                DBG("changing mpll\n");
                s5pc11x_clk_set_mpll(index);
                return 0;
        }


	val = __raw_readl(S5P_CLK_DIV0);

	if ((val & 0xFFFF0000) == clk_info[index].psys_dsys_div0) {
		DBG("No change in psys, dsys domain\n");
		return 0;
	} else {
		mask = (~(S5P_CLKDIV0_HCLK166_MASK)) & (~(S5P_CLKDIV0_HCLK133_MASK)) & 
			(~(S5P_CLKDIV0_PCLK83_MASK)) & (~(S5P_CLKDIV0_PCLK66_MASK)) ;
		val = val & mask;
		val |= clk_info[index].psys_dsys_div0;
		__raw_writel(val, S5P_CLK_DIV0);


		DBG("DSYS/PSYS DIV0 = %x\n",__raw_readl(S5P_CLK_DIV0));
		//udelay(30);
		mask = S5P_CLK_DIV_STAT0_DIV_PCLK_PSYS | S5P_CLK_DIV_STAT0_DIV_HCLK_PSYS | S5P_CLK_DIV_STAT0_DIV_PCLK_DSYS | S5P_CLK_DIV_STAT0_DIV_HCLK_DSYS;
		do {
			val = __raw_readl(S5P_CLK_DIV_STAT0) & mask;
		} while (val);

		return 0;
	}	
}

/*return performance level */
static int get_dvfs_perf_level(enum freq_level_states freq_level, unsigned int *perf_level)
{
	unsigned int freq=0, index = 0;
	struct cpufreq_frequency_table *freq_tab = s5pc110_freq_table[S5PC11X_FREQ_TAB];
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


// for active high with event from TS and key
static int dvfs_perf_lock = 0;
int dvfs_change_quick = 0;

// jump to the given performance level
static void set_dvfs_perf_level(unsigned int perf_level) 
{
	//unsigned long irqflags;

	spin_lock(&g_dvfslock);
	if(s5pc11x_cpufreq_index > perf_level) {
		s5pc11x_cpufreq_index = perf_level; // jump to specified level 
		dvfs_change_quick = 1;
	}
	spin_unlock(&g_dvfslock);
	return;
}

//Jump to the given frequency level 
void set_dvfs_target_level(enum freq_level_states freq_level) 
{
	unsigned int ret = 0, perf_level=0;
	
	ret = get_dvfs_perf_level(freq_level, &perf_level);
	if(ret)
		return;

	set_dvfs_perf_level(perf_level);
	return;
}

#if ENABLE_DVFS_LOCK_HIGH
//Lock and jump to the given frequency level 
void s5pc110_lock_dvfs_high_level(unsigned int nToken, enum freq_level_states freq_level) 
{
	unsigned int nLevel, ret, perf_level=0;
	//printk("dvfs lock with token %d\n",nToken);
	ret = get_dvfs_perf_level(freq_level, &perf_level);
	if(ret)
		return;
	nLevel = perf_level;	
	if (nToken == DVFS_LOCK_TOKEN_6 ) nLevel--; // token for launcher , this can use 1GHz
	// check lock corruption
	if (g_dvfs_high_lock_token & (1 << nToken) ) printk ("\n\n[DVFSLOCK] lock token %d is already used!\n\n", nToken);
	//mutex_lock(&dvfs_high_lock);
	g_dvfs_high_lock_token |= (1 << nToken);
	g_dvfslockval[nToken] = nLevel;
	if (nLevel <  g_dvfs_high_lock_limit)
		g_dvfs_high_lock_limit = nLevel;
	//mutex_unlock(&dvfs_high_lock);
	set_dvfs_perf_level(nLevel);
}
EXPORT_SYMBOL(s5pc110_lock_dvfs_high_level);

void s5pc110_unlock_dvfs_high_level(unsigned int nToken) 
{
	unsigned int i;
	//printk("dvfs unlock with token %d\n",nToken);
	//mutex_lock(&dvfs_high_lock);
	g_dvfs_high_lock_token &= ~(1 << nToken);
	g_dvfslockval[nToken] = MAXFREQ_LEVEL_SUPPORTED-1;
	g_dvfs_high_lock_limit = MAXFREQ_LEVEL_SUPPORTED-1;

	if (g_dvfs_high_lock_token) {
		for (i=0;i<NUMBER_OF_LOCKTOKEN;i++) {
			if (g_dvfslockval[i] < g_dvfs_high_lock_limit)  g_dvfs_high_lock_limit = g_dvfslockval[i];
		}
	}

	//mutex_unlock(&dvfs_high_lock);
}
EXPORT_SYMBOL(s5pc110_unlock_dvfs_high_level);
#endif //ENABLE_DVFS_LOCK_HIGH

unsigned int s5pc11x_target_frq(unsigned int pred_freq, 
				int flag)
{
	int index;
	//unsigned long irqflags;
	unsigned int freq;

	struct cpufreq_frequency_table *freq_tab = s5pc110_freq_table[S5PC11X_FREQ_TAB];
	
	spin_lock(&g_dvfslock);
	if(freq_tab[0].frequency < pred_freq) {
	   index = 0;	
	   goto s5pc11x_target_frq_end;
	}

	if((flag != 1)&&(flag != -1)) {
		printk("s5pc1xx_target_frq: flag error!!!!!!!!!!!!!");
	}

	index = s5pc11x_cpufreq_index;

	if(freq_tab[index].frequency == pred_freq) {	
		if(flag == 1)
			index = transition_state[S5PC11X_FREQ_TAB][index][1];
		else
			index = transition_state[S5PC11X_FREQ_TAB][index][0];
	}
	/*else {
		index = 0; 
	}*/

	if (g_dvfs_high_lock_token) {
		 if(g_dvfs_fix_lock_limit == true) {
			 index = g_dvfs_high_lock_limit;// use the same level
		 }
		 else {
			if (index > g_dvfs_high_lock_limit)
				index = g_dvfs_high_lock_limit;
		 }
	}
	//printk("s5pc11x_target_frq index = %d\n",index);

s5pc11x_target_frq_end:
	//spin_lock_irqsave(&g_cpufreq_lock, irqflags);
	index = CLIP_LEVEL(index, s5pc11x_cpufreq_level);
	s5pc11x_cpufreq_index = index;
	//spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
	
	freq = freq_tab[index].frequency;
	spin_unlock(&g_dvfslock);
	return freq;
}


int s5pc11x_target_freq_index(unsigned int freq)
{
	int index = 0;
	//unsigned long irqflags;
	
	struct cpufreq_frequency_table *freq_tab = s5pc110_freq_table[S5PC11X_FREQ_TAB];

	if(freq >= freq_tab[index].frequency) {
		goto s5pc11x_target_freq_index_end;
	}

	/*Index might have been calculated before calling this function.
	check and early return if it is already calculated*/
	if(freq_tab[s5pc11x_cpufreq_index].frequency == freq) {		
		return s5pc11x_cpufreq_index;
	}

	while((freq < freq_tab[index].frequency) &&
			(freq_tab[index].frequency != CPUFREQ_TABLE_END)) {
		index++;
	}

	if(index > 0) {
		if(freq != freq_tab[index].frequency) {
			index--;
		}
	}

	if(freq_tab[index].frequency == CPUFREQ_TABLE_END) {
		index--;
	}

s5pc11x_target_freq_index_end:
	spin_lock(&g_dvfslock);
	index = CLIP_LEVEL(index, s5pc11x_cpufreq_level);
	s5pc11x_cpufreq_index = index;
	spin_unlock(&g_dvfslock);
	
	return index;
} 


int s5pc110_pm_target(unsigned int target_freq)
{
        int ret = 0;
        unsigned long arm_clk;
        unsigned int index;


        index = s5pc11x_target_freq_index(target_freq);
        if(index == INDX_ERROR) {
           printk("s5pc110_target: INDX_ERROR \n");
           return -EINVAL;
        }

        arm_clk = s5pc110_freq_table[S5PC11X_FREQ_TAB][index].frequency;
        
        target_freq = arm_clk;

#ifdef USE_DVS
#ifdef GPIO_BASED_DVS
	set_voltage_dvs(index);
#else
        set_voltage(index);
#endif
#endif
        /* frequency scaling */
        ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
        if(ret != 0) {
                printk("frequency scaling error\n");
                return -EINVAL;
        }

        return ret;
}

int is_userspace_gov(void)
{
        int ret = 0;
        //unsigned long irqflags;
        //spin_lock_irqsave(&g_cpufreq_lock, irqflags);
        if(!strnicmp(cpufreq_governor_name, userspace_governor, CPUFREQ_NAME_LEN)) {
                ret = 1;
        }
       // spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
        return ret;
}

int is_conservative_gov(void)
{
        int ret = 0;
        //unsigned long irqflags;
        //spin_lock_irqsave(&g_cpufreq_lock, irqflags);
        if(!strnicmp(cpufreq_governor_name, conservative_governor, CPUFREQ_NAME_LEN)) {
                ret = 1;
        }
       // spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
        return ret;
}


/* TODO: Add support for SDRAM timing changes */

int s5pc110_verify_speed(struct cpufreq_policy *policy)
{

	if (policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, s5pc110_freq_table[S5PC11X_FREQ_TAB]);
}

unsigned int s5pc110_getspeed(unsigned int cpu)
{
	unsigned long rate;

	if (cpu)
		return 0;

	rate = clk_get_rate(mpu_clk) / KHZ_T;

	return rate;
}

extern void print_clocks(void);
extern int store_up_down_threshold(unsigned int down_threshold_value,
				unsigned int up_threshold_value);
extern void dvs_set_for_1dot2Ghz (int onoff);
extern bool gbTransitionLogEnable;
static int s5pc110_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	struct cpufreq_freqs freqs;
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index;

	//unsigned long irqflags;

	DBG("s5pc110_target called for freq=%d\n",target_freq);

	freqs.old = s5pc110_getspeed(0);
	DBG("old _freq = %d\n",freqs.old);

	if(policy != NULL) {
		if(policy -> governor) {
			//spin_lock_irqsave(&g_cpufreq_lock, irqflags);
			if (strnicmp(cpufreq_governor_name, policy->governor->name, CPUFREQ_NAME_LEN)) {
				strcpy(cpufreq_governor_name, policy->governor->name);
			}
			//spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
		}
	}

	index = s5pc11x_target_freq_index(target_freq);
	if(index == INDX_ERROR) {
       	printk("s5pc110_target: INDX_ERROR \n");
		return -EINVAL;
        }
	DBG("Got index = %d\n",index);

	if(prevIndex == index)
	{	

		DBG("Target index = Current index\n");
       	return ret;
	}

	arm_clk = s5pc110_freq_table[S5PC11X_FREQ_TAB][index].frequency;

	freqs.new = arm_clk;
	freqs.cpu = 0;

	target_freq = arm_clk;
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	//spin_lock_irqsave(&g_cpufreq_lock, irqflags);

	if(prevIndex < index) { // clock down
       	dvfs_change_direction = 0;
		/* frequency scaling */
		ret = s5pc11x_clk_dsys_psys_change(index);

		ret = s5pc11x_armclk_set_rate(mpu_clk, target_freq * KHZ_T);
		//ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0) {
			printk("frequency scaling error\n");
			ret = -EINVAL;
			goto s5pc110_target_end;
		}
		
		// ARM MCS value set
		if (S5PC11X_FREQ_TAB  == 0) { // for 1G table
			if ((prevIndex < 3) && (index >= 3)) {
				ret = __raw_readl(S5P_ARM_MCS);
				DBG("MDSvalue = %08x\n", ret);
				ret = (ret & ~(0x3)) | 0x3;
				__raw_writel(ret, S5P_ARM_MCS);
			}
		} else if (S5PC11X_FREQ_TAB  == 1) { // for 1.2Ghz table
			if ((prevIndex < 4) && (index >= 4)) {
				ret = __raw_readl(S5P_ARM_MCS);
				ret = (ret & ~(0x3)) | 0x3;
				__raw_writel(ret, S5P_ARM_MCS);
			}		
		} else {
			DBG("\n\nERROR\n\n INVALID DVFS TABLE !!\n");
			return ret;
		}


		if (S5PC11X_FREQ_TAB) {	
			if (index <= 2)
				dvs_set_for_1dot2Ghz(1);
			else if (index >= 3)
				dvs_set_for_1dot2Ghz(0);
		}


#ifdef USE_DVS
#ifdef GPIO_BASED_DVS
		set_voltage_dvs(index);
#else
		/* voltage scaling */
		set_voltage(index);
#endif
#endif
		dvfs_change_direction = -1;
	}else{                                          // clock up
		dvfs_change_direction = 1;

		if (S5PC11X_FREQ_TAB) {	
			if (index <= 2)
				dvs_set_for_1dot2Ghz(1);
			else if (index >= 3)
				dvs_set_for_1dot2Ghz(0);
		}

#ifdef USE_DVS
#ifdef GPIO_BASED_DVS
		set_voltage_dvs(index);
#else
		/* voltage scaling */
		set_voltage(index);
#endif
#endif

		// ARM MCS value set
		if (S5PC11X_FREQ_TAB  == 0) { // for 1G table
			if ((prevIndex >= 3) && (index < 3)) {
				ret = __raw_readl(S5P_ARM_MCS);
				DBG("MDSvalue = %08x\n", ret);				
				ret = (ret & ~(0x3)) | 0x1;
				__raw_writel(ret, S5P_ARM_MCS);
			}
		} else if (S5PC11X_FREQ_TAB  == 1) { // for 1.2G table
			if ((prevIndex >= 4) && (index < 4)) {
				ret = __raw_readl(S5P_ARM_MCS);
				ret = (ret & ~(0x3)) | 0x1;
				__raw_writel(ret, S5P_ARM_MCS);
			}		
		} else {
			DBG("\n\nERROR\n\n INVALID DVFS TABLE !!\n");
			return ret;
		}

		/* frequency scaling */
		ret = s5pc11x_armclk_set_rate(mpu_clk, target_freq * KHZ_T);
		//ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0) {
			printk("frequency scaling error\n");
			ret = -EINVAL;
			goto s5pc110_target_end;
		}
		ret = s5pc11x_clk_dsys_psys_change(index);
		dvfs_change_direction = -1;
        }

	//spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	prevIndex = index; // save to preIndex

	mpu_clk->rate = freqs.new * KHZ_T;

#if 0 // not using it as of now
	/*change the frequency threshold level*/
	store_up_down_threshold(s5pc110_thres_table[S5PC11X_FREQ_TAB][index][0], 
				s5pc110_thres_table[S5PC11X_FREQ_TAB][index][1]);
#endif
	DBG("Perf changed[L%d] freq=%d\n",index,arm_clk);
#ifdef CONFIG_CPU_FREQ_LOG
	if(gbTransitionLogEnable == true)
	{
		DBG("Perf changed[L%d]\n",index);
		printk("[DVFS Transition]...\n");
		print_clocks();
	}
#endif
	return ret;
s5pc110_target_end:
	//spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
	return ret;
}

#ifdef CONFIG_CPU_FREQ_LOG
static void inform_dvfs_clock_status(struct work_struct *work) {
//	if (prevIndex == )
	printk("[Clock Info]...\n");	
	print_clocks();
	schedule_delayed_work(&dvfs_info_print_work, 1 * HZ);
}
#endif

#ifdef CONFIG_HAS_WAKELOCK
#if 0
void s5pc11x_cpufreq_powersave(struct early_suspend *h)
{
	//unsigned long irqflags;
	//spin_lock_irqsave(&g_cpufreq_lock, irqflags);
	s5pc11x_cpufreq_level = S5PC11X_MAXFREQLEVEL + 2;
	//spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
	return;
}

void s5pc11x_cpufreq_performance(struct early_suspend *h)
{
	//unsigned long irqflags;
	if(!is_userspace_gov()) {
		//spin_lock_irqsave(&g_cpufreq_lock, irqflags);
		s5pc11x_cpufreq_level = S5PC11X_MAXFREQLEVEL;
		s5pc11x_cpufreq_index = CLIP_LEVEL(s5pc11x_cpufreq_index, S5PC11X_MAXFREQLEVEL);
		//spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
		s5pc110_target(NULL, s5pc110_freq_table[S5PC11X_FREQ_TAB][s5pc11x_cpufreq_index].frequency, 1);
	}
	else {
		//spin_lock_irqsave(&g_cpufreq_lock, irqflags);
		s5pc11x_cpufreq_level = S5PC11X_MAXFREQLEVEL;
		//spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);
#ifdef USE_DVS
#ifdef GPIO_BASED_DVS
		set_voltage_dvs(s5pc11x_cpufreq_index);
#else
		set_voltage(s5pc11x_cpufreq_index);
#endif
#endif
	}
	return;
}

static struct early_suspend s5pc11x_freq_suspend = {
	.suspend = s5pc11x_cpufreq_powersave,
	.resume = s5pc11x_cpufreq_performance,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
};
#endif
#endif //CONFIG_HAS_WAKELOCK


unsigned int get_min_cpufreq(void)
{	unsigned int frequency;
	frequency = s5pc110_freq_table[S5PC11X_FREQ_TAB][S5PC11X_MAXFREQLEVEL].frequency;
	return frequency;
}

static int __init s5pc110_cpu_init(struct cpufreq_policy *policy)
{
	u32 i;
	//unsigned long irqflags;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	/*set the table for maximum performance*/

	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5pc110_getspeed(0);
	//spin_lock_irqsave(&g_cpufreq_lock, irqflags);

#if USE_1DOT2GHZ
		S5PC11X_FREQ_TAB = 1;
		S5PC11X_MAXFREQLEVEL = 5;
		MAXFREQ_LEVEL_SUPPORTED = 6;
		g_dvfs_high_lock_limit = 5;
#else
		S5PC11X_FREQ_TAB = 0;
		S5PC11X_MAXFREQLEVEL = 4;
		MAXFREQ_LEVEL_SUPPORTED = 5;
		g_dvfs_high_lock_limit = 4;
#endif
	
	printk("S5PC11X_FREQ_TAB=%d , S5PC11X_MAXFREQLEVEL=%d\n",S5PC11X_FREQ_TAB,S5PC11X_MAXFREQLEVEL);

	s5pc11x_cpufreq_level = S5PC11X_MAXFREQLEVEL;
      //spin_unlock_irqrestore(&g_cpufreq_lock, irqflags);

	if (S5PC11X_FREQ_TAB) {	
		prevIndex = 2;// we are currently at 800MHZ level
	} else {
		prevIndex = 1;// we are currently at 800MHZ level
	}

#ifdef CONFIG_CPU_FREQ_LOG
	//schedule_delayed_work(&dvfs_info_print_work, 60 * HZ);
#endif
	cpufreq_frequency_table_get_attr(s5pc110_freq_table[S5PC11X_FREQ_TAB], policy->cpu);

	policy->cpuinfo.transition_latency = 100000; //40000;	//1us

#ifdef CONFIG_HAS_WAKELOCK
//	register_early_suspend(&s5pc11x_freq_suspend);	
#endif

	#if ENABLE_DVFS_LOCK_HIGH
	/*initialise the dvfs lock level table*/
	for(i = 0; i < NUMBER_OF_LOCKTOKEN; i++)
		g_dvfslockval[i] = MAXFREQ_LEVEL_SUPPORTED-1;
	#endif


	return cpufreq_frequency_table_cpuinfo(policy, s5pc110_freq_table[S5PC11X_FREQ_TAB]);
}

static struct cpufreq_driver s5pc110_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5pc110_verify_speed,
	.target		= s5pc110_target,
	.get		= s5pc110_getspeed,
	.init		= s5pc110_cpu_init,
	.name		= "s5pc110",
};

static int __init s5pc110_cpufreq_init(void)
{
	return cpufreq_register_driver(&s5pc110_driver);
}

//arch_initcall(s5pc110_cpufreq_init);
module_init(s5pc110_cpufreq_init);
