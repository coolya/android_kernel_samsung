/* linux/drivers/video/samsung/s3cfb_mdnie.c
 *
 * Register interface file for Samsung IELCD driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>


#include <asm/io.h>
#include <mach/map.h>
#include <plat/clock.h>
#include <plat/fb.h>
#include <plat/regs-fb.h>
#include <plat/pm.h>

#include "s3cfb.h"
#include "s3cfb_mdnie.h"
#include "s3cfb_ielcd.h"



static struct resource *s3c_ielcd_mem;
static void __iomem *s3c_ielcd_base;


#define s3c_ielcd_readl(addr)             __raw_readl((s3c_ielcd_base + addr))
#define s3c_ielcd_writel(val,addr)        __raw_writel(val,(s3c_ielcd_base + addr))


static struct s3cfb_global ielcd_fb;
static struct s3cfb_global *ielcd_fbdev;



static char banner[] __initdata = KERN_INFO "S3C IELCD Driver, (c) 2010 Samsung Electronics\n";

int s3c_ielcd_hw_init(void)
{
	printk("IELCD  INIT ..........\n");

	printk(banner);

        s3c_ielcd_mem = request_mem_region(S3C_IELCD_PHY_BASE,S3C_IELCD_MAP_SIZE,"ielcd");
        if(s3c_ielcd_mem == NULL) {
                printk(KERN_ERR "IELCD: failed to reserved memory region\n");
                return -ENOENT;
        }

        s3c_ielcd_base = ioremap(S3C_IELCD_PHY_BASE,S3C_IELCD_MAP_SIZE);
        if(s3c_ielcd_base == NULL) {
                printk(KERN_ERR "IELCD failed ioremap\n");
                return -ENOENT;
        }

	printk("IELCD  INIT SUCCESS Addr : 0x%p\n",s3c_ielcd_base);

	ielcd_fbdev = &ielcd_fb;

	//s3c_ielcd_writel(3,S3C_IELCD_DULACON);

	return 0;

}


int s3c_ielcd_logic_start(void)
{
	s3c_ielcd_writel(S3C_IELCD_MAGIC_KEY,S3C_IELCD_MODE);
	return 0;
}

int s3c_ielcd_logic_stop(void)
{
	s3c_ielcd_writel(0,S3C_IELCD_MODE);
	return 0;
}


int s3c_ielcd_start(void)
{
	unsigned int con;

	con = s3c_ielcd_readl(S3C_IELCD_VIDCON0);
	con |= (S3C_VIDCON0_ENVID_ENABLE| S3C_VIDCON0_ENVID_F_ENABLE);
	s3c_ielcd_writel(con,S3C_IELCD_VIDCON0);

	return 0;
}

int s3c_ielcd_stop(void)
{
	unsigned int con;

	con = s3c_ielcd_readl(S3C_IELCD_VIDCON0);
	//con &= ~(S3C_VIDCON0_ENVID_ENABLE| S3C_VIDCON0_ENVID_F_ENABLE);
	con &= ~(S3C_VIDCON0_ENVID_F_ENABLE);
	s3c_ielcd_writel(con,S3C_IELCD_VIDCON0);

	return 0;
}


int s3c_ielcd_init_global(struct s3cfb_global *ctrl)
{
	unsigned int cfg;

	*ielcd_fbdev = *ctrl;
	ielcd_fbdev->regs = s3c_ielcd_base;



	s3cfb_set_polarity(ielcd_fbdev);
	s3cfb_set_timing(ielcd_fbdev);
	s3cfb_set_lcd_size(ielcd_fbdev);

	// dithmode

	s3c_ielcd_writel(0x0,S3C_IELCD_DITHMODE);

	// clk mode and mode
	// read from lcd vid con
	cfg = readl(ctrl->regs + S3C_VIDCON0);
	cfg &= ~((7 << 26) |  (1 << 5) | (1 << 0));
	cfg |= (0  << 26| 0 << 5);
	
	s3c_ielcd_writel(cfg,S3C_IELCD_VIDCON0);


	s3c_ielcd_writel(1<<5,S3C_IELCD_VIDINTCON0);

	s3cfb_set_vsync_interrupt(ielcd_fbdev, 0);
	s3cfb_set_global_interrupt(ielcd_fbdev, 0);
	
	//s3cfb_display_on(ielcd_fbdev);
	

	s3c_ielcd_writel(0,S3C_IELCD_VIDOSD0A);
	s3c_ielcd_writel((ctrl->lcd->width - 1) << 11 | (ctrl->lcd->height - 1), S3C_IELCD_VIDOSD0B);
	s3c_ielcd_writel((ctrl->lcd->width  * ctrl->lcd->height ), S3C_IELCD_VIDOSD0C);

	cfg = S3C_WINCON_DATAPATH_LOCAL|S3C_WINCON_BPPMODE_32BPP;
	cfg |= S3C_WINCON_INRGB_RGB;


	s3c_ielcd_writel(cfg,S3C_IELCD_WINCON0);

 	s3cfb_window_on(ielcd_fbdev,0);
	return 0;
}


int s3c_ielcd_set_clock(struct s3cfb_global *ctrl)
{
	*ielcd_fbdev = *ctrl;
	ielcd_fbdev->regs = s3c_ielcd_base;
	s3cfb_set_clock(ctrl);
	//clk_enable(ielcd_clock);
	return 0;
}


//module_init(s3c_ielcd_hw_init);


