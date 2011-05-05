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
#ifndef _WIMAX_HEADERS_H
#define _WIMAX_HEADERS_H

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/proc_fs.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <asm/byteorder.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/wimax/samsung/wimax732.h>

#include "wimax_sdio.h"

#define WIMAX_IMAGE_PATH	"wimaxfw.bin"
#define WIMAX_LOADER_PATH	"wimaxloader.bin"

#define STATUS_SUCCESS			((u_long)0x00000000L)
/* The operation that was requested is pending completion */
#define STATUS_PENDING			((u_long)0x00000103L)
#define STATUS_RESOURCES		((u_long)0x00001003L)
#define STATUS_RESET_IN_PROGRESS	((u_long)0xc001000dL)
#define STATUS_DEVICE_FAILED		((u_long)0xc0010008L)
#define STATUS_NOT_ACCEPTED		((u_long)0x00010003L)
#define STATUS_FAILURE			((u_long)0xC0000001L)
/* The requested operation was unsuccessful */
#define STATUS_UNSUCCESSFUL		((u_long)0xC0000002L)
#define STATUS_CANCELLED		((u_long)0xC0000003L)

/* control.c functions */
u32 control_send(struct net_adapter *adapter, void *buffer, u32 length);
void control_recv(struct net_adapter   *adapter, void *buffer, u32 length);
u32 control_init(struct net_adapter *adapter);
void control_remove(struct net_adapter *adapter);

struct process_descriptor *process_by_id(struct net_adapter *adapter,
		u32 id);
struct process_descriptor *process_by_type(struct net_adapter *adapter,
		u16 type);
void remove_process(struct net_adapter *adapter, u32 id);

u32 buffer_count(struct list_head ListHead);
struct buffer_descriptor *buffer_by_type(struct net_adapter *adapter,
		u16 type);

/* hardware.c functions */

u32 hw_send_data(struct net_adapter *adapter,
		void *buffer, u32 length);
void hw_return_packet(struct net_adapter *adapter, u16 type);

int wimax_hw_start(struct net_adapter *adapter);
int wimax_hw_stop(struct net_adapter *adapter);
int wimax_hw_init(struct net_adapter *adapter);
void wimax_hw_remove(struct net_adapter *adapter);
int wimax_hw_get_mac_address(void *data);

int cmc732_receive_thread(void *data);
int cmc732_send_thread(void *data);

#endif	/* _WIMAX_HEADERS_H */

