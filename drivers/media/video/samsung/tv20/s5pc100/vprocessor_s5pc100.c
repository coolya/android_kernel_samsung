/* linux/drivers/media/video/samsung/tv20/s5pc100/vprocessor_s5pc100.c
 *
 * Video Processor raw ftn  file for Samsung TVOut driver
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
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/io.h>

#include "tv_out_s5pc100.h"

#include "regs/regs-vprocessor.h"
#include "vp_coeff_s5pc100.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_VP_DEBUG 1
#endif

#ifdef S5P_VP_DEBUG
#define VPPRINTK(fmt, args...)\
	printk(KERN_INFO "\t\t[VP] %s: " fmt, __func__ , ## args)
#else
#define VPPRINTK(fmt, args...)
#endif

static struct resource	*vp_mem;
void __iomem		*vp_base;

/*
* set
*  - set functions are only called under running video processor
*  - after running set functions, it is need to run __s5p_vp_update() function
*    for update shadow registers
*/
void __s5p_vp_set_field_id(enum s5p_vp_field mode)
{
	VPPRINTK("%d\n\r", mode);

	writel((mode == VPROC_TOP_FIELD) ?
	       vp_base + S5P_VP_FIELD_ID_TOP :
	       vp_base + S5P_VP_FIELD_ID_BOTTOM,
	       vp_base + S5P_VP_FIELD_ID);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_VP_FIELD_ID));
}

enum s5p_tv_vp_err __s5p_vp_set_top_field_address(u32 top_y_addr,
	u32 top_c_addr)
{
	VPPRINTK("0x%x, 0x%x\n\r", top_y_addr, top_c_addr);

	if (VP_PTR_ILLEGAL(top_y_addr) || VP_PTR_ILLEGAL(top_c_addr)) {
		VPPRINTK(" address is not double word align = 0x%x, 0x%x\n\r",
			 top_y_addr, top_c_addr);
		return S5P_TV_VP_ERR_BASE_ADDRESS_MUST_DOUBLE_WORD_ALIGN;
	}

	writel(top_y_addr, vp_base + S5P_VP_TOP_Y_PTR);

	writel(top_c_addr, vp_base + S5P_VP_TOP_C_PTR);


	VPPRINTK("0x%x, 0x%x\n\r", readl(vp_base + S5P_VP_TOP_Y_PTR),
		 readl(vp_base + S5P_VP_TOP_C_PTR));

	return VPROC_NO_ERROR;
}

enum s5p_tv_vp_err __s5p_vp_set_bottom_field_address(u32 bottom_y_addr,
	u32 bottom_c_addr)
{
	VPPRINTK("0x%x, 0x%x\n\r", bottom_y_addr, bottom_c_addr);

	if (VP_PTR_ILLEGAL(bottom_y_addr) || VP_PTR_ILLEGAL(bottom_c_addr)) {
		VPPRINTK(" address is not double word align = 0x%x, 0x%x\n\r",
			 bottom_y_addr, bottom_c_addr);
		return S5P_TV_VP_ERR_BASE_ADDRESS_MUST_DOUBLE_WORD_ALIGN;
	}

	writel(bottom_y_addr, vp_base + S5P_VP_BOT_Y_PTR);

	writel(bottom_c_addr, vp_base + S5P_VP_BOT_C_PTR);


	VPPRINTK("0x%x, 0x%x\n\r", readl(vp_base + S5P_VP_BOT_Y_PTR),
		 readl(vp_base + S5P_VP_BOT_C_PTR));

	return VPROC_NO_ERROR;
}


enum s5p_tv_vp_err __s5p_vp_set_img_size(u32 img_width, u32 img_height)
{
	VPPRINTK("%d, %d\n\r", img_width, img_height);

	if (VP_IMG_SIZE_ILLEGAL(img_width) ||
		VP_IMG_SIZE_ILLEGAL(img_height / 2)) {
		VPPRINTK(" image full size is not double word align\
			= %d, %d\n\r",
			 img_width, img_height);
		return S5P_TV_VP_ERR_BASE_ADDRESS_MUST_DOUBLE_WORD_ALIGN;
	}

	writel(VP_IMG_HSIZE(img_width) | VP_IMG_VSIZE(img_height),

	       vp_base + S5P_VP_IMG_SIZE_Y);

	writel(VP_IMG_HSIZE(img_width) | VP_IMG_VSIZE(img_height / 2),
	       vp_base + S5P_VP_IMG_SIZE_C);

	VPPRINTK("0x%x, 0x%x\n\r", readl(vp_base + S5P_VP_IMG_SIZE_Y),
		 readl(vp_base + S5P_VP_IMG_SIZE_C));

	return VPROC_NO_ERROR;
}

void __s5p_vp_set_src_position(u32 src_off_x,
			       u32 src_x_fract_step,
			       u32 src_off_y)
{
	VPPRINTK("%d, %d, %d)\n\r", src_off_x, src_x_fract_step, src_off_y);

	writel(VP_SRC_H_POSITION(src_off_x) |
		VP_SRC_X_FRACT_STEP(src_x_fract_step),
		vp_base + S5P_VP_SRC_H_POSITION);
	writel(VP_SRC_V_POSITION(src_off_y), vp_base + S5P_VP_SRC_V_POSITION);

	VPPRINTK("0x%x, 0x%x\n\r", readl(vp_base + S5P_VP_SRC_H_POSITION),
		 readl(vp_base + S5P_VP_SRC_V_POSITION));
}

void __s5p_vp_set_dest_position(u32 dst_off_x,
				u32 dst_off_y)
{
	VPPRINTK("%d, %d)\n\r", dst_off_x, dst_off_y);


	writel(VP_DST_H_POSITION(dst_off_x), vp_base + S5P_VP_DST_H_POSITION);
	writel(VP_DST_V_POSITION(dst_off_y), vp_base + S5P_VP_DST_V_POSITION);

	VPPRINTK("0x%x, 0x%x\n\r", readl(vp_base + S5P_VP_DST_H_POSITION),
		 readl(vp_base + S5P_VP_DST_V_POSITION));
}

void __s5p_vp_set_src_dest_size(u32 src_width,
				u32 src_height,
				u32 dst_width,
				u32 dst_height,
				bool ipc_2d)
{
	u32 h_ratio = (src_width << 16) / dst_width;
	u32 v_ratio = (ipc_2d) ?
		      ((src_height << 17) / dst_height) :
		      ((src_height << 16) / dst_height);

	VPPRINTK("(%d, %d, %d, %d)++\n\r", src_width, src_height,
		dst_width, dst_height);

	writel(VP_SRC_WIDTH(src_width), vp_base + S5P_VP_SRC_WIDTH);
	writel(VP_SRC_HEIGHT(src_height), vp_base + S5P_VP_SRC_HEIGHT);
	writel(VP_DST_WIDTH(dst_width), vp_base + S5P_VP_DST_WIDTH);
	writel(VP_DST_HEIGHT(dst_height), vp_base + S5P_VP_DST_HEIGHT) ;
	writel(VP_H_RATIO(h_ratio), vp_base + S5P_VP_H_RATIO);
	writel(VP_V_RATIO(v_ratio), vp_base + S5P_VP_V_RATIO);

	writel((ipc_2d) ? (readl(vp_base + S5P_VP_MODE) | VP_2D_IPC_ON) :
	       (readl(vp_base + S5P_VP_MODE) & ~VP_2D_IPC_ON),
	       vp_base + S5P_VP_MODE);

	VPPRINTK("%d, %d, %d, %d, 0x%x, 0x%x\n\r",
		readl(vp_base + S5P_VP_SRC_WIDTH),
		readl(vp_base + S5P_VP_SRC_HEIGHT),
		readl(vp_base + S5P_VP_DST_WIDTH),
		readl(vp_base + S5P_VP_DST_HEIGHT),
		readl(vp_base + S5P_VP_H_RATIO),
		readl(vp_base + S5P_VP_V_RATIO));
}

enum s5p_tv_vp_err __s5p_vp_set_poly_filter_coef(
	enum s5p_vp_poly_coeff poly_coeff,
	signed char ch0,
	signed char ch1,
	signed char ch2,
	signed char ch3)
{
	VPPRINTK("%d, %d, %d, %d, %d)\n\r", poly_coeff, ch0, ch1, ch2, ch3);

	if (poly_coeff > VPROC_POLY4_C1_HH ||
		poly_coeff < VPROC_POLY8_Y0_LL ||
		(poly_coeff > VPROC_POLY8_Y3_HH &&
		poly_coeff < VPROC_POLY4_Y0_LL)) {

		VPPRINTK("invaild poly_coeff parameter \n\r");
		return S5P_TV_VP_ERR_INVALID_PARAM;
	}

	writel((((0xff&ch0) << 24) | ((0xff&ch1) << 16) |
		((0xff&ch2) << 8) | (0xff&ch3)),
	       vp_base + S5P_VP_POLY8_Y0_LL + poly_coeff*4);

	VPPRINTK("0x%08x, 0x%08x\n\r",
		readl(vp_base + S5P_VP_POLY8_Y0_LL + poly_coeff*4),
		vp_base + S5P_VP_POLY8_Y0_LL + poly_coeff*4);

	return VPROC_NO_ERROR;
}

void __s5p_vp_set_poly_filter_coef_default(u32 h_ratio, u32 v_ratio)
{
	enum s5p_tv_vp_filter_h_pp e_h_filter;
	enum s5p_tv_vp_filter_v_pp e_v_filter;
	u8 *poly_flt_coeff;
	int i, j;

	VPPRINTK("%d, %d\n\r", h_ratio, v_ratio);

	/*
	* For the real interlace mode, the vertical ratio should be
	* used after divided by 2. Because in the interlace mode,
	* all the VP output is used for SDOUT display and it should
	* be the same as one field of the progressive mode.
	* Therefore the same filter coefficients should be used for
	* the same the final output video.
	* When half of the interlace V_RATIO is same as the progressive
	* V_RATIO, the final output video scale is same. (20051104, ishan)
	*/

	/*Horizontal Y 8tap */
	/*Horizontal C 4tap */

	if (h_ratio <= (0x1 << 16)) {		/* 720->720 or zoom in */
		e_h_filter = VPROC_PP_H_NORMAL;
	}

	else if (h_ratio <= (0x9 << 13))	/* 720->640 */
		e_h_filter = VPROC_PP_H_8_9 ;

	else if (h_ratio <= (0x1 << 17))	/* 2->1 */
		e_h_filter = VPROC_PP_H_1_2;

	else if (h_ratio <= (0x3 << 16))	/* 2->1 */
		e_h_filter = VPROC_PP_H_1_3;
	else
		e_h_filter = VPROC_PP_H_1_4;	/* 4->1 */

	/* Vertical Y 4tap */
	if (v_ratio <= (0x1 << 16))          	/* 720->720 or zoom in*/
		e_v_filter = VPROC_PP_V_NORMAL;
	else if (v_ratio <= (0x3 << 15))	/* 6->5*/
		e_v_filter = VPROC_PP_V_5_6;
	else if (v_ratio <= (0x5 << 14))	/* 4->3*/
		e_v_filter = VPROC_PP_V_3_4;
	else if (v_ratio <= (0x1 << 17))	/* 2->1*/
		e_v_filter = VPROC_PP_V_1_2;
	else if (v_ratio <= (0x3 << 16))	/* 3->1*/
		e_v_filter = VPROC_PP_V_1_3;
	else					/* 4->1*/
		e_v_filter = VPROC_PP_V_1_4;

	poly_flt_coeff = (u8 *)(g_s_vp8tap_coef_y_h + e_h_filter * 16 * 8);

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			__s5p_vp_set_poly_filter_coef(
				VPROC_POLY8_Y0_LL + (i*4) + j,
				*(poly_flt_coeff + 4*j*8 + (7 - i)),
				*(poly_flt_coeff + (4*j + 1)*8 + (7 - i)),
				*(poly_flt_coeff + (4*j + 2)*8 + (7 - i)),
				*(poly_flt_coeff + (4*j + 3)*8 + (7 - i)));
		}
	}

	poly_flt_coeff = (u8 *)(g_s_vp4tap_coef_c_h + e_h_filter * 16 * 4);

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4; j++) {
			__s5p_vp_set_poly_filter_coef(
				VPROC_POLY4_C0_LL + (i*4) + j,
				*(poly_flt_coeff + 4*j*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 1)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 2)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 3)*4 + (3 - i)));
		}
	}

	poly_flt_coeff = (u8 *)(g_s_vp4tap_coef_y_v + e_v_filter * 16 * 4);

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			__s5p_vp_set_poly_filter_coef(
				VPROC_POLY4_Y0_LL + (i*4) + j,
				*(poly_flt_coeff + 4*j*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 1)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 2)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 3)*4 + (3 - i)));
		}
	}

	VPPRINTK("%d, %d\n\r", e_h_filter, e_v_filter);
}

void __s5p_vp_set_src_dest_size_with_default_poly_filter_coef(u32 src_width,
				u32 src_height,
				u32 dst_width,
				u32 dst_height,
				bool ipc_2d)
{
	u32 h_ratio = (src_width << 16) / dst_width;
	u32 v_ratio = (ipc_2d) ? ((src_height << 17) / dst_height) :
		      ((src_height << 16) / dst_height);

	__s5p_vp_set_src_dest_size(src_width, src_height, dst_width,
		dst_height, ipc_2d);
	__s5p_vp_set_poly_filter_coef_default(h_ratio, v_ratio);
}

enum s5p_tv_vp_err __s5p_vp_set_brightness_contrast_control(
	enum s5p_vp_line_eq eq_num, u32 intc, u32 slope)
{
	VPPRINTK("%d, %d, %d\n\r", eq_num, intc, slope);

	if (eq_num > VProc_LINE_EQ_7 || eq_num < VProc_LINE_EQ_0) {
		VPPRINTK("invaild eq_num parameter \n\r");
		return S5P_TV_VP_ERR_INVALID_PARAM;
	}

	writel(VP_LINE_INTC(intc) | VP_LINE_SLOPE(slope),

	       vp_base + S5P_PP_LINE_EQ0 + eq_num*4);

	VPPRINTK("0x%08x, 0x%08x\n\r",
		readl(vp_base + S5P_PP_LINE_EQ0 + eq_num*4),
		vp_base + S5P_PP_LINE_EQ0 + eq_num*4);

	return VPROC_NO_ERROR;
}

void __s5p_vp_set_brightness(bool brightness)
{
	unsigned short i;

	VPPRINTK("%d\n\r", brightness);

	g_vp_contrast_brightness =
		VP_LINE_INTC_CLEAR(g_vp_contrast_brightness) |
		VP_LINE_INTC(brightness);

	for (i = 0; i < 8; i++)
		writel(g_vp_contrast_brightness,
			vp_base + S5P_PP_LINE_EQ0 + i*4);


	VPPRINTK("%d\n\r", g_vp_contrast_brightness);
}

void __s5p_vp_set_contrast(u8 contrast)
{
	unsigned short i;

	VPPRINTK("%d\n\r", contrast);

	g_vp_contrast_brightness =
		VP_LINE_SLOPE_CLEAR(g_vp_contrast_brightness) |
		VP_LINE_SLOPE(contrast);

	for (i = 0; i < 8; i++)
		writel(g_vp_contrast_brightness,
			vp_base + S5P_PP_LINE_EQ0 + i*4);


	VPPRINTK("%d\n\r", g_vp_contrast_brightness);
}

enum s5p_tv_vp_err __s5p_vp_update(void)
{
	VPPRINTK("()\n\r");

	writel(readl(vp_base + S5P_VP_SHADOW_UPDATE) |
		S5P_VP_SHADOW_UPDATE_ENABLE,
		vp_base + S5P_VP_SHADOW_UPDATE);

	VPPRINTK("()\n\r");

	return VPROC_NO_ERROR;
}

/*
* get  - get info
*/
enum s5p_vp_field __s5p_vp_get_field_id(void)
{
	VPPRINTK("()\n\r");
	return (readl(vp_base + S5P_VP_FIELD_ID) ==  S5P_VP_FIELD_ID_BOTTOM) ?
		VPROC_BOTTOM_FIELD : VPROC_TOP_FIELD;
}

/*
* etc
*/
unsigned short __s5p_vp_get_update_status(void)
{
	VPPRINTK("()\n\r");
	return readl(vp_base + S5P_VP_SHADOW_UPDATE) &
		S5P_VP_SHADOW_UPDATE_ENABLE;
}


void __s5p_vp_init_field_id(enum s5p_vp_field mode)
{
	__s5p_vp_set_field_id(mode);
}

void __s5p_vp_init_op_mode(bool line_skip,
			   enum s5p_vp_mem_mode mem_mode,
			   enum s5p_vp_chroma_expansion chroma_exp,
			   enum s5p_vp_filed_id_toggle toggle_id)
{
	u32 temp_reg;
	VPPRINTK("%d, %d, %d, %d\n\r", line_skip, mem_mode,
		chroma_exp, toggle_id);

	temp_reg = (line_skip) ? VP_LINE_SKIP_ON : VP_LINE_SKIP_OFF;
	temp_reg |= (mem_mode == VPROC_2D_TILE_MODE) ?
		    VP_MEM_2D_MODE : VP_MEM_LINEAR_MODE;
	temp_reg |= (chroma_exp == VPROC_USING_C_TOP_BOTTOM) ?
		    VP_CHROMA_USE_TOP_BOTTOM : VP_CHROMA_USE_TOP;
	temp_reg |= (toggle_id == S5P_TV_VP_FILED_ID_TOGGLE_VSYNC) ?
		    VP_FIELD_ID_TOGGLE_VSYNC : VP_FIELD_ID_TOGGLE_USER;

	writel(temp_reg, vp_base + S5P_VP_MODE);
	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_VP_MODE));
}

void __s5p_vp_init_pixel_rate_control(enum s5p_vp_pxl_rate rate)
{
	VPPRINTK("%d\n\r", rate);

	writel(VP_PEL_RATE_CTRL(rate), vp_base + S5P_VP_PER_RATE_CTRL);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_VP_PER_RATE_CTRL));
}

enum s5p_tv_vp_err __s5p_vp_init_layer(u32 top_y_addr,
				  u32 top_c_addr,
				  u32 bottom_y_addr,
				  u32 bottom_c_addr,
				  enum s5p_endian_type src_img_endian,
				  u32 img_width,
				  u32 img_height,
				  u32 src_off_x,
				  u32 src_x_fract_step,
				  u32 src_off_y,
				  u32 src_width,
				  u32 src_height,
				  u32 dst_off_x,
				  u32 dst_off_y,
				  u32 dst_width,
				  u32 dst_height,
				  bool ipc_2d)
{
	enum s5p_tv_vp_err error = VPROC_NO_ERROR;

	VPPRINTK("%d\n\r", src_img_endian);

	writel(1, vp_base + S5P_VP_ENDIAN_MODE);

	error = __s5p_vp_set_top_field_address(top_y_addr, top_c_addr);

	if (error != VPROC_NO_ERROR)
		return error;

	error = __s5p_vp_set_bottom_field_address(bottom_y_addr,
		bottom_c_addr);

	if (error != VPROC_NO_ERROR)
		return error;

	error = __s5p_vp_set_img_size(img_width, img_height);

	if (error != VPROC_NO_ERROR)
		return error;

	__s5p_vp_set_src_position(src_off_x, src_x_fract_step, src_off_y);

	__s5p_vp_set_dest_position(dst_off_x, dst_off_y);

	__s5p_vp_set_src_dest_size(src_width, src_height, dst_width,
		dst_height, ipc_2d);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_VP_ENDIAN_MODE));

	return error;

}

enum s5p_tv_vp_err __s5p_vp_init_layer_def_poly_filter_coef(u32 top_y_addr,
			u32 top_c_addr,
			u32 bottom_y_addr,
			u32 bottom_c_addr,
			enum s5p_endian_type src_img_endian,
			u32 img_width,
			u32 img_height,
			u32 src_off_x,
			u32 src_x_fract_step,
			u32 src_off_y,
			u32 src_width,
			u32 src_height,
			u32 dst_off_x,
			u32 dst_off_y,
			u32 dst_width,
			u32 dst_height,
			bool ipc_2d)
{
	enum s5p_tv_vp_err error = VPROC_NO_ERROR;

	u32 h_ratio = (src_width << 16) / dst_width;
	u32 v_ratio = (ipc_2d) ? ((src_height << 17) / dst_height) :
		      ((src_height << 16) / dst_height);

	__s5p_vp_set_poly_filter_coef_default(h_ratio, v_ratio);
	error = __s5p_vp_init_layer(top_y_addr, top_c_addr,
			bottom_y_addr, bottom_c_addr,
			src_img_endian,
			img_width, img_height,
			src_off_x, src_x_fract_step, src_off_y,
			src_width, src_height,
			dst_off_x, dst_off_y,
			dst_width, dst_height,
			ipc_2d);
	return error;
}

enum s5p_tv_vp_err __s5p_vp_init_poly_filter_coef(
	enum s5p_vp_poly_coeff poly_coeff,
	signed char ch0,
	signed char ch1,
	signed char ch2,
	signed char ch3)
{
	return __s5p_vp_set_poly_filter_coef(poly_coeff, ch0, ch1, ch2, ch3);
}

void __s5p_vp_init_bypass_post_process(bool bypass)
{
	VPPRINTK("%d\n\r", bypass);

	writel((bypass) ? VP_BY_PASS_ENABLE : VP_BY_PASS_DISABLE,
	       vp_base + S5P_PP_BYPASS);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_PP_BYPASS));
}

enum s5p_tv_vp_err __s5p_vp_init_csc_coef(enum s5p_vp_csc_coeff csc_coeff,
	u32 coeff)
{
	VPPRINTK("%d, %d\n\r", csc_coeff, coeff);

	if (csc_coeff > VPROC_CSC_CR2CR_COEF ||
		csc_coeff < VPROC_CSC_Y2Y_COEF) {
		VPPRINTK("invaild csc_coeff parameter \n\r");
		return S5P_TV_VP_ERR_INVALID_PARAM;
	}

	writel(VP_CSC_COEF(coeff),
		vp_base + S5P_PP_CSC_Y2Y_COEF + csc_coeff*4);

	VPPRINTK("0x%08x\n\r",
		readl(vp_base + S5P_PP_CSC_Y2Y_COEF + csc_coeff*4));

	return VPROC_NO_ERROR;
}

void __s5p_vp_init_saturation(u32 sat)
{
	VPPRINTK("%d\n\r", sat);

	writel(VP_SATURATION(sat), vp_base + S5P_PP_SATURATION);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_PP_SATURATION));
}

void __s5p_vp_init_sharpness(u32 th_h_noise,
	enum s5p_vp_sharpness_control sharpness)
{
	VPPRINTK("%d, %d\n\r", th_h_noise, sharpness);

	writel(VP_TH_HNOISE(th_h_noise) | VP_SHARPNESS(sharpness),
	       vp_base + S5P_PP_SHARPNESS);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_PP_SHARPNESS));
}

enum s5p_tv_vp_err __s5p_vp_init_brightness_contrast_control(
	enum s5p_vp_line_eq eq_num, u32 intc, u32 slope)
{
	return __s5p_vp_set_brightness_contrast_control(eq_num, intc, slope);
}

void __s5p_vp_init_brightness(bool brightness)
{
	__s5p_vp_set_brightness(brightness);
}


void __s5p_vp_init_contrast(u8 contrast)
{
	__s5p_vp_set_contrast(contrast);
}

void __s5p_vp_init_brightness_offset(u32 offset)
{
	VPPRINTK("%d\n\r", offset);

	writel(VP_BRIGHT_OFFSET(offset), vp_base + S5P_PP_BRIGHT_OFFSET);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_PP_BRIGHT_OFFSET));
}

void __s5p_vp_init_csc_control(bool sub_y_offset_en, bool csc_en)
{
	u32 temp_reg;
	VPPRINTK("%d, %d\n\r", sub_y_offset_en, csc_en);

	temp_reg = (sub_y_offset_en) ? VP_SUB_Y_OFFSET_ENABLE :
		   VP_SUB_Y_OFFSET_DISABLE;
	temp_reg |= (csc_en) ? VP_CSC_ENABLE : VP_CSC_DISABLE;
	writel(temp_reg, vp_base + S5P_PP_CSC_EN);

	VPPRINTK("0x%08x\n\r", readl(vp_base + S5P_PP_CSC_EN));
}

enum s5p_tv_vp_err __s5p_vp_init_csc_coef_default(enum s5p_vp_csc_type csc_type)
{
	VPPRINTK("%d\n\r", csc_type);

	switch (csc_type) {

	case VPROC_CSC_SD_HD:
		writel(Y2Y_COEF_601_TO_709, vp_base + S5P_PP_CSC_Y2Y_COEF);
		writel(CB2Y_COEF_601_TO_709, vp_base + S5P_PP_CSC_CB2Y_COEF);
		writel(CR2Y_COEF_601_TO_709, vp_base + S5P_PP_CSC_CR2Y_COEF);
		writel(Y2CB_COEF_601_TO_709, vp_base + S5P_PP_CSC_Y2CB_COEF);
		writel(CB2CB_COEF_601_TO_709, vp_base + S5P_PP_CSC_CB2CB_COEF);
		writel(CR2CB_COEF_601_TO_709, vp_base + S5P_PP_CSC_CR2CB_COEF);
		writel(Y2CR_COEF_601_TO_709, vp_base + S5P_PP_CSC_Y2CR_COEF);
		writel(CB2CR_COEF_601_TO_709, vp_base + S5P_PP_CSC_CB2CR_COEF);
		writel(CR2CR_COEF_601_TO_709, vp_base + S5P_PP_CSC_CR2CR_COEF);
		break;

	case VPROC_CSC_HD_SD:
		writel(Y2Y_COEF_709_TO_601, vp_base + S5P_PP_CSC_Y2Y_COEF);
		writel(CB2Y_COEF_709_TO_601, vp_base + S5P_PP_CSC_CB2Y_COEF);
		writel(CR2Y_COEF_709_TO_601, vp_base + S5P_PP_CSC_CR2Y_COEF);
		writel(Y2CB_COEF_709_TO_601, vp_base + S5P_PP_CSC_Y2CB_COEF);
		writel(CB2CB_COEF_709_TO_601, vp_base + S5P_PP_CSC_CB2CB_COEF);
		writel(CR2CB_COEF_709_TO_601, vp_base + S5P_PP_CSC_CR2CB_COEF);
		writel(Y2CR_COEF_709_TO_601, vp_base + S5P_PP_CSC_Y2CR_COEF);
		writel(CB2CR_COEF_709_TO_601, vp_base + S5P_PP_CSC_CB2CR_COEF);
		writel(CR2CR_COEF_709_TO_601, vp_base + S5P_PP_CSC_CR2CR_COEF);
		break;

	default:
		VPPRINTK("invalid csc_type parameter = %d\n\r", csc_type);
		return S5P_TV_VP_ERR_INVALID_PARAM;
		break;
	}

	VPPRINTK("0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x,\
		0x%08x, 0x%08x, 0x%08x, 0x%08x)\n\r",

		 readl(vp_base + S5P_PP_CSC_Y2Y_COEF),
		 readl(vp_base + S5P_PP_CSC_CB2Y_COEF),
		 readl(vp_base + S5P_PP_CSC_CR2Y_COEF),
		 readl(vp_base + S5P_PP_CSC_Y2CB_COEF),
		 readl(vp_base + S5P_PP_CSC_CB2CB_COEF),
		 readl(vp_base + S5P_PP_CSC_CR2CB_COEF),
		 readl(vp_base + S5P_PP_CSC_Y2CR_COEF),
		 readl(vp_base + S5P_PP_CSC_CB2CR_COEF),
		 readl(vp_base + S5P_PP_CSC_CR2CR_COEF));

	return VPROC_NO_ERROR;
}

/*
* start  - start functions are only called under stopping video processor
*/
enum s5p_tv_vp_err __s5p_vp_start(void)
{
	enum s5p_tv_vp_err error = VPROC_NO_ERROR;

	VPPRINTK("()\n\r");

	writel(VP_ON_ENABLE, vp_base + S5P_VP_ENABLE);

	error = __s5p_vp_update();

	VPPRINTK("()\n\r");
	return error;
}

/*
* stop  - stop functions are only called under running video processor
*/
enum s5p_tv_vp_err __s5p_vp_stop(void)
{
	enum s5p_tv_vp_err error = VPROC_NO_ERROR;

	VPPRINTK("()\n\r");

	writel((readl(vp_base + S5P_VP_ENABLE) & ~VP_ON_ENABLE),
	       vp_base + S5P_VP_ENABLE);

	error = __s5p_vp_update();

	while (!(readl(vp_base + S5P_VP_ENABLE) & VP_POWER_DOWN_RDY))
		msleep(1);


	return error;
}

/*
* reset  - reset function
*/
void __s5p_vp_sw_reset(void)
{
	VPPRINTK("()\n\r");

	writel((readl(vp_base + S5P_VP_SRESET) | VP_SOFT_RESET),
	       vp_base + S5P_VP_SRESET);

	while (readl(vp_base + S5P_VP_SRESET) & VP_SOFT_RESET)
		msleep(10);


	VPPRINTK("()\n\r");
}

int __init __s5p_vp_probe(struct platform_device *pdev, u32 res_num)
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

	vp_mem = request_mem_region(res->start, size, pdev->name);

	if (vp_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		ret = -ENOENT;

	}

	vp_base = ioremap(res->start, size);

	if (vp_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		ret = -ENOENT;


	}

	return ret;

}

int __init __s5p_vp_release(struct platform_device *pdev)
{
	iounmap(vp_base);

	/* remove memory region */

	if (vp_mem != NULL) {
		if (release_resource(vp_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(vp_mem);

		vp_mem = NULL;
	}

	return 0;
}

