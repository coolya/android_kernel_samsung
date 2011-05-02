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

static irqreturn_t wimax_hostwake_isr(int irq, void *dev)
{
	struct net_adapter	*adapter = dev;
	wake_lock_timeout(&adapter->pdata->g_cfg->wimax_wake_lock, 1 * HZ);

	return IRQ_HANDLED;
}

static int cmc732_setup_wake_irq(struct net_adapter *adapter)
{
	struct wimax732_platform_data *pdata = adapter->pdata;
	int rc;
	int irq;

	rc = gpio_request(pdata->wimax_int, "gpio_wimax_int");
	if (rc < 0) {
		pr_debug("%s: gpio %d request failed (%d)\n",
			__func__, pdata->wimax_int, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->wimax_int);
	if (rc < 0) {
		pr_debug("%s: failed to set gpio %d as input (%d)\n",
			__func__, pdata->wimax_int, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(pdata->wimax_int);

	rc = request_threaded_irq(irq, NULL, wimax_hostwake_isr,
		IRQF_TRIGGER_FALLING, "wimax_int", adapter);
	if (rc < 0) {
		pr_debug("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			pdata->wimax_int, rc);
		goto err_request_irq;
	}

	rc = enable_irq_wake(irq);
	if (rc < 0) {
		pr_err("%s: enable_irq_wake(%d) failed for gpio %d (%d)\n",
				__func__, irq, pdata->wimax_int, rc);
		goto err_enable_irq_wake;
	}

	adapter->wake_irq = irq;

	return 0;

err_enable_irq_wake:
	free_irq(irq, adapter);
err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->wimax_int);
	return rc;
}

static void cmc732_release_wake_irq(struct net_adapter *adapter)
{
	if (!adapter->wake_irq)
		return;

	disable_irq_wake(adapter->wake_irq);
	free_irq(adapter->wake_irq, adapter);
	gpio_free(adapter->pdata->wimax_int);
}

int wimax_hw_start(struct net_adapter *adapter)
{
	struct wimax732_platform_data *pdata = adapter->pdata;

	pdata->g_cfg->wimax_status = WIMAX_STATE_READY;
	adapter->download_complete = false;

	adapter->rx_task = kthread_create(
					cmc732_receive_thread,	adapter, "%s",
					"cmc732_receive_thread");

	adapter->tx_task = kthread_create(
					cmc732_send_thread,	adapter, "%s",
					"cmc732_send_thread");

	init_waitqueue_head(&adapter->receive_event);
	init_waitqueue_head(&adapter->send_event);

	if (adapter->rx_task && adapter->tx_task) {
		wake_up_process(adapter->rx_task);
		wake_up_process(adapter->tx_task);
	} else {
		pr_debug("Unable to create send-receive threads");
		return STATUS_UNSUCCESSFUL;
	}

	if (load_wimax_image(pdata->g_cfg->wimax_mode, adapter))
		return STATUS_UNSUCCESSFUL;

	if (adapter->downloading) {

		send_cmd_packet(adapter, MSG_DRIVER_OK_REQ);

		switch (wait_event_interruptible_timeout(
				adapter->download_event,
				(adapter->download_complete),
				HZ*FWDOWNLOAD_TIMEOUT)) {
		case 0:
			/* timeout */
			pr_debug("Error wimax_hw_start : \
					F/W Download timeout failed");
			goto download_fail;
		case -ERESTARTSYS:
			/* Interrupted by signal */
			pr_debug("Error wimax_hw_start :"
					"-ERESTARTSYS retry");
			goto download_fail;
		default:
			/* normal condition check */
			if (adapter->removed || adapter->halted) {
				pr_debug("Error wimax_hw_start :	\
						F/W Download surprise removed");
				goto download_fail;
			}
			pr_debug("wimax_hw_start :  F/W Download Complete");
			unload_wimax_image(adapter);

			if (cmc732_setup_wake_irq(adapter) < 0)
				pr_debug("wimax_hw_start :"
						"Error setting up wimax_int");

			break;
		}
		adapter->downloading = false;
	}

	return STATUS_SUCCESS;

download_fail:
	unload_wimax_image(adapter);
	return STATUS_UNSUCCESSFUL;
}

int wimax_hw_stop(struct net_adapter *adapter)
{
	adapter->halted = true;

	/*Remove wakeup  interrupt*/
	cmc732_release_wake_irq(adapter);

	/* Stop Sdio Interface */
	sdio_claim_host(adapter->func);
	sdio_release_irq(adapter->func);
	sdio_disable_func(adapter->func);
	sdio_release_host(adapter->func);

	return STATUS_SUCCESS;
}

int wimax_hw_init(struct net_adapter *adapter)
{
	struct wimax732_platform_data *pdata = adapter->pdata;

	/* set pdata->wimax_wakeup & pdata->wimax_if_mode0 */
	pdata->set_mode();

	/* initilize hardware info structure */
	memset(&adapter->hw, 0x0, sizeof(struct hardware_info));

	adapter->hw.receive_buffer = kzalloc(SDIO_BANK_SIZE, GFP_KERNEL);
	if (adapter->hw.receive_buffer == NULL) {
		pr_debug("kmalloc fail!!");
		return -ENOMEM;
	}

	/* For sending data and control packets */
	INIT_LIST_HEAD(&adapter->hw.q_send);
	spin_lock_init(&adapter->hw.lock);

	init_waitqueue_head(&adapter->download_event);
	init_completion(&adapter->wakeup_event);

	return STATUS_SUCCESS;
}

void wimax_hw_remove(struct net_adapter *adapter)
{
	struct buffer_descriptor *bufdsc;
	struct list_head	*pos, *nxt;

	list_for_each_safe(pos, nxt, &adapter->hw.q_send) {
		bufdsc = list_entry(pos, struct buffer_descriptor, list);
		list_del(pos);
		kfree(bufdsc->buffer);
		kfree(bufdsc);
	}

	if (adapter->hw.receive_buffer != NULL)
		kfree(adapter->hw.receive_buffer);

}
