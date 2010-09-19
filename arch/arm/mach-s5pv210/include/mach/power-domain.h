/* linux/arch/arm/mach-s5pv210/include/mach/power-domain.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Power domain support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_POWER_DOMAIN_H
#define __ASM_ARCH_POWER_DOMAIN_H __FILE__

#define S5PV210_PD_IROM         (1 << 20)
#define S5PV210_PD_AUDIO        (1 << 7)
#define S5PV210_PD_CAM          (1 << 5)
#define S5PV210_PD_TV           (1 << 4)
#define S5PV210_PD_LCD          (1 << 3)
#define S5PV210_PD_G3D          (1 << 2)
#define S5PV210_PD_MFC          (1 << 1)

struct regulator_init_data;

/**
 * struct s5pv210_pd_config - s5pv210_pd_config structure
 * @supply_name:		Name of the regulator supply
 * @microvolts:			Output voltage of regulator
 * @startup_delay:		Start-up time in microseconds
 * @init_data:			regulator_init_data
 * @clk_should_be_running:	the clocks for the IPs in the power domain
 *				should be running when the power domain
 *				is turned on
 * @ctrlbit:			register control bit
 *
 * This structure contains samsung power domain regulator configuration
 * information that must be passed by platform code to the samsung
 * power domain regulator driver.
 */
struct s5pv210_pd_config {
	const char *supply_name;
	int microvolts;
	unsigned startup_delay;
	struct regulator_init_data *init_data;
	struct clk_should_be_running *clk_run;
	int ctrlbit;
};

extern struct platform_device s5pv210_pd_audio;
extern struct platform_device s5pv210_pd_cam;
extern struct platform_device s5pv210_pd_tv;
extern struct platform_device s5pv210_pd_lcd;
extern struct platform_device s5pv210_pd_g3d;
extern struct platform_device s5pv210_pd_mfc;

#endif
