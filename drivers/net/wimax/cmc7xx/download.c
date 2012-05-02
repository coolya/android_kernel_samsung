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
#include "headers.h"
#include "download.h"

#include <linux/firmware.h>
#include <linux/vmalloc.h>

int load_wimax_image(int mode, struct net_adapter *adapter)
{
	const struct firmware *fw = NULL;
	int ret = STATUS_SUCCESS;
	struct device *dev;

	if (!adapter || !adapter->func) {
		pr_err("%s: device is NULL, can't load firmware\n", __func__);
		return STATUS_UNSUCCESSFUL;
	}

	dev = &adapter->func->dev;

	if (request_firmware(&fw, (mode == AUTH_MODE) ?
				WIMAX_LOADER_PATH : WIMAX_IMAGE_PATH, dev)) {
		dev_err(dev, "%s: Can't open firmware file\n", __func__);
		return STATUS_UNSUCCESSFUL;
	}

	if (adapter->wimax_image.data && adapter->wimax_image.size < fw->size) {
		vfree(adapter->wimax_image.data);
		adapter->wimax_image.data = NULL;
	}

	if (!adapter->wimax_image.data)
		adapter->wimax_image.data = vmalloc(fw->size);

	if (!adapter->wimax_image.data) {
		dev_err(dev, "%s: Can't allocate %d bytes of memory for "
					"firmware\n", __func__, fw->size);
		ret = STATUS_UNSUCCESSFUL;
		goto err_vmalloc;
	}
	memcpy(adapter->wimax_image.data, fw->data, fw->size);

	adapter->wimax_image.size = fw->size;
	adapter->wimax_image.address = CMC732_WIMAX_ADDRESS;
	adapter->wimax_image.offset = 0;

err_vmalloc:
	release_firmware(fw);
	return ret;
}

void unload_wimax_image(struct net_adapter *adapter)
{
	if (adapter->wimax_image.data == NULL)
		return;

	pr_debug("Delete the Image Loaded");
	vfree(adapter->wimax_image.data);
	adapter->wimax_image.data = NULL;
}

u8 send_cmd_packet(struct net_adapter *adapter, u16 cmd_id)
{
	struct hw_packet_header	*pkt_hdr;
	struct wimax_msg_header	*msg_hdr;
	u8			tx_buf[CMD_MSG_TOTAL_LENGTH];
	int			status = 0;
	u32			offset;
	u32			size;

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';
	pkt_hdr->length = be16_to_cpu(CMD_MSG_TOTAL_LENGTH);

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);
	msg_hdr->length = be32_to_cpu(CMD_MSG_LENGTH);

	size = CMD_MSG_TOTAL_LENGTH;

	status = sd_send(adapter, tx_buf, size);


	if (status != STATUS_SUCCESS) {
		/* crc error or data error -
		   set PCWRT '1' & send current type A packet again */
		pr_debug("hwSdioWrite : crc or data error");
		return status;
	}
	return status;
}

u8 send_image_info_packet(struct net_adapter *adapter, u16 cmd_id)
{
	struct hw_packet_header	*pkt_hdr;
	struct wimax_msg_header	*msg_hdr;
	u32			image_info[4];
	u8			tx_buf[IMAGE_INFO_MSG_TOTAL_LENGTH];
	int			status;
	u32			offset;
	u32			size;

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';
	pkt_hdr->length = be16_to_cpu(IMAGE_INFO_MSG_TOTAL_LENGTH);

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);
	msg_hdr->length = be32_to_cpu(IMAGE_INFO_MSG_LENGTH);

	image_info[0] = 0;
	image_info[1] = be32_to_cpu(adapter->wimax_image.size);
	image_info[2] = be32_to_cpu(adapter->wimax_image.address);
	image_info[3] = 0;

	offset += sizeof(struct wimax_msg_header);
	memcpy(&(tx_buf[offset]), image_info, sizeof(image_info));

	size = IMAGE_INFO_MSG_TOTAL_LENGTH;

	status = sd_send(adapter, tx_buf, size);

	if (status != STATUS_SUCCESS) {
		/*
		* crc error or data error -
		*  set PCWRT '1' & send current type A packet again
		*/
		pr_debug("hwSdioWrite : crc error");
		return status;
	}
	return status;
}

u8 send_image_data_packet(struct net_adapter *adapter, u16 cmd_id)
{
	struct hw_packet_header		*pkt_hdr;
	struct image_data_payload	*pImageDataPayload;
	struct wimax_msg_header		*msg_hdr;
	u8				*tx_buf = NULL;
	int					status;
	u32				len;
	u32				offset;
	u32				size;

	tx_buf = kmalloc(MAX_IMAGE_DATA_MSG_LENGTH, GFP_KERNEL);
	if (tx_buf == NULL) {
		pr_debug("MALLOC FAIL!!");
		return -1;
	}

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);

	if (adapter->wimax_image.offset <
			(adapter->wimax_image.size - MAX_IMAGE_DATA_LENGTH))
		len = MAX_IMAGE_DATA_LENGTH;
	else
		len = adapter->wimax_image.size - adapter->wimax_image.offset;

	offset += sizeof(struct wimax_msg_header);
	pImageDataPayload = (struct image_data_payload *)(tx_buf + offset);
	pImageDataPayload->offset = be32_to_cpu(adapter->wimax_image.offset);
	pImageDataPayload->size = be32_to_cpu(len);

	memcpy(pImageDataPayload->data,
		adapter->wimax_image.data + adapter->wimax_image.offset, len);

	size = len + 8; /* length of Payload offset + length + data */
	pkt_hdr->length = be16_to_cpu(CMD_MSG_TOTAL_LENGTH + size);
	msg_hdr->length = be32_to_cpu(size);

	size = CMD_MSG_TOTAL_LENGTH + size;

	status = sd_send(adapter, tx_buf, size);

	if (status != STATUS_SUCCESS) {
		/* crc error or data error -
		* set PCWRT '1' & send current type A packet again
		*/
		pr_debug("hwSdioWrite : crc error");
		kfree(tx_buf);
		return status;
	}

	adapter->wimax_image.offset += len;

	kfree(tx_buf);

	return status;
}

