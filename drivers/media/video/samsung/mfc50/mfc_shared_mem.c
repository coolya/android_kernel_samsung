/*
 * drivers/media/video/samsung/mfc50/mfc_shared_mem.c
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Key-Young Park, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.10.08 - Apply 9/30 firmware(Key Young, Park)
 *   2009.11.06 - clean & invalidate shared mem area (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <asm/cacheflush.h>

#include "mfc_shared_mem.h"
#include "mfc_logmsg.h"

#define DEBUG_ENABLE 0

static inline void mfc_write_shared_mem_item(unsigned int host_wr_addr, unsigned int addr, unsigned int value)
{
	MEM_WRITE(host_wr_addr + addr, value);
}

static inline unsigned int mfc_read_shared_mem_item(unsigned int host_wr_addr, unsigned int addr)
{
	return MEM_READ(host_wr_addr + addr);
}

void mfc_write_shared_mem(unsigned int host_wr_addr, struct mfc_shared_mem *shared_mem)
{
	mfc_write_shared_mem_item(host_wr_addr, RC_CONTROL_ENABLE,	 1);/* RC_CONTROL_CONFIG : enable (1), disable(0) */

	mfc_write_shared_mem_item(host_wr_addr, SET_FRAME_TAG,		   shared_mem->set_frame_tag);
	mfc_write_shared_mem_item(host_wr_addr, START_BYTE_NUM,		   shared_mem->start_byte_num);
	mfc_write_shared_mem_item(host_wr_addr, EXT_ENC_CONTROL,	   shared_mem->ext_enc_control);
	mfc_write_shared_mem_item(host_wr_addr, ENC_PARAM_CHANGE,	   shared_mem->enc_param_change);
	mfc_write_shared_mem_item(host_wr_addr, VOP_TIMING,		   shared_mem->vop_timing);
	mfc_write_shared_mem_item(host_wr_addr, HEC_PERIOD,		   shared_mem->hec_period);
	mfc_write_shared_mem_item(host_wr_addr, P_B_FRAME_QP,		   shared_mem->P_B_frame_qp);
	mfc_write_shared_mem_item(host_wr_addr, METADATA_ENABLE,	   shared_mem->metadata_enable);
	mfc_write_shared_mem_item(host_wr_addr, EXT_METADATA_START_ADDR,   shared_mem->ext_metadata_start_addr);
	mfc_write_shared_mem_item(host_wr_addr, PUT_EXTRADATA,	           shared_mem->put_extradata);
	mfc_write_shared_mem_item(host_wr_addr, DBG_INFO_INPUT0,	   shared_mem->dbg_info_input0);
	mfc_write_shared_mem_item(host_wr_addr, DBG_INFO_INPUT1,	   shared_mem->dbg_info_input1);
	mfc_write_shared_mem_item(host_wr_addr, ALLOCATED_LUMA_DPB_SIZE,   shared_mem->allocated_luma_dpb_size);
	mfc_write_shared_mem_item(host_wr_addr, ALLOCATED_CHROMA_DPB_SIZE, shared_mem->allocated_chroma_dpb_size);
	mfc_write_shared_mem_item(host_wr_addr, ALLOCATED_MV_SIZE,	   shared_mem->allocated_mv_size);
	mfc_write_shared_mem_item(host_wr_addr, P720_LIMIT_ENABLE,	   shared_mem->p720_limit_enable);

	dmac_flush_range((void *)host_wr_addr, (void *)host_wr_addr + SHARED_MEM_MAX);

#if DEBUG_ENABLE
	mfc_print_shared_mem(host_wr_addr);
#endif
}

void mfc_read_shared_mem(unsigned int host_wr_addr, struct mfc_shared_mem *shared_mem)
{
	invalidate_kernel_vmap_range((void *)host_wr_addr, SHARED_MEM_MAX);

	shared_mem->extended_decode_status = mfc_read_shared_mem_item(host_wr_addr, EXTENEDED_DECODE_STATUS);
	shared_mem->get_frame_tag_top      = mfc_read_shared_mem_item(host_wr_addr, GET_FRAME_TAG_TOP);
	shared_mem->get_frame_tag_bot      = mfc_read_shared_mem_item(host_wr_addr, GET_FRAME_TAG_BOT);
	shared_mem->pic_time_top           = mfc_read_shared_mem_item(host_wr_addr, PIC_TIME_TOP);
	shared_mem->pic_time_bot           = mfc_read_shared_mem_item(host_wr_addr, PIC_TIME_BOT);
	shared_mem->start_byte_num         = mfc_read_shared_mem_item(host_wr_addr, START_BYTE_NUM);
	shared_mem->dec_frm_size           = mfc_read_shared_mem_item(host_wr_addr, DEC_FRM_SIZE);
	shared_mem->crop_info1             = mfc_read_shared_mem_item(host_wr_addr, CROP_INFO1);
	shared_mem->crop_info2             = mfc_read_shared_mem_item(host_wr_addr, CROP_INFO2);
	shared_mem->metadata_status        = mfc_read_shared_mem_item(host_wr_addr, METADATA_STATUS);
	shared_mem->metadata_display_index = mfc_read_shared_mem_item(host_wr_addr, METADATA_DISPLAY_INDEX);
	shared_mem->dbg_info_output0       = mfc_read_shared_mem_item(host_wr_addr, DBG_INFO_OUTPUT0);
	shared_mem->dbg_info_output1       = mfc_read_shared_mem_item(host_wr_addr, DBG_INFO_OUTPUT1);

#if DEBUG_ENABLE
	mfc_print_shared_mem(host_wr_addr);
#endif
}

void mfc_print_shared_mem(unsigned int host_wr_addr)
{
	mfc_info("set_frame_tag          = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, SET_FRAME_TAG));
	mfc_info("start_byte_num         = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, START_BYTE_NUM));
	mfc_info("ext_enc_control        = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, EXT_ENC_CONTROL));
	mfc_info("enc_param_change       = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, ENC_PARAM_CHANGE));
	mfc_info("vop_timing             = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, VOP_TIMING));
	mfc_info("hec_period             = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, HEC_PERIOD));
	mfc_info("p_b_frame_qp           = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, P_B_FRAME_QP));
	mfc_info("metadata_enable        = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, METADATA_ENABLE));
	mfc_info("ext_metadata_start_addr= 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, EXT_METADATA_START_ADDR));
	mfc_info("put_extradata          = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, PUT_EXTRADATA));
	mfc_info("dbg_info_input0        = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, DBG_INFO_INPUT0));
	mfc_info("dbg_info_input1        = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, DBG_INFO_INPUT1));
	mfc_info("luma_dpb_size          = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, ALLOCATED_LUMA_DPB_SIZE));
	mfc_info("chroma_dpb_size        = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, ALLOCATED_CHROMA_DPB_SIZE));
	mfc_info("mv_size                = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, ALLOCATED_MV_SIZE));
	mfc_info("extended_decode_status = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, EXTENEDED_DECODE_STATUS));
	mfc_info("get_frame_tag_top      = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, GET_FRAME_TAG_TOP));
	mfc_info("get_frame_tag_bot      = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, GET_FRAME_TAG_BOT));
	mfc_info("pic_time_top           = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, PIC_TIME_TOP));
	mfc_info("pic_time_bot           = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, PIC_TIME_BOT));
	mfc_info("start_byte_num         = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, START_BYTE_NUM));
	mfc_info("dec_frm_size           = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, DEC_FRM_SIZE));
	mfc_info("crop_info1             = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, CROP_INFO1));
	mfc_info("crop_info2             = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, CROP_INFO2));
	mfc_info("metadata_status        = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, METADATA_STATUS));
	mfc_info("metadata_display_index = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, METADATA_DISPLAY_INDEX));
	mfc_info("dbg_info_output0       = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, DBG_INFO_OUTPUT0));
	mfc_info("dbg_info_output1       = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, DBG_INFO_OUTPUT1));
	mfc_info("720p_limit_enable	 = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, P720_LIMIT_ENABLE));
	mfc_info("RC_CONTROL_ENABLE	 = 0x%08x\n", mfc_read_shared_mem_item(host_wr_addr, RC_CONTROL_ENABLE));
}
