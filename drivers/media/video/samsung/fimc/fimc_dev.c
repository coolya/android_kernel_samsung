/* linux/drivers/media/video/samsung/fimc/fimc_dev.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Core file for Samsung Camera Interface (FIMC) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <media/v4l2-device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <mach/media.h>
#include <plat/fimc.h>
#include <linux/videodev2_samsung.h>
#include <linux/delay.h>
#include <plat/regs-fimc.h>

#include "fimc.h"

struct fimc_global *fimc_dev;

int fimc_dma_alloc(struct fimc_control *ctrl, struct fimc_buf_set *bs,
							int i, int align)
{
	dma_addr_t end, *curr;

	mutex_lock(&ctrl->alloc_lock);

	end = ctrl->mem.base + ctrl->mem.size;
	curr = &ctrl->mem.curr;

	if (!bs->length[i])
		return -EINVAL;

	if (!align) {
		if (*curr + bs->length[i] > end) {
			goto overflow;
		} else {
			bs->base[i] = *curr;
			bs->garbage[i] = 0;
			*curr += bs->length[i];
		}
	} else {
		if (ALIGN(*curr, align) + bs->length[i] > end)
			goto overflow;
		else {
			bs->base[i] = ALIGN(*curr, align);
			bs->garbage[i] = ALIGN(*curr, align) - *curr;
			*curr += (bs->length[i] + bs->garbage[i]);
		}
	}

	mutex_unlock(&ctrl->alloc_lock);

	return 0;

overflow:
	bs->base[i] = 0;
	bs->length[i] = 0;
	bs->garbage[i] = 0;

	mutex_unlock(&ctrl->alloc_lock);

	return -ENOMEM;
}

void fimc_dma_free(struct fimc_control *ctrl, struct fimc_buf_set *bs, int i)
{
	int total = bs->length[i] + bs->garbage[i];
	mutex_lock(&ctrl->alloc_lock);

	if (bs->base[i]) {
		if (ctrl->mem.curr - total >= ctrl->mem.base)
			ctrl->mem.curr -= total;

		bs->base[i] = 0;
		bs->length[i] = 0;
		bs->garbage[i] = 0;
	}

	mutex_unlock(&ctrl->alloc_lock);
}

void fimc_clk_en(struct fimc_control *ctrl, bool on)
{
	struct platform_device *pdev;
	struct s3c_platform_fimc *pdata;
	struct clk *lclk;

	pdev = to_platform_device(ctrl->dev);
	pdata = to_fimc_plat(ctrl->dev);
	lclk = clk_get(&pdev->dev, pdata->lclk_name);

	if (on) {
		if (!lclk->usage) {
			if (!ctrl->out)
				fimc_info1("(%d) Clock %s(%d) enabled.\n",
					ctrl->id, ctrl->clk->name,
					ctrl->clk->id);

			/* Turn on fimc power domain regulator */
			regulator_enable(ctrl->regulator);
			clk_enable(lclk);
		}
	} else {
		while (lclk->usage > 0) {
			if (!ctrl->out)
				fimc_info1("(%d) Clock %s(%d) disabled.\n",
					ctrl->id, ctrl->clk->name,
					ctrl->clk->id);
			clk_disable(lclk);
			/* Turn off fimc power domain regulator */
			regulator_disable(ctrl->regulator);
		}
	}

}

static inline u32 fimc_irq_out_single_buf(struct fimc_control *ctrl,
					  struct fimc_ctx *ctx)
{
	int ret = -1, ctx_num, next;
	u32 wakeup = 1;

	if (ctx->status == FIMC_READY_OFF) {
		ctrl->out->idxs.active.ctx = -1;
		ctrl->out->idxs.active.idx = -1;
		ctx->status = FIMC_STREAMOFF;
		ctrl->status = FIMC_STREAMOFF;

		return wakeup;
	}

	ctx->status = FIMC_STREAMON_IDLE;

	/* Attach done buffer to outgoing queue. */
	ret = fimc_push_outq(ctrl, ctx, ctrl->out->idxs.active.idx);
	if (ret < 0)
		fimc_err("Failed: fimc_push_outq\n");

	/* Detach buffer from incomming queue. */
	ret = fimc_pop_inq(ctrl, &ctx_num, &next);
	if (ret == 0) {		/* There is a buffer in incomming queue. */
		if (ctx_num != ctrl->out->last_ctx) {
			ctx = &ctrl->out->ctx[ctx_num];
			ctrl->out->last_ctx = ctx->ctx_num;
			fimc_outdev_set_ctx_param(ctrl, ctx);
		}

		fimc_outdev_set_src_addr(ctrl, ctx->src[next].base);

		fimc_output_set_dst_addr(ctrl, ctx, next);

		ret = fimc_outdev_start_camif(ctrl);
		if (ret < 0)
			fimc_err("Fail: fimc_start_camif\n");

		ctrl->out->idxs.active.ctx = ctx_num;
		ctrl->out->idxs.active.idx = next;
		ctx->status = FIMC_STREAMON;
		ctrl->status = FIMC_STREAMON;
	} else {	/* There is no buffer in incomming queue. */
		ctrl->out->idxs.active.ctx = -1;
		ctrl->out->idxs.active.idx = -1;
		ctx->status = FIMC_STREAMON_IDLE;
		ctrl->status = FIMC_STREAMON_IDLE;
	}

	return wakeup;
}

static inline u32 fimc_irq_out_multi_buf(struct fimc_control *ctrl,
					 struct fimc_ctx *ctx)
{
	int ret = -1, ctx_num, next;
	u32 wakeup = 1;

	if (ctx->status == FIMC_READY_OFF) {
		if (ctrl->out->idxs.active.ctx == ctx->ctx_num) {
			ctrl->out->idxs.active.ctx = -1;
			ctrl->out->idxs.active.idx = -1;
		}

		ctx->status = FIMC_STREAMOFF;

		return wakeup;
	}

	/* Attach done buffer to outgoing queue. */
	ret = fimc_push_outq(ctrl, ctx, ctrl->out->idxs.active.idx);
	if (ret < 0)
		fimc_err("Failed: fimc_push_outq\n");

	/* Detach buffer from incomming queue. */
	ret = fimc_pop_inq(ctrl, &ctx_num, &next);
	if (ret == 0) {		/* There is a buffer in incomming queue. */
		if (ctx_num != ctrl->out->last_ctx) {
			ctx = &ctrl->out->ctx[ctx_num];
			ctrl->out->last_ctx = ctx->ctx_num;
			fimc_outdev_set_ctx_param(ctrl, ctx);
		}

		fimc_outdev_set_src_addr(ctrl, ctx->src[next].base);

		fimc_output_set_dst_addr(ctrl, ctx, next);

		ret = fimc_outdev_start_camif(ctrl);
		if (ret < 0)
			fimc_err("Fail: fimc_start_camif\n");

		ctrl->out->idxs.active.ctx = ctx_num;
		ctrl->out->idxs.active.idx = next;
		ctx->status = FIMC_STREAMON;
		ctrl->status = FIMC_STREAMON;
	} else {	/* There is no buffer in incomming queue. */
		ctrl->out->idxs.active.ctx = -1;
		ctrl->out->idxs.active.idx = -1;
		ctx->status = FIMC_STREAMON_IDLE;
		ctrl->status = FIMC_STREAMON_IDLE;
	}

	return wakeup;
}

static inline u32 fimc_irq_out_dma(struct fimc_control *ctrl,
				   struct fimc_ctx *ctx)
{
	struct fimc_buf_set buf_set;
	int idx = ctrl->out->idxs.active.idx;
	int ret = -1, i, ctx_num, next;
	u32 wakeup = 1;

	if (ctx->status == FIMC_READY_OFF) {
		ctrl->out->idxs.active.ctx = -1;
		ctrl->out->idxs.active.idx = -1;
		ctx->status = FIMC_STREAMOFF;
		ctrl->status = FIMC_STREAMOFF;
		return wakeup;
	}

	/* Attach done buffer to outgoing queue. */
	ret = fimc_push_outq(ctrl, ctx, idx);
	if (ret < 0)
		fimc_err("Failed: fimc_push_outq\n");

	if (ctx->overlay.mode == FIMC_OVLY_DMA_AUTO) {
		struct s3cfb_window *win;
		struct fb_info *fbinfo;

		fbinfo = registered_fb[ctx->overlay.fb_id];
		win = (struct s3cfb_window *)fbinfo->par;

		win->other_mem_addr = ctx->dst[idx].base[FIMC_ADDR_Y];

		ret = fb_pan_display(fbinfo, &fbinfo->var);
		if (ret < 0) {
			fimc_err("%s: fb_pan_display fail (ret=%d)\n",
					__func__, ret);
			return -EINVAL;
		}
	}

	/* Detach buffer from incomming queue. */
	ret = fimc_pop_inq(ctrl, &ctx_num, &next);
	if (ret == 0) {		/* There is a buffer in incomming queue. */
		ctx = &ctrl->out->ctx[ctx_num];
		fimc_outdev_set_src_addr(ctrl, ctx->src[next].base);

		memset(&buf_set, 0x00, sizeof(buf_set));
		buf_set.base[FIMC_ADDR_Y] = ctx->dst[next].base[FIMC_ADDR_Y];

		for (i = 0; i < FIMC_PHYBUFS; i++)
			fimc_hwset_output_address(ctrl, &buf_set, i);

		ret = fimc_outdev_start_camif(ctrl);
		if (ret < 0)
			fimc_err("Fail: fimc_start_camif\n");

		ctrl->out->idxs.active.ctx = ctx_num;
		ctrl->out->idxs.active.idx = next;

		ctx->status = FIMC_STREAMON;
		ctrl->status = FIMC_STREAMON;
	} else {		/* There is no buffer in incomming queue. */
		ctrl->out->idxs.active.ctx = -1;
		ctrl->out->idxs.active.idx = -1;

		ctx->status = FIMC_STREAMON_IDLE;
		ctrl->status = FIMC_STREAMON_IDLE;
	}

	return wakeup;
}

static inline u32 fimc_irq_out_fimd(struct fimc_control *ctrl,
				    struct fimc_ctx *ctx)
{
	struct fimc_idx prev;
	int ret = -1, ctx_num, next;
	u32 wakeup = 0;

	/* Attach done buffer to outgoing queue. */
	if (ctrl->out->idxs.prev.idx != -1) {
		ret = fimc_push_outq(ctrl, ctx, ctrl->out->idxs.prev.idx);
		if (ret < 0) {
			fimc_err("Failed: fimc_push_outq\n");
		} else {
			ctrl->out->idxs.prev.ctx = -1;
			ctrl->out->idxs.prev.idx = -1;
			wakeup = 1;	/* To wake up fimc_v4l2_dqbuf */
		}
	}

	/* Update index structure. */
	if (ctrl->out->idxs.next.idx != -1) {
		ctrl->out->idxs.active.ctx = ctrl->out->idxs.next.ctx;
		ctrl->out->idxs.active.idx = ctrl->out->idxs.next.idx;
		ctrl->out->idxs.next.idx = -1;
		ctrl->out->idxs.next.ctx = -1;
	}

	/* Detach buffer from incomming queue. */
	ret = fimc_pop_inq(ctrl, &ctx_num, &next);
	if (ret == 0) { /* There is a buffer in incomming queue. */
		prev.ctx = ctrl->out->idxs.active.ctx;
		prev.idx = ctrl->out->idxs.active.idx;

		ctrl->out->idxs.prev.ctx = prev.ctx;
		ctrl->out->idxs.prev.idx = prev.idx;

		ctrl->out->idxs.next.ctx = ctx_num;
		ctrl->out->idxs.next.idx = next;

		/* set source address */
		fimc_outdev_set_src_addr(ctrl, ctx->src[next].base);
	}

	return wakeup;
}

static inline void fimc_irq_out(struct fimc_control *ctrl)
{
	struct fimc_ctx *ctx;
	u32 wakeup = 1;
	int ctx_num = ctrl->out->idxs.active.ctx;
	ctx = &ctrl->out->ctx[ctx_num];

	/* Interrupt pendding clear */
	fimc_hwset_clear_irq(ctrl);

	switch (ctx->overlay.mode) {
	case FIMC_OVLY_NONE_SINGLE_BUF:
		wakeup = fimc_irq_out_single_buf(ctrl, ctx);
		break;
	case FIMC_OVLY_NONE_MULTI_BUF:
		wakeup = fimc_irq_out_multi_buf(ctrl, ctx);
		break;
	case FIMC_OVLY_DMA_AUTO:	/* fall through */
	case FIMC_OVLY_DMA_MANUAL:
		wakeup = fimc_irq_out_dma(ctrl, ctx);
		break;
	default:
		break;
	}

	if (wakeup == 1)
		wake_up(&ctrl->wq);
}

static inline void fimc_irq_cap(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int pp;
	u32 cfg;

	fimc_hwset_clear_irq(ctrl);
	if (fimc_hwget_overflow_state(ctrl)) {
		/* s/w reset -- added for recovering module in ESD state*/
		cfg = readl(ctrl->regs + S3C_CIGCTRL);
		cfg |= (S3C_CIGCTRL_SWRST);
		writel(cfg, ctrl->regs + S3C_CIGCTRL);
		msleep(1);

		cfg = readl(ctrl->regs + S3C_CIGCTRL);
		cfg &= ~S3C_CIGCTRL_SWRST;
		writel(cfg, ctrl->regs + S3C_CIGCTRL);
	}
	pp = ((fimc_hwget_frame_count(ctrl) + 2) % 4);
	if (cap->fmt.field == V4L2_FIELD_INTERLACED_TB) {
		/* odd value of pp means one frame is made with top/bottom */
		if (pp & 0x1) {
			cap->irq = 1;
			wake_up(&ctrl->wq);
		}
	} else {
		cap->irq = 1;
		wake_up(&ctrl->wq);
	}
}

static irqreturn_t fimc_irq(int irq, void *dev_id)
{
	struct fimc_control *ctrl = (struct fimc_control *) dev_id;

	if (ctrl->cap)
		fimc_irq_cap(ctrl);
	else if (ctrl->out)
		fimc_irq_out(ctrl);

	return IRQ_HANDLED;
}

static
struct fimc_control *fimc_register_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct resource *res;
	int id, mdev_id;

	id = pdev->id;
	mdev_id = S5P_MDEV_FIMC0 + id;
	pdata = to_fimc_plat(&pdev->dev);

	ctrl = get_fimc_ctrl(id);
	ctrl->id = id;
	ctrl->dev = &pdev->dev;
	ctrl->vd = &fimc_video_device[id];
	ctrl->vd->minor = id;

	/* alloc from bank1 as default */
	ctrl->mem.base = pdata->pmem_start;
	ctrl->mem.size = pdata->pmem_size;
	ctrl->mem.curr = ctrl->mem.base;

	ctrl->status = FIMC_STREAMOFF;
	switch (pdata->hw_ver) {
	case 0x40:
		ctrl->limit = &fimc40_limits[id];
		break;
	case 0x43:
	case 0x45:
		ctrl->limit = &fimc43_limits[id];
		break;
	case 0x50:
		ctrl->limit = &fimc50_limits[id];
		break;
	}

	ctrl->log = FIMC_LOG_DEFAULT;

	sprintf(ctrl->name, "%s%d", FIMC_NAME, id);
	strcpy(ctrl->vd->name, ctrl->name);

	atomic_set(&ctrl->in_use, 0);
	mutex_init(&ctrl->lock);
	mutex_init(&ctrl->alloc_lock);
	mutex_init(&ctrl->v4l2_lock);
	init_waitqueue_head(&ctrl->wq);

	/* get resource for io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		fimc_err("%s: failed to get io memory region\n", __func__);
		return NULL;
	}

	/* request mem region */
	res = request_mem_region(res->start, res->end - res->start + 1,
			pdev->name);
	if (!res) {
		fimc_err("%s: failed to request io memory region\n", __func__);
		return NULL;
	}

	/* ioremap for register block */
	ctrl->regs = ioremap(res->start, res->end - res->start + 1);
	if (!ctrl->regs) {
		fimc_err("%s: failed to remap io region\n", __func__);
		return NULL;
	}

	/* irq */
	ctrl->irq = platform_get_irq(pdev, 0);
	if (request_irq(ctrl->irq, fimc_irq, IRQF_DISABLED, ctrl->name, ctrl))
		fimc_err("%s: request_irq failed\n", __func__);

	fimc_hwset_reset(ctrl);

	return ctrl;
}

static int fimc_unregister_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	int id = pdev->id;

	pdata = to_fimc_plat(&pdev->dev);
	ctrl = get_fimc_ctrl(id);

	free_irq(ctrl->irq, ctrl);
	mutex_destroy(&ctrl->lock);
	mutex_destroy(&ctrl->alloc_lock);
	mutex_destroy(&ctrl->v4l2_lock);

	fimc_clk_en(ctrl, false);

	iounmap(ctrl->regs);
	memset(ctrl, 0, sizeof(*ctrl));

	return 0;
}

static void fimc_mmap_open(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= pri_data / 0x100;
	u32 ctx		= (pri_data - (id * 0x100)) / 0x10;
	u32 idx		= pri_data % 0x10;

	atomic_inc(&dev->ctrl[id].out->ctx[ctx].src[idx].mapped_cnt);
}

static void fimc_mmap_close(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= pri_data / 0x100;
	u32 ctx		= (pri_data - (id * 0x100)) / 0x10;
	u32 idx		= pri_data % 0x10;

	atomic_dec(&dev->ctrl[id].out->ctx[ctx].src[idx].mapped_cnt);
}

static struct vm_operations_struct fimc_mmap_ops = {
	.open	= fimc_mmap_open,
	.close	= fimc_mmap_close,
};

static inline
int fimc_mmap_out_src(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_prv_data *prv_data =
				(struct fimc_prv_data *)filp->private_data;
	struct fimc_control *ctrl = prv_data->ctrl;
	int ctx_id = prv_data->ctx_id;
	struct fimc_ctx *ctx = &ctrl->out->ctx[ctx_id];
	u32 start_phy_addr = 0;
	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, idx = vma->vm_pgoff;
	u32 buf_length = 0;
	int pri_data = 0;

	buf_length = PAGE_ALIGN(ctx->src[idx].length[FIMC_ADDR_Y] +
				ctx->src[idx].length[FIMC_ADDR_CB] +
				ctx->src[idx].length[FIMC_ADDR_CR]);
	if (size > PAGE_ALIGN(buf_length)) {
		fimc_err("Requested mmap size is too big\n");
		return -EINVAL;
	}

	pri_data = (ctrl->id * 0x100) + (ctx_id * 0x10) + idx;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &fimc_mmap_ops;
	vma->vm_private_data = (void *)pri_data;

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		fimc_err("writable mapping must be shared\n");
		return -EINVAL;
	}

	start_phy_addr = ctx->src[idx].base[FIMC_ADDR_Y];
	pfn = __phys_to_pfn(start_phy_addr);

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		fimc_err("mmap fail\n");
		return -EINVAL;
	}

	vma->vm_ops->open(vma);

	ctx->src[idx].flags |= V4L2_BUF_FLAG_MAPPED;

	return 0;
}

static inline
int fimc_mmap_out_dst(struct file *filp, struct vm_area_struct *vma, u32 idx)
{
	struct fimc_prv_data *prv_data =
				(struct fimc_prv_data *)filp->private_data;
	struct fimc_control *ctrl = prv_data->ctrl;
	int ctx_id = prv_data->ctx_id;
	unsigned long pfn = 0, size;
	int ret = 0;

	size = vma->vm_end - vma->vm_start;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;

	pfn = __phys_to_pfn(ctrl->out->ctx[ctx_id].dst[idx].base[0]);
	ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
	if (ret != 0)
		fimc_err("remap_pfn_range fail.\n");

	return ret;
}

static inline int fimc_mmap_out(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_prv_data *prv_data =
				(struct fimc_prv_data *)filp->private_data;
	struct fimc_control *ctrl = prv_data->ctrl;
	int ctx_id = prv_data->ctx_id;
	int idx = ctrl->out->ctx[ctx_id].overlay.req_idx;
	int ret = -1;

	if (idx >= 0)
		ret = fimc_mmap_out_dst(filp, vma, idx);
	else if (idx == FIMC_MMAP_IDX)
		ret = fimc_mmap_out_src(filp, vma);

	return ret;
}

static inline int fimc_mmap_cap(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_prv_data *prv_data =
				(struct fimc_prv_data *)filp->private_data;
	struct fimc_control *ctrl = prv_data->ctrl;
	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, idx = vma->vm_pgoff;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;

	/*
	 * page frame number of the address for a source frame
	 * to be stored at.
	 */
	pfn = __phys_to_pfn(ctrl->cap->bufs[idx].base[0]);

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		fimc_err("%s: writable mapping must be shared\n", __func__);
		return -EINVAL;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		fimc_err("%s: mmap fail\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fimc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_prv_data *prv_data =
				(struct fimc_prv_data *)filp->private_data;
	struct fimc_control *ctrl = prv_data->ctrl;
	int ret;

	if (ctrl->cap)
		ret = fimc_mmap_cap(filp, vma);
	else
		ret = fimc_mmap_out(filp, vma);

	return ret;
}

static u32 fimc_poll(struct file *filp, poll_table *wait)
{
	struct fimc_prv_data *prv_data =
				(struct fimc_prv_data *)filp->private_data;
	struct fimc_control *ctrl = prv_data->ctrl;
	struct fimc_capinfo *cap = ctrl->cap;
	u32 mask = 0;

	if (cap) {
		if (cap->irq || (ctrl->status != FIMC_STREAMON)) {
			mask = POLLIN | POLLRDNORM;
			cap->irq = 0;
		} else {
			poll_wait(filp, &ctrl->wq, wait);
		}
	}

	return mask;
}

static
ssize_t fimc_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
	return 0;
}

static
ssize_t fimc_write(struct file *filp, const char *b, size_t c, loff_t *offset)
{
	return 0;
}

u32 fimc_mapping_rot_flip(u32 rot, u32 flip)
{
	u32 ret = 0;

	switch (rot) {
	case 0:
		if (flip & FIMC_XFLIP)
			ret |= FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 90:
		ret = FIMC_ROT;
		if (flip & FIMC_XFLIP)
			ret |= FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 180:
		ret = (FIMC_XFLIP | FIMC_YFLIP);
		if (flip & FIMC_XFLIP)
			ret &= ~FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret &= ~FIMC_YFLIP;
		break;

	case 270:
		ret = (FIMC_XFLIP | FIMC_YFLIP | FIMC_ROT);
		if (flip & FIMC_XFLIP)
			ret &= ~FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret &= ~FIMC_YFLIP;
		break;
	}

	return ret;
}

int fimc_get_scaler_factor(u32 src, u32 tar, u32 *ratio, u32 *shift)
{
	if (src >= tar * 64) {
		return -EINVAL;
	} else if (src >= tar * 32) {
		*ratio = 32;
		*shift = 5;
	} else if (src >= tar * 16) {
		*ratio = 16;
		*shift = 4;
	} else if (src >= tar * 8) {
		*ratio = 8;
		*shift = 3;
	} else if (src >= tar * 4) {
		*ratio = 4;
		*shift = 2;
	} else if (src >= tar * 2) {
		*ratio = 2;
		*shift = 1;
	} else {
		*ratio = 1;
		*shift = 0;
	}

	return 0;
}

void fimc_get_nv12t_size(int img_hres, int img_vres,
				int *y_size, int *cb_size)
{
	int remain;
	int y_hres_byte, y_vres_byte;
	int cb_hres_byte, cb_vres_byte;
	int y_hres_roundup, y_vres_roundup;
	int cb_hres_roundup, cb_vres_roundup;

	/* to make 'img_hres and img_vres' be 16 multiple */
	remain = img_hres % 16;
	if (remain != 0) {
		remain = 16 - remain;
		img_hres = img_hres + remain;
	}
	remain = img_vres % 16;
	if (remain != 0) {
		remain = 16 - remain;
		img_vres = img_vres + remain;
	}

	cb_hres_byte = img_hres;
	cb_vres_byte = img_vres;

	y_hres_byte = img_hres - 1;
	y_vres_byte = img_vres - 1;
	y_hres_roundup = ((y_hres_byte >> 4) >> 3) + 1;
	y_vres_roundup = ((y_vres_byte >> 4) >> 2) + 1;
	if ((y_vres_byte & 0x20) == 0) {
		y_hres_byte = y_hres_byte & 0x7f00;
		y_hres_byte = y_hres_byte >> 8;
		y_hres_byte = y_hres_byte & 0x7f;

		y_vres_byte = y_vres_byte & 0x7fc0;
		y_vres_byte = y_vres_byte >> 6;
		y_vres_byte = y_vres_byte & 0x1ff;

		*y_size = y_hres_byte +\
		(y_vres_byte * y_hres_roundup) + 1;
	} else {
		*y_size = y_hres_roundup * y_vres_roundup;
	}

	*y_size = *(y_size) << 13;

	cb_hres_byte = img_hres - 1;
	cb_vres_byte = (img_vres >> 1) - 1;
	cb_hres_roundup = ((cb_hres_byte >> 4) >> 3) + 1;
	cb_vres_roundup = ((cb_vres_byte >> 4) >> 2) + 1;
	if ((cb_vres_byte & 0x20) == 0) {
		cb_hres_byte = cb_hres_byte & 0x7f00;
		cb_hres_byte = cb_hres_byte >> 8;
		cb_hres_byte = cb_hres_byte & 0x7f;

		cb_vres_byte = cb_vres_byte & 0x7fc0;
		cb_vres_byte = cb_vres_byte >> 6;
		cb_vres_byte = cb_vres_byte & 0x1ff;

		*cb_size = cb_hres_byte + (cb_vres_byte * cb_hres_roundup) + 1;
	} else {
		*cb_size = cb_hres_roundup * cb_vres_roundup;
	}
	*cb_size = (*cb_size) << 13;

}

static int fimc_get_free_ctx(struct fimc_control *ctrl)
{
	int i;

	if (1 != ctrl->id)
		return 0;

	for (i = 0; i < FIMC_MAX_CTXS; i++) {
		if (ctrl->ctx_busy[i] == 0) {
			ctrl->ctx_busy[i] = 1;
			fimc_info1("Current context is %d\n", i);
			return i;
		}
	}

	return -1;
}

static int fimc_open(struct file *filp)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	struct fimc_prv_data *prv_data;
	int in_use;
	int ret;

	ctrl = video_get_drvdata(video_devdata(filp));
	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	in_use = atomic_read(&ctrl->in_use);
	if (in_use >= FIMC_MAX_CTXS || (in_use && 1 != ctrl->id)) {
		fimc_err("%s: Device busy.\n", __func__);
		ret = -EBUSY;
		goto resource_busy;
	} else {
		atomic_inc(&ctrl->in_use);
	}
	in_use = atomic_read(&ctrl->in_use);

	prv_data = kzalloc(sizeof(struct fimc_prv_data), GFP_KERNEL);
	if (!prv_data) {
		fimc_err("%s: not enough memory\n", __func__);
		ret = -ENOMEM;
		goto kzalloc_err;
	}

	prv_data->ctx_id = fimc_get_free_ctx(ctrl);
	if (prv_data->ctx_id < 0) {
		fimc_err("%s: Context busy flag not reset.\n", __func__);
		ret = -EBUSY;
		goto ctx_err;
	}
	prv_data->ctrl = ctrl;
	filp->private_data = prv_data;

	if (in_use == 1) {
		fimc_clk_en(ctrl, true);

		if (pdata->hw_ver == 0x40)
			fimc_hw_reset_camera(ctrl);

		/* Apply things to interface register */
		fimc_hwset_reset(ctrl);

		if (num_registered_fb > 0) {
			struct fb_info *fbinfo = registered_fb[0];
			ctrl->fb.lcd_hres = (int)fbinfo->var.xres;
			ctrl->fb.lcd_vres = (int)fbinfo->var.yres;
			fimc_info1("%s: fd.lcd_hres=%d fd.lcd_vres=%d\n",
					__func__, ctrl->fb.lcd_hres,
					ctrl->fb.lcd_vres);
		}

		ctrl->mem.curr = ctrl->mem.base;
		ctrl->status = FIMC_STREAMOFF;

		if (0 != ctrl->id)
			fimc_clk_en(ctrl, false);
	}

	mutex_unlock(&ctrl->lock);

	fimc_info1("%s opened.\n", ctrl->name);

	return 0;

ctx_err:
	kfree(prv_data);

kzalloc_err:
	atomic_dec(&ctrl->in_use);

resource_busy:
	mutex_unlock(&ctrl->lock);
	return ret;
}

static int fimc_release(struct file *filp)
{
	struct fimc_prv_data *prv_data =
				(struct fimc_prv_data *)filp->private_data;
	struct fimc_control *ctrl = prv_data->ctrl;
	int ctx_id = prv_data->ctx_id;
	struct s3c_platform_fimc *pdata;
	struct fimc_overlay_buf *buf;
	struct mm_struct *mm = current->mm;
	struct fimc_ctx *ctx;
	int ret = 0, i;
	ctx = &ctrl->out->ctx[ctx_id];

	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);
	atomic_dec(&ctrl->in_use);

	/* FIXME: turning off actual working camera */
	if (ctrl->cam && ctrl->id != 2) {
		/* Unload the subdev (camera sensor) module,
		 * reset related status flags
		 */
		fimc_release_subdev(ctrl);
	}

	if (ctrl->cap) {
		ctrl->mem.curr = ctrl->mem.base;
		kfree(filp->private_data);
		filp->private_data = NULL;

		for (i = 0; i < FIMC_CAPBUFS; i++) {
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 0);
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 1);
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 2);
		}

		fimc_clk_en(ctrl, false);

		kfree(ctrl->cap);
		ctrl->cap = NULL;
	}

	if (ctrl->out) {
		if (ctx->status != FIMC_STREAMOFF) {
			fimc_clk_en(ctrl, true);
			ret = fimc_outdev_stop_streaming(ctrl, ctx);
			fimc_clk_en(ctrl, false);
			if (ret < 0)
				fimc_err("Fail: fimc_stop_streaming\n");

			ret = fimc_init_in_queue(ctrl, ctx);
			if (ret < 0) {
				fimc_err("Fail: fimc_init_in_queue\n");
				ret = -EINVAL;
				goto release_err;
			}

			ret = fimc_init_out_queue(ctrl, ctx);
			if (ret < 0) {
				fimc_err("Fail: fimc_init_out_queue\n");
				ret = -EINVAL;
				goto release_err;
			}

			/* Make all buffers DQUEUED state. */
			for (i = 0; i < FIMC_OUTBUFS; i++) {
				ctx->src[i].state = VIDEOBUF_IDLE;
				ctx->src[i].flags = V4L2_BUF_FLAG_MAPPED;
			}

			if (ctx->overlay.mode == FIMC_OVLY_DMA_AUTO) {
				ctrl->mem.curr = ctx->dst[0].base[FIMC_ADDR_Y];

				for (i = 0; i < FIMC_OUTBUFS; i++) {
					ctx->dst[i].base[FIMC_ADDR_Y] = 0;
					ctx->dst[i].length[FIMC_ADDR_Y] = 0;

					ctx->dst[i].base[FIMC_ADDR_CB] = 0;
					ctx->dst[i].length[FIMC_ADDR_CB] = 0;

					ctx->dst[i].base[FIMC_ADDR_CR] = 0;
					ctx->dst[i].length[FIMC_ADDR_CR] = 0;
				}
			}

			ctx->status = FIMC_STREAMOFF;
		}

		buf = &ctx->overlay.buf;
		for (i = 0; i < FIMC_OUTBUFS; i++) {
			if (buf->vir_addr[i]) {
				ret = do_munmap(mm, buf->vir_addr[i],
						buf->size[i]);
				if (ret < 0)
					fimc_err("%s: do_munmap fail\n", \
							__func__);
			}
		}

		ctrl->ctx_busy[ctx_id] = 0;
		memset(ctx, 0x00, sizeof(struct fimc_ctx));

		if (atomic_read(&ctrl->in_use) == 0) {
			ctrl->status = FIMC_STREAMOFF;
			fimc_outdev_init_idxs(ctrl);

			fimc_clk_en(ctrl, false);

			ctrl->mem.curr = ctrl->mem.base;

			kfree(ctrl->out);
			ctrl->out = NULL;

			kfree(filp->private_data);
			filp->private_data = NULL;
		}
	}

	/*
	 * it remain afterimage when I play movie using overlay and exit
	 */
	if (ctrl->fb.is_enable == 1) {
		fimc_info2("WIN_OFF for FIMC%d\n", ctrl->id);
		ret = fb_blank(registered_fb[ctx->overlay.fb_id],
				FB_BLANK_POWERDOWN);
		if (ret < 0) {
			fimc_err("%s: fb_blank: fb[%d] " \
					"mode=FB_BLANK_POWERDOWN\n",
					__func__, ctx->overlay.fb_id);
			ret = -EINVAL;
			goto release_err;
		}

		ctrl->fb.is_enable = 0;
	}

	mutex_unlock(&ctrl->lock);

	fimc_info1("%s released.\n", ctrl->name);

	return 0;

release_err:
	mutex_unlock(&ctrl->lock);
	return ret;

}

static const struct v4l2_file_operations fimc_fops = {
	.owner		= THIS_MODULE,
	.open		= fimc_open,
	.release	= fimc_release,
	.ioctl		= video_ioctl2,
	.read		= fimc_read,
	.write		= fimc_write,
	.mmap		= fimc_mmap,
	.poll		= fimc_poll,
};

static void fimc_vdev_release(struct video_device *vdev)
{
	kfree(vdev);
}

struct video_device fimc_video_device[FIMC_DEVICES] = {
	[0] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release = fimc_vdev_release,
	},
	[1] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release = fimc_vdev_release,
	},
	[2] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release = fimc_vdev_release,
	},
};

static int fimc_init_global(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct s3c_platform_camera *cam;
	int i;

	pdata = to_fimc_plat(&pdev->dev);

	/* Registering external camera modules. re-arrange order to be sure */
	for (i = 0; i < FIMC_MAXCAMS; i++) {
		cam = pdata->camera[i];
		if (!cam)
			break;

		cam->srclk = clk_get(&pdev->dev, cam->srclk_name);
		if (IS_ERR(cam->srclk)) {
			dev_err(&pdev->dev, "%s: failed to get mclk source\n",
					__func__);
			return -EINVAL;
		}

		/* mclk */
		cam->clk = clk_get(&pdev->dev, cam->clk_name);
		if (IS_ERR(cam->clk)) {
			dev_err(&pdev->dev, "%s: failed to get mclk source\n",
					__func__);
			clk_put(cam->srclk);
			return -EINVAL;
		}

		clk_put(cam->clk);
		clk_put(cam->srclk);

		/* Assign camera device to fimc */
		memcpy(&fimc_dev->camera[i], cam, sizeof(*cam));
		fimc_dev->camera_isvalid[i] = 1;
		fimc_dev->camera[i].initialized = 0;
	}

	fimc_dev->active_camera = -1;
	fimc_dev->initialized = 1;

	return 0;
}

static int fimc_show_log_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_control *ctrl;
	struct platform_device *pdev;
	int id = -1;

	char temp[150];

	pdev = to_platform_device(dev);
	id = pdev->id;
	ctrl = get_fimc_ctrl(id);

	sprintf(temp, "\t");
	strcat(buf, temp);
	if (ctrl->log & FIMC_LOG_DEBUG) {
		sprintf(temp, "FIMC_LOG_DEBUG | ");
		strcat(buf, temp);
	}

	if (ctrl->log & FIMC_LOG_INFO_L2) {
		sprintf(temp, "FIMC_LOG_INFO_L2 | ");
		strcat(buf, temp);
	}

	if (ctrl->log & FIMC_LOG_INFO_L1) {
		sprintf(temp, "FIMC_LOG_INFO_L1 | ");
		strcat(buf, temp);
	}

	if (ctrl->log & FIMC_LOG_WARN) {
		sprintf(temp, "FIMC_LOG_WARN | ");
		strcat(buf, temp);
	}

	if (ctrl->log & FIMC_LOG_ERR) {
		sprintf(temp, "FIMC_LOG_ERR\n");
		strcat(buf, temp);
	}

	return strlen(buf);
}

static int fimc_store_log_level(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct fimc_control *ctrl;
	struct platform_device *pdev;

	const char *p = buf;
	char msg[150] = {0, };
	int id = -1;
	u32 match = 0;

	pdev = to_platform_device(dev);
	id = pdev->id;
	ctrl = get_fimc_ctrl(id);

	while (*p != '\0') {
		if (!isspace(*p))
			strncat(msg, p, 1);
		p++;
	}

	ctrl->log = 0;
	printk(KERN_INFO "FIMC.%d log level is set as below.\n", id);

	if (strstr(msg, "FIMC_LOG_ERR") != NULL) {
		ctrl->log |= FIMC_LOG_ERR;
		match = 1;
		printk(KERN_INFO "\tFIMC_LOG_ERR\n");
	}

	if (strstr(msg, "FIMC_LOG_WARN") != NULL) {
		ctrl->log |= FIMC_LOG_WARN;
		match = 1;
		printk(KERN_INFO "\tFIMC_LOG_WARN\n");
	}

	if (strstr(msg, "FIMC_LOG_INFO_L1") != NULL) {
		ctrl->log |= FIMC_LOG_INFO_L1;
		match = 1;
		printk(KERN_INFO "\tFIMC_LOG_INFO_L1\n");
	}

	if (strstr(msg, "FIMC_LOG_INFO_L2") != NULL) {
		ctrl->log |= FIMC_LOG_INFO_L2;
		match = 1;
		printk(KERN_INFO "\tFIMC_LOG_INFO_L2\n");
	}

	if (strstr(msg, "FIMC_LOG_DEBUG") != NULL) {
		ctrl->log |= FIMC_LOG_DEBUG;
		match = 1;
		printk(KERN_INFO "\tFIMC_LOG_DEBUG\n");
	}

	if (!match) {
		printk(KERN_INFO "FIMC_LOG_ERR		\t: Error condition.\n");
		printk(KERN_INFO "FIMC_LOG_WARN		\t: WARNING condition.\n");
		printk(KERN_INFO "FIMC_LOG_INFO_L1	\t: V4L2 API without QBUF, DQBUF.\n");
		printk(KERN_INFO "FIMC_LOG_INFO_L2	\t: V4L2 API QBUF, DQBUF.\n");
		printk(KERN_INFO "FIMC_LOG_DEBUG	\t: Queue status report.\n");
	}

	return len;
}

static DEVICE_ATTR(log_level, 0644, \
			fimc_show_log_level,
			fimc_store_log_level);

static int __devinit fimc_probe(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct clk *srclk;
	int ret;

	if (!fimc_dev) {
		fimc_dev = kzalloc(sizeof(*fimc_dev), GFP_KERNEL);
		if (!fimc_dev) {
			dev_err(&pdev->dev, "%s: not enough memory\n",
				__func__);
			return -ENOMEM;
		}
	}

	ctrl = fimc_register_controller(pdev);
	if (!ctrl) {
		printk(KERN_ERR "%s: cannot register fimc\n", __func__);
		goto err_alloc;
	}

	pdata = to_fimc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* Get fimc power domain regulator */
	ctrl->regulator = regulator_get(&pdev->dev, "pd");
	if (IS_ERR(ctrl->regulator)) {
		fimc_err("%s: failed to get resource %s\n",
				__func__, "s3c-fimc");
		return PTR_ERR(ctrl->regulator);
	}

	/* fimc source clock */
	srclk = clk_get(&pdev->dev, pdata->srclk_name);
	if (IS_ERR(srclk)) {
		fimc_err("%s: failed to get source clock of fimc\n",
				__func__);
		goto err_v4l2;
	}

	/* fimc clock */
	ctrl->clk = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(ctrl->clk)) {
		fimc_err("%s: failed to get fimc clock source\n",
			__func__);
		goto err_v4l2;
	}

	/* set parent for mclk */
	clk_set_parent(ctrl->clk, srclk);

	/* set rate for mclk */
	clk_set_rate(ctrl->clk, pdata->clk_rate);

	/* V4L2 device-subdev registration */
	ret = v4l2_device_register(&pdev->dev, &ctrl->v4l2_dev);
	if (ret) {
		fimc_err("%s: v4l2 device register failed\n", __func__);
		goto err_fimc;
	}

	/* things to initialize once */
	if (!fimc_dev->initialized) {
		ret = fimc_init_global(pdev);
		if (ret)
			goto err_v4l2;
	}

	/* video device register */
	ret = video_register_device(ctrl->vd, VFL_TYPE_GRABBER, ctrl->id);
	if (ret) {
		fimc_err("%s: cannot register video driver\n", __func__);
		goto err_v4l2;
	}

	video_set_drvdata(ctrl->vd, ctrl);

	ret = device_create_file(&(pdev->dev), &dev_attr_log_level);
	if (ret < 0) {
		fimc_err("failed to add sysfs entries\n");
		goto err_global;
	}
	printk(KERN_INFO "FIMC%d registered successfully\n", ctrl->id);

	return 0;

err_global:
	video_unregister_device(ctrl->vd);

err_v4l2:
	v4l2_device_unregister(&ctrl->v4l2_dev);

err_fimc:
	fimc_unregister_controller(pdev);

err_alloc:
	kfree(fimc_dev);
	return -EINVAL;

}

static int fimc_remove(struct platform_device *pdev)
{
	fimc_unregister_controller(pdev);

	device_remove_file(&(pdev->dev), &dev_attr_log_level);

	kfree(fimc_dev);
	fimc_dev = NULL;

	return 0;
}

#ifdef CONFIG_PM
static inline void fimc_suspend_out_ctx(struct fimc_control *ctrl,
					struct fimc_ctx *ctx)
{
	switch (ctx->overlay.mode) {
	case FIMC_OVLY_DMA_AUTO:		/* fall through */
	case FIMC_OVLY_DMA_MANUAL:		/* fall through */
	case FIMC_OVLY_NONE_MULTI_BUF:		/* fall through */
	case FIMC_OVLY_NONE_SINGLE_BUF:
		if (ctx->status == FIMC_STREAMON) {
			if (ctx->inq[0] != -1)
				fimc_err("%s : %d in queue unstable\n",
					__func__, __LINE__);

			fimc_outdev_stop_streaming(ctrl, ctx);
			ctx->status = FIMC_ON_SLEEP;
		} else if (ctx->status == FIMC_STREAMON_IDLE) {
			fimc_outdev_stop_streaming(ctrl, ctx);
			ctx->status = FIMC_ON_IDLE_SLEEP;
		} else {
			ctx->status = FIMC_OFF_SLEEP;
		}

		break;
	case FIMC_OVLY_NOT_FIXED:
		ctx->status = FIMC_OFF_SLEEP;
		break;
	}
}

static inline int fimc_suspend_out(struct fimc_control *ctrl)
{
	struct fimc_ctx *ctx;
	int i, on_sleep = 0, idle_sleep = 0, off_sleep = 0;

	for (i = 0; i < FIMC_MAX_CTXS; i++) {
		ctx = &ctrl->out->ctx[i];
		fimc_suspend_out_ctx(ctrl, ctx);

		switch (ctx->status) {
		case FIMC_ON_SLEEP:
			on_sleep++;
			break;
		case FIMC_ON_IDLE_SLEEP:
			idle_sleep++;
			break;
		case FIMC_OFF_SLEEP:
			off_sleep++;
			break;
		default:
			break;
		}
	}

	if (on_sleep)
		ctrl->status = FIMC_ON_SLEEP;
	else if (idle_sleep)
		ctrl->status = FIMC_ON_IDLE_SLEEP;
	else
		ctrl->status = FIMC_OFF_SLEEP;

	ctrl->out->last_ctx = -1;

	return 0;
}

static inline int fimc_suspend_cap(struct fimc_control *ctrl)
{
	if (ctrl->cam->id == CAMERA_WB && ctrl->status == FIMC_STREAMON)
		fimc_streamoff_capture((void *)ctrl);
	ctrl->status = FIMC_ON_SLEEP;

	return 0;
}

int fimc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int id;

	id = pdev->id;
	ctrl = get_fimc_ctrl(id);
	pdata = to_fimc_plat(ctrl->dev);

	if (ctrl->out)
		fimc_suspend_out(ctrl);

	else if (ctrl->cap)
		fimc_suspend_cap(ctrl);
	else
		ctrl->status = FIMC_OFF_SLEEP;

	if (atomic_read(&ctrl->in_use))
		fimc_clk_en(ctrl, false);

	return 0;
}

static inline void fimc_resume_out_ctx(struct fimc_control *ctrl,
				       struct fimc_ctx *ctx)
{
	int ret = -1;

	switch (ctx->overlay.mode) {
	case FIMC_OVLY_DMA_AUTO:
		if (ctx->status == FIMC_ON_IDLE_SLEEP) {
			fimc_outdev_resume_dma(ctrl, ctx);
			ret = fimc_outdev_set_ctx_param(ctrl, ctx);
			if (ret < 0)
				fimc_err("Fail: fimc_outdev_set_ctx_param\n");

			ctx->status = FIMC_STREAMON_IDLE;
		} else if (ctx->status == FIMC_OFF_SLEEP) {
			ctx->status = FIMC_STREAMOFF;
		} else {
			fimc_err("%s: Abnormal (%d)\n", __func__, ctx->status);
		}

		break;
	case FIMC_OVLY_DMA_MANUAL:
		if (ctx->status == FIMC_ON_IDLE_SLEEP) {
			ret = fimc_outdev_set_ctx_param(ctrl, ctx);
			if (ret < 0)
				fimc_err("Fail: fimc_outdev_set_ctx_param\n");

			ctx->status = FIMC_STREAMON_IDLE;

		} else if (ctx->status == FIMC_OFF_SLEEP) {
			ctx->status = FIMC_STREAMOFF;
		} else {
			fimc_err("%s: Abnormal (%d)\n", __func__, ctx->status);
		}

		break;
	case FIMC_OVLY_NONE_SINGLE_BUF:		/* fall through */
	case FIMC_OVLY_NONE_MULTI_BUF:
		if (ctx->status == FIMC_ON_IDLE_SLEEP) {
			ret = fimc_outdev_set_ctx_param(ctrl, ctx);
			if (ret < 0)
				fimc_err("Fail: fimc_outdev_set_ctx_param\n");

			ctx->status = FIMC_STREAMON_IDLE;
		} else if (ctx->status == FIMC_OFF_SLEEP) {
			ctx->status = FIMC_STREAMOFF;
		} else {
			fimc_err("%s: Abnormal (%d)\n", __func__, ctx->status);
		}

		break;
	default:
		ctx->status = FIMC_STREAMOFF;
		break;
	}
}

static inline int fimc_resume_out(struct fimc_control *ctrl)
{
	struct fimc_ctx *ctx;
	int i;
	u32 state = 0;

	for (i = 0; i < FIMC_MAX_CTXS; i++) {
		ctx = &ctrl->out->ctx[i];
		fimc_resume_out_ctx(ctrl, ctx);

		switch (ctx->status) {
		case FIMC_STREAMON:
			state |= FIMC_STREAMON;
			break;
		case FIMC_STREAMON_IDLE:
			state |= FIMC_STREAMON_IDLE;
			break;
		case FIMC_STREAMOFF:
			state |= FIMC_STREAMOFF;
			break;
		default:
			break;
		}
	}

	if ((state & FIMC_STREAMON) == FIMC_STREAMON)
		ctrl->status = FIMC_STREAMON;
	else if ((state & FIMC_STREAMON_IDLE) == FIMC_STREAMON_IDLE)
		ctrl->status = FIMC_STREAMON_IDLE;
	else
		ctrl->status = FIMC_STREAMOFF;

	return 0;
}

static inline int fimc_resume_cap(struct fimc_control *ctrl)
{
	if (ctrl->cam->id == CAMERA_WB)
		fimc_streamon_capture((void *)ctrl);

	return 0;
}

int fimc_resume(struct platform_device *pdev)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int id = pdev->id;

	ctrl = get_fimc_ctrl(id);
	pdata = to_fimc_plat(ctrl->dev);

	if (atomic_read(&ctrl->in_use))
		fimc_clk_en(ctrl, true);

	if (ctrl->out)
		fimc_resume_out(ctrl);

	else if (ctrl->cap)
		fimc_resume_cap(ctrl);
	else
		ctrl->status = FIMC_STREAMOFF;

	return 0;
}
#else
#define fimc_suspend	NULL
#define fimc_resume	NULL
#endif

static struct platform_driver fimc_driver = {
	.probe		= fimc_probe,
	.remove		= fimc_remove,
	.suspend	= fimc_suspend,
	.resume		= fimc_resume,
	.driver		= {
		.name	= FIMC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int fimc_register(void)
{
	platform_driver_register(&fimc_driver);

	return 0;
}

static void fimc_unregister(void)
{
	platform_driver_unregister(&fimc_driver);
}

late_initcall(fimc_register);
module_exit(fimc_unregister);

MODULE_AUTHOR("Dongsoo,	Kim <dongsoo45.kim@samsung.com>");
MODULE_AUTHOR("Jinsung,	Yang <jsgood.yang@samsung.com>");
MODULE_AUTHOR("Jonghun,	Han <jonghun.han@samsung.com>");
MODULE_DESCRIPTION("Samsung Camera Interface (FIMC) driver");
MODULE_LICENSE("GPL");
