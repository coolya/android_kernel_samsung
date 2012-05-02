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
#ifndef _WIMAX_CTL_TYPES_H
#define _WIMAX_CTL_TYPES_H

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x89
#endif

/* Macro definition for defining IOCTL */
#define CTL_CODE(DeviceType, Function, Method, Access)	\
(	\
	((DeviceType) << 16) | ((Access) << 14) |	\
	((Function) << 2) | (Method) \
)
/* Define the method codes for how buffers are passed for I/O and FS controls */
#define METHOD_BUFFERED			0

/*
*	Define the access check value for any access

*	The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
*	ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
*	constants *MUST* always be in sync.
*/
#define FILE_ANY_ACCESS			0

#define CONTROL_ETH_TYPE_WCM		0x0015

#define	CONTROL_IOCTL_WRITE_REQUEST		\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x820, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_POWER_CTL	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_MODE_CHANGE	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x838, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ETHERNET_ADDRESS_LENGTH		6

/* eth types for control message */
enum {
	ETHERTYPE_HIM	= 0x1500,
	ETHERTYPE_MC	= 0x1501,
	ETHERTYPE_DM	= 0x1502,
	ETHERTYPE_CT	= 0x1503,
	ETHERTYPE_DL	= 0x1504,
	ETHERTYPE_VSP	= 0x1510,
	ETHERTYPE_AUTH	= 0x1521
};

/* eth header structure */
#pragma pack(1)
struct eth_header {
	u8		dest[ETHERNET_ADDRESS_LENGTH];
	u8		src[ETHERNET_ADDRESS_LENGTH];
	u16		type;
};
#pragma pack()

/* process element managed by control type */
struct process_descriptor {
	struct list_head	list;
	wait_queue_head_t	read_wait;
	u32		id;
	u16		type;
	u16		irp;	/* Used for the read thread indication */
};

/* head structure for process element */
struct ctl_app_descriptor {
	struct list_head	process_list;/* there could be
			 undefined number of  applications */
	spinlock_t		lock;
	u8		ready;
};

struct image_data {
	u32	size;
	u32	address;
	u32	offset;
	struct mutex	lock;
	u8	*data;
};

struct ctl_info {
	/* application device structure */
	struct ctl_app_descriptor	apps;
	struct list_head		q_received_ctrl;/* pending queue */
	spinlock_t	lock;		/* protection */
};

#endif	/* _WIMAX_CTL_TYPES_H */
