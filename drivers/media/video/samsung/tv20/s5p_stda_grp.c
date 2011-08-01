/* linux/drivers/media/video/samsung/tv20/s5p_stda_grp.c
 *
 * Graphic Layer ftn. file for Samsung TVOut driver
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
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include "s5p_tv.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_GRP_DEBUG 1
#endif

#ifdef S5P_GRP_DEBUG
#define GRPPRINTK(fmt, args...)	\
	printk(KERN_INFO "\t[GRP] %s: " fmt, __func__ , ## args)
#else
#define GRPPRINTK(fmt, args...)
#endif


bool _s5p_grp_start(enum s5p_tv_vmx_layer vm_layer)
{
	enum s5p_tv_vmx_err merr;
	struct s5p_tv_status *st = &s5ptv_status;

	if (!(st->grp_layer_enable[0] || st->grp_layer_enable[1])) {

		merr = __s5p_vm_init_status_reg(st->grp_burst,
					st->grp_endian);

		if (merr != VMIXER_NO_ERROR)
			return false;
	}

#ifdef CONFIG_CPU_S5PC100
	merr = __s5p_vm_init_layer(vm_layer,
			  true,
			  s5ptv_overlay[vm_layer].win_blending,
			  s5ptv_overlay[vm_layer].win.global_alpha,
			  s5ptv_overlay[vm_layer].priority,
			  s5ptv_overlay[vm_layer].fb.fmt.pixelformat,
			  s5ptv_overlay[vm_layer].blank_change,
			  s5ptv_overlay[vm_layer].pixel_blending,
			  s5ptv_overlay[vm_layer].pre_mul,
			  s5ptv_overlay[vm_layer].blank_color,
			  s5ptv_overlay[vm_layer].base_addr,
			  s5ptv_overlay[vm_layer].fb.fmt.bytesperline,
			  s5ptv_overlay[vm_layer].win.w.width,
			  s5ptv_overlay[vm_layer].win.w.height,
			  s5ptv_overlay[vm_layer].win.w.left,
			  s5ptv_overlay[vm_layer].win.w.top,
			  s5ptv_overlay[vm_layer].dst_rect.left,
			  s5ptv_overlay[vm_layer].dst_rect.top);
#endif

#ifdef CONFIG_CPU_S5PV210
	merr = __s5p_vm_init_layer(s5ptv_status.tvout_param.disp_mode,
			  vm_layer,
			  true,
			  s5ptv_overlay[vm_layer].win_blending,
			  s5ptv_overlay[vm_layer].win.global_alpha,
			  s5ptv_overlay[vm_layer].priority,
			  s5ptv_overlay[vm_layer].fb.fmt.pixelformat,
			  s5ptv_overlay[vm_layer].blank_change,
			  s5ptv_overlay[vm_layer].pixel_blending,
			  s5ptv_overlay[vm_layer].pre_mul,
			  s5ptv_overlay[vm_layer].blank_color,
			  s5ptv_overlay[vm_layer].base_addr,
			  s5ptv_overlay[vm_layer].fb.fmt.bytesperline,
			  s5ptv_overlay[vm_layer].win.w.width,
			  s5ptv_overlay[vm_layer].win.w.height,
			  s5ptv_overlay[vm_layer].win.w.left,
			  s5ptv_overlay[vm_layer].win.w.top,
			  s5ptv_overlay[vm_layer].dst_rect.left,
			  s5ptv_overlay[vm_layer].dst_rect.top,
			  s5ptv_overlay[vm_layer].dst_rect.width,
			  s5ptv_overlay[vm_layer].dst_rect.height);
#endif


	if (merr != VMIXER_NO_ERROR) {
		GRPPRINTK("can't initialize layer(%d)\n\r", merr);
		return false;
	}

	__s5p_vm_start();


	st->grp_layer_enable[vm_layer] = true;

	GRPPRINTK("()\n\r");

	return true;
}

bool _s5p_grp_stop(enum s5p_tv_vmx_layer vm_layer)
{
	enum s5p_tv_vmx_err merr;
	struct s5p_tv_status *st = &s5ptv_status;

	GRPPRINTK("()\n\r");

	merr = __s5p_vm_set_layer_show(vm_layer, false);

	if (merr != VMIXER_NO_ERROR)
		return false;

	merr = __s5p_vm_set_layer_priority(vm_layer, 0);

	if (merr != VMIXER_NO_ERROR)
		return false;

	__s5p_vm_start();


	st->grp_layer_enable[vm_layer] = false;

	GRPPRINTK("()\n\r");

	return true;
}

int s5ptvfb_set_output(struct s5p_tv_status *ctrl) { return 0; }

int s5ptvfb_set_display_mode(struct s5p_tv_status *ctrl)
{
	enum s5p_tv_vmx_layer layer = VM_GPR0_LAYER;
	bool premul = false;
	bool pixel_blending = false;
	bool blank_change = false;
	bool win_blending = false;
	u32 blank_color = 0x0;
	enum s5p_tv_vmx_color_fmt color;
	u32 bpp;
	u32 alpha = 0;

	bpp = ((struct fb_var_screeninfo)(ctrl->fb->var)).bits_per_pixel;

	if  (bpp == 32)
		color = VM_DIRECT_RGB8888;
	else
		color = VM_DIRECT_RGB565;

	__s5p_vm_set_ctrl(layer, premul, pixel_blending, blank_change,
		win_blending, color, alpha, blank_color);

	return 0;
}

int s5ptvfb_display_on(struct s5p_tv_status *ctrl)
{
	__s5p_vm_set_layer_priority(VM_GPR0_LAYER, 10);
	__s5p_vm_set_layer_show(VM_GPR0_LAYER, true);

	return 0;
}

int s5ptvfb_display_off(struct s5p_tv_status *ctrl)
{
	__s5p_vm_set_layer_priority(VM_GPR0_LAYER, 10);
	__s5p_vm_set_layer_show(VM_GPR0_LAYER, false);

	return 0;
}

int s5ptvfb_frame_off(struct s5p_tv_status *ctrl) { return 0; }
int s5ptvfb_set_clock(struct s5p_tv_status *ctrl) { return 0; }
int s5ptvfb_set_polarity(struct s5p_tv_status *ctrl) { return 0; }
int s5ptvfb_set_timing(struct s5p_tv_status *ctrl) { return 0; }
int s5ptvfb_set_lcd_size(struct s5p_tv_status *ctrl) { return 0; }
int s5ptvfb_window_on(struct s5p_tv_status *ctrl, int id)
{
	__s5p_vm_set_layer_show(VM_GPR0_LAYER, true);

	return 0;
}

int s5ptvfb_window_off(struct s5p_tv_status *ctrl, int id)
{
	__s5p_vm_set_layer_show(VM_GPR0_LAYER, false);
	return 0;
}

int s5ptvfb_set_window_control(struct s5p_tv_status *ctrl, int id) { return 0; }
int s5ptvfb_set_alpha_blending(struct s5p_tv_status *ctrl, int id) { return 0; }
int s5ptvfb_set_window_position(struct s5p_tv_status *ctrl, int id)
{
	u32 off_x, off_y;
	u32 w_t, h_t;
	u32 w, h;

	struct fb_var_screeninfo *var = &ctrl->fb->var;
	struct s5ptvfb_window *win = ctrl->fb->par;

	off_x = (u32)win->x;
	off_y = (u32)win->y;

	w = var->xres;
	h = var->yres;

	/*
	 * When tvout resolution was overscanned, there is no
	 * adjust method in H/W. So, framebuffer should be resized.
	 * In this case - TV w/h is greater than FB w/h, grp layer's
	 * dst offset must be changed to fix tv screen.
	 */

	switch (ctrl->tvout_param.disp_mode) {

	case TVOUT_NTSC_M:
	case TVOUT_480P_60_16_9:
	case TVOUT_480P_60_4_3:
	case TVOUT_480P_59:
		w_t = 720;
		h_t = 480;
		break;

	case TVOUT_576P_50_16_9:
	case TVOUT_576P_50_4_3:
		w_t = 720;
		h_t = 576;
		break;

	case TVOUT_720P_60:
	case TVOUT_720P_59:
	case TVOUT_720P_50:
		w_t = 1280;
		h_t = 720;
		break;

	case TVOUT_1080I_60:
	case TVOUT_1080I_59:
	case TVOUT_1080I_50:
	case TVOUT_1080P_60:
	case TVOUT_1080P_59:
	case TVOUT_1080P_50:
	case TVOUT_1080P_30:
		w_t = 1920;
		h_t = 1080;
		break;

	default:
		w_t = 0;
		h_t = 0;
		break;
	}

	if (w_t > w)
		off_x = (w_t - w) / 2;

	if (h_t > h)
		off_y = (h_t - h) / 2;

	__s5p_vm_set_grp_layer_position(VM_GPR0_LAYER, off_x, off_y);

	return 0;
}

int s5ptvfb_set_window_size(struct s5p_tv_status *ctrl, int id)
{
	struct fb_var_screeninfo *var = &ctrl->fb->var;
	int w, h, xo, yo;

	w = var->xres;
	h = var->yres;
	xo = var->xoffset;
	yo = var->yoffset;

	__s5p_vm_set_grp_layer_size(VM_GPR0_LAYER, w, w, h, xo, yo);


	dev_dbg(ctrl->dev_fb, "[fb%d] resolution: %d x %d\n", id,
		var->xres, var->yres);

	return 0;
}
int s5ptvfb_set_buffer_address(struct s5p_tv_status *ctrl, int id)
{
	struct fb_fix_screeninfo *fix = &ctrl->fb->fix;
	struct fb_var_screeninfo *var = &ctrl->fb->var;
	dma_addr_t start_addr = 0, end_addr = 0;

	if (fix->smem_start) {
		start_addr = fix->smem_start + (var->xres_virtual *
				(var->bits_per_pixel / 8) * var->yoffset);

		end_addr = start_addr + (var->xres_virtual *
				(var->bits_per_pixel / 8) * var->yres);
	}

	__s5p_vm_set_grp_base_address(VM_GPR0_LAYER, start_addr);
	return 0;
}

int s5ptvfb_set_buffer_size(struct s5p_tv_status *ctrl, int id) { return 0; }

int s5ptvfb_set_chroma_key(struct s5p_tv_status *ctrl, int id)
{
	struct s5ptvfb_window *win = ctrl->fb->par;
	struct s5ptvfb_chroma *chroma = &win->chroma;

	enum s5p_tv_vmx_layer layer = VM_GPR0_LAYER;

	bool blank_change = (chroma->enabled) ? true : false;
	u32 blank_color = chroma->key;

	bool win_blending = (chroma->blended) ? true : false;
	bool alpha = chroma->alpha;

	enum s5p_tv_vmx_color_fmt color = VM_DIRECT_RGB8888;

	__s5p_vm_set_ctrl(layer, false, false, blank_change,
		win_blending, color, alpha, blank_color);

	return 0;
}




static inline unsigned int __chan_to_field(unsigned int chan,
						struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;

	return chan << bf.offset;
}

static int s5ptvfb_set_alpha_info(struct fb_var_screeninfo *var,
				struct s5ptvfb_window *win)
{
	if (var->transp.length > 0)
		win->alpha.mode = PIXEL_BLENDING;
	else {
		win->alpha.mode = PLANE_BLENDING;
		win->alpha.channel = 0;
		win->alpha.value = S5PTVFB_AVALUE(0xf, 0xf, 0xf);
	}

	return 0;
}


static int s5ptvfb_enable_window(int id)
{
	struct s5ptvfb_window *win = s5ptv_status.fb->par;

	if (s5ptvfb_window_on(&s5ptv_status, id)) {
		win->enabled = 0;
		return -EFAULT;
	} else {
		win->enabled = 1;
		return 0;
	}
}


static int s5ptvfb_disable_window(int id)
{
	struct s5ptvfb_window *win = s5ptv_status.fb->par;

	if (s5ptvfb_window_off(&s5ptv_status, id)) {
		win->enabled = 1;
		return -EFAULT;
	} else {
		win->enabled = 0;
		return 0;
	}
}

int s5ptvfb_unmap_video_memory(struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s5ptvfb_window *win = fb->par;

	if (fix->smem_start) {
		dma_free_writecombine(s5ptv_status.dev_fb, fix->smem_len,
			fb->screen_base, fix->smem_start);
		fix->smem_start = 0;
		fix->smem_len = 0;
		dev_info(s5ptv_status.dev_fb,
			"[fb%d] video memory released\n", win->id);
	}

	return 0;
}

static int s5ptvfb_release_window(struct fb_info *fb)
{
	struct s5ptvfb_window *win = fb->par;

	win->x = 0;
	win->y = 0;

	return 0;
}

int s5ptvfb_map_video_memory(struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s5ptvfb_window *win = fb->par;

	if (win->path == DATA_PATH_FIFO)
		return 0;

	fb->screen_base = dma_alloc_writecombine(s5ptv_status.dev_fb,
				PAGE_ALIGN(fix->smem_len),
				(unsigned int *) &fix->smem_start, GFP_KERNEL);
	if (!fb->screen_base)
		return -ENOMEM;
	else
		dev_info(s5ptv_status.dev_fb,
			"[fb%d] dma: 0x%08x, cpu: 0x%08x,size: 0x%08x\n",
			win->id, (unsigned int) fix->smem_start,
				(unsigned int) fb->screen_base,
				fix->smem_len);

	memset(fb->screen_base, 0, fix->smem_len);

	return 0;
}


static int s5ptvfb_set_bitfield(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel) {
	case 16:
		if (var->transp.length == 1) {
			var->red.offset = 10;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 15;
		} else if (var->transp.length == 4) {
			var->red.offset = 8;
			var->red.length = 4;
			var->green.offset = 4;
			var->green.length = 4;
			var->blue.offset = 0;
			var->blue.length = 4;
			var->transp.offset = 12;
		} else {
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 0;
		}
		break;

	case 24:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;

	case 32:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		break;
	}

	return 0;
}

#define TV_LOGO_W	800
#define TV_LOGO_H	480

int s5ptvfb_draw_logo(struct fb_info *fb)
{
#if 0
	struct fb_fix_screeninfo *fix = &fb->fix;
	int i;

/*	memcpy(s5ptv_status.fb->screen_base,
		TV_LOGO_RGB24, fix->line_length * var->yres);
	*/
	char *base = s5ptv_status.fb->screen_base;

	for (i = 0; i < TV_LOGO_H; i++) {
		memcpy(base, &TV_LOGO_RGB24[i*TV_LOGO_W], TV_LOGO_W * 4);
		base += fix->line_length;
	}
#endif
	return 0;
}

static int s5ptvfb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	/* nothing to do for removing cursor */
	return 0;
}


static int s5ptvfb_setcolreg(unsigned int regno, unsigned int red,
				unsigned int green, unsigned int blue,
				unsigned int transp, struct fb_info *fb)
{
	unsigned int *pal = (unsigned int *) fb->pseudo_palette;
	unsigned int val = 0;

	if (regno < 16) {
		/* fake palette of 16 colors */
		val |= __chan_to_field(red, fb->var.red);
		val |= __chan_to_field(green, fb->var.green);
		val |= __chan_to_field(blue, fb->var.blue);
		val |= __chan_to_field(transp, fb->var.transp);

		pal[regno] = val;
	}

	return 0;
}


static int s5ptvfb_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *fb)
{
	struct s5ptvfb_window *win = fb->par;

	if (var->yoffset + var->yres > var->yres_virtual) {
		dev_err(s5ptv_status.dev_fb, "invalid yoffset value\n");
		return -EINVAL;
	}

	fb->var.yoffset = var->yoffset;

	dev_dbg(s5ptv_status.dev_fb, "[fb%d] yoffset for pan display: %d\n",
		win->id,
		var->yoffset);

	s5ptvfb_set_buffer_address(&s5ptv_status, win->id);

	return 0;
}


static int s5ptvfb_blank(int blank_mode, struct fb_info *fb)
{
	struct s5ptvfb_window *win = fb->par;

	dev_dbg(s5ptv_status.dev_fb, "change blank mode\n");

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (fb->fix.smem_start) {
			s5ptvfb_display_on(&s5ptv_status);
			s5ptvfb_enable_window(win->id);
		} else
			dev_info(s5ptv_status.dev_fb,
				"[fb%d] no allocated memory for unblank\n",
				win->id);
		break;

	case FB_BLANK_POWERDOWN:
		s5ptvfb_display_off(&s5ptv_status);
		s5ptvfb_disable_window(win->id);
		break;

	default:
		dev_dbg(s5ptv_status.dev_fb, "unsupported blank mode\n");
		/* return -EINVAL; */
	}

	return 0;
}

int s5ptvfb_set_par(struct fb_info *fb)
{
	struct s5ptvfb_window *win = fb->par;

	dev_dbg(s5ptv_status.dev_fb, "[fb%d] set_par\n", win->id);

	if (!fb->fix.smem_start) {
#ifndef CONFIG_USER_ALLOC_TVOUT
		GRPPRINTK(" The frame buffer is allocated here\n");
		s5ptvfb_map_video_memory(fb);
#else
		printk(KERN_ERR
		"[Warning] The frame buffer should be allocated by ioctl\n");
#endif
	}

	((struct fb_var_screeninfo) (s5ptv_status.fb->var)).bits_per_pixel =
		((struct fb_var_screeninfo) (fb->var)).bits_per_pixel;

	s5ptvfb_set_display_mode(&s5ptv_status);

	s5ptvfb_set_window_control(&s5ptv_status, win->id);
	s5ptvfb_set_window_position(&s5ptv_status, win->id);
	s5ptvfb_set_window_size(&s5ptv_status, win->id);
	s5ptvfb_set_buffer_address(&s5ptv_status, win->id);
	s5ptvfb_set_buffer_size(&s5ptv_status, win->id);

	if (win->id > 0)
		s5ptvfb_set_alpha_blending(&s5ptv_status, win->id);

	return 0;
}

int s5ptvfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s5ptvfb_window *win = fb->par;
	struct s5ptvfb_lcd *lcd = s5ptv_status.lcd;

	dev_dbg(s5ptv_status.dev_fb, "[fb%d] check_var\n", win->id);

	if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
		var->bits_per_pixel != 32) {
		dev_err(s5ptv_status.dev_fb, "invalid bits per pixel\n");
		return -EINVAL;
	}

	if (var->xres > lcd->width)
		var->xres = lcd->width;

	if (var->yres > lcd->height)
		var->yres = lcd->height;

	if (var->xres_virtual != var->xres)
		var->xres_virtual = var->xres;

	if (var->yres_virtual > var->yres * (fb->fix.ypanstep + 1))
		var->yres_virtual = var->yres * (fb->fix.ypanstep + 1);

	if (var->xoffset != 0)
		var->xoffset = 0;

	if (var->yoffset + var->yres > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (win->x + var->xres > lcd->width)
		win->x = lcd->width - var->xres;

	if (win->y + var->yres > lcd->height)
		win->y = lcd->height - var->yres;

	/* modify the fix info */
	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;


	s5ptvfb_set_bitfield(var);
	s5ptvfb_set_alpha_info(var, win);

	return 0;
}

static int s5ptvfb_release(struct fb_info *fb, int user)
{
/*
 * Following block is deleted for enabling multiple open of TV frame buffer
 *
 *	struct s5ptvfb_window *win = fb->par;
 */
	int ret;
	struct s5ptvfb_window *win = fb->par;

	s5ptvfb_release_window(fb);

/*
 * Following block is deleted for enabling multiple open of TV frame buffer
 *
 *	mutex_lock(&s5ptv_status.fb_lock);
 *	atomic_dec(&win->in_use);
 *	mutex_unlock(&s5ptv_status.fb_lock);
 */

	_s5p_vlayer_stop();
	_s5p_tv_if_stop();

	s5ptv_status.hdcp_en = false;

	s5ptv_status.tvout_output_enable = false;

	/*
	* drv. release
	*        - just check drv. state reg. or not.
	*/

	ret = s5p_tv_clk_gate(false);
	if (ret < 0) {
		printk(KERN_ERR "[Error]Cannot release\n");
		return -1;
	}
	tv_phy_power(false);

	mutex_lock(&s5ptv_status.fb_lock);
	atomic_dec(&win->in_use);
	mutex_unlock(&s5ptv_status.fb_lock);

	return 0;
}

static int s5ptvfb_ioctl(struct fb_info *fb, unsigned int cmd,
			unsigned long arg)
{
	struct fb_var_screeninfo *var = &fb->var;
	struct s5ptvfb_window *win = fb->par;
	struct s5ptvfb_lcd *lcd = s5ptv_status.lcd;
	int ret = 0;
	void *argp = (void *) arg;

	union {
		struct s5ptvfb_user_window user_window;
		struct s5ptvfb_user_plane_alpha user_alpha;
		struct s5ptvfb_user_chroma user_chroma;
		int vsync;
	} p;

	switch (cmd) {

	case FBIO_ALLOC:
		win->path = (enum s5ptvfb_data_path_t) argp;
		break;

	case FBIOGET_FSCREENINFO:
		ret = memcpy(argp, &fb->fix, sizeof(fb->fix)) ? 0 : -EFAULT;
		break;

	case FBIOGET_VSCREENINFO:
		ret = memcpy(argp, &fb->var, sizeof(fb->var)) ? 0 : -EFAULT;
		break;

	case FBIOPUT_VSCREENINFO:
		ret = s5ptvfb_check_var((struct fb_var_screeninfo *) argp, fb);
		if (ret) {
			dev_err(s5ptv_status.dev_fb, "invalid vscreeninfo\n");
			break;
		}

		ret = memcpy(&fb->var, (struct fb_var_screeninfo *) argp,
				sizeof(fb->var)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(s5ptv_status.dev_fb,
				"failed to put new vscreeninfo\n");
			break;
		}

		ret = s5ptvfb_set_par(fb);
		break;

	case S5PTVFB_WIN_POSITION:
		if (copy_from_user(&p.user_window,
			(struct s5ptvfb_user_window __user *) arg,
			sizeof(p.user_window)))
			ret = -EFAULT;
		else {
			if (p.user_window.x < 0)
				p.user_window.x = 0;

			if (p.user_window.y < 0)
				p.user_window.y = 0;

			if (p.user_window.x + var->xres > lcd->width)
				win->x = lcd->width - var->xres;
			else
				win->x = p.user_window.x;

			if (p.user_window.y + var->yres > lcd->height)
				win->y = lcd->height - var->yres;
			else
				win->y = p.user_window.y;

			s5ptvfb_set_window_position(&s5ptv_status, win->id);
		}
		break;

	case S5PTVFB_WIN_SET_PLANE_ALPHA:
		if (copy_from_user(&p.user_alpha,
			(struct s5ptvfb_user_plane_alpha __user *) arg,
			sizeof(p.user_alpha)))
			ret = -EFAULT;
		else {
			win->alpha.mode = PLANE_BLENDING;
			win->alpha.channel = p.user_alpha.channel;
			win->alpha.value =
				S5PTVFB_AVALUE(p.user_alpha.red,
					p.user_alpha.green,
					p.user_alpha.blue);

			s5ptvfb_set_alpha_blending(&s5ptv_status, win->id);
		}
		break;

	case S5PTVFB_WIN_SET_CHROMA:
		if (copy_from_user(&p.user_chroma,
			(struct s5ptvfb_user_chroma __user *) arg,
			sizeof(p.user_chroma)))
			ret = -EFAULT;
		else {
			win->chroma.enabled = p.user_chroma.enabled;
			win->chroma.key = S5PTVFB_CHROMA(p.user_chroma.red,
						p.user_chroma.green,
						p.user_chroma.blue);

			s5ptvfb_set_chroma_key(&s5ptv_status, win->id);
		}
		break;

	case S5PTVFB_WIN_SET_ADDR:
		fb->fix.smem_start = (unsigned long)argp;
		s5ptvfb_set_buffer_address(&s5ptv_status, win->id);
		break;

	case S5PTVFB_SET_WIN_ON:
#ifdef CONFIG_USER_ALLOC_TVOUT
		s5ptvfb_display_on(&s5ptv_status);
		s5ptvfb_enable_window(0);
#endif
		break;

	case S5PTVFB_SET_WIN_OFF:
#ifdef CONFIG_USER_ALLOC_TVOUT
		s5ptvfb_display_off(&s5ptv_status);
		s5ptvfb_disable_window(0);
#endif
		break;

	}

	return 0;
}

static int s5ptvfb_open(struct fb_info *fb, int user)
{
	struct s5ptvfb_window *win = fb->par;
	int ret = 0;

	ret = s5p_tv_clk_gate(true);
	if (ret < 0) {
		printk(KERN_ERR "[Error]Cannot open it\n");
		return -1;
	}

	tv_phy_power(true);

	_s5p_tv_if_init_param();

	s5p_tv_v4l2_init_param();

	/*      s5ptv_status.tvout_param.disp_mode = TVOUT_720P_60; */
	s5ptv_status.tvout_param.out_mode  = TVOUT_OUTPUT_HDMI_RGB;

	_s5p_tv_if_set_disp();

#ifndef CONFIG_USER_ALLOC_TVOUT
	s5ptvfb_display_on(&s5ptv_status);

	s5ptvfb_enable_window(0);
#endif

	mutex_lock(&s5ptv_status.fb_lock);

	if (atomic_read(&win->in_use)) {
		dev_dbg(s5ptv_status.dev_fb,
		"do not allow multiple open "
		"for window\n");
		ret = -EBUSY;

	} else
		atomic_inc(&win->in_use);

	mutex_unlock(&s5ptv_status.fb_lock);

	return ret;

}

struct fb_ops s5ptvfb_ops = {
	.owner = THIS_MODULE,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_check_var = s5ptvfb_check_var,
	.fb_set_par = s5ptvfb_set_par,
	.fb_blank = s5ptvfb_blank,
	.fb_pan_display = s5ptvfb_pan_display,
	.fb_setcolreg = s5ptvfb_setcolreg,
	.fb_cursor = s5ptvfb_cursor,
	.fb_ioctl = s5ptvfb_ioctl,
	.fb_open = s5ptvfb_open,
	.fb_release = s5ptvfb_release,
};

int s5ptvfb_direct_ioctl(int id, unsigned int cmd, unsigned long arg)
{
	struct fb_info *fb = s5ptv_status.fb;
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s5ptvfb_window *win = fb->par;
	void *argp = (void *) arg;
	int ret = 0;

	switch (cmd) {

	case FBIO_ALLOC:
		win->path = (enum s5ptvfb_data_path_t) argp;
		break;

	case FBIOGET_FSCREENINFO:
		ret = memcpy(argp, &fb->fix, sizeof(fb->fix)) ? 0 : -EFAULT;
		break;

	case FBIOGET_VSCREENINFO:
		ret = memcpy(argp, &fb->var, sizeof(fb->var)) ? 0 : -EFAULT;
		break;

	case FBIOPUT_VSCREENINFO:
		ret = s5ptvfb_check_var((struct fb_var_screeninfo *) argp, fb);
		if (ret) {
			dev_err(s5ptv_status.dev_fb, "invalid vscreeninfo\n");
			break;
		}

		ret = memcpy(&fb->var, (struct fb_var_screeninfo *) argp,
				sizeof(fb->var)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(s5ptv_status.dev_fb,
			       "failed to put new vscreeninfo\n");
			break;
		}

		ret = s5ptvfb_set_par(fb);
		break;

	case S5PTVFB_SET_WIN_ON:
#ifdef CONFIG_USER_ALLOC_TVOUT
		s5ptvfb_display_on(&s5ptv_status);
		s5ptvfb_enable_window(0);
#endif
		break;

	case S5PTVFB_SET_WIN_OFF:
#ifdef CONFIG_USER_ALLOC_TVOUT
		s5ptvfb_display_off(&s5ptv_status);
		s5ptvfb_disable_window(0);
#endif
		break;

	case S5PTVFB_POWER_ON:
		s5p_tv_clk_gate(true);
		tv_phy_power(true);

		_s5p_tv_if_init_param();

		s5p_tv_v4l2_init_param();

		/*      s5ptv_status.tvout_param.disp_mode = TVOUT_720P_60; */
		s5ptv_status.tvout_param.out_mode  = TVOUT_OUTPUT_HDMI;

		_s5p_tv_if_set_disp();

		break;

	case S5PTVFB_POWER_OFF:
		_s5p_vlayer_stop();
		_s5p_tv_if_stop();

		s5p_tv_clk_gate(false);
		tv_phy_power(false);
		break;

	case S5PTVFB_WIN_SET_ADDR:
		fix->smem_start = (unsigned long)argp;
		s5ptvfb_set_buffer_address(&s5ptv_status, win->id);
		break;

	default:
		ret = s5ptvfb_ioctl(fb, cmd, arg);
		break;
	}

	return ret;
}
EXPORT_SYMBOL(s5ptvfb_direct_ioctl);

int s5ptvfb_init_fbinfo(int id)
{
	struct fb_info *fb = s5ptv_status.fb;
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct fb_var_screeninfo *var = &fb->var;
	struct s5ptvfb_window *win = fb->par;
	struct s5ptvfb_alpha *alpha = &win->alpha;
	struct s5ptvfb_lcd *lcd = s5ptv_status.lcd;
	struct s5ptvfb_lcd_timing *timing = &lcd->timing;

	memset(win, 0, sizeof(struct s5ptvfb_window));

	platform_set_drvdata(to_platform_device(s5ptv_status.dev_fb), fb);

	strcpy(fix->id, S5PTVFB_NAME);

	/* fimd specific */
	win->id = id;
	win->path = DATA_PATH_DMA;
	win->dma_burst = 16;
	alpha->mode = PLANE_BLENDING;

	/* fbinfo */
	fb->fbops = &s5ptvfb_ops;
	fb->flags = FBINFO_FLAG_DEFAULT;
	fb->pseudo_palette = &win->pseudo_pal;
	fix->xpanstep = 0;
	fix->ypanstep = 0;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;
	var->xres = lcd->width;
	var->yres = lcd->height;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres + (var->yres * fix->ypanstep);
	var->bits_per_pixel = 32;
	var->xoffset = 0;
	var->yoffset = 0;
	var->width = 0;
	var->height = 0;
	var->transp.length = 0;

	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->hsync_len = timing->h_sw;
	var->vsync_len = timing->v_sw;
	var->left_margin = timing->h_fp;
	var->right_margin = timing->h_bp;
	var->upper_margin = timing->v_fp;
	var->lower_margin = timing->v_bp;

	var->pixclock = lcd->freq * (var->left_margin + var->right_margin +
				var->hsync_len + var->xres) *
				(var->upper_margin + var->lower_margin +
				var->vsync_len + var->yres);

	dev_dbg(s5ptv_status.dev_fb, "pixclock: %d\n", var->pixclock);

	s5ptvfb_set_bitfield(var);
	s5ptvfb_set_alpha_info(var, win);

	return 0;
}

static struct s5ptvfb_lcd max_tvfb = {
	.width = 1920,
	.height = 1080,
	.bpp = 32,
	.freq = 60,

	.timing = {
		.h_fp = 49,
		.h_bp = 17,
		.h_sw = 33,
		.v_fp = 4,
		.v_fpe = 1,
		.v_bp = 15,
		.v_bpe = 1,
		.v_sw = 6,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
};

void s5ptvfb_set_lcd_info(struct s5p_tv_status *ctrl)
{
	ctrl->lcd = &max_tvfb;
}
