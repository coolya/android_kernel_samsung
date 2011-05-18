/**
 * header for modem control
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __MODEM_CONTROL_H__
#define __MODEM_CONTROL_H__

struct modemctl_platform_data {
	const char *name;

	unsigned gpio_phone_on;
	unsigned gpio_phone_active;
	unsigned gpio_pda_active;
	unsigned gpio_cp_reset;
	unsigned gpio_reset_req_n;
	unsigned gpio_usim_boot;
	unsigned gpio_flm_sel;
	unsigned gpio_sim_ndetect;

	void (*cfg_gpio)(void);
};

#endif /* __MODEM_CONTROL_H__ */
