/* linux/drivers/media/video/samsung/jpeg_v2/s3c-jpeg.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Core file for Samsung Jpeg Interface driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/page.h>
#include <mach/irqs.h>
#include <linux/semaphore.h>
#include <mach/map.h>
#include <mach/pd.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/platform_device.h>

#include <linux/version.h>
#include <plat/media.h>

#include <linux/time.h>
#include <linux/clk.h>

#include "s3c-jpeg.h"
#include "jpg_mem.h"
#include "jpg_misc.h"
#include "jpg_opr.h"
#include "regs-jpeg.h"

static struct clk		*s3c_jpeg_clk;

static struct resource		*s3c_jpeg_mem;
void __iomem			*s3c_jpeg_base;
static int			irq_no;
static int			instanceNo = 0;
volatile int			jpg_irq_reason;
wait_queue_head_t 		wait_queue_jpeg;


DECLARE_WAIT_QUEUE_HEAD(WaitQueue_JPEG);
irqreturn_t s3c_jpeg_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int	int_status;
	unsigned int	status;

	jpg_dbg("=====enter s3c_jpeg_irq===== \r\n");

	int_status = readl(s3c_jpeg_base + S3C_JPEG_INTST_REG);

	do{
		status = readl(s3c_jpeg_base + S3C_JPEG_OPR_REG);
	}while(status);

	writel(S3C_JPEG_COM_INT_RELEASE, s3c_jpeg_base + S3C_JPEG_COM_REG);
	jpg_dbg("int_status : 0x%08x status : 0x%08x\n", int_status, status);

	if (int_status) {
		switch (int_status) {
		case 0x40 :
			jpg_irq_reason = OK_ENC_OR_DEC;
			break;
		case 0x20 :
			jpg_irq_reason = ERR_ENC_OR_DEC;
			break;
		default :
			jpg_irq_reason = ERR_UNKNOWN;
		}

		wake_up_interruptible(&wait_queue_jpeg);
	} else {
		jpg_irq_reason = ERR_UNKNOWN;
		wake_up_interruptible(&wait_queue_jpeg);
	}

	return IRQ_HANDLED;
}
static int s3c_jpeg_open(struct inode *inode, struct file *file)
{
	sspc100_jpg_ctx *jpg_reg_ctx;
	DWORD	ret;

	ret = s5pv210_pd_enable("jpeg_pd");
	if (ret < 0) {
		jpg_err("failed to enable jpeg power domain\n");
		return FALSE;
	}
	
        /* clock enable */
	clk_enable(s3c_jpeg_clk);
	
	jpg_dbg("JPG_open \r\n");

	jpg_reg_ctx = (sspc100_jpg_ctx *)mem_alloc(sizeof(sspc100_jpg_ctx));
	memset(jpg_reg_ctx, 0x00, sizeof(sspc100_jpg_ctx));

	ret = lock_jpg_mutex();

	if (!ret) {
		jpg_err("JPG Mutex Lock Fail\r\n");
		unlock_jpg_mutex();
		kfree(jpg_reg_ctx);
		return FALSE;
	}

	if (instanceNo > MAX_INSTANCE_NUM) {
		jpg_err("Instance Number error-JPEG is running, \
				instance number is %d\n", instanceNo);
		unlock_jpg_mutex();
		kfree(jpg_reg_ctx);
		return FALSE;
	}

	instanceNo++;

	unlock_jpg_mutex();

	file->private_data = (sspc100_jpg_ctx *)jpg_reg_ctx;

	return 0;
}


static int s3c_jpeg_release(struct inode *inode, struct file *file)
{
	DWORD			ret;
	sspc100_jpg_ctx		*jpg_reg_ctx;

	jpg_dbg("JPG_Close\n");

	jpg_reg_ctx = (sspc100_jpg_ctx *)file->private_data;

	if (!jpg_reg_ctx) {
		jpg_err("JPG Invalid Input Handle\r\n");
		return FALSE;
	}

	ret = lock_jpg_mutex();

	if (!ret) {
		jpg_err("JPG Mutex Lock Fail\r\n");
		return FALSE;
	}

	if ((--instanceNo) < 0)
		instanceNo = 0;

	unlock_jpg_mutex();
	kfree(jpg_reg_ctx);

/* clock disable */
	clk_disable(s3c_jpeg_clk);
	ret = s5pv210_pd_disable("jpeg_pd");
	if (ret < 0) {
		jpg_err("failed to disable jpeg power domain\n");
		return FALSE;
	}

	return 0;
}


static ssize_t s3c_jpeg_write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
	return 0;
}

static ssize_t s3c_jpeg_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	return 0;
}

static int s3c_jpeg_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	static sspc100_jpg_ctx		*jpg_reg_ctx;
	jpg_args			param;
	BOOL				result = TRUE;
	DWORD				ret;
	int 				out;


	jpg_reg_ctx = (sspc100_jpg_ctx *)file->private_data;

	if (!jpg_reg_ctx) {
		jpg_err("JPG Invalid Input Handle\r\n");
		return FALSE;
	}

	ret = lock_jpg_mutex();

	if (!ret) {
		jpg_err("JPG Mutex Lock Fail\r\n");
		return FALSE;
	}

	switch (cmd) {
	case IOCTL_JPG_DECODE:

		jpg_dbg("IOCTL_JPEG_DECODE\n");

		out = copy_from_user(&param, (jpg_args *)arg, sizeof(jpg_args));

		jpg_reg_ctx->jpg_data_addr = (UINT32)jpg_data_base_addr;
		jpg_reg_ctx->img_data_addr = (UINT32)jpg_data_base_addr
						+ JPG_STREAM_BUF_SIZE
						+ JPG_STREAM_THUMB_BUF_SIZE;

		result = decode_jpg(jpg_reg_ctx, param.dec_param);
		out = copy_to_user((void *)arg, (void *) & param, sizeof(jpg_args));
		break;

	case IOCTL_JPG_ENCODE:

		jpg_dbg("IOCTL_JPEG_ENCODE\n");

		out = copy_from_user(&param, (jpg_args *)arg, sizeof(jpg_args));

		jpg_dbg("encode size :: width : %d hegiht : %d\n",
			param.enc_param->width, param.enc_param->height);

		if (param.enc_param->enc_type == JPG_MAIN) {
			jpg_reg_ctx->jpg_data_addr = (UINT32)jpg_data_base_addr ;
			jpg_reg_ctx->img_data_addr = (UINT32)jpg_data_base_addr
							+ JPG_STREAM_BUF_SIZE
							+ JPG_STREAM_THUMB_BUF_SIZE;
			jpg_dbg("enc_img_data_addr=0x%08x, enc_jpg_data_addr=0x%08x\n"
				, jpg_reg_ctx->img_data_addr,jpg_reg_ctx->jpg_data_addr);

			result = encode_jpg(jpg_reg_ctx, param.enc_param);
		} else {
			jpg_reg_ctx->img_thumb_data_addr = (UINT32)jpg_data_base_addr
								+ JPG_STREAM_BUF_SIZE
								+ JPG_STREAM_THUMB_BUF_SIZE
								+ JPG_FRAME_BUF_SIZE;
			jpg_reg_ctx->jpg_thumb_data_addr = (UINT32)jpg_data_base_addr
								+ JPG_STREAM_BUF_SIZE;

			result = encode_jpg(jpg_reg_ctx, param.thumb_enc_param);
		}
		out = copy_to_user((void *)arg, (void *) & param,  sizeof(jpg_args));
		break;

	case IOCTL_JPG_GET_STRBUF:
		jpg_dbg("IOCTL_JPG_GET_STRBUF\n");
		unlock_jpg_mutex();
		return arg + JPG_MAIN_STRART;

	case IOCTL_JPG_GET_THUMB_STRBUF:
		jpg_dbg("IOCTL_JPG_GET_THUMB_STRBUF\n");
		unlock_jpg_mutex();
		return arg + JPG_THUMB_START;

	case IOCTL_JPG_GET_FRMBUF:
		jpg_dbg("IOCTL_JPG_GET_FRMBUF\n");
		unlock_jpg_mutex();
		return arg + IMG_MAIN_START;

	case IOCTL_JPG_GET_THUMB_FRMBUF:
		jpg_dbg("IOCTL_JPG_GET_THUMB_FRMBUF\n");
		unlock_jpg_mutex();
		return arg + IMG_THUMB_START;

	case IOCTL_JPG_GET_PHY_FRMBUF:
		jpg_dbg("IOCTL_JPG_GET_PHY_FRMBUF\n");
		unlock_jpg_mutex();
		return jpg_data_base_addr + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE;

	case IOCTL_JPG_GET_PHY_THUMB_FRMBUF:
		jpg_dbg("IOCTL_JPG_GET_PHY_THUMB_FRMBUF\n");
		unlock_jpg_mutex();
		return jpg_data_base_addr + JPG_STREAM_BUF_SIZE
			+ JPG_STREAM_THUMB_BUF_SIZE + JPG_FRAME_BUF_SIZE;

	default :
		jpg_dbg("JPG Invalid ioctl : 0x%X\n", cmd);
	}

	unlock_jpg_mutex();

	return result;
}

static unsigned int s3c_jpeg_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	jpg_dbg("enter poll \n");
	poll_wait(file, &wait_queue_jpeg, wait);
	mask = POLLOUT | POLLWRNORM;
	return mask;
}
int s3c_jpeg_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size	= vma->vm_end - vma->vm_start;
	unsigned long max_size;
	unsigned long page_frame_no;

	page_frame_no = __phys_to_pfn(jpg_data_base_addr);

	max_size = JPG_TOTAL_BUF_SIZE + PAGE_SIZE - (JPG_TOTAL_BUF_SIZE % PAGE_SIZE);

	if (size > max_size) {
		return -EINVAL;
	}

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_pfn_range(vma, vma->vm_start, page_frame_no, size,	\
			    vma->vm_page_prot)) {
		jpg_err("jpeg remap error");
		return -EAGAIN;
	}

	return 0;
}


static const struct file_operations jpeg_fops = {
	owner:		THIS_MODULE,
	open:		s3c_jpeg_open,
	release:	s3c_jpeg_release,
	ioctl:		s3c_jpeg_ioctl,
	read:		s3c_jpeg_read,
	write:		s3c_jpeg_write,
	mmap:		s3c_jpeg_mmap,
	poll:		s3c_jpeg_poll,
};


static struct miscdevice s3c_jpeg_miscdev = {
	minor:		254,
	name:		"s3c-jpg",
	fops:		&jpeg_fops
};


static int s3c_jpeg_probe(struct platform_device *pdev)
{
	struct resource 	*res;
	static int		size;
	static int		ret;
	HANDLE 			h_mutex;

	s3c_jpeg_clk = clk_get(&pdev->dev, "jpeg");

	if (IS_ERR(s3c_jpeg_clk)) {
		jpg_err("failed to find jpeg clock source\n");
		return -ENOENT;
	}

	clk_enable(s3c_jpeg_clk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res == NULL) {
		jpg_err("failed to get memory region resouce\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;
	s3c_jpeg_mem = request_mem_region(res->start, size, pdev->name);

	if (s3c_jpeg_mem == NULL) {
		jpg_err("failed to get memory region\n");
		return -ENOENT;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	if (res == NULL) {
		jpg_err("failed to get irq resource\n");
		return -ENOENT;
	}

	irq_no = res->start;
	ret = request_irq(res->start, (void *)s3c_jpeg_irq, 0, pdev->name, pdev);

	if (ret != 0) {
		jpg_err("failed to install irq (%d)\n", ret);
		return ret;
	}

	s3c_jpeg_base = ioremap(s3c_jpeg_mem->start, size);

	if (s3c_jpeg_base == 0) {
		jpg_err("failed to ioremap() region\n");
		return -EINVAL;
	}

	init_waitqueue_head(&wait_queue_jpeg);

	jpg_dbg("JPG_Init\n");

	// Mutex initialization
	h_mutex = create_jpg_mutex();

	if (h_mutex == NULL) {
		jpg_err("JPG Mutex Initialize error\r\n");
		return FALSE;
	}

	ret = lock_jpg_mutex();

	if (!ret) {
		jpg_err("JPG Mutex Lock Fail\n");
		return FALSE;
	}

	instanceNo = 0;

	unlock_jpg_mutex();

	ret = misc_register(&s3c_jpeg_miscdev);

	/* clock disable */
	clk_disable(s3c_jpeg_clk);

	return 0;
}

static int s3c_jpeg_remove(struct platform_device *dev)
{
	if (s3c_jpeg_mem != NULL) {
		release_resource(s3c_jpeg_mem);
		kfree(s3c_jpeg_mem);
		s3c_jpeg_mem = NULL;
	}

	free_irq(irq_no, dev);
	misc_deregister(&s3c_jpeg_miscdev);
	return 0;
}

#ifdef CONFIG_CPU_S5PV210
static int s3c_jpeg_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret;
	/* clock disable */
	clk_disable(s3c_jpeg_clk);
	
	ret = s5pv210_pd_disable("jpeg_pd");
	if (ret < 0) {
		jpg_err("failed to disable jpeg power domain\n");
		return FALSE;
	}
	return 0;
}

static int s3c_jpeg_resume(struct platform_device *pdev)
{
	int ret;
	
	ret = s5pv210_pd_enable("jpeg_pd");
	if (ret < 0) {
		jpg_err("failed to enable jpeg power domain\n");
		return FALSE;
	}
	
	/* clock enable */
	clk_enable(s3c_jpeg_clk);

	return 0;
}
#endif

static struct platform_driver s3c_jpeg_driver = {
	.probe		= s3c_jpeg_probe,
	.remove		= s3c_jpeg_remove,
	.shutdown	= NULL,
	.suspend	= s3c_jpeg_suspend,
	.resume		= s3c_jpeg_resume,
	.driver		= {
			.owner	= THIS_MODULE,
			.name	= "s3c-jpg",
	},
};

static char banner[] __initdata = KERN_INFO "S3C JPEG Driver, (c) 2007 Samsung Electronics\n";

static int __init s3c_jpeg_init(void)
{
	printk(banner);
	printk("JPEG driver for S5PV210 \n");
	return platform_driver_register(&s3c_jpeg_driver);
}

static void __exit s3c_jpeg_exit(void)
{
	DWORD	ret;

	jpg_dbg("JPG_Deinit\n");

	ret = lock_jpg_mutex();

	if (!ret) {
		jpg_err("JPG Mutex Lock Fail\r\n");
	}

	unlock_jpg_mutex();

	delete_jpg_mutex();

	platform_driver_unregister(&s3c_jpeg_driver);
	jpg_dbg("S3C JPEG driver module exit\n");
}

module_init(s3c_jpeg_init);
module_exit(s3c_jpeg_exit);

MODULE_AUTHOR("Peter, Oh");
MODULE_DESCRIPTION("S3C JPEG Encoder/Decoder Device Driver");
MODULE_LICENSE("GPL");
