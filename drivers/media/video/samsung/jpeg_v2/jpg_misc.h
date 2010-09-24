/* linux/drivers/media/video/samsung/jpeg_v2/jpg_misc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Definition for Operation of Jpeg encoder/docoder with mutex
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __JPG_MISC_H__
#define __JPG_MISC_H__

#include <linux/types.h>

enum BOOL {FALSE, TRUE};

#define INT_TIMEOUT			1000

struct mutex *create_jpg_mutex(void);
unsigned long lock_jpg_mutex(void);
unsigned long unlock_jpg_mutex(void);
void delete_jpg_mutex(void);

#endif
