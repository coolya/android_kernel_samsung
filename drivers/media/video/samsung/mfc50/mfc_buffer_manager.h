/*
 * drivers/media/video/samsung/mfc50/mfc_buffer_manager.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Key-Young Park, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.11.04 - remove mfc_common.[ch]
 *                seperate buffer alloc & set (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _MFC_BUFFER_MANAGER_H_
#define _MFC_BUFFER_MANAGER_H_

#include <linux/list.h>
#include "mfc_interface.h"
#include "mfc_opr.h"

#define MFC_MAX_PORT_NUM 2

/*  Struct Definition */
struct mfc_alloc_mem  {
	struct list_head list;     /* strcut list_head for alloc mem        */
	unsigned int p_addr;       /* physical address                      */
	unsigned char *v_addr;     /* virtual address                       */
	unsigned char *u_addr;     /* virtual address for user mode process */
	int size;                  /* memory size                           */
	int inst_no;               /* instance no                           */
};


struct mfc_free_mem  {
	struct list_head list;     /* struct list_head for free mem         */
	unsigned int start_addr;   /* start address of free mem             */
	unsigned int size;         /* size of free mem                      */
};


/* Function Prototype */
void mfc_print_mem_list(void);
int mfc_init_buffer(void);
void mfc_merge_fragment(int inst_no);
void mfc_release_all_buffer(int inst_no);
void mfc_free_alloc_mem(struct mfc_alloc_mem *alloc_node, int port_no);
enum mfc_error_code mfc_release_buffer(unsigned char *u_addr);
enum mfc_error_code mfc_get_phys_addr(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args);
enum mfc_error_code mfc_allocate_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args, int port_no);

#endif /* _MFC_BUFFER_MANAGER_H_ */
