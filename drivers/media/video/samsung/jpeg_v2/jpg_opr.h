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

extern void __iomem		*s3c_jpeg_base;
extern int			jpg_irq_reason;

/* debug macro */
#define JPG_DEBUG(fmt, ...)					\
	do {							\
		printk(KERN_DEBUG				\
			"%s: " fmt, __func__, ##__VA_ARGS__);	\
	} while (0)


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

enum jpg_return_status {
	JPG_FAIL,
	JPG_SUCCESS,
	OK_HD_PARSING,
	ERR_HD_PARSING,
	OK_ENC_OR_DEC,
	ERR_ENC_OR_DEC,
	ERR_UNKNOWN
};

enum image_type {
	JPG_RGB16,
	JPG_YCBYCR,
	JPG_TYPE_UNKNOWN
};

enum sample_mode {
	JPG_444,
	JPG_422,
	JPG_420,
	JPG_400,
	RESERVED1,
	RESERVED2,
	JPG_411,
	JPG_SAMPLE_UNKNOWN
};

enum out_mode {
	YCBCR_422,
	YCBCR_420,
	YCBCR_SAMPLE_UNKNOWN
};

enum in_mode {
	JPG_MODESEL_YCBCR = 1,
	JPG_MODESEL_RGB,
	JPG_MODESEL_UNKNOWN
};

enum encode_type {
	JPG_MAIN,
	JPG_THUMBNAIL
};

enum image_quality_type {
	JPG_QUALITY_LEVEL_1 = 0, /*high quality*/
	JPG_QUALITY_LEVEL_2,
	JPG_QUALITY_LEVEL_3,
	JPG_QUALITY_LEVEL_4     /*low quality*/
};

struct jpg_dec_proc_param {
	enum sample_mode	sample_mode;
	enum encode_type	dec_type;
	enum out_mode		out_format;
	unsigned int		width;
	unsigned int		height;
	unsigned int		data_size;
	unsigned int		file_size;
};

struct jpg_enc_proc_param {
	enum sample_mode	sample_mode;
	enum encode_type	enc_type;
	enum in_mode		in_format;
	enum image_quality_type	quality;
	unsigned int		width;
	unsigned int		height;
	unsigned int		data_size;
	unsigned int		file_size;
};

struct jpg_args {
	char			*in_buf;
	char			*phy_in_buf;
	int			in_buf_size;
	char			*out_buf;
	char			*phy_out_buf;
	int			out_buf_size;
	char			*in_thumb_buf;
	char			*phy_in_thumb_buf;
	int			in_thumb_buf_size;
	char			*out_thumb_buf;
	char			*phy_out_thumb_buf;
	int			out_thumb_buf_size;
	char			*mapped_addr;
	struct jpg_dec_proc_param	*dec_param;
	struct jpg_enc_proc_param	*enc_param;
	struct jpg_enc_proc_param	*thumb_enc_param;
};

void reset_jpg(struct s5pc110_jpg_ctx *jpg_ctx);
enum jpg_return_status decode_jpg(struct s5pc110_jpg_ctx *jpg_ctx, \
		struct jpg_dec_proc_param *dec_param);
enum jpg_return_status encode_jpg(struct s5pc110_jpg_ctx *jpg_ctx, \
		struct jpg_enc_proc_param *enc_param);
enum jpg_return_status wait_for_interrupt(void);
enum sample_mode get_sample_type(struct s5pc110_jpg_ctx *jpg_ctx);
void get_xy(struct s5pc110_jpg_ctx *jpg_ctx, unsigned int *x, unsigned int *y);
unsigned int get_yuv_size(enum out_mode out_format, \
		unsigned int width, unsigned int height);

#endif
