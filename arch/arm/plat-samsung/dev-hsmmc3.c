/* linux/arch/arm/plat-samsung/dev-hsmmc3.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Based on dev-hsmmc.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mmc/host.h>

#include <mach/map.h>
#include <plat/sdhci.h>
#include <plat/devs.h>
#include <plat/cpu.h>

#define S3C_SZ_HSMMC	(0x1000)

static struct resource s3c_hsmmc3_resource[] = {
	[0] = {
		.start = S3C_PA_HSMMC3,
		.end   = S3C_PA_HSMMC3 + S3C_SZ_HSMMC - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MMC3,
		.end   = IRQ_MMC3,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_hsmmc3_dmamask = 0xffffffffUL;

struct s3c_sdhci_platdata s3c_hsmmc3_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA |
			   MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
};

struct platform_device s3c_device_hsmmc3 = {
	.name		= "s3c-sdhci",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(s3c_hsmmc3_resource),
	.resource	= s3c_hsmmc3_resource,
	.dev		= {
		.dma_mask		= &s3c_device_hsmmc3_dmamask,
		.coherent_dma_mask	= 0xffffffffUL,
		.platform_data		= &s3c_hsmmc3_def_platdata,
	},
};

void s3c_sdhci3_set_platdata(struct s3c_sdhci_platdata *pd)
{
	struct s3c_sdhci_platdata *set = &s3c_hsmmc3_def_platdata;

	if (pd->max_width)
		set->max_width = pd->max_width;
	if (pd->host_caps)
		set->host_caps |= pd->host_caps;
	if (pd->cfg_gpio)
		set->cfg_gpio = pd->cfg_gpio;
	if (pd->cfg_card)
		set->cfg_card = pd->cfg_card;
	if (pd->cfg_ext_cd)
		set->cfg_ext_cd = pd->cfg_ext_cd;
	if (pd->ext_cd)
		set->ext_cd = pd->ext_cd;
	if (pd->cfg_wp)
		set->cfg_wp = pd->cfg_wp;
	if (pd->get_ro)
		set->get_ro = pd->get_ro;
	if (pd->detect_ext_cd)
		set->detect_ext_cd = pd->detect_ext_cd;
        if (pd->built_in)
                set->built_in = pd->built_in;
}
