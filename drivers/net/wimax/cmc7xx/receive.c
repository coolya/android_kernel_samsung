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

static void process_indicate_packet(struct net_adapter *adapter, u8 *buffer)
{
	struct wimax_msg_header *packet;
	struct wimax_cfg *g_cfg = adapter->pdata->g_cfg;
	char *tmp_byte;

	packet = (struct wimax_msg_header *)buffer;

	if (packet->type != be16_to_cpu(ETHERTYPE_DL)) {
		pr_warn("%s: not a download packet\n", __func__);
		return;
	}

	switch (be16_to_cpu(packet->id)) {
	case MSG_DRIVER_OK_RESP:
		pr_debug("%s: MSG_DRIVER_OK_RESP\n", __func__);
		send_image_info_packet(adapter, MSG_IMAGE_INFO_REQ);
		break;
	case MSG_IMAGE_INFO_RESP:
		pr_debug("%s: MSG_IMAGE_INFO_RESP\n", __func__);
		send_image_data_packet(adapter, MSG_IMAGE_DATA_REQ);
		break;
	case MSG_IMAGE_DATA_RESP:
		if (adapter->wimax_image.offset == adapter->wimax_image.size) {
			pr_debug("%s: Image Download Complete\n", __func__);
			send_cmd_packet(adapter, MSG_RUN_REQ);
		} else {
			send_image_data_packet(adapter, MSG_IMAGE_DATA_REQ);
		}
		break;
	case MSG_RUN_RESP:
		tmp_byte = (char *)(buffer + sizeof(struct wimax_msg_header));

		if (*tmp_byte != 0x01)
			break;

		adapter->download_complete = true;
		wake_up_interruptible(&adapter->download_event);

		pr_debug("%s: MSG_RUN_RESP\n", __func__);
		if (g_cfg->wimax_mode == SDIO_MODE
			|| g_cfg->wimax_mode == DM_MODE
			|| g_cfg->wimax_mode == USB_MODE
			|| g_cfg->wimax_mode	== USIM_RELAY_MODE) {

			adapter->mac_task = kthread_create(
					wimax_hw_get_mac_address, adapter,
					"%s", "mac_request_thread");
			if (adapter->mac_task)
				wake_up_process(adapter->mac_task);

		} else if (g_cfg->wimax_mode == WTM_MODE
				|| g_cfg->wimax_mode == AUTH_MODE) {
			adapter->download_complete = true;
			wake_up_interruptible(&adapter->download_event);
		}
		break;
	default:
		pr_err("%s: Unknown packet type\n", __func__);
		break;
	}
}

static u32 process_private_cmd(struct net_adapter *adapter, void *buffer)
{
	struct hw_private_packet	*cmd;
	struct wimax_cfg	*g_cfg = adapter->pdata->g_cfg;
	u8				*bufp;
	int					ret;

	cmd = (struct hw_private_packet *)buffer;

	switch (cmd->code) {
	case HWCODEMACRESPONSE: {
		u8 mac_addr[ETHERNET_ADDRESS_LENGTH];
		bufp = (u8 *)buffer;

		/* processing for mac_req request */
		pr_debug("MAC address = %02x:%02x:%02x:%02x:%02x:%02x",
				bufp[3], bufp[4], bufp[5],
				bufp[6], bufp[7], bufp[8]);
		memcpy(mac_addr, bufp + 3,
				ETHERNET_ADDRESS_LENGTH);

		/* create ethernet header */
		memcpy(adapter->hw.eth_header,
				mac_addr, ETHERNET_ADDRESS_LENGTH);
		memcpy(adapter->hw.eth_header + ETHERNET_ADDRESS_LENGTH,
				mac_addr, ETHERNET_ADDRESS_LENGTH);
		adapter->hw.eth_header[(ETHERNET_ADDRESS_LENGTH * 2) - 1] += 1;

		memcpy(adapter->net->dev_addr, bufp + 3,
				ETHERNET_ADDRESS_LENGTH);
		adapter->mac_ready = true;

		ret = sizeof(struct hw_private_packet)
			+ ETHERNET_ADDRESS_LENGTH - sizeof(u8);
		return ret;
	}
	case HWCODELINKINDICATION: {
		if ((cmd->value == HW_PROT_VALUE_LINK_DOWN)
			&& (adapter->media_state != MEDIA_DISCONNECTED)) {
			pr_debug("LINK_DOWN_INDICATION");

			/* set values */
			adapter->media_state = MEDIA_DISCONNECTED;
			g_cfg->wimax_status = WIMAX_STATE_READY;

			/* indicate link down */
			netif_stop_queue(adapter->net);
			netif_carrier_off(adapter->net);
		} else if ((cmd->value == HW_PROT_VALUE_LINK_UP)
			&& (adapter->media_state != MEDIA_CONNECTED)) {
			pr_debug("LINK_UP_INDICATION");

			/* set values */
			adapter->media_state = MEDIA_CONNECTED;
			g_cfg->wimax_status = WIMAX_STATE_NORMAL;
			adapter->net->mtu = WIMAX_MTU_SIZE;

			/* indicate link up */
			netif_start_queue(adapter->net);
			netif_carrier_on(adapter->net);
		}
		break;
	}
	case HWCODEHALTEDINDICATION: {
		pr_debug("Device is about to reset, stop driver");
		adapter->halted = true;
		break;
	}
	case HWCODERXREADYINDICATION: {
		pr_debug("Device RxReady");
		/*
		* to start the data packet send
		* queue again after stopping in xmit
		*/
		if (adapter->media_state == MEDIA_CONNECTED)
			netif_wake_queue(adapter->net);
		break;
	}
	case HWCODEIDLENTFY: {
		/* set idle / vi */
		if (g_cfg->wimax_status == WIMAX_STATE_NORMAL
				|| g_cfg->wimax_status == WIMAX_STATE_IDLE) {
			pr_debug("process_private_cmd: IDLE");
			g_cfg->wimax_status = WIMAX_STATE_IDLE;
		} else {
			pr_debug("process_private_cmd: VIRTUAL IDLE");
			g_cfg->wimax_status = WIMAX_STATE_VIRTUAL_IDLE;
		}
		break;
	}
	case HWCODEWAKEUPNTFY: {
		/*
		* IMPORTANT!! at least 4 sec
		* is required after modem waked up
		*/
		wake_lock_timeout(&g_cfg->wimax_wake_lock, 4 * HZ);

		if (g_cfg->wimax_status ==
				WIMAX_STATE_AWAKE_REQUESTED) {
			complete(&adapter->wakeup_event);
			break;
		}

		if (g_cfg->wimax_status == WIMAX_STATE_IDLE
			|| g_cfg->wimax_status == WIMAX_STATE_NORMAL) {
			pr_debug("process_private_cmd: IDLE -> NORMAL");
			g_cfg->wimax_status = WIMAX_STATE_NORMAL;
		} else {
			pr_debug("process_private_cmd: VI -> READY");
			g_cfg->wimax_status = WIMAX_STATE_READY;
		}
		break;
	}
	default:
		pr_debug("Command = %04x", cmd->code);
		break;
	}

	return sizeof(struct hw_private_packet);
}


static void adapter_sdio_rx_packet(struct net_adapter *adapter, int len)
{
	struct hw_packet_header	*hdr;
	int							rlen;
	u32			type;
	u8						*ofs;
	struct sk_buff				*rx_skb;

	rlen = len;
	ofs = (u8 *)adapter->hw.receive_buffer;

	while (rlen > 0) {
		hdr = (struct hw_packet_header *)ofs;
		type = HWPKTTYPENONE;

		/* "WD", "WC", "WP" or "WE" */
		if (unlikely(hdr->id0 != 'W')) {
			if (rlen > 4) {
				pr_debug("Wrong packet	\
				ID (%02x %02x)", hdr->id0, hdr->id1);
			}
			/* skip rest of packets */
			break;
		}

		/* check packet type */
		switch (hdr->id1) {
		case 'P': {
			u32 l = 0;
			type = HWPKTTYPEPRIVATE;

			/* process packet */
			l = process_private_cmd(adapter, ofs);

			/* shift */
			ofs += l;
			rlen -= l;

			/* process next packet */
			continue;
		}
		case 'C':
			type = HWPKTTYPECONTROL;
			break;
		case 'D':
			type = HWPKTTYPEDATA;
			break;
		case 'E':
			/* skip rest of buffer */
			break;
		default:
			pr_debug("hwParseReceivedData :	\
					Wrong packet ID [%02x %02x]",
						hdr->id0, hdr->id1);
			/* skip rest of buffer */
			break;
		}

		if (type == HWPKTTYPENONE)
			break;

		if (likely(!adapter->downloading)) {
			if (unlikely(hdr->length > WIMAX_MAX_TOTAL_SIZE
					|| ((hdr->length +
				sizeof(struct hw_packet_header)) > rlen))) {
				pr_debug("Packet length is	\
						too big (%d)", hdr->length);
				/* skip rest of packets */
				break;
			}
		}

		/* change offset */
		ofs += sizeof(struct hw_packet_header);
		rlen -= sizeof(struct hw_packet_header);

		/* process download packet, data and control packet */
		if (likely(!adapter->downloading)) {
			ofs += 2;
			rlen -= 2;

			if (unlikely(type == HWPKTTYPECONTROL))
				control_recv(adapter, (u8 *)ofs,
							hdr->length);
			else {
				if (hdr->length > BUFFER_DATA_SIZE) {
					pr_debug("Data packet too large");
					adapter->netstats.rx_dropped++;
					break;
				}

				rx_skb = dev_alloc_skb(hdr->length +
						(ETHERNET_ADDRESS_LENGTH * 2));
				if (!rx_skb) {
					pr_debug("MEMORY PINCH:	\
						unable to allocate skb");
					break;
					}
				skb_reserve(rx_skb,
						(ETHERNET_ADDRESS_LENGTH * 2));

				memcpy(skb_push(rx_skb,
						(ETHERNET_ADDRESS_LENGTH * 2)),
						adapter->hw.eth_header,
						(ETHERNET_ADDRESS_LENGTH * 2));

				memcpy(skb_put(rx_skb, hdr->length),
							(u8 *)ofs,
							hdr->length);

				rx_skb->dev = adapter->net;
				rx_skb->protocol =
					eth_type_trans(rx_skb, adapter->net);
				rx_skb->ip_summed = CHECKSUM_UNNECESSARY;

				if (netif_rx_ni(rx_skb) == NET_RX_DROP) {
					pr_debug("packet dropped!");
					adapter->netstats.rx_dropped++;
				}
				adapter->netstats.rx_packets++;
				adapter->netstats.rx_bytes +=
					(hdr->length +
					 (ETHERNET_ADDRESS_LENGTH * 2));

			}
		} else {
			hdr->length -= sizeof(struct hw_packet_header);
			process_indicate_packet(adapter, ofs);
		}
		/*
		* If the packet is unreasonably long,
		* quietly drop it rather than
		* kernel panicing by calling skb_put.
		*
		* shift
		*/
		ofs += hdr->length;
		rlen -= hdr->length;
	}

	return;
}

static int hw_sdio_read_bank_index(struct net_adapter *adapter, int *read_idx)
{
	int ret = 0;

	*read_idx = sdio_readb(adapter->func, SDIO_C2H_RP_REG, &ret);
	if (ret)
		return ret;

	if (*read_idx == sdio_readb(adapter->func, SDIO_C2H_WP_REG, &ret))
		*read_idx = -1;

	return ret;
}

static int hw_sdio_read_counter(struct net_adapter *adapter, u32 *len,
								int read_idx)
{
	int ret = 0;

	*len = sdio_readw(adapter->func, (SDIO_RX_BANK_ADDR +
				(read_idx * SDIO_BANK_SIZE)), &ret);
	if (ret)
		return ret;

	*len |= sdio_readw(adapter->func, (SDIO_RX_BANK_ADDR +
				(read_idx * SDIO_BANK_SIZE) + 2), &ret) << 16;

	if (*len > SDIO_BUFFER_SIZE)
		*len = 0;

	return ret;
}

int cmc732_receive_thread(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	int							err = 1;
	int							nReadIdx;
	u32							len = 0;
	u32							t_len;
	u32							t_index;
	u32							t_size;
	u8							*t_buff;

	do {

		wait_event_interruptible(adapter->receive_event,
				adapter->rx_data_available
				|| (!adapter) || adapter->halted);
		adapter->rx_data_available = false;
		if ((!adapter) || adapter->halted)
			break;
		sdio_claim_host(adapter->func);
		err = hw_sdio_read_bank_index(adapter, &nReadIdx);

		if (err) {
			pr_debug("adapter_sdio_rx_packet :	\
				error occurred during fetch bank	\
				index!! err = %d, nReadIdx = %d",	\
				err, nReadIdx);
			sdio_release_host(adapter->func);
			continue;
		}
		if (nReadIdx < 0) {
			sdio_release_host(adapter->func);
			continue;
			}
		err = hw_sdio_read_counter(adapter, &len, nReadIdx);
		if (unlikely(err || !len)) {
			pr_debug("adapter_sdio_rx_packet :	\
					error in reading bank length!!	\
					err = %d, len = %d", err, len);
			sdio_release_host(adapter->func);
			continue;
		}

		if (unlikely(len > SDIO_BUFFER_SIZE)) {
			pr_debug("ERROR RECV length (%d) >	\
					SDIO_BUFFER_SIZE", len);
			len = SDIO_BUFFER_SIZE;
		}

		sdio_writeb(adapter->func, (nReadIdx + 1) % 16,
				SDIO_C2H_RP_REG, NULL);

		t_len = len;
		t_index = (SDIO_RX_BANK_ADDR + (SDIO_BANK_SIZE * nReadIdx) + 4);
		t_buff = (u8 *)adapter->hw.receive_buffer;

		while (t_len) {
			t_size = (t_len > CMC_BLOCK_SIZE) ?
				(CMC_BLOCK_SIZE) : t_len;
			err = sdio_memcpy_fromio(adapter->func, (void *)t_buff,
					t_index, t_size);
			t_len -= t_size;
			t_buff += t_size;
			t_index += t_size;
		}

		if (unlikely(err || !len)) {
			pr_debug("adapter_sdio_rx_packet :	\
				error in receiving packet!!drop	the	\
				packet errt = %d, len = %d", err, len);
			sdio_release_host(adapter->func);
			continue;
		}
		sdio_release_host(adapter->func);
		adapter_sdio_rx_packet(adapter, len);
	} while (adapter);

	adapter->halted = true;

	pr_debug("cmc732_receive_thread exiting");

	do_exit(0);

return 0;
}
