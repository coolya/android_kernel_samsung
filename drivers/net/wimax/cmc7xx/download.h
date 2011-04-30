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
#ifndef _WIMAX_DOWNLOAD_H__
#define _WIMAX_DOWNLOAD_H__

#define CMC732_RAM_START		0xC0000000
#define CMC732_WIMAX_ADDRESS		CMC732_RAM_START

#define CMD_MSG_TOTAL_LENGTH		12
#define IMAGE_INFO_MSG_TOTAL_LENGTH	28
#define CMD_MSG_LENGTH			0
#define IMAGE_INFO_MSG_LENGTH		16
#define MAX_IMAGE_DATA_LENGTH		3564
#define MAX_IMAGE_DATA_MSG_LENGTH	4096

#define FWDOWNLOAD_TIMEOUT		5
#define MAX_WIMAXFW_SIZE		2100000

/* used for host boot (firmware download) */
enum {
	MSG_DRIVER_OK_REQ	= 0x5010,
	MSG_DRIVER_OK_RESP	= 0x6010,
	MSG_IMAGE_INFO_REQ	= 0x3021,
	MSG_IMAGE_INFO_RESP	= 0x4021,
	MSG_IMAGE_DATA_REQ	= 0x3022,
	MSG_IMAGE_DATA_RESP	= 0x4022,
	MSG_RUN_REQ		= 0x5014,
	MSG_RUN_RESP		= 0x6014
};

struct image_data_payload {
	u32	offset;
	u32	size;
	u8	data[MAX_IMAGE_DATA_LENGTH];
};

int load_wimax_image(int mode, struct net_adapter *adapter);
void unload_wimax_image(struct net_adapter *adapter);

u8 send_image_info_packet(struct net_adapter *adapter, u16 cmd_id);
u8 send_image_data_packet(struct net_adapter *adapter, u16 cmd_id);
u8 send_cmd_packet(struct net_adapter *adapter, u16 cmd_id);
u32 sd_send(struct net_adapter *adapter, u8 *buffer, u32 len);

#endif	/* _WIMAX_DOWNLOAD_H__ */
