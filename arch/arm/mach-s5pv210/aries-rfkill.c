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
#include <mach/gpio-aries.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include "aries.h"

#define BT_SLEEP_ENABLE
#define USE_LOCK_DVFS

#define IRQ_BT_HOST_WAKE      IRQ_EINT(21)

#ifndef	GPIO_LEVEL_LOW
#define GPIO_LEVEL_LOW		0
#define GPIO_LEVEL_HIGH		1
#endif

static struct wake_lock rfkill_wake_lock;
static struct rfkill *bt_rfk;

static const char bt_name[] = "bcm4329";

#ifdef BT_SLEEP_ENABLE
static struct wake_lock bt_wake_lock;
static struct rfkill *bt_sleep_rfk;
#endif /* BT_SLEEP_ENABLE */

volatile int bt_is_running = 0;
EXPORT_SYMBOL(bt_is_running);

#ifdef USE_LOCK_DVFS
static struct rfkill *bt_lock_dvfs_rfk;
static struct rfkill *bt_lock_dvfs_l2_rfk;
#include <mach/cpu-freq-v210.h>
#endif

void bt_uart_rts_ctrl(int flag)
{
	if(!gpio_get_value(GPIO_BT_nRST))
		return;

	if(flag) {
		// BT RTS Set to HIGH
		s3c_gpio_cfgpin(S5PV210_GPA0(3), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV210_GPA0(3), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPA0(3), 1);

                s3c_gpio_slp_cfgpin(S5PV210_GPA0(3), S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(S5PV210_GPA0(3), S3C_GPIO_PULL_NONE);
	}
	else {
		// BT RTS Set to LOW
		s3c_gpio_cfgpin(S5PV210_GPA0(3), S3C_GPIO_OUTPUT);
		gpio_set_value(S5PV210_GPA0(3), 0);

		s3c_gpio_cfgpin(S5PV210_GPA0(3), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPA0(3), S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(bt_uart_rts_ctrl);

static int bluetooth_set_power(void *data, enum rfkill_user_states state)
{
	int ret = 0;
	int irq;
	/* BT Host Wake IRQ */
	irq = IRQ_BT_HOST_WAKE;

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

		/*
		 * 50msec, delay after bt rst
		 * (bcm4329 powerup sequence)
		 */
		msleep(50);

		ret = enable_irq_wake(irq);
		if (ret < 0)
			pr_err("[BT] set wakeup src failed\n");

		enable_irq(irq);
		break;

	case RFKILL_USER_STATE_SOFT_BLOCKED:
		pr_debug("[BT] Device Powering OFF\n");

		bt_is_running = 0;

		ret = disable_irq_wake(irq);
		if (ret < 0)
			pr_err("[BT] unset wakeup src failed\n");

		disable_irq(irq);
		wake_unlock(&rfkill_wake_lock);

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

	bt_is_running = 1;

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

#ifdef BT_SLEEP_ENABLE
static int bluetooth_set_sleep(void *data, enum rfkill_user_states state)
{	
	switch (state) {

		case RFKILL_USER_STATE_UNBLOCKED:
			bt_is_running = 0;
			gpio_set_value(GPIO_BT_WAKE, 0);
			pr_debug("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
			pr_debug("[BT] wake_unlock(bt_wake_lock)\n");
			wake_unlock(&bt_wake_lock);
			break;

		case RFKILL_USER_STATE_SOFT_BLOCKED:
			bt_is_running = 1;
			gpio_set_value(GPIO_BT_WAKE, 1);
			pr_debug("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
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
	int ret =0;
	
	ret = bluetooth_set_sleep(data, blocked?
			RFKILL_USER_STATE_SOFT_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);
		
	return ret;
}

static const struct rfkill_ops btsleep_rfkill_ops = {
	.set_block = btsleep_rfkill_set_block,
};
#endif

#ifdef USE_LOCK_DVFS
static int bluetooth_lock_dvfs(void *data, enum rfkill_user_states state)
{
	if(!gpio_get_value(GPIO_BT_nRST))
		return 0;
	
	switch (state) {
		case RFKILL_USER_STATE_UNBLOCKED:
			s5pv210_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_9);
			pr_debug("[BT] dvfs unlock\n");
			break;
		case RFKILL_USER_STATE_SOFT_BLOCKED:
			s5pv210_lock_dvfs_high_level(DVFS_LOCK_TOKEN_9, L3);
			pr_debug("[BT] dvfs lock to L3\n");
			break;
		case RFKILL_USER_STATE_HARD_BLOCKED:
			s5pv210_lock_dvfs_high_level(DVFS_LOCK_TOKEN_9, L2);
			pr_debug("[BT] dvfs lock to L2\n");
			break;			
		default:
			pr_err("[BT] bad bluetooth rfkill state %d\n", state);
	}
	return 0;
}

static int bt_lock_dvfs_rfkill_set_block(void *data, bool blocked)
{
	int ret =0;
	
	ret = bluetooth_lock_dvfs(data, blocked?
			RFKILL_USER_STATE_SOFT_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);
		
	return ret;
}

static int bt_lock_dvfs_l2_rfkill_set_block(void *data, bool blocked)
{
	int ret =0;
	
	ret = bluetooth_lock_dvfs(data, blocked?
			RFKILL_USER_STATE_HARD_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);
		
	return ret;
}


static const struct rfkill_ops bt_lock_dvfs_rfkill_ops = {
	.set_block = bt_lock_dvfs_rfkill_set_block,
};


static const struct rfkill_ops bt_lock_dvfs_l2_rfkill_ops = {
	.set_block = bt_lock_dvfs_l2_rfkill_set_block,
};
#endif


static int __init aries_rfkill_probe(struct platform_device *pdev)
{
	int irq;
	int ret;

	/* Initialize wake locks */
	wake_lock_init(&rfkill_wake_lock, WAKE_LOCK_SUSPEND, "bt_host_wake");

	ret = gpio_request(GPIO_WLAN_BT_EN, "GPB");
	if (ret < 0) {
		pr_err("[BT] Failed to request GPIO_WLAN_BT_EN!\n");
		goto err_req_gpio_wlan_bt_en;
	}

	ret = gpio_request(GPIO_BT_nRST, "GPB");
	if (ret < 0) {
		pr_err("[BT] Failed to request GPIO_BT_nRST!\n");
		goto err_req_gpio_bt_nrst;
	}

	/* BT Host Wake IRQ */
	irq = IRQ_BT_HOST_WAKE;

	ret = request_irq(irq, bt_host_wake_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"bt_host_wake_irq_handler", NULL);

	if (ret < 0) {
		pr_err("[BT] Request_irq failed\n");
		goto err_req_irq;
	}

	disable_irq(irq);

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
		pr_err("********ERROR IN REGISTERING THE bt_rfk********\n");
		goto err_register;
	}

	rfkill_set_sw_state(bt_rfk, 1);
	bluetooth_set_power(NULL, RFKILL_USER_STATE_SOFT_BLOCKED);

#ifdef BT_SLEEP_ENABLE
	wake_lock_init(&bt_wake_lock, WAKE_LOCK_SUSPEND, "bt_wake");

	ret = gpio_request(GPIO_BT_WAKE, "gpio_bt_wake");
	if (ret < 0) {
		pr_err("[BT] Failed to request GPIO_BT_WAKE\n");
		goto err_req_gpio_bt_wake;
	}

	gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

	bt_sleep_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&btsleep_rfkill_ops, NULL);

	if (!bt_sleep_rfk) {
		pr_err("[BT] bt_sleep_rfk : rfkill_alloc is failed\n");
		ret = -ENOMEM;
		goto err_sleep_alloc;
	}

	rfkill_set_sw_state(bt_sleep_rfk, 1);

	pr_debug("[BT] rfkill_register(bt_sleep_rfk)\n");

	ret = rfkill_register(bt_sleep_rfk);
	if (ret) {
		pr_err("********ERROR IN REGISTERING THE bt_sleep_rfk********\n");
		goto err_sleep_register;
	}
#endif

#ifdef USE_LOCK_DVFS
	bt_lock_dvfs_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&bt_lock_dvfs_rfkill_ops, NULL);

	if (!bt_lock_dvfs_rfk) {
		pr_err("[BT] bt_lock_dvfs_rfk : rfkill_alloc is failed\n");
		ret = -ENOMEM;
		goto err_dvfs_lock_alloc;
	}

	pr_debug("[BT] rfkill_register(bt_lock_dvfs_rfk)\n");

	ret = rfkill_register(bt_lock_dvfs_rfk);
	if (ret) {
		pr_err("********ERROR IN REGISTERING THE bt_lock_dvfs_rfk********\n");
		goto err_lock_dvfs_register;
	}

	bt_lock_dvfs_l2_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&bt_lock_dvfs_l2_rfkill_ops, NULL);

	if (!bt_lock_dvfs_l2_rfk) {
		pr_err("[BT] bt_lock_dvfs_l2_rfk : rfkill_alloc is failed\n");
		ret = -ENOMEM;
		goto err_dvfs_l2_lock_alloc;
	}

	pr_debug("[BT] rfkill_register(bt_lock_dvfs_l2_rfk)\n");

	ret = rfkill_register(bt_lock_dvfs_l2_rfk);
	if (ret) {
		pr_err("********ERROR IN REGISTERING THE bt_lock_dvfs_l2_rfk********\n");
		goto err_lock_dvfs_l2_register;
	}	
#endif
	return ret;

#ifdef USE_LOCK_DVFS
err_lock_dvfs_l2_register:
	rfkill_destroy(bt_lock_dvfs_l2_rfk);

err_dvfs_l2_lock_alloc:
	rfkill_unregister(bt_lock_dvfs_rfk);
	
err_lock_dvfs_register:
	rfkill_destroy(bt_lock_dvfs_rfk);

err_dvfs_lock_alloc:
	rfkill_unregister(bt_sleep_rfk);
#endif

#ifdef BT_SLEEP_ENABLE
err_sleep_register:
	rfkill_destroy(bt_sleep_rfk);

err_sleep_alloc:
	gpio_free(GPIO_BT_WAKE);
	
err_req_gpio_bt_wake:
	rfkill_unregister(bt_rfk);
#endif

 err_register:
	rfkill_destroy(bt_rfk);

 err_alloc:
	free_irq(irq, NULL);

 err_req_irq:
	gpio_free(GPIO_BT_nRST);

 err_req_gpio_bt_nrst:
	gpio_free(GPIO_WLAN_BT_EN);

 err_req_gpio_wlan_bt_en:
	return ret;
}

static struct platform_driver aries_device_rfkill = {
	.probe = aries_rfkill_probe,
	.driver = {
		.name = "bt_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init aries_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&aries_device_rfkill);

	bt_is_running = 0;

	return rc;
}

module_init(aries_rfkill_init);
MODULE_DESCRIPTION("aries rfkill");
MODULE_LICENSE("GPL");
