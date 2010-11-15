/*
 * drivers/media/video/samsung/mfc50/mfc_memory.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code (Key Young, Park)
 *   2009.10.08 - Apply 9/30 firmware(Key Young, Park)
 *   2009.11.04 - remove mfc_common.[ch]
 *                seperate buffer alloc & set (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _MFC_MEMORY_H_
#define _MFC_MEMORY_H_

#include "mfc_opr.h"

#ifdef CONFIG_VIDEO_MFC_MAX_INSTANCE
#define MFC_MAX_INSTANCE_NUM (CONFIG_VIDEO_MFC_MAX_INSTANCE)
#endif

#define SET_MEM_1080P	1
#define	SET_MEM_720P	0

#if SET_MEM_1080P
/*
 * Memory Configuration for 1080P
 * MFC_FW_TOTAL_BUF_SIZE should be aligned to 4KB (page size)
 */
#define MFC_FW_TOTAL_BUF_SIZE (ALIGN_TO_4KB(MFC_FW_MAX_SIZE + MFC_MAX_INSTANCE_NUM * MFC_FW_BUF_SIZE))
#define MFC_FW_MAX_SIZE       (2 * 1024 * 1024) /* 2MB : 2x1024x1024                  */
#define MFC_FW_BUF_SIZE       (512 * 1024)     /* 512KB : 512x1024 size per instance */

#define RISC_BUF_SIZE         (0x80000)        /* 512KB : 512x1024 size per instance */
#define CPB_BUF_SIZE          (0x300000)       /* 3MB   : 3x1024x1024 for decoder    */
#define DESC_BUF_SIZE         (0x20000)        /* 128KB : 128x1024                   */
#define SHARED_BUF_SIZE       (0x10000)        /* 64KB  :  64x1024                   */
#define PRED_BUF_SIZE         (0x10000)        /* 64KB  :  64x1024                   */
#define DEC_CODEC_BUF_SIZE    (0x80000)        /* 512KB : 512x1024 size per instance */
#define ENC_CODEC_BUF_SIZE    (0x50000)        /* 320KB : 512x1024 size per instance */

#define STREAM_BUF_SIZE       (0x300000)       /* 3MB   : 3x1024x1024 for encoder    */
#define MV_BUF_SIZE           (0x10000)        /* 64KB  : 64x1024 for encoder        */

#define H264DEC_CONTEXT_SIZE  (640 * 1024)     /* 600KB -> 640KB for alignment       */
#define VC1DEC_CONTEXT_SIZE   (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define MPEG2DEC_CONTEXT_SIZE (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define H263DEC_CONTEXT_SIZE  (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define MPEG4DEC_CONTEXT_SIZE (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define H264ENC_CONTEXT_SIZE  (640 * 1024)      /* 64KB  ->  640KB for alignment       */
#define MPEG4ENC_CONTEXT_SIZE (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define H263ENC_CONTEXT_SIZE  (64 * 1024)      /* 10KB  ->  64KB for alignment       */

#else
/* Memory Configuration for 720P */
#define MFC_FW_TOTAL_BUF_SIZE (ALIGN_TO_4KB(MFC_FW_MAX_SIZE + MFC_MAX_INSTANCE_NUM * MFC_FW_BUF_SIZE))
#define MFC_FW_MAX_SIZE       (2 * 1024 * 1024) /* 2MB : 2x1024x1024                  */
#define MFC_FW_BUF_SIZE       (512 * 1024)     /* 512KB : 512x1024 size per instance */

#define RISC_BUF_SIZE         (0x80000)        /* 512KB : 512x1024 size per instance */
#define CPB_BUF_SIZE          (0x300000)       /* 3MB   : 3x1024x1024 for decoder    */
#define DESC_BUF_SIZE         (0x20000)        /* 128KB : 128x1024                   */
#define SHARED_BUF_SIZE       (0x10000)        /* 64KB  :  64x1024                   */
#define PRED_BUF_SIZE         (0x10000)        /* 64KB  :  64x1024                   */
#define DEC_CODEC_BUF_SIZE    (0x80000)        /* 512KB : 512x1024 size per instance */
#define ENC_CODEC_BUF_SIZE    (0x50000)        /* 320KB : 512x1024 size per instance */

#define STREAM_BUF_SIZE       (0x200000)       /* 2MB   : 2x1024x1024 for encoder    */
#define MV_BUF_SIZE           (0x10000)        /* 64KB  : 64x1024 for encoder        */

#define H264DEC_CONTEXT_SIZE  (640 * 1024)     /* 600KB -> 640KB for alignment       */
#define VC1DEC_CONTEXT_SIZE   (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define MPEG2DEC_CONTEXT_SIZE (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define H263DEC_CONTEXT_SIZE  (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define MPEG4DEC_CONTEXT_SIZE (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define H264ENC_CONTEXT_SIZE  (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define MPEG4ENC_CONTEXT_SIZE (64 * 1024)      /* 10KB  ->  64KB for alignment       */
#define H263ENC_CONTEXT_SIZE  (64 * 1024)      /* 10KB  ->  64KB for alignment       */

#endif

unsigned int mfc_get_fw_buf_phys_addr(void);
unsigned int mfc_get_risc_buf_phys_addr(int instNo);

extern unsigned char *mfc_port0_base_vaddr; /* port1 */
extern unsigned char *mfc_port1_base_vaddr; /* port0 */
extern unsigned int mfc_port0_base_paddr, mfc_port1_base_paddr;
extern unsigned int  mfc_port0_memsize, mfc_port1_memsize;

unsigned int mfc_get_fw_buff_paddr(void);
unsigned char *mfc_get_fw_buff_vaddr(void);
unsigned int mfc_get_port0_buff_paddr(void);
unsigned char *mfc_get_port0_buff_vaddr(void);
unsigned int mfc_get_port1_buff_paddr(void);
unsigned char *mfc_get_port1_buff_vaddr(void);

extern void __iomem *mfc_sfr_base_vaddr;

#define READL(offset)         readl(mfc_sfr_base_vaddr + (offset))
#define WRITEL(data, offset)  writel((data), mfc_sfr_base_vaddr + (offset))

#endif /* _MFC_MEMORY_H_ */
