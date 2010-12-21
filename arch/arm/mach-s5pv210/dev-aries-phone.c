/* linux/arch/arm/mach-s5pv210/dev-herring-phone.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>

#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/gpio-herring.h>

#include "../../../drivers/misc/samsung_modemctl/modem_ctl.h"

/* Modem control */
static struct modemctl_data mdmctl_data = {
	.name = "xmm",
	.gpio_phone_active = GPIO_PHONE_ACTIVE,
	.gpio_pda_active = GPIO_PDA_ACTIVE,
	.gpio_cp_reset = GPIO_CP_RST,
};

static struct resource mdmctl_res[] = {
	[0] = {
		.name = "active",
		.start = IRQ_EINT15,
		.end = IRQ_EINT15,
		.flags = IORESOURCE_IRQ,
	},
	[1] = {
		.name = "onedram",
		.start = IRQ_EINT11,
		.end = IRQ_EINT11,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.name = "onedram",
		.start = (S5PV210_PA_SDRAM + 0x05000000),
		.end = (S5PV210_PA_SDRAM + 0x05000000 + SZ_16M - 1),
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device modemctl = {
	.name = "modemctl",
	.id = -1,
	.num_resources = ARRAY_SIZE(mdmctl_res),
	.resource = mdmctl_res,
	.dev = {
		.platform_data = &mdmctl_data,
	},
};

static int __init herring_init_phone_interface(void)
{
	platform_device_register(&modemctl);
	return 0;
}
device_initcall(herring_init_phone_interface);
