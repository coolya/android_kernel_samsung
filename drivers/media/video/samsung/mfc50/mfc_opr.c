/*
 * drivers/media/video/samsung/mfc50/mfc_opr.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code. (Key Young, Park)
 *   2009.09.21 - Implement clock & power gating.
 *                including suspend & resume fuction. (Key Young, Park)
 *   2009.09.23 - Use TILE mode encoding / Move MFC reserved memory
 *                before FIMC reserved memory. (Key Young, Park)
 *   2009.09.24 - Minor patch. (Key Young, Park)
 *   2009.10.08 - Apply 9/30 firmware(Key Young, Park)
 *   2009.10.13 - Change wait_for_done (Key Young, Park)
 *   2009.10.17 - Add error handling routine for buffer allocation (Key Young, Park)
 *   2009.10.22 - Change codec name VC1AP_DEC -> VC1_DEC (Key Young, Park)
 *   2009.10.27 - Update firmware (2009.10.15) (Key Young, Park)
 *   2009.11.04 - get physical address via mfc_allocate_buffer (Key Young, Park)
 *   2009.11.04 - remove mfc_common.[ch]
 *                seperate buffer alloc & set (Key Young, Park)
 *   2009.11.06 - Apply common MFC API (Key Young, Park)
 *   2009.11.06 - memset shared_memory (Key Young, Park)
 *   2009.11.09 - implement packed PB (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <plat/regs-mfc.h>
#include <asm/cacheflush.h>
#include <mach/map.h>
#include <plat/map-s5p.h>

#include "mfc_opr.h"
#include "mfc_logmsg.h"
#include "mfc_memory.h"
#include "mfc_buffer_manager.h"
#include "mfc_interface.h"
#include "mfc_shared_mem.h"
#include "mfc_intr.h"


/* DEBUG_MAKE_RAW is option to dump input stream data of MFC.*/
#define DEBUG_MAKE_RAW					0 /* Making Dec/Enc Debugging Files */
#define ENABLE_DEBUG_MFC_INIT				0
#define ENABLE_MFC_REGISTER_DEBUG			0 /* 0: Disable	1: Enable */

#define ENABLE_CHECK_START_CODE				1
#define ENABLE_CHECK_NULL_STREAM			1
#define ENABLE_CHECK_STREAM_SIZE			1
#define ENABLE_CHECK_SEQ_HEADER				0

#if DEBUG_MAKE_RAW
#define ENABLE_DEBUG_DEC_EXE_INTR_ERR		1 /* You must make  the "dec_in" folder in data folder.*/
#define ENABLE_DEBUG_DEC_EXE_INTR_OK		1 /* Make log about Normal Interrupts.*/
#define ENABLE_DEBUG_ENC_EXE_INTR_ERR		1 /* You must make  the "enc_in" folder in data folder.*/
#endif

#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_DEC_EXE_INTR_ERR
#define ENABLE_DEBUG_DEC_EXE_PARSER_ERR		1 /* Firstly, Set ENABLE_DEBUG_DEC_EXE_INTR_ERR is "1". */
#endif
#endif

#define WRITEL_SHARED_MEM(data, address) \
	{ writel(data, address); \
	dmac_flush_range((void *)address, (void *)(address + 4));}

#if DEBUG_MAKE_RAW
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#endif


static void mfc_backup_context(struct mfc_inst_ctx  *mfc_ctx);
static void mfc_restore_context(struct mfc_inst_ctx  *mfc_ctx);
static void mfc_set_encode_init_param(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args);
static int mfc_get_inst_no(struct mfc_inst_ctx *mfc_ctx, unsigned int context_addr, int context_size);
static enum mfc_error_code mfc_encode_header(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args);
static enum mfc_error_code mfc_decode_one_frame(struct mfc_inst_ctx *mfc_ctx, struct mfc_dec_exe_arg *dec_arg, unsigned int *consumed_strm_size);

static void mfc_set_codec_buffer(struct mfc_inst_ctx *mfc_ctx);
static void mfc_set_dec_frame_buffer(struct mfc_inst_ctx *mfc_ctx);
static void mfc_set_dec_stream_buffer(struct mfc_inst_ctx *mfc_ctx, int buf_addr, unsigned int buf_size);
static void mfc_set_enc_ref_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args);

static enum mfc_error_code mfc_alloc_codec_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args);
static enum mfc_error_code mfc_alloc_dec_frame_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args);
static enum mfc_error_code mfc_alloc_context_buffer(struct mfc_inst_ctx *mfc_ctx, unsigned int mapped_addr, unsigned int *context_addr, int *size);
static enum mfc_error_code mfc_alloc_stream_ref_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args);

static int CheckMPEG4StartCode(unsigned char *src_mem, unsigned int remainSize);
static int CheckDecStartCode(unsigned char *src_mem, unsigned int nstreamSize, enum ssbsip_mfc_codec_type nCodecType);
static int CheckNullStream(unsigned char *src_mem, unsigned int streamSize);

static int mfc_mem_inst_no[MFC_MAX_INSTANCE_NUM];
static bool mCheckType;

/*
 * Debugging Functions	Definition
 * tile_to_linear_4x2(..)
 * calculate_seq_size(..)
 * printk_mfc_init_info(..)
 */
#if DEBUG_MAKE_RAW
static int mframe_cnt;
static int mImgWidth;
static int mImgHight;

static void copy16(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, int mm, int nn);
static void write_file(char *filename,  unsigned char *data, unsigned int nSize);
static int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos);
static void tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size);
#endif

#if ENABLE_CHECK_SEQ_HEADER
static int calculate_seq_size(mfc_args *args);
#endif

#if ENABLE_DEBUG_MFC_INIT
void printk_mfc_init_info(struct mfc_inst_ctx *mfc_ctx, struct mfc_args *args);
#endif

#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_DEC_EXE_INTR_ERR
void printk_mfc_dec_exe_info(struct mfc_inst_ctx *mfc_ctx, struct  mfc_dec_exe_arg *dec_arg);
void makefile_mfc_dec_err_info(struct mfc_inst_ctx *mfc_ctx, struct  mfc_dec_exe_arg *dec_arg, int nReturnErrCode);
void makefile_mfc_decinit_err_info(struct mfc_inst_ctx *mfc_ctx, struct mfc_dec_init_arg *decinit_arg, int nReturnErrCode);

static unsigned int mcontext_addr;
static int mcontext_size;
static int mIsDangerError;
#endif
#endif

#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_ENC_EXE_INTR_ERR
void makefile_mfc_enc_err_info(struct mfc_enc_exe_arg *enc_arg);
#endif
#endif

#if ENABLE_MFC_REGISTER_DEBUG
void mfc_fw_debug(mfc_wait_done_type command);
#endif

#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_ENC_EXE_INTR_ERR
static unsigned char pResLinearbuf[1280*720*3/2];
#endif
#endif

static	unsigned int predisplay_Yaddr;
static	unsigned int predisplay_Caddr;

#define MC_STATUS_TIMEOUT	1000	/* ms */

bool mfc_cmd_reset(void)
{
	unsigned int mc_status;
	unsigned long timeo = jiffies;

	timeo += msecs_to_jiffies(MC_STATUS_TIMEOUT);

	/* Stop procedure */
	WRITEL(0x3f6, MFC_SW_RESET); /*  reset RISC */
	WRITEL(0x3e2, MFC_SW_RESET); /*  All reset except for MC */
	mdelay(10);

	/* Check MC status */
	do {
		mc_status = (READL(MFC_MC_STATUS) & 0x3);

		if (mc_status == 0)
			break;

		schedule_timeout_uninterruptible(1);
	} while (time_before(jiffies, timeo));

	if (mc_status != 0)
		return false;

	WRITEL(0x0, MFC_SW_RESET);
	WRITEL(0x3fe, MFC_SW_RESET);

	return true;
}

static bool mfc_cmd_host2risc(enum mfc_facade_cmd cmd, int arg1, int arg2, int arg3, int arg4)
{
	enum mfc_facade_cmd cur_cmd = 0;
	unsigned long timeo = jiffies;
	timeo += 20;    /* waiting for 100ms */

	/* wait until host to risc command register becomes 'H2R_CMD_EMPTY' */
	while (time_before(jiffies, timeo)) {
		cur_cmd = READL(MFC_HOST2RISC_COMMAND);
		if (cur_cmd == H2R_CMD_EMPTY)
			break;
		msleep_interruptible(2);
	}

	if (cur_cmd != H2R_CMD_EMPTY)
		return false;

	WRITEL(arg1, MFC_HOST2RISC_ARG1);
	WRITEL(arg2, MFC_HOST2RISC_ARG2);
	WRITEL(arg3, MFC_HOST2RISC_ARG3);
	WRITEL(arg4, MFC_HOST2RISC_ARG4);
	WRITEL(cmd, MFC_HOST2RISC_COMMAND);

	return true;
}

static void mfc_backup_context(struct mfc_inst_ctx  *mfc_ctx)
{
}

static void mfc_restore_context(struct mfc_inst_ctx  *mfc_ctx)
{
}

static void mfc_set_dec_stream_buffer(struct mfc_inst_ctx *mfc_ctx, int buf_addr, unsigned int buf_size)
{
	unsigned int port0_base_paddr;

	mfc_debug_L0("inst_no : %d, buf_addr : 0x%08x, buf_size : 0x%08x\n", mfc_ctx->InstNo, buf_addr, buf_size);

	if (mfc_ctx->buf_type == MFC_BUFFER_CACHE) {
		unsigned char *in_vir;
		in_vir = phys_to_virt(buf_addr);
		dmac_unmap_area(in_vir, 1, ALIGN_TO_32B(buf_size));
	}	
	
	port0_base_paddr = mfc_port0_base_paddr;

	/* release buffer */
	WRITEL(0xffffffff, MFC_SI_CH0_RELEASE_BUFFER);

	/* Set stream & desc buffer */
	WRITEL((buf_addr - port0_base_paddr) >> 11, MFC_SI_CH0_ES_ADDR);
	WRITEL(buf_size, MFC_SI_CH0_ES_DEC_UNIT_SIZE);
	WRITEL(CPB_BUF_SIZE, MFC_SI_CH0_CPB_SIZE);
	WRITEL((buf_addr + CPB_BUF_SIZE - port0_base_paddr) >> 11, MFC_SI_CH0_DESC_ADDR);
	WRITEL(DESC_BUF_SIZE, MFC_SI_CH0_DESC_SIZE);

	mfc_debug_L0("stream_paddr: 0x%08x, desc_paddr: 0x%08x\n", buf_addr, buf_addr + CPB_BUF_SIZE);
}

static enum mfc_error_code mfc_alloc_dec_frame_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_dec_init_arg *init_arg;
	enum mfc_error_code ret_code;
	union mfc_args local_param;
	struct mfc_frame_buf_arg buf_size;
	unsigned int luma_size, chroma_size;
	unsigned int luma_plane_sz, chroma_plane_sz, mv_plane_sz;

	init_arg = (struct mfc_dec_init_arg *)args;

	/* width : 128B align, height : 32B align, size: 8KB align */
	luma_plane_sz   = ALIGN_TO_128B(init_arg->out_img_width) * ALIGN_TO_32B(init_arg->out_img_height);
	luma_plane_sz   = ALIGN_TO_8KB(luma_plane_sz);
	chroma_plane_sz = ALIGN_TO_128B(init_arg->out_img_width) * ALIGN_TO_32B(init_arg->out_img_height / 2);
	chroma_plane_sz = ALIGN_TO_8KB(chroma_plane_sz);
	mv_plane_sz     = 0;

	buf_size.luma   = luma_plane_sz;
	buf_size.chroma = chroma_plane_sz;

	if (mfc_ctx->MfcCodecType == H264_DEC) {
		/* width : 128B align, height : 32B align, size: 8KB align */
		mv_plane_sz = ALIGN_TO_128B(init_arg->out_img_width) * ALIGN_TO_32B(init_arg->out_img_height / 4);
		mv_plane_sz = ALIGN_TO_8KB(mv_plane_sz);
		buf_size.luma += mv_plane_sz;
	}

	mfc_ctx->shared_mem.allocated_luma_dpb_size   = luma_plane_sz;
	mfc_ctx->shared_mem.allocated_chroma_dpb_size = chroma_plane_sz;
	mfc_ctx->shared_mem.allocated_mv_size         = mv_plane_sz;

	luma_size = buf_size.luma * mfc_ctx->totalDPBCnt;
	chroma_size = buf_size.chroma * mfc_ctx->totalDPBCnt;

	/*
	 * Allocate chroma & (Mv in case of H264) buf
	 */
	init_arg->out_frame_buf_size.chroma = chroma_size;

	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size = chroma_size;
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;

	ret_code = mfc_allocate_buffer(mfc_ctx, &(local_param), 0);
	if (ret_code < 0)
		return ret_code;

	init_arg->out_u_addr.chroma = local_param.mem_alloc.out_uaddr;
	init_arg->out_p_addr.chroma = local_param.mem_alloc.out_paddr;

	/*
	 * Allocate luma buf
	 */
	init_arg->out_frame_buf_size.luma = luma_size;

	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size = luma_size;
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;

	ret_code = mfc_allocate_buffer(mfc_ctx, &(local_param), 1);
	if (ret_code < 0)
		return ret_code;

	init_arg->out_u_addr.luma = local_param.mem_alloc.out_uaddr;
	init_arg->out_p_addr.luma = local_param.mem_alloc.out_paddr;

	mfc_ctx->dec_dpb_buff_paddr = init_arg->out_p_addr;

	return MFCINST_RET_OK;
}



static void mfc_set_dec_frame_buffer(struct mfc_inst_ctx *mfc_ctx)
{
	unsigned int port0_base_paddr, port1_base_paddr, i;
	struct mfc_frame_buf_arg dpb_buff_addr;
	unsigned int luma_plane_sz, chroma_plane_sz, mv_plane_sz;

	dpb_buff_addr = mfc_ctx->dec_dpb_buff_paddr;
	port0_base_paddr = mfc_port0_base_paddr;
	port1_base_paddr = mfc_port1_base_paddr;

	luma_plane_sz   = mfc_ctx->shared_mem.allocated_luma_dpb_size;
	chroma_plane_sz = mfc_ctx->shared_mem.allocated_chroma_dpb_size;
	mv_plane_sz     = mfc_ctx->shared_mem.allocated_mv_size;

	mfc_debug("luma_buf_addr start   : 0x%08x  luma_buf_size : %d\n", dpb_buff_addr.luma, (luma_plane_sz + mv_plane_sz));
	mfc_debug("chroma_buf_addr start : 0x%08x  chroma_buf_size : %d\n", dpb_buff_addr.chroma, chroma_plane_sz);

	if (mfc_ctx->MfcCodecType == H264_DEC) {
		for (i = 0; i < mfc_ctx->totalDPBCnt; i++) {
			mfc_debug("DPB[%d] luma_buf_addr   : 0x%08x  luma_buf_size   : %d\n", i, dpb_buff_addr.luma, luma_plane_sz);
			mfc_debug("DPB[%d] chroma_buf_addr : 0x%08x  chroma_buf_size : %d\n", i, dpb_buff_addr.chroma, chroma_plane_sz);
			mfc_debug("DPB[%d] mv_buf_addr     : 0x%08x  mv_plane_sz     : %d\n", i, dpb_buff_addr.luma + luma_plane_sz, mv_plane_sz);

			/* set Luma address */
			WRITEL((dpb_buff_addr.luma - port1_base_paddr) >> 11, MFC_H264DEC_LUMA + (4 * i));
			WRITEL((dpb_buff_addr.luma + luma_plane_sz - port1_base_paddr) >> 11, MFC_H264DEC_MV + (4 * i));
			dpb_buff_addr.luma += (luma_plane_sz + mv_plane_sz);

			/* set Chroma address & set MV address */
			WRITEL((dpb_buff_addr.chroma - port0_base_paddr) >> 11, MFC_H264DEC_CHROMA + (4 * i));
			dpb_buff_addr.chroma += chroma_plane_sz;
		}
	} else {
		for (i = 0; i < mfc_ctx->totalDPBCnt; i++) {
			mfc_debug("DPB[%d] luma_buf_addr   : 0x%08x  luma_buf_size   : %d\n", i, dpb_buff_addr.luma, luma_plane_sz);
			mfc_debug("DPB[%d] chroma_buf_addr : 0x%08x  chroma_buf_size : %d\n", i, dpb_buff_addr.chroma, chroma_plane_sz);

			/* set Luma address */
			WRITEL((dpb_buff_addr.luma - port1_base_paddr) >> 11, MFC_DEC_LUMA + (4 * i));
			dpb_buff_addr.luma += luma_plane_sz;

			/* set Chroma address */
			WRITEL((dpb_buff_addr.chroma - port0_base_paddr) >> 11, MFC_DEC_CHROMA + (4 * i));
			dpb_buff_addr.chroma += chroma_plane_sz;
		}
	}

	mfc_debug("luma_buf_addr end   : 0x%08x\n", dpb_buff_addr.luma);
	mfc_debug("chroma_buf_addr end : 0x%08x\n", dpb_buff_addr.chroma);
}

/* Allocate buffers for encoder */
static enum mfc_error_code mfc_alloc_stream_ref_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_enc_init_mpeg4_arg *init_arg;
	union mfc_args local_param;
	unsigned int luma_plane_sz, chroma_plane_sz;
	enum mfc_error_code ret_code = MFCINST_RET_OK;
	struct mfc_frame_buf_arg buf_size;

	init_arg = (struct mfc_enc_init_mpeg4_arg *)args;

	/* width : 128B align, height : 32B align, size: 8KB align */
	luma_plane_sz   = ALIGN_TO_128B(init_arg->in_width) * ALIGN_TO_32B(init_arg->in_height);
	luma_plane_sz   = ALIGN_TO_8KB(luma_plane_sz);
	chroma_plane_sz = ALIGN_TO_128B(init_arg->in_width) * ALIGN_TO_32B(init_arg->in_height / 2);
	chroma_plane_sz = ALIGN_TO_8KB(chroma_plane_sz);

	buf_size.luma   = luma_plane_sz;
	buf_size.chroma = chroma_plane_sz;

	/*
	 * Allocate stream ref Y0, Y1 buf
	 */
	init_arg->out_buf_size.strm_ref_y = STREAM_BUF_SIZE + (buf_size.luma * 2);

	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size = init_arg->out_buf_size.strm_ref_y;
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;

	ret_code = mfc_allocate_buffer(mfc_ctx, &(local_param), 0);
	if (ret_code < 0)
		return ret_code;

	init_arg->out_u_addr.strm_ref_y = local_param.mem_alloc.out_uaddr;
	init_arg->out_p_addr.strm_ref_y = local_param.mem_alloc.out_paddr;

	/*
	 * Allocate ref C0, C1, Y2, C2, Y3, C3 buf XXX : remove MV buffer
	 */
	init_arg->out_buf_size.mv_ref_yc = (buf_size.luma * 2) + (buf_size.chroma * 4);

	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size = init_arg->out_buf_size.mv_ref_yc;
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;

	ret_code = mfc_allocate_buffer(mfc_ctx, &(local_param), 1);
	if (ret_code < 0)
		return ret_code;

	init_arg->out_u_addr.mv_ref_yc = local_param.mem_alloc.out_uaddr;
	init_arg->out_p_addr.mv_ref_yc = local_param.mem_alloc.out_paddr;

	return ret_code;
}

static void mfc_set_enc_ref_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	unsigned int port0_base_paddr, port1_base_paddr, i;
	struct mfc_strm_ref_buf_arg ref_buf_addr;
	unsigned int luma_plane_sz, chroma_plane_sz;
	struct mfc_enc_init_mpeg4_arg *init_arg;

	init_arg = (struct mfc_enc_init_mpeg4_arg *)args;

	/* width : 128B align, height : 32B align, size: 8KB align */
	luma_plane_sz   = ALIGN_TO_128B(init_arg->in_width) * ALIGN_TO_32B(init_arg->in_height);
	luma_plane_sz   = ALIGN_TO_8KB(luma_plane_sz);
	chroma_plane_sz = ALIGN_TO_128B(init_arg->in_width) * ALIGN_TO_32B(init_arg->in_height / 2);
	chroma_plane_sz = ALIGN_TO_8KB(chroma_plane_sz);

	ref_buf_addr = init_arg->out_p_addr;
	mfc_debug("strm_ref_y_buf_addr : 0x%08x, strm_ref_y_buf_size : %d\n",
			ref_buf_addr.strm_ref_y, init_arg->out_buf_size.strm_ref_y);
	mfc_debug("mv_ref_yc_buf_addr  : 0x%08x, mv_ref_yc_buf_size  : %d\n",
			ref_buf_addr.mv_ref_yc, init_arg->out_buf_size.mv_ref_yc);

	port0_base_paddr = mfc_port0_base_paddr;
	port1_base_paddr = mfc_port1_base_paddr;

	mfc_debug("stream_buf_addr     : 0x%08x\n", ref_buf_addr.strm_ref_y);
	ref_buf_addr.strm_ref_y += STREAM_BUF_SIZE;

	/* Set Y0, Y1 ref buffer address */
	mfc_debug("REF[0] luma_buf_addr : 0x%08x  luma_buf_size : %d\n", ref_buf_addr.strm_ref_y, luma_plane_sz);
	WRITEL((ref_buf_addr.strm_ref_y - port0_base_paddr) >> 11, MFC_ENC_DPB_Y0_ADDR);
	ref_buf_addr.strm_ref_y += luma_plane_sz;
	mfc_debug("REF[1] luma_buf_addr : 0x%08x  luma_buf_size : %d\n", ref_buf_addr.strm_ref_y, luma_plane_sz);
	WRITEL((ref_buf_addr.strm_ref_y - port0_base_paddr) >> 11, MFC_ENC_DPB_Y1_ADDR);

	/* Set Y2, Y3 ref buffer address */
	mfc_debug("REF[2] luma_buf_addr : 0x%08x  luma_buf_size : %d\n", ref_buf_addr.mv_ref_yc, luma_plane_sz);
	WRITEL((ref_buf_addr.mv_ref_yc - port1_base_paddr) >> 11, MFC_ENC_DPB_Y2_ADDR);
	ref_buf_addr.mv_ref_yc += luma_plane_sz;
	mfc_debug("REF[3] luma_buf_addr : 0x%08x  luma_buf_size : %d\n", ref_buf_addr.mv_ref_yc, luma_plane_sz);
	WRITEL((ref_buf_addr.mv_ref_yc - port1_base_paddr) >> 11, MFC_ENC_DPB_Y3_ADDR);
	ref_buf_addr.mv_ref_yc += luma_plane_sz;

	/* Set C0, C1, C2, C3 ref buffer address */
	for (i = 0; i < 4; i++) {
		mfc_debug("REF[%d] chroma_buf_addr : 0x%08x  chroma_buf_size : %d\n", i, ref_buf_addr.mv_ref_yc, chroma_plane_sz);
		WRITEL((ref_buf_addr.mv_ref_yc - port1_base_paddr) >> 11, MFC_ENC_DPB_C0_ADDR + (4 * i));
		ref_buf_addr.mv_ref_yc += chroma_plane_sz;
	}
}

static enum mfc_error_code mfc_alloc_codec_buffer(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_dec_init_arg *init_arg;
	enum mfc_error_code ret_code;
	union mfc_args local_param;

	init_arg = (struct mfc_dec_init_arg *)args;

	memset(&local_param, 0, sizeof(local_param));
	if (is_dec_codec(mfc_ctx->MfcCodecType))
		local_param.mem_alloc.buff_size = DEC_CODEC_BUF_SIZE + SHARED_BUF_SIZE;
	else
		local_param.mem_alloc.buff_size = ENC_CODEC_BUF_SIZE + SHARED_BUF_SIZE;

	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;
	ret_code = mfc_allocate_buffer(mfc_ctx, &(local_param), 0);
	if (ret_code < 0)
		return ret_code;

	mfc_ctx->codec_buff_paddr = local_param.mem_alloc.out_paddr;
	if (is_dec_codec(mfc_ctx->MfcCodecType))
		mfc_ctx->shared_mem_paddr = mfc_ctx->codec_buff_paddr + DEC_CODEC_BUF_SIZE;
	else
		mfc_ctx->shared_mem_paddr = mfc_ctx->codec_buff_paddr + ENC_CODEC_BUF_SIZE;

	mfc_ctx->shared_mem_vaddr = (unsigned int)mfc_get_fw_buff_vaddr() + (mfc_ctx->shared_mem_paddr - mfc_port0_base_paddr);
	memset((void *)mfc_ctx->shared_mem_vaddr, 0x0, SHARED_MEM_MAX);

	if (mfc_ctx->MfcCodecType == H264_ENC) {
		memset(&local_param, 0, sizeof(local_param));
		local_param.mem_alloc.buff_size = PRED_BUF_SIZE;
		local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;
		ret_code = mfc_allocate_buffer(mfc_ctx, &(local_param), 1);
		if (ret_code < 0)
			return ret_code;

		mfc_ctx->pred_buff_paddr = local_param.mem_alloc.out_paddr;
	}

	return MFCINST_RET_OK;
}



static void mfc_set_codec_buffer(struct mfc_inst_ctx *mfc_ctx)
{
	unsigned int codec_buff_paddr;
	unsigned int pred_buff_paddr;
	unsigned int port0_base_paddr;
	unsigned int port1_base_paddr;

	port0_base_paddr = mfc_port0_base_paddr;
	port1_base_paddr = mfc_port1_base_paddr;
	codec_buff_paddr = mfc_ctx->codec_buff_paddr;
	pred_buff_paddr  = mfc_ctx->pred_buff_paddr;

	mfc_debug("inst_no : %d, codec_buf_start: 0x%08x\n", mfc_ctx->InstNo, codec_buff_paddr);

	switch (mfc_ctx->MfcCodecType) {
	case H264_DEC:
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_H264DEC_VERT_NB_MV);
		codec_buff_paddr += (16 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_H264DEC_NB_IP);
		codec_buff_paddr += (32 << 10);
		break;

	case MPEG4_DEC:
	case H263_DEC:
	case XVID_DEC:
	case FIMV1_DEC:
	case FIMV2_DEC:
	case FIMV3_DEC:
	case FIMV4_DEC:
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_NB_DCAC);
		codec_buff_paddr += (16 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_UPNB_MV);
		codec_buff_paddr += (68 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_SUB_ANCHOR_MV);
		codec_buff_paddr += (136 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_OVERLAP_TRANSFORM);
		codec_buff_paddr += (32 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_STX_PARSER);
		codec_buff_paddr += (68 << 10);
		break;

	case VC1_DEC:
	case VC1RCV_DEC:
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_NB_DCAC);
		codec_buff_paddr += (16 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_UPNB_MV);
		codec_buff_paddr += (68 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_SUB_ANCHOR_MV);
		codec_buff_paddr += (136 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_OVERLAP_TRANSFORM);
		codec_buff_paddr += (32 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_BITPLANE3);
		codec_buff_paddr += (8 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_BITPLANE2);
		codec_buff_paddr += (8 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_BITPLANE1);
		codec_buff_paddr += (8 << 10);
		break;

	case MPEG1_DEC:
	case MPEG2_DEC:
		break;

	case H264_ENC:
	case MPEG4_ENC:
	case H263_ENC:
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_UPPER_MV_ADDR);
		codec_buff_paddr += (64 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_DIRECT_COLZERO_FLAG_ADDR);
		codec_buff_paddr += (64 << 10);
		WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_UPPER_INTRA_MD_ADDR);
		codec_buff_paddr += (64 << 10);
		if (mfc_ctx->MfcCodecType == H264_ENC) {
			WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_NBOR_INFO_MPENC_ADDR);
			WRITEL((pred_buff_paddr - port1_base_paddr) >> 11, MFC_UPPER_INTRA_PRED_ADDR);
		} else {
			WRITEL((codec_buff_paddr - port0_base_paddr) >> 11, MFC_ACDC_COEF_BASE_ADDR);
		}
		codec_buff_paddr += (64 << 10);
		break;

	default:
		break;
	}

	mfc_debug("inst_no : %d, codec_buf_end : 0x%08x\n", mfc_ctx->InstNo, codec_buff_paddr);
}

/* This function sets the MFC SFR values according to the input arguments. */
static void mfc_set_encode_init_param(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	unsigned int ms_size;
	struct mfc_enc_init_mpeg4_arg *enc_init_mpeg4_arg;
	struct mfc_enc_init_h264_arg *enc_init_h264_arg;

	enc_init_mpeg4_arg = (struct mfc_enc_init_mpeg4_arg *)args;
	enc_init_h264_arg = (struct mfc_enc_init_h264_arg *)args;

#if DEBUG_MAKE_RAW
	mImgHight = enc_init_mpeg4_arg->in_height;
	mImgWidth = enc_init_mpeg4_arg->in_width;
#endif

	mfc_debug("mfc_codec_type : %d\n", mfc_ctx->MfcCodecType);

	WRITEL(enc_init_mpeg4_arg->in_width, MFC_HSIZE_PX);
	if (enc_init_mpeg4_arg->in_interlace_mode)
		WRITEL(enc_init_mpeg4_arg->in_height >> 1, MFC_VSIZE_PX);
	else
		WRITEL(enc_init_mpeg4_arg->in_height, MFC_VSIZE_PX);

	/* H.263 does not support field picture */
	WRITEL(enc_init_mpeg4_arg->in_interlace_mode, MFC_PICTURE_STRUCT);
	WRITEL(0, MFC_ENC_INT_MASK);     /* mask interrupt */
	WRITEL(1, MFC_STR_BF_MODE_CTRL); /* stream buf frame mode */
	WRITEL((1 << 18) | (enc_init_mpeg4_arg->in_BframeNum << 16) |
		enc_init_mpeg4_arg->in_gop_num, MFC_ENC_PIC_TYPE_CTRL);

	/* Multi-slice options */
	if (enc_init_mpeg4_arg->in_MS_mode) {
		ms_size = (mfc_ctx->MfcCodecType == H263_ENC) ? 0 : enc_init_mpeg4_arg->in_MS_size;
		switch (enc_init_mpeg4_arg->in_MS_mode) {
		case 1:
			WRITEL(0x1, MFC_ENC_MSLICE_CTRL);
			WRITEL(ms_size, MFC_ENC_MSLICE_MB);
			break;

		case 2:
			WRITEL(0x3, MFC_ENC_MSLICE_CTRL);
			WRITEL(ms_size, MFC_ENC_MSLICE_BYTE);
			break;

		default:
			mfc_err("Invalid Multi-slice mode type\n");
			break;
		}
	} else {
		WRITEL(0, MFC_ENC_MSLICE_CTRL);
	}

	/* Set circular intra refresh MB count */
	WRITEL(enc_init_mpeg4_arg->in_mb_refresh, MFC_ENC_CIR_CTRL);

	if(enc_init_mpeg4_arg->in_frame_map == 1)
	        WRITEL(MEM_STRUCT_TILE_ENC, MFC_ENC_MAP_FOR_CUR);
	else
		WRITEL(MEM_STRUCT_LINEAR, MFC_ENC_MAP_FOR_CUR);

	/* Set padding control */
	WRITEL((enc_init_mpeg4_arg->in_pad_ctrl_on << 31) |
			(enc_init_mpeg4_arg->in_cr_pad_val << 16) |
			(enc_init_mpeg4_arg->in_cb_pad_val << 8) |
			(enc_init_mpeg4_arg->in_luma_pad_val << 0), MFC_ENC_PADDING_CTRL);

	/* Set Rate Control */
	if (enc_init_mpeg4_arg->in_RC_frm_enable) {
		WRITEL(enc_init_mpeg4_arg->in_RC_framerate, MFC_RC_FRAME_RATE);
		WRITEL(enc_init_mpeg4_arg->in_RC_bitrate, MFC_RC_BIT_RATE);
		WRITEL(enc_init_mpeg4_arg->in_RC_rpara, MFC_RC_RPARA);
	}

	WRITEL(enc_init_mpeg4_arg->in_RC_qbound, MFC_RC_QBOUND);

	switch (mfc_ctx->MfcCodecType) {
	case H264_ENC:
		WRITEL(enc_init_h264_arg->in_profile_level, MFC_PROFILE);
		WRITEL(enc_init_h264_arg->in_transform8x8_mode, MFC_H264_ENC_TRANS_8X8_FLAG);
		WRITEL(enc_init_h264_arg->in_deblock_filt, MFC_LF_CONTROL);
		WRITEL(((enc_init_h264_arg->in_deblock_alpha_C0 * 2) & 0x1f), MFC_LF_ALPHA_OFF);
		WRITEL(((enc_init_h264_arg->in_deblock_beta * 2) & 0x1f), MFC_LF_BETA_OFF);
		WRITEL(1, MFC_EDFU_SF_EPB_ON_CTRL); /* Auto EPB insertion on, only for h264 */

		WRITEL((enc_init_h264_arg->in_RC_frm_enable << 9) |
			(enc_init_h264_arg->in_RC_mb_enable << 8) |
			(enc_init_h264_arg->in_frame_qp & 0x3f),
			MFC_RC_CONFIG);

		if (enc_init_h264_arg->in_RC_mb_enable) {
			WRITEL((enc_init_h264_arg->in_RC_mb_dark_disable << 3)|
				(enc_init_h264_arg->in_RC_mb_smooth_disable << 2)|
				(enc_init_h264_arg->in_RC_mb_static_disable << 1)|
				(enc_init_h264_arg->in_RC_mb_activity_disable << 0),
				MFC_RC_MB_CTRL);
		}

		WRITEL((enc_init_h264_arg->in_symbolmode & 0x1), MFC_H264_ENC_ENTRP_MODE);

		if (enc_init_h264_arg->in_reference_num > 2)
			enc_init_h264_arg->in_reference_num = 2;
		if (enc_init_h264_arg->in_ref_num_p > enc_init_h264_arg->in_reference_num)
			enc_init_h264_arg->in_ref_num_p = enc_init_h264_arg->in_reference_num;
			WRITEL((enc_init_h264_arg->in_ref_num_p << 5) |
				(enc_init_h264_arg->in_reference_num),
				MFC_H264_ENC_NUM_OF_REF);

		WRITEL(enc_init_h264_arg->in_md_interweight_pps, MFC_H264_ENC_MDINTER_WEIGHT);
		WRITEL(enc_init_h264_arg->in_md_intraweight_pps, MFC_H264_ENC_MDINTRA_WEIGHT);
		break;

	case MPEG4_ENC:
		WRITEL(enc_init_mpeg4_arg->in_profile_level, MFC_PROFILE);
		WRITEL((enc_init_mpeg4_arg->in_RC_frm_enable << 9) |
			(enc_init_mpeg4_arg->in_frame_qp & 0x3f),
			MFC_RC_CONFIG);
		WRITEL(enc_init_mpeg4_arg->in_qpelME_enable, MFC_MPEG4_ENC_QUART_PXL);
		if (enc_init_mpeg4_arg->in_time_increament_res) {
			mfc_ctx->shared_mem.vop_timing = (1 << 31) |
			(enc_init_mpeg4_arg->in_time_increament_res << 16) |
			(enc_init_mpeg4_arg->in_time_vop_time_increament);
		}
		break;

	case H263_ENC:
		WRITEL(0x20, MFC_PROFILE);
		WRITEL((enc_init_mpeg4_arg->in_RC_frm_enable << 9) |
			(enc_init_mpeg4_arg->in_frame_qp & 0x3f),
			MFC_RC_CONFIG);
			break;

	default:
		mfc_err("Invalid MFC codec type\n");
	}
}

int mfc_load_firmware(const unsigned char *data, size_t size)
{
	volatile unsigned char *fw_virbuf;

	mfc_debug("mfc_load_firmware : MFC F/W Loading Start.................\n");

	fw_virbuf = mfc_get_fw_buff_vaddr();
	memset((void *)fw_virbuf, 0, MFC_FW_MAX_SIZE);

	invalidate_kernel_vmap_range((void *)data, size);
	memcpy((void *)fw_virbuf, data, size);
	dmac_flush_range((void *)fw_virbuf, (void *)fw_virbuf + size);

	mfc_debug("mfc_load_firmware : MFC F/W Loading Stop.................(fw_virbuf: 0x%08x)\n", fw_virbuf);

	return 0;
}

enum mfc_error_code mfc_init_hw()
{
	int fw_buf_size;
	unsigned int fw_version;
	unsigned int mc_status;
	unsigned long idcode;
	int nIntrRet = 0;

	mfc_debug("mfc_init_hw++\n");

	/*
	 * 0-1. Check Type
	 */
	idcode = readl(S5P_VA_CHIPID);
	if ((idcode & 0x0F) == 0x02)
		mCheckType = false;
	else
		mCheckType = true;

	/*
	 * 1. MFC reset
	 */
	do {
		mc_status = READL(MFC_MC_STATUS);
	} while (mc_status != 0);

	if (mfc_cmd_reset() == false) {
		mfc_err("MFCINST_ERR_INIT_FAIL\n");
		return MFCINST_ERR_INIT_FAIL;
	}

	/*
	 * 2. Set DRAM base Addr
	 */
	WRITEL(mfc_port0_base_paddr, MFC_MC_DRAMBASE_ADDR_A);
	WRITEL(mfc_port1_base_paddr, MFC_MC_DRAMBASE_ADDR_B);
	WRITEL(1, MFC_NUM_MASTER);

	/*
	 * 3. Initialize registers of stream I/F for decoder
	 */
	WRITEL(0xffff, MFC_SI_CH0_INST_ID);
	WRITEL(0xffff, MFC_SI_CH1_INST_ID);

	WRITEL(0, MFC_RISC2HOST_COMMAND);
	WRITEL(0, MFC_HOST2RISC_COMMAND);

	/*
	 * 4. Release reset signal to the RISC.
	 */
	WRITEL(0x3ff, MFC_SW_RESET);
	nIntrRet = mfc_wait_for_done(R2H_CMD_FW_STATUS_RET);
	if (nIntrRet != R2H_CMD_FW_STATUS_RET) {
		/*
		 * 4-1. MFC FW downloading
		 */

		mfc_err("MFCINST_ERR_FW_LOAD_MFC_SW_RESET_FAIL............(Ret = %d)", nIntrRet);


		mfc_err("MFCINST_ERR_FW_LOAD_FAIL\n");
		return MFCINST_ERR_FW_LOAD_FAIL;
	}

	/*
	 * 5. Initialize firmware
	 */
	fw_buf_size = MFC_FW_MAX_SIZE;
	if (mfc_cmd_host2risc(H2R_CMD_SYS_INIT, fw_buf_size, 0, 0, 0) == false) {
		mfc_err("R2H_CMD_SYS_INIT FAIL\n");
		return MFCINST_ERR_FW_INIT_FAIL;
	}

	if (mfc_wait_for_done(R2H_CMD_SYS_INIT_RET) != R2H_CMD_SYS_INIT_RET) {
		mfc_err("R2H_CMD_SYS_INIT_RET FAIL\n");
		return MFCINST_ERR_FW_INIT_FAIL;
	}

	fw_version = READL(MFC_FW_VERSION);


	mfc_debug("MFC FW version : %02xyy, %02xmm, %02xdd\n",
		(fw_version >> 16) & 0xff, (fw_version >> 8) & 0xff, fw_version & 0xff);

	mfc_debug("DRAM PORT0 BASE ADDRESS: 0x%08x\n", READL(MFC_MC_DRAMBASE_ADDR_A));
	mfc_debug("DRAM PORT1 BASE ADDRESS: 0x%08x\n", READL(MFC_MC_DRAMBASE_ADDR_B));
	mfc_debug("mfc_init_hw-\n");

	return MFCINST_RET_OK;
}

static unsigned int mfc_get_codec_arg(enum ssbsip_mfc_codec_type codec_type)
{
	unsigned int codec_no = 99;

	switch (codec_type) {
	case H264_DEC:
		codec_no = 0;
		break;

	case VC1_DEC:
		codec_no = 1;
		break;

	case MPEG4_DEC:
	case XVID_DEC:
		codec_no = 2;
		break;

	case MPEG1_DEC:
	case MPEG2_DEC:
		codec_no = 3;
		break;

	case H263_DEC:
		codec_no = 4;
		break;

	case VC1RCV_DEC:
		codec_no = 5;
		break;

	case FIMV1_DEC:
		codec_no = 6;
		break;

	case FIMV2_DEC:
		codec_no = 7;
		break;

	case FIMV3_DEC:
		codec_no = 8;
		break;

	case FIMV4_DEC:
		codec_no = 9;
		break;

	case H264_ENC:
		codec_no = 16;
		break;

	case MPEG4_ENC:
		codec_no = 17;
		break;

	case H263_ENC:
		codec_no = 18;
		break;

	default:
		break;
	}

	return codec_no;
}

static int mfc_get_inst_no(struct mfc_inst_ctx *mfc_ctx, unsigned int context_addr, int context_size)
{
	unsigned int codec_no;
	int inst_no;
	unsigned int port0_base_paddr;
	int	mfc_wait_ret = 0;
	port0_base_paddr = mfc_port0_base_paddr;

	codec_no = (unsigned int)mfc_get_codec_arg(mfc_ctx->MfcCodecType);

	if (mfc_cmd_host2risc(H2R_CMD_OPEN_INSTANCE,
					codec_no,
					(mfc_ctx->crcEnable << 31) | PIXEL_CACHE_ON_ONLY_P_PICTURE,
					(context_addr - port0_base_paddr) >> 11,
					context_size) == false) {
		mfc_err("R2H_CMD_OPEN_INSTANCE FAIL\n");
		return MFCINST_ERR_OPEN_FAIL;
	}

	mfc_wait_ret = mfc_wait_for_done(R2H_CMD_OPEN_INSTANCE_RET);
	if (mfc_wait_ret != R2H_CMD_OPEN_INSTANCE_RET) {
		mfc_err("R2H_CMD_OPEN_INSTANCE_RET FAIL..........(ret:%d)\n", mfc_wait_ret);
		return MFCINST_ERR_OPEN_FAIL;
	}

	inst_no = READL(MFC_RISC2HOST_ARG1);
	if (inst_no >= MFC_MAX_INSTANCE_NUM) {
		mfc_err("mfc_get_inst_no() - INSTANCE NO : %d, CODEC_TYPE : %d --\n", inst_no, codec_no);
		return -1;
	} else {
		mfc_debug("mfc_get_inst_no() - INSTANCE NO : %d, CODEC_TYPE : %d --\n", inst_no, codec_no);
		return inst_no;
	}
}

int mfc_return_inst_no(int inst_no, enum ssbsip_mfc_codec_type codec_type)
{
	unsigned int codec_no;
	int	mfc_wait_ret = 0;

	codec_no = (unsigned int)mfc_get_codec_arg(codec_type);

	if (mfc_cmd_host2risc(H2R_CMD_CLOSE_INSTANCE, inst_no, 0, 0, 0) == false) {
		mfc_err("R2H_CMD_CLOSE_INSTANCE FAIL\n");
		return MFCINST_ERR_CLOSE_FAIL;
	}

	mfc_wait_ret = mfc_wait_for_done(R2H_CMD_CLOSE_INSTANCE_RET);
	if (mfc_wait_ret != R2H_CMD_CLOSE_INSTANCE_RET) {
		mfc_err("R2H_CMD_CLOSE_INSTANCE_RET FAIL\n");
		return MFCINST_ERR_CLOSE_FAIL;
	}

	mfc_debug("mfc_return_inst_no() - INSTANCE NO : %d, CODEC_TYPE : %d --\n", READL(MFC_RISC2HOST_ARG1), codec_no);
	return MFCINST_RET_OK;
}

enum mfc_error_code mfc_init_encode(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_enc_init_mpeg4_arg *enc_init_mpeg4_arg;
	enum mfc_error_code ret_code;
	unsigned int context_addr;
	int context_size;
	int frame_P_qp, frame_B_qp;
	int nSize = 0;

	enc_init_mpeg4_arg = (struct mfc_enc_init_mpeg4_arg *)args;

	mfc_debug("++\n");
	mfc_ctx->MfcCodecType = enc_init_mpeg4_arg->in_codec_type;
	mfc_ctx->img_width = (unsigned int)enc_init_mpeg4_arg->in_width;
	mfc_ctx->img_height = (unsigned int)enc_init_mpeg4_arg->in_height;
	mfc_ctx->interlace_mode = enc_init_mpeg4_arg->in_interlace_mode;


	/*
	  * Set Available Type
	  */
	if (mCheckType == false) {
		nSize = mfc_ctx->img_width * mfc_ctx->img_height;
		mfc_ctx->shared_mem.p720_limit_enable = 49425;
		if (nSize > BOUND_MEMORY_SIZE)
			return MFCINST_ERR_FRM_BUF_SIZE;
	} else {
		mfc_ctx->shared_mem.p720_limit_enable = 49424;
	}




	/* OPEN CHANNEL
	 *	- set open instance using codec_type
	 *	- get the instance no
	 */
	ret_code = mfc_alloc_context_buffer(mfc_ctx, enc_init_mpeg4_arg->in_mapped_addr, &context_addr, &context_size);
	if (ret_code != MFCINST_RET_OK)
		return ret_code;

	mfc_ctx->InstNo = mfc_get_inst_no(mfc_ctx, context_addr, context_size);
	if (mfc_ctx->InstNo < 0) {
		mfc_err("MFCINST_INST_NUM_EXCEEDED\n");
		return MFCINST_INST_NUM_EXCEEDED;
	}

	/* INIT CODEC
	 *  - set init parameter
	 *  - set init sequence done command
	 *  - set codec buffer
	 *  - set input risc buffer
	 */

	mfc_set_encode_init_param(mfc_ctx, args);

	ret_code = mfc_alloc_codec_buffer(mfc_ctx, args);
	if (ret_code != MFCINST_RET_OK) {
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return ret_code;
	}

	mfc_set_codec_buffer(mfc_ctx);

	/* Set Ref YC0~3 & MV */
	ret_code = mfc_alloc_stream_ref_buffer(mfc_ctx, args);
	if (ret_code != MFCINST_RET_OK) {
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return ret_code;
	}

	mfc_set_enc_ref_buffer(mfc_ctx, args);

	if (enc_init_mpeg4_arg->in_frame_P_qp)
		frame_P_qp = enc_init_mpeg4_arg->in_frame_P_qp;
	else
		frame_P_qp = enc_init_mpeg4_arg->in_frame_qp;

	if (enc_init_mpeg4_arg->in_frame_B_qp)
		frame_B_qp = enc_init_mpeg4_arg->in_frame_B_qp;
	else
		frame_B_qp = enc_init_mpeg4_arg->in_frame_qp;
	mfc_ctx->shared_mem.P_B_frame_qp = (frame_B_qp << 6 | frame_P_qp);

	if (enc_init_mpeg4_arg->in_RC_frm_enable)
		mfc_ctx->shared_mem.vop_timing = ((1 << 31) | (enc_init_mpeg4_arg->in_RC_framerate << 16) | 1);


	mfc_write_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));

	ret_code = mfc_encode_header(mfc_ctx, args);

	mfc_read_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));

	mfc_backup_context(mfc_ctx);

	mfc_debug_L0("--\n");

	return ret_code;
}

static enum mfc_error_code mfc_encode_header(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_enc_init_mpeg4_arg *init_arg;
	unsigned int port0_base_paddr;
	int nIntrRet = 0;
	int nReturnErrCode = 0;

	init_arg = (struct mfc_enc_init_mpeg4_arg *)args;

	mfc_debug("++ enc_arg->in_strm_st : 0x%08x\n", init_arg->out_p_addr.strm_ref_y);

	port0_base_paddr = mfc_port0_base_paddr;

	/* Set share memory */
	WRITEL((mfc_ctx->shared_mem_paddr - port0_base_paddr), MFC_SI_CH0_HOST_WR_ADR);

	/* Set stream buffer addr */
	WRITEL((init_arg->out_p_addr.strm_ref_y - port0_base_paddr) >> 11, MFC_SI_CH0_SB_U_ADDR);
	WRITEL((init_arg->out_p_addr.strm_ref_y - port0_base_paddr) >> 11, MFC_SI_CH0_SB_L_ADDR);
	WRITEL(STREAM_BUF_SIZE, MFC_SI_CH0_BUFFER_SIZE);

	WRITEL(1, MFC_STR_BF_U_EMPTY);
	WRITEL(1, MFC_STR_BF_L_EMPTY);

	/* buf reset command if stream buffer is frame mode */
	WRITEL(0x1 << 1, MFC_EDFU_SF_BUF_CTRL);

	WRITEL((SEQ_HEADER << 16) | (mfc_ctx->InstNo), MFC_SI_CH0_INST_ID);
	nIntrRet = mfc_wait_for_done(R2H_CMD_SEQ_DONE_RET);
	nReturnErrCode = mfc_return_code();
	if (nIntrRet == 0) {
		mfc_err("MFCINST_ERR_ENC_EXE_TIME_OUT\n");
		return MFCINST_ERR_INTR_TIME_OUT;
	} else if ((nIntrRet != R2H_CMD_SEQ_DONE_RET) && (nReturnErrCode < MFC_WARN_START_NO)) {
		mfc_err("MFCINST_ERR_ENC_SEQ_HEADER_FAIL ....Intr Code (%d)\n", nIntrRet);
		return MFCINST_ERR_ENC_HEADER_DECODE_FAIL;
	} else if (nIntrRet != R2H_CMD_SEQ_DONE_RET) {
		mfc_warn("MFCINST_WARN_ENC_SEQ_HEADER.........(code: %d)\n", nIntrRet);
	}


	init_arg->out_header_size = READL(MFC_SI_ENC_STREAM_SIZE);

	if (mfc_ctx->buf_type == MFC_BUFFER_CACHE) {
		unsigned char *in_vir;
		in_vir = phys_to_virt(init_arg->out_p_addr.strm_ref_y);
		dmac_map_area(in_vir, 1, init_arg->out_header_size);
	}	
		
	mfc_debug("encoded header size (%d)\n", init_arg->out_header_size);

	return MFCINST_RET_OK;
}

static enum mfc_error_code mfc_encode_one_frame(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_enc_exe_arg *enc_arg;
	unsigned int port0_base_paddr, port1_base_paddr;
	int interrupt_flag;
	int nReturnErrCode;


	enc_arg = (struct mfc_enc_exe_arg *)args;

	mfc_debug("++ enc_arg->in_strm_st : 0x%08x enc_arg->in_strm_end :0x%08x \r\n",
				enc_arg->in_strm_st, enc_arg->in_strm_end);
	mfc_debug("enc_arg->in_Y_addr : 0x%08x enc_arg->in_CbCr_addr :0x%08x \r\n",
				enc_arg->in_Y_addr, enc_arg->in_CbCr_addr);

	mfc_restore_context(mfc_ctx);

	port0_base_paddr = mfc_port0_base_paddr;
	port1_base_paddr = mfc_port1_base_paddr;

#ifdef ENABLE_DEBUG_ENC_EXE_INTR_ERR
#if ENABLE_DEBUG_ENC_EXE_INTR_ERR
	makefile_mfc_enc_err_info(enc_arg);
#endif
#endif

	/* Set share memory */
	WRITEL((mfc_ctx->shared_mem_paddr - port0_base_paddr), MFC_SI_CH0_HOST_WR_ADR);

	/* Set stream buffer addr */
	WRITEL((enc_arg->in_strm_st - port0_base_paddr) >> 11, MFC_SI_CH0_SB_U_ADDR);
	WRITEL((enc_arg->in_strm_st - port0_base_paddr) >> 11, MFC_SI_CH0_SB_L_ADDR);
	WRITEL(STREAM_BUF_SIZE, MFC_SI_CH0_BUFFER_SIZE);

	/* Set current frame buffer addr */
	WRITEL((enc_arg->in_Y_addr - port1_base_paddr) >> 11, MFC_SI_CH0_CURRENT_Y_ADDR);
	WRITEL((enc_arg->in_CbCr_addr - port1_base_paddr) >> 11, MFC_SI_CH0_CURRENT_C_ADDR);

	WRITEL(1, MFC_STR_BF_U_EMPTY);
	WRITEL(1, MFC_STR_BF_L_EMPTY);

	/* buf reset command if stream buffer is frame mode */
	WRITEL(0x1 << 1, MFC_EDFU_SF_BUF_CTRL);

	if (mfc_ctx->forceSetFrameType == NOT_CODED)
		WRITEL((0x1 << 1), MFC_SI_CH0_ENC_PARA);
	else if (mfc_ctx->forceSetFrameType == I_FRAME)
		WRITEL(0x1, MFC_SI_CH0_ENC_PARA);
	else
		WRITEL(0x0, MFC_SI_CH0_ENC_PARA);

	mfc_ctx->forceSetFrameType = DONT_CARE;

	/* Check MFC status */
	if (mfc_ctx->buf_type == MFC_BUFFER_CACHE) {
		unsigned char *in_vir;
		unsigned int aligned_width;
		unsigned int aligned_height;

		in_vir = phys_to_virt(enc_arg->in_Y_addr);
		aligned_width = ALIGN_TO_128B(mfc_ctx->img_width);
		aligned_height = ALIGN_TO_32B(mfc_ctx->img_height);
		dmac_unmap_area(in_vir, 1, aligned_width*aligned_height);

		in_vir = phys_to_virt(enc_arg->in_CbCr_addr);
		aligned_height = ALIGN_TO_32B(mfc_ctx->img_height/2);
		dmac_unmap_area(in_vir, 1, aligned_width*aligned_height);
	}	
		
        if (mfc_ctx->dynamic_framerate != 0) {
		mfc_debug("mfc_ctx->dynamic_framerate = %d\n", mfc_ctx->dynamic_framerate);
		WRITEL_SHARED_MEM((1 << 1), mfc_ctx->shared_mem_vaddr + 0x2c);
		WRITEL_SHARED_MEM(mfc_ctx->dynamic_framerate,  mfc_ctx->shared_mem_vaddr + 0x94);
        } 

	if (mfc_ctx->dynamic_bitrate != 0) {
		mfc_debug("mfc_ctx->dynamic_bitrate = 0x%x\n", mfc_ctx->dynamic_bitrate);
		WRITEL_SHARED_MEM((1 << 2), mfc_ctx->shared_mem_vaddr + 0x2c);
		WRITEL_SHARED_MEM(mfc_ctx->dynamic_bitrate,
					  mfc_ctx->shared_mem_vaddr + 0x90);
	}

	if (mfc_ctx->dynamic_iperoid != 0) {
		mfc_debug("mfc_ctx->dynamic_iperoid = 0x%x\n", mfc_ctx->dynamic_iperoid);
		WRITEL_SHARED_MEM((1 << 0), mfc_ctx->shared_mem_vaddr + 0x2c);
		WRITEL_SHARED_MEM(mfc_ctx->dynamic_bitrate,
					  mfc_ctx->shared_mem_vaddr + 0x98);
	}

	/* Try frame encoding */
	WRITEL((FRAME << 16) | (mfc_ctx->InstNo), MFC_SI_CH0_INST_ID);
	interrupt_flag = mfc_wait_for_done(R2H_CMD_FRAME_DONE_RET);
	nReturnErrCode = mfc_return_code();
	if (interrupt_flag == 0) {
		mfc_err("MFCINST_ERR_ENC_EXE_TIME_OUT\n");
		return MFCINST_ERR_INTR_TIME_OUT;
	} else if ((interrupt_flag != R2H_CMD_FRAME_DONE_RET) && (nReturnErrCode < MFC_WARN_START_NO)) {
		mfc_err("MFCINST_ERR_ENC_DONE_FAIL\n");
		return MFCINST_ERR_ENC_ENCODE_DONE_FAIL;
	} else if (interrupt_flag != R2H_CMD_FRAME_DONE_RET) {
		mfc_warn("MFCINST_WARN_ENC_EXE.........(code: %d)\n", interrupt_flag);
	}

	/* Get encoded infromation */
	enc_arg->out_frame_type = READL(MFC_SI_ENC_SLICE_TYPE);
	enc_arg->out_encoded_size = READL(MFC_SI_ENC_STREAM_SIZE);
	enc_arg->out_encoded_Y_paddr = READL(MFC_SI_ENCODED_Y_ADDR);
	enc_arg->out_encoded_C_paddr = READL(MFC_SI_ENCODED_C_ADDR);

	if (mfc_ctx->buf_type == MFC_BUFFER_CACHE) {
		unsigned char *in_vir;
		in_vir = phys_to_virt(enc_arg->in_strm_st);
		dmac_map_area(in_vir, 1, enc_arg->out_encoded_size);
	}	
	
	WRITEL_SHARED_MEM(0,  mfc_ctx->shared_mem_vaddr + 0x2c);
	mfc_ctx->dynamic_framerate = 0;
	mfc_ctx->dynamic_bitrate = 0;
        mfc_ctx->dynamic_iperoid = 0;

	mfc_debug("-- frame type(%d) encodedSize(%d)\r\n",
		   enc_arg->out_frame_type, enc_arg->out_encoded_size);

	return MFCINST_RET_OK;
}

enum mfc_error_code mfc_exe_encode(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	enum mfc_error_code ret_code;
	struct mfc_enc_exe_arg *enc_arg;

	enc_arg = (struct mfc_enc_exe_arg *)args;

	mfc_ctx->shared_mem.set_frame_tag = enc_arg->in_frametag;
	mfc_write_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));

	/* 5. Encode Frame	 */
	ret_code = mfc_encode_one_frame(mfc_ctx, args);

	if (ret_code != MFCINST_RET_OK) {
		mfc_debug("mfc_exe_encode() : Encode Fail..(%d)\n", ret_code);
		return ret_code;
	}

	mfc_read_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));
	enc_arg->out_frametag_top = mfc_ctx->shared_mem.get_frame_tag_top;
	enc_arg->out_frametag_bottom = mfc_ctx->shared_mem.get_frame_tag_bot;

	mfc_debug_L0("--\n");

	return ret_code;
}

enum mfc_error_code mfc_init_decode(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	enum mfc_error_code ret_code;
	struct mfc_dec_init_arg *init_arg;
	unsigned int context_addr;
	int context_size;
	int nSize;
	int nIntrRet = 0;
	int nReturnErrCode = 0;


	mfc_debug("[%d] mfc_init_decode() start\n", current->pid);
	init_arg = (struct mfc_dec_init_arg *)args;

	/*	Calculate stream header size	*/
#if ENABLE_CHECK_SEQ_HEADER
	init_arg->in_strm_size = calculate_seq_size(init_arg);
#endif

	/* Context setting from input param */
	mfc_ctx->MfcCodecType = init_arg->in_codec_type;
	mfc_ctx->IsPackedPB = init_arg->in_packed_PB;

	/* OPEN CHANNEL
	 *	- set open instance using codec_type
	 *	- get the instance no
	 */
	ret_code = mfc_alloc_context_buffer(mfc_ctx, init_arg->in_mapped_addr, &context_addr, &context_size);
	if (ret_code != MFCINST_RET_OK)
		return ret_code;

	mfc_ctx->InstNo = mfc_get_inst_no(mfc_ctx, context_addr, context_size);
	if (mfc_ctx->InstNo < 0) {
		mfc_err("MFCINST_INST_NUM_EXCEEDED\n");
		return MFCINST_INST_NUM_EXCEEDED;
	}

#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_DEC_EXE_INTR_ERR
	mcontext_addr = context_addr;
	mcontext_size = context_size;
#endif
#endif

	/*
	 * MFC_LF_CONTROL used both encoding and decoding
	 * H.264 encoding, MPEG4 decoding(post filter)
	 * should disable : need more DPB for loop filter
	 */

	/* INIT CODEC
	 *   set input stream buffer
	 *   set sequence done command
	 *   set NUM_EXTRA_DPB
	 */
	if (mfc_ctx->MfcCodecType == FIMV1_DEC) {
		WRITEL(mfc_ctx->widthFIMV1, MFC_SI_CH0_FIMV1_HRESOL);
		WRITEL(mfc_ctx->heightFIMV1, MFC_SI_CH0_FIMV1_VRESOL);
	}

	ret_code = mfc_alloc_codec_buffer(mfc_ctx, args);
	if (ret_code != MFCINST_RET_OK) {
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return ret_code;
	}

	if (init_arg->in_strm_size < 0)
		return MFCINST_ERR_DEC_INVALID_STRM;

	mfc_set_dec_stream_buffer(mfc_ctx, init_arg->in_strm_buf, init_arg->in_strm_size);

	/* Set Display Delay and SliceEnable */
	mfc_ctx->sliceEnable = 0;
	WRITEL(((mfc_ctx->sliceEnable << 31) |
			(mfc_ctx->displayDelay ? ((1 << 30) |
			(mfc_ctx->displayDelay << 16)) : 0)),
			MFC_SI_CH0_DPB_CONFIG_CTRL);

	/* Codec Command : Decode a sequence header */
	WRITEL((SEQ_HEADER << 16) | (mfc_ctx->InstNo), MFC_SI_CH0_INST_ID);

	nIntrRet = mfc_wait_for_done(R2H_CMD_SEQ_DONE_RET);
	nReturnErrCode = mfc_return_code();
	if (nIntrRet == 0) {

#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_DEC_EXE_INTR_ERR
		makefile_mfc_decinit_err_info(mfc_ctx, init_arg, 300);
#endif
#endif

		mfc_err("MFCINST_ERR_DEC_INIT_TIME_OUT..........[#1]\n");
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return MFCINST_ERR_INTR_TIME_OUT;
	} else if ((nIntrRet != R2H_CMD_SEQ_DONE_RET) && (nReturnErrCode < MFC_WARN_START_NO)) {
		mfc_err("MFCINST_ERR_DEC_SEQ_HEADER_FAIL ....Intr Code (%d)\n", nIntrRet);
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return	MFCINST_ERR_DEC_SEQ_DONE_FAIL;
	} else if (nIntrRet != R2H_CMD_SEQ_DONE_RET) {
		mfc_warn("MFCINST_WARN_DEC_INIT.........(code: %d)\n", nIntrRet);
	}

#ifdef ENABLE_DEBUG_DEC_EXE_INTR_OK
 #if ENABLE_DEBUG_DEC_EXE_INTR_OK
		makefile_mfc_decinit_err_info(mfc_ctx, init_arg, 1000);
#endif
#endif

	/* out param & context setting from header decoding result */
	mfc_ctx->img_width = READL(MFC_SI_HOR_RESOL);
	mfc_ctx->img_height = READL(MFC_SI_VER_RESOL);

	init_arg->out_img_width = READL(MFC_SI_HOR_RESOL);
	init_arg->out_img_height = READL(MFC_SI_VER_RESOL);

	/* in the case of VC1 interlace, height will be the multiple of 32
	 * otherwise, height and width is the mupltiple of 16
	 */
	init_arg->out_buf_width = ALIGN_TO_128B(READL(MFC_SI_HOR_RESOL));
	init_arg->out_buf_height = ALIGN_TO_32B(READL(MFC_SI_VER_RESOL));

	if (mfc_ctx->MfcCodecType == FIMV1_DEC) {
		mfc_ctx->img_width = mfc_ctx->widthFIMV1;
		mfc_ctx->img_height = mfc_ctx->heightFIMV1;

		init_arg->out_img_width = mfc_ctx->widthFIMV1;
		init_arg->out_img_height = mfc_ctx->heightFIMV1;
		init_arg->out_buf_width = ALIGN_TO_128B(mfc_ctx->widthFIMV1);
		init_arg->out_buf_height = ALIGN_TO_32B(mfc_ctx->heightFIMV1);
	}


	/* Set totalDPB */
	init_arg->out_dpb_cnt = READL(MFC_SI_MIN_NUM_DPB);
	mfc_ctx->DPBCnt = READL(MFC_SI_MIN_NUM_DPB);

	mfc_ctx->totalDPBCnt = init_arg->out_dpb_cnt;
	mfc_ctx->totalDPBCnt = init_arg->out_dpb_cnt + mfc_ctx->extraDPB;
	if (mfc_ctx->totalDPBCnt < mfc_ctx->displayDelay)
		mfc_ctx->totalDPBCnt = mfc_ctx->displayDelay;

	WRITEL(((mfc_ctx->sliceEnable << 31) |
		(mfc_ctx->displayDelay ? ((1 << 30) |
		(mfc_ctx->displayDelay << 16)) : 0) |
		mfc_ctx->totalDPBCnt), MFC_SI_CH0_DPB_CONFIG_CTRL);

	mfc_debug("buf_width : %d buf_height : %d out_dpb_cnt : %d mfc_ctx->DPBCnt : %d\n",
		   init_arg->out_img_width, init_arg->out_img_height, init_arg->out_dpb_cnt, mfc_ctx->DPBCnt);
	mfc_debug("img_width : %d img_height : %d\n",
		   init_arg->out_img_width, init_arg->out_img_height);

	mfc_set_codec_buffer(mfc_ctx);

	ret_code = mfc_alloc_dec_frame_buffer(mfc_ctx, args);
	if (ret_code != MFCINST_RET_OK) {
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return ret_code;
	}

	mfc_set_dec_frame_buffer(mfc_ctx);

	/*
	  * Set Available Type
	  */
   if (mCheckType == false) {
		nSize = mfc_ctx->img_width * mfc_ctx->img_height;
		mfc_ctx->shared_mem.p720_limit_enable = 49425;
		if (nSize > BOUND_MEMORY_SIZE) {
			/* In case of no instance, we should not release codec instance */
			if (mfc_ctx->InstNo >= 0)
				mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

			return MFCINST_ERR_FRM_BUF_SIZE;
		}
   } else {
	   mfc_ctx->shared_mem.p720_limit_enable = 49424;
   }

#ifdef ENABLE_DEBUG_MFC_INIT
#if ENABLE_DEBUG_MFC_INIT
	printk_mfc_init_info(mfc_ctx, init_arg);
#endif
#endif

	mfc_write_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));
	WRITEL((mfc_ctx->shared_mem_paddr - mfc_port0_base_paddr), MFC_SI_CH0_HOST_WR_ADR);
	WRITEL((INIT_BUFFER << 16) | (mfc_ctx->InstNo), MFC_SI_CH0_INST_ID);

	nIntrRet = mfc_wait_for_done(R2H_CMD_INIT_BUFFERS_RET);
	nReturnErrCode = mfc_return_code();
	if (nIntrRet == 0) {
#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_DEC_EXE_INTR_ERR
		makefile_mfc_decinit_err_info(mfc_ctx, init_arg, 300);
#endif
#endif

		mfc_err("MFCINST_ERR_DEC_INIT_TIME_OUT..............[#2]\n");
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return MFCINST_ERR_INTR_TIME_OUT;
	} else if ((nIntrRet != R2H_CMD_INIT_BUFFERS_RET) && (nReturnErrCode < MFC_WARN_START_NO)) {
		mfc_err("MFCINST_ERR_DEC_INIT_BUFFER_FAIL ........(Intr Code : %d)\n", nIntrRet);
		/* In case of no instance, we should not release codec instance */
		if (mfc_ctx->InstNo >= 0)
			mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

		return MFCINST_ERR_DEC_INIT_BUFFER_FAIL;
	} else if (nIntrRet != R2H_CMD_INIT_BUFFERS_RET) {
		mfc_warn("MFCINST_WARN_DEC_INIT_BUFFER.........(Intr code: %d)\n", nIntrRet);
	}

	mfc_ctx->IsStartedIFrame = 0;

	mfc_backup_context(mfc_ctx);

	mfc_debug("[%d] mfc_init_decode() end\n", current->pid);

	mfc_read_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));
	init_arg->out_crop_top_offset = (mfc_ctx->shared_mem.crop_info2 & 0xffff);
	init_arg->out_crop_bottom_offset = (mfc_ctx->shared_mem.crop_info2 >> 16);
	init_arg->out_crop_left_offset = (mfc_ctx->shared_mem.crop_info1 & 0xffff);
	init_arg->out_crop_right_offset = (mfc_ctx->shared_mem.crop_info1 >> 16);
	mfc_debug("out_crop_top_offset : %d out_crop_bottom_offset : %d\n", init_arg->out_crop_top_offset, init_arg->out_crop_bottom_offset);

	return MFCINST_RET_OK;
}

static enum mfc_error_code mfc_decode_one_frame(struct mfc_inst_ctx *mfc_ctx, struct mfc_dec_exe_arg *dec_arg, unsigned int *consumed_strm_size)
{
	unsigned int frame_type;
	static int count;
	int interrupt_flag;
	int nReturnErrCode = 0;
	int nMaxFrameSize = 0;
	int nOffSet = 0;
	int nSum = 0;
	unsigned char *stream_vir;

	/*		Check Invalid Stream Size	*/
#if ENABLE_CHECK_STREAM_SIZE
	nMaxFrameSize = mfc_ctx->img_height * mfc_ctx->img_width;
	if ((dec_arg->in_strm_size < 1) || (dec_arg->in_strm_size > nMaxFrameSize) || (dec_arg->in_strm_size > STREAM_BUF_SIZE)) {
		mfc_err("MFCINST_ERR_DEC_STRM_SIZE_INVALID : (stream size : %d), (resolution : %d)\n", dec_arg->in_strm_size, nMaxFrameSize);
		return	MFCINST_ERR_DEC_STRM_SIZE_INVALID;
	}
#endif

	/*		Check Invalid Null Stream */
#if ENABLE_CHECK_NULL_STREAM
	if ((dec_arg->in_strm_size > 10) && (!(mfc_ctx->IsPackedPB))) {
		stream_vir = phys_to_virt(dec_arg->in_strm_buf);
		nSum = CheckNullStream(stream_vir, dec_arg->in_strm_size);

		if (nSum != 0) {
#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
 #if ENABLE_DEBUG_DEC_EXE_INTR_ERR
			makefile_mfc_dec_err_info(mfc_ctx, dec_arg, 500);
#endif
#endif
			return	MFCINST_ERR_STRM_BUF_INVALID;
		}
	}
#endif

	/*		Check H.263 Strat Code	*/
#if ENABLE_CHECK_START_CODE
	stream_vir = phys_to_virt(dec_arg->in_strm_buf);
	nOffSet = CheckDecStartCode(stream_vir, dec_arg->in_strm_size, dec_arg->in_codec_type);

	if (nOffSet < 0) {
#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
 #if ENABLE_DEBUG_DEC_EXE_INTR_ERR
		makefile_mfc_dec_err_info(mfc_ctx, dec_arg, 400);
#endif
#endif
		return MFCINST_ERR_STRM_BUF_INVALID;
	}
#endif

	count++;
	mfc_debug_L0("++ IntNo%d(%d)\r\n", mfc_ctx->InstNo, count);

	WRITEL((mfc_ctx->shared_mem_paddr - mfc_port0_base_paddr), MFC_SI_CH0_HOST_WR_ADR);
	mfc_set_dec_stream_buffer(mfc_ctx, dec_arg->in_strm_buf, dec_arg->in_strm_size);

	if (mfc_ctx->endOfFrame) {
		WRITEL((LAST_FRAME<<16) | (mfc_ctx->InstNo), MFC_SI_CH0_INST_ID);
		mfc_ctx->endOfFrame = 0;
	} else {
		WRITEL((FRAME<<16) | (mfc_ctx->InstNo), MFC_SI_CH0_INST_ID);
	}

	interrupt_flag = mfc_wait_for_done(R2H_CMD_FRAME_DONE_RET);
	nReturnErrCode = mfc_return_code();
	if (interrupt_flag == 0) {
#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
 #if ENABLE_DEBUG_DEC_EXE_INTR_ERR
		makefile_mfc_dec_err_info(mfc_ctx, dec_arg, 300);
#endif
#endif


#if ENABLE_MFC_REGISTER_DEBUG
		mfc_fw_debug(R2H_CMD_FRAME_DONE_RET);
#endif

		mfc_err("MFCINST_ERR_DEC_EXE_TIME_OUT\n");
		return MFCINST_ERR_INTR_TIME_OUT;
	} else if ((interrupt_flag != R2H_CMD_FRAME_DONE_RET) && (nReturnErrCode < MFC_WARN_START_NO)) {

#ifdef ENABLE_DEBUG_DEC_EXE_PARSER_ERR
 #if ENABLE_DEBUG_DEC_EXE_PARSER_ERR
		makefile_mfc_dec_err_info(mfc_ctx, dec_arg, nReturnErrCode);
#endif
#endif
		/* Clear start_byte_num in case of error */
		mfc_ctx->shared_mem.start_byte_num = 0x0;
		mfc_write_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));

#if ENABLE_MFC_REGISTER_DEBUG
		mfc_fw_debug(R2H_CMD_FRAME_DONE_RET);
#endif

		mfc_err("MFCINST_ERR_DEC_DONE_FAIL.......(interrupt_flag: %d), (ERR Code: %d)\n", interrupt_flag, nReturnErrCode);
		return MFCINST_ERR_DEC_DECODE_DONE_FAIL;

	} else if (interrupt_flag != R2H_CMD_FRAME_DONE_RET) {
		mfc_warn("MFCINST_WARN_DEC_EXE.........(interrupt_flag: %d), (WARN Code: %d)\n", interrupt_flag, nReturnErrCode);
	}

	mfc_read_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));

	dec_arg->out_res_change = (READL(MFC_SI_DISPLAY_STATUS) >> 4) & 0x3;

	frame_type = READL(MFC_SI_FRAME_TYPE);
	mfc_ctx->FrameType = (enum mfc_frame_type)(frame_type & 0x3);
	
	if (((READL(MFC_SI_DISPLAY_STATUS) & 0x3) != DECODING_DISPLAY) &&
		((READL(MFC_SI_DISPLAY_STATUS) & 0x3) != DISPLAY_ONLY)) {
		dec_arg->out_display_Y_addr = 0;
		dec_arg->out_display_C_addr = 0;
		mfc_debug("DECODING_ONLY frame decoded\n");
	} else if (mfc_ctx->IsPackedPB) {
		if ((mfc_ctx->FrameType == MFC_RET_FRAME_P_FRAME) ||
		    (mfc_ctx->FrameType == MFC_RET_FRAME_I_FRAME)) {
			dec_arg->out_display_Y_addr = READL(MFC_SI_DISPLAY_Y_ADR) << 11;
			dec_arg->out_display_C_addr = READL(MFC_SI_DISPLAY_C_ADR) << 11;
		} else {
   		       dec_arg->out_display_Y_addr = predisplay_Yaddr;
			dec_arg->out_display_C_addr = predisplay_Caddr;
		}
		/* save the display addr */
		predisplay_Yaddr =  READL(MFC_SI_DISPLAY_Y_ADR) << 11;
		predisplay_Caddr = READL(MFC_SI_DISPLAY_C_ADR) << 11;
		mfc_debug("(pre_Y_ADDR : 0x%08x  pre_C_ADDR : 0x%08x)\r\n",
			(predisplay_Yaddr ),
			(predisplay_Caddr ));
	} else {
		/* address shift */
		dec_arg->out_display_Y_addr = READL(MFC_SI_DISPLAY_Y_ADR) << 11;
		dec_arg->out_display_C_addr = READL(MFC_SI_DISPLAY_C_ADR) << 11;
		mfc_debug("DISPLAY Able frame decoded\n");
	}

	if ((READL(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_EMPTY)
		dec_arg->out_display_status = 0;
	else if ((READL(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_DISPLAY)
		dec_arg->out_display_status = 1;
	else if ((READL(MFC_SI_DISPLAY_STATUS) & 0x3) == DISPLAY_ONLY)
		dec_arg->out_display_status = 2;
	else
		dec_arg->out_display_status = 3;

	mfc_debug_L0("(Y_ADDR : 0x%08x  C_ADDR : 0x%08x)\r\n",
			dec_arg->out_display_Y_addr, dec_arg->out_display_C_addr);

	*consumed_strm_size = READL(MFC_SI_DEC_FRM_SIZE);

	return MFCINST_RET_OK;
}

enum mfc_error_code mfc_exe_decode(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	enum mfc_error_code ret_code;
	struct mfc_dec_exe_arg *dec_arg;
	int consumed_strm_size;

	/* 6. Decode Frame */
	mfc_debug_L0("[%d] mfc_exe_decode() start\n", current->pid);

	dec_arg = (struct mfc_dec_exe_arg *)args;

	mfc_ctx->shared_mem.set_frame_tag = dec_arg->in_frametag;

	mfc_write_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));

	ret_code = mfc_decode_one_frame(mfc_ctx, dec_arg, &consumed_strm_size);

	if (ret_code != MFCINST_RET_OK) {
		mfc_debug("mfc_exe_decode() : Decode Fail..(%d)\n", ret_code);
		return ret_code;
	}

	mfc_read_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));
	dec_arg->out_frametag_top = mfc_ctx->shared_mem.get_frame_tag_top;
	dec_arg->out_frametag_bottom = mfc_ctx->shared_mem.get_frame_tag_bot;
	dec_arg->out_timestamp_top = mfc_ctx->shared_mem.pic_time_top;
	dec_arg->out_timestamp_bottom = mfc_ctx->shared_mem.pic_time_bot;
	dec_arg->out_consume_bytes = consumed_strm_size;
	dec_arg->out_crop_top_offset = (mfc_ctx->shared_mem.crop_info2 & 0xffff);
	dec_arg->out_crop_bottom_offset = (mfc_ctx->shared_mem.crop_info2 >> 16);
	dec_arg->out_crop_left_offset = (mfc_ctx->shared_mem.crop_info1 & 0xffff);
	dec_arg->out_crop_right_offset = (mfc_ctx->shared_mem.crop_info1 >> 16);


	/*	PackedPB Stream Processing */
	if ((mfc_ctx->IsPackedPB) &&
		(mfc_ctx->FrameType == MFC_RET_FRAME_P_FRAME) &&
		(dec_arg->in_strm_size - consumed_strm_size > 4)) {

		unsigned char *stream_vir;
		int offset = 0;

		stream_vir = phys_to_virt(dec_arg->in_strm_buf);

		invalidate_kernel_vmap_range((void *)stream_vir, dec_arg->in_strm_size);

		offset = CheckMPEG4StartCode(stream_vir+consumed_strm_size , dec_arg->in_strm_size - consumed_strm_size);
		if (offset > 4)
			consumed_strm_size += offset;
		dec_arg->in_strm_size -= consumed_strm_size;

		mfc_ctx->shared_mem.set_frame_tag = dec_arg->in_frametag;
		mfc_ctx->shared_mem.start_byte_num = consumed_strm_size;
		mfc_write_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));
		ret_code = mfc_decode_one_frame(mfc_ctx, dec_arg, &consumed_strm_size);
		if (ret_code != MFCINST_RET_OK)
			return ret_code;

		mfc_read_shared_mem(mfc_ctx->shared_mem_vaddr, &(mfc_ctx->shared_mem));
		dec_arg->out_frametag_top = mfc_ctx->shared_mem.get_frame_tag_top;
		dec_arg->out_frametag_bottom = mfc_ctx->shared_mem.get_frame_tag_bot;
		dec_arg->out_timestamp_top = mfc_ctx->shared_mem.pic_time_top;
		dec_arg->out_timestamp_bottom = mfc_ctx->shared_mem.pic_time_bot;
		dec_arg->out_consume_bytes += consumed_strm_size;
		dec_arg->out_crop_top_offset = (mfc_ctx->shared_mem.crop_info2 & 0xffff);
		dec_arg->out_crop_bottom_offset = (mfc_ctx->shared_mem.crop_info2 >> 16);
		dec_arg->out_crop_left_offset = (mfc_ctx->shared_mem.crop_info1 & 0xffff);
		dec_arg->out_crop_right_offset = (mfc_ctx->shared_mem.crop_info1 >> 16);

		mfc_ctx->shared_mem.start_byte_num = 0;

	}

	if (mfc_ctx->buf_type == MFC_BUFFER_CACHE) {
		if (((READL(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_DISPLAY) ||
			 ((READL(MFC_SI_DISPLAY_STATUS) & 0x3) == DISPLAY_ONLY)) {
				 unsigned char *out_Y_vir;
				 unsigned char *out_C_vir;
				 unsigned int aligned_width;
				 unsigned int aligned_height;

				 out_Y_vir = phys_to_virt(dec_arg->out_display_Y_addr);
				 aligned_width = ALIGN_TO_128B(mfc_ctx->img_width);
				 aligned_height = ALIGN_TO_32B(mfc_ctx->img_height);
				 dmac_map_area(out_Y_vir, 1, aligned_width*aligned_height);

				 out_C_vir = phys_to_virt(dec_arg->out_display_C_addr);	
				 aligned_height = ALIGN_TO_32B(mfc_ctx->img_height/2);
				 dmac_map_area(out_C_vir, 1, aligned_width*aligned_height);
		}	 
	}	  
		
	mfc_debug_L0("--\n");

	return ret_code;
}

enum mfc_error_code mfc_deinit_hw(struct mfc_inst_ctx *mfc_ctx)
{
	mfc_restore_context(mfc_ctx);

	return MFCINST_RET_OK;
}

enum mfc_error_code mfc_get_config(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_get_config_arg *get_cnf_arg;
	get_cnf_arg = (struct mfc_get_config_arg *)args;

	switch (get_cnf_arg->in_config_param) {
	case MFC_DEC_GETCONF_CRC_DATA:
		if (mfc_ctx->MfcState != MFCINST_STATE_DEC_EXE) {
			mfc_err("MFC_DEC_GETCONF_CRC_DATA : state is invalid\n");
			return MFC_DEC_GETCONF_CRC_DATA;
		}
		get_cnf_arg->out_config_value[0] = READL(MFC_CRC_LUMA0);
		get_cnf_arg->out_config_value[1] = READL(MFC_CRC_CHROMA0);
		break;

	default:
		mfc_err("invalid config param\n");
		return MFCINST_ERR_GET_CONF; /* peter, it should be mod. */
	}

	return MFCINST_RET_OK;
}

enum mfc_error_code mfc_set_config(struct mfc_inst_ctx *mfc_ctx, union mfc_args *args)
{
	struct mfc_set_config_arg *set_cnf_arg;
	set_cnf_arg = (struct mfc_set_config_arg *)args;

	switch (set_cnf_arg->in_config_param) {
	case MFC_DEC_SETCONF_POST_ENABLE:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFC_DEC_SETCONF_POST_ENABLE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0) || (set_cnf_arg->in_config_value[0] == 1)) {
			mfc_ctx->postEnable = set_cnf_arg->in_config_value[0];
		} else {
			mfc_warn("POST_ENABLE should be 0 or 1\n");
			mfc_ctx->postEnable = 0;
		}
		break;

	case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFC_DEC_SETCONF_EXTRA_BUFFER_NUM : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}
		if ((set_cnf_arg->in_config_value[0] >= 0) || (set_cnf_arg->in_config_value[0] <= MFC_MAX_EXTRA_DPB)) {
			mfc_ctx->extraDPB = set_cnf_arg->in_config_value[0];
		} else {
			mfc_warn("EXTRA_BUFFER_NUM should be between 0 and 5...It will be set 5 by default\n");
			mfc_ctx->extraDPB = MFC_MAX_EXTRA_DPB;
		}
		break;

	case MFC_DEC_SETCONF_DISPLAY_DELAY:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFC_DEC_SETCONF_DISPLAY_DELAY : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] >= 0) || (set_cnf_arg->in_config_value[0] < 16)) {
			mfc_ctx->displayDelay = set_cnf_arg->in_config_value[0];
			mfc_debug("DISPLAY_DELAY Number = %d\n", mfc_ctx->displayDelay);
		} else {
			mfc_warn("DISPLAY_DELAY should be between 0 and 16\n");
			mfc_ctx->displayDelay = 0;
		}
		break;

	case MFC_DEC_SETCONF_IS_LAST_FRAME:
		if (mfc_ctx->MfcState != MFCINST_STATE_DEC_EXE) {
			mfc_err("MFC_DEC_SETCONF_IS_LAST_FRAME : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0) || (set_cnf_arg->in_config_value[0] == 1)) {
			mfc_ctx->endOfFrame = set_cnf_arg->in_config_value[0];
		} else {
			mfc_warn("IS_LAST_FRAME should be 0 or 1\n");
			mfc_ctx->endOfFrame = 0;
		}
		break;

	case MFC_DEC_SETCONF_SLICE_ENABLE:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFC_DEC_SETCONF_SLICE_ENABLE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0) || (set_cnf_arg->in_config_value[0] == 1)) {
			mfc_ctx->sliceEnable = set_cnf_arg->in_config_value[0];
		} else {
			mfc_warn("SLICE_ENABLE should be 0 or 1\n");
			mfc_ctx->sliceEnable = 0;
		}
		break;

	case MFC_DEC_SETCONF_CRC_ENABLE:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFC_DEC_SETCONF_CRC_ENABLE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0) || (set_cnf_arg->in_config_value[0] == 1)) {
			mfc_ctx->crcEnable = set_cnf_arg->in_config_value[0];
		} else {
			mfc_warn("CRC_ENABLE should be 0 or 1\n");
			mfc_ctx->crcEnable = 0;
		}
		break;

	case MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		mfc_ctx->widthFIMV1 = set_cnf_arg->in_config_value[0];
		mfc_ctx->heightFIMV1 = set_cnf_arg->in_config_value[1];
		break;

	case MFC_ENC_SETCONF_FRAME_TYPE:
		if (mfc_ctx->MfcState != MFCINST_STATE_ENC_EXE) {
			mfc_err("MFC_ENC_SETCONF_FRAME_TYPE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] >= DONT_CARE) && (set_cnf_arg->in_config_value[0] <= NOT_CODED)) {
			mfc_ctx->forceSetFrameType = set_cnf_arg->in_config_value[0];
		} else {
			mfc_warn("FRAME_TYPE should be between 0 and 2\n");
			mfc_ctx->forceSetFrameType = DONT_CARE;
		}
		break;

	case MFC_ENC_SETCONF_ALLOW_FRAME_SKIP:
		if (mfc_ctx->MfcState >= MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err("MFC_ENC_SETCONF_ALLOW_FRAME_SKIP : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if (set_cnf_arg->in_config_value[0])
			mfc_ctx->shared_mem.ext_enc_control = (mfc_ctx->shared_mem.ext_enc_control | (0x1 << 1));
		break;

	case MFC_ENC_SETCONF_CHANGE_FRAME_RATE:
		if (mfc_ctx->MfcState != MFCINST_STATE_ENC_EXE) {
			mfc_err("MFC_ENC_SETCONF_FRAME_TYPE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}
		mfc_ctx->dynamic_framerate = set_cnf_arg->in_config_value[0];
		break;

	case MFC_ENC_SETCONF_CHANGE_BIT_RATE:
		if (mfc_ctx->MfcState != MFCINST_STATE_ENC_EXE) {
			mfc_err("MFC_ENC_SETCONF_FRAME_TYPE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}
		mfc_ctx->dynamic_bitrate = set_cnf_arg->in_config_value[0];
		break;

	case MFC_ENC_SETCONF_I_PERIOD:
		if (mfc_ctx->MfcState != MFCINST_STATE_ENC_EXE) {
			mfc_err("MFC_ENC_SETCONF_I_PERIOD : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}
		mfc_ctx->dynamic_iperoid = set_cnf_arg->in_config_value[0];
		break;

	default:
		mfc_err("invalid config param\n");
		return MFCINST_ERR_SET_CONF;
	}

	return MFCINST_RET_OK;
}

enum mfc_error_code mfc_set_sleep()
{
	if (mfc_cmd_host2risc(H2R_CMD_SLEEP, 0, 0, 0, 0) == false) {
		mfc_err("R2H_CMD_SLEEP FAIL\n");
		return MFCINST_SLEEP_FAIL;
	}

	if (mfc_wait_for_done(R2H_CMD_SLEEP_RET) != R2H_CMD_SLEEP_RET) {
		mfc_err("R2H_CMD_SLEEP_RET FAIL\n");
		return MFCINST_SLEEP_FAIL;
	}

	return MFCINST_RET_OK;
}

enum mfc_error_code mfc_set_wakeup()
{
	int ret;

	if (mfc_cmd_host2risc(H2R_CMD_WAKEUP, 0, 0, 0, 0) == false) {
		mfc_err("R2H_CMD_WAKEUP FAIL\n");
		return MFCINST_WAKEUP_FAIL;
	}

	WRITEL(0x3ff, MFC_SW_RESET);

	ret = mfc_wait_for_done(R2H_CMD_WAKEUP_RET);
	if ((ret != R2H_CMD_WAKEUP_RET) && (ret != R2H_CMD_FW_STATUS_RET)) {
		mfc_err("R2H_CMD_WAKEUP_RET FAIL\n");
		return MFCINST_WAKEUP_FAIL;
	}

	return MFCINST_RET_OK;
}

static enum
mfc_error_code mfc_alloc_context_buffer(struct mfc_inst_ctx *mfc_ctx, unsigned int mapped_addr,
					unsigned int *context_addr, int *size)
{
	union mfc_args local_param;
	enum mfc_error_code ret_code;
	unsigned char *context_vir;

	switch (mfc_ctx->MfcCodecType) {
	case H264_ENC:
		*size = H264ENC_CONTEXT_SIZE;
		break;

	case MPEG4_ENC:
		*size = MPEG4ENC_CONTEXT_SIZE;
		break;

	case H263_ENC:
		*size = H263ENC_CONTEXT_SIZE;
		break;

	case H264_DEC:
		*size = H264DEC_CONTEXT_SIZE;
		break;

	case H263_DEC:
		*size = H263DEC_CONTEXT_SIZE;
		break;

	case MPEG2_DEC:
		*size = MPEG2DEC_CONTEXT_SIZE;
		break;

	case MPEG4_DEC:
	case FIMV1_DEC:
	case FIMV2_DEC:
	case FIMV3_DEC:
	case FIMV4_DEC:
		*size = MPEG4DEC_CONTEXT_SIZE;
		break;

	case VC1_DEC:
	case VC1RCV_DEC:
		*size = VC1DEC_CONTEXT_SIZE;
		break;

	default:
		return MFCINST_ERR_WRONG_CODEC_MODE;
	}

	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size = *size;
	local_param.mem_alloc.mapped_addr = mapped_addr;

	ret_code = mfc_allocate_buffer(mfc_ctx, &(local_param), 0);
	if (ret_code < 0)
		return ret_code;

	/*	Set mfc context to "0". */
	context_vir = phys_to_virt(local_param.mem_alloc.out_paddr);
	memset(context_vir, 0x0, local_param.mem_alloc.buff_size);

	dmac_flush_range(context_vir, context_vir + local_param.mem_alloc.buff_size);

	*context_addr = local_param.mem_alloc.out_paddr;

	return ret_code;
}

void mfc_init_mem_inst_no(void)
{
	memset(&mfc_mem_inst_no, 0x00, sizeof(mfc_mem_inst_no));
}

int mfc_get_mem_inst_no(void)
{
	unsigned int i;

	for (i = 0; i < MFC_MAX_INSTANCE_NUM; i++) {
		if (mfc_mem_inst_no[i] == 0) {
			mfc_mem_inst_no[i] = 1;
			return i;
		}
	}

	return -1;
}

void mfc_return_mem_inst_no(int inst_no)
{
	if ((inst_no >= 0) && (inst_no < MFC_MAX_INSTANCE_NUM))
		mfc_mem_inst_no[inst_no] = 0;
}

bool mfc_is_running(void)
{
	unsigned int i;
	bool ret = false;

	for (i = 0; i < MFC_MAX_INSTANCE_NUM; i++) {
		mfc_debug("mfc_mem_inst_no[%d] = %d\n", i, mfc_mem_inst_no[i]);
		if (mfc_mem_inst_no[i] == 1)
			ret = true;
	}

	return ret;
}

int mfc_set_state(struct mfc_inst_ctx *ctx, enum mfc_inst_state state)
{
	if (ctx->MfcState > state)
		return -1;

	ctx->MfcState = state;
	return  0;
}

bool is_dec_codec(enum ssbsip_mfc_codec_type codec_type)
{
	switch (codec_type) {
	case H264_DEC:
	case VC1_DEC:
	case MPEG4_DEC:
	case XVID_DEC:
	case MPEG1_DEC:
	case MPEG2_DEC:
	case H263_DEC:
	case VC1RCV_DEC:
	case FIMV1_DEC:
	case FIMV2_DEC:
	case FIMV3_DEC:
	case FIMV4_DEC:
		return true;

	case H264_ENC:
	case MPEG4_ENC:
	case H263_ENC:
		return false;

	default:
		return false;
	}
}





/*
 * Debugging Functions	Definition
 * tile_to_linear_4x2(..)
 * calculate_seq_size(..)
 * printk_mfc_init_info(..)
 */


#if DEBUG_MAKE_RAW
static void write_file(char *filename,  unsigned char *data, unsigned int nSize)
{
	struct file *file;
	loff_t pos = 0;
	int fd;
	mm_segment_t old_fs;

	invalidate_kernel_vmap_range(data, nSize);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fd = sys_open(filename, O_WRONLY|O_CREAT, 0644);
	if (fd >= 0) {
		sys_write(fd, data, nSize);
		file = fget(fd);
		if (file) {
			vfs_write(file, data, nSize, &pos);
			fput(file);
		}
		sys_close(fd);
	} else {
		mfc_err("........Open fail : %d\n", fd);
	}
	set_fs(old_fs);

	dmac_flush_range(data, data + nSize);

}

static int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos)
{
	int pixel_x_m1, pixel_y_m1;
	int roundup_x, roundup_y;
	int linear_addr0, linear_addr1, bank_addr ;
	int x_addr;
	int trans_addr;

	pixel_x_m1 = x_size - 1;
	pixel_y_m1 = y_size - 1;

	roundup_x = ((pixel_x_m1 >> 7) + 1);
	roundup_y = ((pixel_x_m1 >> 6) + 1);

	x_addr = (x_pos >> 2);

	if ((y_size <= y_pos+32) && (y_pos < y_size) &&
		(((pixel_y_m1 >> 5) & 0x1) == 0) && (((y_pos >> 5) & 0x1) == 0)) {
		linear_addr0 = (((y_pos & 0x1f) << 4) | (x_addr & 0xf));
		linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 6) & 0x3f));

		if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
			bank_addr = ((x_addr >> 4) & 0x1);
		else
			bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
	} else {
		linear_addr0 = (((y_pos & 0x1f) << 4) | (x_addr & 0xf));
		linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 5) & 0x7f));

		if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
			bank_addr = ((x_addr >> 4) & 0x1);
		else
			bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
	}

	linear_addr0 = linear_addr0 << 2;
	trans_addr = (linear_addr1 << 13) | (bank_addr << 11) | linear_addr0;

	return trans_addr;
}


static void copy16(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, int mm, int nn)
{
	p_linear_addr[mm] = p_tiled_addr[nn];
	p_linear_addr[mm  + 1] = p_tiled_addr[nn + 1];
	p_linear_addr[mm  + 2] = p_tiled_addr[nn + 2];
	p_linear_addr[mm  + 3] = p_tiled_addr[nn + 3];

	p_linear_addr[mm  + 4] = p_tiled_addr[nn + 4];
	p_linear_addr[mm  + 5] = p_tiled_addr[nn + 5];
	p_linear_addr[mm  + 6] = p_tiled_addr[nn + 6];
	p_linear_addr[mm  + 7] = p_tiled_addr[nn + 7];

	p_linear_addr[mm  + 8] = p_tiled_addr[nn + 8];
	p_linear_addr[mm  + 9] = p_tiled_addr[nn + 9];
	p_linear_addr[mm  + 10] = p_tiled_addr[nn + 10];
	p_linear_addr[mm  + 11] = p_tiled_addr[nn + 11];

	p_linear_addr[mm  + 12] = p_tiled_addr[nn + 12];
	p_linear_addr[mm  + 13]	= p_tiled_addr[nn + 13];
	p_linear_addr[mm  + 14] = p_tiled_addr[nn + 14];
	p_linear_addr[mm  + 15]	= p_tiled_addr[nn + 15];
}


static void
tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr,
		   unsigned int x_size, unsigned int y_size)
{
	int trans_addr;
	unsigned int i, j, k, nn, mm, index;

	/*. TILE 4x2 test */
	for (i = 0; i < y_size; i = i + 16) {
		for (j = 0; j < x_size; j = j + 16) {
			trans_addr = tile_4x2_read(x_size, y_size, j, i);
			index = i*x_size + j;

			k = 0; nn = trans_addr + (k << 6); mm = index;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 1; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 2; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 3; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 4; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 5; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 6; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 7; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 8; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 9; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 10; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 11; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 12; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 13; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 14; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

			k = 15; nn = trans_addr + (k << 6); mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);

		}
	}
}
#endif



#if ENABLE_CHECK_SEQ_HEADER
static int calculate_seq_size(mfc_args *args)
{
	int nn = 0;
	int nCnt = 0;
	unsigned char nSum = 0;
	unsigned char *stream_vir;

	stream_vir = phys_to_virt(args->dec_init.in_strm_buf);
	if (args->dec_init.in_strm_size > 31) {
		for (nn = 0; nn < args->dec_init.in_strm_size - 4; nn++) {
			nSum = (unsigned char)(((*(stream_vir + nn)) << 1) + ((*(stream_vir + nn + 1)) << 1)
						+ ((*(stream_vir + nn + 2)) << 1) + (*(stream_vir+nn+3)));
			if (nSum == 0x1) {
				nCnt++;
			}

			if (nCnt == 3) {
				mfc_info("After Stream Size : %d , nCnt = %d\n", args->dec_init.in_strm_size, nCnt);
				return nn;
			}
		}
	}

	return	args->dec_init.in_strm_size;
}
#endif


#if ENABLE_DEBUG_MFC_INIT
void printk_mfc_init_info(mfc_inst_ctx *mfc_ctx, mfc_args *args)
{
	int nn = 0;
	unsigned char *stream_vir;

	mfc_info("MFC Decoder/Encoder Init Information\n");
	mfc_info("[InstNo : %d],  [DPBCnt : %d],  [totalDPBCnt : %d],  [extraDPB : %d],  [displayDelay : %d],\n",
		 mfc_ctx->InstNo, mfc_ctx->DPBCnt, mfc_ctx->totalDPBCnt, mfc_ctx->extraDPB, mfc_ctx->displayDelay);
	mfc_info("[img_width : %d],  [img_height : %d], [MfcCodecType : %d], [MfcState : %d]\n",
		  mfc_ctx->img_width, mfc_ctx->img_height, mfc_ctx->MfcCodecType, mfc_ctx->MfcState);

	mfc_info("Input Stream Buffer Information\n");
	mfc_info("[in_strm_size : %d],  [in_strm_buf : %d]\n",
		 args->dec_init.in_strm_size, args->dec_init.in_strm_buf);

	stream_vir = phys_to_virt(args->dec_init.in_strm_buf);
	if (args->dec_init.in_strm_size > 0) {
		mfc_info("Input Stream Buffer\n");
		for (nn = 0; nn < 40; nn++)
			printk("%02x ", *(stream_vir+nn));
		printk("\n");
	}

}
#endif


#ifdef ENABLE_DEBUG_DEC_EXE_INTR_ERR
#if ENABLE_DEBUG_DEC_EXE_INTR_ERR
void printk_mfc_dec_exe_info(struct mfc_inst_ctx *mfc_ctx, struct mfc_dec_exe_arg *dec_arg)
{
	int nn = 0;
	unsigned char *stream_vir;

	mfc_info("MFC Decoder/Encoder Exe Information\n");
	mfc_info("[InstNo : %d],  [DPBCnt : %d],  [totalDPBCnt : %d],  [extraDPB : %d],  [displayDelay : %d],\n",
		 mfc_ctx->InstNo, mfc_ctx->DPBCnt, mfc_ctx->totalDPBCnt, mfc_ctx->extraDPB, mfc_ctx->displayDelay);
	mfc_info("[img_width : %d],  [img_height : %d], [MfcCodecType : %d], [MfcState : %d], [FrameType: %d]\n",
		 mfc_ctx->img_width, mfc_ctx->img_height, mfc_ctx->MfcCodecType, mfc_ctx->MfcState, mfc_ctx->FrameType);

	mfc_info("Input Stream Buffer Information\n");
	mfc_info("[in_strm_size : %d],  [in_strm_buf : %d]\n",
		 dec_arg->in_strm_size, dec_arg->in_strm_buf);

	stream_vir = phys_to_virt(dec_arg->in_strm_buf);
	if (dec_arg->in_strm_size > 0) {
		mfc_info("Input Stream Buffer\n");
		for (nn = 0; nn < 50; nn++)
			printk("%02x ", *(stream_vir+nn));
		printk("\n");
	}
}

void
makefile_mfc_dec_err_info(struct mfc_inst_ctx *mfc_ctx,
			  struct mfc_dec_exe_arg *dec_arg, int nReturnErrCode)
{
	char fileName0[50];
	char fileName1[50];
	unsigned char *ctx_virbuf;
	unsigned char *mfc_dec_in_base_vaddr;

	mframe_cnt++;

	if ((nReturnErrCode < 145) || (nReturnErrCode == 300)) {
		mIsDangerError = 1;
		printk_mfc_dec_exe_info(mfc_ctx, dec_arg);
	}

	memset(fileName0, 0, 50);
	memset(fileName1, 0, 50);

	sprintf(fileName0, "/data/dec_in/mfc_decexe_instream_%d_%d.raw", nReturnErrCode, mframe_cnt);
	sprintf(fileName1, "/data/dec_in/mfc_decexe_mfcctx_%d_%d.bin", nReturnErrCode, mframe_cnt);

	mfc_dec_in_base_vaddr = phys_to_virt(dec_arg->in_strm_buf);
	ctx_virbuf = phys_to_virt(mcontext_addr);

	if (mfc_ctx->buf_type == MFC_BUFFER_NO_CACHE)
	write_file(fileName0, mfc_dec_in_base_vaddr, dec_arg->in_strm_size);
	write_file(fileName1, ctx_virbuf, mcontext_size);

}


void makefile_mfc_decinit_err_info(struct mfc_inst_ctx *mfc_ctx, struct mfc_dec_init_arg *decinit_arg, int nReturnErrCode)
{
	char fileName0[50];
	char fileName1[50];
	unsigned char *ctx_virbuf;
	unsigned char *mfc_dec_in_base_vaddr;

	mframe_cnt++;

	pr_info("makefile_mfc_decinit_err_info : in_strm_size(%d)\n", decinit_arg->in_strm_size);

	memset(fileName0, 0, 50);
	memset(fileName1, 0, 50);

	sprintf(fileName0, "/data/dec_in/mfc_decinit_instream_%d_%d.raw", nReturnErrCode, mframe_cnt);
	sprintf(fileName1, "/data/dec_in/mfc_decinit_mfcctx_%d_%d.bin", nReturnErrCode, mframe_cnt);

	mfc_dec_in_base_vaddr = phys_to_virt(decinit_arg->in_strm_buf);
	ctx_virbuf = phys_to_virt(mcontext_addr);

	if (mfc_ctx->buf_type == MFC_BUFFER_NO_CACHE)
	write_file(fileName0, mfc_dec_in_base_vaddr, decinit_arg->in_strm_size);
	write_file(fileName1, ctx_virbuf, mcontext_size);
}
#endif
#endif

#ifdef ENABLE_DEBUG_ENC_EXE_INTR_ERR
#if ENABLE_DEBUG_ENC_EXE_INTR_ERR
void makefile_mfc_enc_err_info(struct mfc_enc_exe_arg *enc_arg)
{
	int nFrameSize = 0;
	char fileName[50];
	unsigned char *mfc_enc_in_base_Y_vaddr;
	unsigned char *mfc_enc_in_base_CbCr_vaddr;

	mframe_cnt++;

	memset(fileName, 0, 50);
	sprintf(fileName, "/data/enc_in/mfc_in_%04d.yuv", mframe_cnt);
	nFrameSize = mImgHight * mImgWidth * 3/2;

	mfc_enc_in_base_Y_vaddr = phys_to_virt(enc_arg->in_Y_addr);
	mfc_enc_in_base_CbCr_vaddr = phys_to_virt(enc_arg->in_CbCr_addr);

	mfc_debug("enc_arg->in_Y_addr : 0x%08x enc_arg->in_Y_addr_vir :0x%08x\r\n",
			enc_arg->in_Y_addr, mfc_enc_in_base_Y_vaddr);

	tile_to_linear_4x2(pResLinearbuf, mfc_enc_in_base_Y_vaddr,
			   mImgWidth, mImgHight);
	tile_to_linear_4x2(pResLinearbuf + (mImgHight * mImgWidth),
			   mfc_enc_in_base_CbCr_vaddr, mImgWidth, mImgHight/2);
	write_file(fileName, pResLinearbuf, nFrameSize);

}
#endif
#endif


static int CheckMPEG4StartCode(unsigned char *src_mem, unsigned int remainSize)
{
	unsigned int index = 0;

	for (index = 0; index < remainSize-3; index++) {
		if ((src_mem[index] == 0x00)
		  && (src_mem[index+1] == 0x00)
		  && (src_mem[index+2] == 0x01))
			return index;
	}

	return -1;
}

#if ENABLE_CHECK_START_CODE
static int CheckDecStartCode(unsigned char *src_mem, unsigned int nstreamSize, enum ssbsip_mfc_codec_type nCodecType)
{
	unsigned int index = 0;
	/* Check Start Code within "isearchSize" bytes. */
	unsigned int isearchSize = 20;
	unsigned int nShift = 0;
	unsigned char nFlag = 0xFF;

	if (nCodecType == H263_DEC) {
		nFlag = 0x08;
		nShift = 4;
	} else if (nCodecType == MPEG4_DEC) {
		nFlag = 0x01;
		nShift = 0;
	} else if (nCodecType == H264_DEC) {
		nFlag = 0x01;
		nShift = 0;
	} else	{
		nFlag = 0xFF;
	}

	if (nFlag != 0xFF) {
		if (nstreamSize > 3) {
			if (nstreamSize > isearchSize) {
				for (index = 0; index < isearchSize-3; index++) {
					if ((src_mem[index] == 0x00)
					    && (src_mem[index+1] == 0x00)
					    && ((src_mem[index+2] >> nShift) == nFlag))
						return index;
				}
			} else {
				for (index = 0; index < nstreamSize - 3; index++) {
					if ((src_mem[index] == 0x00)
					    && (src_mem[index+1] == 0x00)
					    && ((src_mem[index+2] >> nShift) == nFlag))
						return index;
				}
			}
		} else {
			return -1;
		}
	} else {
		return 0;
	}

	return -1;
}
#endif

#if ENABLE_CHECK_NULL_STREAM
static int CheckNullStream(unsigned char *src_mem, unsigned int streamSize)
{
	unsigned int temp = 0;
	unsigned int	nn;

	if (streamSize < 30) {
		for (nn = 0; nn < streamSize; nn++)
			temp += src_mem[nn];
	} else {
		for (nn = 0; nn < 10; nn++)
			temp += src_mem[nn];

		if (temp == 0) {
			for (nn = streamSize-10; nn < streamSize; nn++)
				temp += src_mem[nn];
		}
	}

	if (temp == 0) {
		mfc_debug("Null Stream......Error\n");
		return -1;
	}

	return 0;

}
#endif

#if ENABLE_MFC_REGISTER_DEBUG
void mfc_fw_debug(mfc_wait_done_type command)
{
	mfc_err("=== MFC FW Debug (Cmd: %d)"
		"(Ver: 0x%08x)  ===\n", command, READL(0x58));
	mfc_err("=== (0x64: 0x%08x) (0x68: 0x%08x)"
		"(0xE4: 0x%08x) (0xE8: 0x%08x)\n",
		 READL(0x64), READL(0x68), READL(0xe4), READL(0xe8));
	mfc_err("=== (0xF0: 0x%08x) (0xF4: 0x%08x)"
		"(0xF8: 0x%08x) (0xFC: 0x%08x)\n",
		READL(0xf0), READL(0xf4), READL(0xf8), READL(0xfc));
}
#endif
