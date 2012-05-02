/* linux/drivers/media/video/samsung/tv20/s5pc100/tv_clock_s5pc100.c
 *
 * clock raw ftn  file for Samsung TVOut driver
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
#include <linux/clk.h>

#include <linux/uaccess.h>
#include <linux/io.h>

#include <plat/map.h>
#include <plat/regs-clock.h>

#include "tv_out_s5pc100.h"
#include "regs/regs-clock_extra.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_TVOUT_CLK_DEBUG 1
#endif

#ifdef S5P_TVOUT_CLK_DEBUG
#define TVCLKPRINTK(fmt, args...) \
	printk(KERN_INFO "\t\t[TVCLK] %s: " fmt, __func__ , ## args)
#else
#define TVCLKPRINTK(fmt, args...)
#endif

static struct resource	*tvclk_mem;
void __iomem		*tvclk_base;

/*
* initialization
*  - iniization functions are only called under stopping tvout clock
*/

void __s5p_tv_clk_init_hpll(unsigned int lock_time,
			    unsigned int mdiv,
			    unsigned int pdiv,
			    unsigned int sdiv)
{
	TVCLKPRINTK("%d,%d,%d,%d\n\r", lock_time, mdiv, pdiv, sdiv);

	writel(HPLL_LOCKTIME(lock_time), S5P_HPLL_LOCK);
	writel(MDIV(mdiv) | PDIV(pdiv) | SDIV(sdiv), S5P_HPLL_CON);

	TVCLKPRINTK("0x%08x,0x%08x\n\r", readl(S5P_HPLL_LOCK),
		readl(S5P_HPLL_CON));
}

void __s5p_tv_clk_hpll_onoff(bool en)
{
	TVCLKPRINTK("%d\n\r", en);

	if (en) {
		writel(readl(S5P_HPLL_CON) | HPLL_ENABLE, S5P_HPLL_CON) ;

		while (!HPLL_LOCKED(readl(S5P_HPLL_CON)))
			msleep(1);

	} else
		writel(readl(S5P_HPLL_CON) & ~HPLL_ENABLE, S5P_HPLL_CON);


	TVCLKPRINTK("0x%08x,0x%08x\n\r", readl(S5P_HPLL_LOCK),
		readl(S5P_HPLL_CON));
}

enum s5p_tv_clk_err __s5p_tv_clk_init_href(enum s5p_tv_clk_hpll_ref hpll_ref)
{
	TVCLKPRINTK("(%d)\n\r", hpll_ref);

	switch (hpll_ref) {

	case S5P_TV_CLK_HPLL_REF_27M:
		writel(readl(S5P_CLK_SRC0) & HREF_SEL_MASK, S5P_CLK_SRC0);
		break;

	case S5P_TV_CLK_HPLL_REF_SRCLK:
		writel(readl(S5P_CLK_SRC0) | HREF_SEL_SRCLK, S5P_CLK_SRC0);
		break;

	default:
		TVCLKPRINTK("invalid hpll_ref parameter = %d\n\r", hpll_ref);
		return S5P_TV_CLK_ERR_INVALID_PARAM;
		break;
	}

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_SRC0));

	return S5P_TV_CLK_ERR_NO_ERROR;
}

enum s5p_tv_clk_err __s5p_tv_clk_init_mout_hpll(
	enum s5p_tv_clk_mout_hpll mout_hpll)
{
	TVCLKPRINTK("(%d)\n\r", mout_hpll);

	switch (mout_hpll) {

	case S5P_TV_CLK_MOUT_HPLL_27M:
		writel(readl(S5P_CLK_SRC0) & HPLL_SEL_MASK, S5P_CLK_SRC0);
		break;

	case S5P_TV_CLK_MOUT_HPLL_FOUT_HPLL:
		writel(readl(S5P_CLK_SRC0) | HPLL_SEL_FOUT_HPLL, S5P_CLK_SRC0);
		break;

	default:
		TVCLKPRINTK(" invalid mout_hpll parameter = %d\n\r",
			mout_hpll);
		return S5P_TV_CLK_ERR_INVALID_PARAM;
		break;
	}

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_SRC0));

	return S5P_TV_CLK_ERR_NO_ERROR;
}

enum s5p_tv_clk_err __s5p_tv_clk_init_video_mixer(
	enum s5p_tv_clk_vmiexr_srcclk src_clk)
{
	TVCLKPRINTK("(%d)\n\r", src_clk);

	switch (src_clk) {

	case TVOUT_CLK_VMIXER_SRCCLK_CLK27M:
		writel(((readl(S5P_CLK_SRC2) & VMIXER_SEL_MASK) |
			VMIXER_SEL_CLK27M), S5P_CLK_SRC2);
		break;

	case TVOUT_CLK_VMIXER_SRCCLK_VCLK_54:
		writel(((readl(S5P_CLK_SRC2) & VMIXER_SEL_MASK) |
			VMIXER_SEL_VCLK_54), S5P_CLK_SRC2);
		break;

	case TVOUT_CLK_VMIXER_SRCCLK_MOUT_HPLL:
		writel(((readl(S5P_CLK_SRC2) & VMIXER_SEL_MASK) |
			VMIXER_SEL_MOUT_HPLL), S5P_CLK_SRC2);
		break;

	default:
		TVCLKPRINTK("invalid src_clk parameter = %d\n\r", src_clk);
		return S5P_TV_CLK_ERR_INVALID_PARAM;
		break;
	}

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_SRC2));

	return S5P_TV_CLK_ERR_NO_ERROR;
}

void __s5p_tv_clk_init_hdmi_ratio(unsigned int clk_div)
{
	TVCLKPRINTK("(%d)\n\r", clk_div);

	writel((readl(S5P_CLK_DIV3) & HDMI_DIV_RATIO_MASK) |
		HDMI_DIV_RATIO(clk_div), S5P_CLK_DIV3);

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_DIV3));
}

/*
* set
*  - set functions are only called under running tvout clock
*/
void __s5p_tv_clk_set_vp_clk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on)
		writel(readl(S5P_CLKGATE_D12) | CLK_HCLK_VP_PASS,
			S5P_CLKGATE_D12);
	else
		writel(readl(S5P_CLKGATE_D12) & ~CLK_HCLK_VP_PASS,
			S5P_CLKGATE_D12);


	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLKGATE_D12));
}

void __s5p_tv_clk_set_vmixer_hclk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on)
		writel(readl(S5P_CLKGATE_D12) | CLK_HCLK_VMIXER_PASS,
			S5P_CLKGATE_D12);
	else
		writel(readl(S5P_CLKGATE_D12) & ~CLK_HCLK_VMIXER_PASS,
			S5P_CLKGATE_D12);


	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLKGATE_D12));
}

void __s5p_tv_clk_set_vmixer_sclk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on)
		writel(readl(S5P_SCLKGATE1) | CLK_SCLK_VMIXER_PASS,
			S5P_SCLKGATE1);
	else
		writel(readl(S5P_SCLKGATE1) & ~CLK_SCLK_VMIXER_PASS,
			S5P_SCLKGATE1);


	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_SCLKGATE1));
}

void __s5p_tv_clk_set_sdout_hclk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on) {
		writel((readl(S5P_CLKGATE_D12) | CLK_HCLK_SDOUT_PASS),
			S5P_CLKGATE_D12);
		writel(readl(tvclk_base + 0x304) | VMIXER_OUT_SEL_SDOUT,
			tvclk_base + 0x304);
	} else
		writel((readl(S5P_CLKGATE_D12) & ~CLK_HCLK_SDOUT_PASS),
			S5P_CLKGATE_D12);



	TVCLKPRINTK("physical %p (0x%08x)\n\r", tvclk_base,
		readl(tvclk_base + 0x304));

	TVCLKPRINTK("after (0x%08x)\n\r", readl(S5P_CLKGATE_D12));
}

void __s5p_tv_clk_set_sdout_sclk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on)
		writel((readl(S5P_SCLKGATE1) | CLK_SCLK_TV54_PASS |
			CLK_SCLK_VDAC54_PASS),
		       S5P_SCLKGATE1);
	else
		writel((readl(S5P_SCLKGATE1) & (~CLK_SCLK_TV54_PASS &
			~CLK_SCLK_VDAC54_PASS)),
		       S5P_SCLKGATE1);


	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_SCLKGATE1));
}

void __s5p_tv_clk_set_hdmi_hclk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on) {
		writel((readl(S5P_CLKGATE_D12) | CLK_HCLK_HDMI_PASS),
			S5P_CLKGATE_D12);
		writel(readl(tvclk_base + 0x304) | VMIXER_OUT_SEL_HDMI,
			tvclk_base + 0x304);
	} else
		writel((readl(S5P_CLKGATE_D12) & ~CLK_HCLK_HDMI_PASS),
			S5P_CLKGATE_D12) ;


	TVCLKPRINTK("physical %p (0x%08x)\n\r", tvclk_base,
		readl(tvclk_base + 0x304));

	TVCLKPRINTK("after (0x%08x)\n\r", readl(S5P_CLKGATE_D12));
}

void __s5p_tv_clk_set_hdmi_sclk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on)
		writel((readl(S5P_SCLKGATE1) | CLK_SCLK_HDMI_PASS),
			S5P_SCLKGATE1);
	else
		writel((readl(S5P_SCLKGATE1) & ~CLK_SCLK_HDMI_PASS),
			S5P_SCLKGATE1);


	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_SCLKGATE1));
}

void __s5p_tv_clk_set_hdmi_i2c_clk_onoff(bool clk_on)
{
	TVCLKPRINTK("(%d)\n\r", clk_on);

	if (clk_on)
		writel((readl(S5P_CLKGATE_D14) | CLK_PCLK_IIC_HDMI_PASS),
			S5P_CLKGATE_D14);
	else
		writel((readl(S5P_CLKGATE_D14) & ~CLK_PCLK_IIC_HDMI_PASS),
			S5P_CLKGATE_D14);


	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLKGATE_D14));
}

/*
* start
*  - start functions are only called under stopping tvout clock
*/
void __s5p_tv_clk_start(bool vp_hclk_on,
			bool sdout_hclk_on,
			bool hdmi_hclk_on)
{
	TVCLKPRINTK("(%d,%d,%d)\n\r", vp_hclk_on, sdout_hclk_on, hdmi_hclk_on);

	__s5p_tv_clk_set_vp_clk_onoff(vp_hclk_on);
	__s5p_tv_clk_set_sdout_hclk_onoff(sdout_hclk_on);
	__s5p_tv_clk_set_sdout_sclk_onoff(sdout_hclk_on);
	__s5p_tv_clk_set_hdmi_hclk_onoff(hdmi_hclk_on);
	__s5p_tv_clk_set_vmixer_hclk_onoff(true);
	__s5p_tv_clk_set_vmixer_sclk_onoff(true);

	if (hdmi_hclk_on)
		__s5p_tv_clk_hpll_onoff(true);

}


/*
* stop
* - stop functions are only called under running tvout clock
*/
void __s5p_tv_clk_stop(void)
{
	__s5p_tv_clk_set_sdout_sclk_onoff(false);
	__s5p_tv_clk_set_sdout_hclk_onoff(false);
	__s5p_tv_clk_set_hdmi_sclk_onoff(false);
	__s5p_tv_clk_set_hdmi_hclk_onoff(false);
	__s5p_tv_clk_set_vp_clk_onoff(false);
	__s5p_tv_clk_set_vmixer_sclk_onoff(false);
	__s5p_tv_clk_set_vmixer_hclk_onoff(false);
	__s5p_tv_clk_hpll_onoff(false);
}

int __init __s5p_tvclk_probe(struct platform_device *pdev, u32 res_num)
{

	struct resource *res;
	size_t	size;
	int 	ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, res_num);

	if (res == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region resource\n");
		ret = -ENOENT;

	}

	size = (res->end - res->start) + 1;

	tvclk_mem = request_mem_region(res->start, size, pdev->name);

	if (tvclk_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		ret = -ENOENT;

	}

	tvclk_base = ioremap(res->start, size);

	if (tvclk_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		ret = -ENOENT;

	}

	return ret;
}

int __init __s5p_tvclk_release(struct platform_device *pdev)
{
	iounmap(tvclk_base);

	/* remove memory region */

	if (tvclk_mem != NULL) {
		if (release_resource(tvclk_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(tvclk_mem);

		tvclk_mem = NULL;
	}

	return 0;
}
