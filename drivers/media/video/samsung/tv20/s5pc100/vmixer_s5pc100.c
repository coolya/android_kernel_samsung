/* linux/drivers/media/video/samsung/tv20/s5pc100/vmixer_s5pc100.c
 *
 * Mixer raw ftn  file for Samsung TVOut driver
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
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <linux/io.h>

#include "tv_out_s5pc100.h"

#include "regs/regs-vmx.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_MXR_DEBUG 1
#endif

#ifdef S5P_MXR_DEBUG
#define VMPRINTK(fmt, args...)	\
	printk(KERN_INFO "\t\t[VM] %s: " fmt, __func__ , ## args)
#else
#define VMPRINTK(fmt, args...)
#endif

static struct resource	*mixer_mem;
void __iomem		*mixer_base;

/*
*set  - set functions are only called under running vmixer
*/

enum s5p_tv_vmx_err __s5p_vm_set_layer_show(enum s5p_tv_vmx_layer layer,
	bool show)
{
	u32 mxr_config;

	VMPRINTK("%d, %d\n\r", layer, show);

	switch (layer) {

	case VM_VIDEO_LAYER:
		mxr_config = (show) ?
				(readl(mixer_base + S5P_MXR_CFG) |
				S5P_MXR_VIDEO_LAYER_SHOW) :
				(readl(mixer_base + S5P_MXR_CFG) &
				~S5P_MXR_VIDEO_LAYER_SHOW);
		break;

	case VM_GPR0_LAYER:
		mxr_config = (show) ?
			     (readl(mixer_base + S5P_MXR_CFG) |
			     S5P_MXR_GRAPHIC0_LAYER_SHOW) :
			     (readl(mixer_base + S5P_MXR_CFG) &
			     ~S5P_MXR_GRAPHIC0_LAYER_SHOW);
		break;

	case VM_GPR1_LAYER:
		mxr_config = (show) ?
			     (readl(mixer_base + S5P_MXR_CFG) |
			     S5P_MXR_GRAPHIC1_LAYER_SHOW) :
			     (readl(mixer_base + S5P_MXR_CFG) &
			     ~S5P_MXR_GRAPHIC1_LAYER_SHOW);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	writel(mxr_config, mixer_base + S5P_MXR_CFG);

	VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_CFG));

	return VMIXER_NO_ERROR;
}

enum s5p_tv_vmx_err __s5p_vm_set_layer_priority(enum s5p_tv_vmx_layer layer,
	u32 priority)
{
	u32 layer_cfg;

	VMPRINTK("%d, %d\n\r", layer, priority);

	switch (layer) {

	case VM_VIDEO_LAYER:
		layer_cfg = S5P_MXR_VP_LAYER_PRIORITY_CLEAR(
			readl(mixer_base + S5P_MXR_LAYER_CFG)) |
			S5P_MXR_VP_LAYER_PRIORITY(priority);
		break;

	case VM_GPR0_LAYER:
		layer_cfg = S5P_MXR_GRP0_LAYER_PRIORITY_CLEAR(
			readl(mixer_base + S5P_MXR_LAYER_CFG)) |
			S5P_MXR_GRP0_LAYER_PRIORITY(priority);
		break;

	case VM_GPR1_LAYER:
		layer_cfg = S5P_MXR_GRP1_LAYER_PRIORITY_CLEAR(
			readl(mixer_base + S5P_MXR_LAYER_CFG)) |
			S5P_MXR_GRP1_LAYER_PRIORITY(priority);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	writel(layer_cfg, mixer_base + S5P_MXR_LAYER_CFG);

	return VMIXER_NO_ERROR;
}

enum s5p_tv_vmx_err __s5p_vm_set_win_blend(
	enum s5p_tv_vmx_layer layer, bool enable)
{
	u32 temp_reg;
	VMPRINTK("%d, %d\n\r", layer, enable);

	switch (layer) {

	case VM_VIDEO_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_VIDEO_CFG)
			   & (~S5P_MXR_VP_BLEND_ENABLE) ;

		if (enable)
			temp_reg |= S5P_MXR_VP_BLEND_ENABLE;
		else
			temp_reg |= S5P_MXR_VP_BLEND_DISABLE;

		writel(temp_reg, mixer_base + S5P_MXR_VIDEO_CFG);

		break;

	case VM_GPR0_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG)
			   & (~S5P_MXR_WIN_BLEND_ENABLE) ;

		if (enable)
			temp_reg |= S5P_MXR_WIN_BLEND_ENABLE;
		else
			temp_reg |= S5P_MXR_WIN_BLEND_DISABLE;

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);

		break;

	case VM_GPR1_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG)
			   & (~S5P_MXR_WIN_BLEND_ENABLE) ;

		if (enable)
			temp_reg |= S5P_MXR_WIN_BLEND_ENABLE;
		else
			temp_reg |= S5P_MXR_WIN_BLEND_DISABLE;

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);

		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);

		return S5P_TV_VMX_ERR_INVALID_PARAM;

		break;
	}

	VMPRINTK("0x08%x\n\r", readl(mixer_base + S5P_MXR_VIDEO_CFG));

	VMPRINTK("0x08%x\n\r", readl(mixer_base + S5P_MXR_GRAPHIC0_CFG));
	VMPRINTK("0x08%x\n\r", readl(mixer_base + S5P_MXR_GRAPHIC1_CFG));

	return VMIXER_NO_ERROR;
}


enum s5p_tv_vmx_err __s5p_vm_set_layer_alpha(enum s5p_tv_vmx_layer layer,
	u32 alpha)
{
	u32 temp_reg;
	VMPRINTK("%d, %d\n\r", layer, alpha);

	switch (layer) {

	case VM_VIDEO_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_VIDEO_CFG)
			   & (~S5P_MXR_ALPHA) ;
		temp_reg |= S5P_MXR_VP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_VIDEO_CFG);
		break;

	case VM_GPR0_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG)
			   & (~S5P_MXR_ALPHA) ;
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		break;

	case VM_GPR1_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG)
			   & (~S5P_MXR_ALPHA) ;
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	VMPRINTK("0x08%x\n\r", readl(mixer_base + S5P_MXR_VIDEO_CFG));

	VMPRINTK("0x08%x\n\r", readl(mixer_base + S5P_MXR_GRAPHIC0_CFG));
	VMPRINTK("0x08%x\n\r", readl(mixer_base + S5P_MXR_GRAPHIC1_CFG));

	return VMIXER_NO_ERROR;
}


enum s5p_tv_vmx_err __s5p_vm_set_grp_base_address(enum s5p_tv_vmx_layer layer,
	u32 base_addr)
{
	VMPRINTK("%d, 0x%x\n\r", layer, base_addr);

	if (S5P_MXR_GRP_ADDR_ILLEGAL(base_addr)) {
		VMPRINTK(" address is not word align = %d\n\r", base_addr);
		return S5P_TV_VMX_ERR_BASE_ADDRESS_MUST_WORD_ALIGN;
	}

	switch (layer) {

	case VM_GPR0_LAYER:
		writel(S5P_MXR_GPR_BASE(base_addr),
			mixer_base + S5P_MXR_GRAPHIC0_BASE);
		VMPRINTK("0x%x\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC0_BASE));
		break;

	case VM_GPR1_LAYER:
		writel(S5P_MXR_GPR_BASE(base_addr),
			mixer_base + S5P_MXR_GRAPHIC1_BASE);
		VMPRINTK("0x%x\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC1_BASE));
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	return VMIXER_NO_ERROR;
}

enum s5p_tv_vmx_err __s5p_vm_set_grp_layer_position(enum s5p_tv_vmx_layer
	layer, u32 dst_offs_x, u32 dst_offs_y)
{
	VMPRINTK("%d, %d, %d)\n\r", layer, dst_offs_x, dst_offs_y);

	switch (layer) {

	case VM_GPR0_LAYER:
		writel(S5P_MXR_GRP_DESTX(dst_offs_x) |
			S5P_MXR_GRP_DESTY(dst_offs_y),
			mixer_base + S5P_MXR_GRAPHIC0_DXY);
		VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_GRAPHIC0_DXY));
		break;

	case VM_GPR1_LAYER:
		writel(S5P_MXR_GRP_DESTX(dst_offs_x) |
			S5P_MXR_GRP_DESTY(dst_offs_y),
			mixer_base + S5P_MXR_GRAPHIC1_DXY);
		VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_GRAPHIC1_DXY));
		break;

	default:
		VMPRINTK("invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	return VMIXER_NO_ERROR;
}

enum s5p_tv_vmx_err __s5p_vm_set_grp_layer_size(enum s5p_tv_vmx_layer layer,
					   u32 span,
					   u32 width,
					   u32 height,
					   u32 src_offs_x,
					   u32 src_offs_y)
{
	VMPRINTK("%d, %d, %d, %d, %d, %d)\n\r", layer, span, width, height,
		 src_offs_x, src_offs_y);

	switch (layer) {

	case VM_GPR0_LAYER:
		writel(S5P_MXR_GRP_SPAN(span),
			mixer_base + S5P_MXR_GRAPHIC0_SPAN);
		writel(S5P_MXR_GRP_WIDTH(width) |
			S5P_MXR_GRP_HEIGHT(height),
			mixer_base + S5P_MXR_GRAPHIC0_WH);
		writel(S5P_MXR_GRP_STARTX(src_offs_x) |
			S5P_MXR_GRP_STARTY(src_offs_y),
			mixer_base + S5P_MXR_GRAPHIC0_SXY);
		VMPRINTK("0x%x, 0x%x, 0x%x\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC0_SPAN),
			readl(mixer_base + S5P_MXR_GRAPHIC0_WH),
			readl(mixer_base + S5P_MXR_GRAPHIC0_SXY));
		break;

	case VM_GPR1_LAYER:
		writel(S5P_MXR_GRP_SPAN(span),
			mixer_base + S5P_MXR_GRAPHIC1_SPAN);
		writel(S5P_MXR_GRP_WIDTH(width) | S5P_MXR_GRP_HEIGHT(height),
		       mixer_base + S5P_MXR_GRAPHIC1_WH);
		writel(S5P_MXR_GRP_STARTX(src_offs_x) |
			S5P_MXR_GRP_STARTY(src_offs_y),
			mixer_base + S5P_MXR_GRAPHIC1_SXY);
		VMPRINTK("0x%x, 0x%x, 0x%x\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC1_SPAN),
			readl(mixer_base + S5P_MXR_GRAPHIC1_WH),
			readl(mixer_base + S5P_MXR_GRAPHIC1_SXY));
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	return VMIXER_NO_ERROR;
}

enum s5p_tv_vmx_err __s5p_vm_set_bg_color(
	enum s5p_tv_vmx_bg_color_num colornum,
	u32 color_y,
	u32 color_cb,
	u32 color_cr)
{
	u32 reg_value;
	VMPRINTK("%d, %d, %d, %d)\n\r", colornum, color_y, color_cb, color_cr);

	reg_value = S5P_MXR_BG_COLOR_Y(color_y) |
		S5P_MXR_BG_COLOR_CB(color_cb) |
		S5P_MXR_BG_COLOR_CR(color_cr);

	switch (colornum) {

	case VMIXER_BG_COLOR_0:
		writel(reg_value, mixer_base + S5P_MXR_BG_COLOR0);
		VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_BG_COLOR0));
		break;

	case VMIXER_BG_COLOR_1:
		writel(reg_value, mixer_base + S5P_MXR_BG_COLOR1);
		VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_BG_COLOR1));
		break;

	case VMIXER_BG_COLOR_2:
		writel(reg_value, mixer_base + S5P_MXR_BG_COLOR2);
		VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_BG_COLOR2));
		break;

	default:
		VMPRINTK(" invalid uiColorNum parameter = %d\n\r", colornum);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	return VMIXER_NO_ERROR;
}



/*
* initialization  - iniization functions are only called under stopping vmixer
*/
enum s5p_tv_vmx_err __s5p_vm_init_status_reg(enum s5p_vmx_burst_mode burst,
	enum s5p_endian_type endian)
{
	u32 temp_reg = 0;

	VMPRINTK("++(%d, %d)\n\r", burst, endian);

	temp_reg = S5P_MXR_MIXER_RESERVED |
		S5P_MXR_CMU_CANNOT_STOP_CLOCK;

	switch (burst) {

	case VM_BURST_8:
		temp_reg |= S5P_MXR_BURST8_MODE;
		break;

	case VM_BURST_16:
		temp_reg |= S5P_MXR_BURST16_MODE;
		break;

	default:
		VMPRINTK("[ERR] : invalid burst parameter = %d\n\r", burst);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	switch (endian) {

	case TVOUT_BIG_ENDIAN_MODE:
		temp_reg |= S5P_MXR_BIG_ENDIAN_SOURCE_FORMAT;
		break;

	case TVOUT_LITTLE_ENDIAN_MODE:
		temp_reg |= S5P_MXR_LITTLE_ENDIAN_SOURCE_FORMAT;
		break;

	default:
		VMPRINTK("[ERR] : invalid endian parameter = %d\n\r", endian);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	writel(temp_reg, mixer_base + S5P_MXR_STATUS);

	VMPRINTK("--(0x%x)\n\r", readl(mixer_base + S5P_MXR_STATUS));

	return VMIXER_NO_ERROR;
}

enum s5p_tv_vmx_err __s5p_vm_init_display_mode(enum s5p_tv_disp_mode mode,
	enum s5p_tv_o_mode output_mode)
{
	u32 temp_reg = 0;

	VMPRINTK("%d, %d)\n\r", mode, output_mode);

	switch (mode) {

	case TVOUT_NTSC_M:

	case TVOUT_NTSC_443:
		temp_reg = S5P_MXR_SD | S5P_MXR_NTSC;
		break;

	case TVOUT_PAL_BDGHI:

	case TVOUT_PAL_M:

	case TVOUT_PAL_N:

	case TVOUT_PAL_NC:

	case TVOUT_PAL_60:
		temp_reg = S5P_MXR_SD | S5P_MXR_PAL;
		break;

	case TVOUT_480P_60_16_9:

	case TVOUT_480P_60_4_3:
		temp_reg = S5P_MXR_SD | S5P_MXR_NTSC;
		break;

	case TVOUT_576P_50_16_9:

	case TVOUT_576P_50_4_3:
		temp_reg = S5P_MXR_SD | S5P_MXR_PAL;
		break;

	case TVOUT_720P_50:

	case TVOUT_720P_60:
		temp_reg = S5P_MXR_HD | S5P_MXR_HD_720P_MODE;
		break;

	default:
		VMPRINTK(" invalid mode parameter = %d\n\r", mode);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	switch (output_mode) {

	case TVOUT_OUTPUT_COMPOSITE:

	case TVOUT_OUTPUT_SVIDEO:

	case TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED:
		temp_reg |= S5P_MXR_INTERLACE_MODE;
		break;

	case TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE:

	case TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE:
		temp_reg |= S5P_MXR_PROGRESSVE_MODE;
		break;

	case TVOUT_OUTPUT_HDMI:
		temp_reg |= S5P_MXR_PROGRESSVE_MODE;
		break;

	default:
		VMPRINTK(" invalid mode parameter = %d\n\r", mode);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	writel(temp_reg, mixer_base + S5P_MXR_CFG);

	VMPRINTK("--(0x%x)\n\r", readl(mixer_base + S5P_MXR_CFG));

	return VMIXER_NO_ERROR;
}

enum s5p_tv_vmx_err __s5p_vm_init_layer(enum s5p_tv_vmx_layer layer,
				   bool show,
				   bool win_blending,
				   u32 alpha,
				   u32 priority,
				   enum s5p_tv_vmx_color_fmt color,
				   bool blank_change,
				   bool pixel_blending,
				   bool premul,
				   u32 blank_color,
				   u32 base_addr,
				   u32 span,
				   u32 width,
				   u32 height,
				   u32 src_offs_x,
				   u32 src_offs_y,
				   u32 dst_offs_x,
				   u32 dst_offs_y)
{
	u32 temp_reg = 0;

	VMPRINTK("%d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x,\
		0x%x, %d, %d, %d, %d, %d, %d, %d)\n\r",
		layer, show, win_blending, alpha, priority,
		color, blank_change, pixel_blending, premul,
		blank_color, base_addr, span, width, height,
		src_offs_x, src_offs_y, dst_offs_x, dst_offs_y);

	switch (layer) {

	case VM_VIDEO_LAYER:
		temp_reg = (win_blending) ? S5P_MXR_VP_BLEND_ENABLE :
			S5P_MXR_VP_BLEND_DISABLE;
		temp_reg |= S5P_MXR_VP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_VIDEO_CFG);
		break;

	case VM_GPR0_LAYER:
		temp_reg = (blank_change) ?
			S5P_MXR_BLANK_NOT_CHANGE_NEW_PIXEL :
			S5P_MXR_BLANK_CHANGE_NEW_PIXEL;
		temp_reg |= (premul) ? S5P_MXR_PRE_MUL_MODE :
			    S5P_MXR_NORMAL_MODE;
		temp_reg |= (win_blending) ? S5P_MXR_WIN_BLEND_ENABLE :
			    S5P_MXR_WIN_BLEND_DISABLE;
		temp_reg |= (pixel_blending) ? S5P_MXR_PIXEL_BLEND_ENABLE :
			    S5P_MXR_PIXEL_BLEND_DISABLE;
		temp_reg |= S5P_MXR_EG_COLOR_FORMAT(color);
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		writel(S5P_MXR_GPR_BLANK_COLOR(blank_color),
			mixer_base + S5P_MXR_GRAPHIC0_BLANK);

		VMPRINTK("--(0x%x)\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC0_CFG));
		VMPRINTK("--(0x%x)\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC0_BLANK));

		__s5p_vm_set_grp_layer_size(layer, span, width, height,
			src_offs_x, src_offs_y);

		__s5p_vm_set_grp_base_address(layer, base_addr);
		__s5p_vm_set_grp_layer_position(layer, dst_offs_x, dst_offs_y);

		break;

	case VM_GPR1_LAYER:
		temp_reg = (blank_change) ?
			S5P_MXR_BLANK_NOT_CHANGE_NEW_PIXEL :
			S5P_MXR_BLANK_CHANGE_NEW_PIXEL;
		temp_reg |= (premul) ? S5P_MXR_PRE_MUL_MODE :
			S5P_MXR_NORMAL_MODE;
		temp_reg |= (win_blending) ? S5P_MXR_WIN_BLEND_ENABLE :
			    S5P_MXR_WIN_BLEND_DISABLE;
		temp_reg |= (pixel_blending) ? S5P_MXR_PIXEL_BLEND_ENABLE :
			    S5P_MXR_PIXEL_BLEND_DISABLE;
		temp_reg |= S5P_MXR_EG_COLOR_FORMAT(color);
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		writel(S5P_MXR_GPR_BLANK_COLOR(blank_color),
		       mixer_base + S5P_MXR_GRAPHIC1_BLANK);

		VMPRINTK("--(0x%x)\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC1_CFG));
		VMPRINTK("--(0x%x)\n\r",
			readl(mixer_base + S5P_MXR_GRAPHIC1_BLANK));

		__s5p_vm_set_grp_layer_size(layer, span, width, height,
					    src_offs_x, src_offs_y);

		__s5p_vm_set_grp_base_address(layer, base_addr);
		__s5p_vm_set_grp_layer_position(layer, dst_offs_x, dst_offs_y);
		break;

	default:
		VMPRINTK("invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	__s5p_vm_set_layer_priority(layer, priority);

	__s5p_vm_set_layer_show(layer, show);

	return VMIXER_NO_ERROR;
}

void __s5p_vm_init_bg_dither_enable(bool cr_dither_enable,
				    bool cb_dither_enable,
				    bool y_dither_enable)
{
	u32 temp_reg = 0;

	VMPRINTK("%d, %d, %d\n\r", cr_dither_enable,
		cb_dither_enable, y_dither_enable);

	temp_reg = (cr_dither_enable) ?
		   (temp_reg | S5P_MXR_BG_CR_DIHER_EN) :
		   (temp_reg & ~S5P_MXR_BG_CR_DIHER_EN);
	temp_reg = (cb_dither_enable) ?
		   (temp_reg | S5P_MXR_BG_CB_DIHER_EN) :
		   (temp_reg & ~S5P_MXR_BG_CB_DIHER_EN);
	temp_reg = (y_dither_enable) ?
		   (temp_reg | S5P_MXR_BG_Y_DIHER_EN) :
		   (temp_reg & ~S5P_MXR_BG_Y_DIHER_EN);

	writel(temp_reg, mixer_base + S5P_MXR_BG_CFG);
	VMPRINTK("--(0x%x)\n\r", readl(mixer_base + S5P_MXR_BG_CFG));

}


enum s5p_tv_vmx_err __s5p_vm_init_bg_color(
	enum s5p_tv_vmx_bg_color_num color_num,
	u32 color_y,
	u32 color_cb,
	u32 color_cr)
{
	return __s5p_vm_set_bg_color(color_num, color_y, color_cb, color_cr);
}

enum s5p_tv_vmx_err __s5p_vm_init_csc_coef(enum s5p_yuv_fmt_component component,
				      enum s5p_tv_coef_y_mode mode,
				      u32 coeff0,
				      u32 coeff1,
				      u32 coeff2)
{
	u32 mxr_cm;

	VMPRINTK("%d, %d, %d, %d, %d\n\r", component, mode, coeff0,
		coeff1, coeff2);

	switch (component) {

	case TVOUT_YUV_Y:
		mxr_cm 	= (mode == VMIXER_COEF_Y_WIDE) ?
			  S5P_MXR_BG_COLOR_WIDE : S5P_MXR_BG_COLOR_NARROW;
		mxr_cm |= S5P_MXR_BG_COEFF_0(coeff0) |
			  S5P_MXR_BG_COEFF_1(coeff1) |
			  S5P_MXR_BG_COEFF_2(coeff2);
		writel(mxr_cm, mixer_base + S5P_MXR_CM_COEFF_Y);
		VMPRINTK("--(0x%x)\n\r",
			readl(mixer_base + S5P_MXR_CM_COEFF_Y));
		break;

	case TVOUT_YUV_CB:
		mxr_cm 	= S5P_MXR_BG_COEFF_0(coeff0) |
			  S5P_MXR_BG_COEFF_1(coeff1) |
			  S5P_MXR_BG_COEFF_2(coeff2);
		writel(mxr_cm, mixer_base + S5P_MXR_CM_COEFF_CB);
		VMPRINTK("--(0x%x)\n\r",
			readl(mixer_base + S5P_MXR_CM_COEFF_CB));
		break;

	case TVOUT_YUV_CR:
		mxr_cm 	= S5P_MXR_BG_COEFF_0(coeff0) |
			  S5P_MXR_BG_COEFF_1(coeff1) |
			  S5P_MXR_BG_COEFF_2(coeff2);
		writel(mxr_cm, S5P_MXR_CM_COEFF_CR);
		VMPRINTK("--(0x%x)\n\r",
			readl(mixer_base + S5P_MXR_CM_COEFF_CR));
		break;

	default:
		VMPRINTK("invalid component parameter = %d\n\r", component);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	return VMIXER_NO_ERROR;
}

void __s5p_vm_init_csc_coef_default(enum s5p_tv_vmx_csc_type csc_type)
{
	VMPRINTK("%d\n\r", csc_type);

	switch (csc_type) {

	case VMIXER_CSC_RGB_TO_YUV601_LR:
		writel((0 << 30) | (153 << 20) | (300 << 10) | (58 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((936 << 20) | (851 << 10) | (262 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((262 << 20) | (805 << 10) | (982 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	case VMIXER_CSC_RGB_TO_YUV601_FR:
		writel((1 << 30) | (132 << 20) | (258 << 10) | (50 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((948 << 20) | (875 << 10) | (225 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((225 << 20) | (836 << 10) | (988 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	case VMIXER_CSC_RGB_TO_YUV709_LR:
		writel((0 << 30) | (109 << 20) | (366 << 10) | (36 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((964 << 20) | (822 << 10) | (216 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((262 << 20) | (787 << 10) | (1000 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	case VMIXER_CSC_RGB_TO_YUV709_FR:
		writel((1 << 30) | (94 << 20) | (314 << 10) | (32 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((972 << 20) | (851 << 10) | (225 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((225 << 20) | (820 << 10) | (1004 << 0),
		       mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	default:
		VMPRINTK(" invalid csc_type parameter = %d\n\r", csc_type);
		break;
	}

	VMPRINTK("--(0x%x)\n\r", readl(mixer_base + S5P_MXR_CM_COEFF_Y));

	VMPRINTK("--(0x%x)\n\r", readl(mixer_base + S5P_MXR_CM_COEFF_CB));
	VMPRINTK("--(0x%x)\n\r", readl(mixer_base + S5P_MXR_CM_COEFF_CR));
}

/*
* etc
*/
enum s5p_tv_vmx_err __s5p_vm_get_layer_info(enum s5p_tv_vmx_layer layer,
				       bool *show,
				       u32 *priority)
{
	VMPRINTK("%d\n\r", layer);

	switch (layer) {

	case VM_VIDEO_LAYER:
		*show = (readl(mixer_base + S5P_MXR_LAYER_CFG) &
			S5P_MXR_VIDEO_LAYER_SHOW) ? 1 : 0;
		*priority = S5P_MXR_VP_LAYER_PRIORITY_INFO(
			readl(mixer_base + S5P_MXR_LAYER_CFG));
		break;

	case VM_GPR0_LAYER:
		*show = (readl(mixer_base + S5P_MXR_LAYER_CFG) &
			S5P_MXR_GRAPHIC0_LAYER_SHOW) ? 1 : 0;
		*priority = S5P_MXR_GRP0_LAYER_PRIORITY_INFO(
			readl(mixer_base + S5P_MXR_LAYER_CFG));
		break;

	case VM_GPR1_LAYER:
		*show = (readl(mixer_base + S5P_MXR_LAYER_CFG) &
			S5P_MXR_GRAPHIC1_LAYER_SHOW) ? 1 : 0;
		*priority = S5P_MXR_GRP1_LAYER_PRIORITY_INFO(
			readl(mixer_base + S5P_MXR_LAYER_CFG));
		break;

	default:
		VMPRINTK("invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	VMPRINTK("%d, %d\n\r", *show, *priority);

	return VMIXER_NO_ERROR;
}

/*
* start  - start functions are only called under stopping vmixer
*/

void __s5p_vm_start(void)
{
	VMPRINTK("()\n\r");
	writel((readl(mixer_base + S5P_MXR_STATUS) | S5P_MXR_MIXER_START),
		mixer_base + S5P_MXR_STATUS);
	VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_STATUS));


	VMPRINTK("S5P_MXR_STATUS \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_STATUS));
	VMPRINTK("S5P_MXR_INT_EN \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_INT_EN));
	VMPRINTK("S5P_MXR_BG_CFG \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_BG_CFG));
	VMPRINTK("S5P_MXR_BG_COLOR0 \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_BG_COLOR0));
	VMPRINTK("S5P_MXR_BG_COLOR1 \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_BG_COLOR1));
	VMPRINTK("S5P_MXR_BG_COLOR2 \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_BG_COLOR2));
	VMPRINTK("S5P_MXR_CM_COEFF_Y \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_CM_COEFF_Y));
	VMPRINTK("S5P_MXR_CM_COEFF_CB \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_CM_COEFF_CB));
	VMPRINTK("S5P_MXR_CM_COEFF_CR \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_CM_COEFF_CR));
	VMPRINTK("S5P_MXR_CM_COEFF_Y \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_CM_COEFF_Y));
	VMPRINTK("S5P_MXR_CM_COEFF_CB \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_CM_COEFF_CB));
	VMPRINTK("S5P_MXR_CM_COEFF_CR \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_CM_COEFF_CR));
	VMPRINTK("S5P_MXR_GRAPHIC0_CFG \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC0_CFG));
	VMPRINTK("S5P_MXR_GRAPHIC0_BASE \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC0_BASE));
	VMPRINTK("S5P_MXR_GRAPHIC0_SPAN \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC0_SPAN));
	VMPRINTK("S5P_MXR_GRAPHIC0_WH \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC0_WH));
	VMPRINTK("S5P_MXR_GRAPHIC0_SXY \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC0_SXY));
	VMPRINTK("S5P_MXR_GRAPHIC0_DXY \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC0_DXY));
	VMPRINTK("S5P_MXR_GRAPHIC0_BLANK \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC0_BLANK));
	VMPRINTK("S5P_MXR_GRAPHIC1_BASE \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC1_BASE));
	VMPRINTK("S5P_MXR_GRAPHIC1_SPAN \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC1_SPAN));
	VMPRINTK("S5P_MXR_GRAPHIC1_WH \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC1_WH));
	VMPRINTK("S5P_MXR_GRAPHIC1_SXY \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC1_SXY));
	VMPRINTK("S5P_MXR_GRAPHIC1_DXY \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC1_DXY));
	VMPRINTK("S5P_MXR_GRAPHIC1_BLANK \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_GRAPHIC1_BLANK));
	VMPRINTK("S5P_MXR_CFG \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_CFG));
	VMPRINTK("S5P_MXR_LAYER_CFG \t\t 0x%08x\n ",
		readl(mixer_base + S5P_MXR_LAYER_CFG));

}

/*
* stop  - stop functions are only called under running vmixer
*/

void __s5p_vm_stop(void)
{
	VMPRINTK("()\n\r");
	writel((readl(mixer_base + S5P_MXR_STATUS) & ~S5P_MXR_MIXER_START),
	       mixer_base + S5P_MXR_STATUS);
	VMPRINTK("0x%x\n\r", readl(mixer_base + S5P_MXR_STATUS));
}

/*
* interrupt - for debug
*/

enum s5p_tv_vmx_err __s5p_vm_set_underflow_interrupt_enable(
	enum s5p_tv_vmx_layer layer, bool en)
{
	u32 enablemaks;

	VMPRINTK("%d, %d\n\r", layer, en);

	switch (layer) {

	case VM_VIDEO_LAYER:
		enablemaks = S5P_MXR_VP_INT_ENABLE;
		break;

	case VM_GPR0_LAYER:
		enablemaks = S5P_MXR_GRP0_INT_ENABLE;
		break;

	case VM_GPR1_LAYER:
		enablemaks = S5P_MXR_GRP1_INT_ENABLE;
		break;

	default:
		VMPRINTK("invalid layer parameter = %d\n\r", layer);
		return S5P_TV_VMX_ERR_INVALID_PARAM;
		break;
	}

	if (en) {
		writel((readl(mixer_base + S5P_MXR_INT_EN) | enablemaks),
		       mixer_base + S5P_MXR_INT_EN);
	} else {
		writel((readl(mixer_base + S5P_MXR_INT_EN) & ~enablemaks),
		       mixer_base + S5P_MXR_INT_EN);
	}

	VMPRINTK("0x%x)\n\r", readl(mixer_base + S5P_MXR_INT_EN));

	return VMIXER_NO_ERROR;
}

void __s5p_vm_clear_pend_all(void)
{
	writel(S5P_MXR_INT_FIRED | S5P_MXR_VP_INT_FIRED |
	       S5P_MXR_GRP0_INT_FIRED | S5P_MXR_GRP1_INT_FIRED,
	       mixer_base + S5P_MXR_INT_EN);
}

irqreturn_t __s5p_mixer_irq(int irq, void *dev_id)
{
	bool v_i_f;
	bool g0_i_f;
	bool g1_i_f;
	bool mxr_i_f;
	u32 temp_reg = 0;

	v_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
		 & S5P_MXR_VP_INT_FIRED) ? true : false;
	g0_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
		  & S5P_MXR_GRP0_INT_FIRED) ? true : false;
	g1_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
		  & S5P_MXR_GRP1_INT_FIRED) ? true : false;
	mxr_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
		   & S5P_MXR_INT_FIRED) ? true : false;

	if (mxr_i_f) {
		temp_reg |= S5P_MXR_INT_FIRED;

		if (v_i_f) {
			temp_reg |= S5P_MXR_VP_INT_FIRED;
			printk("VP fifo under run!!\n\r");
		}

		if (g0_i_f) {
			temp_reg |= S5P_MXR_GRP0_INT_FIRED;
			printk("GRP0 fifo under run!!\n\r");
		}

		if (g1_i_f) {
			temp_reg |= S5P_MXR_GRP1_INT_FIRED;
			printk("GRP1 fifo under run!!\n\r");
		}

		writel(temp_reg, mixer_base + S5P_MXR_INT_STATUS);
	}

	return IRQ_HANDLED;
}

int __init __s5p_mixer_probe(struct platform_device *pdev, u32 res_num)
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

	mixer_mem = request_mem_region(res->start, size, pdev->name);

	if (mixer_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		ret = -ENOENT;

	}

	mixer_base = ioremap(res->start, size);

	if (mixer_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		ret = -ENOENT;


	}

	return ret;

}

int __init __s5p_mixer_release(struct platform_device *pdev)
{
	iounmap(mixer_base);

	/* remove memory region */

	if (mixer_mem != NULL) {
		if (release_resource(mixer_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(mixer_mem);

		mixer_mem = NULL;
	}

	return 0;
}
