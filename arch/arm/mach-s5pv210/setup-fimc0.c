/* linux/arch/arm/mach-s5pv210/setup-fimc0.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base FIMC 0 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/map-s5p.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>
#include <plat/regs-fimc.h>

struct platform_device; /* don't need the contents */

void s3c_fimc0_cfg_gpio(struct platform_device *pdev)
{
	int i = 0;

	/* CAM A port(b0010) : PCLK, VSYNC, HREF, DATA[0-4] */
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPE0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPE0(i), S3C_GPIO_PULL_NONE);
	}
	/* CAM A port(b0010) : DATA[5-7], CLKOUT(MIPI CAM also), FIELD */
	for (i = 0; i < 3; i++) {
		s3c_gpio_cfgpin(S5PV210_GPE1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPE1(i), S3C_GPIO_PULL_NONE);
	}

#if defined(CONFIG_MACH_SMDKC110) || defined(CONFIG_MACH_SMDKV210)
	s3c_gpio_cfgpin(S5PV210_GPE1(4), S5PV210_GPE1_4_CAM_A_FIELD);
	s3c_gpio_setpull(S5PV210_GPE1(4), S3C_GPIO_PULL_NONE);

	/* CAM B port(b0011) : DATA[0-7] */
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPJ0(i), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5PV210_GPJ0(i), S3C_GPIO_PULL_NONE);
	}
	/* CAM B port(b0011) : PCLK, VSYNC, HREF, FIELD, CLCKOUT */
	for (i = 0; i < 5; i++) {
		s3c_gpio_cfgpin(S5PV210_GPJ1(i), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5PV210_GPJ1(i), S3C_GPIO_PULL_NONE);
	}
#endif

	/* note : driver strength to max is unnecessary */
}

int s3c_fimc_clk_on(struct platform_device *pdev, struct clk *clk)
{
	struct clk *sclk_fimc_lclk = NULL;
	struct clk *mout_fimc_lclk = NULL;
	struct clk *mout_mpll = NULL;

	mout_mpll = clk_get(&pdev->dev, "mout_mpll");
	if (IS_ERR(mout_mpll)) {
		dev_err(&pdev->dev, "failed to get mout_mpll\n");
		goto err_clk1;
	}

	mout_fimc_lclk = clk_get(&pdev->dev, "mout_fimc_lclk");
	if (IS_ERR(mout_fimc_lclk)) {
		dev_err(&pdev->dev, "failed to get mout_fimc_lclk\n");
		goto err_clk2;
	}

	sclk_fimc_lclk = clk_get(&pdev->dev, "sclk_fimc_lclk");
	if (IS_ERR(sclk_fimc_lclk)) {
		dev_err(&pdev->dev, "failed to get sclk_fimc_lclk\n");
		goto err_clk3;
	}

	clk_set_parent(mout_fimc_lclk, mout_mpll);
	clk_set_rate(sclk_fimc_lclk, 166750000);

	/* be able to handle clock on/off only with this clock */
	clk = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get interface clock\n");
		goto err_clk3;
	}

	clk_put(mout_mpll);
	clk_put(mout_fimc_lclk);

	clk_enable(clk);

	return 0;

err_clk3:
	clk_put(mout_fimc_lclk);

err_clk2:
	clk_put(mout_mpll);

err_clk1:
	return -EINVAL;
}

int s3c_fimc_clk_off(struct platform_device *pdev, struct clk *clk)
{
	clk_disable(clk);
	clk_put(clk);

	clk = NULL;

	return 0;
}
