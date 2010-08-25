/* linux/arch/arm/plat-s5p/setup-csis.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P - Base MIPI-CSI2 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <mach/map.h>
#include <mach/regs-clock.h>
#include <linux/io.h>

struct platform_device; /* don't need the contents */

void s3c_csis_cfg_gpio(void) { }

void s3c_csis_cfg_phy_global(int on)
{
	u32 cfg;

	if (on) {
		/* MIPI D-PHY Power Enable */
		cfg = __raw_readl(S5P_MIPI_CONTROL);
		cfg |= S5P_MIPI_DPHY_EN;
		__raw_writel(cfg, S5P_MIPI_CONTROL);
	} else {
		/* MIPI Power Disable */
		cfg = __raw_readl(S5P_MIPI_CONTROL);
		cfg &= ~S5P_MIPI_DPHY_EN;
		__raw_writel(cfg, S5P_MIPI_CONTROL);
	}
}

