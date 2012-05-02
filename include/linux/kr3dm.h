/*
 *  STMicroelectronics kr3dm acceleration sensor driver
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifndef __KR3DM_ACC_HEADER__
#define __KR3DM__ACC_HEADER__

#include <linux/types.h>
#include <linux/ioctl.h>

struct kr3dm_acceldata {
	__s16 x;
	__s16 y;
	__s16 z;
};

/* dev info */
#define ACC_DEV_NAME "accelerometer"

/* kr3dm ioctl command label */
#define KR3DM_IOCTL_BASE 'a'
#define KR3DM_IOCTL_SET_DELAY       _IOW(KR3DM_IOCTL_BASE, 0, int64_t)
#define KR3DM_IOCTL_GET_DELAY       _IOR(KR3DM_IOCTL_BASE, 1, int64_t)
#define KR3DM_IOCTL_READ_ACCEL_XYZ  _IOR(KR3DM_IOCTL_BASE, 8, \
						struct kr3dm_acceldata)

#ifdef __KERNEL__
struct kr3dm_platform_data {
	int gpio_acc_int;	/* gpio for kr3dm int output */
	s8 *rotation;		/* rotation matrix, if NULL assume Id */
};
#endif /* __KERNEL__ */

#endif
