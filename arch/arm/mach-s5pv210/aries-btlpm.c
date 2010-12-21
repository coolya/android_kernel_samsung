/* linux/arch/arm/mach-s5pv210/aries-btlpm.c
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

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>

#include <asm/mach-types.h>

#include <mach/gpio-aries.h>

#include "aries.h"

static struct aries_bt_lpm {
	struct hrtimer bt_lpm_timer;
	ktime_t bt_lpm_delay;
} bt_lpm;

static enum hrtimer_restart bt_enter_lpm(struct hrtimer *timer)
{
	gpio_set_value(GPIO_BT_WAKE, 0);

	return HRTIMER_NORESTART;
}

void aries_bt_uart_wake_peer(struct uart_port *port)
{
	if (!bt_lpm.bt_lpm_timer.function)
		return;

	hrtimer_try_to_cancel(&bt_lpm.bt_lpm_timer);
	gpio_set_value(GPIO_BT_WAKE, 1);
	hrtimer_start(&bt_lpm.bt_lpm_timer, bt_lpm.bt_lpm_delay, HRTIMER_MODE_REL);
}

static int __init bt_lpm_init(void)
{
	int ret;

	if (!machine_is_aries())
		return 0;

	ret = gpio_request(GPIO_BT_WAKE, "gpio_bt_wake");
	if (ret) {
		printk(KERN_ERR "Failed to request gpio_bt_wake control\n");
		return 0;
	}

	gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

	hrtimer_init(&bt_lpm.bt_lpm_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	bt_lpm.bt_lpm_delay = ktime_set(1, 0);	/* 1 sec */
	bt_lpm.bt_lpm_timer.function = bt_enter_lpm;
	return 0;
}
device_initcall(bt_lpm_init);
