/* linux/drivers/media/video/samsung/jpeg_v2/jpg_opr.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Operation for Jpeg encoder/docoder
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/io.h>

#include "jpg_mem.h"
#include "jpg_misc.h"
#include "jpg_opr.h"
#include "jpg_conf.h"

#include "regs-jpeg.h"

enum {
	UNKNOWN,
	BASELINE = 0xC0,
	EXTENDED_SEQ = 0xC1,
	PROGRESSIVE = 0xC2
} jpg_sof_marker;

enum jpg_return_status wait_for_interrupt(void)
{
	if (interruptible_sleep_on_timeout(&wait_queue_jpeg,	\
					 INT_TIMEOUT) == 0) {
		jpg_err("waiting for interrupt is timeout\n");
	}

	return jpg_irq_reason;
}

enum jpg_return_status decode_jpg(struct s5pc110_jpg_ctx *jpg_ctx,
				  struct jpg_dec_proc_param *dec_param)
{
	int		ret;
	enum sample_mode sample_mode;
	unsigned int	width, height;
	jpg_dbg("enter decode_jpg function\n");

	if (jpg_ctx)
		reset_jpg(jpg_ctx);
	else {
		jpg_err("jpg ctx is NULL\n");
		return JPG_FAIL;
	}

/* set jpeg clock register : power on */
	writel(readl(s3c_jpeg_base + S3C_JPEG_CLKCON_REG) |
			(S3C_JPEG_CLKCON_REG_POWER_ON_ACTIVATE),
			s3c_jpeg_base + S3C_JPEG_CLKCON_REG);
	/* set jpeg mod register : decode */
	writel(readl(s3c_jpeg_base + S3C_JPEG_MOD_REG) |
			(S3C_JPEG_MOD_REG_PROC_DEC),
			s3c_jpeg_base + S3C_JPEG_MOD_REG);
	/* set jpeg interrupt setting register */
	writel(readl(s3c_jpeg_base + S3C_JPEG_INTSE_REG) |
			(S3C_JPEG_INTSE_REG_RSTM_INT_EN	|
			S3C_JPEG_INTSE_REG_DATA_NUM_INT_EN |
			S3C_JPEG_INTSE_REG_FINAL_MCU_NUM_INT_EN),
			s3c_jpeg_base + S3C_JPEG_INTSE_REG);
	/* set jpeg deocde ouput format register */
	writel(readl(s3c_jpeg_base + S3C_JPEG_OUTFORM_REG) &
			~(S3C_JPEG_OUTFORM_REG_YCBCY420),
			s3c_jpeg_base + S3C_JPEG_OUTFORM_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_OUTFORM_REG) |
			(dec_param->out_format << 0),
			s3c_jpeg_base + S3C_JPEG_OUTFORM_REG);

	/* set the address of compressed input data */
	writel(jpg_ctx->img_data_addr, s3c_jpeg_base + S3C_JPEG_IMGADR_REG);

	/* set the address of decompressed image */
	writel(jpg_ctx->jpg_data_addr, s3c_jpeg_base + S3C_JPEG_JPGADR_REG);

	/* start decoding */
	writel(readl(s3c_jpeg_base + S3C_JPEG_JRSTART_REG) |
			S3C_JPEG_JRSTART_REG_ENABLE,
			s3c_jpeg_base + S3C_JPEG_JSTART_REG);

	ret = wait_for_interrupt();

	if (ret != OK_ENC_OR_DEC) {
		jpg_err("jpg decode error(%d)\n", ret);
		return JPG_FAIL;
	}

	sample_mode = get_sample_type(jpg_ctx);
	jpg_dbg("sample_mode : %d\n", sample_mode);

	if (sample_mode == JPG_SAMPLE_UNKNOWN) {
		jpg_err("jpg has invalid sample_mode\r\n");
		return JPG_FAIL;
	}

	dec_param->sample_mode = sample_mode;

	get_xy(jpg_ctx, &width, &height);
	jpg_dbg("decode size:: width : %d height : %d\n", width, height);

	dec_param->data_size = get_yuv_size(dec_param->out_format,
					    width, height);
	dec_param->width = width;
	dec_param->height = height;

	return JPG_SUCCESS;
}

void reset_jpg(struct s5pc110_jpg_ctx *jpg_ctx)
{
	jpg_dbg("s3c_jpeg_base %p\n", s3c_jpeg_base);
	writel(S3C_JPEG_SW_RESET_REG_ENABLE,
			s3c_jpeg_base + S3C_JPEG_SW_RESET_REG);

	do {
		writel(S3C_JPEG_SW_RESET_REG_ENABLE,
				s3c_jpeg_base + S3C_JPEG_SW_RESET_REG);
	} while (((readl(s3c_jpeg_base + S3C_JPEG_SW_RESET_REG))
		   & S3C_JPEG_SW_RESET_REG_ENABLE)
		   == S3C_JPEG_SW_RESET_REG_ENABLE);
}

enum sample_mode get_sample_type(struct s5pc110_jpg_ctx *jpg_ctx)
{
	unsigned long		jpgMode;
	enum sample_mode	sample_mode = JPG_SAMPLE_UNKNOWN;

	jpgMode = readl(s3c_jpeg_base + S3C_JPEG_MOD_REG);

	sample_mode =
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_444) ? JPG_444 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_422) ? JPG_422 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_420) ? JPG_420 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_400) ? JPG_400 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_411) ? JPG_411 :
						    JPG_SAMPLE_UNKNOWN;

	return sample_mode;
}

void get_xy(struct s5pc110_jpg_ctx *jpg_ctx, unsigned int *x, unsigned int *y)
{
	*x = (readl(s3c_jpeg_base + S3C_JPEG_X_U_REG)<<8)|
		readl(s3c_jpeg_base + S3C_JPEG_X_L_REG);
	*y = (readl(s3c_jpeg_base + S3C_JPEG_Y_U_REG)<<8)|
		readl(s3c_jpeg_base + S3C_JPEG_Y_L_REG);
}

unsigned int get_yuv_size(enum out_mode out_format,
			  unsigned int width, unsigned int height)
{
	switch (out_format) {
	case YCBCR_422:

		if (width % 16 != 0)
			width += 16 - (width % 16);

		if (height % 8 != 0)
			height += 8 - (height % 8);

		break;

	case YCBCR_420:

		if (width % 16 != 0)
			width += 16 - (width % 16);

		if (height % 16 != 0)
			height += 16 - (height % 16);

		break;

	case YCBCR_SAMPLE_UNKNOWN:
		break;
	}

	jpg_dbg("get_yuv_size width(%d) height(%d)\n", width, height);

	switch (out_format) {
	case YCBCR_422:
		return width * height * 2;
	case YCBCR_420:
		return (width * height) + (width * height >> 1);
	default:
		return 0;
	}
}

enum jpg_return_status encode_jpg(struct s5pc110_jpg_ctx *jpg_ctx,
				  struct jpg_enc_proc_param *enc_param)
{

	unsigned int	i, ret;
	unsigned int	cmd_val;

	if (enc_param->width <= 0
			|| enc_param->width > jpg_ctx->limits->max_main_width
			|| enc_param->height <= 0
			|| enc_param->height > jpg_ctx->limits->max_main_height) {
		jpg_err("::encoder : width: %d, height: %d\n",
				enc_param->width, enc_param->height);
		jpg_err("::encoder : invalid width/height\n");
		return JPG_FAIL;
	}

	/* SW reset */
	if (jpg_ctx)
		reset_jpg(jpg_ctx);
	else {
		jpg_err("::jpg ctx is NULL\n");
		return JPG_FAIL;
	}
	/* set jpeg clock register : power on */
	writel(readl(s3c_jpeg_base + S3C_JPEG_CLKCON_REG) |
			(S3C_JPEG_CLKCON_REG_POWER_ON_ACTIVATE),
			s3c_jpeg_base + S3C_JPEG_CLKCON_REG);
	/* set jpeg mod register : encode */
	writel(readl(s3c_jpeg_base + S3C_JPEG_CMOD_REG) |
			(enc_param->in_format << JPG_MODE_SEL_BIT),
			s3c_jpeg_base + S3C_JPEG_CMOD_REG);
	cmd_val = (enc_param->sample_mode == JPG_422) ?
			(S3C_JPEG_MOD_REG_SUBSAMPLE_422) :
			(S3C_JPEG_MOD_REG_SUBSAMPLE_420);

	writel(cmd_val | S3C_JPEG_MOD_REG_PROC_ENC,
			 s3c_jpeg_base + S3C_JPEG_MOD_REG);

	/* set DRI(Define Restart Interval) */
	writel(JPG_RESTART_INTRAVEL, s3c_jpeg_base + S3C_JPEG_DRI_L_REG);
	writel((JPG_RESTART_INTRAVEL>>8), s3c_jpeg_base + S3C_JPEG_DRI_U_REG);

	writel(S3C_JPEG_QHTBL_REG_QT_NUM1, s3c_jpeg_base + S3C_JPEG_QTBL_REG);
	writel(0x00, s3c_jpeg_base + S3C_JPEG_HTBL_REG);

	/* Horizontal resolution */
	writel((enc_param->width>>8), s3c_jpeg_base + S3C_JPEG_X_U_REG);
	writel(enc_param->width, s3c_jpeg_base + S3C_JPEG_X_L_REG);

	/* Vertical resolution */
	writel((enc_param->height>>8), s3c_jpeg_base + S3C_JPEG_Y_U_REG);
	writel(enc_param->height, s3c_jpeg_base + S3C_JPEG_Y_L_REG);

	jpg_dbg("enc_param->enc_type : %d\n", enc_param->enc_type);

	if (enc_param->enc_type == JPG_MAIN) {
		jpg_dbg("encode image size width: %d, height: %d\n",
				enc_param->width, enc_param->height);
		writel(jpg_ctx->img_data_addr,
			s3c_jpeg_base + S3C_JPEG_IMGADR_REG);
		writel(jpg_ctx->jpg_data_addr,
			s3c_jpeg_base + S3C_JPEG_JPGADR_REG);
	} else { /* thumbnail encoding */
		jpg_dbg("thumb image size width: %d, height: %d\n",
				enc_param->width, enc_param->height);
		writel(jpg_ctx->img_thumb_data_addr,
			s3c_jpeg_base + S3C_JPEG_IMGADR_REG);
		writel(jpg_ctx->jpg_thumb_data_addr,
			s3c_jpeg_base + S3C_JPEG_JPGADR_REG);
	}

	/*  Coefficient value 1~3 for RGB to YCbCr */
	writel(COEF1_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF1_REG);
	writel(COEF2_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF2_REG);
	writel(COEF3_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF3_REG);

	/* Quantiazation and Huffman Table setting */
	for (i = 0; i < 64; i++) {
		writel((unsigned int)qtbl_luminance[enc_param->quality][i],
			s3c_jpeg_base + S3C_JPEG_QTBL0_REG + (i*0x04));
	}
	for (i = 0; i < 64; i++) {
		writel((unsigned int)qtbl_chrominance[enc_param->quality][i],
			s3c_jpeg_base + S3C_JPEG_QTBL1_REG + (i*0x04));
	}
	for (i = 0; i < 16; i++) {
		writel((unsigned int)hdctbl0[i],
			s3c_jpeg_base + S3C_JPEG_HDCTBL0_REG + (i*0x04));
	}
	for (i = 0; i < 12; i++) {
		writel((unsigned int)hdctblg0[i],
			s3c_jpeg_base + S3C_JPEG_HDCTBLG0_REG + (i*0x04));
	}
	for (i = 0; i < 16; i++) {
		writel((unsigned int)hactbl0[i],
			s3c_jpeg_base + S3C_JPEG_HACTBL0_REG + (i*0x04));
	}
	for (i = 0; i < 162; i++) {
		writel((unsigned int)hactblg0[i],
			s3c_jpeg_base + S3C_JPEG_HACTBLG0_REG + (i*0x04));
	}
	writel(readl(s3c_jpeg_base + S3C_JPEG_INTSE_REG) |
			(S3C_JPEG_INTSE_REG_RSTM_INT_EN	|
			S3C_JPEG_INTSE_REG_DATA_NUM_INT_EN |
			S3C_JPEG_INTSE_REG_FINAL_MCU_NUM_INT_EN),
			s3c_jpeg_base + S3C_JPEG_INTSE_REG);

	writel(readl(s3c_jpeg_base + S3C_JPEG_JSTART_REG) |
			S3C_JPEG_JSTART_REG_ENABLE,
			s3c_jpeg_base + S3C_JPEG_JSTART_REG);
	ret = wait_for_interrupt();

	if (ret != OK_ENC_OR_DEC) {
		jpg_err("jpeg encoding error(%d)\n", ret);
		return JPG_FAIL;
	}

	enc_param->file_size = readl(s3c_jpeg_base + S3C_JPEG_CNT_U_REG) << 16;
	enc_param->file_size |= readl(s3c_jpeg_base + S3C_JPEG_CNT_M_REG) << 8;
	enc_param->file_size |= readl(s3c_jpeg_base + S3C_JPEG_CNT_L_REG);

	return JPG_SUCCESS;

}
