/*
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_CONTROL_H__
#define __MODEM_CONTROL_H__

#define IOCTL_MODEM_RAMDUMP             _IO('o', 0x19)
#define IOCTL_MODEM_RESET               _IO('o', 0x20)
#define IOCTL_MODEM_START               _IO('o', 0x21)
#define IOCTL_MODEM_OFF                 _IO('o', 0x22)

#define IOCTL_MODEM_SEND		_IO('o', 0x23)
#define IOCTL_MODEM_RECV		_IO('o', 0x24)

struct modem_io {
	uint32_t size;
	uint32_t id;
	uint32_t cmd;
	void *data;
};

/* platform data */
struct modemctl_data {
	const char *name;
	unsigned gpio_phone_active;
	unsigned gpio_pda_active;
	unsigned gpio_cp_reset;
	unsigned gpio_phone_on;
	bool is_cdma_modem; /* 1:CDMA Modem */
};

#endif
