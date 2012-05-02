/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
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
#include <linux/miscdevice.h>

#ifndef __WIMAX_CMC732_H
#define __WIMAX_CMC732_H

#ifdef __KERNEL__

#define WIMAX_POWER_SUCCESS		0
#define WIMAX_ALREADY_POWER_ON		-1
#define WIMAX_PROBE_FAIL		-2
#define WIMAX_ALREADY_POWER_OFF		-3

/* wimax mode */
enum {
	SDIO_MODE = 0,
	WTM_MODE,
	MAC_IMEI_WRITE_MODE,
	USIM_RELAY_MODE,
	DM_MODE,
	USB_MODE,
	AUTH_MODE
};

/* wimax state */
enum {
	WIMAX_STATE_NOT_READY,
	WIMAX_STATE_READY,
	WIMAX_STATE_VIRTUAL_IDLE,
	WIMAX_STATE_NORMAL,
	WIMAX_STATE_IDLE,
	WIMAX_STATE_RESET_REQUESTED,
	WIMAX_STATE_RESET_ACKED,
	WIMAX_STATE_AWAKE_REQUESTED,
};

struct wimax_cfg {
	int			temp_tgid;	/* handles unexpected close */
	struct wake_lock	wimax_wake_lock;	/* resume wake lock */
	struct wake_lock	wimax_rxtx_lock;/* sdio wake lock */
	struct wake_lock	wimax_tx_lock;/* sdio tx lock */
	struct mutex suspend_mutex;
	u8		wimax_status;
	u8		wimax_mode;/* wimax mode (SDIO, USB, etc..) */
	u8		sleep_mode;/* suspend mode (0: VI, 1: IDLE) */
	u8		card_removed;/*
						 * set if host has acknowledged
						 * card removal
						 */
};

struct wimax732_platform_data {
	int (*power) (int);
	void (*set_mode) (void);
	void (*signal_ap_active) (int);
	int (*get_sleep_mode) (void);
	int (*is_modem_awake) (void);
	void (*wakeup_assert) (int);
	struct wimax_cfg *g_cfg;
	struct miscdevice swmxctl_dev;
	int wimax_int;
};

#endif

#endif
