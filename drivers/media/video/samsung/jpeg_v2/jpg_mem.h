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

#define JPG_REG_BASE_ADDR    	(0xFB600000)

#define jpg_data_base_addr	(UINT32)s3c_get_media_memory_bank(S3C_MDEV_JPEG, 0)

#define MAX_JPG_WIDTH       	2560
#define MAX_JPG_HEIGHT       	1920
#define MAX_JPG_RESOLUTION	(MAX_JPG_WIDTH * MAX_JPG_HEIGHT)

/* It assumes that JPG thumbnail is not used 
 * Set the resolution when you need to use */
#define MAX_JPG_THUMBNAIL_WIDTH	 0
#define MAX_JPG_THUMBNAIL_HEIGHT 0
#define MAX_JPG_THUMBNAIL_RESOLUTION (MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT)

/* It assumes that RGB data encoding is not used
 * Set the resolution when you need to use */
#define MAX_RGB_WIDTH        0
#define MAX_RGB_HEIGHT       0
#define MAX_RGB_RESOLUTION	(MAX_RGB_WIDTH * MAX_RGB_HEIGHT)

/*******************************************************************************/
/* define JPG & image memory */
/* memory area is 4k(PAGE_SIZE) aligned because of VirtualCopyEx() */

/* Below definitions assume that JPEG_V2 uses only decoding without thumbnail.
 * Max jpeg size of camera input is 3M, and decoded output data is YUV422.
 * To reduce unnecessary memory reservation, 
 * useless definitions are set to 0. */
#define JPG_STREAM_BUF_SIZE		3*1024*1024
#define JPG_STREAM_THUMB_BUF_SIZE 	0
#define JPG_FRAME_BUF_SIZE		\
		((MAX_JPG_RESOLUTION * 2) / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_FRAME_THUMB_BUF_SIZE	0
#define JPG_RGB_BUF_SIZE		0

/* Use below definitions if you need to use encoding and thumbnail 
 * Begin of definitions
#define JPG_STREAM_BUF_SIZE		\
		(MAX_JPG_RESOLUTION / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_STREAM_THUMB_BUF_SIZE 	\
		(MAX_JPG_THUMBNAIL_RESOLUTION / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_FRAME_BUF_SIZE		\
		((MAX_JPG_RESOLUTION * 3) / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_FRAME_THUMB_BUF_SIZE	\
		((MAX_JPG_THUMBNAIL_RESOLUTION * 3) / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_RGB_BUF_SIZE		\
		((MAX_RGB_RESOLUTION * 4) / PAGE_SIZE + 1) * PAGE_SIZE
end of definitions */


#define JPG_TOTAL_BUF_SIZE	(JPG_STREAM_BUF_SIZE + \
				JPG_STREAM_THUMB_BUF_SIZE + \
				JPG_FRAME_BUF_SIZE + \
				JPG_FRAME_THUMB_BUF_SIZE + \
				JPG_RGB_BUF_SIZE)

#define JPG_MAIN_STRART		0x00
#define JPG_THUMB_START		JPG_STREAM_BUF_SIZE
#define IMG_MAIN_START		(JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE)
#define IMG_THUMB_START		(IMG_MAIN_START + JPG_FRAME_BUF_SIZE)

/*******************************************************************************/
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

#define ENABLE_MOTION_DEC       (0x1<<0)
#define DISABLE_MOTION_DEC      (0x0<<0)

#define ENABLE_HW_DEC           (0x1<<2)
#define DISABLE_HW_DEC          (0x0<<2)

#define INCREMENTAL_DEC         (0x1<<3)
#define NORMAL_DEC              (0x0<<3)
#define YCBCR_MEMORY			(0x1<<5)

#define	ENABLE_IRQ				(0xf<<3)

typedef struct __s5pc100_jpg_ctx {
	volatile UINT32                  jpg_data_addr;
	volatile UINT32                  img_data_addr;
	volatile UINT32                  jpg_thumb_data_addr;
	volatile UINT32                  img_thumb_data_addr;
	int                          caller_process;
} sspc100_jpg_ctx;

void *phy_to_vir_addr(UINT32 phy_addr, int mem_size);
void *mem_move(void *dst, const void *src, unsigned int size);
void *mem_alloc(unsigned int size);
#endif

