/* linux/drivers/media/video/samsung/fimc/fimc_capture.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * V4L2 Capture device support file for Samsung Camera Interface (FIMC) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/videodev2_samsung.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <plat/media.h>
#include <plat/clock.h>
#include <plat/fimc.h>
#include <linux/delay.h>

#include "fimc.h"

/* subdev handling macro */
#define subdev_call(ctrl, o, f, args...) \
	v4l2_subdev_call(ctrl->cam->sd, o, f, ##args)

/* #define FIMC_CAP_DEBUG */

#ifdef FIMC_CAP_DEBUG
#ifdef fimc_dbg
#undef fimc_dbg
#endif
#define fimc_dbg fimc_err
#endif

static int vtmode = 0;
static int device_id = 0;

static const struct v4l2_fmtdesc capture_fmts[] = {
	{
		.index		= 0,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-5-6-5",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
	}, {
		.index		= 1,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-8-8-8, unpacked 24 bpp",
		.pixelformat	= V4L2_PIX_FMT_RGB32,
	}, {
		.index		= 2,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	}, {
		.index		= 3,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, CbYCrY",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	}, {
		.index		= 4,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, CrYCbY",
		.pixelformat	= V4L2_PIX_FMT_VYUY,
	}, {
		.index		= 5,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, YCrYCb",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
	}, {
		.index		= 6,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
	}, {
		.index		= 7,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,
	}, {
		.index		= 8,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CbCr, Tiled",
		.pixelformat	= V4L2_PIX_FMT_NV12T,
	}, {
		.index		= 9,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21,
	}, {
		.index		= 10,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,
	}, {
		.index		= 11,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61,
	}, {
		.index		= 12,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420,
	}, {
		.index		= 13,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_ENCODED,
		.description	= "Encoded JPEG bitstream",
		.pixelformat	= V4L2_PIX_FMT_JPEG,
	},
};

static const struct v4l2_queryctrl fimc_controls[] = {
	{
		.id = V4L2_CID_ROTATION,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Roataion",
		.minimum = 0,
		.maximum = 270,
		.step = 90,
		.default_value = 0,
	}, {
		.id = V4L2_CID_HFLIP,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Horizontal Flip",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	}, {
		.id = V4L2_CID_VFLIP,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Vertical Flip",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	}, {
		.id = V4L2_CID_PADDR_Y,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address Y",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.id = V4L2_CID_PADDR_CB,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address Cb",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.id = V4L2_CID_PADDR_CR,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address Cr",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.id = V4L2_CID_PADDR_CBCR,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address CbCr",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	},
};

#ifndef CONFIG_VIDEO_FIMC_MIPI
void s3c_csis_start(int lanes, int settle, int align, int width, int height,
		int pixel_format) {}
#endif

static int fimc_camera_init(struct fimc_control *ctrl)
{
	int ret;

	fimc_dbg("%s\n", __func__);

	/* do nothing if already initialized */
	if (ctrl->cam->initialized)
		return 0;

	/* enable camera power if needed */
	if (ctrl->cam->cam_power)
		ctrl->cam->cam_power(1);

	/* subdev call for init */
	ret = subdev_call(ctrl, core, init, 0);
	if (ret == -ENOIOCTLCMD) {
		fimc_err("%s: init subdev api not supported\n",
			__func__);
		return ret;
	}

	if (ctrl->cam->type == CAM_TYPE_MIPI) {
		/* subdev call for sleep/wakeup:
		 * no error although no s_stream api support
		 */
		u32 pixelformat;
		if (ctrl->cap->fmt.pixelformat == V4L2_PIX_FMT_JPEG)
			pixelformat = V4L2_PIX_FMT_JPEG;
		else
			pixelformat = ctrl->cam->pixelformat;

		subdev_call(ctrl, video, s_stream, 0);
		s3c_csis_start(ctrl->cam->mipi_lanes, ctrl->cam->mipi_settle, \
				ctrl->cam->mipi_align, ctrl->cam->width, \
				ctrl->cam->height, pixelformat);
		subdev_call(ctrl, video, s_stream, 1);
	}

	ctrl->cam->initialized = 1;

	return 0;
}


/* This function must be called after s_fmt and s_parm call to the subdev
 * has already been made.
 *
 *	- obtains the camera output (input to FIMC) resolution.
 *	- sets the preview size (aka camera output resolution) and framerate.
 *	- starts the preview operation.
 *
 * On success, returns 0.
 * On failure, returns the error code of the call that failed.
 */
static int fimc_camera_start(struct fimc_control *ctrl)
{
	struct v4l2_frmsizeenum cam_frmsize;
	struct v4l2_control cam_ctrl;
	int ret;
	ret = subdev_call(ctrl, video, enum_framesizes, &cam_frmsize);
	if (ret < 0) {
		fimc_err("%s: enum_framesizes failed\n", __func__);
		if (ret != -ENOIOCTLCMD)
			return ret;
	} else {
		if (vtmode == 1 && device_id != 0 && (ctrl->cap->rotate == 90 || ctrl->cap->rotate == 270)) {
			ctrl->cam->window.left = 136;
			ctrl->cam->window.top = 0;
			ctrl->cam->window.width = 368;
			ctrl->cam->window.height = 480;
			ctrl->cam->width = cam_frmsize.discrete.width;
			ctrl->cam->height = cam_frmsize.discrete.height;
			dev_err(ctrl->dev, "vtmode = 1, rotate = %d, device = front, cam->width = %d, cam->height = %d\n", ctrl->cap->rotate, ctrl->cam->width, ctrl->cam->height);
		} else {		
			ctrl->cam->window.left = 0;
			ctrl->cam->window.top = 0;
			ctrl->cam->window.width = ctrl->cam->width;
			ctrl->cam->window.height = ctrl->cam->height;
		}
	}

	cam_ctrl.id = V4L2_CID_CAM_PREVIEW_ONOFF;
	cam_ctrl.value = 1;
	ret = subdev_call(ctrl, core, s_ctrl, &cam_ctrl);

	/* When the device is waking up from sleep, this call may fail. In
	 * that case, it is better to reset the camera sensor and start again.
	 * If the preview fails again, the reason might be something else and
	 * we should return the error.
	 */
	if (ret < 0 && ret != -ENOIOCTLCMD) {
		ctrl->cam->initialized = 0;
		fimc_camera_init(ctrl);
		ret = subdev_call(ctrl, core, s_ctrl, &cam_ctrl);
		if (ret < 0 && ret != -ENOIOCTLCMD) {
			fimc_err("%s: Error in V4L2_CID_CAM_PREVIEW_ONOFF"
					" - start\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int fimc_camera_get_jpeg_memsize(struct fimc_control *ctrl)
{
	int ret;
	struct v4l2_control cam_ctrl;
	cam_ctrl.id = V4L2_CID_CAM_JPEG_MEMSIZE;

	ret = subdev_call(ctrl, core, g_ctrl, &cam_ctrl);
	if (ret < 0) {
		fimc_err("%s: Subdev doesn't support JEPG encoding.\n", \
				__func__);
		return 0;
	}

	return cam_ctrl.value;
}


static int fimc_capture_scaler_info(struct fimc_control *ctrl)
{
	struct fimc_scaler *sc = &ctrl->sc;
	struct v4l2_rect *window = &ctrl->cam->window;
	int tx, ty, sx, sy;

	sx = window->width;
	sy = window->height;
	tx = ctrl->cap->fmt.width;
	ty = ctrl->cap->fmt.height;

	sc->real_width = sx;
	sc->real_height = sy;

	fimc_dbg("%s: CamOut (%d, %d), TargetOut (%d, %d)\n", \
			__func__, sx, sy, tx, ty);

	if (sx <= 0 || sy <= 0) {
		fimc_err("%s: invalid source size\n", __func__);
		return -EINVAL;
	}

	if (tx <= 0 || ty <= 0) {
		fimc_err("%s: invalid target size\n", __func__);
		return -EINVAL;
	}

	fimc_get_scaler_factor(sx, tx, &sc->pre_hratio, &sc->hfactor);
	fimc_get_scaler_factor(sy, ty, &sc->pre_vratio, &sc->vfactor);

	/* Tushar - sx and sy should be multiple of pre_hratio and pre_vratio */
	sc->pre_dst_width = sx / sc->pre_hratio;
	sc->pre_dst_height = sy / sc->pre_vratio;

	sc->main_hratio = (sx << 8) / (tx << sc->hfactor);
	sc->main_vratio = (sy << 8) / (ty << sc->vfactor);

	sc->scaleup_h = (tx >= sx) ? 1 : 0;
	sc->scaleup_v = (ty >= sy) ? 1 : 0;

	return 0;
}

/**
 *  fimc_add_inqueue: used to add the buffer at given index to inqueue
 *
 *  Called from qbuf().
 *
 *  Returns error if buffer is already in queue or buffer index is out of range.
 */
static int fimc_add_inqueue(struct fimc_control *ctrl, int i)
{
	struct fimc_capinfo *cap = ctrl->cap;

	struct fimc_buf_set *buf;

	if (i >= cap->nr_bufs)
		return -EINVAL;

	list_for_each_entry(buf, &cap->inq, list) {
		if (buf->id == i) {
			fimc_dbg("%s: buffer %d already in inqueue.\n", \
					__func__, i);
			return -EINVAL;
		}
	}
	list_add_tail(&cap->bufs[i].list, &cap->inq);

	return 0;
}

static int fimc_add_outqueue(struct fimc_control *ctrl, int i)
{
	struct fimc_capinfo *cap = ctrl->cap;
	struct fimc_buf_set *buf;

	if (cap->nr_bufs > FIMC_PHYBUFS) {
		if (list_empty(&cap->inq))
			return -ENOENT;

		buf = list_first_entry(&cap->inq, struct fimc_buf_set, list);
		list_del(&buf->list);
	} else {
		buf = &cap->bufs[i];
	}

	cap->outq[i] = buf->id;
	fimc_hwset_output_address(ctrl, buf, i);

	return 0;
}

static int fimc_update_hwaddr(struct fimc_control *ctrl)
{
	int i;

	for (i = 0; i < FIMC_PHYBUFS; i++)
		fimc_add_outqueue(ctrl, i);

	return 0;
}

int fimc_g_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cam || !ctrl->cam->sd) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);
	ret = subdev_call(ctrl, video, g_parm, a);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_s_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret = 0;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cam || !ctrl->cam->sd) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);
	if (ctrl->id != 2)
		ret = subdev_call(ctrl, video, s_parm, a);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

/* Enumerate controls */
int fimc_queryctrl(struct file *file, void *fh, struct v4l2_queryctrl *qc)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int i, ret;

	fimc_dbg("%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(fimc_controls); i++) {
		if (fimc_controls[i].id == qc->id) {
			memcpy(qc, &fimc_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	mutex_lock(&ctrl->v4l2_lock);
	ret = subdev_call(ctrl, core, queryctrl, qc);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

/* Menu control items */
int fimc_querymenu(struct file *file, void *fh, struct v4l2_querymenu *qm)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);
	ret = subdev_call(ctrl, core, querymenu, qm);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}


/* Given the index, we will return the camera name if there is any camera
 * present at the given id.
 */
int fimc_enum_input(struct file *file, void *fh, struct v4l2_input *inp)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	fimc_dbg("%s: index %d\n", __func__, inp->index);

	if (inp->index < 0 || inp->index >= FIMC_MAXCAMS) {
		fimc_err("%s: invalid input index, received = %d\n", \
				__func__, inp->index);
		return -EINVAL;
	}

	if (!fimc->camera_isvalid[inp->index])
		return -EINVAL;

	strcpy(inp->name, fimc->camera[inp->index].info->type);
	inp->type = V4L2_INPUT_TYPE_CAMERA;

	return 0;
}

int fimc_g_input(struct file *file, void *fh, unsigned int *i)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct fimc_global *fimc = get_fimc_dev();

	/* In case of isueing g_input before s_input */
	if (!ctrl->cam) {
		fimc_err("no camera device selected yet!" \
				"do VIDIOC_S_INPUT first\n");
		return -ENODEV;
	}

	*i = (unsigned int) fimc->active_camera;

	fimc_dbg("%s: index %d\n", __func__, *i);

	return 0;
}

int fimc_release_subdev(struct fimc_control *ctrl)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct i2c_client *client;

	if (ctrl && ctrl->cam && ctrl->cam->sd) {
		fimc_dbg("%s called\n", __func__);
		client = v4l2_get_subdevdata(ctrl->cam->sd);
		i2c_unregister_device(client);
		ctrl->cam->sd = NULL;
		if (ctrl->cam->cam_power)
			ctrl->cam->cam_power(0);
		ctrl->cam->initialized = 0;
		ctrl->cam = NULL;
		fimc->active_camera = -1;
	}
	return 0;
}

static int fimc_configure_subdev(struct fimc_control *ctrl)
{
	struct i2c_adapter *i2c_adap;
	struct i2c_board_info *i2c_info;
	struct v4l2_subdev *sd;
	unsigned short addr;
	char *name;

	/* set parent for mclk */
	if (clk_get_parent(ctrl->cam->clk->parent))
		clk_set_parent(ctrl->cam->clk->parent, ctrl->cam->srclk);

	/* set rate for mclk */
	if (clk_get_rate(ctrl->cam->clk))
		clk_set_rate(ctrl->cam->clk, ctrl->cam->clk_rate);

	i2c_adap = i2c_get_adapter(ctrl->cam->i2c_busnum);
	if (!i2c_adap)
		fimc_err("subdev i2c_adapter missing-skip registration\n");

	i2c_info = ctrl->cam->info;
	if (!i2c_info) {
		fimc_err("%s: subdev i2c board info missing\n", __func__);
		return -ENODEV;
	}

	name = i2c_info->type;
	if (!name) {
		fimc_err("subdev i2c driver name missing-skip registration\n");
		return -ENODEV;
	}

	addr = i2c_info->addr;
	if (!addr) {
		fimc_err("subdev i2c address missing-skip registration\n");
		return -ENODEV;
	}
	/*
	 * NOTE: first time subdev being registered,
	 * s_config is called and try to initialize subdev device
	 * but in this point, we are not giving MCLK and power to subdev
	 * so nothing happens but pass platform data through
	 */
	sd = v4l2_i2c_new_subdev_board(&ctrl->v4l2_dev, i2c_adap,
			name, i2c_info, &addr);
	if (!sd) {
		fimc_err("%s: v4l2 subdev board registering failed\n",
				__func__);
	}

	/* Assign subdev to proper camera device pointer */
	ctrl->cam->sd = sd;

	return 0;
}

int fimc_s_input(struct file *file, void *fh, unsigned int i)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret = 0;

	fimc_dbg("%s: index %d\n", __func__, i);

	if (i < 0 || i >= FIMC_MAXCAMS) {
		fimc_err("%s: invalid input index\n", __func__);
		return -EINVAL;
	}

	if (!fimc->camera_isvalid[i])
		return -EINVAL;

	if (fimc->camera[i].sd && ctrl->id != 2) {
		fimc_err("%s: Camera already in use.\n", __func__);
		return -EBUSY;
	}

	mutex_lock(&ctrl->v4l2_lock);
	/* If ctrl->cam is not NULL, there is one subdev already registered.
	 * We need to unregister that subdev first.
	 */
	if (i != fimc->active_camera) {
		fimc_release_subdev(ctrl);
		ctrl->cam = &fimc->camera[i];
		ret = fimc_configure_subdev(ctrl);
		if (ret < 0) {
			mutex_unlock(&ctrl->v4l2_lock);
			fimc_err("%s: Could not register camera sensor "
					"with V4L2.\n", __func__);
			return -ENODEV;
		}
		fimc->active_camera = i;
	}

	if (ctrl->id == 2) {
		if (i == fimc->active_camera) {
			ctrl->cam = &fimc->camera[i];
		} else {
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
		}
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_enum_fmt_vid_capture(struct file *file, void *fh,
					struct v4l2_fmtdesc *f)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int i = f->index;
	int num_entries = 0;
	int ret = 0;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cam || !ctrl->cam->sd) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	num_entries = sizeof(capture_fmts)/sizeof(struct v4l2_fmtdesc);

	if (i >= num_entries) {
		f->index -= num_entries;
		mutex_lock(&ctrl->v4l2_lock);
		ret = subdev_call(ctrl, video, enum_fmt, f);
		mutex_unlock(&ctrl->v4l2_lock);
		f->index += num_entries;
		return ret;
	}

	memset(f, 0, sizeof(*f));
	memcpy(f, &capture_fmts[i], sizeof(*f));

	return 0;
}

int fimc_g_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cap) {
		fimc_err("%s: no capture device info\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(&f->fmt.pix, 0, sizeof(f->fmt.pix));
	memcpy(&f->fmt.pix, &ctrl->cap->fmt, sizeof(f->fmt.pix));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

/*
 * Check for whether the requested format
 * can be streamed out from FIMC
 * depends on FIMC node
 */
static int fimc_fmt_avail(struct fimc_control *ctrl,
		struct v4l2_format *f)
{
	int i;

	/*
	 * TODO: check for which FIMC is used.
	 * Available fmt should be varied for each FIMC
	 */

	for (i = 0; i < sizeof(capture_fmts); i++) {
		if (capture_fmts[i].pixelformat == f->fmt.pix.pixelformat)
			return 0;
	}

	fimc_err("Not supported pixelformat requested\n");

	return -1;
}

/*
 * figures out the depth of requested format
 */
static int fimc_fmt_depth(struct fimc_control *ctrl, struct v4l2_format *f)
{
	int err, depth = 0;

	/* First check for available format or not */
	err = fimc_fmt_avail(ctrl, f);
	if (err < 0)
		return -EINVAL;

	/* handles only supported pixelformats */
	switch (f->fmt.pix.pixelformat) {
	case V4L2_PIX_FMT_RGB32:
		depth = 32;
		fimc_dbg("32bpp\n");
		break;
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		depth = 16;
		fimc_dbg("16bpp\n");
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV12T:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_YUV420:
		depth = 12;
		fimc_dbg("12bpp\n");
		break;
	case V4L2_PIX_FMT_JPEG:
		depth = -1;
		fimc_dbg("Compressed format.\n");
		break;
	default:
		fimc_dbg("why am I here? - received %x\n",
				f->fmt.pix.pixelformat);
		break;
	}

	return depth;
}

int fimc_s_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct fimc_capinfo *cap = ctrl->cap;
	int ret = 0;
	int depth;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cam || !ctrl->cam->sd) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}
	/*
	 * The first time alloc for struct cap_info, and will be
	 * released at the file close.
	 * Anyone has better idea to do this?
	*/
	if (!cap) {
		cap = kzalloc(sizeof(*cap), GFP_KERNEL);
		if (!cap) {
			fimc_err("%s: no memory for "
				"capture device info\n", __func__);
			return -ENOMEM;
		}

		/* assign to ctrl */
		ctrl->cap = cap;
	} else {
		memset(cap, 0, sizeof(*cap));
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(&cap->fmt, 0, sizeof(cap->fmt));
	memcpy(&cap->fmt, &f->fmt.pix, sizeof(cap->fmt));

	/*
	 * Note that expecting format only can be with
	 * available output format from FIMC
	 * Following items should be handled in driver
	 * bytesperline = width * depth / 8
	 * sizeimage = bytesperline * height
	 */
	/* This function may return 0 or -1 in case of error, hence need to
	 * check here.
	 */
	depth = fimc_fmt_depth(ctrl, f);
	if (depth == 0) {
		mutex_unlock(&ctrl->v4l2_lock);
		fimc_err("%s: Invalid pixel format\n", __func__);
		return -EINVAL;
	} else if (depth < 0) {
		/*
		 * When the pixelformat is JPEG, the application is requesting
		 * for data in JPEG compressed format.
		 */
		ret = subdev_call(ctrl, video, try_fmt, f);
		if (ret < 0) {
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
		}
		cap->fmt.colorspace = V4L2_COLORSPACE_JPEG;
	} else {
		cap->fmt.bytesperline = (cap->fmt.width * depth) >> 3;
		cap->fmt.sizeimage = (cap->fmt.bytesperline * cap->fmt.height);
	}

	if (cap->fmt.colorspace == V4L2_COLORSPACE_JPEG) {
		ctrl->sc.bypass = 1;
		cap->lastirq = 1;
	}

	if (ctrl->id != 2)
		ret = subdev_call(ctrl, video, s_fmt, f);

	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_try_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	return 0;
}

static int fimc_alloc_buffers(struct fimc_control *ctrl, int size[], int align)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int i, plane;

	for (i = 0; i < cap->nr_bufs; i++) {
		for (plane = 0; plane < 4; plane++) {
			cap->bufs[i].length[plane] = size[plane];
			if (!cap->bufs[i].length[plane])
				continue;

			fimc_dma_alloc(ctrl, &cap->bufs[i], plane, align);

			if (!cap->bufs[i].base[plane])
				goto err_alloc;
		}

		cap->bufs[i].state = VIDEOBUF_PREPARED;
		cap->bufs[i].id = i;
	}

	return 0;

err_alloc:
	for (i = 0; i < cap->nr_bufs; i++) {
		if (cap->bufs[i].base[plane])
			fimc_dma_free(ctrl, &cap->bufs[i], plane);

		memset(&cap->bufs[i], 0, sizeof(cap->bufs[i]));
	}

	return -ENOMEM;
}

static void fimc_free_buffers(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap;
	int i;

	if (ctrl && ctrl->cap)
		cap = ctrl->cap;
	else
		return;


	for (i = 0; i < FIMC_PHYBUFS; i++) {
		memset(&cap->bufs[i], 0, sizeof(cap->bufs[i]));
		cap->bufs[i].state = VIDEOBUF_NEEDS_INIT;
	}

	ctrl->mem.curr = ctrl->mem.base;
}

int fimc_reqbufs_capture(void *fh, struct v4l2_requestbuffers *b)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct fimc_capinfo *cap = ctrl->cap;
	int ret = 0, i;
	int size[4] = { 0, 0, 0, 0};
	int align = 0;

	if (b->memory != V4L2_MEMORY_MMAP) {
		fimc_err("%s: invalid memory type\n", __func__);
		return -EINVAL;
	}

	if (!cap) {
		fimc_err("%s: no capture device info\n", __func__);
		return -ENODEV;
	}

	if (!ctrl->cam || !ctrl->cam->sd) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);

	if (b->count < 1 || b->count > FIMC_CAPBUFS)
		return -EINVAL;

	/* It causes flickering as buf_0 and buf_3 refer to same hardware
	 * address.
	 */
	if (b->count == 3)
		b->count = 4;

	cap->nr_bufs = b->count;

	fimc_dbg("%s: requested %d buffers\n", __func__, b->count);

	INIT_LIST_HEAD(&cap->inq);
	fimc_free_buffers(ctrl);

	switch (cap->fmt.pixelformat) {
	case V4L2_PIX_FMT_RGB32:	/* fall through */
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_YUYV:		/* fall through */
	case V4L2_PIX_FMT_UYVY:		/* fall through */
	case V4L2_PIX_FMT_VYUY:		/* fall through */
	case V4L2_PIX_FMT_YVYU:		/* fall through */
	case V4L2_PIX_FMT_YUV422P:	/* fall through */
		size[0] = cap->fmt.sizeimage;
		break;

	case V4L2_PIX_FMT_NV16:		/* fall through */
	case V4L2_PIX_FMT_NV61:
		size[0] = cap->fmt.width * cap->fmt.height;
		size[1] = cap->fmt.width * cap->fmt.height;
		size[3] = 16; /* Padding buffer */
		break;
	case V4L2_PIX_FMT_NV12:
		size[0] = cap->fmt.width * cap->fmt.height;
		size[1] = cap->fmt.width * cap->fmt.height/2;
		break;
	case V4L2_PIX_FMT_NV21:
		size[0] = cap->fmt.width * cap->fmt.height;
		size[1] = cap->fmt.width * cap->fmt.height/2;
		size[3] = 16; /* Padding buffer */
		break;
	case V4L2_PIX_FMT_NV12T:
		/* Tiled frame size calculations as per 4x2 tiles
		 *	- Width: Has to be aligned to 2 times the tile width
		 *	- Height: Has to be aligned to the tile height
		 *	- Alignment: Has to be aligned to the size of the
		 *	macrotile (size of 4 tiles)
		 *
		 * NOTE: In case of rotation, we need modified calculation as
		 * width and height are aligned to different values.
		 */
		if (cap->rotate == 90 || cap->rotate == 270) {
			size[0] = ALIGN(ALIGN(cap->fmt.height, 128) *
					ALIGN(cap->fmt.width, 32),
					SZ_8K);
			size[1] = ALIGN(ALIGN(cap->fmt.height, 128) *
					ALIGN(cap->fmt.width/2, 32),
					SZ_8K);
		} else {
			size[0] = ALIGN(ALIGN(cap->fmt.width, 128) *
					ALIGN(cap->fmt.height, 32),
					SZ_8K);
			size[1] = ALIGN(ALIGN(cap->fmt.width, 128) *
					ALIGN(cap->fmt.height/2, 32),
					SZ_8K);
		}
		align = SZ_8K;
		break;

	case V4L2_PIX_FMT_YUV420:
		size[0] = cap->fmt.width * cap->fmt.height;
		size[1] = cap->fmt.width * cap->fmt.height >> 2;
		size[2] = cap->fmt.width * cap->fmt.height >> 2;
		size[3] = 16; /* Padding buffer */
		break;

	case V4L2_PIX_FMT_JPEG:
		size[0] = fimc_camera_get_jpeg_memsize(ctrl);
	default:
		break;
	}

	ret = fimc_alloc_buffers(ctrl, size, align);
	if (ret) {
		fimc_err("%s: no memory for "
				"capture buffer\n", __func__);
		mutex_unlock(&ctrl->v4l2_lock);
		return -ENOMEM;
	}

	for (i = cap->nr_bufs; i < FIMC_PHYBUFS; i++) {
		memcpy(&cap->bufs[i], \
			&cap->bufs[i - cap->nr_bufs], sizeof(cap->bufs[i]));
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_querybuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	if (!ctrl->cap || !ctrl->cap->bufs) {
		fimc_err("%s: no capture device info\n", __func__);
		return -ENODEV;
	}

	if (ctrl->status != FIMC_STREAMOFF) {
		fimc_err("fimc is running\n");
		return -EBUSY;
	}

	mutex_lock(&ctrl->v4l2_lock);

	b->length = ctrl->cap->bufs[b->index].length[FIMC_ADDR_Y]
			+ ctrl->cap->bufs[b->index].length[FIMC_ADDR_CB]
			+ ctrl->cap->bufs[b->index].length[FIMC_ADDR_CR];

	b->m.offset = b->index * PAGE_SIZE;

	ctrl->cap->bufs[b->index].state = VIDEOBUF_IDLE;

	mutex_unlock(&ctrl->v4l2_lock);

	fimc_dbg("%s: %d bytes at index: %d\n", __func__, b->length, b->index);

	return 0;
}

int fimc_g_ctrl_capture(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret = 0;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cam || !ctrl->cam->sd || !ctrl->cap) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);

	switch (c->id) {
	case V4L2_CID_ROTATION:
		c->value = ctrl->cap->rotate;
		break;

	case V4L2_CID_HFLIP:
		c->value = (ctrl->cap->flip & FIMC_XFLIP) ? 1 : 0;
		break;

	case V4L2_CID_VFLIP:
		c->value = (ctrl->cap->flip & FIMC_YFLIP) ? 1 : 0;
		break;

	default:
		/* get ctrl supported by subdev */
		mutex_unlock(&ctrl->v4l2_lock);
		ret = subdev_call(ctrl, core, g_ctrl, c);
		mutex_lock(&ctrl->v4l2_lock);
		break;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

/**
 * We used s_ctrl API to get the physical address of the buffers.
 * In g_ctrl, we can pass only one parameter, thus we cannot pass
 * the index of the buffer.
 * In order to use g_ctrl for obtaining the physical address, we
 * will have to create CID ids for all values (4 ids for Y0~Y3 and 4 ids
 * for C0~C3). Currently, we will continue with the existing
 * implementation till we get any better idea to implement.
 */
int fimc_s_ctrl_capture(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret = 0;

	fimc_info2("%s\n", __func__);

	if (!ctrl->cam || !ctrl->cam->sd || !ctrl->cap || !ctrl->cap->bufs) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);

	switch (c->id) {
	case V4L2_CID_ROTATION:
		ctrl->cap->rotate = c->value;
		break;

	case V4L2_CID_HFLIP:	/* fall through */
		ctrl->cap->flip |= FIMC_XFLIP;
		break;
	case V4L2_CID_VFLIP:
		ctrl->cap->flip |= FIMC_YFLIP;
		break;

	case V4L2_CID_PADDR_Y:
		c->value = ctrl->cap->bufs[c->value].base[FIMC_ADDR_Y];
		break;

	case V4L2_CID_PADDR_CB:		/* fall through */
	case V4L2_CID_PADDR_CBCR:
		c->value = ctrl->cap->bufs[c->value].base[FIMC_ADDR_CB];
		break;

	case V4L2_CID_PADDR_CR:
		c->value = ctrl->cap->bufs[c->value].base[FIMC_ADDR_CR];
		break;

	/* Implementation as per C100 FIMC driver */
	case V4L2_CID_STREAM_PAUSE:
		fimc_hwset_stop_processing(ctrl);
		break;

	case V4L2_CID_IMAGE_EFFECT_APPLY:
		ctrl->fe.ie_on = c->value ? 1 : 0;
		ctrl->fe.ie_after_sc = 0;
		ret = fimc_hwset_image_effect(ctrl);
		break;

	case V4L2_CID_IMAGE_EFFECT_FN:
		if (c->value < 0 || c->value > FIMC_EFFECT_FIN_SILHOUETTE)
			return -EINVAL;
		ctrl->fe.fin = c->value;
		ret = 0;
		break;

	case V4L2_CID_IMAGE_EFFECT_CB:
		ctrl->fe.pat_cb = c->value & 0xFF;
		ret = 0;
		break;

	case V4L2_CID_IMAGE_EFFECT_CR:
		ctrl->fe.pat_cr = c->value & 0xFF;
		ret = 0;
		break;
		
	case V4L2_CID_CAMERA_VT_MODE:
		vtmode = c->value;
		ret = subdev_call(ctrl, core, s_ctrl, c);
		break;

	default:
		/* try on subdev */
		mutex_unlock(&ctrl->v4l2_lock);
		if (2 != ctrl->id)
			ret = subdev_call(ctrl, core, s_ctrl, c);
		else
			ret = 0;
		mutex_lock(&ctrl->v4l2_lock);
		break;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_s_ext_ctrls_capture(void *fh, struct v4l2_ext_controls *c)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret = 0;

	mutex_lock(&ctrl->v4l2_lock);

	/* try on subdev */
	ret = subdev_call(ctrl, core, s_ext_ctrls, c);

	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_cropcap_capture(void *fh, struct v4l2_cropcap *a)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct fimc_capinfo *cap = ctrl->cap;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cam || !ctrl->cam->sd || !ctrl->cap) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);

	/* crop limitations */
	cap->cropcap.bounds.left = 0;
	cap->cropcap.bounds.top = 0;
	cap->cropcap.bounds.width = ctrl->cam->width;
	cap->cropcap.bounds.height = ctrl->cam->height;

	/* crop default values */
	cap->cropcap.defrect.left = 0;
	cap->cropcap.defrect.top = 0;
	cap->cropcap.defrect.width = ctrl->cam->width;
	cap->cropcap.defrect.height = ctrl->cam->height;

	a->bounds = cap->cropcap.bounds;
	a->defrect = cap->cropcap.defrect;

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_crop_capture(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cap) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);
	a->c = ctrl->cap->crop;
	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

static int fimc_capture_crop_size_check(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int win_hor_offset = 0, win_hor_offset2 = 0;
	int win_ver_offset = 0, win_ver_offset2 = 0;
	int crop_width = 0, crop_height = 0;

	/* check win_hor_offset, win_hor_offset2 */
	win_hor_offset = ctrl->cam->window.left;
	win_hor_offset2 = ctrl->cam->width - ctrl->cam->window.left -
						ctrl->cam->window.width;

	win_ver_offset = ctrl->cam->window.top;
	win_ver_offset2 = ctrl->cam->height - ctrl->cam->window.top -
						ctrl->cam->window.height;

	if (win_hor_offset < 0 || win_hor_offset2 < 0) {
		fimc_err("%s: Offset (left-side(%d) or right-side(%d) "
				"is negative.\n", __func__, \
				win_hor_offset, win_hor_offset2);
		return -1;
	}

	if (win_ver_offset < 0 || win_ver_offset2 < 0) {
		fimc_err("%s: Offset (top-side(%d) or bottom-side(%d)) "
				"is negative.\n", __func__, \
				win_ver_offset, win_ver_offset2);
		return -1;
	}

	if ((win_hor_offset % 2) || (win_hor_offset2 % 2)) {
		fimc_err("%s: win_hor_offset must be multiple of 2\n", \
				__func__);
		return -1;
	}

	/* check crop_width, crop_height */
	crop_width = ctrl->cam->window.width;
	crop_height = ctrl->cam->window.height;

	if (crop_width % 16) {
		fimc_err("%s: crop_width must be multiple of 16\n", __func__);
		return -1;
	}

	switch (cap->fmt.pixelformat) {
	case V4L2_PIX_FMT_YUV420:       /* fall through */
	case V4L2_PIX_FMT_NV12:         /* fall through */
	case V4L2_PIX_FMT_NV21:         /* fall through */
	case V4L2_PIX_FMT_NV12T:         /* fall through */
		if ((crop_height % 2) || (crop_height < 8)) {
			fimc_err("%s: crop_height error!\n", __func__);
			return -1;
		}
		break;
	default:
		break;
	}

	return 0;
}

/** Given crop parameters are w.r.t. target resolution. Scale
 *  it w.r.t. camera source resolution.
 *
 * Steps:
 *	1. Scale as camera resolution with fixed-point calculation
 *	2. Check for overflow condition
 *	3. Apply FIMC constrainsts
 */
static void fimc_capture_update_crop_window(struct fimc_control *ctrl)
{
	unsigned int zoom_hor = 0;
	unsigned int zoom_ver = 0;
	unsigned int multiplier = 1024;

	if (!ctrl->cam->width || !ctrl->cam->height)
		return;

	zoom_hor = ctrl->cap->fmt.width * multiplier / ctrl->cam->width;
	zoom_ver = ctrl->cap->fmt.height * multiplier / ctrl->cam->height;

	if (!zoom_hor || !zoom_ver)
		return;

	/* Width */
	ctrl->cam->window.width = ctrl->cap->crop.width * multiplier / zoom_hor;
	if (ctrl->cam->window.width > ctrl->cam->width)
		ctrl->cam->window.width = ctrl->cam->width;
	if (ctrl->cam->window.width % 16)
		ctrl->cam->window.width =
			(ctrl->cam->window.width + 0xF) & ~0xF;

	/* Left offset */
	ctrl->cam->window.left = ctrl->cap->crop.left * multiplier / zoom_hor;
	if (ctrl->cam->window.width + ctrl->cam->window.left > ctrl->cam->width)
		ctrl->cam->window.left =
			(ctrl->cam->width - ctrl->cam->window.width)/2;
	if (ctrl->cam->window.left % 2)
		ctrl->cam->window.left--;

	/* Height */
	ctrl->cam->window.height =
		(ctrl->cap->crop.height * multiplier) / zoom_ver;
	if (ctrl->cam->window.top > ctrl->cam->height)
		ctrl->cam->window.height = ctrl->cam->height;
	if (ctrl->cam->window.height % 2)
		ctrl->cam->window.height--;

	/* Top offset */
	ctrl->cam->window.top = ctrl->cap->crop.top * multiplier / zoom_ver;
	if (ctrl->cam->window.height + ctrl->cam->window.top >
			ctrl->cam->height)
		ctrl->cam->window.top =
			(ctrl->cam->height - ctrl->cam->window.height)/2;
	if (ctrl->cam->window.top % 2)
		ctrl->cam->window.top--;

	fimc_dbg("Cam (%dx%d) Crop: (%d %d %d %d) Win: (%d %d %d %d)\n", \
			ctrl->cam->width, ctrl->cam->height, \
			ctrl->cap->crop.left, ctrl->cap->crop.top, \
			ctrl->cap->crop.width, ctrl->cap->crop.height, \
			ctrl->cam->window.left, ctrl->cam->window.top, \
			ctrl->cam->window.width, ctrl->cam->window.height);

}

int fimc_s_crop_capture(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret = 0;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cap) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);
	ctrl->cap->crop = a->c;

	fimc_capture_update_crop_window(ctrl);

	ret = fimc_capture_crop_size_check(ctrl);
	if (ret < 0) {
		mutex_unlock(&ctrl->v4l2_lock);
		fimc_err("%s: Invalid crop parameters.\n", __func__);
		return -EINVAL;
	}

	if (ctrl->status == FIMC_STREAMON &&
			ctrl->cap->fmt.pixelformat != V4L2_PIX_FMT_JPEG) {
		fimc_hwset_shadow_disable(ctrl);
		fimc_hwset_camera_offset(ctrl);
		fimc_capture_scaler_info(ctrl);
		fimc_hwset_prescaler(ctrl, &ctrl->sc);
		fimc_hwset_scaler(ctrl, &ctrl->sc);
		fimc_hwset_shadow_enable(ctrl);
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_start_capture(struct fimc_control *ctrl)
{
	fimc_dbg("%s\n", __func__);

	if (!ctrl->sc.bypass)
		fimc_hwset_start_scaler(ctrl);

	fimc_hwset_enable_capture(ctrl, ctrl->sc.bypass);

	return 0;
}

int fimc_stop_capture(struct fimc_control *ctrl)
{
	fimc_dbg("%s\n", __func__);

	if (ctrl->cap->lastirq) {
		fimc_hwset_enable_lastirq(ctrl);
		fimc_hwset_disable_capture(ctrl);
		fimc_hwset_disable_lastirq(ctrl);
		ctrl->cap->lastirq = 0;
	} else {
		fimc_hwset_disable_capture(ctrl);
	}

	fimc_hwset_disable_irq(ctrl);
	fimc_hwset_clear_irq(ctrl);

	if (!ctrl->sc.bypass)
		fimc_hwset_stop_scaler(ctrl);
	else
		ctrl->sc.bypass = 0;

	fimc_wait_disable_capture(ctrl);

	return 0;
}

static void fimc_reset_capture(struct fimc_control *ctrl)
{
	int i;

	ctrl->status = FIMC_READY_OFF;

	fimc_stop_capture(ctrl);

	for (i = 0; i < FIMC_PHYBUFS; i++)
		fimc_add_inqueue(ctrl, ctrl->cap->outq[i]);

	fimc_hwset_reset(ctrl);

	if (0 != ctrl->id)
		fimc_clk_en(ctrl, false);

	ctrl->status = FIMC_STREAMOFF;
}


int fimc_streamon_capture(void *fh)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct fimc_capinfo *cap = ctrl->cap;
	struct v4l2_frmsizeenum cam_frmsize;
	int rot;
	int ret;

	fimc_dbg("%s\n", __func__);
	char *ce147 = "CE147 0-003c";
	device_id = strcmp(ctrl->cam->sd->name, ce147);
	fimc_dbg("%s, name(%s), device_id(%d), vtmode(%d)\n", __func__, ctrl->cam->sd->name , device_id, vtmode);

	if (!ctrl->cam || !ctrl->cam->sd) {
		fimc_err("%s: No capture device.\n", __func__);
		return -ENODEV;
	}

	if (ctrl->status == FIMC_STREAMON) {
		fimc_err("%s: Camera already running.\n", __func__);
		return -EBUSY;
	}

	mutex_lock(&ctrl->v4l2_lock);

	if (0 != ctrl->id)
		fimc_clk_en(ctrl, true);

	ctrl->status = FIMC_READY_ON;
	cap->irq = 0;

	fimc_hwset_enable_irq(ctrl, 0, 1);

	if (!ctrl->cam->initialized)
		fimc_camera_init(ctrl);
	
	ret = subdev_call(ctrl, video, enum_framesizes, &cam_frmsize);
	if (ret < 0) {
		dev_err(ctrl->dev, "%s: enum_framesizes failed\n", __func__);
		if(ret != -ENOIOCTLCMD)
			return ret;
	} else {
		if (vtmode == 1 && device_id != 0 && (cap->rotate == 90 || cap->rotate == 270)) {
		ctrl->cam->window.left = 136;
			ctrl->cam->window.top = 0;//
			ctrl->cam->window.width = 368;
			ctrl->cam->window.height = 480;
			ctrl->cam->width = cam_frmsize.discrete.width;
			ctrl->cam->height = cam_frmsize.discrete.height;
			dev_err(ctrl->dev, "vtmode = 1, rotate = %d, device = front, cam->width = %d, cam->height = %d\n", cap->rotate, ctrl->cam->width, ctrl->cam->height);
		} else {
			ctrl->cam->window.left = 0;
			ctrl->cam->window.top = 0;
			ctrl->cam->width = ctrl->cam->window.width = cam_frmsize.discrete.width;
			ctrl->cam->height = ctrl->cam->window.height = cam_frmsize.discrete.height;
		}
	}

	if (ctrl->id != 2 &&
			ctrl->cap->fmt.colorspace != V4L2_COLORSPACE_JPEG) {
		ret = fimc_camera_start(ctrl);
		if (ret < 0) {
			fimc_reset_capture(ctrl);
			mutex_unlock(&ctrl->v4l2_lock);
			return ret;
		}
	}

	fimc_hwset_camera_type(ctrl);
	fimc_hwset_camera_polarity(ctrl);
	fimc_update_hwaddr(ctrl);

	if (cap->fmt.pixelformat != V4L2_PIX_FMT_JPEG) {
		fimc_hwset_camera_source(ctrl);
		fimc_hwset_camera_offset(ctrl);

		fimc_capture_scaler_info(ctrl);
		fimc_hwset_prescaler(ctrl, &ctrl->sc);
		fimc_hwset_scaler(ctrl, &ctrl->sc);

		fimc_hwset_output_colorspace(ctrl, cap->fmt.pixelformat);
		fimc_hwset_output_addr_style(ctrl, cap->fmt.pixelformat);
		fimc_hwset_output_area(ctrl, cap->fmt.width, cap->fmt.height);

		if (cap->fmt.pixelformat == V4L2_PIX_FMT_RGB32 ||
				cap->fmt.pixelformat == V4L2_PIX_FMT_RGB565)
			fimc_hwset_output_rgb(ctrl, cap->fmt.pixelformat);
		else
			fimc_hwset_output_yuv(ctrl, cap->fmt.pixelformat);

		fimc_hwset_output_size(ctrl, cap->fmt.width, cap->fmt.height);

		fimc_hwset_output_scan(ctrl, &cap->fmt);
		fimc_hwset_output_rot_flip(ctrl, cap->rotate, cap->flip);
		rot = fimc_mapping_rot_flip(cap->rotate, cap->flip);

		if (rot & FIMC_ROT) {
			fimc_hwset_org_output_size(ctrl, cap->fmt.height,
					cap->fmt.width);
		} else {
			fimc_hwset_org_output_size(ctrl, cap->fmt.width,
					cap->fmt.height);
		}
		fimc_hwset_jpeg_mode(ctrl, false);
	} else {
		fimc_hwset_output_area_size(ctrl, \
				fimc_camera_get_jpeg_memsize(ctrl)/2);
		fimc_hwset_jpeg_mode(ctrl, true);
	}

	if (ctrl->cap->fmt.colorspace == V4L2_COLORSPACE_JPEG)
		fimc_hwset_scaler_bypass(ctrl);

	fimc_start_capture(ctrl);

	if (ctrl->cap->fmt.colorspace == V4L2_COLORSPACE_JPEG &&
			ctrl->id != 2) {
		struct v4l2_control cam_ctrl;

		cam_ctrl.id = V4L2_CID_CAM_CAPTURE;
		ret = subdev_call(ctrl, core, s_ctrl, &cam_ctrl);
		if (ret < 0 && ret != -ENOIOCTLCMD) {
			fimc_reset_capture(ctrl);
			mutex_unlock(&ctrl->v4l2_lock);
			fimc_err("%s: Error in V4L2_CID_CAM_CAPTURE\n", \
					__func__);
			return -EPERM;
		}
	}

	ctrl->status = FIMC_STREAMON;

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_streamoff_capture(void *fh)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cap || !ctrl->cam || !ctrl->cam->sd) {
		fimc_err("%s: No capture info.\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);
	fimc_reset_capture(ctrl);
	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	if (b->memory != V4L2_MEMORY_MMAP) {
		fimc_err("%s: invalid memory type\n", __func__);
		return -EINVAL;
	}

	if (ctrl->cap->nr_bufs > FIMC_PHYBUFS) {
		mutex_lock(&ctrl->v4l2_lock);
		fimc_add_inqueue(ctrl, b->index);
		mutex_unlock(&ctrl->v4l2_lock);
	}

	return 0;
}

int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct fimc_capinfo *cap;
	int pp, ret = 0;

	if (!ctrl->cap || !ctrl->cap->nr_bufs) {
		fimc_err("%s: Invalid capture setting.\n", __func__);
		return -EINVAL;
	}

	if (b->memory != V4L2_MEMORY_MMAP) {
		fimc_err("%s: invalid memory type\n", __func__);
		return -EINVAL;
	}

	cap = ctrl->cap;

	mutex_lock(&ctrl->v4l2_lock);

	if (ctrl->status != FIMC_STREAMON) {
		mutex_unlock(&ctrl->v4l2_lock);
		fimc_dbg("%s: FIMC is not active.\n", __func__);
		return -EINVAL;
	}

	/* find out the real index */
	pp = ((fimc_hwget_frame_count(ctrl) + 2) % 4) % cap->nr_bufs;

	/* We have read the latest frame, hence should reset availability
	 * flag
	 */
	cap->irq = 0;

	/* skip even frame: no data */
	if (cap->fmt.field == V4L2_FIELD_INTERLACED_TB)
		pp &= ~0x1;

	if (cap->nr_bufs > FIMC_PHYBUFS) {
		b->index = cap->outq[pp];
		ret = fimc_add_outqueue(ctrl, pp);
		if (ret) {
			b->index = -1;
			fimc_err("%s: no inqueue buffer\n", __func__);
		}
	} else {
		b->index = pp;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	/* fimc_dbg("%s: buf_index = %d\n", __func__, b->index); */

	return ret;
}

