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
#ifndef _WIMAX_HW_TYPES_H
#define _WIMAX_HW_TYPES_H

#include "ctl_types.h"

/* private protocol defines */
#define HW_PROT_VALUE_LINK_DOWN			0x00
#define HW_PROT_VALUE_LINK_UP			0xff

/*
* SDIO general defines
* size of a bank in cmc's rx and tx buffers
*/
#define SDIO_BANK_SIZE					4096
/* maximum block size (SDIO) */
#define SDIO_MAX_BLOCK_SIZE			2048
/* maximum size in byte mode */
#define SDIO_MAX_BYTE_SIZE			511
#define SDIO_BUFFER_SIZE			4092
#define CMC_BLOCK_SIZE				512

/* SDIO function addresses  */
#define SDIO_TX_BANK_ADDR			0x1000
#define SDIO_RX_BANK_ADDR			0x10000
#define SDIO_INT_STATUS_REG		0xC0
#define SDIO_INT_STATUS_CLR_REG	0xC4

#define SDIO_C2H_WP_REG			0xE4
#define SDIO_C2H_RP_REG			0xE8
#define SDIO_H2C_WP_REG			0xEC
#define SDIO_H2C_RP_REG			0xF0

/* SDIO function registers */
#define SDIO_INT_DATA_READY			0x01
#define SDIO_INT_ERROR				0x02

#define WAKEUP_MAX_TRY				6
#define WAKEUP_TIMEOUT				2000

/* packet types */
enum {
	HWPKTTYPENONE = 0xff00,
	HWPKTTYPEPRIVATE,
	HWPKTTYPECONTROL,
	HWPKTTYPEDATA,
	HWPKTTYPETIMEOUT
};

/* private packet opcodes */
enum {
	HWCODEMACREQUEST = 0x01,
	HWCODEMACRESPONSE,
	HWCODELINKINDICATION,
	HWCODERXREADYINDICATION,
	HWCODEHALTEDINDICATION,
	HWCODEIDLENTFY,
	HWCODEWAKEUPNTFY
};

#pragma pack(1)
struct hw_packet_header {
	char		id0;	/* packet ID */
	char		id1;
	u16	length;	/* packet length */
};

struct hw_private_packet {
	char		id0;	/* packet ID */
	char		id1;
	u8	code;	/* command code */
	u8	value;	/* command value */
};
#pragma pack()

struct wimax_msg_header {
	u16	type;
	u16	id;
	u32	length;
};

struct buffer_descriptor {
	struct list_head	list;		/* list node */
	void	*buffer;			/* allocated buffer: */
	u32			length;		/* current data length */
	u16			type;		/* buffer type used
								*  for control
								* buffers
								*/
};

struct hardware_info {
	void		*receive_buffer;
	u8		eth_header[ETHERNET_ADDRESS_LENGTH * 2];
	struct list_head	q_send;	/* send pending queue */
	spinlock_t		lock;
};

#endif	/* _WIMAX_HW_TYPES_H */
