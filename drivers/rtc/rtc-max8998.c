
/*
 * rtc-max8998.c 
 *
 * PMIC rtc utility driver 
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <linux/rtc.h>
#include <linux/bcd.h>


#include <linux/i2c.h>






#define MAX8998_REG_SECOND		0
#define MAX8998_REG_MIN			1	
#define MAX8998_REG_HOUR		2
#define MAX8998_REG_WEEKDAY		3
#define MAX8998_REG_DATE		4	
#define MAX8998_REG_MONTH		5
#define MAX8998_REG_YEAR0		6
#define MAX8998_REG_YEAR1		7


#define MAX8998_RTC_LEN			8


#define MAX8998_RTC_TIME_ADDR		0
#define MAX8998_RTC_ALARMO_ADDR		8
#define MAX8998_RTC_ALARM1_ADDR		8


#define MAX8998RTC_DEBUG    0

#if MAX8998RTC_DEBUG    
#define max8998_dbg	pr_info
#else

#define	max8998_dbg(...)	
#endif


/* Slave address */
#define MAX8998_RTC_SLAVE_ADDR	0x0D

/* Register address */
static struct i2c_driver max8998_rtc_i2c_driver;
static struct i2c_client *max8998_rtc_i2c_client = NULL;

unsigned short max8998_rtc_ignore[] = {I2C_CLIENT_END };
static unsigned short max8998_rtc_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short max8998_rtc_probe[] = {6, MAX8998_RTC_SLAVE_ADDR >> 1, I2C_CLIENT_END };
/*
static struct i2c_client_address_data max8998_rtc_addr_data = {
	.normal_i2c	= max8998_rtc_normal_i2c,
	.ignore		= max8998_rtc_ignore,
	.probe		= max8998_rtc_probe,
};
*/

static int max8998_rtc_i2c_read(struct i2c_client *client, u8 reg, u8 *data, int len)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[2];

	max8998_dbg("%s\n", __func__);

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = data;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

#if MAX8998RTC_DEBUG
	int i;
	for(i = 0; i < len ; i++)
		max8998_dbg(" %d : %x \n",i,*(data + i));
#endif

	return 0;
}

static int max8998_rtc_i2c_write(struct i2c_client *client, u8 reg, u8 *data,int len)
{
	int ret;
	struct i2c_msg msg[1];

	max8998_dbg("%s\n", __func__);

	data[0] = reg; 

#if MAX8998RTC_DEBUG
	int i;
	for(i = 0; i < len ; i++)
		max8998_dbg(" %d : %x \n",i,*(data + i));
#endif

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = len;
	msg[0].buf = data;


	ret = i2c_transfer(client->adapter, msg, 1);
	max8998_dbg(" max8998 i2c write ret %d\n",ret);

	if (ret != 1) 
		return -EIO;


	return 0;
}


static int max8998_rtc_i2c_read_time(struct i2c_client *client, struct rtc_time *tm)
{
        int ret;
        u8 regs[MAX8998_RTC_LEN];

        ret = max8998_rtc_i2c_read(client, MAX8998_RTC_TIME_ADDR,regs,MAX8998_RTC_LEN);
        if (ret < 0)
                return ret;

        tm->tm_sec =  bcd2bin(regs[MAX8998_REG_SECOND]);
        tm->tm_min =  bcd2bin(regs [MAX8998_REG_MIN]);
        tm->tm_hour = bcd2bin(regs[MAX8998_REG_HOUR] & 0x3f);
	if(regs[MAX8998_REG_HOUR] & (1 << 7))
        	tm->tm_hour += 12;
        tm->tm_wday = bcd2bin(regs[MAX8998_REG_WEEKDAY]);
        tm->tm_mday = bcd2bin(regs[MAX8998_REG_DATE]);
        tm->tm_mon =  bcd2bin(regs [MAX8998_REG_MONTH]) - 1;
        tm->tm_year = bcd2bin(regs[MAX8998_REG_YEAR0]) +
                      bcd2bin(regs[MAX8998_REG_YEAR1]) * 100 - 1900;


	
        return 0;
}


static int max8998_rtc_i2c_set_time(struct i2c_client *client, struct rtc_time *tm)
{
        int ret;
        u8 regs[MAX8998_RTC_LEN + 1];

	tm->tm_year += 1900;

	regs[MAX8998_REG_SECOND + 1] = bin2bcd(tm->tm_sec);
	regs[MAX8998_REG_MIN + 1] = bin2bcd(tm->tm_min);
	regs[MAX8998_REG_HOUR + 1] = bin2bcd(tm->tm_hour);
	regs[MAX8998_REG_WEEKDAY + 1] = bin2bcd(tm->tm_wday);
	regs[MAX8998_REG_DATE + 1] = bin2bcd(tm->tm_mday);
	regs[MAX8998_REG_MONTH + 1] = bin2bcd(tm->tm_mon + 1);
	regs[MAX8998_REG_YEAR0 + 1] = bin2bcd(tm->tm_year % 100);
	regs[MAX8998_REG_YEAR1 + 1] = bin2bcd((tm->tm_year /100) );

        ret = max8998_rtc_i2c_write(client, MAX8998_RTC_TIME_ADDR,regs,MAX8998_RTC_LEN +1);

	return ret;
}

int max8998_rtc_read_time(struct rtc_time *tm)
{
	int ret;

	max8998_dbg("%s \n", __func__);

	ret = max8998_rtc_i2c_read_time(max8998_rtc_i2c_client, tm);

	max8998_dbg("read %s time %02d.%02d.%02d %02d/%02d/%02d\n",__func__,
		 tm->tm_year, tm->tm_mon, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);
	return ret;
}

int max8998_rtc_set_time(struct rtc_time *tm)
{
	max8998_dbg("%s  %02d.%02d.%02d %02d/%02d/%02d\n",__func__,
		 tm->tm_year, tm->tm_mon, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);
	return max8998_rtc_i2c_set_time(max8998_rtc_i2c_client, tm);
}

static int
max8998_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;
	max8998_rtc_i2c_client = client;
	return 0;
}

static int max8998_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_device_id max8998_id[] = {
	{ "rtc_max8998", 0 },
	{ }
};

static struct i2c_driver max8998_rtc_i2c_driver = {
	.driver = {
		.name = "rtc_max8998",
		.owner = THIS_MODULE,
	},
	.probe = max8998_probe,
	.remove = max8998_remove,
	.id_table = max8998_id,
};


static int __init s3c_max8998_rtc_init(void)
{
	int ret;
	max8998_dbg("%s\n", __func__);

	if ((ret = i2c_add_driver(&max8998_rtc_i2c_driver)))
		pr_err("%s: Can't add max8998_rtc_ i2c drv\n", __func__);
	return ret;
}

static void __exit s3c_max8998_rtc_exit(void)
{
	max8998_dbg("%s\n", __func__);
	i2c_del_driver(&max8998_rtc_i2c_driver);
}

module_init(s3c_max8998_rtc_init);
module_exit(s3c_max8998_rtc_exit);

MODULE_AUTHOR("samsung");
MODULE_DESCRIPTION("MX 8998 rtc driver");
MODULE_LICENSE("GPL");
