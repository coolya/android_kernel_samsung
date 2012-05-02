/* linux/drivers/media/video/samsung/tv20/s5p_stda_hdmi.c
 *
 * HDMI ftn. file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "s5p_tv.h"

#include <mach/map.h>
#include <mach/regs-clock.h>

#ifdef COFIG_TVOUT_DBG
#define S5P_STDA_HDMI_DEBUG 1
#endif

#ifdef S5P_STDA_HDMI_DEBUG
#define STHDPRINTK(fmt, args...) \
	printk(KERN_INFO "\t[STDA_HDMI] %s: " fmt, __func__ , ## args)
#else
#define STHDPRINTK(fmt, args...)
#endif

/*
static bool _s5p_hdmi_spd_infoframe(unsigned long p_buf_in)
{
	STHDPRINTK("(0x%x)\n\r", (unsigned int)p_buf_in);

	if (!p_buf_in)
	{
		STHDPRINTK("(ERR) p_buf_in is NULL\n\r");
		return false;
	}

	memcpy((void *)(&(s5ptv_status.hdmi_spd_info_frame)),
		(const void *)p_buf_in, sizeof(s5p_hdmi_spd_infoframe));

	memcpy((void *)(s5ptv_status.spd_header),
	(const void *)(s5ptv_status.hdmi_spd_info_frame.spd_header), 3);
	s5ptv_status.hdmi_spd_info_frame.spd_header = s5ptv_status.spd_header;

	memcpy((void *)(s5ptv_status.spd_data),
		(const void *)(s5ptv_status.hdmi_spd_info_frame.spd_data), 28);
	s5ptv_status.hdmi_spd_info_frame.spd_data = s5ptv_status.spd_data;

	STHDPRINTK("(0x%08x)\n\r",
		(unsigned int)&s5ptv_status.hdmi_spd_info_frame);

	return true;
}
*/

#if 0
static bool _s5p_hdmi_init_hdcp_en(unsigned long p_buf_in)
{
	if (!p_buf_in) {
		STHDPRINTK("(ERR) p_buf_in is NULL\n\r");
		return false;
	}

	STHDPRINTK("(%d)\n\r", (bool)p_buf_in);

	s5ptv_status.hdcp_en = (bool)p_buf_in;

	STHDPRINTK("(%d)\n\r", s5ptv_status.hdcp_en);

	return true;
}

static bool _s5p_hdmi_init_audio(unsigned long p_buf_in)
{
	STHDPRINTK("(%d)\n\r", (bool)p_buf_in);

	if (!p_buf_in) {
		STHDPRINTK("(ERR) p_buf_in is NULL\n\r");
		return false;
	}

	s5ptv_status.hdmi_audio_type = (enum s5p_hdmi_audio_type)p_buf_in;

	STHDPRINTK("(%d)\n\r", s5ptv_status.hdmi_audio_type);

	return true;
}

static bool _s5p_hdmi_get_hpd_status(unsigned long p_buf_out)
{
	bool *pOut;

	STHDPRINTK("()\n\r");

	if (!p_buf_out) {
		STHDPRINTK("(ERR) p_buf_out is NULL\n\r");
		return false;
	}

	pOut = (bool *)p_buf_out;

	*pOut = s5ptv_status.hpd_status;

	STHDPRINTK("()\n\r");

	return true;
}

static bool _s5p_hdmi_wait_hpd_status_change(unsigned long p_buf_out)
{
	unsigned int *pOut;

	STHDPRINTK("()\n\r");

	if (!p_buf_out) {
		STHDPRINTK("(ERR) p_buf_out is NULL\n\r");
		return false;
	}

	pOut = (unsigned int *)p_buf_out;


	/*
		if (*pOut == WAIT_TIMEOUT)
		{
			STHDPRINTK("(ERR) TIMEOUT~\n\r");
		}
		else if (*pOut == WAIT_FAILED)
		{
			STHDPRINTK("(ERR) WAIT_FAILED\n\r");
		}
	*/

	return true;
}

#endif

/*============================================================================*/

/*
static bool _s5p_hdmi_video_init_bluescreen(unsigned long p_buf_in)
{

	STHDPRINTK("(0x%x)\n\r", (unsigned int)p_buf_in);

	if (!p_buf_in)
	{
		STHDPRINTK("(ERR) p_buf_in is NULL\n\r");
		return false;
	}

	memcpy((void *)(&(s5ptv_status.hdmi_video_blue_screen)),
		(const void *)p_buf_in, sizeof(s5p_hdmi_bluescreen));

	STHDPRINTK("(0x%08x)\n\r",
		(unsigned int)&s5ptv_status.hdmi_video_blue_screen);

	return true;
}

static bool _s5p_hdmi_video_init_avi_infoframe(unsigned long p_buf_in)
{
	STHDPRINTK("(0x%x)\n\r", (unsigned int)p_buf_in);

	if (!p_buf_in)
	{
		STHDPRINTK("(ERR) p_buf_in is NULL\n\r");
		return false;
	}

	memcpy((void *)(&(s5ptv_status.hdmi_av_info_frame)),
		(const void *)p_buf_in, sizeof(s5p_hdmi_video_infoframe));

	memcpy((void *)(s5ptv_status.avi_byte),
		(const void *)(s5ptv_status.hdmi_av_info_frame.data), 13);
	s5ptv_status.hdmi_av_info_frame.data = s5ptv_status.avi_byte;

	STHDPRINTK("(0x%08x)\n\r",
		(unsigned int)&s5ptv_status.hdmi_av_info_frame);

	return true;
}

static bool _s5p_hdmi_video_init_mpg_infoframe(unsigned long p_buf_in)
{
	STHDPRINTK("(0x%x)\n\r", (unsigned int)p_buf_in);

	if (!p_buf_in)
	{
		STHDPRINTK("(ERR) p_buf_in is NULL\n\r");
		return false;
	}

	memcpy((void *)(&(s5ptv_status.hdmi_mpg_info_frame)),
		(const void *)p_buf_in, sizeof(s5p_hdmi_video_infoframe));

	memcpy((void *)(s5ptv_status.mpg_byte),
		(const void *)(s5ptv_status.hdmi_mpg_info_frame.data), 5);
	s5ptv_status.hdmi_mpg_info_frame.data = s5ptv_status.avi_byte;

	STHDPRINTK("(0x%08x)\n\r",
		(unsigned int)&s5ptv_status.hdmi_mpg_info_frame);

	return true;
}

static bool _s5p_hdmi_video_set_bluescreen(unsigned long p_buf_in)
{
	STHDPRINTK("(0x%x)\n\r", (unsigned int)p_buf_in);

	if (!p_buf_in)
	{
		STHDPRINTK("(ERR) p_buf_in is NULL\n\r");
		return false;
	}

	memcpy((void *)(&(s5ptv_status.hdmi_video_blue_screen)),
		(const void *)p_buf_in, sizeof(s5p_hdmi_bluescreen));

	__s5p_hdmi_video_set_bluescreen(s5ptv_status.hdmi_video_blue_screen.en,
		s5ptv_status.hdmi_video_blue_screen.cb_b,
		s5ptv_status.hdmi_video_blue_screen.y_g,
		s5ptv_status.hdmi_video_blue_screen.cr_r);

	STHDPRINTK("(0x%08x)\n\r",
		(unsigned int)&s5ptv_status.hdmi_video_blue_screen);

	return true;
}

*/

