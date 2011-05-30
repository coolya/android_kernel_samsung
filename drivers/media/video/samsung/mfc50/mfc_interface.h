/*
 * drivers/media/video/samsung/mfc50/mfc_interface.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code (Key Young, Park)
 *   2009.10.22 - Change codec name VC1AP_DEC -> VC1_DEC (Key Young, Park)
 *   2009.11.04 - get physical address via mfc_allocate_buffer (Key Young, Park)
 *   2009.11.06 - Apply common MFC API (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MFC_INTERFACE_H_
#define _MFC_INTERFACE_H_

#include "mfc_errorno.h"

#define IOCTL_MFC_DEC_INIT			0x00800001
#define IOCTL_MFC_ENC_INIT			0x00800002
#define IOCTL_MFC_DEC_EXE			0x00800003
#define IOCTL_MFC_ENC_EXE			0x00800004

#define IOCTL_MFC_GET_IN_BUF			0x00800010
#define IOCTL_MFC_FREE_BUF			0x00800011
#define IOCTL_MFC_GET_PHYS_ADDR			0x00800012
#define IOCTL_MFC_GET_MMAP_SIZE			0x00800014

#define IOCTL_MFC_SET_CONFIG			0x00800101
#define IOCTL_MFC_GET_CONFIG			0x00800102

#define IOCTL_MFC_BUF_CACHE			0x00801000

/* MFC H/W support maximum 32 extra DPB */
#define MFC_MAX_EXTRA_DPB                      5

#define ENC_PROFILE_LEVEL(profile, level)      ((profile) | ((level) << 8))

#define ENC_PROFILE_MPEG4_SP                   0
#define ENC_PROFILE_MPEG4_ASP                  1
#define ENC_PROFILE_H264_BP                    0
#define ENC_PROFILE_H264_MAIN                  1
#define ENC_PROFILE_H264_HIGH                  2

#define ENC_RC_DISABLE                         0
#define ENC_RC_ENABLE_MACROBLOCK               1
#define ENC_RC_ENABLE_FRAME                    2

#define ENC_RC_QBOUND(min_qp, max_qp)          ((min_qp) | ((max_qp) << 8))
#define ENC_RC_MB_CTRL_DARK_DISABLE            (1 << 3)
#define ENC_RC_MB_CTRL_SMOOTH_DISABLE          (1 << 2)
#define ENC_RC_MB_CTRL_STATIC_DISABLE          (1 << 1)
#define ENC_RC_MB_CTRL_ACTIVITY_DISABLE        (1 << 0)


enum ssbsip_mfc_codec_type {
	H264_DEC,
	VC1_DEC,     /* VC1 advaced Profile decoding  */
	MPEG4_DEC,
	XVID_DEC,
	MPEG1_DEC,
	MPEG2_DEC,
	H263_DEC,
	VC1RCV_DEC,  /* VC1 simple/main profile decoding  */
	FIMV1_DEC,
	FIMV2_DEC,
	FIMV3_DEC,
	FIMV4_DEC,
	H264_ENC,
	MPEG4_ENC,
	H263_ENC,
	UNKNOWN_TYPE
};

enum ssbsip_mfc_force_set_frame_type {
	DONT_CARE = 0,
	I_FRAME = 1,
	NOT_CODED = 2
};

enum  ssbsip_mfc_dec_conf {
	MFC_DEC_SETCONF_POST_ENABLE = 1,
	MFC_DEC_SETCONF_EXTRA_BUFFER_NUM,
	MFC_DEC_SETCONF_DISPLAY_DELAY,
	MFC_DEC_SETCONF_IS_LAST_FRAME,
	MFC_DEC_SETCONF_SLICE_ENABLE,
	MFC_DEC_SETCONF_CRC_ENABLE,
	MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT,
	MFC_DEC_SETCONF_FRAME_TAG,
	MFC_DEC_GETCONF_CRC_DATA,
	MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT,
	FC_DEC_GETCONF_CROP_INFO,
	MFC_DEC_GETCONF_FRAME_TAG
};

enum  ssbsip_mfc_enc_conf {
	MFC_ENC_SETCONF_FRAME_TYPE = 100,
	MFC_ENC_SETCONF_CHANGE_FRAME_RATE,
	MFC_ENC_SETCONF_CHANGE_BIT_RATE,
	MFC_ENC_SETCONF_FRAME_TAG,
	MFC_ENC_SETCONF_ALLOW_FRAME_SKIP,
	MFC_ENC_GETCONF_FRAME_TAG,
	MFC_ENC_SETCONF_I_PERIOD
};

struct mfc_strm_ref_buf_arg {
	unsigned int strm_ref_y;
	unsigned int mv_ref_yc;
};

struct mfc_frame_buf_arg {
	unsigned int luma;
	unsigned int chroma;
};

struct mfc_enc_init_mpeg4_arg {
	enum ssbsip_mfc_codec_type in_codec_type; /* [IN] codec type                                   */
	int in_width;                        /* [IN] width of YUV420 frame to be encoded          */
	int in_height;                       /* [IN] height of YUV420 frame to be encoded         */
	int in_profile_level;                /* [IN] profile & level                              */
	int in_gop_num;                      /* [IN] GOP Number (interval of I-frame)             */
	int in_frame_qp;                     /* [IN] the quantization parameter of the frame      */
	int in_frame_P_qp;                   /* [IN] the quantization parameter of the P frame    */
	int in_frame_B_qp;                   /* [IN] the quantization parameter of the B frame    */

	int in_RC_frm_enable;                /* [IN] RC enable (0:disable, 1:frame level RC)      */
	int in_RC_framerate;                 /* [IN] RC parameter (framerate)                     */
	int in_RC_bitrate;                   /* [IN] RC parameter (bitrate in kbps)               */
	int in_RC_qbound;                    /* [IN] RC parameter (Q bound)                       */
	int in_RC_rpara;                     /* [IN] RC parameter (Reaction Coefficient)          */

	int in_MS_mode;                      /* [IN] Multi-slice mode (0:single, 1:multiple)      */
	int in_MS_size;                      /* [IN] Multi-slice size (in num. of mb or byte)     */
	int in_mb_refresh;                   /* [IN] Macroblock refresh                           */
	int in_interlace_mode;               /* [IN] interlace mode(0:progressive, 1:interlace)   */
	int in_BframeNum;                    /* [IN] B frame number                               */

	int in_pad_ctrl_on;                  /* [IN] Enable (1) / Disable (0) padding             */
	int in_luma_pad_val;                 /* [IN] pad value if pad_ctrl_on is Enable           */
	int in_cb_pad_val;
	int in_cr_pad_val;

	int in_frame_map;				/* [IN] Encoding input NV12 type ( 0 : tiled , 1: linear)   */
    
	unsigned int in_mapped_addr;
	struct mfc_strm_ref_buf_arg out_u_addr;
	struct mfc_strm_ref_buf_arg out_p_addr;
	struct mfc_strm_ref_buf_arg out_buf_size;
	unsigned int out_header_size;

	/* MPEG4 Only */
	int in_qpelME_enable;                /* [IN] Quarter-pel MC enable(1:enable, 0:disable)      */
	int in_time_increament_res;          /* [IN] time increment resolution                       */
	int in_time_vop_time_increament;     /* [IN] time increment                                  */
};

struct mfc_enc_init_h264_arg {
	enum ssbsip_mfc_codec_type in_codec_type; /* [IN] codec type                                       */
	int in_width;                        /* [IN] width  of YUV420 frame to be encoded             */
	int in_height;                       /* [IN] height of YUV420 frame to be encoded             */
	int in_profile_level;                /* [IN] profile & level                                  */
	int in_gop_num;                      /* [IN] GOP Number (interval of I-frame)                 */
	int in_frame_qp;                     /* [IN] the quantization parameter of the frame          */
	int in_frame_P_qp;                   /* [IN] the quantization parameter of the P frame        */
	int in_frame_B_qp;                   /* [IN] the quantization parameter of the B frame        */

	int in_RC_frm_enable;                /* [IN] RC enable (0:disable, 1:frame level RC)          */
	int in_RC_framerate;                 /* [IN]  RC parameter (framerate)                        */
	int in_RC_bitrate;                   /* [IN]  RC parameter (bitrate in kbps)                  */
	int in_RC_qbound;                    /* [IN]  RC parameter (Q bound)                          */
	int in_RC_rpara;                     /* [IN]  RC parameter (Reaction Coefficient)             */

	int in_MS_mode;                      /* [IN] Multi-slice mode (0:single, 1:multiple)          */
	int in_MS_size;                      /* [IN] Multi-slice size (in num. of mb or byte)         */
	int in_mb_refresh;                   /* [IN] Macroblock refresh                               */
	int in_interlace_mode;               /* [IN] interlace mode(0:progressive, 1:interlace)       */
	int in_BframeNum;

	int in_pad_ctrl_on;                  /* [IN] Enable padding control                           */
	int in_luma_pad_val;                 /* [IN] Luma pel value used to fill padding area         */
	int in_cb_pad_val;                   /* [IN] CB pel value used to fill padding area           */
	int in_cr_pad_val;                   /* [IN] CR pel value used to fill padding area           */

	int in_frame_map;				/* [IN] Encoding input NV12 type ( 0 : tiled , 1: linear)   */
	
	unsigned int in_mapped_addr;
	struct mfc_strm_ref_buf_arg out_u_addr;
	struct mfc_strm_ref_buf_arg out_p_addr;
	struct mfc_strm_ref_buf_arg out_buf_size;
	unsigned int out_header_size;

	/* H264 Only */
	int in_RC_mb_enable;                 /* [IN] RC enable (0:disable, 1:MB level RC)                    */
	int in_reference_num;                /* [IN] The number of reference pictures used                   */
	int in_ref_num_p;                    /* [IN] The number of reference pictures used for P pictures    */
	int in_RC_mb_dark_disable;           /* [IN] Disable adaptive rate control on dark region            */
	int in_RC_mb_smooth_disable;         /* [IN] Disable adaptive rate control on smooth region          */
	int in_RC_mb_static_disable;         /* [IN] Disable adaptive rate control on static region          */
	int in_RC_mb_activity_disable;       /* [IN] Disable adaptive rate control on static region          */
	int in_deblock_filt;                 /* [IN] disable the loop filter                                 */
	int in_deblock_alpha_C0;             /* [IN] Alpha & C0 offset for H.264 loop filter                 */
	int in_deblock_beta;                 /* [IN] Beta offset for H.264 loop filter                       */
	int in_symbolmode;                   /* [IN] The mode of entropy coding(CABAC, CAVLC)                */
	int in_transform8x8_mode;            /* [IN] Allow 8x8 transform(only for high profile)              */
	int in_md_interweight_pps;           /* [IN] Inter weighted parameter for mode decision              */
	int in_md_intraweight_pps;           /* [IN] Intra weighted parameter for mode decision              */
};

struct mfc_enc_exe_arg {
	enum ssbsip_mfc_codec_type in_codec_type; /* [IN]  codec type                                             */
	unsigned int in_Y_addr;              /* [IN]  In-buffer addr of Y component                          */
	unsigned int in_CbCr_addr;           /* [IN]  In-buffer addr of CbCr component                       */
	unsigned int in_Y_addr_vir;          /* [IN]  In-buffer addr of Y component                          */
	unsigned int in_CbCr_addr_vir;       /* [IN]  In-buffer addr of CbCr component                       */
	unsigned int in_strm_st;             /* [IN]  Out-buffer start addr of encoded strm                  */
	unsigned int in_strm_end;            /* [IN]  Out-buffer end addr of encoded strm                    */
	int in_frametag;                     /* [IN]  unique frame ID                                        */

	unsigned int out_frame_type;         /* [OUT] frame type                                             */
	int out_encoded_size;                /* [OUT] Length of Encoded video stream                         */
	unsigned int out_encoded_Y_paddr;    /* [OUT] physical Y address which is flushed                    */
	unsigned int out_encoded_C_paddr;    /* [OUT] physical C address which is flushed                    */
	int out_frametag_top;                /* [OUT] unique frame ID of an output frame or top field        */
	int out_frametag_bottom;             /* [OUT] unique frame ID of bottom field                        */
};

struct mfc_dec_init_arg {
	enum ssbsip_mfc_codec_type in_codec_type; /* [IN]  codec type                                             */
	unsigned int in_strm_buf;            /* [IN]  the physical address of STRM_BUF                       */
	int in_strm_size;                    /* [IN]  size of video stream filled in STRM_BUF                */
	int in_packed_PB;                    /* [IN]  Is packed PB frame or not, 1: packedPB  0: unpacked    */

	int out_img_width;                   /* [OUT] width  of YUV420 frame                                 */
	int out_img_height;                  /* [OUT] height of YUV420 frame                                 */
	int out_buf_width;                   /* [OUT] width  of YUV420 frame                                 */
	int out_buf_height;                  /* [OUT] height of YUV420 frame                                 */
	int out_dpb_cnt;                     /* [OUT] the number of buffers which is nessary during decoding */

	int out_crop_top_offset;             /* [OUT] crop information, top offset                           */
	int out_crop_bottom_offset;          /* [OUT] crop information, bottom offset                        */
	int out_crop_left_offset;            /* [OUT] crop information, left offset                          */
	int out_crop_right_offset;           /* [OUT] crop information, right offset                         */

	struct mfc_frame_buf_arg in_frm_buf;      /* [IN] the address of dpb FRAME_BUF                            */
	struct mfc_frame_buf_arg in_frm_size;     /* [IN] size of dpb FRAME_BUF                                   */
	unsigned int in_mapped_addr;

	struct mfc_frame_buf_arg out_u_addr;
	struct mfc_frame_buf_arg out_p_addr;
	struct mfc_frame_buf_arg out_frame_buf_size;
};

struct mfc_dec_exe_arg {
	enum ssbsip_mfc_codec_type in_codec_type; /* [IN]  codec type                                             */
	unsigned int in_strm_buf;            /* [IN]  the physical address of STRM_BUF                       */
	int in_strm_size;                    /* [IN]  Size of video stream filled in STRM_BUF                */
	struct mfc_frame_buf_arg in_frm_buf;      /* [IN]  the address of dpb FRAME_BUF                           */
	struct mfc_frame_buf_arg in_frm_size;     /* [IN]  size of dpb FRAME_BUF                                  */
	int in_frametag;                     /* [IN]  unique frame ID                                        */

	unsigned int out_display_Y_addr;     /* [OUT] the physical address of display buf                    */
	unsigned int out_display_C_addr;     /* [OUT] the physical address of display buf                    */
	int out_display_status;              /* [OUT] whether display frame exist or not.                    */
	int out_timestamp_top;               /* [OUT] presentation time of an output frame or top field      */
	int out_timestamp_bottom;            /* [OUT] presentation time of bottom field                      */
	int out_consume_bytes;               /* [OUT] consumed bytes when decoding finished                  */
	int out_frametag_top;                /* [OUT] unique frame ID of an output frame or top field        */
	int out_frametag_bottom;             /* [OUT] unique frame ID of bottom field                        */
	int out_res_change;                  /* [OUT] whether resolution is changed or not (0, 1, 2)         */
	int out_crop_top_offset;             /* [OUT] crop information, top offset                           */
	int out_crop_bottom_offset;          /* [OUT] crop information, bottom offset                        */
	int out_crop_left_offset;            /* [OUT] crop information, left offset                          */
	int out_crop_right_offset;           /* [OUT] crop information, right offset                         */
};

struct mfc_get_config_arg {
	int in_config_param;                 /* [IN]  Configurable parameter type                            */
	int out_config_value[4];             /* [IN]  Values to get for the configurable parameter.          */
};

struct mfc_set_config_arg {
	int in_config_param;                 /* [IN]  Configurable parameter type                            */
	int in_config_value[2];              /* [IN]  Values to be set for the configurable parameter.       */
	int out_config_value_old[2];         /* [OUT] Old values of the configurable parameters              */
};

struct mfc_get_phys_addr_arg {
	unsigned int u_addr;
	unsigned int p_addr;
};

struct mfc_mem_alloc_arg {
	enum ssbsip_mfc_codec_type codec_type;
	int buff_size;
	unsigned int mapped_addr;
	unsigned int out_uaddr;
	unsigned int out_paddr;
};

struct mfc_mem_free_arg {
	unsigned int u_addr;
};

typedef enum {
	MFC_BUFFER_NO_CACHE = 0,
	MFC_BUFFER_CACHE = 1
} mfc_buffer_type;	

union mfc_args {
	struct mfc_enc_init_mpeg4_arg enc_init_mpeg4;
	struct mfc_enc_init_mpeg4_arg enc_init_h263;
	struct mfc_enc_init_h264_arg enc_init_h264;
	struct mfc_enc_exe_arg enc_exe;

	struct mfc_dec_init_arg dec_init;
	struct mfc_dec_exe_arg dec_exe;

	struct mfc_get_config_arg get_config;
	struct mfc_set_config_arg set_config;

	struct mfc_mem_alloc_arg mem_alloc;
	struct mfc_mem_free_arg mem_free;
	struct mfc_get_phys_addr_arg get_phys_addr;

	mfc_buffer_type buf_type;
};

struct mfc_common_args {
	enum mfc_error_code ret_code; /* [OUT] error code */
	union mfc_args args;
};

#endif /* _MFC_INTERFACE_H_ */
