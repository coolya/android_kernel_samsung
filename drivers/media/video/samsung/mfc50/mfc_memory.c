/*
 * drivers/media/video/samsung/mfc50/mfc_memory.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <mach/map.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/sizes.h>
#include <linux/memory.h>
#include <plat/media.h>

#include "mfc_memory.h"
#include "mfc_logmsg.h"
#include "mfc_interface.h"

void __iomem *mfc_sfr_base_vaddr;
unsigned int mfc_port0_base_paddr, mfc_port1_base_paddr;
unsigned char *mfc_port0_base_vaddr, *mfc_port1_base_vaddr;
unsigned int  mfc_port0_memsize, mfc_port1_memsize;

unsigned int mfc_get_fw_buff_paddr(void)
{
	return mfc_port0_base_paddr;
}

unsigned char *mfc_get_fw_buff_vaddr(void)
{
	return mfc_port0_base_vaddr;
}

unsigned int mfc_get_port0_buff_paddr(void)
{
	return mfc_port0_base_paddr + MFC_FW_MAX_SIZE;
}

unsigned char *mfc_get_port0_buff_vaddr(void)
{
	return mfc_port0_base_vaddr + MFC_FW_MAX_SIZE;
}

unsigned char *mfc_get_port1_buff_vaddr(void)
{
	return mfc_port1_base_vaddr;
}

unsigned int mfc_get_port1_buff_paddr(void)
{
	return mfc_port1_base_paddr;
}

