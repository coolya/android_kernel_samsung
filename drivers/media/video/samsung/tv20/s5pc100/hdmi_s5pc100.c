/* linux/drivers/media/video/samsung/tv20/s5pc100/hdmi_s5pc100.c
 *
 * hdmi raw ftn  file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "tv_out_s5pc100.h"

#include "regs/regs-hdmi.h"

#include "plat/regs-clock.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_HDMI_DEBUG 1
#endif

#ifdef S5P_HDMI_DEBUG
#define HDMIPRINTK(fmt, args...) \
	printk(KERN_INFO "\t\t[HDMI] %s: " fmt, __func__ , ## args)
#else
#define HDMIPRINTK(fmt, args...)
#endif

static struct resource	*hdmi_mem;
void __iomem		*hdmi_base;

static unsigned short g_hdmi_video_parm_tbl[] = {
	/*  480P_60, 576P_50, 720P_60, 720P_50, 1080I_60, 1080I_50, VGAP_60*/
	138,    144,    370,    700,    280,    720,    160,
	525,    625,    750,    750,    562,    562,    525,
	45,     49,     30,     30,     22,     22,    45,
	525,    625,    750,    750,    1125,   1125,   525,
	858,    864,    1650,   1980,   2200,   2640,    800,
	1,      1,      0,      0,      0,      0,     1,
	0,      0,      0,      0,     585,    585,    0,
	0,      0,      0,      0,     1125,   1125,   0,
	14,     10,     108,    438,    86,     526,    14,
	76,     74,     148,    478,    130,    570,    110,
	1,      1,      0,      0,      0,      0,     1,
	15,     10,     10,     10,     7,      7,     12,
	9,      5,      5,      5,      2,      2,     10,
	0,      0,      0,      0,     569,    569,    0,
	0,      0,      0,      0,     564,    564,    0,
	0,      0,      0,      0,     1187,   1847,   0,
	0,      0,      0,      0,     1187,   1847,   0,
	858,    864,    1650,   1980,   2200,   2640,    800
	138,    144,    370,    700,    280,    720,    160,
	720,    720,    1280,   1280,   1920,   1920,   640,
	525,    625,    750,    750,    1125,   1125,   525,
	7,      1,      1,      1,      1,      1,     1,
	0,      0,      0,      0,     563,    563,    0,
	45,     49,     30,     30,     22,     22,    45,
	480,    576,    720,    720,    540,    540,    480,
	0,      0,      0,      0,     563,    563,    0,
	0,      0,      0,      0,     584,    584,    0,
	7,      1,      1,      1,      1,      1,     1,
	7,      1,      1,      1,     563,    563,    1,
	7,      1,      1,      1,      1,      1,     1,
	7,      1,      1,      1,     563,    563,     1
};



/*
* set  - set functions are only called under running HDMI
*/
void __s5p_hdmi_set_hpd_onoff(bool on_off)
{
	HDMIPRINTK("%d\n\r", on_off);

	if (on_off)
		writel(SW_HPD_PLUGGED, hdmi_base + S5P_HPD);
	else
		writel(SW_HPD_UNPLUGGED, hdmi_base + S5P_HPD);


	HDMIPRINTK("0x%08x\n\r", readl(hdmi_base + S5P_HPD));
}


void __s5p_hdmi_audio_set_config(enum s5p_tv_audio_codec_type audio_codec)
{

	u32 data_type = (audio_codec == PCM) ? CONFIG_LINEAR_PCM_TYPE :
			(audio_codec == AC3) ? CONFIG_NON_LINEAR_PCM_TYPE :
				0xff;

	HDMIPRINTK("(%d)\n\r", audio_codec);

	writel(CONFIG_FILTER_2_SAMPLE | data_type
	       | CONFIG_PCPD_MANUAL_SET | CONFIG_WORD_LENGTH_MANUAL_SET
	       | CONFIG_U_V_C_P_REPORT | CONFIG_BURST_SIZE_2
	       | CONFIG_DATA_ALIGN_32BIT
	       , hdmi_base + S5P_SPDIFIN_CONFIG_1);
	writel(0, hdmi_base + S5P_SPDIFIN_CONFIG_2);
	HDMIPRINTK("()\n\r");
}

void __s5p_hdmi_audio_set_acr(u32 sample_rate)
{
	u32 value_n = (sample_rate == 32000) ? 4096 :
		      (sample_rate == 44100) ? 6272 :
		      (sample_rate == 88200) ? 12544 :
		      (sample_rate == 176400) ? 25088 :
		      (sample_rate == 48000) ? 6144 :
		      (sample_rate == 96000) ? 12288 :
		      (sample_rate == 192000) ? 24576 : 0;

	u32 cts = (sample_rate == 32000) ? 27000 :
		  (sample_rate == 44100) ? 30000 :
		  (sample_rate == 88200) ? 30000 :
		  (sample_rate == 176400) ? 30000 :
		  (sample_rate == 48000) ? 27000 :
		  (sample_rate == 96000) ? 27000 :
		  (sample_rate == 192000) ? 27000 : 0;

	HDMIPRINTK("(%d)\n\r", sample_rate);

	writel(value_n & 0xff, hdmi_base + S5P_ACR_N0);
	writel((value_n >> 8) & 0xff, hdmi_base + S5P_ACR_N1);
	writel((value_n >> 16) & 0xff, hdmi_base + S5P_ACR_N2);

	writel(cts & 0xff, hdmi_base + S5P_ACR_MCTS0);
	writel((cts >> 8) & 0xff, hdmi_base + S5P_ACR_MCTS1);
	writel((cts >> 16) & 0xff, hdmi_base + S5P_ACR_MCTS2);

	writel(cts & 0xff, hdmi_base + S5P_ACR_CTS0);
	writel((cts >> 8) & 0xff, hdmi_base + S5P_ACR_CTS1);
	writel((cts >> 16) & 0xff, hdmi_base + S5P_ACR_CTS2);

	writel(4, hdmi_base + S5P_ACR_CON);

	HDMIPRINTK("()\n\r");
}

void __s5p_hdmi_audio_set_asp(void)
{
	HDMIPRINTK("()\n\r");
	writel(0x0, hdmi_base + S5P_ASP_CON);
	writel(0x0, hdmi_base + S5P_ASP_SP_FLAT);

	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG0);
	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG1);
	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG2);
	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG3);
	HDMIPRINTK("()\n\r");
}

void  __s5p_hdmi_audio_clock_enable(void)
{
	HDMIPRINTK("()\n\r");
	writel(0x1, hdmi_base + S5P_SPDIFIN_CLK_CTRL);
	writel(0x3, hdmi_base + S5P_SPDIFIN_OP_CTRL);
	HDMIPRINTK("()\n\r");
}

void __s5p_hdmi_audio_set_repetition_time(
	enum s5p_tv_audio_codec_type audio_codec,
	u32 bits, u32 frame_size_code)
{
	u32 wl = 5 << 1 | 1;
	u32 rpt_cnt = (audio_codec == AC3) ? 1536 * 2 - 1 : 0;

	HDMIPRINTK("()\n\r");

	writel(((rpt_cnt&0xf) << 4) | wl,
		hdmi_base + S5P_SPDIFIN_USER_VALUE_1);
	writel((rpt_cnt >> 4)&0xff, hdmi_base + S5P_SPDIFIN_USER_VALUE_2);
	writel(frame_size_code&0xff, hdmi_base + S5P_SPDIFIN_USER_VALUE_3);
	writel((frame_size_code >> 8)&0xff,
		hdmi_base + S5P_SPDIFIN_USER_VALUE_4);
	HDMIPRINTK("()\n\r");
}



void __s5p_hdmi_audio_irq_enable(u32 irq_en)
{
	writel(irq_en, hdmi_base + S5P_SPDIFIN_IRQ_MASK);
}


void __s5p_hdmi_audio_set_aui(enum s5p_tv_audio_codec_type audio_codec,
			      u32 sample_rate,
			      u32 bits)
{
	u8 sum_of_bits, bytes1, bytes2, bytes3, check_sum;
	u32 bit_rate;
	u32 bps = (audio_codec == PCM) ? bits : 16;

	u32 type = (audio_codec == PCM) ? 1 :
		   (audio_codec == AC3) ? 2 : 0;
	u32 ch = (audio_codec == PCM) ? 1 : 0;

	u32 sample = (sample_rate == 32000) ? 1 :
		     (sample_rate == 44100) ? 2 :
		     (sample_rate == 48000) ? 3 :
		     (sample_rate == 88200) ? 4 :
		     (sample_rate == 96000) ? 5 :
		     (sample_rate == 176400) ? 6 :
		     (sample_rate == 192000) ? 7 : 0;

	u32 bpsType = (bps == 16) ? 1 :
		      (bps == 20) ? 2 :
		      (bps == 24) ? 3 : 0;

	HDMIPRINTK("()\n\r");

	bpsType = (audio_codec == PCM) ? bpsType : 0;

	sum_of_bits = (0x84 + 0x1 + 10);

	bytes1 = (u8)((type << 4) | ch);

	bytes2 = (u8)((sample << 2) | bpsType);

	bit_rate = 256;

	bytes3 = (audio_codec == PCM) ? (u8)0 : (u8)(bit_rate / 8) ;


	sum_of_bits += (bytes1 + bytes2 + bytes3);
	check_sum = 256 - sum_of_bits;

	writel(check_sum , hdmi_base + S5P_AUI_CHECK_SUM);
	writel(bytes1 , hdmi_base + S5P_AUI_BYTE1);
	writel(bytes2 , hdmi_base + S5P_AUI_BYTE2);
	writel(bytes3 , hdmi_base + S5P_AUI_BYTE3);
	writel(0x00 , hdmi_base + S5P_AUI_BYTE4);
	writel(0x00 , hdmi_base + S5P_AUI_BYTE5);


	writel(2 , hdmi_base + S5P_ACP_CON);
	writel(1 , hdmi_base + S5P_ACP_TYPE);

	writel(0x10 , hdmi_base + S5P_GCP_BYTE1);
	writel(0x2 , hdmi_base + S5P_GCP_CON);

	HDMIPRINTK("()\n\r");

}

void __s5p_hdmi_video_set_bluescreen(bool en,
				     u8 cb_b,
				     u8 y_g,
				     u8 cr_r)
{
	HDMIPRINTK("%d,%d,%d,%d\n\r", en, cb_b, y_g, cr_r);

	if (en) {
		writel(SET_BLUESCREEN_0(cb_b), hdmi_base + S5P_BLUE_SCREEN_0);
		writel(SET_BLUESCREEN_1(y_g), hdmi_base + S5P_BLUE_SCREEN_1);
		writel(SET_BLUESCREEN_2(cr_r), hdmi_base + S5P_BLUE_SCREEN_2);
		writel(readl(hdmi_base + S5P_HDMI_CON_0) | BLUE_SCR_EN,
		       hdmi_base + S5P_HDMI_CON_0);

		HDMIPRINTK("HDMI_BLUE_SCREEN0 = 0x%08x \n\r",
			   readl(hdmi_base + S5P_BLUE_SCREEN_0));
		HDMIPRINTK("HDMI_BLUE_SCREEN1 = 0x%08x \n\r",
			   readl(hdmi_base + S5P_BLUE_SCREEN_1));
		HDMIPRINTK("HDMI_BLUE_SCREEN2 = 0x%08x \n\r",
			   readl(hdmi_base + S5P_BLUE_SCREEN_2));
	} else {
		writel(readl(hdmi_base + S5P_HDMI_CON_0)&~BLUE_SCR_EN,
		       hdmi_base + S5P_HDMI_CON_0);
	}

	HDMIPRINTK("HDMI_CON0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_CON_0));
}


/*
* initialization  - iniization functions are only called under stopping HDMI
*/
enum s5p_tv_hdmi_err __s5p_hdmi_init_spd_infoframe(
				enum s5p_hdmi_transmit trans_type,
				u8 *spd_header,
				u8 *spd_data)
{
	HDMIPRINTK("%d,%d,%d\n\r", (u32)trans_type,
		   (u32)spd_header,
		   (u32)spd_data);

	switch (trans_type) {

	case HDMI_DO_NOT_TANS:
		writel(SPD_TX_CON_NO_TRANS, hdmi_base + S5P_SPD_CON);
		break;

	case HDMI_TRANS_ONCE:
		writel(SPD_TX_CON_TRANS_ONCE, hdmi_base + S5P_SPD_CON);
		break;

	case HDMI_TRANS_EVERY_SYNC:
		writel(SPD_TX_CON_TRANS_EVERY_VSYNC, hdmi_base + S5P_SPD_CON);
		break;

	default:
		HDMIPRINTK(" invalid out_mode parameter(%d)\n\r", trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_SPD_HEADER(*(spd_header)), hdmi_base + S5P_SPD_HEADER0);

	writel(SET_SPD_HEADER(*(spd_header + 1)) , hdmi_base + S5P_SPD_HEADER1);
	writel(SET_SPD_HEADER(*(spd_header + 2)) , hdmi_base + S5P_SPD_HEADER2);

	writel(SET_SPD_DATA(*(spd_data)), hdmi_base + S5P_SPD_DATA0);
	writel(SET_SPD_DATA(*(spd_data + 1)) , hdmi_base + S5P_SPD_DATA1);
	writel(SET_SPD_DATA(*(spd_data + 2)) , hdmi_base + S5P_SPD_DATA2);
	writel(SET_SPD_DATA(*(spd_data + 3)) , hdmi_base + S5P_SPD_DATA3);
	writel(SET_SPD_DATA(*(spd_data + 4)) , hdmi_base + S5P_SPD_DATA4);
	writel(SET_SPD_DATA(*(spd_data + 5)) , hdmi_base + S5P_SPD_DATA5);
	writel(SET_SPD_DATA(*(spd_data + 6)) , hdmi_base + S5P_SPD_DATA6);
	writel(SET_SPD_DATA(*(spd_data + 7)) , hdmi_base + S5P_SPD_DATA7);
	writel(SET_SPD_DATA(*(spd_data + 8)) , hdmi_base + S5P_SPD_DATA8);
	writel(SET_SPD_DATA(*(spd_data + 9)) , hdmi_base + S5P_SPD_DATA9);
	writel(SET_SPD_DATA(*(spd_data + 10)) , hdmi_base + S5P_SPD_DATA10);
	writel(SET_SPD_DATA(*(spd_data + 11)) , hdmi_base + S5P_SPD_DATA11);
	writel(SET_SPD_DATA(*(spd_data + 12)) , hdmi_base + S5P_SPD_DATA12);
	writel(SET_SPD_DATA(*(spd_data + 13)) , hdmi_base + S5P_SPD_DATA13);
	writel(SET_SPD_DATA(*(spd_data + 14)) , hdmi_base + S5P_SPD_DATA14);
	writel(SET_SPD_DATA(*(spd_data + 15)) , hdmi_base + S5P_SPD_DATA15);
	writel(SET_SPD_DATA(*(spd_data + 16)) , hdmi_base + S5P_SPD_DATA16);
	writel(SET_SPD_DATA(*(spd_data + 17)) , hdmi_base + S5P_SPD_DATA17);
	writel(SET_SPD_DATA(*(spd_data + 18)) , hdmi_base + S5P_SPD_DATA18);
	writel(SET_SPD_DATA(*(spd_data + 19)) , hdmi_base + S5P_SPD_DATA19);
	writel(SET_SPD_DATA(*(spd_data + 20)) , hdmi_base + S5P_SPD_DATA20);
	writel(SET_SPD_DATA(*(spd_data + 21)) , hdmi_base + S5P_SPD_DATA21);
	writel(SET_SPD_DATA(*(spd_data + 22)) , hdmi_base + S5P_SPD_DATA22);
	writel(SET_SPD_DATA(*(spd_data + 23)) , hdmi_base + S5P_SPD_DATA23);
	writel(SET_SPD_DATA(*(spd_data + 24)) , hdmi_base + S5P_SPD_DATA24);
	writel(SET_SPD_DATA(*(spd_data + 25)) , hdmi_base + S5P_SPD_DATA25);
	writel(SET_SPD_DATA(*(spd_data + 26)) , hdmi_base + S5P_SPD_DATA26);
	writel(SET_SPD_DATA(*(spd_data + 27)) , hdmi_base + S5P_SPD_DATA27);

	HDMIPRINTK("SPD_CON = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_CON));
	HDMIPRINTK("SPD_HEADER0 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_HEADER0));
	HDMIPRINTK("SPD_HEADER1 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_HEADER1));
	HDMIPRINTK("SPD_HEADER2 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_HEADER2));
	HDMIPRINTK("SPD_DATA0  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA0));
	HDMIPRINTK("SPD_DATA1  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA1));
	HDMIPRINTK("SPD_DATA2  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA2));
	HDMIPRINTK("SPD_DATA3  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA3));
	HDMIPRINTK("SPD_DATA4  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA4));
	HDMIPRINTK("SPD_DATA5  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA5));
	HDMIPRINTK("SPD_DATA6  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA6));
	HDMIPRINTK("SPD_DATA7  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA7));
	HDMIPRINTK("SPD_DATA8  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA8));
	HDMIPRINTK("SPD_DATA9  = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA9));
	HDMIPRINTK("SPD_DATA10 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA10));
	HDMIPRINTK("SPD_DATA11 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA11));
	HDMIPRINTK("SPD_DATA12 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA12));
	HDMIPRINTK("SPD_DATA13 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA13));
	HDMIPRINTK("SPD_DATA14 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA14));
	HDMIPRINTK("SPD_DATA15 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA15));
	HDMIPRINTK("SPD_DATA16 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA16));
	HDMIPRINTK("SPD_DATA17 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA17));
	HDMIPRINTK("SPD_DATA18 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA18));
	HDMIPRINTK("SPD_DATA19 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA19));
	HDMIPRINTK("SPD_DATA20 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA20));
	HDMIPRINTK("SPD_DATA21 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA21));
	HDMIPRINTK("SPD_DATA22 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA22));
	HDMIPRINTK("SPD_DATA23 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA23));
	HDMIPRINTK("SPD_DATA24 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA24));
	HDMIPRINTK("SPD_DATA25 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA25));
	HDMIPRINTK("SPD_DATA26 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA26));
	HDMIPRINTK("SPD_DATA27 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_SPD_DATA27));

	return HDMI_NO_ERROR;
}

void __s5p_hdmi_init_hpd_onoff(bool on_off)
{
	HDMIPRINTK("%d\n\r", on_off);
	__s5p_hdmi_set_hpd_onoff(on_off);
	HDMIPRINTK("0x%08x\n\r", readl(hdmi_base + S5P_HPD));
}

enum s5p_tv_hdmi_err __s5p_hdmi_audio_init(
	enum s5p_tv_audio_codec_type audio_codec,
	u32 sample_rate, u32 bits, u32 frame_size_code)
{
	__s5p_hdmi_audio_set_config(audio_codec);
	__s5p_hdmi_audio_set_repetition_time(audio_codec, bits,
		frame_size_code);
	__s5p_hdmi_audio_irq_enable(IRQ_BUFFER_OVERFLOW_ENABLE);
	__s5p_hdmi_audio_clock_enable();
	__s5p_hdmi_audio_set_asp();
	__s5p_hdmi_audio_set_acr(sample_rate);
	__s5p_hdmi_audio_set_aui(audio_codec, sample_rate, bits);

	return HDMI_NO_ERROR;
}

enum s5p_tv_hdmi_err __s5p_hdmi_video_init_display_mode(
	enum s5p_tv_disp_mode disp_mode,
	enum s5p_tv_o_mode out_mode)
{
	enum s5p_tv_hdmi_disp_mode hdmi_disp_num;

	HDMIPRINTK("%d,%d\n\r", disp_mode, out_mode);

	switch (disp_mode) {

	case TVOUT_480P_60_16_9:

	case TVOUT_480P_60_4_3:

	case TVOUT_576P_50_16_9:

	case TVOUT_576P_50_4_3:

	case TVOUT_720P_60:

	case TVOUT_720P_50:
		writel(INT_PRO_MODE_PROGRESSIVE, hdmi_base + S5P_INT_PRO_MODE);
		break;

	default:
		HDMIPRINTK("invalid disp_mode parameter(%d)\n\r", disp_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	switch (disp_mode) {

	case TVOUT_480P_60_16_9:

	case TVOUT_480P_60_4_3:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_480P_60;
		break;

	case TVOUT_576P_50_16_9:

	case TVOUT_576P_50_4_3:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_576P_50;
		break;

	case TVOUT_720P_60:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_720P_60;
		break;

	case TVOUT_720P_50:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_720P_50;
		break;

	default:
		HDMIPRINTK(" invalid disp_mode parameter(%d)\n\r", disp_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	switch (out_mode) {

	case TVOUT_OUTPUT_HDMI:
		writel(PX_LMT_CTRL_BYPASS, hdmi_base + S5P_HDMI_CON_1);
		writel(VID_PREAMBLE_EN | GUARD_BAND_EN,
			hdmi_base + S5P_HDMI_CON_2);
		writel(HDMI_MODE_EN | DVI_MODE_DIS, hdmi_base + S5P_MODE_SEL);
		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r", out_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(0x2b, hdmi_base + S5P_VACT_ST_MG);

	writel(0x30, hdmi_base + S5P_VACT_END_MG);

	writel(SET_H_BLANK_L(g_hdmi_video_parm_tbl
		[H_BLANK*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_H_BLANK_0);
	writel(SET_H_BLANK_H(g_hdmi_video_parm_tbl
		[H_BLANK*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_H_BLANK_1);

	writel(SET_V2_BLANK_L(g_hdmi_video_parm_tbl
		[V2_BLANK*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_BLANK_0);
	writel(SET_V2_BLANK_H(g_hdmi_video_parm_tbl
		[V2_BLANK*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       SET_V1_BLANK_L(g_hdmi_video_parm_tbl
	       [V1_BLANK*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_BLANK_1);
	writel(SET_V1_BLANK_H(g_hdmi_video_parm_tbl
		[V1_BLANK*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_BLANK_2);

	writel(SET_V_LINE_L(g_hdmi_video_parm_tbl
		[V_LINE*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_H_V_LINE_0);
	writel(SET_V_LINE_H(g_hdmi_video_parm_tbl
		[V_LINE*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       SET_H_LINE_L(g_hdmi_video_parm_tbl
	       [H_LINE*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_H_V_LINE_1);
	writel(SET_H_LINE_H(g_hdmi_video_parm_tbl
		[H_LINE*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_H_V_LINE_2);

	writel(g_hdmi_video_parm_tbl
		[VSYNC_POL*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num],
		hdmi_base + S5P_SYNC_MODE);

	writel(0xfd, hdmi_base + S5P_SEND_PER_START0);
	writel(0x01, hdmi_base + S5P_SEND_PER_START1);
	writel(0x0d, hdmi_base + S5P_SEND_PER_END0);
	writel(0x3a, hdmi_base + S5P_SEND_PER_END1);
	writel(0x08, hdmi_base + S5P_SEND_PER_END2);

	writel(SET_V_BOT_ST_L(g_hdmi_video_parm_tbl
		[V_BOT_ST*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_BLANK_F_0);
	writel(SET_V_BOT_ST_H(g_hdmi_video_parm_tbl
		[V_BOT_ST*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       SET_V_BOT_END_L(g_hdmi_video_parm_tbl
	       [V_BOT_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_BLANK_F_1);
	writel(SET_V_BOT_END_H(g_hdmi_video_parm_tbl
		[V_BOT_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_BLANK_F_2);


	writel(SET_HSYNC_START_L(g_hdmi_video_parm_tbl
		[HSYNC_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_H_SYNC_GEN_0);
	writel(SET_HSYNC_START_H(g_hdmi_video_parm_tbl
		[HSYNC_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       SET_HSYNC_END_L(g_hdmi_video_parm_tbl
	       [HSYNC_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_H_SYNC_GEN_1);
	writel(SET_HSYNC_END_H(g_hdmi_video_parm_tbl
		[HSYNC_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       ((g_hdmi_video_parm_tbl
	       [HSYNC_POL*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) ?
	       SET_HSYNC_POL_ACT_LOW : SET_HSYNC_POL_ACT_HIGH) ,
	       hdmi_base + S5P_H_SYNC_GEN_2);


	writel(SET_VSYNC_T_END_L(g_hdmi_video_parm_tbl
		[VSYNC_T_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_1_0);
	writel(SET_VSYNC_T_END_H(g_hdmi_video_parm_tbl
		[VSYNC_T_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       SET_VSYNC_T_ST_L(g_hdmi_video_parm_tbl
	       [VSYNC_T_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),

	       hdmi_base + S5P_V_SYNC_GEN_1_1);
	writel(SET_VSYNC_T_ST_H(g_hdmi_video_parm_tbl
		[VSYNC_T_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_1_2);

	writel(SET_VSYNC_B_END_L(g_hdmi_video_parm_tbl
		[VSYNC_B_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_2_0);
	writel(SET_VSYNC_B_END_H(g_hdmi_video_parm_tbl
		[VSYNC_B_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       SET_VSYNC_B_ST_L(g_hdmi_video_parm_tbl
	       [VSYNC_B_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_2_1);
	writel(SET_VSYNC_B_ST_H(g_hdmi_video_parm_tbl
		[VSYNC_B_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_2_2);

	writel(SET_VSYNC_H_POST_END_L(g_hdmi_video_parm_tbl
		[VSYNC_h_POS_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_3_0);

	writel(SET_VSYNC_H_POST_END_H(g_hdmi_video_parm_tbl
		[VSYNC_h_POS_END*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) |
	       SET_VSYNC_H_POST_ST_L(g_hdmi_video_parm_tbl
	       [VSYNC_h_POS_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_3_1);

	writel(SET_VSYNC_H_POST_ST_H(g_hdmi_video_parm_tbl
		[VSYNC_h_POS_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_V_SYNC_GEN_3_2);


	writel(0 , hdmi_base + S5P_TG_CMD);
	writel(SET_TG_H_FSZ_L(g_hdmi_video_parm_tbl
		[TG_H_FSZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) ,
	       hdmi_base + S5P_TG_H_FSZ_L);
	writel(SET_TG_H_FSZ_H(g_hdmi_video_parm_tbl
		[TG_H_FSZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) ,
	       hdmi_base + S5P_TG_H_FSZ_H);

	writel(SET_TG_HACT_ST_L(g_hdmi_video_parm_tbl
		[TG_HACT_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]) ,
	       hdmi_base + S5P_TG_HACT_ST_L);
	writel(SET_TG_HACT_ST_H(g_hdmi_video_parm_tbl
		[TG_HACT_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_HACT_ST_H);
	writel(SET_TG_HACT_SZ_L(g_hdmi_video_parm_tbl
		[TG_HACT_SZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_HACT_SZ_L);
	writel(SET_TG_HACT_SZ_H(g_hdmi_video_parm_tbl
		[TG_HACT_SZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_HACT_SZ_H);


	writel(SET_TG_V_FSZ_L(g_hdmi_video_parm_tbl
		[TG_V_FSZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_V_FSZ_L);
	writel(SET_TG_V_FSZ_H(g_hdmi_video_parm_tbl
		[TG_V_FSZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_V_FSZ_H);
	writel(SET_TG_VSYNC_L(g_hdmi_video_parm_tbl
		[TG_VSYNC*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC_L);
	writel(SET_TG_VSYNC_H(g_hdmi_video_parm_tbl
		[TG_VSYNC*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC_H);
	writel(SET_TG_VSYNC2_L(g_hdmi_video_parm_tbl
		[TG_VSYNC2*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC2_L);
	writel(SET_TG_VSYNC2_H(g_hdmi_video_parm_tbl
		[TG_VSYNC2*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC2_H);

	writel(SET_TG_VACT_ST_L(g_hdmi_video_parm_tbl
		[TG_VACT_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VACT_ST_L);
	writel(SET_TG_VACT_ST_H(g_hdmi_video_parm_tbl
		[TG_VACT_START*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VACT_ST_H);
	writel(SET_TG_VACT_SZ_L(g_hdmi_video_parm_tbl
		[TG_VACT_SZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VACT_SZ_L);
	writel(SET_TG_VACT_SZ_H(g_hdmi_video_parm_tbl
		[TG_VACT_SZ*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VACT_SZ_H);

	writel(SET_TG_FIELD_CHG_L(g_hdmi_video_parm_tbl
		[TG_FIELD_CHG*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_FIELD_CHG_L);
	writel(SET_TG_FIELD_CHG_H(g_hdmi_video_parm_tbl
		[TG_FIELD_CHG*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_FIELD_CHG_H);

	writel(SET_TG_VACT_ST2_L(g_hdmi_video_parm_tbl
		[TG_VACT_START2*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VACT_ST2_L);
	writel(SET_TG_VACT_ST2_H(g_hdmi_video_parm_tbl
		[TG_VACT_START2*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VACT_ST2_H);

	writel(SET_TG_VSYNC_TOP_HDMI_L(g_hdmi_video_parm_tbl
		[TG_VSYNC_TOP_HDMI*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC_TOP_HDMI_L);
	writel(SET_TG_VSYNC_TOP_HDMI_H(g_hdmi_video_parm_tbl
		[TG_VSYNC_TOP_HDMI*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC_TOP_HDMI_H);
	writel(SET_TG_VSYNC_BOT_HDMI_L(g_hdmi_video_parm_tbl
		[TG_VSYNC_BOTTOM_HDMI*S5P_TV_HDMI_DISP_MODE_NUM +
		hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC_BOT_HDMI_L);
	writel(SET_TG_VSYNC_BOT_HDMI_H(g_hdmi_video_parm_tbl
		[TG_VSYNC_BOTTOM_HDMI*S5P_TV_HDMI_DISP_MODE_NUM +
		hdmi_disp_num]),
	       hdmi_base + S5P_TG_VSYNC_BOT_HDMI_H);

	writel(SET_TG_FIELD_TOP_HDMI_L(g_hdmi_video_parm_tbl
		[TG_FIELD_TOP_HDMI*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_FIELD_TOP_HDMI_L);
	writel(SET_TG_FIELD_TOP_HDMI_H(g_hdmi_video_parm_tbl
		[TG_FIELD_TOP_HDMI*S5P_TV_HDMI_DISP_MODE_NUM + hdmi_disp_num]),
	       hdmi_base + S5P_TG_FIELD_TOP_HDMI_H);
	writel(SET_TG_FIELD_BOT_HDMI_L(g_hdmi_video_parm_tbl
		[TG_FIELD_BOTTOM_HDMI*S5P_TV_HDMI_DISP_MODE_NUM +
		hdmi_disp_num]),
	       hdmi_base + S5P_TG_FIELD_BOT_HDMI_L);
	writel(SET_TG_FIELD_BOT_HDMI_H(g_hdmi_video_parm_tbl
		[TG_FIELD_BOTTOM_HDMI*S5P_TV_HDMI_DISP_MODE_NUM +
		hdmi_disp_num]),
	       hdmi_base + S5P_TG_FIELD_BOT_HDMI_H);

	HDMIPRINTK("HDMI_CON_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_CON_1));
	HDMIPRINTK("HDMI_CON_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_CON_2));
	HDMIPRINTK("MODE_SEL = 0x%08x \n\r",
		readl(hdmi_base + S5P_MODE_SEL));
	HDMIPRINTK("BLUE_SCREEN_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_BLUE_SCREEN_0));
	HDMIPRINTK("BLUE_SCREEN_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_BLUE_SCREEN_1));
	HDMIPRINTK("BLUE_SCREEN_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_BLUE_SCREEN_2));
	HDMIPRINTK("VBI_ST_MG = 0x%08x \n\r",
		readl(hdmi_base + S5P_VBI_ST_MG));
	HDMIPRINTK("VBI_END_MG = 0x%08x \n\r",
		readl(hdmi_base + S5P_VBI_END_MG));
	HDMIPRINTK("VACT_ST_MG = 0x%08x \n\r",
		readl(hdmi_base + S5P_VACT_ST_MG));
	HDMIPRINTK("VACT_END_MG = 0x%08x \n\r",
		readl(hdmi_base + S5P_VACT_END_MG));
	HDMIPRINTK("H_BLANK_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_BLANK_0));
	HDMIPRINTK("H_BLANK_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_BLANK_1));
	HDMIPRINTK("V_BLANK_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_BLANK_0));
	HDMIPRINTK("V_BLANK_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_BLANK_1));
	HDMIPRINTK("V_BLANK_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_BLANK_2));
	HDMIPRINTK("H_V_LINE_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_V_LINE_0));
	HDMIPRINTK("H_V_LINE_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_V_LINE_1));
	HDMIPRINTK("H_V_LINE_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_V_LINE_2));
	HDMIPRINTK("SYNC_MODE = 0x%08x \n\r",
		readl(hdmi_base + S5P_SYNC_MODE));
	HDMIPRINTK("INT_PRO_MODE = 0x%08x \n\r",
		readl(hdmi_base + S5P_INT_PRO_MODE));
	HDMIPRINTK("SEND_PER_START0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SEND_PER_START0));
	HDMIPRINTK("SEND_PER_START1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SEND_PER_START1));
	HDMIPRINTK("SEND_PER_END0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SEND_PER_END0));
	HDMIPRINTK("SEND_PER_END1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SEND_PER_END1));
	HDMIPRINTK("SEND_PER_END2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SEND_PER_END2));
	HDMIPRINTK("V_BLANK_F_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_BLANK_F_0));
	HDMIPRINTK("V_BLANK_F_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_BLANK_F_1));
	HDMIPRINTK("V_BLANK_F_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_BLANK_F_2));
	HDMIPRINTK("H_SYNC_GEN_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_SYNC_GEN_0));
	HDMIPRINTK("H_SYNC_GEN_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_SYNC_GEN_1));
	HDMIPRINTK("H_SYNC_GEN_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_H_SYNC_GEN_2));
	HDMIPRINTK("V_SYNC_GEN_1_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_1_0));
	HDMIPRINTK("V_SYNC_GEN_1_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_1_1));
	HDMIPRINTK("V_SYNC_GEN_1_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_1_2));
	HDMIPRINTK("V_SYNC_GEN_2_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_2_0));
	HDMIPRINTK("V_SYNC_GEN_2_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_2_1));
	HDMIPRINTK("V_SYNC_GEN_2_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_2_2));
	HDMIPRINTK("V_SYNC_GEN_3_0 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_3_0));
	HDMIPRINTK("V_SYNC_GEN_3_1 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_3_1));
	HDMIPRINTK("V_SYNC_GEN_3_2 = 0x%08x \n\r",
		readl(hdmi_base + S5P_V_SYNC_GEN_3_2));
	HDMIPRINTK("TG_CMD = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_CMD));
	HDMIPRINTK("TG_H_FSZ_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_H_FSZ_L));
	HDMIPRINTK("TG_H_FSZ_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_H_FSZ_H));
	HDMIPRINTK("TG_HACT_ST_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_HACT_ST_L));
	HDMIPRINTK("TG_HACT_ST_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_HACT_ST_H));
	HDMIPRINTK("TG_HACT_SZ_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_HACT_SZ_L));
	HDMIPRINTK("TG_HACT_SZ_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_HACT_SZ_H));
	HDMIPRINTK("TG_V_FSZ_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_V_FSZ_L));
	HDMIPRINTK("TG_V_FSZ_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_V_FSZ_H));
	HDMIPRINTK("TG_VSYNC_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC_L));
	HDMIPRINTK("TG_VSYNC_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC_H));
	HDMIPRINTK("TG_VSYNC2_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC2_L));
	HDMIPRINTK("TG_VSYNC2_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC2_H));
	HDMIPRINTK("TG_VACT_ST_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VACT_ST_L));
	HDMIPRINTK("TG_VACT_ST_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VACT_ST_H));
	HDMIPRINTK("TG_VACT_SZ_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VACT_SZ_L));
	HDMIPRINTK("TG_VACT_SZ_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VACT_SZ_H));
	HDMIPRINTK("TG_FIELD_CHG_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_FIELD_CHG_L));
	HDMIPRINTK("TG_FIELD_CHG_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_FIELD_CHG_H));
	HDMIPRINTK("TG_VACT_ST2_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VACT_ST2_L));
	HDMIPRINTK("TG_VACT_ST2_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VACT_ST2_H));
	HDMIPRINTK("TG_VSYNC_TOP_HDMI_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC_TOP_HDMI_L));
	HDMIPRINTK("TG_VSYNC_TOP_HDMI_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC_TOP_HDMI_H));
	HDMIPRINTK("TG_VSYNC_BOT_HDMI_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC_BOT_HDMI_L));
	HDMIPRINTK("TG_VSYNC_BOT_HDMI_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_VSYNC_BOT_HDMI_H));
	HDMIPRINTK("TG_FIELD_TOP_HDMI_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_FIELD_TOP_HDMI_L));
	HDMIPRINTK("TG_FIELD_TOP_HDMI_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_FIELD_TOP_HDMI_H));
	HDMIPRINTK("TG_FIELD_BOT_HDMI_L = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_FIELD_BOT_HDMI_L));
	HDMIPRINTK("TG_FIELD_BOT_HDMI_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_TG_FIELD_BOT_HDMI_H));

	return HDMI_NO_ERROR;
}

void __s5p_hdmi_video_init_bluescreen(bool en,
				      u8 cb_b,
				      u8 y_g,
				      u8 cr_r)
{
	HDMIPRINTK("()\n\r");

	__s5p_hdmi_video_set_bluescreen(en, cb_b, y_g, cr_r);

	HDMIPRINTK("()\n\r");
}

void __s5p_hdmi_video_init_color_range(u8 y_min,
				       u8 y_max,
				       u8 c_min,
				       u8 c_max)
{
	HDMIPRINTK("%d,%d,%d,%d\n\r", y_max, y_min, c_max, c_min);

	writel(y_max, hdmi_base + S5P_HDMI_YMAX);
	writel(y_min, hdmi_base + S5P_HDMI_YMIN);
	writel(c_max, hdmi_base + S5P_HDMI_CMAX);
	writel(c_min, hdmi_base + S5P_HDMI_CMIN);

	HDMIPRINTK("HDMI_YMAX = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_YMAX));
	HDMIPRINTK("HDMI_YMIN = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_YMIN));
	HDMIPRINTK("HDMI_CMAX = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_CMAX));
	HDMIPRINTK("HDMI_CMIN = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_CMIN));
}

enum s5p_tv_hdmi_err __s5p_hdmi_video_init_csc(
	enum s5p_tv_hdmi_csc_type csc_type)
{
	unsigned short us_csc_coeff[10];

	HDMIPRINTK("%d)\n\r", csc_type);

	switch (csc_type) {

	case HDMI_CSC_YUV601_TO_RGB_LR:
		us_csc_coeff[0] = 0x23;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 938;
		us_csc_coeff[3] = 846;
		us_csc_coeff[4] = 256;
		us_csc_coeff[5] = 443;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 256;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 350;
		break;

	case HDMI_CSC_YUV601_TO_RGB_FR:
		us_csc_coeff[0] = 0x03;
		us_csc_coeff[1] = 298;
		us_csc_coeff[2] = 924;
		us_csc_coeff[3] = 816;
		us_csc_coeff[4] = 298;
		us_csc_coeff[5] = 516;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 298;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 408;
		break;

	case HDMI_CSC_YUV709_TO_RGB_LR:
		us_csc_coeff[0] = 0x23;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 978;
		us_csc_coeff[3] = 907;
		us_csc_coeff[4] = 256;
		us_csc_coeff[5] = 464;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 256;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 394;
		break;

	case HDMI_CSC_YUV709_TO_RGB_FR:
		us_csc_coeff[0] = 0x03;
		us_csc_coeff[1] = 298;
		us_csc_coeff[2] = 970;
		us_csc_coeff[3] = 888;
		us_csc_coeff[4] = 298;
		us_csc_coeff[5] = 540;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 298;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 458;
		break;

	case HDMI_CSC_YUV601_TO_YUV709:
		us_csc_coeff[0] = 0x33;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 995;
		us_csc_coeff[3] = 971;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 260;
		us_csc_coeff[6] = 29;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 19;
		us_csc_coeff[9] = 262;
		break;

	case HDMI_CSC_RGB_FR_TO_RGB_LR:
		us_csc_coeff[0] = 0x20;
		us_csc_coeff[1] = 220;
		us_csc_coeff[2] = 0;
		us_csc_coeff[3] = 0;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 220;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 220;
		break;

	case HDMI_CSC_RGB_FR_TO_YUV601:
		us_csc_coeff[0] = 0x30;
		us_csc_coeff[1] = 129;
		us_csc_coeff[2] = 25;
		us_csc_coeff[3] = 65;
		us_csc_coeff[4] = 950;
		us_csc_coeff[5] = 112;
		us_csc_coeff[6] = 986;
		us_csc_coeff[7] = 930;
		us_csc_coeff[8] = 1006;
		us_csc_coeff[9] = 112;
		break;

	case HDMI_CSC_RGB_FR_TO_YUV709:
		us_csc_coeff[0] = 0x30;
		us_csc_coeff[1] = 157;
		us_csc_coeff[2] = 16;
		us_csc_coeff[3] = 47;
		us_csc_coeff[4] = 937;
		us_csc_coeff[5] = 112;
		us_csc_coeff[6] = 999;
		us_csc_coeff[7] = 922;
		us_csc_coeff[8] = 1014;
		us_csc_coeff[9] = 112;
		break;

	case HDMI_BYPASS:
		us_csc_coeff[0] = 0x33;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 0;
		us_csc_coeff[3] = 0;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 256;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 256;
		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r", csc_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(us_csc_coeff[0], hdmi_base + S5P_HDMI_CSC_CON);;

	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[1]),
	       hdmi_base + S5P_HDMI_Y_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[1]),
	       hdmi_base + S5P_HDMI_Y_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[2]),
	       hdmi_base + S5P_HDMI_Y_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[2]),
	       hdmi_base + S5P_HDMI_Y_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[3]),
	       hdmi_base + S5P_HDMI_Y_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[3]),
	       hdmi_base + S5P_HDMI_Y_R_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[4]),
	       hdmi_base + S5P_HDMI_CB_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[4]),
	       hdmi_base + S5P_HDMI_CB_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[5]),
	       hdmi_base + S5P_HDMI_CB_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[5]),
	       hdmi_base + S5P_HDMI_CB_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[6]),
	       hdmi_base + S5P_HDMI_CB_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[6]),
	       hdmi_base + S5P_HDMI_CB_R_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[7]),
	       hdmi_base + S5P_HDMI_CR_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[7]),
	       hdmi_base + S5P_HDMI_CR_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[8]),
	       hdmi_base + S5P_HDMI_CR_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[8]),
	       hdmi_base + S5P_HDMI_CR_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[9]),
	       hdmi_base + S5P_HDMI_CR_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[9]),
	       hdmi_base + S5P_HDMI_CR_R_COEF_H);

	HDMIPRINTK("HDMI_CSC_CON = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CSC_CON));
	HDMIPRINTK("HDMI_Y_G_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_Y_G_COEF_L));
	HDMIPRINTK("HDMI_Y_G_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_Y_G_COEF_H));
	HDMIPRINTK("HDMI_Y_B_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_Y_B_COEF_L));
	HDMIPRINTK("HDMI_Y_B_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_Y_B_COEF_H));
	HDMIPRINTK("HDMI_Y_R_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_Y_R_COEF_L));
	HDMIPRINTK("HDMI_Y_R_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_Y_R_COEF_H));
	HDMIPRINTK("HDMI_CB_G_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CB_G_COEF_L));
	HDMIPRINTK("HDMI_CB_G_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CB_G_COEF_H));
	HDMIPRINTK("HDMI_CB_B_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CB_B_COEF_L));
	HDMIPRINTK("HDMI_CB_B_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CB_B_COEF_H));
	HDMIPRINTK("HDMI_CB_R_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CB_R_COEF_L));
	HDMIPRINTK("HDMI_CB_R_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CB_R_COEF_H));
	HDMIPRINTK("HDMI_CR_G_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CR_G_COEF_L));
	HDMIPRINTK("HDMI_CR_G_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CR_G_COEF_H));
	HDMIPRINTK("HDMI_CR_B_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CR_B_COEF_L));
	HDMIPRINTK("HDMI_CR_B_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CR_B_COEF_H));
	HDMIPRINTK("HDMI_CR_R_COEF_L = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CR_R_COEF_L));
	HDMIPRINTK("HDMI_CR_R_COEF_H = 0x%08x \n\r",
		   readl(hdmi_base + S5P_HDMI_CR_R_COEF_H));

	return HDMI_NO_ERROR;
}

enum s5p_tv_hdmi_err __s5p_hdmi_video_init_avi_infoframe(
	enum s5p_hdmi_transmit trans_type, u8 check_sum, u8 *avi_data)
{
	HDMIPRINTK("%d,%d,%d\n\r", (u32)trans_type, (u32)check_sum,
		(u32)avi_data);

	switch (trans_type) {

	case HDMI_DO_NOT_TANS:
		writel(AVI_TX_CON_NO_TRANS, hdmi_base + S5P_AVI_CON);
		break;

	case HDMI_TRANS_ONCE:
		writel(AVI_TX_CON_TRANS_ONCE, hdmi_base + S5P_AVI_CON);
		break;

	case HDMI_TRANS_EVERY_SYNC:
		writel(AVI_TX_CON_TRANS_EVERY_VSYNC, hdmi_base + S5P_AVI_CON);
		break;

	default:
		HDMIPRINTK(" invalid out_mode parameter(%d)\n\r", trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_AVI_CHECK_SUM(check_sum), hdmi_base + S5P_AVI_CHECK_SUM);

	writel(SET_AVI_BYTE(*(avi_data)), hdmi_base + S5P_AVI_BYTE1);
	writel(SET_AVI_BYTE(*(avi_data + 1)), hdmi_base + S5P_AVI_BYTE2);
	writel(SET_AVI_BYTE(*(avi_data + 2)), hdmi_base + S5P_AVI_BYTE3);
	writel(SET_AVI_BYTE(*(avi_data + 3)), hdmi_base + S5P_AVI_BYTE4);
	writel(SET_AVI_BYTE(*(avi_data + 4)), hdmi_base + S5P_AVI_BYTE5);
	writel(SET_AVI_BYTE(*(avi_data + 5)), hdmi_base + S5P_AVI_BYTE6);
	writel(SET_AVI_BYTE(*(avi_data + 6)), hdmi_base + S5P_AVI_BYTE7);
	writel(SET_AVI_BYTE(*(avi_data + 7)), hdmi_base + S5P_AVI_BYTE8);
	writel(SET_AVI_BYTE(*(avi_data + 8)), hdmi_base + S5P_AVI_BYTE9);
	writel(SET_AVI_BYTE(*(avi_data + 9)), hdmi_base + S5P_AVI_BYTE10);
	writel(SET_AVI_BYTE(*(avi_data + 10)), hdmi_base + S5P_AVI_BYTE11);
	writel(SET_AVI_BYTE(*(avi_data + 11)), hdmi_base + S5P_AVI_BYTE12);
	writel(SET_AVI_BYTE(*(avi_data + 12)), hdmi_base + S5P_AVI_BYTE13);

	HDMIPRINTK("AVI_CON = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_CON));
	HDMIPRINTK("AVI_CHECK_SUM = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_CHECK_SUM));
	HDMIPRINTK("AVI_BYTE1  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE1));
	HDMIPRINTK("AVI_BYTE2  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE2));
	HDMIPRINTK("AVI_BYTE3  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE3));
	HDMIPRINTK("AVI_BYTE4  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE4));
	HDMIPRINTK("AVI_BYTE5  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE5));
	HDMIPRINTK("AVI_BYTE6  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE6));
	HDMIPRINTK("AVI_BYTE7  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE7));
	HDMIPRINTK("AVI_BYTE8  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE8));
	HDMIPRINTK("AVI_BYTE9  = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE9));
	HDMIPRINTK("AVI_BYTE10 = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE10));
	HDMIPRINTK("AVI_BYTE11 = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE11));
	HDMIPRINTK("AVI_BYTE12 = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE12));
	HDMIPRINTK("AVI_BYTE13 = 0x%08x \n\r",
		readl(hdmi_base + S5P_AVI_BYTE13));

	/*
	* for test color bar
	*/
	/*
	{

		writel( 0xff&32,HDMI_TPGEN_1);
		writel( (32>>8)&0xff,HDMI_TPGEN_2);
		writel( 0xff&480,HDMI_TPGEN_3);
		writel( (480>>8)&0xff,HDMI_TPGEN_4);
		writel( 0xff&720,HDMI_TPGEN_5);
		writel( (720>>8)&0xff,HDMI_TPGEN_6);

		u8 uTpGen0Reg =	0x1 | (0x1<<3) | (1<<4);
		writel( uTpGen0Reg, HDMI_TPGEN_0);

		HDMIPRINTK("HDMI_TPGEN_0 0x%08x\n",readl(HDMI_TPGEN_0));
		HDMIPRINTK("HDMI_TPGEN_1 0x%08x\n",readl(HDMI_TPGEN_1));
		HDMIPRINTK("HDMI_TPGEN_2 0x%08x\n",readl(HDMI_TPGEN_2));
		HDMIPRINTK("HDMI_TPGEN_3 0x%08x\n",readl(HDMI_TPGEN_3));
		HDMIPRINTK("HDMI_TPGEN_5 0x%08x\n",readl(HDMI_TPGEN_4));
		HDMIPRINTK("HDMI_TPGEN_0 0x%08x\n",readl(HDMI_TPGEN_5));
		HDMIPRINTK("HDMI_TPGEN_1 0x%08x\n",readl(HDMI_TPGEN_6));
	}
	*/

	return HDMI_NO_ERROR;
}

enum s5p_tv_hdmi_err __s5p_hdmi_video_init_mpg_infoframe(
	enum s5p_hdmi_transmit trans_type, u8 check_sum, u8 *mpg_data)
{
	HDMIPRINTK("trans_type : %d,%d,%d\n\r", (u32)trans_type,
		   (u32)check_sum, (u32)mpg_data);

	switch (trans_type) {

	case HDMI_DO_NOT_TANS:
		writel(MPG_TX_CON_NO_TRANS,
		       hdmi_base + S5P_MPG_CON);
		break;

	case HDMI_TRANS_ONCE:
		writel(MPG_TX_CON_TRANS_ONCE,
		       hdmi_base + S5P_MPG_CON);
		break;

	case HDMI_TRANS_EVERY_SYNC:
		writel(MPG_TX_CON_TRANS_EVERY_VSYNC,
		       hdmi_base + S5P_MPG_CON);
		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r",
			   trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_MPG_CHECK_SUM(check_sum),

	       hdmi_base + S5P_MPG_CHECK_SUM);

	writel(SET_MPG_BYTE(*(mpg_data)),
	       hdmi_base + S5P_MPEG_BYTE1);
	writel(SET_MPG_BYTE(*(mpg_data + 1)),
	       hdmi_base + S5P_MPEG_BYTE2);
	writel(SET_MPG_BYTE(*(mpg_data + 2)),
	       hdmi_base + S5P_MPEG_BYTE3);
	writel(SET_MPG_BYTE(*(mpg_data + 3)),
	       hdmi_base + S5P_MPEG_BYTE4);
	writel(SET_MPG_BYTE(*(mpg_data + 4)),
	       hdmi_base + S5P_MPEG_BYTE5);

	HDMIPRINTK("MPG_CON = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPG_CON));
	HDMIPRINTK("MPG_CHECK_SUM = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPG_CHECK_SUM));
	HDMIPRINTK("MPEG_BYTE1 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE1));
	HDMIPRINTK("MPEG_BYTE2 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE2));
	HDMIPRINTK("MPEG_BYTE3 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE3));
	HDMIPRINTK("MPEG_BYTE4 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE4));
	HDMIPRINTK("MPEG_BYTE5 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE5));

	return HDMI_NO_ERROR;
}

void __s5p_hdmi_video_init_tg_cmd(bool time_c_e,
				  bool bt656_sync_en,
				  bool tg_en)
{
	u32 temp_reg = 0;

	temp_reg = readl(hdmi_base + S5P_TG_CMD);

	if (time_c_e)
		temp_reg |= GETSYNC_TYPE_EN;
	else
		temp_reg &= GETSYNC_TYPE_DIS;

	if (bt656_sync_en)
		temp_reg |= GETSYNC_EN;
	else
		temp_reg &= GETSYNC_DIS;

	if (tg_en)
		temp_reg |= TG_EN;
	else
		temp_reg &= TG_DIS;

	writel(temp_reg, hdmi_base + S5P_TG_CMD);

	HDMIPRINTK("TG_CMD = 0x%08x \n\r", readl(hdmi_base + S5P_TG_CMD));
}


/*
* start  - start functions are only called under stopping HDMI
*/
bool __s5p_hdmi_start(enum s5p_hdmi_audio_type hdmi_audio_type,
		      bool hdcp_en,
		      struct i2c_client *ddc_port)
{
	u32 temp_reg = PWDN_ENB_NORMAL | HDMI_EN;;

	HDMIPRINTK("aud type : %d, hdcp enable : %d\n\r",
		   hdmi_audio_type, hdcp_en);

	switch (hdmi_audio_type) {

	case HDMI_AUDIO_PCM:
		temp_reg |= ASP_EN;
		break;

	case HDMI_AUDIO_NO:
		break;

	default:
		HDMIPRINTK(" invalid hdmi_audio_type(%d)\n\r",
			   hdmi_audio_type);
		return false;
		break;
	}

	writel(readl(hdmi_base + S5P_HDMI_CON_0) | temp_reg,

	       hdmi_base + S5P_HDMI_CON_0);

	if (hdcp_en) {
		__s5p_init_hdcp(true, ddc_port);

		if (!__s5p_start_hdcp())
			HDMIPRINTK("HDCP start failed\n");

	}

	HDMIPRINTK("HDCP_CTRL : 0x%08x, HPD : 0x%08x, HDMI_CON_0 :\
		0x%08x\n\r",

		   readl(hdmi_base + S5P_HDCP_CTRL),
		   readl(hdmi_base + S5P_HPD),
		   readl(hdmi_base + S5P_HDMI_CON_0));

	return true;
}



/*
* stop  - stop functions are only called under running HDMI
*/
void __s5p_hdmi_stop(void)
{
	HDMIPRINTK("\n\r");

	__s5p_stop_hdcp();

	writel(readl(hdmi_base + S5P_HDMI_CON_0) &
	       ~(PWDN_ENB_NORMAL | HDMI_EN | ASP_EN),
	       hdmi_base + S5P_HDMI_CON_0);

	HDMIPRINTK("HDCP_CTRL 0x%08x, HPD 0x%08x,HDMI_CON_0 0x%08x\n\r",
		   readl(hdmi_base + S5P_HDCP_CTRL),
		   readl(hdmi_base + S5P_HPD),
		   readl(hdmi_base + S5P_HDMI_CON_0));
}

int __init __s5p_hdmi_probe(struct platform_device *pdev, u32 res_num)
{

	struct resource *res;
	size_t	size;
	int 	ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, res_num);

	if (res == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region resource\n");
		ret = -ENOENT;
	}

	size = (res->end - res->start) + 1;

	hdmi_mem = request_mem_region(res->start, size, pdev->name);

	if (hdmi_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		ret = -ENOENT;
	}

	hdmi_base = ioremap(res->start, size);

	if (hdmi_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		ret = -ENOENT;
	}

	return ret;

}

int __init __s5p_hdmi_release(struct platform_device *pdev)
{
	iounmap(hdmi_base);

	/* remove memory region */

	if (hdmi_mem != NULL) {
		if (release_resource(hdmi_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(hdmi_mem);

		hdmi_mem = NULL;
	}

	return 0;
}

