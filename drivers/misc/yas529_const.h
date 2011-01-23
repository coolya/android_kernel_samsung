/*
 * Copyright (c) 2010 Yamaha Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef _YAS529_CONST_H_
#define _YAS529_CONST_H_

#define YAS529_DEFAULT_THRESHOLD            (1)
#define YAS529_DEFAULT_DISTORTION           (15)
#define YAS529_DEFAULT_SHAPE                (0)

#define MEASURE_RESULT_OVER_UNDER_FLOW  (0x07)
#define MEASURE_ERROR_DELAY             (100)
#define MEASURE_STATE_NORMAL            (0)
#define MEASURE_STATE_INIT_COIL         (1)
#define MEASURE_STATE_ROUGH_MEASURE     (2)
#define MEASURE_RESULT_OVER_UNDER_FLOW  (0x07)

#define __LINUX_KERNEL_DRIVER__

#ifndef STATIC
#define STATIC static
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!(0))
#endif

#ifndef NELEMS
#define NELEMS(a) ((int)(sizeof(a)/sizeof(a[0])))
#endif

#ifdef __LINUX_KERNEL_DRIVER__
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#define LOGV(...) printk(KERN_DEBUG __VA_ARGS__)
#define LOGD(...) printk(KERN_DEBUG __VA_ARGS__)
#define LOGI(...) printk(KERN_INFO __VA_ARGS__)
#define LOGW(...) printk(KERN_WARNING __VA_ARGS__)
#define LOGE(...) printk(KERN_ERR __VA_ARGS__)
#else
#include <stdint.h>
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#endif


#ifndef __YAS529_CDRIVER_H__
#define __YAS529_CDRIVER_H__

# define YAS529_CDRV_CENTER_X  512
# define YAS529_CDRV_CENTER_Y1 512
# define YAS529_CDRV_CENTER_Y2 512
# define YAS529_CDRV_CENTER_T  256
# define YAS529_CDRV_CENTER_I1 512
# define YAS529_CDRV_CENTER_I2 512
# define YAS529_CDRV_CENTER_I3 512

#define YAS529_CDRV_ROUGHOFFSET_MEASURE_OF_VALUE 33
#define YAS529_CDRV_ROUGHOFFSET_MEASURE_UF_VALUE  0
#define YAS529_CDRV_NORMAL_MEASURE_OF_VALUE 1024
#define YAS529_CDRV_NORMAL_MEASURE_UF_VALUE    1

#define MS3CDRV_CMD_MEASURE_ROUGHOFFSET 0x1
#define MS3CDRV_CMD_MEASURE_XY1Y2T      0x2

#define MS3CDRV_RDSEL_MEASURE     0xc0
#define MS3CDRV_RDSEL_CALREGISTER 0xc8

#define MS3CDRV_WAIT_MEASURE_ROUGHOFFSET  2 /*  1.5[ms] */
#define MS3CDRV_WAIT_MEASURE_XY1Y2T      13 /* 12.3[ms] */

//#define MS3CDRV_I2C_SLAVE_ADDRESS 0x2e
#define MS3CDRV_GSENSOR_INITIALIZED     (0x01)
#define MS3CDRV_MSENSOR_INITIALIZED     (0x02)

#define YAS529_CDRV_NO_ERROR 0
#define YAS529_CDRV_ERR_ARG (-1)
#define YAS529_CDRV_ERR_NOT_INITIALIZED (-3)
#define YAS529_CDRV_ERR_BUSY (-4)
#define YAS529_CDRV_ERR_I2CCTRL (-5)
#define YAS529_CDRV_ERR_ROUGHOFFSET_NOT_WRITTEN (-126)
#define YAS529_CDRV_ERROR (-127)

#define YAS529_CDRV_MEASURE_X_OFUF  0x1
#define YAS529_CDRV_MEASURE_Y1_OFUF 0x2
#define YAS529_CDRV_MEASURE_Y2_OFUF 0x4


#endif

#endif /* _YAS529_CONST_H_ */
