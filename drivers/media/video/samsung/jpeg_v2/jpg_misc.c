/* linux/drivers/media/video/samsung/jpeg_v2/jpg_misc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Operation for Jpeg encoder/docoder with mutex
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdarg.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/version.h>

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "jpg_misc.h"
#include "jpg_mem.h"

static struct mutex *h_mutex;

/*
 * Function: create_jpg_mutex
 * Implementation Notes: Create Mutex handle
 */
struct mutex *create_jpg_mutex(void)
{
	h_mutex = kmalloc(sizeof(struct mutex), GFP_KERNEL);

	if (h_mutex == NULL)
		return NULL;

	mutex_init(h_mutex);

	return h_mutex;
}

/*
 * Function: lock_jpg_mutex
 * Implementation Notes: lock mutex
 */
unsigned long lock_jpg_mutex(void)
{
	mutex_lock(h_mutex);
	return 1;
}

/*
 * Function: unlock_jpg_mutex
 * Implementation Notes: unlock mutex
*/
unsigned long unlock_jpg_mutex(void)
{
       mutex_unlock(h_mutex);

       return 1;
}


/*
 * Function: delete_jpg_mutex
 * Implementation Notes: delete mutex handle
 */
void delete_jpg_mutex(void)
{
	if (h_mutex == NULL)
		return;

	mutex_destroy(h_mutex);
}

