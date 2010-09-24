/* linux/drivers/media/video/samsung/jpeg_v2/s3c-jpeg.h
  *
  * Copyright (c) 2010 Samsung Electronics Co., Ltd.
  * http://www.samsung.com/
  *
  * Header file for Samsung Jpeg Interface driver
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
 */


#ifndef __JPEG_DRIVER_H__
#define __JPEG_DRIVER_H__


#define MAX_INSTANCE_NUM	1
#define MAX_PROCESSING_THRESHOLD 1000	/* 1Sec */

#define JPEG_IOCTL_MAGIC 'J'

#define IOCTL_JPG_DECODE			_IO(JPEG_IOCTL_MAGIC, 1)
#define IOCTL_JPG_ENCODE			_IO(JPEG_IOCTL_MAGIC, 2)
#define IOCTL_JPG_GET_STRBUF			_IO(JPEG_IOCTL_MAGIC, 3)
#define IOCTL_JPG_GET_FRMBUF			_IO(JPEG_IOCTL_MAGIC, 4)
#define IOCTL_JPG_GET_THUMB_STRBUF		_IO(JPEG_IOCTL_MAGIC, 5)
#define IOCTL_JPG_GET_THUMB_FRMBUF		_IO(JPEG_IOCTL_MAGIC, 6)
#define IOCTL_JPG_GET_PHY_FRMBUF		_IO(JPEG_IOCTL_MAGIC, 7)
#define IOCTL_JPG_GET_PHY_THUMB_FRMBUF		_IO(JPEG_IOCTL_MAGIC, 8)
#define JPG_CLOCK_DIVIDER_RATIO_QUARTER	4

/* Driver Helper function */
#define to_jpeg_plat(d)		(to_platform_device(d)->dev.platform_data)

#endif /*__JPEG_DRIVER_H__*/
