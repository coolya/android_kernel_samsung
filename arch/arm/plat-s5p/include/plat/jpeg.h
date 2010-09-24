/* linux/arch/arm/plat-s5p/include/plat/jpeg.h
 *
 * Copyright 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Platform header file for JPEG device driver driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __ASM_PLAT_JPEG_H
#define __ASM_PLAT_JPEG_H __FILE__

struct platform_device;

/* For camera interface driver */
struct s3c_platform_jpeg {
	unsigned int max_main_width;
	unsigned int max_main_height;
	unsigned int max_thumb_width;
	unsigned int max_thumb_height;
};

extern void s3c_jpeg_set_platdata(struct s3c_platform_jpeg *jpeg);

#endif /*__ASM_PLAT_FIMC_H */
