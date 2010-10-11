/* drivers/rtc/rtc-s3c.c
 *
 * Copyright (c) 2004,2006 Simtec Electronics
 *	Ben Dooks, <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410/S3C2440/S3C24XX Internal RTC Driver
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach/time.h>
#include <plat/regs-rtc.h>

#include "rtc-s3c.h"

enum s3c_cpu_type {
	TYPE_S3C2410,
	TYPE_S3C64XX,
};

/* I have yet to find an S3C implementation with more than one
 * of these rtc blocks in */

static struct resource *s3c_rtc_mem;

static void __iomem *s3c_rtc_base;
static int s3c_rtc_alarmno = NO_IRQ;
static int s3c_rtc_tickno  = NO_IRQ;
static int s3c_rtc_freq    = 1;
static enum s3c_cpu_type s3c_rtc_cpu_type;

static DEFINE_SPINLOCK(s3c_rtc_pie_lock);
static unsigned int tick_count;

static void s3c_rtc_enable(struct device *dev, int en);

static int s3c_rtc_settime(struct device *dev, struct rtc_time *tm);
/* IRQ Handlers */

static irqreturn_t s3c_rtc_alarmirq(int irq, void *id)
{
	struct rtc_device *rdev = id;

	rtc_update_irq(rdev, 1, RTC_AF | RTC_IRQF);

	writeb(S3C_INTP_ALM, s3c_rtc_base + S3C_INTP);

	return IRQ_HANDLED;
}

#ifndef CONFIG_HRT_RTC
static irqreturn_t s3c_rtc_tickirq(int irq, void *id)
{
	struct rtc_device *rdev = id;

	rtc_update_irq(rdev, 1, RTC_PF | RTC_IRQF);

	writeb(S3C_INTP_TIC, s3c_rtc_base + S3C_INTP);

	return IRQ_HANDLED;
}
#endif

/* Update control registers */
static void s3c_rtc_setaie(int to)
{
	unsigned int tmp;

	pr_debug("%s: aie=%d\n", __func__, to);

	tmp = readb(s3c_rtc_base + S3C2410_RTCALM) & ~S3C2410_RTCALM_ALMEN;

	if (to)
		tmp |= S3C2410_RTCALM_ALMEN;

	writeb(tmp, s3c_rtc_base + S3C2410_RTCALM);
}

static int s3c_rtc_setpie(struct device *dev, int enabled)
{
	unsigned int tmp;

	pr_debug("%s: pie=%d\n", __func__, enabled);

	spin_lock_irq(&s3c_rtc_pie_lock);

	if (s3c_rtc_cpu_type == TYPE_S3C64XX) {
		tmp = readw(s3c_rtc_base + S3C2410_RTCCON);
		tmp &= ~S3C64XX_RTCCON_TICEN;

		if (enabled)
			tmp |= S3C64XX_RTCCON_TICEN;

		writew(tmp, s3c_rtc_base + S3C2410_RTCCON);
	} else {
		tmp = readw(s3c_rtc_base + S3C2410_TICNT);
		tmp &= ~S3C2410_TICNT_ENABLE;

		if (enabled)
			tmp |= S3C2410_TICNT_ENABLE;

		writew(tmp, s3c_rtc_base + S3C2410_TICNT);
	}

	spin_unlock_irq(&s3c_rtc_pie_lock);

	return 0;
}

static int s3c_rtc_setfreq(struct device *dev, int freq)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);
	unsigned int tmp = 0;

	if (!is_power_of_2(freq))
		return -EINVAL;

	spin_lock_irq(&s3c_rtc_pie_lock);

	if (s3c_rtc_cpu_type == TYPE_S3C2410) {
		tmp = readw(s3c_rtc_base + S3C2410_TICNT);
		tmp &= S3C2410_TICNT_ENABLE;
	}

	tmp |= (rtc_dev->max_user_freq / freq)-1;

	writel(tmp, s3c_rtc_base + S3C2410_TICNT);
	spin_unlock_irq(&s3c_rtc_pie_lock);

	return 0;
}

/* Time read/write */

static int s3c_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int have_retried = 0;
	void __iomem *base = s3c_rtc_base;

	s3c_rtc_enable(dev, 1);
 retry_get_time:
	rtc_tm->tm_sec  = readb(base + S3C2410_RTCSEC);
	rtc_tm->tm_min  = readb(base + S3C2410_RTCMIN);
	rtc_tm->tm_hour = readb(base + S3C2410_RTCHOUR);
	rtc_tm->tm_mday = readb(base + S3C2410_RTCDATE);
	rtc_tm->tm_mon  = readb(base + S3C2410_RTCMON);

#if defined(CONFIG_CPU_S5PV210)
	rtc_tm->tm_year = readl(base + S3C2410_RTCYEAR);
#else
	rtc_tm->tm_year = readb(base + S3C2410_RTCYEAR);
#endif


	/* the only way to work out wether the system was mid-update
	 * when we read it is to check the second counter, and if it
	 * is zero, then we re-try the entire read
	 */

	if (rtc_tm->tm_sec == 0 && !have_retried) {
		have_retried = 1;
		goto retry_get_time;
	}
	s3c_rtc_enable(dev, 0);

	pr_debug("read time %02x.%02x.%02x %02x/%02x/%02x\n",
		 rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
		 rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	rtc_tm->tm_sec = bcd2bin(rtc_tm->tm_sec);
	rtc_tm->tm_min = bcd2bin(rtc_tm->tm_min);
	rtc_tm->tm_hour = bcd2bin(rtc_tm->tm_hour);
	rtc_tm->tm_mday = bcd2bin(rtc_tm->tm_mday);
	rtc_tm->tm_mon = bcd2bin(rtc_tm->tm_mon);
#if defined(CONFIG_CPU_S5PV210)
	rtc_tm->tm_year = bcd2bin(rtc_tm->tm_year & 0xff) +
			  bcd2bin(rtc_tm->tm_year >> 8) * 100;
#else
	rtc_tm->tm_year = bcd2bin(rtc_tm->tm_year);
	rtc_tm->tm_year += 100;
#endif

	rtc_tm->tm_mon -= 1;

	return 0;
}

static int s3c_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	void __iomem *base = s3c_rtc_base;
#if defined(CONFIG_CPU_S5PV210)
	int year = tm->tm_year;
	int year100;
#else
	int year = tm->tm_year - 100;
#endif

	pr_debug("set time %02d.%02d.%02d %02d/%02d/%02d\n",
		 tm->tm_year, tm->tm_mon, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* we get around y2k by simply not supporting it */


#if defined(CONFIG_CPU_S5PV210)
	if (year < 0 || year >= 1000) {
		dev_err(dev, "rtc only supports 0~999 years\n");
#else
	if (year < 0 || year >= 100) {
		dev_err(dev, "rtc only supports 100 years\n");
#endif
		return -EINVAL;
	}

	s3c_rtc_enable(dev, 1);

	writeb(bin2bcd(tm->tm_sec),  base + S3C2410_RTCSEC);
	writeb(bin2bcd(tm->tm_min),  base + S3C2410_RTCMIN);
	writeb(bin2bcd(tm->tm_hour), base + S3C2410_RTCHOUR);
	writeb(bin2bcd(tm->tm_mday), base + S3C2410_RTCDATE);
	writeb(bin2bcd(tm->tm_mon + 1), base + S3C2410_RTCMON);

#if defined(CONFIG_CPU_S5PV210)
	year100 = year/100;
	year = year%100;
	year = bin2bcd(year) | ((bin2bcd(year100)) << 8);
	year = (0x00000fff & year);
	pr_debug("year %x\n", year);
	writel(year, base + S3C2410_RTCYEAR);
#else
	writeb(bin2bcd(year), base + S3C2410_RTCYEAR);
#endif
	s3c_rtc_enable(dev, 0);

	max8998_rtc_set_time(tm);

	return 0;
}

static int s3c_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *alm_tm = &alrm->time;
	void __iomem *base = s3c_rtc_base;
	unsigned int alm_en;

	s3c_rtc_enable(dev, 1);

	alm_tm->tm_sec  = readb(base + S3C2410_ALMSEC);
	alm_tm->tm_min  = readb(base + S3C2410_ALMMIN);
	alm_tm->tm_hour = readb(base + S3C2410_ALMHOUR);
	alm_tm->tm_mon  = readb(base + S3C2410_ALMMON);
	alm_tm->tm_mday = readb(base + S3C2410_ALMDATE);
#if defined(CONFIG_CPU_S5PV210)
	alm_tm->tm_year = readl(base + S3C2410_ALMYEAR);
	alm_tm->tm_year = (0x00000fff & alm_tm->tm_year);
#else
	alm_tm->tm_year = readb(base + S3C2410_ALMYEAR);
#endif

	s3c_rtc_enable(dev, 0);

	alm_en = readb(base + S3C2410_RTCALM);

	alrm->enabled = (alm_en & S3C2410_RTCALM_ALMEN) ? 1 : 0;

	pr_debug("read alarm %02x %02x.%02x.%02x %02x/%02x/%02x\n",
		 alm_en,
		 alm_tm->tm_year, alm_tm->tm_mon, alm_tm->tm_mday,
		 alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);


	/* decode the alarm enable field */

	if (alm_en & S3C2410_RTCALM_SECEN)
		alm_tm->tm_sec = bcd2bin(alm_tm->tm_sec);
	else
		alm_tm->tm_sec = 0xff;

	if (alm_en & S3C2410_RTCALM_MINEN)
		alm_tm->tm_min = bcd2bin(alm_tm->tm_min);
	else
		alm_tm->tm_min = 0xff;

	if (alm_en & S3C2410_RTCALM_HOUREN)
		alm_tm->tm_hour = bcd2bin(alm_tm->tm_hour);
	else
		alm_tm->tm_hour = 0xff;

	if (alm_en & S3C2410_RTCALM_DAYEN)
		alm_tm->tm_mday = bcd2bin(alm_tm->tm_mday);
	else
		alm_tm->tm_mday = 0xff;

	if (alm_en & S3C2410_RTCALM_MONEN) {
		alm_tm->tm_mon = bcd2bin(alm_tm->tm_mon);
		alm_tm->tm_mon -= 1;
	} else {
		alm_tm->tm_mon = 0xff;
	}

	if (alm_en & S3C2410_RTCALM_YEAREN) {
#if defined(CONFIG_CPU_S5PV210)
		alm_tm->tm_year = bcd2bin(alm_tm->tm_year & 0xff) +
			  bcd2bin(alm_tm->tm_year >> 8) * 100;
#else
		alm_tm->tm_year = bcd2bin(alm_tm->tm_year);
#endif
	} else {
		alm_tm->tm_year = 0xffff;
	}

	return 0;
}

static int s3c_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	void __iomem *base = s3c_rtc_base;
	unsigned int alrm_en;

#if defined(CONFIG_CPU_S5PV210)
	int year = tm->tm_year;
	int year100;
#else
	int year = tm->tm_year - 100;
#endif

	pr_debug("s3c_rtc_setalarm: %d, %02x/%02x/%02x %02x.%02x.%02x\n",
		 alrm->enabled,
		 tm->tm_mday & 0xff, tm->tm_mon & 0xff, tm->tm_year & 0xff,
		 tm->tm_hour & 0xff, tm->tm_min & 0xff, tm->tm_sec);


	s3c_rtc_enable(dev, 1);

	alrm_en = readb(base + S3C2410_RTCALM) & S3C2410_RTCALM_ALMEN;
	writeb(0x00, base + S3C2410_RTCALM);

	if (tm->tm_sec < 60 && tm->tm_sec >= 0) {
		alrm_en |= S3C2410_RTCALM_SECEN;
		writeb(bin2bcd(tm->tm_sec), base + S3C2410_ALMSEC);
	}

	if (tm->tm_min < 60 && tm->tm_min >= 0) {
		alrm_en |= S3C2410_RTCALM_MINEN;
		writeb(bin2bcd(tm->tm_min), base + S3C2410_ALMMIN);
	}

	if (tm->tm_hour < 24 && tm->tm_hour >= 0) {
		alrm_en |= S3C2410_RTCALM_HOUREN;
		writeb(bin2bcd(tm->tm_hour), base + S3C2410_ALMHOUR);
	}

	if (tm->tm_mday >= 0) {
		alrm_en |= S3C2410_RTCALM_DAYEN;
		writeb(bin2bcd(tm->tm_mday), base + S3C2410_ALMDATE);
	}

	if (tm->tm_mon < 13 && tm->tm_mon >= 0) {
		alrm_en |= S3C2410_RTCALM_MONEN;
		writeb(bin2bcd(tm->tm_mon + 1), base + S3C2410_ALMMON);
	}

#if defined(CONFIG_CPU_S5PV210)
	if (year < 1000 && year >= 0) {
		alrm_en |= S3C2410_RTCALM_YEAREN;
		year100 = year/100;
		year = year%100;
		year = bin2bcd(year) | ((bin2bcd(year100)) << 8);
		year = (0x00000fff & year);
		pr_debug("year %x\n", year);
		writel(year, base + S3C2410_ALMYEAR);
	}
#else
	if (year < 100 && year >= 0) {
		alrm_en |= S3C2410_RTCALM_YEAREN;
		writeb(bin2bcd(year), base + S3C2410_ALMYEAR);
	}
#endif

	s3c_rtc_enable(dev, 0);

	pr_debug("setting S3C2410_RTCALM to %08x\n", alrm_en);

	writeb(alrm_en, base + S3C2410_RTCALM);

	s3c_rtc_setaie(alrm->enabled);

	return 0;
}

static int s3c_rtc_ioctl(struct device *dev,
			 unsigned int cmd, unsigned long arg)
{
	unsigned int ret = -ENOIOCTLCMD;

	switch (cmd) {
	case RTC_AIE_OFF:
	case RTC_AIE_ON:
		s3c_rtc_setaie((cmd == RTC_AIE_ON) ? 1 : 0);
		ret = 0;
		break;

	case RTC_PIE_OFF:
	case RTC_PIE_ON:
		tick_count = 0;
		s3c_rtc_setpie(dev, (cmd == RTC_PIE_ON) ? 1 : 0);
		ret = 0;
		break;

	case RTC_IRQP_READ:
		ret = put_user(s3c_rtc_freq, (unsigned long __user *)arg);
		break;

	case RTC_IRQP_SET:
		/* check for power of 2 */

		if ((arg & (arg-1)) != 0 || arg < 1) {
			ret = -EINVAL;
			goto exit;
		}

		pr_debug("s3c2410_rtc: setting frequency %ld\n", arg);

		s3c_rtc_setfreq(dev, arg);
		ret = 0;
		break;

	case RTC_UIE_ON:
	case RTC_UIE_OFF:
		ret = -EINVAL;
}

 exit:
	return ret;
}
static int s3c_rtc_proc(struct device *dev, struct seq_file *seq)
{
	unsigned int ticnt = readw(s3c_rtc_base + S3C2410_TICNT);

	seq_printf(seq, "periodic_IRQ\t: %s\n",
		     (ticnt & S3C2410_TICNT_ENABLE) ? "yes" : "no");
	return 0;
}

static int s3c_rtc_open(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);
	int ret;

	ret = request_irq(s3c_rtc_alarmno, s3c_rtc_alarmirq,
			  IRQF_DISABLED,  "s3c2410-rtc alarm", rtc_dev);

	if (ret) {
		dev_err(dev, "IRQ%d error %d\n", s3c_rtc_alarmno, ret);
		return ret;
	}

#ifndef CONFIG_HRT_RTC
	ret = request_irq(s3c_rtc_tickno, s3c_rtc_tickirq,
			  IRQF_DISABLED,  "s3c2410-rtc tick", rtc_dev);

	if (ret) {
		dev_err(dev, "IRQ%d error %d\n", s3c_rtc_tickno, ret);
		goto tick_err;
	}
#endif

	return ret;

#ifndef CONFIG_HRT_RTC
 tick_err:
	free_irq(s3c_rtc_alarmno, rtc_dev);
	return ret;
#endif
}

static void s3c_rtc_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);

#ifndef CONFIG_HRT_RTC
	/* do not clear AIE here, it may be needed for wake */

	s3c_rtc_setpie(dev, 0);
	free_irq(s3c_rtc_alarmno, rtc_dev);
	free_irq(s3c_rtc_tickno, rtc_dev);
#endif
	free_irq(s3c_rtc_alarmno, rtc_dev);

}

static const struct rtc_class_ops s3c_rtcops = {
	.open		= s3c_rtc_open,
	.release	= s3c_rtc_release,
	.ioctl		= s3c_rtc_ioctl,
	.read_time	= s3c_rtc_gettime,
	.set_time	= s3c_rtc_settime,
	.read_alarm	= s3c_rtc_getalarm,
	.set_alarm	= s3c_rtc_setalarm,
	.irq_set_freq	= s3c_rtc_setfreq,
	.irq_set_state	= s3c_rtc_setpie,
	.proc	        = s3c_rtc_proc,
};

static void s3c_rtc_enable(struct device *dev, int en)
{
	void __iomem *base = s3c_rtc_base;
	unsigned int tmp;

	if (s3c_rtc_base == NULL)
		return;

	if (!en) {
		tmp = readw(base + S3C2410_RTCCON);
#ifdef CONFIG_HRT_RTC
		writew(tmp & ~(S3C2410_RTCCON_RTCEN), base + S3C2410_RTCCON);
#else
		writew(tmp & ~(S3C2410_RTCCON_RTCEN | S3C_RTCCON_TICEN),
			     base + S3C2410_RTCCON);
#endif /* CONFIG_HRT_RTC */
	} else {
		/* re-enable the device, and check it is ok */

		if ((readw(base+S3C2410_RTCCON) & S3C2410_RTCCON_RTCEN) == 0) {
			dev_info(dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp|S3C2410_RTCCON_RTCEN, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CNTSEL)) {
			dev_info(dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp & ~S3C2410_RTCCON_CNTSEL,
				 base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CLKRST)) {
			dev_info(dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp & ~S3C2410_RTCCON_CLKRST,
				     base+S3C2410_RTCCON);
		}
	}
}

static int __devexit s3c_rtc_remove(struct platform_device *dev)
{
	struct rtc_device *rtc = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);
	rtc_device_unregister(rtc);

	s3c_rtc_setpie(&dev->dev, 0);
	s3c_rtc_setaie(0);

	iounmap(s3c_rtc_base);
	release_resource(s3c_rtc_mem);
	kfree(s3c_rtc_mem);

	return 0;
}

static int __devinit s3c_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	struct resource *res;
	unsigned char bcd_tmp, bcd_loop;
	int ret;
	struct rtc_time tm;
	int year;
#if defined(CONFIG_CPU_S5PV210)
	int year100;
#endif

	pr_debug("%s: probe=%p\n", __func__, pdev);

	/* find the IRQs */

	s3c_rtc_tickno = platform_get_irq(pdev, 1);
	if (s3c_rtc_tickno < 0) {
		dev_err(&pdev->dev, "no irq for rtc tick\n");
		return -ENOENT;
	}

	s3c_rtc_alarmno = platform_get_irq(pdev, 0);
	if (s3c_rtc_alarmno < 0) {
		dev_err(&pdev->dev, "no irq for alarm\n");
		return -ENOENT;
	}

	pr_debug("s3c2410_rtc: tick irq %d, alarm irq %d\n",
		 s3c_rtc_tickno, s3c_rtc_alarmno);

	/* get the memory region */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		return -ENOENT;
	}

	s3c_rtc_mem = request_mem_region(res->start,
					 res->end-res->start+1,
					 pdev->name);

	if (s3c_rtc_mem == NULL) {
		dev_err(&pdev->dev, "failed to reserve memory region\n");
		ret = -ENOENT;
		goto err_nores;
	}

	s3c_rtc_base = ioremap(res->start, res->end - res->start + 1);
	if (s3c_rtc_base == NULL) {
		dev_err(&pdev->dev, "failed ioremap()\n");
		ret = -EINVAL;
		goto err_nomap;
	}

	/* check to see if everything is setup correctly */


	pr_debug("s3c2410_rtc: RTCCON=%02x\n",
		readw(s3c_rtc_base + S3C2410_RTCCON));
#ifdef CONFIG_PM
	s3c_rtc_setfreq(&pdev->dev, 0);
#else
	s3c_rtc_setfreq(&pdev->dev, 1);
#endif

	device_init_wakeup(&pdev->dev, 1);


	s3c_rtc_cpu_type = platform_get_device_id(pdev)->driver_data;

	max8998_rtc_read_time(&tm);

	/* update time from pmic */


#if defined(CONFIG_CPU_S5PV210)
	year = tm.tm_year;
#else
	year = tm.tm_year - 100;
#endif

#if defined(CONFIG_CPU_S5PV210)
	if (year < 0 || year >= 1000) {
		dev_err(&pdev->dev, "rtc only supports 0 ~ 999 years\n");
#else
	if (year < 0 || year >= 100) {
		dev_err(&pdev->dev, "rtc only supports 100 years\n");
#endif
		/* Set the default time. 2010:1:1:12:0:0 */
		year = 110;
		tm.tm_mon = 0;
		tm.tm_mday = 1;
		tm.tm_hour = 12;
		tm.tm_min = 0;
		tm.tm_sec = 0;
	}

	s3c_rtc_enable(&pdev->dev, 1);

	writeb(bin2bcd(tm.tm_sec),  s3c_rtc_base + S3C2410_RTCSEC);
	writeb(bin2bcd(tm.tm_min),  s3c_rtc_base + S3C2410_RTCMIN);
	writeb(bin2bcd(tm.tm_hour), s3c_rtc_base + S3C2410_RTCHOUR);
	writeb(bin2bcd(tm.tm_mday), s3c_rtc_base + S3C2410_RTCDATE);
	writeb(bin2bcd(tm.tm_mon + 1), s3c_rtc_base + S3C2410_RTCMON);

#if defined(CONFIG_CPU_S5PV210)
	year100 = year/100;
	year = year%100;
	year = bin2bcd(year) | ((bin2bcd(year100)) << 8);
	year = (0x00000fff & year);
	pr_debug("year %x\n", year);
	writel(year, s3c_rtc_base + S3C2410_RTCYEAR);
#else
	writeb(bin2bcd(year), s3c_rtc_base + S3C2410_RTCYEAR);
#endif

	/* check rtc time */
	for (bcd_loop = S3C2410_RTCSEC; bcd_loop <= S3C2410_RTCYEAR
					; bcd_loop += 0x4)	{
		bcd_tmp = readw(s3c_rtc_base + bcd_loop);
		if (((bcd_tmp & 0xf) > 0x9) || ((bcd_tmp & 0xf0) > 0x90))
			writew(0, s3c_rtc_base + bcd_loop);
	}

	s3c_rtc_enable(&pdev->dev, 0);

	/* register RTC and exit */

	rtc = rtc_device_register("s3c", &pdev->dev, &s3c_rtcops,
				  THIS_MODULE);

	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		goto err_nortc;
	}

	if (s3c_rtc_cpu_type == TYPE_S3C64XX)
		rtc->max_user_freq = 32768;
	else
		rtc->max_user_freq = 128;

	platform_set_drvdata(pdev, rtc);

	s3c_rtc_setfreq(&pdev->dev, 1);

	return 0;

 err_nortc:
	iounmap(s3c_rtc_base);

 err_nomap:
	release_resource(s3c_rtc_mem);

 err_nores:
	return ret;
}

#ifdef CONFIG_PM

/* RTC Power management control */

static int ticnt_save;

static int s3c_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct timespec time;

	time.tv_nsec = 0;
	/* save TICNT for anyone using periodic interrupts */
	ticnt_save = readl(s3c_rtc_base + S3C2410_TICNT);

	if (device_may_wakeup(&pdev->dev))
		enable_irq_wake(s3c_rtc_alarmno);

	return 0;
}

static int s3c_rtc_resume(struct platform_device *pdev)
{
	struct timespec time;

	time.tv_nsec = 0;

	writel(ticnt_save, s3c_rtc_base + S3C2410_TICNT);

	if (device_may_wakeup(&pdev->dev))
		disable_irq_wake(s3c_rtc_alarmno);

	return 0;
}
#else
#define s3c_rtc_suspend NULL
#define s3c_rtc_resume  NULL
#endif

static struct platform_device_id s3c_rtc_driver_ids[] = {
	{
		.name		= "s3c2410-rtc",
		.driver_data	= TYPE_S3C2410,
	}, {
		.name		= "s3c64xx-rtc",
		.driver_data	= TYPE_S3C64XX,
	},
	{ }
};

MODULE_DEVICE_TABLE(platform, s3c_rtc_driver_ids);

static struct platform_driver s3c_rtc_driver = {
	.probe		= s3c_rtc_probe,
	.remove		= __devexit_p(s3c_rtc_remove),
	.suspend	= s3c_rtc_suspend,
	.resume		= s3c_rtc_resume,
	.id_table	= s3c_rtc_driver_ids,
	.driver		= {
		.name	= "s3c-rtc",
		.owner	= THIS_MODULE,
	},
};

static char __initdata banner[] = "S3C24XX RTC, (c) 2004,2006 "
				  "Simtec Electronics\n";

static int __init s3c_rtc_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c_rtc_driver);
}

static void __exit s3c_rtc_exit(void)
{
	platform_driver_unregister(&s3c_rtc_driver);
}

module_init(s3c_rtc_init);
module_exit(s3c_rtc_exit);

MODULE_DESCRIPTION("Samsung S3C RTC Driver");
MODULE_AUTHOR("Ben Dooks <ben@simtec.co.uk>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c2410-rtc");
