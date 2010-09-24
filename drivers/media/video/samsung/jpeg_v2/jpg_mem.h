/* linux/drivers/media/video/samsung/jpeg_v2/jpg_mem.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Definition for Operation of Jpeg encoder/docoder with memory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __JPG_MEM_H__
#define __JPG_MEM_H__

#include "jpg_misc.h"

#include <linux/version.h>
#include <plat/media.h>
#include <mach/media.h>

#define JPG_REG_BASE_ADDR	(0xFB600000)

#define jpg_data_base_addr	\
	(unsigned int)s5p_get_media_memory_bank(S5P_MDEV_JPEG, 0)
#define jpg_reserved_mem_size		\
	((unsigned int)s5p_get_media_memsize_bank(S5P_MDEV_JPEG, 0))

#define COEF1_RGB_2_YUV         0x4d971e
#define COEF2_RGB_2_YUV         0x2c5783
#define COEF3_RGB_2_YUV         0x836e13

/*
 * JPEG HW Register Macro Definition
 */
#define JPG_1BIT_MASK           1
#define JPG_4BIT_MASK           0xF

/* SubSampling_Mode Mask is JPGMOD Register [2:0] bits mask */
#define JPG_SMPL_MODE_MASK	0x07

/* Restart Interval value in JPGDRI Register is 2*/
#define JPG_RESTART_INTRAVEL    2

/* HCLK_JPEG is CLK_GATE_D1_1 Register 5th bit */
#define JPG_HCLK_JPEG_BIT       5
/* SubSampling_Mode is JPGMOD Register 0th bit */
#define JPG_SMPL_MODE_BIT       0
/* Quantization Table #1 is JPGQHNO Register 8th bit */
#define JPG_QUANT_TABLE1_BIT    8
/* Quantization Table #2 is JPGQHNO Register 10th bit */
#define JPG_QUANT_TABLE2_BIT    10
/* Quantization Table #3 is JPGQHNO Register 12th bit */
#define JPG_QUANT_TABLE3_BIT    12
/* Mode Sel is JPGCMOD Register 5th bit */
#define JPG_MODE_SEL_BIT        5

#define JPG_DECODE              (0x1 << 3)
#define JPG_ENCODE              (0x0 << 3)

#define JPG_RESERVE_ZERO        (0b000 << 2)

#define ENABLE_MOTION_ENC       (0x1<<3)
#define DISABLE_MOTION_ENC      (0x0<<3)

#define ENABLE_MOTION_DEC	(0x1<<0)
#define DISABLE_MOTION_DEC	(0x0<<0)

#define ENABLE_HW_DEC		(0x1<<2)
#define DISABLE_HW_DEC		(0x0<<2)

#define INCREMENTAL_DEC		(0x1<<3)
#define NORMAL_DEC		(0x0<<3)
#define YCBCR_MEMORY		(0x1<<5)

#define	ENABLE_IRQ		(0xf<<3)

struct jpegv2_limits {
	unsigned int max_main_width;
	unsigned int max_main_height;
	unsigned int max_thumb_width;
	unsigned int max_thumb_height;
};

struct jpegv2_buf {
	unsigned int main_stream_start;
	unsigned int main_stream_size;
	unsigned int main_frame_start;
	unsigned int main_frame_size;
	unsigned int thumb_stream_start;
	unsigned int thumb_stream_size;
	unsigned int thumb_frame_start;
	unsigned int thumb_frame_size;
	unsigned int total_buf_size;
};

struct s5pc110_jpg_ctx {
	unsigned int		jpg_data_addr;
	unsigned int		img_data_addr;
	unsigned int		jpg_thumb_data_addr;
	unsigned int		img_thumb_data_addr;
	int			caller_process;
	struct jpegv2_limits	*limits;
	struct jpegv2_buf	*bufinfo;
};

void *phy_to_vir_addr(unsigned int phy_addr, int mem_size);
void *mem_move(void *dst, const void *src, unsigned int size);
void *mem_alloc(unsigned int size);
#endif

