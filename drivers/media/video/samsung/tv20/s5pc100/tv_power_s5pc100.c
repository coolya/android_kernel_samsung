/* linux/drivers/media/video/samsung/tv20/s5pc100/tv_power_s5pc100.c
 *
 * power raw ftn  file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/uaccess.h>
#include <linux/io.h>

#include <mach/map.h>
#include <plat/regs-clock.h>
#include <plat/regs-power.h>

#include "tv_out_s5pc100.h"

#if defined USE_POWERCON_FUNCTION
#undef USE_POWERCON_FUNCTION
#endif

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_TVOUT_PM_DEBUG 1
#endif

#ifdef S5P_TVOUT_PM_DEBUG
#define TVPMPRINTK(fmt, args...) \
	printk(KERN_INFO "\t\t[TVPM] %s: " fmt, __func__ , ## args)
#else
#define TVPMPRINTK(fmt, args...)
#endif

#define TVPWR_SUBSYSTEM_ACTIVE (1<<4)
#define TVPWR_SUBSYSTEM_LP     (0<<4)

#define TVPWR_MTC_COUNTER_CLEAR(a) (((~0xf)<<16)&a)
#define TVPWR_MTC_COUNTER_SET(a)   ((0xf&a)<<16)

#define TVPWR_TV_BLOCK_STATUS(a)    ((0x1<<4)&a)

#define TVPWR_DAC_STATUS(a)   	((0x1<<26)&a)
#define TVPWR_DAC_ON    	(1<<26)

static unsigned short g_dacPwrOn;


void __s5p_tv_power_init_mtc_stable_counter(unsigned int value)
{
	TVPMPRINTK("(%d)\n\r", value);

	writel(TVPWR_MTC_COUNTER_CLEAR((readl(S5P_MTC_STABLE) |
		TVPWR_MTC_COUNTER_SET(value))),
		S5P_MTC_STABLE);

	TVPMPRINTK("(0x%08x)\n\r", readl(S5P_MTC_STABLE));
}

void __s5p_tv_powerinitialize_dac_onoff(unsigned short on)
{
	TVPMPRINTK("(%d)\n\r", on);

	g_dacPwrOn = on;

	TVPMPRINTK("(0x%08x)\n\r", g_dacPwrOn);
}

void __s5p_tv_powerset_dac_onoff(unsigned short on)
{
	TVPMPRINTK("(%d)\n\r", on);

	if (on)
		writel(readl(S5P_OTHERS) | TVPWR_DAC_ON, S5P_OTHERS);
	else
		writel(readl(S5P_OTHERS) & ~TVPWR_DAC_ON, S5P_OTHERS);


	TVPMPRINTK("(0x%08x)\n\r", readl(S5P_OTHERS));
}


unsigned short __s5p_tv_power_get_power_status(void)
{

	TVPMPRINTK("()\n\r");

	TVPMPRINTK("(0x%08x)\n\r", readl(S5P_BLK_PWR_STAT));


	return TVPWR_TV_BLOCK_STATUS(readl(S5P_BLK_PWR_STAT)) ? 1 : 0;
}

unsigned short __s5p_tv_power_get_dac_power_status(void)
{
	TVPMPRINTK("()\n\r");

	TVPMPRINTK("(0x%08x)\n\r", readl(S5P_OTHERS));

	return TVPWR_DAC_STATUS(readl(S5P_OTHERS)) ? 1 : 0;
}

void __s5p_tv_poweron(void)
{
	TVPMPRINTK("()\n\r");

	writel(readl(S5P_NORMAL_CFG) | TVPWR_SUBSYSTEM_ACTIVE,
		S5P_NORMAL_CFG);

	while (!TVPWR_TV_BLOCK_STATUS(readl(S5P_BLK_PWR_STAT)))
		msleep(1);


	TVPMPRINTK("0x%08x,0x%08x)\n\r", readl(S5P_NORMAL_CFG),
		readl(S5P_BLK_PWR_STAT));
}


void __s5p_tv_poweroff(void)
{
	TVPMPRINTK("()\n\r");

	__s5p_tv_powerset_dac_onoff(0);

	writel(readl(S5P_NORMAL_CFG) & ~TVPWR_SUBSYSTEM_ACTIVE,
		S5P_NORMAL_CFG);

	while (TVPWR_TV_BLOCK_STATUS(readl(S5P_BLK_PWR_STAT)))
		msleep(1);


	TVPMPRINTK("0x%08x,0x%08x)\n\r", readl(S5P_NORMAL_CFG),
		readl(S5P_BLK_PWR_STAT));
}
