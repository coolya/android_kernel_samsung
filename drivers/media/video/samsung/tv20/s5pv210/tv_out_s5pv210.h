/* linux/drivers/media/video/samsung/tv20/s5pv210/tv_out_s5pv210.h
 *
 * tv out header file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/mm.h>
/* #define COFIG_TVOUT_RAW_DBG */


#define HDMI_START_NUM 0x1000

#define bit_add_l(val, addr)	writel(readl(addr) | val, addr)
#define bit_add_s(val, addr)	writes(reads(addr) | val, addr)
#define bit_add_b(val, addr)	writeb(readb(addr) | val, addr)
#define bit_del_l(val, addr)	writel(readl(addr) & ~val, addr)
#define bit_del_s(val, addr)	writes(reads(addr) & ~val, addr)
#define bit_del_b(val, addr)	writeb(readb(addr) & ~val, addr)


enum s5p_tv_audio_codec_type {
	PCM = 1, AC3, MP3, WMA
};

enum s5p_endian_type {
	TVOUT_LITTLE_ENDIAN_MODE = 0,
	TVOUT_BIG_ENDIAN_MODE = 1
};

enum s5p_tv_disp_mode {
	TVOUT_NTSC_M = 0,
	TVOUT_PAL_BDGHI,
	TVOUT_PAL_M,
	TVOUT_PAL_N,
	TVOUT_PAL_NC,
	TVOUT_PAL_60,
	TVOUT_NTSC_443,

	TVOUT_480P_60_16_9 = HDMI_START_NUM,
	TVOUT_480P_60_4_3,
	TVOUT_480P_59,

	TVOUT_576P_50_16_9,
	TVOUT_576P_50_4_3,

	TVOUT_720P_60,
	TVOUT_720P_50,
	TVOUT_720P_59,

	TVOUT_1080P_60,
	TVOUT_1080P_50,
	TVOUT_1080P_59,
	TVOUT_1080P_30,

	TVOUT_1080I_60,
	TVOUT_1080I_50,
	TVOUT_1080I_59,
};

enum s5p_tv_o_mode {
	TVOUT_OUTPUT_COMPOSITE,
	TVOUT_OUTPUT_SVIDEO,
	TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED,
	TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE,
	TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE,
	TVOUT_OUTPUT_HDMI,
	TVOUT_OUTPUT_HDMI_RGB,
	TVOUT_OUTPUT_DVI
};

enum s5p_tv_pwr_err {
	S5P_TV_PWR_ERR_NO_ERROR = 0,
	S5P_TV_PWR_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x5000,
	S5P_TV_PWR_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_PWR_ERR_INVALID_PARAM
};

enum s5p_tv_clk_err {
	S5P_TV_CLK_ERR_NO_ERROR = 0,
	S5P_TV_CLK_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x4000,
	S5P_TV_CLK_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_CLK_ERR_INVALID_PARAM
};

enum s5p_tv_vp_err {
	VPROC_NO_ERROR = 0,
	S5P_TV_VP_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x2000,
	S5P_TV_VP_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_VP_ERR_BASE_ADDRESS_MUST_DOUBLE_WORD_ALIGN,
	S5P_TV_VP_ERR_NOT_UPDATE_FOR_ANOTHER_UPDATE,
	S5P_TV_VP_ERR_INVALID_PARAM
};

enum s5p_tv_vmx_err {
	VMIXER_NO_ERROR = 0,
	S5P_TV_VMX_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x1000,
	S5P_TV_VMX_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_VMX_ERR_BASE_ADDRESS_MUST_WORD_ALIGN,
	S5P_TV_VMX_ERR_INVALID_PARAM
};

enum s5p_tv_vmx_color_fmt {
	VM_DIRECT_RGB565  = 4,
	VM_DIRECT_RGB1555 = 5,
	VM_DIRECT_RGB4444 = 6,
	VM_DIRECT_RGB8888 = 7
};

enum s5p_tv_sd_err {
	SDOUT_NO_ERROR = 0,
	S5P_TV_SD_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x3000,
	S5P_TV_SD_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_SD_ERR_INVALID_PARAM
};

enum s5p_sd_order {
	S5P_TV_SD_O_ORDER_COMPONENT_RGB_PRYPB,
	S5P_TV_SD_O_ORDER_COMPONENT_RBG_PRPBY,
	S5P_TV_SD_O_ORDER_COMPONENT_BGR_PBYPR,
	S5P_TV_SD_O_ORDER_COMPONENT_BRG_PBPRY,
	S5P_TV_SD_O_ORDER_COMPONENT_GRB_YPRPB,
	S5P_TV_SD_O_ORDER_COMPONENT_GBR_YPBPR,
	S5P_TV_SD_O_ORDER_COMPOSITE_CVBS_Y_C,
	S5P_TV_SD_O_ORDER_COMPOSITE_CVBS_C_Y,
	S5P_TV_SD_O_ORDER_COMPOSITE_Y_C_CVBS,
	S5P_TV_SD_O_ORDER_COMPOSITE_Y_CVBS_C,
	S5P_TV_SD_O_ORDER_COMPOSITE_C_CVBS_Y,
	S5P_TV_SD_O_ORDER_COMPOSITE_C_Y_CVBS
};

enum s5p_tv_hdmi_err {
	HDMI_NO_ERROR = 0,
	S5P_TV_HDMI_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x6000,
	S5P_TV_HDMI_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_HDMI_ERR_INVALID_PARAM
};

enum s5p_hdmi_transmit {
	HDMI_DO_NOT_TANS = 0,
	HDMI_TRANS_ONCE,
	HDMI_TRANS_EVERY_SYNC
};

enum s5p_hdmi_audio_type {
	HDMI_AUDIO_NO,
	HDMI_AUDIO_PCM
};


enum s5p_tv_stda_err {
	STDA_NO_ERROR = 0,
	S5P_TV_STDA_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x7000,
	S5P_TV_STDA_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_STDA_ERR_INVALID_PARAM
};


/*
* enum
*/

enum s5p_tv_active_polarity {
	TVOUT_POL_ACTIVE_LOW,
	TVOUT_POL_ACTIVE_HIGH
};

enum s5p_yuv_fmt_component {
	TVOUT_YUV_Y,
	TVOUT_YUV_CB,
	TVOUT_YUV_CR
};

enum s5p_tv_clk_hpll_ref {
	S5P_TV_CLK_HPLL_REF_27M,
	S5P_TV_CLK_HPLL_REF_SRCLK
};

enum s5p_tv_clk_mout_hpll {
	S5P_TV_CLK_MOUT_HPLL_27M,
	S5P_TV_CLK_MOUT_HPLL_FOUT_HPLL
};

enum s5p_tv_clk_vmiexr_srcclk {
	TVOUT_CLK_VMIXER_SRCCLK_CLK27M,
	TVOUT_CLK_VMIXER_SRCCLK_VCLK_54,
	TVOUT_CLK_VMIXER_SRCCLK_MOUT_HPLL
};

enum s5p_vp_src_color {
	VPROC_SRC_COLOR_NV12	= 0,
	VPROC_SRC_COLOR_NV12IW  = 1,
	VPROC_SRC_COLOR_TILE_NV12	= 2,
	VPROC_SRC_COLOR_TILE_NV12IW	= 3
};

enum s5p_vp_pxl_rate {
	VPROC_PIXEL_PER_RATE_1_1  = 0,
	VPROC_PIXEL_PER_RATE_1_2  = 1,
	VPROC_PIXEL_PER_RATE_1_3  = 2,
	VPROC_PIXEL_PER_RATE_1_4  = 3
};

enum s5p_vp_sharpness_control {
	VPROC_SHARPNESS_NO     = 0,
	VPROC_SHARPNESS_MIN    = 1,
	VPROC_SHARPNESS_MOD    = 2,
	VPROC_SHARPNESS_MAX    = 3
};

enum s5p_vp_line_eq {
	VProc_LINE_EQ_0  = 0,
	VProc_LINE_EQ_1  = 1,
	VProc_LINE_EQ_2  = 2,
	VProc_LINE_EQ_3  = 3,
	VProc_LINE_EQ_4  = 4,
	VProc_LINE_EQ_5  = 5,
	VProc_LINE_EQ_6  = 6,
	VProc_LINE_EQ_7  = 7
};

enum s5p_vp_mem_mode {
	VPROC_LINEAR_MODE,
	VPROC_2D_TILE_MODE
};

enum s5p_vp_chroma_expansion {
	VPROC_USING_C_TOP,
	VPROC_USING_C_TOP_BOTTOM
};

enum s5p_vp_filed_id_toggle {
	S5P_TV_VP_FILED_ID_TOGGLE_USER,
	S5P_TV_VP_FILED_ID_TOGGLE_VSYNC
};

enum s5p_vp_field {
	VPROC_TOP_FIELD,
	VPROC_BOTTOM_FIELD
};

enum s5p_vp_poly_coeff {
	VPROC_POLY8_Y0_LL = 0,
	VPROC_POLY8_Y0_LH,
	VPROC_POLY8_Y0_HL,
	VPROC_POLY8_Y0_HH,
	VPROC_POLY8_Y1_LL,
	VPROC_POLY8_Y1_LH,
	VPROC_POLY8_Y1_HL,
	VPROC_POLY8_Y1_HH,
	VPROC_POLY8_Y2_LL,
	VPROC_POLY8_Y2_LH,
	VPROC_POLY8_Y2_HL,
	VPROC_POLY8_Y2_HH,
	VPROC_POLY8_Y3_LL,
	VPROC_POLY8_Y3_LH,
	VPROC_POLY8_Y3_HL,
	VPROC_POLY8_Y3_HH,
	VPROC_POLY4_Y0_LL = 32,
	VPROC_POLY4_Y0_LH,
	VPROC_POLY4_Y0_HL,
	VPROC_POLY4_Y0_HH,
	VPROC_POLY4_Y1_LL,
	VPROC_POLY4_Y1_LH,
	VPROC_POLY4_Y1_HL,
	VPROC_POLY4_Y1_HH,
	VPROC_POLY4_Y2_LL,
	VPROC_POLY4_Y2_LH,
	VPROC_POLY4_Y2_HL,
	VPROC_POLY4_Y2_HH,
	VPROC_POLY4_Y3_LL,
	VPROC_POLY4_Y3_LH,
	VPROC_POLY4_Y3_HL,
	VPROC_POLY4_Y3_HH,
	VPROC_POLY4_C0_LL,
	VPROC_POLY4_C0_LH,
	VPROC_POLY4_C0_HL,
	VPROC_POLY4_C0_HH,
	VPROC_POLY4_C1_LL,
	VPROC_POLY4_C1_LH,
	VPROC_POLY4_C1_HL,
	VPROC_POLY4_C1_HH
};

enum s5p_vp_csc_coeff {
	VPROC_CSC_Y2Y_COEF = 0,
	VPROC_CSC_CB2Y_COEF,
	VPROC_CSC_CR2Y_COEF,
	VPROC_CSC_Y2CB_COEF,
	VPROC_CSC_CB2CB_COEF,
	VPROC_CSC_CR2CB_COEF,
	VPROC_CSC_Y2CR_COEF,
	VPROC_CSC_CB2CR_COEF,
	VPROC_CSC_CR2CR_COEF
};

enum s5p_vp_csc_type {
	VPROC_CSC_SD_HD,
	VPROC_CSC_HD_SD
};

enum s5p_tv_vp_filter_h_pp {
	/* Don't change the order and the value */
	VPROC_PP_H_NORMAL = 0,
	VPROC_PP_H_8_9,      /* 720 to 640 */
	VPROC_PP_H_1_2,
	VPROC_PP_H_1_3,
	VPROC_PP_H_1_4
};

enum s5p_tv_vp_filter_v_pp {
	/* Don't change the order and the value */
	VPROC_PP_V_NORMAL = 0,
	VPROC_PP_V_5_6,      /* PAL to NTSC */
	VPROC_PP_V_3_4,
	VPROC_PP_V_1_2,
	VPROC_PP_V_1_3,
	VPROC_PP_V_1_4
};

enum s5p_vmx_burst_mode {
	VM_BURST_8 = 0,
	VM_BURST_16 = 1
};

enum s5p_tv_vmx_scan_mode {
	VMIXER_INTERLACED_MODE = 0,
	VMIXER_PROGRESSIVE_MODE = 1
};

enum s5p_tv_vmx_layer {
	VM_VIDEO_LAYER = 2,
	VM_GPR0_LAYER = 0,
	VM_GPR1_LAYER = 1
};

enum s5p_tv_vmx_bg_color_num {
	VMIXER_BG_COLOR_0 = 0,
	VMIXER_BG_COLOR_1 = 1,
	VMIXER_BG_COLOR_2 = 2
};

enum s5p_tv_coef_y_mode {
	VMIXER_COEF_Y_NARROW = 0,
	VMIXER_COEF_Y_WIDE = 1
};

enum s5p_tv_vmx_csc_type {
	VMIXER_CSC_RGB_TO_YUV601_LR,
	VMIXER_CSC_RGB_TO_YUV601_FR,
	VMIXER_CSC_RGB_TO_YUV709_LR,
	VMIXER_CSC_RGB_TO_YUV709_FR
};

enum s5p_tv_vmx_rgb {
	RGB601_0_255,
	RGB601_16_235,
	RGB709_0_255,
	RGB709_16_235
};

enum s5p_tv_vmx_out_type {
	MX_YUV444,
	MX_RGB888
};

enum s5p_sd_level {
	S5P_TV_SD_LEVEL_0IRE,
	S5P_TV_SD_LEVEL_75IRE
};

enum s5p_sd_vsync_ratio {
	SDOUT_VTOS_RATIO_10_4,
	SDOUT_VTOS_RATIO_7_3
};

enum s5p_sd_sync_sig_pin {
	SDOUT_SYNC_SIG_NO,
	SDOUT_SYNC_SIG_YG,
	SDOUT_SYNC_SIG_ALL
};

enum s5p_sd_closed_caption_type {
	SDOUT_NO_INS,
	SDOUT_INS_1,
	SDOUT_INS_2,
	SDOUT_INS_OTHERS
};

enum s5p_sd_channel_sel {
	SDOUT_CHANNEL_0 = 0,
	SDOUT_CHANNEL_1 = 1,
	SDOUT_CHANNEL_2 = 2
};

enum s5p_sd_vesa_rgb_sync_type {
	SDOUT_VESA_RGB_SYNC_COMPOSITE,
	SDOUT_VESA_RGB_SYNC_SEPARATE
};

enum s5p_sd_525_copy_permit {
	SDO_525_COPY_PERMIT,
	SDO_525_ONECOPY_PERMIT,
	SDO_525_NOCOPY_PERMIT
};

enum s5p_sd_525_mv_psp {
	SDO_525_MV_PSP_OFF,
	SDO_525_MV_PSP_ON_2LINE_BURST,
	SDO_525_MV_PSP_ON_BURST_OFF,
	SDO_525_MV_PSP_ON_4LINE_BURST,
};

enum s5p_sd_525_copy_info {
	SDO_525_COPY_INFO,
	SDO_525_DEFAULT,
};

enum s5p_sd_525_aspect_ratio {
	SDO_525_4_3_NORMAL,
	SDO_525_16_9_ANAMORPIC,
	SDO_525_4_3_LETTERBOX
};

enum s5p_sd_625_subtitles {
	SDO_625_NO_OPEN_SUBTITLES,
	SDO_625_INACT_OPEN_SUBTITLES,
	SDO_625_OUTACT_OPEN_SUBTITLES
};

enum s5p_sd_625_camera_film {
	SDO_625_CAMERA,
	SDO_625_FILM
};

enum s5p_sd_625_color_encoding {
	SDO_625_NORMAL_PAL,
	SDO_625_MOTION_ADAPTIVE_COLORPLUS
};

enum s5p_sd_625_aspect_ratio {
	SDO_625_4_3_FULL_576,
	SDO_625_14_9_LETTERBOX_CENTER_504,
	SDO_625_14_9_LETTERBOX_TOP_504,
	SDO_625_16_9_LETTERBOX_CENTER_430,
	SDO_625_16_9_LETTERBOX_TOP_430,
	SDO_625_16_9_LETTERBOX_CENTER,
	SDO_625_14_9_FULL_CENTER_576,
	SDO_625_16_9_ANAMORPIC_576
};

enum s5p_tv_hdmi_csc_type {
	HDMI_CSC_YUV601_TO_RGB_LR,
	HDMI_CSC_YUV601_TO_RGB_FR,
	HDMI_CSC_YUV709_TO_RGB_LR,
	HDMI_CSC_YUV709_TO_RGB_FR,
	HDMI_CSC_YUV601_TO_YUV709,
	HDMI_CSC_RGB_FR_TO_RGB_LR,
	HDMI_CSC_RGB_FR_TO_YUV601,
	HDMI_CSC_RGB_FR_TO_YUV709,
	HDMI_BYPASS
};

/*
 * Color Depth for HDMI HW (settings and GCP packet),
 * EDID and PHY HW
 */
enum s5p_hdmi_color_depth {
	HDMI_CD_48,
	HDMI_CD_36,
	HDMI_CD_30,
	HDMI_CD_24
};

enum phy_freq {
	ePHY_FREQ_25_200,
	ePHY_FREQ_25_175,
	ePHY_FREQ_27,
	ePHY_FREQ_27_027,
	ePHY_FREQ_54,
	ePHY_FREQ_54_054,
	ePHY_FREQ_74_250,
	ePHY_FREQ_74_176,
	ePHY_FREQ_148_500,
	ePHY_FREQ_148_352,
	ePHY_FREQ_108_108,
	ePHY_FREQ_72,
	ePHY_FREQ_25,
	ePHY_FREQ_65,
	ePHY_FREQ_108,
	ePHY_FREQ_162
};

/* video format for HDMI HW (timings and AVI) and EDID */
enum s5p_hdmi_v_fmt {
	v640x480p_60Hz = 0,
	v720x480p_60Hz,
	v1280x720p_60Hz,
	v1920x1080i_60Hz,
	v720x480i_60Hz,
	v720x240p_60Hz,
	v2880x480i_60Hz,
	v2880x240p_60Hz,
	v1440x480p_60Hz,
	v1920x1080p_60Hz,
	v720x576p_50Hz,
	v1280x720p_50Hz,
	v1920x1080i_50Hz,
	v720x576i_50Hz,
	v720x288p_50Hz,
	v2880x576i_50Hz,
	v2880x288p_50Hz,
	v1440x576p_50Hz,
	v1920x1080p_50Hz,
	v1920x1080p_24Hz,
	v1920x1080p_25Hz,
	v1920x1080p_30Hz,
	v2880x480p_60Hz,
	v2880x576p_50Hz,
	v1920x1080i_50Hz_1250,
	v1920x1080i_100Hz,
	v1280x720p_100Hz,
	v720x576p_100Hz,
	v720x576i_100Hz,
	v1920x1080i_120Hz,
	v1280x720p_120Hz,
	v720x480p_120Hz,
	v720x480i_120Hz,
	v720x576p_200Hz,
	v720x576i_200Hz,
	v720x480p_240Hz,
	v720x480i_240Hz,
	v720x480p_59Hz,
	v1280x720p_59Hz,
	v1920x1080i_59Hz,
	v1920x1080p_59Hz,

};


enum s5p_tv_hdmi_disp_mode {
	S5P_TV_HDMI_DISP_MODE_480P_60 = 0,
	S5P_TV_HDMI_DISP_MODE_576P_50 = 1,
	S5P_TV_HDMI_DISP_MODE_720P_60 = 2,
	S5P_TV_HDMI_DISP_MODE_720P_50 = 3,
	S5P_TV_HDMI_DISP_MODE_1080I_60 = 4,
	S5P_TV_HDMI_DISP_MODE_1080I_50 = 5,
	S5P_TV_HDMI_DISP_MODE_VGA_60 = 6,
	S5P_TV_HDMI_DISP_MODE_1080P_60 = 7,
	S5P_TV_HDMI_DISP_MODE_1080P_50 = 8,
	S5P_TV_HDMI_DISP_MODE_NUM = 9
};

/* pixel aspect ratio for HDMI HW (AVI packet and EDID) */
enum s5p_tv_hdmi_pxl_aspect {
    HDMI_PIXEL_RATIO_4_3,
    HDMI_PIXEL_RATIO_16_9
};

enum s5p_tv_hdmi_interrrupt {
	HDMI_IRQ_PIN_POLAR_CTL	= 7,
	HDMI_IRQ_GLOBAL		= 6,
	HDMI_IRQ_I2S		= 5,
	HDMI_IRQ_CEC		= 4,
	HDMI_IRQ_HPD_PLUG	= 3,
	HDMI_IRQ_HPD_UNPLUG	= 2,
	HDMI_IRQ_SPDIF		= 1,
	HDMI_IRQ_HDCP		= 0
};

typedef int (*hdmi_isr)(int irq);


extern int s5p_hdmi_register_isr(hdmi_isr isr, u8 irq_num);
extern void s5p_hdmi_enable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
extern void s5p_hdmi_disable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
extern void s5p_hdmi_hpd_gen(void);
extern u8 s5p_hdmi_get_interrupts(void);
extern u8 s5p_hdmi_get_enabled_interrupt(void);
extern int s5p_hdmi_set_dvi(bool en);
extern void s5p_hdmi_mute_en(bool en);

extern void __iomem *hdmi_base;
extern bool __s5p_start_hdcp(void);
extern bool __s5p_stop_hdcp(void);
extern void __s5p_init_hdcp(bool hpd_status, struct i2c_client *ddc_port);

 /* 0 - hdcp stopped, 1 - hdcp started, 2 - hdcp reset */
extern u8 hdcp_protocol_status;

void  __s5p_hdmi_video_set_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);
