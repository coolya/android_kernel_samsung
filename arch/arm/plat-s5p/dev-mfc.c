/* arch/arm/plat-s5p/dev-mfc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Device definition for MFC device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <mach/map.h>
#include <asm/irq.h>
#include <plat/mfc.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/media.h>
#include <mach/media.h>

static struct s3c_platform_mfc s3c_mfc_pdata = {
	.buf_phy_base[0] = 0,
	.buf_phy_base[1] = 0,
	.buf_phy_size[0] = 0,
	.buf_phy_size[1] = 0,
};

static struct resource s3c_mfc_resources[] = {
	[0] = {
		.start  = S5PV210_PA_MFC,
		.end    = S5PV210_PA_MFC + S5PV210_SZ_MFC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_MFC,
		.end    = IRQ_MFC,
		.flags  = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_mfc = {
	.name           = "s3c-mfc",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s3c_mfc_resources),
	.resource       = s3c_mfc_resources,
	.dev		= {
		.platform_data = &s3c_mfc_pdata,
	},
};

void __init s3c_mfc_set_platdata(struct s3c_platform_mfc *pd)
{
	s3c_mfc_pdata.buf_phy_base[0] =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_MFC, 0);
	s3c_mfc_pdata.buf_phy_size[0] =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_MFC, 0);
	s3c_mfc_pdata.buf_phy_base[1] =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_MFC, 1);
	s3c_mfc_pdata.buf_phy_size[1] =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_MFC, 1);
}

