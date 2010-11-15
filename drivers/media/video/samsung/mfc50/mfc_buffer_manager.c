/*
 * drivers/media/video/samsung/mfc50/mfc_buffer_manager.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Key-Young Park, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - use struct list_head for duble linked list
 *   2009.11.04 - get physical address via mfc_allocate_buffer (Key Young, Park)
 *   2009.11.13 - fix free buffer fragmentation (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <linux/io.h>
#include <linux/uaccess.h>

#include <plat/media.h>
#include <mach/media.h>

#include "mfc_buffer_manager.h"
#include "mfc_errorno.h"
#include "mfc_logmsg.h"
#include "mfc_memory.h"

static struct list_head mfc_alloc_mem_head[MFC_MAX_PORT_NUM];
static struct list_head mfc_free_mem_head[MFC_MAX_PORT_NUM];

void mfc_print_mem_list(void)
{
	struct list_head *pos;
	struct mfc_alloc_mem *alloc_node;
	struct mfc_free_mem *free_node;
	int port_no;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		mfc_info("===== %s port%d list =====\n", __func__,  port_no);
		list_for_each(pos, &mfc_alloc_mem_head[port_no])
		{
			alloc_node = list_entry(pos, struct mfc_alloc_mem, list);
			mfc_info("[alloc_list] inst_no: %d, p_addr: 0x%08x, "
					"u_addr: 0x%p, size: %d\n",
					alloc_node->inst_no,
					alloc_node->p_addr,
					alloc_node->u_addr,
					alloc_node->size);
		}

		list_for_each(pos, &mfc_free_mem_head[port_no])
		{
			free_node = list_entry(pos, struct mfc_free_mem, list);
			mfc_info("[free_list] start_addr: 0x%08x size:%d\n",
					free_node->start_addr , free_node->size);
		}
	}
}

void mfc_merge_fragment(int inst_no)
{
	struct list_head *pos, *n;
	struct mfc_free_mem *node1, *node2;
	int port_no;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		list_for_each_safe(pos, n, &mfc_free_mem_head[port_no])
		{
			node1 = list_entry(pos, struct mfc_free_mem, list);
			node2 = list_entry(n, struct mfc_free_mem, list);
			if ((node1->start_addr + node1->size) == node2->start_addr) {
				node2->start_addr = node1->start_addr;
				node2->size += node1->size;
				list_del(&(node1->list));
				kfree(node1);
			}
		}
	}

#if defined(DEBUG)
	mfc_print_mem_list();
#endif
}



static unsigned int mfc_get_free_mem(int alloc_size, int inst_no, int port_no)
{
	struct list_head *pos;
	struct mfc_free_mem *free_node, *match_node = NULL;
	unsigned int alloc_addr = 0;

	mfc_debug("request Size : %d\n", alloc_size);

	if (list_empty(&mfc_free_mem_head[port_no])) {
		mfc_err("all memory is gone\n");
		return alloc_addr;
	}
	/* find best chunk of memory */
	list_for_each(pos, &mfc_free_mem_head[port_no])
	{
		free_node = list_entry(pos, struct mfc_free_mem, list);

		if (match_node != NULL) {
			if ((free_node->size >= alloc_size) &&
				(free_node->size < match_node->size))
				match_node = free_node;
		} else {
			if (free_node->size >= alloc_size)
				match_node = free_node;
		}
	}


	if (match_node != NULL) {
		mfc_debug("match : startAddr(0x%08x) size(%d)\n", match_node->start_addr, match_node->size);

		alloc_addr = match_node->start_addr;
		match_node->start_addr += alloc_size;
		match_node->size -= alloc_size;

		if (match_node->size < 0x1)	/* delete match_node. */
			mfc_err("there is no suitable chunk...[case 0]\n");
	} else {
		mfc_err("there is no suitable chunk....[case 1]\n");
		return 0;
	}

	return alloc_addr;
}


int mfc_init_buffer(void)
{
	struct mfc_free_mem *free_node;
	int	port_no;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		INIT_LIST_HEAD(&mfc_alloc_mem_head[port_no]);
		INIT_LIST_HEAD(&mfc_free_mem_head[port_no]);
		/* init free head node */
		free_node =
			(struct mfc_free_mem *)kmalloc(sizeof(struct mfc_free_mem), GFP_KERNEL);
		memset(free_node, 0x00, sizeof(struct mfc_free_mem));

		if (port_no) {
			free_node->start_addr = mfc_get_port1_buff_paddr();
			free_node->size = mfc_port1_memsize;
		} else {
			free_node->start_addr = mfc_get_port0_buff_paddr();
			free_node->size = mfc_port1_memsize -
				(mfc_get_port0_buff_paddr() - mfc_get_fw_buff_paddr());
		}

		list_add_tail(&(free_node->list), &mfc_free_mem_head[port_no]);
	}

#if defined(DEBUG)
	mfc_print_mem_list();
#endif
	return 0;
}

enum mfc_error_code mfc_release_buffer(unsigned char *u_addr)
{
	struct list_head *pos;
	int port_no;
	struct mfc_alloc_mem *alloc_node;
	bool found = false;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		list_for_each(pos, &mfc_alloc_mem_head[port_no])
		{
			alloc_node = list_entry(pos, struct mfc_alloc_mem, list);
			if (alloc_node->u_addr == u_addr) {
				mfc_free_alloc_mem(alloc_node, port_no);
				found = true;
				break;
			}
		}
	}

#if defined(DEBUG)
	mfc_print_mem_list();
#endif

	if (found)
		return MFCINST_RET_OK;
	else
		return MFCINST_MEMORY_INVALID_ADDR;
}

void mfc_release_all_buffer(int inst_no)
{
	struct list_head *pos, *n;
	int port_no;
	struct mfc_alloc_mem *alloc_node;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		list_for_each_safe(pos, n, &mfc_alloc_mem_head[port_no]) {
			alloc_node = list_entry(pos, struct mfc_alloc_mem, list);
			if (alloc_node->inst_no == inst_no) {
				mfc_free_alloc_mem(alloc_node, port_no);
			}
		}
	}

#if defined(DEBUG)
	mfc_print_mem_list();
#endif
}

void mfc_free_alloc_mem(struct mfc_alloc_mem *alloc_node, int port_no)
{
	struct list_head *pos;
	struct mfc_free_mem *free_node;
	struct mfc_free_mem *target_node;

	free_node = (struct mfc_free_mem *)kmalloc(sizeof(struct mfc_free_mem), GFP_KERNEL);
	free_node->start_addr = alloc_node->p_addr;
	free_node->size = alloc_node->size;

	list_for_each(pos, &mfc_free_mem_head[port_no])
	{
		target_node = list_entry(pos, struct mfc_free_mem, list);
		if (alloc_node->p_addr < target_node->start_addr)
			break;
	}

	if (pos == &mfc_free_mem_head[port_no])
		list_add_tail(&(free_node->list), &(mfc_free_mem_head[port_no]));
	else
		list_add_tail(&(free_node->list), pos);

	list_del(&(alloc_node->list));
	kfree(alloc_node);
}

enum mfc_error_code mfc_get_phys_addr(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	int ret, port_no;
	struct list_head *pos;
	struct mfc_alloc_mem *alloc_node;
	struct mfc_get_phys_addr_arg *phys_addr_arg;

	phys_addr_arg = (struct mfc_get_phys_addr_arg *)args;
	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		list_for_each(pos, &mfc_alloc_mem_head[port_no])
		{
			alloc_node = list_entry(pos, struct mfc_alloc_mem, list);
			if (alloc_node->u_addr == (unsigned char *)phys_addr_arg->u_addr) {
				mfc_debug("u_addr(0x%08x), p_addr(0x%08x) is found\n",
						alloc_node->u_addr, alloc_node->p_addr);
				goto found;
			}
		}
	}

	mfc_err("invalid virtual address(0x%08x)\r\n", phys_addr_arg->u_addr);
	ret = MFCINST_MEMORY_INVALID_ADDR;
	goto out_getphysaddr;

found:
	phys_addr_arg->p_addr = alloc_node->p_addr;
	ret = MFCINST_RET_OK;

out_getphysaddr:
	return ret;
}

enum mfc_error_code mfc_allocate_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args, int port_no)
{
	int ret;
	int inst_no = mfc_ctx->mem_inst_no;
	unsigned int start_paddr;
	struct mfc_mem_alloc_arg *in_param;
	struct mfc_alloc_mem *alloc_node;

	in_param = (struct mfc_mem_alloc_arg *)args;

	alloc_node = (struct mfc_alloc_mem *)kmalloc(sizeof(struct mfc_alloc_mem), GFP_KERNEL);
	if (!alloc_node) {
		mfc_err("There is no more kernel memory");
		ret = MFCINST_MEMORY_ALLOC_FAIL;
		goto out_getcodecviraddr;
	}
	memset(alloc_node, 0x00, sizeof(struct mfc_alloc_mem));

	/* if user request area, allocate from reserved area */
	start_paddr = mfc_get_free_mem((int)in_param->buff_size, inst_no, port_no);
	mfc_debug("start_paddr = 0x%X\n\r", start_paddr);

	if (!start_paddr) {
		mfc_err("There is no more memory\n\r");
		in_param->out_uaddr = -1;
		ret = MFCINST_MEMORY_ALLOC_FAIL;
		kfree(alloc_node);
		goto out_getcodecviraddr;
	}

	alloc_node->p_addr = start_paddr;
	if (port_no) {
		alloc_node->v_addr = (unsigned char *)(mfc_get_port1_buff_vaddr() +
			(alloc_node->p_addr - mfc_get_port1_buff_paddr()));
		alloc_node->u_addr = (unsigned char *)(in_param->mapped_addr +
			mfc_ctx->port0_mmap_size +
			(alloc_node->p_addr - mfc_get_port1_buff_paddr()));
	} else {
		alloc_node->v_addr = (unsigned char *)(mfc_get_port0_buff_vaddr() +
			(alloc_node->p_addr - mfc_get_port0_buff_paddr()));
		alloc_node->u_addr = (unsigned char *)(in_param->mapped_addr +
			(alloc_node->p_addr - mfc_get_port0_buff_paddr()));
	}

	in_param->out_uaddr = (unsigned int)alloc_node->u_addr;
	in_param->out_paddr = (unsigned int)alloc_node->p_addr;
	mfc_debug("u_addr : 0x%08x v_addr : 0x%08x p_addr : 0x%08x\n",
			(unsigned int)alloc_node->u_addr,
			(unsigned int)alloc_node->v_addr,
			alloc_node->p_addr);

	alloc_node->size = (int)in_param->buff_size;
	alloc_node->inst_no = inst_no;

	list_add(&(alloc_node->list), &mfc_alloc_mem_head[port_no]);
	ret = MFCINST_RET_OK;

#if defined(DEBUG)
	mfc_print_mem_list();
#endif

out_getcodecviraddr:
	return ret;
}
