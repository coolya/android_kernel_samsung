/* linux/drivers/media/video/samsung/jpeg_v2/jpg_opr.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Definition for Operation of Jpeg encoder/docoder
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __JPG_OPR_H__
#define __JPG_OPR_H__

#include <linux/interrupt.h>


/* debug macro */
#define JPG_DEBUG(fmt, ...)					\
	do {							\
		printk(KERN_DEBUG 				\
			"%s: " fmt, __func__, ##__VA_ARGS__);	\
	} while(0)


#define JPG_WARN(fmt, ...)					\
	do {							\
		printk(KERN_WARNING				\
			fmt, ##__VA_ARGS__);			\
	} while (0)



#define JPG_ERROR(fmt, ...)					\
	do {							\
		printk(KERN_ERR					\
			"%s: " fmt, __func__, ##__VA_ARGS__);	\
	} while (0)



#ifdef CONFIG_VIDEO_JPEG_DEBUG
#define jpg_dbg(fmt, ...)		JPG_DEBUG(fmt, ##__VA_ARGS__)
#else
#define jpg_dbg(fmt, ...)
#endif

#define jpg_warn(fmt, ...)		JPG_WARN(fmt, ##__VA_ARGS__)
#define jpg_err(fmt, ...)		JPG_ERROR(fmt, ##__VA_ARGS__)

extern wait_queue_head_t wait_queue_jpeg;

typedef enum {
	JPG_FAIL,
	JPG_SUCCESS,
	OK_HD_PARSING,
	ERR_HD_PARSING,
	OK_ENC_OR_DEC,
	ERR_ENC_OR_DEC,
	ERR_UNKNOWN
} jpg_return_status;

typedef enum {
	JPG_RGB16,
	JPG_YCBYCR,
	JPG_TYPE_UNKNOWN
} image_type_t;

typedef enum {
	JPG_444,
	JPG_422,
	JPG_420,
	JPG_400,
	RESERVED1,
	RESERVED2,
	JPG_411,
	JPG_SAMPLE_UNKNOWN
} sample_mode_t;

typedef enum {
	YCBCR_422,
	YCBCR_420,
	YCBCR_SAMPLE_UNKNOWN
} out_mode_t;

typedef enum {
	JPG_MODESEL_YCBCR = 1,
	JPG_MODESEL_RGB,
	JPG_MODESEL_UNKNOWN
} in_mode_t;

typedef enum {
	JPG_MAIN,
	JPG_THUMBNAIL
} encode_type_t;

typedef enum {
	JPG_QUALITY_LEVEL_1 = 0, /*high quality*/
	JPG_QUALITY_LEVEL_2,
	JPG_QUALITY_LEVEL_3,
	JPG_QUALITY_LEVEL_4     /*low quality*/
} image_quality_type_t;

typedef struct {
	sample_mode_t		sample_mode;
	encode_type_t		dec_type;
	out_mode_t       	out_format;
	UINT32			width;
	UINT32			height;
	UINT32			data_size;
	UINT32			file_size;
} jpg_dec_proc_param;

typedef struct {
	sample_mode_t		sample_mode;
	encode_type_t		enc_type;
	in_mode_t      		in_format;
	image_quality_type_t 	quality;
	UINT32			width;
	UINT32			height;
	UINT32			data_size;
	UINT32			file_size;
} jpg_enc_proc_param;

typedef struct {
	char			*in_buf;
	char			*phy_in_buf;
	int			in_buf_size;
	char			*out_buf;
	char    		*phy_out_buf;
	int			out_buf_size;
	char			*in_thumb_buf;
	char			*phy_in_thumb_buf;
	int			in_thumb_buf_size;
	char			*out_thumb_buf;
	char			*phy_out_thumb_buf;
	int			out_thumb_buf_size;
	char			*mapped_addr;
	jpg_dec_proc_param	*dec_param;
	jpg_enc_proc_param	*enc_param;
	jpg_enc_proc_param	*thumb_enc_param;
} jpg_args;

void reset_jpg(sspc100_jpg_ctx *jpg_ctx);
jpg_return_status decode_jpg(sspc100_jpg_ctx *jpg_ctx, jpg_dec_proc_param *dec_param);
jpg_return_status encode_jpg(sspc100_jpg_ctx *jpg_ctx, jpg_enc_proc_param *enc_param);
jpg_return_status wait_for_interrupt(void);
sample_mode_t get_sample_type(sspc100_jpg_ctx *jpg_ctx);
void get_xy(sspc100_jpg_ctx *jpg_ctx, UINT32 *x, UINT32 *y);
UINT32 get_yuv_size(out_mode_t out_format, UINT32 width, UINT32 height);

#endif
