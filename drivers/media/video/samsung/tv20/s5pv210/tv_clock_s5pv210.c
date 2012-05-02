/* linux/drivers/media/video/samsung/tv20/s5pv210/tv_clock_s5pc110.c
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsung.com/
 *
 * clock raw ftn  file for Samsung TVOut driver
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

#include <mach/map.h>
#include <mach/regs-clock.h>

#include "tv_out_s5pv210.h"
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

void __s5p_tv_clk_init_hpll(unsigned int lock_time,
			bool vsel,
			unsigned int mdiv,
			unsigned int pdiv,
			unsigned int sdiv)
{
	u32 temp;

	TVCLKPRINTK("%d,%d,%d,%d\n\r", lock_time, mdiv, pdiv, sdiv);

	temp = readl(S5P_VPLL_CON);

	temp &= ~VPLL_ENABLE;

	writel(temp, S5P_VPLL_CON);

	temp = 0;

	if (vsel)
		temp |= VCO_FREQ_SEL;

	temp |= VPLL_ENABLE;
	temp |= MDIV(mdiv) | PDIV(pdiv) | SDIV(sdiv);

	writel(VPLL_LOCKTIME(lock_time), S5P_VPLL_LOCK);
	writel(temp, S5P_VPLL_CON);

	while (!VPLL_LOCKED(readl(S5P_VPLL_CON)));

	TVCLKPRINTK("0x%08x,0x%08x\n\r", readl(S5P_VPLL_LOCK), \
			readl(S5P_VPLL_CON));
}

void __s5p_tv_clk_hpll_onoff(bool en)
{
}

s5p_tv_clk_err __s5p_tv_clk_init_href(s5p_tv_clk_hpll_ref hpll_ref)
{
    return S5P_TV_CLK_ERR_NO_ERROR;
}

/* prevent hdmi hang-up when reboot */
int __s5p_tv_clk_change_internal(void)
{
	u32 reg = readl(S5P_CLK_SRC1);
	/* set to SCLK_DAC */
	reg &= HDMI_SEL_MASK;
	/* set to SCLK_PIXEL */
	reg &= VMIXER_SEL_MASK;

	writel(reg, S5P_CLK_SRC1);

	return 0;
}

s5p_tv_clk_err __s5p_tv_clk_init_mout_hpll(s5p_tv_clk_mout_hpll mout_hpll)
{
	TVCLKPRINTK("(%d)\n\r", mout_hpll);

	writel(readl(S5P_CLK_SRC1) | HDMI_SEL_HDMIPHY, S5P_CLK_SRC1);

	TVCLKPRINTK("S5P_CLK_SRC1 :0x%08x\n", readl(S5P_CLK_SRC1));
	return S5P_TV_CLK_ERR_NO_ERROR;
}

s5p_tv_clk_err __s5p_tv_clk_init_video_mixer(s5p_tv_clk_vmiexr_srcclk src_clk)
{
	switch (src_clk) {

	/* for analog tv out 0:SCLK_DAC */
	case TVOUT_CLK_VMIXER_SRCCLK_VCLK_54:
		writel(readl(S5P_CLK_SRC1) & VMIXER_SEL_MASK, S5P_CLK_SRC1);
		break;

	/* for digital hdmi_phy 1: SCLK_HDMI */
	case TVOUT_CLK_VMIXER_SRCCLK_MOUT_HPLL:
		writel(readl(S5P_CLK_SRC1) | VMIXER_SEL_MOUT_VPLL, \
				S5P_CLK_SRC1);
		break;

	default:
		TVCLKPRINTK("[ERR] invalid src_clk parameter = %d\n", src_clk);
		return S5P_TV_CLK_ERR_INVALID_PARAM;
		break;
	}

	TVCLKPRINTK("S5P_CLK_SRC1 :0x%08x\n", readl(S5P_CLK_SRC1));

	return S5P_TV_CLK_ERR_NO_ERROR;
}

void __s5p_tv_clk_init_hdmi_ratio(unsigned int clk_div)
{
	TVCLKPRINTK("(%d)\n\r", clk_div);

	writel((readl(S5P_CLK_DIV1) & HDMI_DIV_RATIO_MASK) | \
			HDMI_DIV_RATIO(clk_div), S5P_CLK_DIV1);

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_DIV3));
}

/*
 * hclk gating
 */
 
/* VP */
void __s5p_tv_clk_set_vp_clk_onoff(bool clk_on)
{
    /*
       TVCLKPRINTK("VP hclk : %s\n\r", clk_on ? "on":"off");

       if (clk_on)
 		bit_add_l(S5P_CLKGATE_IP1_VP, S5P_CLKGATE_IP1);
       else
		bit_del_l(S5P_CLKGATE_IP1_VP, S5P_CLKGATE_IP1);

       TVCLKPRINTK("S5P_CLKGATE_MAIN1 :0x%08x\n\r", readl(S5P_CLKGATE_MAIN1));
     */
}

/* MIXER */
void __s5p_tv_clk_set_vmixer_hclk_onoff(bool clk_on)
{
    /*
       TVCLKPRINTK("MIXER hclk : %s\n\r", clk_on ? "on":"off");

       if (clk_on)
		bit_add_l(S5P_CLKGATE_IP1_MIXER, S5P_CLKGATE_IP1);
       else
		bit_del_l(S5P_CLKGATE_IP1_MIXER, S5P_CLKGATE_IP1);

       TVCLKPRINTK("S5P_CLKGATE_MAIN1 :0x%08x\n\r", readl(S5P_CLKGATE_MAIN1));
     */
}

/* TVENC */
void __s5p_tv_clk_set_sdout_hclk_onoff(bool clk_on)
{
    /*
       TVCLKPRINTK("TVENC hclk : %s\n\r", clk_on ? "on":"off");

    if (clk_on)
        bit_add_l(S5P_CLKGATE_IP1_TVENC, S5P_CLKGATE_IP1);
    else
        bit_del_l(S5P_CLKGATE_IP1_TVENC, S5P_CLKGATE_IP1);
     */
}

/* HDMI */
void __s5p_tv_clk_set_hdmi_hclk_onoff(bool clk_on)
{
    /*
       TVCLKPRINTK("HDMI hclk : %s\n\r", clk_on ? "on":"off");

       if (clk_on) {
		bit_add_l(S5P_CLKGATE_IP1_HDMI, S5P_CLKGATE_IP1);
		bit_add_l(VMIXER_OUT_SEL_HDMI, S5P_MIXER_OUT_SEL);
       } else
		bit_del_l(S5P_CLKGATE_IP1_HDMI, S5P_CLKGATE_IP1);

       TVCLKPRINTK("S5P_CLKGATE_PERI1 :0x%08x\n\r", readl(S5P_CLKGATE_PERI1));
       TVCLKPRINTK("clk output is %s\n\r", readl(S5P_MIXER_OUT_SEL) ? "HDMI":"SDOUT");
     */
}

/* 
 * sclk gating 
 */
 
/* MIXER */
void __s5p_tv_clk_set_vmixer_sclk_onoff(bool clk_on)
{
#if 0
	TVCLKPRINTK("MIXER sclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on)
		bit_add_l(CLK_SCLK_VMIXER_PASS, S5P_SCLKGATE0);
	else
		bit_del_l(CLK_SCLK_VMIXER_PASS, S5P_SCLKGATE0);

	TVCLKPRINTK("S5P_SCLKGATE0 :0x%08x\n\r", readl(S5P_SCLKGATE0));
#endif
}

/* TVENC */
void __s5p_tv_clk_set_sdout_sclk_onoff(bool clk_on)
{
#if 0
	TVCLKPRINTK("TVENC sclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on)
		bit_add_l(CLK_SCLK_TV54_PASS | CLK_SCLK_VDAC54_PASS, S5P_SCLKGATE0);
	else
		bit_del_l(CLK_SCLK_TV54_PASS | CLK_SCLK_VDAC54_PASS, S5P_SCLKGATE0);

	TVCLKPRINTK("S5P_SCLKGATE0 :0x%08x\n\r", readl(S5P_SCLKGATE0));
#endif
}

/* HDMI */
void __s5p_tv_clk_set_hdmi_sclk_onoff(bool clk_on)
{
#if 0
	TVCLKPRINTK("HDMI sclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on)
		bit_add_l(CLK_SCLK_HDMI_PASS, S5P_SCLKGATE0);
	else
		bit_del_l(CLK_SCLK_HDMI_PASS, S5P_SCLKGATE0);

	TVCLKPRINTK("S5P_SCLKGATE0 :0x%08x\n\r", readl(S5P_SCLKGATE0));
#endif
}

void __s5p_tv_clk_start(bool vp, bool sdout, bool hdmi)
{
	__s5p_tv_clk_set_vp_clk_onoff(vp);
	__s5p_tv_clk_set_sdout_hclk_onoff(sdout);
	__s5p_tv_clk_set_sdout_sclk_onoff(sdout);
	__s5p_tv_clk_set_hdmi_hclk_onoff(hdmi);
	__s5p_tv_clk_set_vmixer_hclk_onoff(true);
	__s5p_tv_clk_set_vmixer_sclk_onoff(true);

	if (hdmi)
		__s5p_tv_clk_hpll_onoff(true);
}

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
	return 0;
}

int __init __s5p_tvclk_release(struct platform_device *pdev)
{
	return 0;
}
