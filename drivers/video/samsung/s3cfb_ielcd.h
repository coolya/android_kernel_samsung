
/* linux/drivers/video/samsung/s3cfb_ielcd.h
 *
 * Header file for Samsung (IELCD) driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3CFB_IELCD_H
#define _S3CFB_IELCD_H


#define S3C_IELCD_MAGIC_KEY            			0x2ff47

#define S3C_IELCD_PHY_BASE    				0xF8200000 
#define S3C_IELCD_MAP_SIZE    				0x00008000 

/*
 * Register Map
*/
#define S3C_IELCD_VIDCON0		(0x0000)  
#define S3C_IELCD_VIDCON1		(0x0004) 
#define S3C_IELCD_VIDCON2		(0x0008)
#define S3C_IELCD_VIDCON3		(0x000C)

#define S3C_IELCD_VIDTCON0		(0x0010)
#define S3C_IELCD_VIDTCON1		(0x0014)
#define S3C_IELCD_VIDTCON2		(0x0018)
#define S3C_IELCD_VIDTCON3		(0x001C)

#define S3C_IELCD_WINCON0		(0x0020) 

#define S3C_IELCD_SHADOWCON             (0x0034)   

#define S3C_IELCD_VIDOSD0A		(0x0040) 
#define S3C_IELCD_VIDOSD0B		(0x0044)
#define S3C_IELCD_VIDOSD0C		(0x0048)

#define S3C_IELCD_VIDINTCON0		(0x0130)
#define S3C_IELCD_VIDINTCON1		(0x0134)


#define S3C_IELCD_DITHMODE		(0x0170)

#define S3C_IELCD_WIN0MAP		(0x0180)

#define S3C_IELCD_WPALCON_H		(0x019C)
#define S3C_IELCD_WPALCON_L		(0x01A0)

#define S3C_IELCD_TRIGCON		(0x01A4)

#define S3C_IELCD_ITUIFCON		(0x01A8)

#define S3C_IELCD_I80IFCONA0		(0x01B0)
#define S3C_IELCD_I80IFCONA1		(0x01B4)
#define S3C_IELCD_I80IFCONB0		(0x01B8)
#define S3C_IELCD_I80IFCONB1		(0x01BC)

#define S3C_IELCD_CMDCON0		(0x01D0)
#define S3C_IELCD_CMDCON1		(0x01D4)


#define S3C_IELCD_SIFCCON0		(0x01E8)
#define S3C_IELCD_SIFCCON1		(0x01E4)
#define S3C_IELCD_SIFCCON2		(0x01E8)


#define S3C_IELCD_MODE			(0x0278)
#define S3C_IELCD_DULACON		(0x027c)

#define S3C_IELCD_LDICMD0		(0x0280)
#define S3C_IELCD_LDICMD1		(0x0284)
#define S3C_IELCD_LDICMD2		(0x0288)
#define S3C_IELCD_LDICMD3		(0x028C)
#define S3C_IELCD_LDICMD4		(0x0290)
#define S3C_IELCD_LDICMD5		(0x0294)
#define S3C_IELCD_LDICMD6		(0x0298)
#define S3C_IELCD_LDICMD7		(0x029C)
#define S3C_IELCD_LDICMD8		(0x02A0)
#define S3C_IELCD_LDICMD9		(0x02A4)
#define S3C_IELCD_LDICMD10		(0x02A8)
#define S3C_IELCD_LDICMD11		(0x02AC)

int s3c_ielcd_hw_init(void);
int s3c_ielcd_logic_start(void);
int s3c_ielcd_logic_stop(void);
int s3c_ielcd_start(void);
int s3c_ielcd_stop(void);


int s3c_ielcd_init_global(struct s3cfb_global *ctrl);
int s3c_ielcd_set_clock(struct s3cfb_global *ctrl);
//int s3c_ielcd_clk_enable(struct s3cfb_global *ctrl);

#endif
