/* arch/arm/mach-s5pv210/herring-vibrator.c
 *
 * Copyright (C) 2010 Samsung Electronics Co. Ltd. All Rights Reserved.
 * Author: Rom Lemarchand <rlemarchand@sta.samsung.com>
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

#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/workqueue.h>

#include <asm/mach-types.h>

#include <../../../drivers/staging/android/timed_output.h>

#include <mach/gpio-herring.h>

#define GPD0_TOUT_1		2 << 4

#define PWM_PERIOD		(89284 / 2)
#define PWM_DUTY		(87280 / 2)
#define MAX_TIMEOUT		10000 /* 10s */

static struct vibrator {
	struct wake_lock wklock;
	struct pwm_device *pwm_dev;
	struct hrtimer timer;
	struct mutex lock;
	struct work_struct work;
} vibdata;

static void herring_vibrator_off(void)
{
	pwm_disable(vibdata.pwm_dev);
	gpio_direction_output(GPIO_VIBTONE_EN1, GPIO_LEVEL_LOW);
	wake_unlock(&vibdata.wklock);
}

static int herring_vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibdata.timer)) {
		ktime_t r = hrtimer_get_remaining(&vibdata.timer);
		return ktime_to_ms(r);
	}

	return 0;
}

static void herring_vibrator_enable(struct timed_output_dev *dev, int value)
{
	mutex_lock(&vibdata.lock);

	/* cancel previous timer and set GPIO according to value */
	hrtimer_cancel(&vibdata.timer);
	cancel_work_sync(&vibdata.work);
	if (value) {
		wake_lock(&vibdata.wklock);
		pwm_config(vibdata.pwm_dev, PWM_DUTY, PWM_PERIOD);
		pwm_enable(vibdata.pwm_dev);
		gpio_direction_output(GPIO_VIBTONE_EN1, GPIO_LEVEL_HIGH);

		if (value > 0) {
			if (value > MAX_TIMEOUT)
				value = MAX_TIMEOUT;

			hrtimer_start(&vibdata.timer,
				ns_to_ktime((u64)value * NSEC_PER_MSEC),
				HRTIMER_MODE_REL);
		}
	} else
		herring_vibrator_off();

	mutex_unlock(&vibdata.lock);
}

static struct timed_output_dev to_dev = {
	.name		= "vibrator",
	.get_time	= herring_vibrator_get_time,
	.enable		= herring_vibrator_enable,
};

static enum hrtimer_restart herring_vibrator_timer_func(struct hrtimer *timer)
{
	schedule_work(&vibdata.work);
	return HRTIMER_NORESTART;
}

static void herring_vibrator_work(struct work_struct *work)
{
	herring_vibrator_off();
}

static int __init herring_init_vibrator(void)
{
	int ret = 0;

#ifdef CONFIG_MACH_HERRING
	if (!machine_is_herring())
		return 0;
#endif
	hrtimer_init(&vibdata.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibdata.timer.function = herring_vibrator_timer_func;
	INIT_WORK(&vibdata.work, herring_vibrator_work);

	ret = gpio_request(GPIO_VIBTONE_EN1, "vibrator-en");
	if (ret < 0)
		return ret;

	s3c_gpio_cfgpin(GPIO_VIBTONE_PWM, GPD0_TOUT_1);

	vibdata.pwm_dev = pwm_request(1, "vibrator-pwm");
	if (IS_ERR(vibdata.pwm_dev)) {
		ret = PTR_ERR(vibdata.pwm_dev);
		goto err_pwm_req;
	}

	wake_lock_init(&vibdata.wklock, WAKE_LOCK_SUSPEND, "vibrator");
	mutex_init(&vibdata.lock);

	ret = timed_output_dev_register(&to_dev);
	if (ret < 0)
		goto err_to_dev_reg;

	return 0;

err_to_dev_reg:
	mutex_destroy(&vibdata.lock);
	wake_lock_destroy(&vibdata.wklock);
	pwm_free(vibdata.pwm_dev);
err_pwm_req:
	gpio_free(GPIO_VIBTONE_EN1);
	return ret;
}

device_initcall(herring_init_vibrator);
