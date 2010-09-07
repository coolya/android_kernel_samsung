/*
 * Copyright (C) 2010 Samsung Electronics Co., Ltd.
 *
 * Copyright (C) 2008 Google, Inc.
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
 * Modified for Crespo on August, 2010 By Samsung Electronics Co.
 * This is modified operate according to each status.
 *
 */

/* Control bluetooth power for Crespo platform */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <mach/gpio.h>
#include <mach/gpio-herring.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <mach/rfkill-herring.h>
#define BT_SLEEP_ENABLER

#define IRQ_BT_HOST_WAKE      IRQ_EINT(21)

static struct wake_lock rfkill_wake_lock;
#ifdef BT_SLEEP_ENABLER
static struct wake_lock bt_wake_lock;
#endif

#ifndef	GPIO_LEVEL_LOW
#define GPIO_LEVEL_LOW		0
#define GPIO_LEVEL_HIGH		1
#endif

static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4329";

static int bluetooth_set_power(void *data, enum rfkill_user_states state)
{
	switch (state) {

	case RFKILL_USER_STATE_UNBLOCKED:
		pr_debug("[BT] Device Powering ON\n");
		s3c_setup_uart_cfg_gpio(0);

		if (gpio_is_valid(GPIO_WLAN_BT_EN))
			gpio_direction_output(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);

		if (gpio_is_valid(GPIO_BT_nRST))
			gpio_direction_output(GPIO_BT_nRST, GPIO_LEVEL_LOW);

		pr_debug("[BT] GPIO_BT_nRST = %d\n",
				gpio_get_value(GPIO_BT_nRST));

		/* Set GPIO_BT_WLAN_REG_ON high */
		s3c_gpio_setpull(GPIO_WLAN_BT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);

		s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN,
				S3C_GPIO_PULL_NONE);

		pr_debug("[BT] GPIO_WLAN_BT_EN = %d\n",
				gpio_get_value(GPIO_WLAN_BT_EN));
		/*
		 * FIXME sleep should be enabled disabled since the device is
		 * not booting if its enabled
		 */
		/*
		 * 100msec, delay between reg_on & rst.
		 * (bcm4329 powerup sequence)
		 */
		msleep(100);

		/* Set GPIO_BT_nRST high */
		s3c_gpio_setpull(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_nRST, GPIO_LEVEL_HIGH);

		s3c_gpio_slp_cfgpin(GPIO_BT_nRST, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);

		pr_debug("[BT] GPIO_BT_nRST = %d\n",
				gpio_get_value(GPIO_BT_nRST));

		break;

	case RFKILL_USER_STATE_SOFT_BLOCKED:
		pr_debug("[BT] Device Powering OFF\n");

		s3c_gpio_setpull(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_nRST, GPIO_LEVEL_LOW);

		s3c_gpio_slp_cfgpin(GPIO_BT_nRST, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);

		pr_debug("[BT] GPIO_BT_nRST = %d\n",
				gpio_get_value(GPIO_BT_nRST));

		if (gpio_get_value(GPIO_WLAN_nRST) == 0) {
			s3c_gpio_setpull(GPIO_WLAN_BT_EN, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);

			s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN,
					S3C_GPIO_PULL_NONE);

			pr_debug("[BT] GPIO_WLAN_BT_EN = %d\n",
					gpio_get_value(GPIO_WLAN_BT_EN));
		}

		break;

	default:
		pr_err("[BT] Bad bluetooth rfkill state %d\n", state);
	}

	return 0;
}

irqreturn_t bt_host_wake_irq_handler(int irq, void *dev_id)
{
	pr_debug("[BT] bt_host_wake_irq_handler start\n");

	wake_lock_timeout(&rfkill_wake_lock, 5*HZ);

	return IRQ_HANDLED;
}

static int bt_rfkill_set_block(void *data, bool blocked)
{
	unsigned int ret = 0;

	ret = bluetooth_set_power(data, blocked ?
			RFKILL_USER_STATE_SOFT_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);

	return ret;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_rfkill_set_block,
};

static int __init herring_rfkill_probe(struct platform_device *pdev)
{
	int irq;
	int ret;

	/* Initialize wake locks */
	wake_lock_init(&rfkill_wake_lock, WAKE_LOCK_SUSPEND, "board-rfkill");
#ifdef BT_SLEEP_ENABLER
	wake_lock_init(&bt_wake_lock, WAKE_LOCK_SUSPEND, "bt-rfkill");
#endif
	ret = gpio_request(GPIO_WLAN_BT_EN, "GPB");
	if(ret < 0) {
		pr_err("[BT] Failed to request GPIO_WLAN_BT_EN!\n");
		goto err_req_gpio_wlan_bt_en;
	}

	ret = gpio_request(GPIO_BT_nRST, "GPB");
	if(ret < 0) {
		pr_err("[BT] Failed to request GPIO_BT_nRST!\n");
		goto err_req_gpio_bt_nrst;
	}

	ret = gpio_request(GPIO_BT_WAKE, "GPH2");
	if(ret < 0) {
		pr_err("[BT] Failed to request GPIO_BT_WAKE!\n");
		goto err_req_gpio_bt_wake;
	}

	/* BT Host Wake IRQ */
	irq = IRQ_BT_HOST_WAKE;

	set_irq_type(irq, IRQ_TYPE_EDGE_RISING);
	ret = request_irq(irq, bt_host_wake_irq_handler, 0,
			"bt_host_wake_irq_handler", NULL);

	if (ret < 0) {
		pr_err("[BT] Request_irq failed\n");
		goto err_req_irq;
	}

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&bt_rfkill_ops, NULL);

	if (!bt_rfk) {
		pr_err("[BT] bt_rfk : rfkill_alloc is failed\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	rfkill_init_sw_state(bt_rfk, 0);

	pr_debug("[BT] rfkill_register(bt_rfk)\n");

	ret = rfkill_register(bt_rfk);
	if (ret) {
		pr_debug("********ERROR IN REGISTERING THE RFKILL********\n");
		goto err_register;
	}

	rfkill_set_sw_state(bt_rfk, 1);
	bluetooth_set_power(NULL, RFKILL_USER_STATE_SOFT_BLOCKED);

	return ret;

 err_register:
	rfkill_destroy(bt_rfk);

 err_alloc:
	free_irq(irq, NULL);

 err_req_irq:
	gpio_free(GPIO_BT_WAKE);

 err_req_gpio_bt_wake:
	gpio_free(GPIO_BT_nRST);

 err_req_gpio_bt_nrst:
	gpio_free(GPIO_WLAN_BT_EN);

 err_req_gpio_wlan_bt_en:
	return ret;
}

static struct platform_driver herring_device_rfkill = {
	.probe = herring_rfkill_probe,
	.driver = {
		.name = "bt_rfkill",
		.owner = THIS_MODULE,
	},
};

#ifdef BT_SLEEP_ENABLER
static struct rfkill *bt_sleep;

static int bluetooth_set_sleep(void *data, enum rfkill_user_states state)
{
	switch (state) {

	case RFKILL_USER_STATE_UNBLOCKED:
		pr_debug("[BT] In the unblocked state of the sleep\n");
		if (gpio_is_valid(GPIO_BT_WAKE))
			gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

		s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

		pr_debug("[BT] GPIO_BT_WAKE = %d\n",
				gpio_get_value(GPIO_BT_WAKE));
		pr_debug("[BT] wake_unlock(bt_wake_lock)\n");
		wake_unlock(&bt_wake_lock);
		break;

	case RFKILL_USER_STATE_SOFT_BLOCKED:
		pr_debug("[BT] In the soft blocked state of the sleep\n");
		if (gpio_is_valid(GPIO_BT_WAKE))
			gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);

		s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);

		pr_debug("[BT] GPIO_BT_WAKE = %d\n",
				gpio_get_value(GPIO_BT_WAKE));
		pr_debug("[BT] wake_lock(bt_wake_lock)\n");
		wake_lock(&bt_wake_lock);
		break;

	default:
		pr_err("[BT] bad bluetooth rfkill state %d\n", state);
	}
	return 0;
}

static int btsleep_rfkill_set_block(void *data, bool blocked)
{
	unsigned int ret = 0;

	ret = bluetooth_set_sleep(data, blocked ?
			RFKILL_USER_STATE_SOFT_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);

	return ret;
}

static const struct rfkill_ops btsleep_rfkill_ops = {
	.set_block = btsleep_rfkill_set_block,
};

static int __init herring_btsleep_probe(struct platform_device *pdev)
{
	int rc = 0;

	bt_sleep = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&btsleep_rfkill_ops, NULL);
	if (!bt_sleep) {
		pr_err("[BT] bt_sleep : rfkill_alloc is failed\n");
		return -ENOMEM;
	}

	rfkill_set_sw_state(bt_sleep, 1);

	rc = rfkill_register(bt_sleep);
	if (rc)
		rfkill_destroy(bt_sleep);

	bluetooth_set_sleep(NULL, RFKILL_USER_STATE_UNBLOCKED);

	return rc;
}

static struct platform_driver herring_device_btsleep = {
	.probe = herring_btsleep_probe,
	.driver = {
		.name = "bt_sleep",
		.owner = THIS_MODULE,
	},
};
#endif

static int __init herring_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&herring_device_rfkill);

#ifdef BT_SLEEP_ENABLER
	platform_driver_register(&herring_device_btsleep);
#endif

	return rc;
}

module_init(herring_rfkill_init);
MODULE_DESCRIPTION("herring rfkill");
MODULE_LICENSE("GPL");
