/*
 * drivers/media/video/samsung/mfc50/mfc_shared_mem.h
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

#ifndef _MFC_SHARED_MEM_H_
#define _MFC_SHARED_MEM_H_

#define MEM_WRITE(ADDR, VALUE)   (*(volatile unsigned int *)(ADDR) = (VALUE))
#define MEM_READ(ADDR)           (*(volatile unsigned int *)(ADDR))

enum mfc_shared {
	EXTENEDED_DECODE_STATUS     = 0x0,
	SET_FRAME_TAG               = 0x4,
	GET_FRAME_TAG_TOP           = 0x8,
	GET_FRAME_TAG_BOT           = 0xC,
	PIC_TIME_TOP                = 0x10,
	PIC_TIME_BOT                = 0x14,
	START_BYTE_NUM              = 0x18,
	DEC_FRM_SIZE                = 0x1C,
	CROP_INFO1                  = 0x20,
	CROP_INFO2                  = 0x24,
	EXT_ENC_CONTROL             = 0x28,
	ENC_PARAM_CHANGE            = 0x2C,
	VOP_TIMING                  = 0x30,
	HEC_PERIOD                  = 0x34,
	METADATA_ENABLE             = 0x38,
	METADATA_STATUS             = 0x3C,
	METADATA_DISPLAY_INDEX      = 0x40,
	EXT_METADATA_START_ADDR     = 0x44,
	PUT_EXTRADATA               = 0x48,
	DBG_INFO_OUTPUT0            = 0x4C,
	DBG_INFO_OUTPUT1            = 0x50,
	DBG_INFO_INPUT0             = 0x54,
	DBG_INFO_INPUT1             = 0x58,
	REF_L0_PHY_IDX              = 0x5C,
	REF_L1_PHY_IDX              = 0x60,
	ALLOCATED_LUMA_DPB_SIZE     = 0x64,
	ALLOCATED_CHROMA_DPB_SIZE   = 0x68,
	ALLOCATED_MV_SIZE           = 0x6C,
	P_B_FRAME_QP                = 0x70,
	RC_CONTROL_ENABLE	    = 0xA0,
	P720_LIMIT_ENABLE	    = 0xB4,
	SHARED_MEM_MAX		    = 0x1000,
};

struct mfc_shared_mem  {
	unsigned int num_dpb;
	unsigned int allocated_dpb_size;
	unsigned int extended_decode_status;
	unsigned int set_frame_tag;
	unsigned int get_frame_tag_top;
	unsigned int get_frame_tag_bot;
	unsigned int pic_time_top;
	unsigned int pic_time_bot;
	unsigned int start_byte_num;
	unsigned int dec_frm_size;
	unsigned int crop_info1;
	unsigned int crop_info2;
	unsigned int ext_enc_control;
	unsigned int enc_param_change;
	unsigned int vop_timing;
	unsigned int hec_period;
	unsigned int P_B_frame_qp;
	unsigned int metadata_enable;
	unsigned int metadata_status;
	unsigned int metadata_display_index;
	unsigned int ext_metadata_start_addr;
	unsigned int put_extradata;
	unsigned int dbg_info_output0;
	unsigned int dbg_info_output1;
	unsigned int dbg_info_input0;
	unsigned int dbg_info_input1;
	unsigned int ref_l0_phy_idx;
	unsigned int ref_l1_phy_idx;
	unsigned int allocated_luma_dpb_size;
	unsigned int allocated_chroma_dpb_size;
	unsigned int allocated_mv_size;
	unsigned int p720_limit_enable;
};

void mfc_write_shared_mem(unsigned int host_wr_addr, struct mfc_shared_mem *shared_mem);
void mfc_read_shared_mem(unsigned int host_wr_addr, struct mfc_shared_mem *shared_mem);
void mfc_print_shared_mem(unsigned int host_wr_addr);
#endif
