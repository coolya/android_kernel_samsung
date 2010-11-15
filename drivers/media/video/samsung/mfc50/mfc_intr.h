/*
 * drivers/media/video/samsung/mfc50/mfc_intr.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Key-Young Park, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.10.13 - Separate from mfc_common.h(Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MFC_INTR_H_
#define _MFC_INTR_H_

#include <linux/interrupt.h>

irqreturn_t mfc_irq(int irq, void *dev_id);
int mfc_wait_for_done(enum mfc_wait_done_type command);
int mfc_return_code(void);
#endif
