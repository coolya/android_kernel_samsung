/* linux/drivers/media/video/samsung/csis.c
 *
 * Copyright (c) 2010 Samsung Electronics Co,. Ltd.
 *		http://www.samsung.com/
 *
 * MIPI-CSI2 Support file for FIMC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/slab.h>

#include <linux/io.h>
#include <linux/memory.h>
#include <plat/clock.h>
#include <plat/regs-csis.h>
#include <plat/csis.h>
#include "csis.h"

static struct s3c_csis_info *s3c_csis;

static struct s3c_platform_csis *to_csis_plat(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return (struct s3c_platform_csis *) pdev->dev.platform_data;
}

static int s3c_csis_set_info(void)
{
	s3c_csis = (struct s3c_csis_info *) \
			kmalloc(sizeof(struct s3c_csis_info), GFP_KERNEL);
	if (!s3c_csis) {
		err("no memory for configuration\n");
		return -ENOMEM;
	}

	strcpy(s3c_csis->name, S3C_CSIS_NAME);
	s3c_csis->nr_lanes = S3C_CSIS_NR_LANES;

	return 0;
}

static void s3c_csis_reset(void)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONTROL);
	cfg |= S3C_CSIS_CONTROL_RESET;
	writel(cfg, s3c_csis->regs + S3C_CSIS_CONTROL);
}

static void s3c_csis_set_nr_lanes(int lanes)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONFIG);
	cfg &= ~S3C_CSIS_CONFIG_NR_LANE_MASK;

	if (lanes == 1)
		cfg |= S3C_CSIS_CONFIG_NR_LANE_1;
	else
		cfg |= S3C_CSIS_CONFIG_NR_LANE_2;

	writel(cfg, s3c_csis->regs + S3C_CSIS_CONFIG);
}

static void s3c_csis_enable_interrupt(void)
{
	u32 cfg = 0;

	/* enable all interrupts */
	cfg |= S3C_CSIS_INTMSK_EVEN_BEFORE_ENABLE | \
		S3C_CSIS_INTMSK_EVEN_AFTER_ENABLE | \
		S3C_CSIS_INTMSK_ODD_BEFORE_ENABLE | \
		S3C_CSIS_INTMSK_ODD_AFTER_ENABLE | \
		S3C_CSIS_INTMSK_ERR_SOT_HS_ENABLE | \
		S3C_CSIS_INTMSK_ERR_ESC_ENABLE | \
		S3C_CSIS_INTMSK_ERR_CTRL_ENABLE | \
		S3C_CSIS_INTMSK_ERR_ECC_ENABLE | \
		S3C_CSIS_INTMSK_ERR_CRC_ENABLE | \
		S3C_CSIS_INTMSK_ERR_ID_ENABLE;

	writel(cfg, s3c_csis->regs + S3C_CSIS_INTMSK);
}

static void s3c_csis_disable_interrupt(void)
{
	/* disable all interrupts */
	writel(0, s3c_csis->regs + S3C_CSIS_INTMSK);
}

static void s3c_csis_system_on(void)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONTROL);
	cfg |= S3C_CSIS_CONTROL_ENABLE;
	writel(cfg, s3c_csis->regs + S3C_CSIS_CONTROL);
}

static void s3c_csis_system_off(void)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONTROL);
	cfg &= ~S3C_CSIS_CONTROL_ENABLE;
	writel(cfg, s3c_csis->regs + S3C_CSIS_CONTROL);
}

static void s3c_csis_phy_on(void)
{
	u32 cfg;
	cfg = readl(s3c_csis->regs + S3C_CSIS_DPHYCTRL);
	cfg |= S3C_CSIS_DPHYCTRL_ENABLE;
	writel(cfg, s3c_csis->regs + S3C_CSIS_DPHYCTRL);
}

static void s3c_csis_phy_off(void)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_DPHYCTRL);
	cfg &= ~S3C_CSIS_DPHYCTRL_ENABLE;
	writel(cfg, s3c_csis->regs + S3C_CSIS_DPHYCTRL);
}

#ifdef CONFIG_MIPI_CSI_ADV_FEATURE
static void s3c_csis_update_shadow(void)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONTROL);
	cfg |= S3C_CSIS_CONTROL_UPDATE_SHADOW;
	writel(cfg, s3c_csis->regs + S3C_CSIS_CONTROL);
}

static void s3c_csis_set_data_align(int align)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONTROL);
	cfg &= ~S3C_CSIS_CONTROL_ALIGN_MASK;

	if (align == 24)
		cfg |= S3C_CSIS_CONTROL_ALIGN_24BIT;
	else
		cfg |= S3C_CSIS_CONTROL_ALIGN_32BIT;

	writel(cfg, s3c_csis->regs + S3C_CSIS_CONTROL);
}

static void s3c_csis_set_wclk(int extclk)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONTROL);
	cfg &= ~S3C_CSIS_CONTROL_WCLK_MASK;

	if (extclk)
		cfg |= S3C_CSIS_CONTROL_WCLK_EXTCLK;
	else
		cfg |= S3C_CSIS_CONTROL_WCLK_PCLK;

	writel(cfg, s3c_csis->regs + S3C_CSIS_CONTROL);
}

static void s3c_csis_set_format(enum mipi_format fmt)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_CONFIG);
	cfg &= ~S3C_CSIS_CONFIG_FORMAT_MASK;
	cfg |= (fmt << S3C_CSIS_CONFIG_FORMAT_SHIFT);

	writel(cfg, s3c_csis->regs + S3C_CSIS_CONFIG);
}

static void s3c_csis_set_resol(int width, int height)
{
	u32 cfg = 0;

	cfg |= width << S3C_CSIS_RESOL_HOR_SHIFT;
	cfg |= height << S3C_CSIS_RESOL_VER_SHIFT;

	writel(cfg, s3c_csis->regs + S3C_CSIS_RESOL);
}

static void s3c_csis_set_hs_settle(int settle)
{
	u32 cfg;

	cfg = readl(s3c_csis->regs + S3C_CSIS_DPHYCTRL);
	cfg &= ~S3C_CSIS_DPHYCTRL_HS_SETTLE_MASK;
	cfg |= (settle << S3C_CSIS_DPHYCTRL_HS_SETTLE_SHIFT);

	writel(cfg, s3c_csis->regs + S3C_CSIS_DPHYCTRL);
}
#endif

void s3c_csis_start(int lanes, int settle, int align, int width,
		int height, int pixel_format)
{
	struct s3c_platform_csis *pdata;

	pdata = to_csis_plat(s3c_csis->dev);
	if (pdata->cfg_phy_global)
		pdata->cfg_phy_global(1);

	s3c_csis_reset();
	s3c_csis_set_nr_lanes(lanes);

#ifdef CONFIG_MIPI_CSI_ADV_FEATURE
	/* FIXME: how configure the followings with FIMC dynamically? */
	s3c_csis_set_hs_settle(settle);	/* s5k6aa */
	s3c_csis_set_data_align(align);
	s3c_csis_set_wclk(0);
	if (pixel_format == V4L2_PIX_FMT_JPEG)
		s3c_csis_set_format(MIPI_USER_DEF_PACKET_1);
	else
		s3c_csis_set_format(MIPI_CSI_YCBCR422_8BIT);
	s3c_csis_set_resol(width, height);
	s3c_csis_update_shadow();
#endif

	s3c_csis_enable_interrupt();
	s3c_csis_system_on();
	s3c_csis_phy_on();

	info("Samsung MIPI-CSI2 operation started\n");
}

static void s3c_csis_stop(struct platform_device *pdev)
{
	struct s3c_platform_csis *plat;

	s3c_csis_disable_interrupt();
	s3c_csis_system_off();
	s3c_csis_phy_off();

	plat = to_csis_plat(&pdev->dev);
	if (plat->cfg_phy_global)
		plat->cfg_phy_global(0);
}

static irqreturn_t s3c_csis_irq(int irq, void *dev_id)
{
	u32 cfg;

	/* just clearing the pends */
	cfg = readl(s3c_csis->regs + S3C_CSIS_INTSRC);
	writel(cfg, s3c_csis->regs + S3C_CSIS_INTSRC);

	return IRQ_HANDLED;
}

static int s3c_csis_clk_on(struct platform_device *pdev)
{
	struct s3c_platform_csis *pdata;
	struct clk *parent, *mout_csis;

	pdata = to_csis_plat(&pdev->dev);

	/* mout_mpll */
	parent = clk_get(&pdev->dev, pdata->srclk_name);
	if (IS_ERR(parent)) {
		err("failed to get parent clock for csis\n");
		return -EINVAL;
	}

	/* mout_csis */
	mout_csis = clk_get(&pdev->dev, "mout_csis");

	/* sclk_csis */
	s3c_csis->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(s3c_csis->clock)) {
		err("failed to get csis clock source\n");
		return -EINVAL;
	}

	clk_set_parent(mout_csis, parent);
	clk_set_parent(s3c_csis->clock, mout_csis);

	/* Turn on csis power domain regulator */
	regulator_enable(s3c_csis->regulator);
	/* clock enable for csis */
	clk_enable(s3c_csis->clock);

	return 0;
}

static int s3c_csis_clk_off(struct platform_device *pdev)
{
	struct s3c_platform_csis *plat;

	plat = to_csis_plat(&pdev->dev);

	/* sclk_csis */
	s3c_csis->clock = clk_get(&pdev->dev, plat->clk_name);
	if (IS_ERR(s3c_csis->clock)) {
		err("failed to get csis clock source\n");
		return -EINVAL;
	}

	/* clock disable for csis */
	clk_disable(s3c_csis->clock);
	/* Turn off csis power domain regulator */
	regulator_disable(s3c_csis->regulator);

	return 0;
}

static int s3c_csis_probe(struct platform_device *pdev)
{
	struct s3c_platform_csis *pdata;
	struct resource *res;

	s3c_csis_set_info();

	s3c_csis->dev = &pdev->dev;

	pdata = to_csis_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio();

	/* Get csis power domain regulator */
	s3c_csis->regulator = regulator_get(&pdev->dev, "pd");
	if (IS_ERR(s3c_csis->regulator)) {
		err("%s: failed to get resource %s\n",
				__func__, "s3c-csis");
		return PTR_ERR(s3c_csis->regulator);
	}
	/* clock & power on */
	s3c_csis_clk_on(pdev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err("failed to get io memory region\n");
		return -EINVAL;
	}

	res = request_mem_region(res->start,
			res->end - res->start + 1, pdev->name);
	if (!res) {
		err("failed to request io memory region\n");
		return -EINVAL;
	}

	/* ioremap for register block */
	s3c_csis->regs = ioremap(res->start, res->end - res->start + 1);
	if (!s3c_csis->regs) {
		err("failed to remap io region\n");
		return -EINVAL;
	}

	/* irq */
	s3c_csis->irq = platform_get_irq(pdev, 0);
	if (request_irq(s3c_csis->irq, s3c_csis_irq, IRQF_DISABLED, \
		s3c_csis->name, s3c_csis))
		err("request_irq failed\n");

	info("Samsung MIPI-CSI2 driver probed successfully\n");

	return 0;
}

static int s3c_csis_remove(struct platform_device *pdev)
{
	s3c_csis_stop(pdev);
	kfree(s3c_csis);

	return 0;
}

/* sleep */
int s3c_csis_suspend(struct platform_device *pdev, pm_message_t state)
{
	s3c_csis_clk_off(pdev);
	return 0;
}

/* wakeup */
int s3c_csis_resume(struct platform_device *pdev)
{
	s3c_csis_clk_on(pdev);
	return 0;
}

static struct platform_driver s3c_csis_driver = {
	.probe		= s3c_csis_probe,
	.remove		= s3c_csis_remove,
	.suspend	= s3c_csis_suspend,
	.resume		= s3c_csis_resume,
	.driver		= {
		.name	= "s3c-csis",
		.owner	= THIS_MODULE,
	},
};

static int s3c_csis_register(void)
{
	platform_driver_register(&s3c_csis_driver);

	return 0;
}

static void s3c_csis_unregister(void)
{
	platform_driver_unregister(&s3c_csis_driver);
}

module_init(s3c_csis_register);
module_exit(s3c_csis_unregister);

MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_AUTHOR("Sewoon, Park <seuni.park@samsung.com>");
MODULE_DESCRIPTION("MIPI-CSI2 support for FIMC driver");
MODULE_LICENSE("GPL");
