/* linux/drivers/media/video/samsung/tv20/s5p_tv.h
 *
 * TV out driver header file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/videodev2.h>
#include <linux/videodev2_samsung.h>
#include <linux/platform_device.h>
#include <linux/fb.h>

#ifdef CONFIG_CPU_S5PV210
#include "s5pv210/tv_out_s5pv210.h"
#endif

#ifdef CONFIG_CPU_S5PC100
#include "s5pc100/tv_out_s5pc100.h"
#endif

/* #define COFIG_TVOUT_DBG */

#ifdef CONFIG_CPU_S5PC100
#define FIX_27M_UNSTABLE_ISSUE
#define S5P_HDCP_I2C_ADDR               0x74
#define I2C_DRIVERID_S5P_HDCP           510
#endif

#define V4L2_STD_ALL_HD			((v4l2_std_id)0xffffffff)


#ifdef CONFIG_TV_FB
#define TVOUT_MINOR_TVOUT		14
#define TVOUT_MINOR_VID			21
#else
#define TVOUT_MINOR_VIDEO		14
#define TVOUT_MINOR_GRP0		21
#define TVOUT_MINOR_GRP1		22
#endif

#define USE_VMIXER_INTERRUPT		1

#define AVI_SAME_WITH_PICTURE_AR	(0x1<<3)

#define AVI_RGB_IF			(0x0<<5)
#define AVI_YCBCR444_IF			(0x2<<5)

#define AVI_ITU601			(0x1<<6)
#define AVI_ITU709			(0x2<<6)

#define AVI_PAR_4_3			(0x1<<4)
#define AVI_PAR_16_9			(0x2<<4)

#define AVI_NO_PIXEL_REPEAT		(0x0<<0)

#define AVI_VIC_2			(2<<0)
#define AVI_VIC_3			(3<<0)
#define AVI_VIC_4			(4<<0)
#define AVI_VIC_5			(5<<0)
#define AVI_VIC_16			(16<<0)
#define AVI_VIC_17			(17<<0)
#define AVI_VIC_18			(18<<0)
#define AVI_VIC_19			(19<<0)
#define AVI_VIC_20			(20<<0)
#define AVI_VIC_31			(31<<0)
#define AVI_VIC_34			(34<<0)


#define VP_UPDATE_RETRY_MAXIMUM		30
#define VP_WAIT_UPDATE_SLEEP		3

struct tvout_output_if {
	enum s5p_tv_disp_mode	disp_mode;
	enum s5p_tv_o_mode	out_mode;
};

struct s5p_img_size {
	u32 img_width;
	u32 img_height;
};

struct s5p_img_offset {
	u32 offset_x;
	u32 offset_y;
};

struct s5p_video_img_address {
	u32 y_address;
	u32 c_address;
};

struct s5p_vl_mode {
	bool line_skip;
	enum s5p_vp_mem_mode mem_mode;
	enum s5p_vp_chroma_expansion chroma_exp;
	enum s5p_vp_filed_id_toggle toggle_id;
};

struct s5p_vl_sharpness {
	u32 th_noise;
	enum s5p_vp_sharpness_control sharpness;
};

struct s5p_vl_csc_ctrl {
	bool sub_y_offset_en;
	bool csc_en;
};

struct s5p_video_poly_filter_coef {
	enum s5p_vp_poly_coeff poly_coeff;
	signed char ch0;
	signed char ch1;
	signed char ch2;
	signed char ch3;
};

struct s5p_vl_bright_contrast_ctrl {
	enum s5p_vp_line_eq eq_num;
	u32 intc;
	u32 slope;
};

struct s5p_video_csc_coef {
	enum s5p_vp_csc_coeff csc_coeff;
	u32 coeff;
};

struct s5p_vl_param {
	bool win_blending;
	u32 alpha;
	u32 priority;
	u32 top_y_address;
	u32 top_c_address;
	enum s5p_endian_type src_img_endian;
	u32 img_width;
	u32 img_height;
	u32 src_offset_x;
	u32 src_offset_y;
	u32 src_width;
	u32 src_height;
	u32 dest_offset_x;
	u32 dest_offset_y;
	u32 dest_width;
	u32 dest_height;
};

struct s5p_tv_vo {
	u32 index;
	struct v4l2_framebuffer fb;
	struct v4l2_window win;
	struct v4l2_rect dst_rect;
	bool win_blending;
	bool blank_change;
	bool pixel_blending;
	bool pre_mul;
	u32 blank_color;
	u32 priority;
	u32 base_addr;
};

struct s5p_bg_dither {
	bool cr_dither_en;
	bool cb_dither_en;
	bool y_dither_en;
};


struct s5p_bg_color {
	u32 color_y;
	u32 color_cb;
	u32 color_cr;
};

struct s5p_vm_csc_coef {
	enum s5p_yuv_fmt_component component;
	enum s5p_tv_coef_y_mode mode;
	u32 coeff_0;
	u32 coeff_1;
	u32 coeff_2;
};

struct s5p_sdout_order {
	enum s5p_sd_order order;
	bool dac[3];
};

struct s5p_sd_vscale_cfg {
	enum s5p_sd_level component_level;
	enum s5p_sd_vsync_ratio component_ratio;
	enum s5p_sd_level composite_level;
	enum s5p_sd_vsync_ratio composite_ratio;
};

struct s5p_sd_vbi {
	bool wss_cvbs;
	enum s5p_sd_closed_caption_type caption_cvbs;
	bool wss_y_svideo;
	enum s5p_sd_closed_caption_type caption_y_svideo;
	bool cgmsa_rgb;
	bool wss_rgb;
	enum s5p_sd_closed_caption_type caption_rgb;
	bool cgmsa_y_pb_pr;
	bool wss_y_pb_pr;
	enum s5p_sd_closed_caption_type caption_y_pb_pr;
};

struct s5p_sd_offset_gain {
	enum s5p_sd_channel_sel channel;
	u32 offset;
	u32 gain;
};

struct s5p_sd_delay {
	u32 delay_y;
	u32 offset_video_start;
	u32 offset_video_end;
};

struct s5p_sd_bright_hue_saturat {
	bool bright_hue_sat_adj;
	u32 gain_brightness;
	u32 offset_brightness;
	u32 gain0_cb_hue_saturation;
	u32 gain1_cb_hue_saturation;
	u32 gain0_cr_hue_saturation;
	u32 gain1_cr_hue_saturation;
	u32 offset_cb_hue_saturation;
	u32 offset_cr_hue_saturation;
};

struct s5p_sd_rgb_compensat {
	bool rgb_color_compensation;
	u32 max_rgb_cube;
	u32 min_rgb_cube;
};

struct s5p_sd_cvbs_compensat {
	bool cvbs_color_compensation;
	u32 y_lower_mid;
	u32 y_bottom;
	u32 y_top;
	u32 y_upper_mid;
	u32 radius;
};

struct s5p_sd_svideo_compensat {
	bool y_color_compensation;
	u32 y_top;
	u32 y_bottom;
	u32 yc_cylinder;
};

struct s5p_sd_component_porch {
	u32 back_525;
	u32 front_525;
	u32 back_625;
	u32 front_625;
};

struct s5p_sd_vesa_rgb_sync {
	enum s5p_sd_vesa_rgb_sync_type sync_type;
	enum s5p_tv_active_polarity vsync_active;
	enum s5p_tv_active_polarity hsync_active;
};

struct s5p_sd_ch_xtalk_cancellat_coeff {
	enum s5p_sd_channel_sel channel;
	u32 coeff1;
	u32 coeff2;
};

struct s5p_sd_closed_caption {
	u32 display_cc;
	u32 nondisplay_cc;
};

struct s5p_sd_525_data {
	bool analog_on;
	enum s5p_sd_525_copy_permit copy_permit;
	enum s5p_sd_525_mv_psp mv_psp;
	enum s5p_sd_525_copy_info copy_info;
	enum s5p_sd_525_aspect_ratio display_ratio;
};

struct s5p_sd_625_data {
	bool surroun_f_sound;
	bool copyright;
	bool copy_protection;
	bool text_subtitles;
	enum s5p_sd_625_subtitles open_subtitles;
	enum s5p_sd_625_camera_film camera_film;
	enum s5p_sd_625_color_encoding color_encoding;
	bool helper_signal;
	enum s5p_sd_625_aspect_ratio display_ratio;
};

struct s5p_hdmi_bluescreen {
	bool enable;
	u8 cb_b;
	u8 y_g;
	u8 cr_r;
};

struct s5p_hdmi_color_range {
	u8 y_min;
	u8 y_max;
	u8 c_min;
	u8 c_max;
};

struct s5p_hdmi_video_infoframe {
	enum s5p_hdmi_transmit trans_type;
	u8 check_sum;
	u8 *data;
};

struct s5p_hdmi_tg_cmd {
	bool timing_correction_en;
	bool bt656_sync_en;
	bool tg_en;
};

struct s5p_hdmi_spd_infoframe {
	enum s5p_hdmi_transmit trans_type;
	u8 *spd_header;
	u8 *spd_data;
};

struct s5p_tv_v4l2 {
	struct v4l2_output      *output;
	struct v4l2_standard	*std;
	struct v4l2_format	*fmt_v;
	struct v4l2_format	*fmt_vo_0;
	struct v4l2_format	*fmt_vo_1;
};


#define S5PTVFB_AVALUE(r, g, b)	\
	(((r & 0xf) << 8) | ((g & 0xf) << 4) | ((b & 0xf) << 0))
#define S5PTVFB_CHROMA(r, g, b)	\
	(((r & 0xff) << 16) | ((g & 0xff) << 8) | ((b & 0xff) << 0))

#define S5PTVFB_WIN_POSITION \
	_IOW('F', 213, struct s5ptvfb_user_window)
#define S5PTVFB_WIN_SET_PLANE_ALPHA \
	_IOW('F', 214, struct s5ptvfb_user_plane_alpha)
#define S5PTVFB_WIN_SET_CHROMA \
	_IOW('F', 215, struct s5ptvfb_user_chroma)

enum s5ptvfb_data_path_t {
	DATA_PATH_FIFO = 0,
	DATA_PATH_DMA = 1,
};

enum s5ptvfb_alpha_t {
	PLANE_BLENDING,
	PIXEL_BLENDING,
};

enum s5ptvfb_chroma_dir_t {
	CHROMA_FG,
	CHROMA_BG,
};

struct s5ptvfb_alpha {
	enum		s5ptvfb_alpha_t mode;
	int		channel;
	unsigned int	value;
};

struct s5ptvfb_chroma {
	int		enabled;
	int		blended;
	unsigned int	key;
	unsigned int	comp_key;
	unsigned int	alpha;
	enum		s5ptvfb_chroma_dir_t dir;
};

struct s5ptvfb_user_window {
	int x;
	int y;
};

struct s5ptvfb_user_plane_alpha {
	int		channel;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

struct s5ptvfb_user_chroma {
	int		enabled;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};


struct s5ptvfb_window {
	int		id;
	int		enabled;
	atomic_t	in_use;
	int		x;
	int		y;
	enum		s5ptvfb_data_path_t path;
	int		local_channel;
	int		dma_burst;
	unsigned int	pseudo_pal[16];
	struct		s5ptvfb_alpha alpha;
	struct		s5ptvfb_chroma chroma;
	int		(*suspend_fifo)(void);
	int		(*resume_fifo)(void);
};

struct s5ptvfb_lcd_timing {
	int	h_fp;
	int	h_bp;
	int	h_sw;
	int	v_fp;
	int	v_fpe;
	int	v_bp;
	int	v_bpe;
	int	v_sw;
};

struct s5ptvfb_lcd_polarity {
	int	rise_vclk;
	int	inv_hsync;
	int	inv_vsync;
	int	inv_vden;
};

struct s5ptvfb_lcd {
	int	width;
	int	height;
	int	bpp;
	int	freq;
	struct	s5ptvfb_lcd_timing timing;
	struct	s5ptvfb_lcd_polarity polarity;

	void	(*init_ldi)(void);
};

struct s5p_tv_status {
	/* TVOUT_SET_INTERFACE_PARAM */
	bool tvout_param_available;
	struct tvout_output_if tvout_param;

	/* TVOUT_SET_OUTPUT_ENABLE/DISABLE */
	bool tvout_output_enable;

	/* TVOUT_SET_LAYER_MODE/POSITION */
	bool vl_mode;
	bool grp_mode[2];

	/* Video Layer Parameters */
	struct s5p_vl_param vl_basic_param;
	struct s5p_vl_mode vl_op_mode;
	struct s5p_vl_sharpness vl_sharpness;
	struct s5p_vl_csc_ctrl vl_csc_control;
	struct s5p_vl_bright_contrast_ctrl vl_bc_control[8];

	enum s5p_vp_src_color src_color;
	enum s5p_vp_field field_id;
	enum s5p_vp_pxl_rate vl_rate;
	enum s5p_vp_csc_type vl_csc_type;

	u32 vl_top_y_address;
	u32 vl_top_c_address;
	u32 vl_bottom_y_address;
	u32 vl_bottom_c_address;
	u32 vl_src_offset_x;
	u32 vl_src_x_fact_step;
	u32 vl_src_offset_y;
	u32 vl_src_width;
	u32 vl_src_height;
	u32 vl_dest_offset_x;
	u32 vl_dest_offset_y;
	u32 vl_dest_width;
	u32 vl_dest_height;
	bool vl2d_ipc;

	bool vl_poly_filter_default;
	bool vl_bypass_post_process;
	u32 vl_saturation;
	bool us_vl_brightness;
	u8 vl_contrast;
	u32 vl_bright_offset;
	bool vl_csc_coef_default;

	/* GRP Layer Common Parameters */
	enum s5p_vmx_burst_mode grp_burst;
	enum s5p_endian_type grp_endian;

	/* BackGroung Layer Parameters */
	struct s5p_bg_dither bg_dither;
	struct s5p_bg_color bg_color[3];

	/* Video Mixer Parameters */
	bool vm_csc_coeff_default;

	/* SDout Parameters */
	struct s5p_sd_vscale_cfg sdout_video_scale_cfg;
	struct s5p_sd_vbi sdout_vbi;
	struct s5p_sd_offset_gain sdout_offset_gain[3];
	struct s5p_sd_delay sdout_delay;
	struct s5p_sd_bright_hue_saturat sdout_bri_hue_set;
	struct s5p_sd_rgb_compensat sdout_rgb_compen;
	struct s5p_sd_cvbs_compensat sdout_cvbs_compen;
	struct s5p_sd_svideo_compensat sdout_svideo_compen;
	struct s5p_sd_component_porch sdout_comp_porch;
	struct s5p_sd_vesa_rgb_sync sdout_rgb_sync;
	struct s5p_sd_ch_xtalk_cancellat_coeff sdout_xtalk_cc[3];
	struct s5p_sd_closed_caption sdout_closed_capt;
	struct s5p_sd_525_data sdout_wss_525;
	struct s5p_sd_625_data sdout_wss_625;
	struct s5p_sd_525_data sdout_cgms_525;
	struct s5p_sd_625_data sdout_cgms_625;

	enum s5p_sd_order sdout_order;
	enum s5p_sd_sync_sig_pin sdout_sync_pin;

	bool sdout_color_sub_carrier_phase_adj;
	bool sdout_dac_on[3];
	bool sdout_y_pb_pr_comp;

	/* HDMI video parameters */
	struct s5p_hdmi_bluescreen hdmi_video_blue_screen;
	struct s5p_hdmi_color_range hdmi_color_range;
	struct s5p_hdmi_video_infoframe hdmi_av_info_frame;
	struct s5p_hdmi_video_infoframe hdmi_mpg_info_frame;
	struct s5p_hdmi_tg_cmd hdmi_tg_cmd;
	u8 avi_byte[13];
	u8 mpg_byte[5];

	/* HDMI parameters */
	struct s5p_hdmi_spd_infoframe hdmi_spd_info_frame;
	u8 spd_header[3];
	u8 spd_data[28];
	bool hdcp_en;
	enum s5p_hdmi_audio_type hdmi_audio_type;
	bool hpd_status;
	bool suspend_status;

	/* TVOUT_SET_LAYER_ENABLE/DISABLE */
	bool vp_layer_enable;
	bool grp_layer_enable[2];

	/* i2c for hdcp port */
	struct i2c_client	*hdcp_i2c_client;

	struct s5p_tv_vo	overlay[2];

	struct video_device      *video_dev[3];

	struct regulator	*tv_tv;
	struct regulator	*tv_tvout;
	struct regulator	*tv_regulator;

	struct clk *tvenc_clk;
	struct clk *vp_clk;
	struct clk *mixer_clk;
	struct clk *hdmi_clk;
	struct clk *i2c_phy_clk;
	struct clk *sclk_hdmiphy;
	struct clk *sclk_pixel;
	struct clk *sclk_dac;
	struct clk *sclk_hdmi;
	struct clk *sclk_mixer;

	struct s5p_tv_v4l2	v4l2;

	struct s5ptvfb_window win;
	struct fb_info	*fb;
	struct device *dev_fb;

	struct s5ptvfb_lcd *lcd;
	struct mutex fb_lock;
};

/* F R A M E  B U F F E R */
#define S5PTVFB_NAME		"s5ptvfb"

/*
* V4L2 TVOUT EXTENSIONS
*
*/
#define V4L2_INPUT_TYPE_MSDMA			3
#define V4L2_INPUT_TYPE_FIFO			4

#define V4L2_OUTPUT_TYPE_MSDMA			4
#define V4L2_OUTPUT_TYPE_COMPOSITE		5
#define V4L2_OUTPUT_TYPE_SVIDEO			6
#define V4L2_OUTPUT_TYPE_YPBPR_INERLACED	7
#define V4L2_OUTPUT_TYPE_YPBPR_PROGRESSIVE	8
#define V4L2_OUTPUT_TYPE_RGB_PROGRESSIVE	9
#define V4L2_OUTPUT_TYPE_DIGITAL		10
#define V4L2_OUTPUT_TYPE_HDMI			V4L2_OUTPUT_TYPE_DIGITAL
#define V4L2_OUTPUT_TYPE_HDMI_RGB		11
#define V4L2_OUTPUT_TYPE_DVI			12

#define V4L2_STD_PAL_BDGHI	(V4L2_STD_PAL_B| \
				V4L2_STD_PAL_D|	\
				V4L2_STD_PAL_G|	\
				V4L2_STD_PAL_H|	\
				V4L2_STD_PAL_I)

#define V4L2_STD_480P_60_16_9	((v4l2_std_id)0x04000000)
#define V4L2_STD_480P_60_4_3	((v4l2_std_id)0x05000000)
#define V4L2_STD_576P_50_16_9	((v4l2_std_id)0x06000000)
#define V4L2_STD_576P_50_4_3	((v4l2_std_id)0x07000000)
#define V4L2_STD_720P_60	((v4l2_std_id)0x08000000)
#define V4L2_STD_720P_50	((v4l2_std_id)0x09000000)
#define V4L2_STD_1080P_60	((v4l2_std_id)0x0a000000)
#define V4L2_STD_1080P_50	((v4l2_std_id)0x0b000000)
#define V4L2_STD_1080I_60	((v4l2_std_id)0x0c000000)
#define V4L2_STD_1080I_50	((v4l2_std_id)0x0d000000)
#define V4L2_STD_480P_59	((v4l2_std_id)0x0e000000)
#define V4L2_STD_720P_59	((v4l2_std_id)0x0f000000)
#define V4L2_STD_1080I_59	((v4l2_std_id)0x10000000)
#define V4L2_STD_1080P_59	((v4l2_std_id)0x11000000)
#define V4L2_STD_1080P_30	((v4l2_std_id)0x12000000)

#define FORMAT_FLAGS_DITHER		0x01
#define FORMAT_FLAGS_PACKED		0x02
#define FORMAT_FLAGS_PLANAR		0x04
#define FORMAT_FLAGS_RAW		0x08
#define FORMAT_FLAGS_CrCb		0x10

#define V4L2_FBUF_FLAG_PRE_MULTIPLY	0x0040
#define V4L2_FBUF_CAP_PRE_MULTIPLY	0x0080

struct v4l2_window_s5p_tvout {
	u32	capability;
	u32	flags;
	u32	priority;

	struct v4l2_window	win;
};

struct v4l2_pix_format_s5p_tvout {
	void *base_y;
	void *base_c;
	bool src_img_endian;

	struct v4l2_pix_format	pix_fmt;
};

extern const struct v4l2_ioctl_ops s5p_tv_v4l2_v_ops;
extern const struct v4l2_ioctl_ops s5p_tv_v4l2_vo_ops;
extern const struct v4l2_ioctl_ops s5p_tv_v4l2_ops;
extern const struct v4l2_ioctl_ops s5p_tv_v4l2_vid_ops;


extern void s5p_tv_v4l2_init_param(void);

extern long s5p_tv_ioctl(struct file *file, u32 cmd, unsigned long arg);
extern long s5p_tv_vid_ioctl(struct file *file, u32 cmd, unsigned long arg);
extern long s5p_tv_v_ioctl(struct file *file, u32 cmd, unsigned long arg);
extern long s5p_tv_vo_ioctl(struct file *file, u32 cmd, unsigned long arg);


#ifdef CONFIG_CPU_S5PC100
int __init __s5p_hdmi_probe(struct platform_device *pdev, u32 res_num);
#endif

#ifdef CONFIG_CPU_S5PV210
int __init __s5p_hdmi_probe(struct platform_device *pdev,
		u32 res_num, u32 res_num2);
int __s5p_hdmi_phy_power(bool on);
#endif

int __init __s5p_sdout_probe(struct platform_device *pdev, u32 res_num);
int __init __s5p_mixer_probe(struct platform_device *pdev, u32 res_num);
int __init __s5p_vp_probe(struct platform_device *pdev, u32 res_num);
int __init __s5p_tvclk_probe(struct platform_device *pdev, u32 res_num);

int __init __s5p_hdmi_release(struct platform_device *pdev);
int __init __s5p_sdout_release(struct platform_device *pdev);
int __init __s5p_mixer_release(struct platform_device *pdev);
int __init __s5p_vp_release(struct platform_device *pdev);
int __init __s5p_tvclk_release(struct platform_device *pdev);


extern	bool _s5p_hdmi_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_hdmi_video_api_proc(unsigned long arg, u32 cmd);

extern	bool _s5p_grp_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_grp_init_param(enum s5p_tv_vmx_layer vm_layer,
		unsigned long p_buf_in);
extern	bool _s5p_grp_start(enum s5p_tv_vmx_layer vmLayer);
extern	bool _s5p_grp_stop(enum s5p_tv_vmx_layer vmLayer);

extern	bool _s5p_tv_if_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_tv_if_init_param(void);
extern	bool _s5p_tv_if_start(void);
extern	bool _s5p_tv_if_stop(void);
extern	bool _s5p_tv_if_set_disp(void);

extern	bool _s5p_bg_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_sdout_api_proc(unsigned long arg, u32 cmd);

extern	bool _s5p_vlayer_set_blending(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_alpha(unsigned long p_buf_in);
extern	bool _s5p_vlayer_api_proc(unsigned long arg, u32 cmd);
extern	bool _s5p_vlayer_init_param(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_priority(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_field_id(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_top_address(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_bottom_address(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_img_size(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_src_position(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_dest_position(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_src_size(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_dest_size(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_brightness(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_contrast(unsigned long p_buf_in);
extern	void _s5p_vlayer_get_priority(unsigned long p_buf_out);
extern	bool _s5p_vlayer_set_brightness_contrast_control(unsigned long
		p_buf_in);
extern	bool _s5p_vlayer_set_poly_filter_coef(unsigned long p_buf_in);
extern	bool _s5p_vlayer_set_csc_coef(unsigned long p_buf_in);
extern	bool _s5p_vlayer_start(void);
extern	bool _s5p_vlayer_stop(void);

void __s5p_read_hdcp_data(u8 reg_addr, u8 count, u8 *data);
void __s5p_write_hdcp_data(u8 reg_addr, u8 count, u8 *data);
void __s5p_write_ainfo(void);
void __s5p_write_an(void);
void __s5p_write_aksv(void);
void __s5p_read_bcaps(void);
void __s5p_read_bksv(void);
bool __s5p_compare_p_value(void);
bool __s5p_compare_r_value(void);
void __s5p_reset_authentication(void);
void __s5p_make_aes_key(void);
void __s5p_set_av_mute_on_off(u32 on_off);
void __s5p_start_encryption(void);
void __s5p_start_decrypting(const u8 *hdcp_key, u32 hdcp_key_size);
bool __s5p_check_repeater(void);
bool __s5p_is_occurred_hdcp_event(void);
irqreturn_t __s5p_hdmi_irq(int irq, void *dev_id);
bool __s5p_is_decrypting_done(void);
void __s5p_set_hpd_detection(u32 detection_type, bool hdcp_enabled,
		struct i2c_client *client);

#ifdef CONFIG_CPU_S5PC100
bool __s5p_start_hdcp(void);
void __s5p_stop_hdcp(void);
void __s5p_hdcp_reset(void);
#endif

void __s5p_hdmi_set_hpd_onoff(bool on_off);
void __s5p_hdmi_audio_set_config(enum s5p_tv_audio_codec_type audio_codec);
void __s5p_hdmi_audio_set_acr(u32 sample_rate);
void __s5p_hdmi_audio_set_asp(void);
void __s5p_hdmi_audio_clock_enable(void);
void __s5p_hdmi_audio_set_repetition_time(
	enum s5p_tv_audio_codec_type audio_codec,
	u32 bits, u32 frame_size_code);
void __s5p_hdmi_audio_irq_enable(u32 irq_en);
void __s5p_hdmi_audio_set_aui(enum s5p_tv_audio_codec_type audio_codec,
	u32 sample_rate, u32 bits);
void __s5p_hdmi_video_set_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);
enum s5p_tv_hdmi_err __s5p_hdmi_init_spd_infoframe(
	enum s5p_hdmi_transmit trans_type,
	u8 *spd_header, u8 *spd_data);
void __s5p_hdmi_init_hpd_onoff(bool on_off);
enum s5p_tv_hdmi_err __s5p_hdmi_audio_init(
	enum s5p_tv_audio_codec_type audio_codec,
	u32 sample_rate, u32 bits, u32 frame_size_code);
enum s5p_tv_hdmi_err __s5p_hdmi_video_init_display_mode(
	enum s5p_tv_disp_mode disp_mode,
	enum s5p_tv_o_mode out_mode, u8 *avidata);
void __s5p_hdmi_video_init_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);
void __s5p_hdmi_video_init_color_range(u8 y_min, u8 y_max, u8 c_min, u8 c_max);
enum s5p_tv_hdmi_err __s5p_hdmi_video_init_csc(
	enum s5p_tv_hdmi_csc_type csc_type);
enum s5p_tv_hdmi_err __s5p_hdmi_video_init_avi_infoframe(
	enum s5p_hdmi_transmit trans_type, u8 check_sum, u8 *pavi_data);
enum s5p_tv_hdmi_err __s5p_hdmi_video_init_mpg_infoframe(
	enum s5p_hdmi_transmit trans_type, u8 check_sum, u8 *pmpg_data);
void __s5p_hdmi_video_init_tg_cmd(bool t_correction_en, bool BT656_sync_en,
	bool tg_en);
bool __s5p_hdmi_start(enum s5p_hdmi_audio_type hdmi_audio_type, bool HDCP_en,
	struct i2c_client *ddc_port);
void __s5p_hdmi_stop(void);

enum s5p_tv_sd_err __s5p_sdout_init_video_scale_cfg(
	enum s5p_sd_level component_level,
	enum s5p_sd_vsync_ratio component_ratio,
	enum s5p_sd_level composite_level,
	enum s5p_sd_vsync_ratio composite_ratio);
enum s5p_tv_sd_err __s5p_sdout_init_sync_signal_pin(
	enum s5p_sd_sync_sig_pin pin);
enum s5p_tv_sd_err __s5p_sdout_init_vbi(bool wss_cvbs,
	enum s5p_sd_closed_caption_type caption_cvbs, bool wss_y_sideo,
	enum s5p_sd_closed_caption_type caption_y_sideo, bool cgmsa_rgb,
	bool wss_rgb, enum s5p_sd_closed_caption_type caption_rgb,
	bool cgmsa_y_ppr, bool wss_y_ppr,
	enum s5p_sd_closed_caption_type caption_y_ppr);
enum s5p_tv_sd_err __s5p_sdout_init_offset_gain(
	enum s5p_sd_channel_sel channel,
	u32 offset, u32 gain);
void __s5p_sdout_init_delay(u32 delay_y, u32 offset_video_start,
	u32 offset_video_end);
void __s5p_sdout_init_schlock(bool color_sucarrier_pha_adj);
enum s5p_tv_sd_err __s5p_sdout_init_dac_power_onoff(
	enum s5p_sd_channel_sel channel,
	bool dac_on);
void __s5p_sdout_init_color_compensaton_onoff(bool bright_hue_saturation_adj,
	bool y_ppr_color_compensation, bool rgb_color_compensation,
	bool y_c_color_compensation, bool y_cvbs_color_compensation);
void __s5p_sdout_init_brightness_hue_saturation(u32 gain_brightness,
	u32 offset_brightness, u32 gain0_cb_hue_saturation,
	u32 gain1_cb_hue_saturation, u32 gain0_cr_hue_saturation,
	u32 gain1_cr_hue_saturation, u32 offset_cb_hue_saturation,
	u32 offset_cr_hue_saturation);
void __s5p_sdout_init_rgb_color_compensation(u32 max_rgb_cube,
	u32 min_rgb_cube);
void __s5p_sdout_init_cvbs_color_compensation(u32 y_lower_mid,
	u32 y_bottom, u32 y_top, u32 y_upper_mid, u32 radius);
void __s5p_sdout_init_svideo_color_compensation(u32 y_top, u32 y_bottom,
	u32 y_c_cylinder);
void __s5p_sdout_init_component_porch(u32 back_525, u32 front_525,
	u32 back_625, u32 front_625);
enum s5p_tv_sd_err __s5p_sdout_init_vesa_rgb_sync(
	enum s5p_sd_vesa_rgb_sync_type sync_type,
	enum s5p_tv_active_polarity v_sync_active,
	enum s5p_tv_active_polarity h_sync_active);
void __s5p_sdout_init_oversampling_filter_coeff(u32 size, u32 *pcoeff0,
	u32 *pcoeff1, u32 *pcoeff2);
enum s5p_tv_sd_err __s5p_sdout_init_ch_xtalk_cancel_coef(
	enum s5p_sd_channel_sel channel,
	u32 coeff2, u32 coeff1);
void __s5p_sdout_init_closed_caption(u32 display_cc, u32 non_display_cc);
enum s5p_tv_sd_err __s5p_sdout_init_wss525_data(
	enum s5p_sd_525_copy_permit copy_permit,
	enum s5p_sd_525_mv_psp mv_psp, enum s5p_sd_525_copy_info copy_info,
	bool analog_on, enum s5p_sd_525_aspect_ratio display_ratio);
enum s5p_tv_sd_err __s5p_sdout_init_wss625_data(bool surround_sound,
	bool copyright, bool copy_protection, bool text_subtitles,
	enum s5p_sd_625_subtitles open_subtitles,
	enum s5p_sd_625_camera_film camera_film,
	enum s5p_sd_625_color_encoding color_encoding,
	bool helper_signal, enum s5p_sd_625_aspect_ratio display_ratio);
enum s5p_tv_sd_err __s5p_sdout_init_cgmsa525_data(
	enum s5p_sd_525_copy_permit copy_permit, enum s5p_sd_525_mv_psp mv_psp,
	enum s5p_sd_525_copy_info copy_info, bool analog_on,
	enum s5p_sd_525_aspect_ratio display_ratio);
enum s5p_tv_sd_err __s5p_sdout_init_cgmsa625_data(bool surround_sound,
	bool copyright, bool copy_protection, bool text_subtitles,
	enum s5p_sd_625_subtitles open_subtitles,
	enum s5p_sd_625_camera_film camera_film,
	enum s5p_sd_625_color_encoding color_encoding, bool helper_signal,
	enum s5p_sd_625_aspect_ratio display_ratio);
enum s5p_tv_sd_err __s5p_sdout_init_display_mode(
	enum s5p_tv_disp_mode disp_mode,
	enum s5p_tv_o_mode out_mode, enum s5p_sd_order order);
void __s5p_sdout_start(void);
void __s5p_sdout_stop(void);
void __s5p_sdout_sw_reset(bool active);
void __s5p_sdout_set_interrupt_enable(bool vsync_intr_en);
void __s5p_sdout_clear_interrupt_pending(void);
bool __s5p_sdout_get_interrupt_pending(void);

enum s5p_tv_vmx_err __s5p_vm_set_win_blend(enum s5p_tv_vmx_layer layer,
	bool enable);
enum s5p_tv_vmx_err __s5p_vm_set_layer_alpha(enum s5p_tv_vmx_layer layer,
	u32 alpha);
enum s5p_tv_vmx_err __s5p_vm_set_layer_show(enum s5p_tv_vmx_layer layer,
	bool show);
enum s5p_tv_vmx_err __s5p_vm_set_layer_priority(enum s5p_tv_vmx_layer layer,
	u32 priority);
enum s5p_tv_vmx_err __s5p_vm_set_grp_base_address(enum s5p_tv_vmx_layer layer,
	u32 baseaddr);
enum s5p_tv_vmx_err __s5p_vm_set_grp_layer_position(
	enum s5p_tv_vmx_layer layer,
	u32 dst_offs_x, u32 dst_offs_y);
enum s5p_tv_vmx_err __s5p_vm_set_grp_layer_size(enum s5p_tv_vmx_layer layer,
	u32 span, u32 width, u32 height, u32 src_offs_x, u32 src_offs_y);
enum s5p_tv_vmx_err __s5p_vm_set_bg_color(
	enum s5p_tv_vmx_bg_color_num colornum,
	u32 color_y, u32 color_cb, u32 color_cr);
enum s5p_tv_vmx_err __s5p_vm_init_status_reg(enum s5p_vmx_burst_mode burst,
	enum s5p_endian_type endian);
enum s5p_tv_vmx_err __s5p_vm_init_display_mode(enum s5p_tv_disp_mode mode,
	enum s5p_tv_o_mode output_mode);

#ifdef CONFIG_CPU_S5PC100
enum s5p_tv_vmx_err __s5p_vm_init_layer(enum s5p_tv_vmx_layer layer, bool show,
	bool winblending, u32 alpha, u32 priority,
	enum s5p_tv_vmx_color_fmt color, bool blankchange,
	bool pixelblending, bool premul, u32 blankcolor,
	u32 baseaddr, u32 span, u32 width, u32 height,
	u32 src_offs_x, u32 src_offs_y, u32 dst_offs_x, u32 dst_offs_y);
#endif

#ifdef CONFIG_CPU_S5PV210
enum s5p_tv_vmx_err __s5p_vm_init_layer(enum s5p_tv_disp_mode mode,
	enum s5p_tv_vmx_layer layer, bool show, bool winblending,
	u32 alpha, u32 priority, enum s5p_tv_vmx_color_fmt color,
	bool blankchange, bool pixelblending, bool premul,
	u32 blankcolor, u32 baseaddr, u32 span, u32 width,
	u32 height, u32 src_offs_x, u32 src_offs_y, u32 dst_offs_x,
	u32 dst_offs_y, u32 dst_x, u32 dst_y);
void __s5p_vm_set_ctrl(enum s5p_tv_vmx_layer layer, bool premul,
	bool pixel_blending, bool blank_change, bool win_blending,
	enum s5p_tv_vmx_color_fmt color, u32 alpha, u32 blank_color);
#endif

void __s5p_vm_init_bg_dither_enable(bool cr_dither_enable,
	bool cdither_enable, bool y_dither_enable);
enum s5p_tv_vmx_err __s5p_vm_init_bg_color(
	enum s5p_tv_vmx_bg_color_num color_num,
	u32 color_y, u32 color_cb, u32 color_cr);
enum s5p_tv_vmx_err __s5p_vm_init_csc_coef(
	enum s5p_yuv_fmt_component component,
	enum s5p_tv_coef_y_mode mode, u32 coeff0, u32 coeff1, u32 coeff2);
void __s5p_vm_init_csc_coef_default(enum s5p_tv_vmx_csc_type csc_type);
enum s5p_tv_vmx_err __s5p_vm_get_layer_info(enum s5p_tv_vmx_layer layer,
	bool *show,
	u32 *priority);
void __s5p_vm_start(void);
void __s5p_vm_stop(void);
enum s5p_tv_vmx_err __s5p_vm_set_underflow_interrupt_enable(
	enum s5p_tv_vmx_layer layer,
	bool en);
void __s5p_vm_clear_pend_all(void);
irqreturn_t __s5p_mixer_irq(int irq, void *dev_id);

void __s5p_vp_set_field_id(enum s5p_vp_field mode);
enum s5p_tv_vp_err __s5p_vp_set_top_field_address(u32 top_y_addr,
	u32 top_c_addr);
enum s5p_tv_vp_err __s5p_vp_set_bottom_field_address(u32 bottom_y_addr,
	u32 bottom_c_addr);
enum s5p_tv_vp_err __s5p_vp_set_img_size(u32 img_width, u32 img_height);
void __s5p_vp_set_src_position(u32 src_off_x, u32 src_x_fract_step,
	u32 src_off_y);
void __s5p_vp_set_dest_position(u32 dst_off_x, u32 dst_off_y);
void __s5p_vp_set_src_dest_size(u32 src_width, u32 src_height,
	u32 dst_width, u32 dst_height, bool ipc_2d);
enum s5p_tv_vp_err __s5p_vp_set_poly_filter_coef(
	enum s5p_vp_poly_coeff poly_coeff,
	signed char ch0, signed char ch1, signed char ch2, signed char ch3);
void __s5p_vp_set_poly_filter_coef_default(u32 h_ratio, u32 v_ratio);
void __s5p_vp_set_src_dest_size_with_default_poly_filter_coef(u32 src_width,
	u32 src_height, u32 dst_width, u32 dst_height, bool ipc_2d);
enum s5p_tv_vp_err __s5p_vp_set_brightness_contrast_control(
	enum s5p_vp_line_eq eq_num,
	u32 intc, u32 slope);
void __s5p_vp_set_brightness(bool brightness);
void __s5p_vp_set_contrast(u8 contrast);
enum s5p_tv_vp_err __s5p_vp_update(void);
enum s5p_vp_field __s5p_vp_get_field_id(void);
bool __s5p_vp_get_update_status(void);
void __s5p_vp_init_field_id(enum s5p_vp_field mode);
void __s5p_vp_init_op_mode(bool line_skip, enum s5p_vp_mem_mode mem_mode,
	enum s5p_vp_chroma_expansion chroma_exp,
	enum s5p_vp_filed_id_toggle toggle_id);
void __s5p_vp_init_pixel_rate_control(enum s5p_vp_pxl_rate rate);
enum s5p_tv_vp_err __s5p_vp_init_layer(u32 top_y_addr, u32 top_c_addr,
	u32 bottom_y_addr, u32 bottom_c_addr,
	enum s5p_endian_type src_img_endian,
	u32 img_width, u32 img_height, u32 src_off_x, u32 src_x_fract_step,
	u32 src_off_y, u32 src_width, u32 src_height, u32 dst_off_x,
	u32 dst_off_y, u32 dst_width, u32 dst_height, bool ipc_2d);
enum s5p_tv_vp_err __s5p_vp_init_layer_def_poly_filter_coef(u32 top_y_addr,
	u32 top_c_addr, u32 bottom_y_addr, u32 bottom_c_addr,
	enum s5p_endian_type src_img_endian, u32 img_width, u32 img_height,
	u32 src_off_x, u32 src_x_fract_step, u32 src_off_y, u32 src_width,
	u32 src_height, u32 dst_off_x, u32 dst_off_y, u32 dst_width,
	u32 dst_height, bool ipc_2d);
enum s5p_tv_vp_err __s5p_vp_init_poly_filter_coef(
	enum s5p_vp_poly_coeff poly_coeff,
	signed char ch0, signed char ch1, signed char ch2, signed char ch3);
void __s5p_vp_init_bypass_post_process(bool bypass);
enum s5p_tv_vp_err __s5p_vp_init_csc_coef(enum s5p_vp_csc_coeff csc_coeff,
	u32 coeff);
void __s5p_vp_init_saturation(u32 sat);
void __s5p_vp_init_sharpness(u32 th_h_noise,
	enum s5p_vp_sharpness_control sharpness);
enum s5p_tv_vp_err __s5p_vp_init_brightness_contrast_control(
	enum s5p_vp_line_eq eq_num,
	u32 intc, u32 slope);
void __s5p_vp_init_brightness(bool brightness);
void __s5p_vp_init_contrast(u8 contrast);
void __s5p_vp_init_brightness_offset(u32 offset);
void __s5p_vp_init_csc_control(bool suy_offset_en, bool csc_en);
enum s5p_tv_vp_err __s5p_vp_init_csc_coef_default(
	enum s5p_vp_csc_type csc_type);
enum s5p_tv_vp_err __s5p_vp_start(void);
enum s5p_tv_vp_err __s5p_vp_stop(void);
void __s5p_vp_sw_reset(void);

#ifdef CONFIG_CPU_S5PC100
void __s5p_tv_clk_init_hpll(u32 lock_time, u32 mdiv, u32 pdiv, u32 sdiv);
#endif

#ifdef CONFIG_CPU_S5PV210
int __s5p_tv_clk_change_internal(void);
#endif

void __s5p_tv_clk_hpll_onoff(bool en);
#ifdef CONFIG_CPU_S5PC100
enum s5p_tv_clk_err __s5p_tv_clk_init_href(
	enum s5p_tv_clk_hpll_ref hpll_ref);
enum s5p_tv_clk_err __s5p_tv_clk_init_mout_hpll(
	enum s5p_tv_clk_mout_hpll mout_hpll);
enum s5p_tv_clk_err __s5p_tv_clk_init_video_mixer(
	enum s5p_tv_clk_vmiexr_srcclk src_clk);
#endif
void __s5p_tv_clk_init_hdmi_ratio(u32 clk_div);
void __s5p_tv_clk_set_vp_clk_onoff(bool clk_on);
void __s5p_tv_clk_set_vmixer_hclk_onoff(bool clk_on);
void __s5p_tv_clk_set_vmixer_sclk_onoff(bool clk_on);
void __s5p_tv_clk_set_sdout_hclk_onoff(bool clk_on);
void __s5p_tv_clk_set_sdout_sclk_onoff(bool clk_on);
void __s5p_tv_clk_set_hdmi_hclk_onoff(bool clk_on);
void __s5p_tv_clk_set_hdmi_sclk_onoff(bool clk_on);
void __s5p_tv_clk_set_hdmi_i2c_clk_onoff(bool clk_on);
void __s5p_tv_clk_start(bool vp_hclk_on, bool sdout_hclk_on,
	bool hdmi_hclk_on);
void __s5p_tv_clk_stop(void);

void __s5p_tv_power_init_mtc_stable_counter(u32 value);
void __s5p_tv_powerinitialize_dac_onoff(bool on);
void __s5p_tv_powerset_dac_onoff(bool on);
bool __s5p_tv_power_get_power_status(void);
bool __s5p_tv_power_get_dac_power_status(void);
void __s5p_tv_poweron(void);
void __s5p_tv_poweroff(void);

extern struct s5p_tv_status s5ptv_status;
extern struct s5p_tv_vo s5ptv_overlay[2];

#ifdef CONFIG_CPU_S5PV210
extern void s5p_hdmi_enable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
extern void s5p_hdmi_disable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
extern void s5p_hdmi_clear_pending(enum s5p_tv_hdmi_interrrupt intr);
extern u8 s5p_hdmi_get_interrupts(void);
extern int s5p_hdmi_register_isr(hdmi_isr isr, u8 irq_num);
extern int s5p_hpd_init(void);
extern u8 s5p_hdmi_get_swhpd_status(void);
extern u8 s5p_hdmi_get_hpd_status(void);
extern void s5p_hdmi_swhpd_disable(void);
extern void s5p_hdmi_hpd_gen(void);
extern int __init __s5p_hdcp_init(void);
extern int s5p_tv_clk_gate(bool on);
#endif

extern int s5ptvfb_init_fbinfo(int id);
extern int s5ptvfb_map_video_memory(struct fb_info *fb);
extern int s5ptvfb_check_var(struct fb_var_screeninfo *var,
	struct fb_info *fb);
extern int s5ptvfb_set_par(struct fb_info *fb);
extern int s5ptvfb_draw_logo(struct fb_info *fb);
extern void s5ptvfb_set_lcd_info(struct s5p_tv_status *ctrl);
extern int s5ptvfb_display_on(struct s5p_tv_status *ctrl);
extern int s5p_hpd_get_state(void);
extern void s5p_handle_cable(void);

#define S5PTVFB_POWER_OFF	_IOW('F', 217, u32)
#define S5PTVFB_POWER_ON	_IOW('F', 218, u32)
#define S5PTVFB_WIN_SET_ADDR	_IOW('F', 219, u32)
#define S5PTVFB_SET_WIN_ON	_IOW('F', 220, u32)
#define S5PTVFB_SET_WIN_OFF	_IOW('F', 221, u32)

extern int s5p_tv_clk_gate(bool on);
extern int s5p_hdcp_is_reset(void);
extern int tv_phy_power(bool on);
extern int s5ptvfb_unmap_video_memory(struct fb_info *fb);

extern struct s5p_tv_status s5ptv_status;
extern bool _s5p_tv_if_set_disp(void);
extern int s5p_hdcp_encrypt_stop(bool on);
extern int s5p_hdmi_set_dvi(bool en);
extern int s5p_hdmi_set_mute(bool en);
extern int s5p_hdmi_get_mute(void);
extern int s5p_hdmi_audio_enable(bool en);
extern void s5p_hdmi_set_audio(bool en);
extern void s5p_hdmi_mute_en(bool en);
extern bool __s5p_start_hdcp(void);
extern bool __s5p_stop_hdcp(void);

#if defined(CONFIG_MACH_P1)
extern struct i2c_driver SII9234_i2c_driver;
extern struct i2c_driver SII9234A_i2c_driver;
extern struct i2c_driver SII9234B_i2c_driver;
extern struct i2c_driver SII9234C_i2c_driver;
#endif

