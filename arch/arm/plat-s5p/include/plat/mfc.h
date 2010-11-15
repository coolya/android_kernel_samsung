/* arch/arm/plat-s5p/include/plat/mfc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com
 *
 * Platform header file for MFC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C_MFC_H
#define _S3C_MFC_H

#include <linux/types.h>

struct s3c_platform_mfc {
	dma_addr_t buf_phy_base[2];
	size_t buf_phy_size[2];
};

extern void s3c_mfc_set_platdata(struct s3c_platform_mfc *pd);
#endif
