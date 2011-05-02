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

u32 control_init(struct net_adapter *adapter)
{
	INIT_LIST_HEAD(&adapter->ctl.q_received_ctrl);
	spin_lock_init(&adapter->ctl.lock);

	INIT_LIST_HEAD(&adapter->ctl.apps.process_list);
	spin_lock_init(&adapter->ctl.apps.lock);

	adapter->ctl.apps.ready = true;

	return STATUS_SUCCESS;
}

void control_remove(struct net_adapter *adapter)
{
	struct list_head	*pos, *nxt;
	struct buffer_descriptor	*bufdsc;
	struct process_descriptor	*procdsc;

	/* Free the received control packets queue */
	list_for_each_safe(pos, nxt, &adapter->ctl.q_received_ctrl) {
		bufdsc = list_entry(pos, struct buffer_descriptor, list);
		list_del(pos);
		kfree(bufdsc->buffer);
		kfree(bufdsc);
	}

	if (!adapter->ctl.apps.ready)
		return;

	list_for_each_safe(pos, nxt, &adapter->ctl.apps.process_list) {
		procdsc = list_entry(pos,
				struct process_descriptor, list);
		if (procdsc->irp) {
			procdsc->irp = false;
			wake_up_interruptible
				(&procdsc->read_wait);
		}
	}
	/* delay for the process release */
	msleep(100);
	adapter->ctl.apps.ready = false;
}

/* add received packet to pending list */
static void control_enqueue_buffer(struct net_adapter *adapter,
					void *buffer, u32 length)
{
	struct buffer_descriptor	*bufdsc;
	struct process_descriptor	*procdsc;
	struct eth_header			hdr;

	/* get the packet type for the process check */
	memcpy(&hdr.type, (unsigned short *)buffer, sizeof(unsigned short));

	/* Queue and wake read only if process exist. */
	procdsc = process_by_type(adapter, hdr.type);
	if (!procdsc) {
		pr_debug("Waiting process not found skip the packet");
		return;
	}

	bufdsc = (struct buffer_descriptor *)
		kmalloc(sizeof(*bufdsc), GFP_KERNEL);
	if (bufdsc == NULL) {
		pr_debug("bufdsc Memory Alloc Failure *****");
		return;
	}
	bufdsc->buffer = kmalloc(
		(length + (ETHERNET_ADDRESS_LENGTH * 2)), GFP_KERNEL);
	if (bufdsc->buffer == NULL) {
		kfree(bufdsc);
		pr_debug("bufdsc->buffer Memory Alloc Failure *****");
		return;
	}

	/* add ethernet header to control packet */
	memcpy(bufdsc->buffer, adapter->hw.eth_header,
			(ETHERNET_ADDRESS_LENGTH * 2));
	memcpy(bufdsc->buffer + (ETHERNET_ADDRESS_LENGTH * 2),
			buffer, length);

	/* fill out descriptor */
	bufdsc->length = length + (ETHERNET_ADDRESS_LENGTH * 2);
	bufdsc->type = hdr.type;

	/* add to pending list */
	list_add_tail(&bufdsc->list, &adapter->ctl.q_received_ctrl);

	if (procdsc->irp) {
		procdsc->irp = false;
		wake_up_interruptible(&procdsc->read_wait);
	}
}

/* receive control data */
void control_recv(struct net_adapter *adapter, void *buffer, u32 length)
{
	/* check halt flag */
	if (adapter->halted)
		return;

	/* not found, add to pending buffers list */
	spin_lock(&adapter->ctl.lock);
	control_enqueue_buffer(adapter, buffer, length);
	spin_unlock(&adapter->ctl.lock);
}

u32 control_send(struct net_adapter *adapter, void *buffer, u32 length)
{
	struct buffer_descriptor	*bufdsc;
	struct hw_packet_header		*hdr;
	u8				*ptr;

	if ((length + sizeof(struct hw_packet_header)) >= WIMAX_MAX_TOTAL_SIZE)
		return STATUS_RESOURCES;/* changed from SUCCESS return status */

	bufdsc = (struct buffer_descriptor *)
		kmalloc(sizeof(*bufdsc), GFP_ATOMIC);
	if (bufdsc == NULL)
		return STATUS_RESOURCES;
	bufdsc->buffer = kmalloc(BUFFER_DATA_SIZE, GFP_ATOMIC);
	if (bufdsc->buffer == NULL) {
		kfree(bufdsc);
		return STATUS_RESOURCES;
	}

	ptr = bufdsc->buffer;
	hdr = (struct hw_packet_header *)bufdsc->buffer;

	ptr += sizeof(struct hw_packet_header);
	ptr += 2;

	memcpy(ptr, buffer + (ETHERNET_ADDRESS_LENGTH * 2),
					length - (ETHERNET_ADDRESS_LENGTH * 2));

	/* add packet header */
	hdr->id0 = 'W';
	hdr->id1 = 'C';
	hdr->length = (u16)length - (ETHERNET_ADDRESS_LENGTH * 2);

	/* set length */
	bufdsc->length = (u16)length - (ETHERNET_ADDRESS_LENGTH * 2)
					+ sizeof(struct hw_packet_header);
	bufdsc->length += 2;

	spin_lock(&adapter->hw.lock);
	list_add_tail(&bufdsc->list, &adapter->hw.q_send);
	spin_unlock(&adapter->hw.lock);

	wake_up_interruptible(&adapter->send_event);

	return STATUS_SUCCESS;
}

struct process_descriptor *process_by_id(struct net_adapter *adapter, u32 id)
{
	struct process_descriptor	*procdsc;

	list_for_each_entry(procdsc, &adapter->ctl.apps.process_list, list) {
		if (procdsc->id == id)	/* process found */
			return procdsc;
	}
	return NULL;
}

struct process_descriptor
	*process_by_type(struct net_adapter *adapter, u16 type)
{
	struct process_descriptor	*procdsc;

	list_for_each_entry(procdsc, &adapter->ctl.apps.process_list, list) {
		if (procdsc->type == type)	/* process found */
			return procdsc;
	}
	return NULL;
}

/* find buffer by buffer type */
struct buffer_descriptor
*buffer_by_type(struct net_adapter *adapter, u16 type)
{
	struct buffer_descriptor	*bufdsc;

	list_for_each_entry(bufdsc, &adapter->ctl.q_received_ctrl, list) {
		if (bufdsc->type == type)	/* process found */ {
			return bufdsc;
		}
	}
	return NULL;
}

void remove_process(struct net_adapter *adapter, u32 id)
{
	struct process_descriptor	*procdsc;
	struct list_head	*pos, *nxt;

	list_for_each_safe(pos, nxt, &adapter->ctl.apps.process_list) {
		procdsc = list_entry(pos, struct process_descriptor, list);
		if (procdsc->id == id) {
			list_del(pos);
			kfree(procdsc);
		}
	}
}
