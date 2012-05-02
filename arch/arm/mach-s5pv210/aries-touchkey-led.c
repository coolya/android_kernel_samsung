/*
 * Copyright (C) 2010 Samsung Electronics Co. Ltd. All Rights Reserved.
 * Author: Rom Lemarchand <rlemarchand@sta.samsung.com>
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

#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <asm/mach-types.h>

static int led_gpios[] = { 2, 3, 6, 7 };

static void aries_touchkey_led_onoff(int onoff)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(led_gpios); i++)
		gpio_direction_output(S5PV210_GPJ3(led_gpios[i]), !!onoff);
}

static void aries_touchkey_led_early_suspend(struct early_suspend *h)
{
	aries_touchkey_led_onoff(0);
}

static void aries_touchkey_led_late_resume(struct early_suspend *h)
{
	aries_touchkey_led_onoff(1);
}

static struct early_suspend early_suspend = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = aries_touchkey_led_early_suspend,
	.resume = aries_touchkey_led_late_resume,
};

static int __init aries_init_touchkey_led(void)
{
	int i;
	int ret = 0;

	if (!machine_is_aries() || system_rev < 0x10)
		return 0;

	for (i = 0; i < ARRAY_SIZE(led_gpios); i++) {
		ret = gpio_request(S5PV210_GPJ3(led_gpios[i]), "touchkey led");
		if (ret) {
			pr_err("Failed to request touchkey led gpio %d\n", i);
			goto err_req;
		}
		s3c_gpio_setpull(S5PV210_GPJ3(led_gpios[i]),
							S3C_GPIO_PULL_NONE);
	}

	aries_touchkey_led_onoff(1);

	register_early_suspend(&early_suspend);

	return 0;

err_req:
	while (--i >= 0)
		gpio_free(S5PV210_GPJ3(led_gpios[i]));
	return ret;
}

device_initcall(aries_init_touchkey_led);
