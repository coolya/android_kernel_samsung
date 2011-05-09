/*
 * Copyright (C) 2011 Samsung Electronics.
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
#ifndef _WIMAX_SDIO_H
#define _WIMAX_SDIO_H

#include "hw_types.h"
#include "ctl_types.h"

/* WiMAX Constants */
#define WIMAX_MTU_SIZE				1400
#define WIMAX_MAX_FRAMESIZE			1500
#define WIMAX_HEADER_SIZE			14
#define WIMAX_MAX_TOTAL_SIZE	(WIMAX_MAX_FRAMESIZE + WIMAX_HEADER_SIZE)
/* maximum allocated data size,  mtu 1400 so 3 blocks max 1536 */
#define BUFFER_DATA_SIZE			1600
#define ADAPTER_TIMEOUT				(HZ * 10)

#define MEDIA_DISCONNECTED			0
#define MEDIA_CONNECTED				1

/* network adapter structure */
struct net_adapter {
	struct sdio_func		*func;
	struct net_device		*net;
	struct net_device_stats		netstats;
	struct miscdevice		 uwibro_dev;

	struct task_struct		*tx_task;
	struct task_struct		*rx_task;
	struct task_struct		*mac_task;

	s32			wake_irq;

	u32			msg_enable;

	u32			XmitErr; /* packet send fails */

	struct hardware_info		hw;
	struct ctl_info			ctl;
	struct image_data		wimax_image;
	struct completion		wakeup_event;
	struct wimax732_platform_data	*pdata;
	wait_queue_head_t		download_event;
	wait_queue_head_t		receive_event;
	wait_queue_head_t		send_event;
	u8			downloading;	/* firmware downloading */
	u8			download_complete;
	u8			mac_ready;

	u8			media_state;/* mac completion */
	u8			prev_wimax_status;
	u8			rx_data_available;
	u8			halted;	/* device halt pending flag */
	u8			removed;
};

#endif	/* _WIMAX_SDIO_H */
