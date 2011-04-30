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
#include <linux/mutex.h>

static int hw_sdio_write_bank_index(struct net_adapter *adapter, int *write_idx)
{
	int ret = 0;

	*write_idx = sdio_readb(adapter->func, SDIO_H2C_WP_REG, &ret);
	if (ret)
		return ret;

	if (((*write_idx + 1) % 15) == sdio_readb(adapter->func,
							SDIO_H2C_RP_REG, &ret))
		*write_idx = -1;

	return ret;
}

u32 sd_send(struct net_adapter *adapter, u8 *buffer, u32 len)
{
	int nRet;

	int nWriteIdx;

	len += (len & 1) ? 1 : 0;

	if (adapter->halted || adapter->removed) {
		pr_debug("Halted Already");
		return STATUS_UNSUCCESSFUL;
	}
	sdio_claim_host(adapter->func);
	nRet = hw_sdio_write_bank_index(adapter, &nWriteIdx);

	if (nRet || (nWriteIdx < 0)) {
		pr_debug("sd_send : error occurred during	\
				fetch bank index!! nRet = %d, nWriteIdx = %d",
				nRet, nWriteIdx);
		sdio_release_host(adapter->func);
		return STATUS_UNSUCCESSFUL;
	}

	sdio_writeb(adapter->func, (nWriteIdx + 1) % 15, SDIO_H2C_WP_REG, NULL);
	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR+(SDIO_BANK_SIZE * nWriteIdx)+4,
			buffer, len);

	if (nRet < 0)
		pr_debug("sd_send : error in sending packet!! nRet = %d",
				nRet);

	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR + (SDIO_BANK_SIZE * nWriteIdx),
			&len, 4);
	sdio_release_host(adapter->func);
	if (nRet < 0)
		pr_debug("sd_send : error in writing bank length!! nRet = %d",
				nRet);

	return nRet;
}


/* get MAC address from device */
int wimax_hw_get_mac_address(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	struct hw_private_packet	req;
	int				nResult = 0;
	int				retry = 4;

	req.id0 = 'W';
	req.id1 = 'P';
	req.code = HWCODEMACREQUEST;
	req.value = 0;

	pr_debug("wait for SDIO ready..");
	msleep(1700); /*
					*  IMPORTANT! wait for cmc720 can
					* handle mac req packet
					*/
	do {

		nResult = sd_send(adapter, (u8 *)&req,
				sizeof(struct hw_private_packet));

		if (nResult != STATUS_SUCCESS)
			pr_debug("wimax_hw_get_mac_address: sd_send fail!!");
		msleep(300);

		}
	while ((!adapter->mac_ready) && (adapter) && (retry--));

	do_exit(0);

	return 0;
}

u32 hw_send_data(struct net_adapter *adapter, void *buffer , u32 length)
{
	struct buffer_descriptor	*bufdsc;
	struct hw_packet_header		*hdr;
	struct net_device		*net = adapter->net;
	u8				*ptr;

	bufdsc = (struct buffer_descriptor *)
		kmalloc(sizeof(struct buffer_descriptor), GFP_ATOMIC);
	if (bufdsc == NULL)
		return STATUS_RESOURCES;

	bufdsc->buffer = kmalloc(BUFFER_DATA_SIZE, GFP_ATOMIC);
	if (bufdsc->buffer == NULL) {
		kfree(bufdsc);
		return STATUS_RESOURCES;
	}

	ptr = bufdsc->buffer;

	/* shift data pointer */
	ptr += sizeof(struct hw_packet_header);
	ptr += 2;
	hdr = (struct hw_packet_header *)bufdsc->buffer;

	length -= (ETHERNET_ADDRESS_LENGTH * 2);
	buffer += (ETHERNET_ADDRESS_LENGTH * 2);

	memcpy(ptr, buffer, length);

	hdr->id0 = 'W';
	hdr->id1 = 'D';
	hdr->length = (u16)length;

	bufdsc->length = length + sizeof(struct hw_packet_header);
	bufdsc->length += 2;

	/* add statistics */
	adapter->netstats.tx_packets++;
	adapter->netstats.tx_bytes += bufdsc->length;

	spin_lock(&adapter->hw.lock);
	list_add_tail(&bufdsc->list, &adapter->hw.q_send);
	spin_unlock(&adapter->hw.lock);

	wake_up_interruptible(&adapter->send_event);
	if (!netif_running(net))
		pr_debug("!netif_running");

	return STATUS_SUCCESS;
}

static u32 sd_send_data(struct net_adapter *adapter,
		struct buffer_descriptor *bufdsc)
{
	if (bufdsc->length > SDIO_MAX_BYTE_SIZE)
		bufdsc->length = (bufdsc->length + SDIO_MAX_BYTE_SIZE) &
			~(SDIO_MAX_BYTE_SIZE);

	if (adapter->halted) {
		pr_debug("Halted Already");
		return STATUS_UNSUCCESSFUL;
	}

	return sd_send(adapter, bufdsc->buffer, bufdsc->length);
}

int hw_device_wakeup(struct net_adapter *adapter)
{
	struct wimax732_platform_data *pdata = adapter->pdata;
	int	rc = 0;
	u8	retryCount = 0;

	if (pdata->g_cfg->wimax_status == WIMAX_STATE_READY) {
		pr_debug("not ready!!");
		return 0;
	}

	adapter->prev_wimax_status = pdata->g_cfg->wimax_status;
	pdata->g_cfg->wimax_status = WIMAX_STATE_AWAKE_REQUESTED;

	/* try to wake up */
	while (retryCount < WAKEUP_MAX_TRY) {
		rc = wait_for_completion_interruptible_timeout(
				&adapter->wakeup_event,
				msecs_to_jiffies(WAKEUP_TIMEOUT));

		/* received wakeup ack */
		if (rc)
			break;

		retryCount++;

		if (pdata->is_modem_awake()) {
			pr_debug("WiMAX active pin HIGH ..");
			break;
		}
	}

	/* check WiMAX modem response */
	if (!pdata->is_modem_awake()) {
		pr_debug("FATAL ERROR!! MODEM DOES NOT WAKEUP!!");
		adapter->halted = true;
		return STATUS_UNSUCCESSFUL;
	}

	if (pdata->g_cfg->wimax_status == WIMAX_STATE_AWAKE_REQUESTED) {
		if (adapter->prev_wimax_status == WIMAX_STATE_IDLE
				||
			adapter->prev_wimax_status == WIMAX_STATE_NORMAL) {
			pr_debug("hw_device_wakeup: IDLE -> NORMAL");
			pdata->g_cfg->wimax_status = WIMAX_STATE_NORMAL;
		} else {
			pr_debug("hw_device_wakeup: VI -> READY");
			pdata->g_cfg->wimax_status = WIMAX_STATE_READY;
		}
	}

	return STATUS_SUCCESS;
}

int cmc732_send_thread(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	struct wimax732_platform_data *pdata;
	struct buffer_descriptor	*bufdsc = NULL;
	int				nRet = 0;
	bool	reset_modem = false;

	pdata = adapter->pdata;

	do {
		wait_event_interruptible(adapter->send_event,
				(!list_empty(&adapter->hw.q_send))
				|| (!adapter) || adapter->halted);

		if ((!adapter) || adapter->halted)
			break;

		wake_lock(&pdata->g_cfg->wimax_tx_lock);
		mutex_lock(&pdata->g_cfg->suspend_mutex);
		pdata->wakeup_assert(1);

		if ((pdata->g_cfg->wimax_status == WIMAX_STATE_IDLE ||
			pdata->g_cfg->wimax_status == WIMAX_STATE_VIRTUAL_IDLE)
			&& !pdata->is_modem_awake()) {
			if (hw_device_wakeup(adapter)) {
				reset_modem = true;
				mutex_unlock(&pdata->g_cfg->suspend_mutex);
				wake_unlock(&pdata->g_cfg->wimax_tx_lock);
				break;
			}
		}

		spin_lock(&adapter->hw.lock);
		if (!list_empty(&adapter->hw.q_send)) {
			bufdsc = list_first_entry(&adapter->hw.q_send,
					struct buffer_descriptor, list);
			list_del(&bufdsc->list);
		}
		spin_unlock(&adapter->hw.lock);

		if (!bufdsc) {
			pr_debug("Fail...node is null");
			mutex_unlock(&pdata->g_cfg->suspend_mutex);
			wake_unlock(&pdata->g_cfg->wimax_tx_lock);
			continue;
		}
		nRet = sd_send_data(adapter, bufdsc);
		pdata->wakeup_assert(0);
		mutex_unlock(&pdata->g_cfg->suspend_mutex);
		wake_unlock(&pdata->g_cfg->wimax_tx_lock);
		kfree(bufdsc->buffer);
		kfree(bufdsc);
		if (nRet != STATUS_SUCCESS) {
			pr_debug("SendData Fail******");
			++adapter->XmitErr;
			reset_modem = true;
			break;
		}
	} while (adapter);

	pr_debug("cmc732_send_thread exiting");

	adapter->halted = true;
	if (reset_modem)
		pdata->power(0);

	do_exit(0);

	return 0;
}
