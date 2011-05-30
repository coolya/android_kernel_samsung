/*
 * drivers/media/video/samsung/mfc50/mfc.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code (Key Young, Park)
 *   2009.09.21 - Implement clock & power gating.
 *                including suspend & resume fuction. (Key Young, Park)
 *   2009.10.08 - Apply 9/30 firmware (Key Young, Park)
 *   2009.10.09 - Add error handling rountine (Key Young, Park)
 *   2009.10.13 - move mfc interrupt routine into mfc_irq.c (Key Young, Park)
 *   2009.10.27 - Update firmware (2009.10.15) (Key Young, Park)
 *   2009.11.04 - get physical address via mfc_allocate_buffer (Key Young, Park)
 *   2009.11.04 - remove mfc_common.[ch]
 *                seperate buffer alloc & set (Key Young, Park)
 *   2009.11.24 - add state check when decoding & encoding (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>

#include <linux/sched.h>
#include <linux/firmware.h>

#include <linux/io.h>
#include <linux/uaccess.h>

#include <plat/media.h>
#include <mach/media.h>
#include <plat/mfc.h>
#ifdef CONFIG_DVFS_LIMIT
#include <mach/cpu-freq-v210.h>
#endif

#include "mfc_interface.h"
#include "mfc_logmsg.h"
#include "mfc_opr.h"
#include "mfc_memory.h"
#include "mfc_buffer_manager.h"
#include "mfc_intr.h"

#define MFC_FW_NAME	"samsung_mfc_fw.bin"

static struct resource *mfc_mem;
static struct mutex mfc_mutex;
static struct clk *mfc_sclk;
static struct regulator *mfc_pd_regulator;
const struct firmware	*mfc_fw_info;

static int mfc_open(struct inode *inode, struct file *file)
{
	struct mfc_inst_ctx *mfc_ctx;
	int ret;

	mutex_lock(&mfc_mutex);

	if (!mfc_is_running()) {
		/* Turn on mfc power domain regulator */
		ret = regulator_enable(mfc_pd_regulator);
		if (ret < 0) {
			mfc_err("MFC_RET_POWER_ENABLE_FAIL\n");
			ret = -EINVAL;
			goto err_open;
		}

#ifdef CONFIG_DVFS_LIMIT
		s5pv210_lock_dvfs_high_level(DVFS_LOCK_TOKEN_1, L2);
#endif
		clk_enable(mfc_sclk);

		mfc_load_firmware(mfc_fw_info->data, mfc_fw_info->size);

		if (mfc_init_hw() != true) {
			clk_disable(mfc_sclk);
			ret =  -ENODEV;
			goto err_regulator;
		}
		clk_disable(mfc_sclk);
	}

	mfc_ctx = (struct mfc_inst_ctx *)kmalloc(sizeof(struct mfc_inst_ctx), GFP_KERNEL);
	if (mfc_ctx == NULL) {
		mfc_err("MFCINST_MEMORY_ALLOC_FAIL\n");
		ret = -ENOMEM;
		goto err_regulator;
	}

	memset(mfc_ctx, 0, sizeof(struct mfc_inst_ctx));

	/* get the inst no allocating some part of memory among reserved memory */
	mfc_ctx->mem_inst_no = mfc_get_mem_inst_no();
	mfc_ctx->InstNo = -1;
	if (mfc_ctx->mem_inst_no < 0) {
		mfc_err("MFCINST_INST_NUM_EXCEEDED\n");
		ret = -EPERM;
		goto err_mem_inst;
	}

	if (mfc_set_state(mfc_ctx, MFCINST_STATE_OPENED) < 0) {
		mfc_err("MFCINST_ERR_STATE_INVALID\n");
		ret = -ENODEV;
		goto err_set_state;
	}

	/* Decoder only */
	mfc_ctx->extraDPB = MFC_MAX_EXTRA_DPB;
	mfc_ctx->FrameType = MFC_RET_FRAME_NOT_SET;

	file->private_data = mfc_ctx;

	mutex_unlock(&mfc_mutex);

	return 0;

err_set_state:
	mfc_return_mem_inst_no(mfc_ctx->mem_inst_no);
err_mem_inst:
	kfree(mfc_ctx);
err_regulator:
	if (!mfc_is_running()) {
#ifdef CONFIG_DVFS_LIMIT
		s5pv210_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_1);
#endif
		/* Turn off mfc power domain regulator */
		ret = regulator_disable(mfc_pd_regulator);
		if (ret < 0)
			mfc_err("MFC_RET_POWER_DISABLE_FAIL\n");
	}
err_open:
	mutex_unlock(&mfc_mutex);

	return ret;
}

static int mfc_release(struct inode *inode, struct file *file)
{
	struct mfc_inst_ctx *mfc_ctx;
	int ret;

	mutex_lock(&mfc_mutex);

	mfc_ctx = (struct mfc_inst_ctx *)file->private_data;
	if (mfc_ctx == NULL) {
		mfc_err("MFCINST_ERR_INVALID_PARAM\n");
		ret = -EIO;
		goto out_release;
	}

	mfc_release_all_buffer(mfc_ctx->mem_inst_no);
	mfc_merge_fragment(mfc_ctx->mem_inst_no);

	mfc_return_mem_inst_no(mfc_ctx->mem_inst_no);

	/* In case of no instance, we should not release codec instance */
	if (mfc_ctx->InstNo >= 0) {
		clk_enable(mfc_sclk);
		mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);
		clk_disable(mfc_sclk);
	}

	kfree(mfc_ctx);

	ret = 0;

out_release:
	if (!mfc_is_running()) {
#ifdef CONFIG_DVFS_LIMIT
		s5pv210_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_1);
#endif
		/* Turn off mfc power domain regulator */
		ret = regulator_disable(mfc_pd_regulator);
		if (ret < 0) {
			mfc_err("MFC_RET_POWER_DISABLE_FAIL\n");
		}
	}

	mutex_unlock(&mfc_mutex);
	return ret;
}

static int mfc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret, ex_ret;
	struct mfc_inst_ctx *mfc_ctx = NULL;
	struct mfc_common_args in_param;

	mutex_lock(&mfc_mutex);
	clk_enable(mfc_sclk);

	ret = copy_from_user(&in_param, (struct mfc_common_args *)arg, sizeof(struct mfc_common_args));
	if (ret < 0) {
		mfc_err("Inparm copy error\n");
		ret = -EIO;
		in_param.ret_code = MFCINST_ERR_INVALID_PARAM;
		goto out_ioctl;
	}

	mfc_ctx = (struct mfc_inst_ctx *)file->private_data;
	mutex_unlock(&mfc_mutex);

	switch (cmd) {
	case IOCTL_MFC_ENC_INIT:
		mutex_lock(&mfc_mutex);

		if (mfc_set_state(mfc_ctx, MFCINST_STATE_ENC_INITIALIZE) < 0) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		/* MFC encode init */
		in_param.ret_code = mfc_init_encode(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_ENC_EXE:
		mutex_lock(&mfc_mutex);
		if (mfc_ctx->MfcState < MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		if (mfc_set_state(mfc_ctx, MFCINST_STATE_ENC_EXE) < 0) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		in_param.ret_code = mfc_exe_encode(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_DEC_INIT:
		mutex_lock(&mfc_mutex);
		if (mfc_set_state(mfc_ctx, MFCINST_STATE_DEC_INITIALIZE) < 0) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		/* MFC decode init */
		in_param.ret_code = mfc_init_decode(mfc_ctx, &(in_param.args));
		if (in_param.ret_code < 0) {
			ret = in_param.ret_code;
			mutex_unlock(&mfc_mutex);
			break;
		}

		if (in_param.args.dec_init.out_dpb_cnt <= 0) {
			mfc_err("MFC out_dpb_cnt error\n");
			mutex_unlock(&mfc_mutex);
			break;
		}

		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_DEC_EXE:
		mutex_lock(&mfc_mutex);
		if (mfc_ctx->MfcState < MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		if (mfc_set_state(mfc_ctx, MFCINST_STATE_DEC_EXE) < 0) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		in_param.ret_code = mfc_exe_decode(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_GET_CONFIG:
		mutex_lock(&mfc_mutex);
		if (mfc_ctx->MfcState < MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		in_param.ret_code = mfc_get_config(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_SET_CONFIG:
		mutex_lock(&mfc_mutex);
		in_param.ret_code = mfc_set_config(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_GET_IN_BUF:
		mutex_lock(&mfc_mutex);
		if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		if (in_param.args.mem_alloc.buff_size <= 0) {
			mfc_err("MFCINST_ERR_INVALID_PARAM\n");
			in_param.ret_code = MFCINST_ERR_INVALID_PARAM;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		if ((is_dec_codec(in_param.args.mem_alloc.codec_type)) &&
				(in_param.args.mem_alloc.buff_size < (CPB_BUF_SIZE + DESC_BUF_SIZE))) {
			in_param.args.mem_alloc.buff_size = CPB_BUF_SIZE + DESC_BUF_SIZE;
		}

		/* Buffer manager should have 64KB alignment for MFC base addresses */
		in_param.args.mem_alloc.buff_size = ALIGN_TO_8KB(in_param.args.mem_alloc.buff_size);

		/* allocate stream buf for decoder & current YC buf for encoder */
		if (is_dec_codec(in_param.args.mem_alloc.codec_type))
			in_param.ret_code = mfc_allocate_buffer(mfc_ctx, &in_param.args, 0);
		else
			in_param.ret_code = mfc_allocate_buffer(mfc_ctx, &in_param.args, 1);

		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_FREE_BUF:
		mutex_lock(&mfc_mutex);
		if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		in_param.ret_code = mfc_release_buffer((unsigned char *)in_param.args.mem_free.u_addr);
		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_GET_PHYS_ADDR:
		mutex_lock(&mfc_mutex);
		mfc_debug("IOCTL_MFC_GET_PHYS_ADDR\n");

		if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&mfc_mutex);
			break;
		}

		in_param.ret_code = mfc_get_phys_addr(mfc_ctx, &(in_param.args));
		ret = in_param.ret_code;
		mutex_unlock(&mfc_mutex);
		break;

	case IOCTL_MFC_GET_MMAP_SIZE:

		if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
			mfc_err("MFC_RET_STATE_INVALID\n");
			in_param.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;

			break;
		}

		in_param.ret_code = MFCINST_RET_OK;
		ret = mfc_ctx->port0_mmap_size;

		break;

       case IOCTL_MFC_BUF_CACHE:
		mutex_lock(&mfc_mutex);
		
		mfc_ctx->buf_type = in_param.args.buf_type;

		mutex_unlock(&mfc_mutex);
		break;
		
	default:
		mfc_err("Requested ioctl command is not defined. (ioctl cmd=0x%08x)\n", cmd);
		in_param.ret_code  = MFCINST_ERR_INVALID_PARAM;
		ret = -EINVAL;
	}

out_ioctl:
	clk_disable(mfc_sclk);

	ex_ret = copy_to_user((struct mfc_common_args *)arg, &in_param, sizeof(struct mfc_common_args));
	if (ex_ret < 0) {
		mfc_err("Outparm copy to user error\n");
		ret = -EIO;
	}

	mfc_debug_L0("---------------IOCTL return = %d ---------------\n", ret);

	return ret;
}

static int mfc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long vir_size = vma->vm_end - vma->vm_start;
	unsigned long phy_size, firmware_size;
	unsigned long page_frame_no = 0;
	struct mfc_inst_ctx *mfc_ctx;

	mfc_debug("vma->vm_start = 0x%08x, vma->vm_end = 0x%08x\n",
			(unsigned int)vma->vm_start,
			(unsigned int)vma->vm_end);
	mfc_debug("vma->vm_end - vma->vm_start = %ld\n", vir_size);

	mfc_ctx = (struct mfc_inst_ctx *)filp->private_data;

	firmware_size = mfc_get_port0_buff_paddr() - mfc_get_fw_buff_paddr();
	phy_size = (unsigned long)(mfc_port0_memsize - firmware_size + mfc_port1_memsize);

	/* if memory size required from appl. mmap() is bigger than max data memory
	 * size allocated in the driver */
	if (vir_size > phy_size) {
		mfc_err("virtual requested mem(%ld) is bigger than physical mem(%ld)\n",
				vir_size, phy_size);
		return -EINVAL;
	}

	mfc_ctx->port0_mmap_size = (vir_size / 2);

	if (mfc_ctx->buf_type == MFC_BUFFER_CACHE) {
		vma->vm_flags |= VM_RESERVED | VM_IO;
	 	/*
 	  	* port0 mapping for stream buf & frame buf (chroma + MV)
 	  	*/
 	  	page_frame_no = __phys_to_pfn(mfc_get_port0_buff_paddr());
	 	if (remap_pfn_range(vma, vma->vm_start, page_frame_no,
	 		mfc_ctx->port0_mmap_size, vma->vm_page_prot)) {
	 		 mfc_err("mfc remap port0 error\n");
	 		 return -EAGAIN;
	 	}
		vma->vm_flags |= VM_RESERVED | VM_IO;
		/*
	 	* port1 mapping for frame buf (luma)
	 	*/
	 	page_frame_no = __phys_to_pfn(mfc_get_port1_buff_paddr());
		if (remap_pfn_range(vma, vma->vm_start + mfc_ctx->port0_mmap_size,
			page_frame_no, vir_size - mfc_ctx->port0_mmap_size, vma->vm_page_prot)) {
			mfc_err("mfc remap port1 error\n");
			return -EAGAIN;
		 }
	} else {
	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/*
	 * port0 mapping for stream buf & frame buf (chroma + MV)
	 */
	page_frame_no = __phys_to_pfn(mfc_get_port0_buff_paddr());
	if (remap_pfn_range(vma, vma->vm_start, page_frame_no,
		mfc_ctx->port0_mmap_size, vma->vm_page_prot)) {
		mfc_err("mfc remap port0 error\n");
		return -EAGAIN;
	}

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/*
	 * port1 mapping for frame buf (luma)
	 */
	page_frame_no = __phys_to_pfn(mfc_get_port1_buff_paddr());
	if (remap_pfn_range(vma, vma->vm_start + mfc_ctx->port0_mmap_size,
		page_frame_no, vir_size - mfc_ctx->port0_mmap_size, vma->vm_page_prot)) {
		mfc_err("mfc remap port1 error\n");
		return -EAGAIN;
	}
	}	

	mfc_debug("virtual requested mem = %ld, physical reserved data mem = %ld\n", vir_size, phy_size);

	return 0;
}

static const struct file_operations mfc_fops = {
	.owner      = THIS_MODULE,
	.open       = mfc_open,
	.release    = mfc_release,
	.ioctl      = mfc_ioctl,
	.mmap       = mfc_mmap
};


static struct miscdevice mfc_miscdev = {
	.minor      = 252,
	.name       = "s3c-mfc",
	.fops       = &mfc_fops,
};

static void mfc_firmware_request_complete_handler(const struct firmware *fw,
						  void *context)
{
	if (fw != NULL) {
		mfc_load_firmware(fw->data, fw->size);
		mfc_fw_info = fw;
	} else {
		mfc_err("failed to load MFC F/W, MFC will not working\n");
	}
}

static int mfc_probe(struct platform_device *pdev)
{
	struct s3c_platform_mfc *pdata;
	struct resource *res;
	size_t size;
	int ret;

	if (!pdev || !pdev->dev.platform_data) {
		dev_err(&pdev->dev, "Unable to probe mfc!\n");
		return -1;
	}

	pdata = pdev->dev.platform_data;

	/* mfc clock enable should be here */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		ret = -ENOENT;
		goto probe_out;
	}

	/* 60K is required for mfc register (0x0 ~ 0xe008) */
	size = (res->end - res->start) + 1;
	mfc_mem = request_mem_region(res->start, size, pdev->name);
	if (mfc_mem == NULL) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_mem_req;
	}

	mfc_sfr_base_vaddr = ioremap(mfc_mem->start, mfc_mem->end - mfc_mem->start + 1);
	if (mfc_sfr_base_vaddr == NULL) {
		dev_err(&pdev->dev, "failed to ioremap address region\n");
		ret = -ENOENT;
		goto err_mem_map;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get irq resource\n");
		ret = -ENOENT;
		goto err_irq_res;
	}

#if !defined(MFC_POLLING)
	ret = request_irq(res->start, mfc_irq, IRQF_DISABLED, pdev->name, pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "failed to install irq (%d)\n", ret);
		goto err_irq_req;
	}
#endif

	mutex_init(&mfc_mutex);

	/*
	 * buffer memory secure
	 */
	mfc_port0_base_paddr =(unsigned int)pdata->buf_phy_base[0];
	mfc_port0_memsize =  (unsigned int)pdata->buf_phy_size[0];

	mfc_debug(" mfc_port0_base_paddr= 0x%x \n", mfc_port0_base_paddr);
	mfc_debug(" mfc_port0_memsize = 0x%x \n", mfc_port0_memsize);

	mfc_port0_base_paddr = ALIGN_TO_128KB(mfc_port0_base_paddr);
	mfc_port0_base_vaddr = phys_to_virt(mfc_port0_base_paddr);

	if (mfc_port0_base_vaddr == NULL) {
		mfc_err("fail to mapping port0 buffer\n");
		ret = -EPERM;
		goto err_vaddr_map;
	}

	mfc_port1_base_paddr = (unsigned int)pdata->buf_phy_base[1];
	mfc_port1_memsize =  (unsigned int)pdata->buf_phy_size[1];

	mfc_debug(" mfc_port1_base_paddr= 0x%x \n", mfc_port1_base_paddr);
	mfc_debug(" mfc_port1_memsize = 0x%x \n", mfc_port1_memsize);

	mfc_port1_base_paddr = ALIGN_TO_128KB(mfc_port1_base_paddr);
	mfc_port1_base_vaddr = phys_to_virt(mfc_port1_base_paddr);

	if (mfc_port1_base_vaddr == NULL) {
		mfc_err("fail to mapping port1 buffer\n");
		ret = -EPERM;
		goto err_vaddr_map;
	}

	mfc_debug("mfc_port0_base_paddr = 0x%08x, mfc_port1_base_paddr = 0x%08x <<\n",
		(unsigned int)mfc_port0_base_paddr, (unsigned int)mfc_port1_base_paddr);
	mfc_debug("mfc_port0_base_vaddr = 0x%08x, mfc_port1_base_vaddr = 0x%08x <<\n",
		(unsigned int)mfc_port0_base_vaddr, (unsigned int)mfc_port1_base_vaddr);

	/* Get mfc power domain regulator */
	mfc_pd_regulator = regulator_get(&pdev->dev, "pd");
	if (IS_ERR(mfc_pd_regulator)) {
		mfc_err("failed to find mfc power domain\n");
		ret = PTR_ERR(mfc_pd_regulator);
		goto err_regulator_get;
	}

	mfc_sclk = clk_get(&pdev->dev, "sclk_mfc");
	if (IS_ERR(mfc_sclk)) {
		mfc_err("failed to find mfc clock source\n");
		ret = PTR_ERR(mfc_sclk);
		goto err_clk_get;
	}

	mfc_init_mem_inst_no();
	mfc_init_buffer();

	ret = misc_register(&mfc_miscdev);
	if (ret) {
		mfc_err("MFC can't misc register on minor\n");
		goto err_misc_reg;
	}

	/*
	 * MFC FW downloading
	 */
	ret = request_firmware_nowait(THIS_MODULE,
				      FW_ACTION_HOTPLUG,
				      MFC_FW_NAME,
				      &pdev->dev,
				      GFP_KERNEL,
				      pdev,
				      mfc_firmware_request_complete_handler);
	if (ret) {
		mfc_err("MFCINST_ERR_FW_INIT_FAIL\n");
		ret = -EPERM;
		goto err_req_fw;
	}

	return 0;

err_req_fw:
	misc_deregister(&mfc_miscdev);
err_misc_reg:
	clk_put(mfc_sclk);
err_clk_get:
	regulator_put(mfc_pd_regulator);
err_regulator_get:
err_vaddr_map:
	free_irq(res->start, pdev);
	mutex_destroy(&mfc_mutex);
err_irq_req:
err_irq_res:
	iounmap(mfc_sfr_base_vaddr);
err_mem_map:
	release_mem_region((unsigned int)mfc_mem, size);
err_mem_req:
probe_out:
	dev_err(&pdev->dev, "not found (%d).\n", ret);
	return ret;
}

static int mfc_remove(struct platform_device *pdev)
{
	iounmap(mfc_sfr_base_vaddr);
	iounmap(mfc_port0_base_vaddr);

	/* remove memory region */
	if (mfc_mem != NULL) {
		release_resource(mfc_mem);
		kfree(mfc_mem);
		mfc_mem = NULL;
	}

	free_irq(IRQ_MFC, pdev);

	mutex_destroy(&mfc_mutex);

	clk_put(mfc_sclk);

	misc_deregister(&mfc_miscdev);

	if (mfc_fw_info)
		release_firmware(mfc_fw_info);

	return 0;
}

static int mfc_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;

	mutex_lock(&mfc_mutex);

	if (!mfc_is_running()) {
		mutex_unlock(&mfc_mutex);
		return 0;
	}
	clk_enable(mfc_sclk);

	ret = mfc_set_sleep();
	if (ret != MFCINST_RET_OK) {
		clk_disable(mfc_sclk);
		mutex_unlock(&mfc_mutex);
		return ret;
	}

	clk_disable(mfc_sclk);

	mutex_unlock(&mfc_mutex);

	return 0;
}

static int mfc_resume(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int mc_status;

	mutex_lock(&mfc_mutex);

	if (!mfc_is_running()) {
		mutex_unlock(&mfc_mutex);
		return 0;
	}

	clk_enable(mfc_sclk);

	/*
	 * 1. MFC reset
	 */
	do {
		mc_status = READL(MFC_MC_STATUS);
	} while (mc_status != 0);

	if (mfc_cmd_reset() == false) {
		clk_disable(mfc_sclk);
		mutex_unlock(&mfc_mutex);
		mfc_err("MFCINST_ERR_INIT_FAIL\n");
		return MFCINST_ERR_INIT_FAIL;
	}

	WRITEL(mfc_port0_base_paddr, MFC_MC_DRAMBASE_ADDR_A);
	WRITEL(mfc_port1_base_paddr, MFC_MC_DRAMBASE_ADDR_B);
	WRITEL(1, MFC_NUM_MASTER);

	ret = mfc_set_wakeup();
	if (ret != MFCINST_RET_OK) {
		clk_disable(mfc_sclk);
		mutex_unlock(&mfc_mutex);
		return ret;
	}

	clk_disable(mfc_sclk);

	mutex_unlock(&mfc_mutex);

	return 0;
}

static struct platform_driver mfc_driver = {
	.probe      = mfc_probe,
	.remove     = mfc_remove,
	.shutdown   = NULL,
	.suspend    = mfc_suspend,
	.resume     = mfc_resume,

	.driver     = {
		.owner  = THIS_MODULE,
		.name   = "s3c-mfc",
	},
};

static char banner[] __initdata = KERN_INFO "S5PC110 MFC Driver, (c) 2009 Samsung Electronics\n";

static int __init mfc_init(void)
{
	mfc_info("%s\n", banner);

	if (platform_driver_register(&mfc_driver) != 0) {
		mfc_err(KERN_ERR "platform device registration failed..\n");
		return -1;
	}

	return 0;
}

static void __exit mfc_exit(void)
{
	platform_driver_unregister(&mfc_driver);
	mfc_info("S5PC110 MFC Driver exit.\n");
}

module_init(mfc_init);
module_exit(mfc_exit);

MODULE_AUTHOR("Jaeryul, Oh");
MODULE_DESCRIPTION("S3C MFC (Multi Function Codec - FIMV5.0) Device Driver");
MODULE_LICENSE("GPL");
