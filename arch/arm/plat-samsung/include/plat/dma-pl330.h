/* linux/arch/arm/plat-s5p/include/plat/dma-pl330.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Header file for dma pl330
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ARM_MACH_DMA_PL330_H
#define __ARM_MACH_DMA_PL330_H __FILE__

#define DMACH_LOW_LEVEL			(1<<28)	/* use this to specifiy hardware ch no */

/* flags */
#define S3C2410_DMAF_SLOW         	(1<<0)   /* slow, so don't worry about */
#define S3C2410_DMAF_AUTOSTART    	(1<<1)   /* auto-start if buffer queued */
#define S3C2410_DMAF_CIRCULAR		(1<<2)   /* If DMA driver supports buffer looping */

/*=================================================*/
/*   DMA Register Definitions for PL330 DMAC       */

#define S3C_DMAC_DS  			(0x00)
#define S3C_DMAC_DPC   			(0x04)
#define S3C_DMAC_INTEN  		(0x20)		/* R/W */
#define S3C_DMAC_ES  			(0x24)
#define S3C_DMAC_INTSTATUS	   	(0x28)
#define S3C_DMAC_INTCLR			(0x2C)		/* W/O */
#define S3C_DMAC_FSM		  	(0x30)
#define S3C_DMAC_FSC		   	(0x34)
#define S3C_DMAC_FTM		   	(0x38)

#define S3C_DMAC_FTC0   		(0x40)
#define S3C_DMAC_CS0  			(0x100)
#define S3C_DMAC_CPC0   		(0x104)
#define S3C_DMAC_SA_0   		(0x400)
#define S3C_DMAC_DA_0  			(0x404)
#define S3C_DMAC_CC_0   		(0x408)
#define S3C_DMAC_LC0_0  		(0x40C)
#define S3C_DMAC_LC1_0   		(0x410)

#define S3C_DMAC_FTC(ch)   		(S3C_DMAC_FTC0+ch*0x4)
#define S3C_DMAC_CS(ch)   		(S3C_DMAC_CS0+ch*0x8)
#define S3C_DMAC_CPC(ch)   		(S3C_DMAC_CPC0+ch*0x8)
#define S3C_DMAC_SA(ch)   		(S3C_DMAC_SA_0+ch*0x20)
#define S3C_DMAC_DA(ch)   		(S3C_DMAC_DA_0+ch*0x20)
#define S3C_DMAC_CC(ch)   		(S3C_DMAC_CC_0+ch*0x20)
#define S3C_DMAC_LC0(ch)   		(S3C_DMAC_LC0_0+ch*0x20)
#define S3C_DMAC_LC10(ch)   		(S3C_DMAC_LC1_0+ch*0x20)

#define S3C_DMAC_DBGSTATUS 		(0xD00)
#define S3C_DMAC_DBGCMD   		(0xD04)		/* W/O */
#define S3C_DMAC_DBGINST0  		(0xD08)		/* W/O */
#define S3C_DMAC_DBGINST1   		(0xD0C)		/* W/O */
#define S3C_DMAC_CR0   			(0xE00)
#define S3C_DMAC_CR1   			(0xE04)
#define S3C_DMAC_CR2   			(0xE08)
#define S3C_DMAC_CR3   			(0xE0C)
#define S3C_DMAC_CR4   			(0xE10)
#define S3C_DMAC_CRDn   		(0xE14)

#define S3C_DMAC_PERI_ID  		(0xFE0)
#define S3C_DMAC_PCELL_ID  		(0xFF0)

/* S3C_DMAC_CS[3:0] - Channel status */
#define S3C_DMAC_CS_STOPPED		0x0
#define S3C_DMAC_CS_EXECUTING		0x1
#define S3C_DMAC_CS_CACHE_MISS		0x2
#define S3C_DMAC_CS_UPDATING_PC		0x3
#define S3C_DMAC_CS_WAITING_FOR_EVENT	0x4
#define S3C_DMAC_CS_AT_BARRIER		0x5
#define S3C_DMAC_CS_QUEUE_BUSY		0x6
#define S3C_DMAC_CS_WAITING_FOR_PERI	0x7
#define S3C_DMAC_CS_KILLING		0x8
#define S3C_DMAC_CS_COMPLETING		0x9
#define S3C_DMAC_CS_FAULT_COMPLETING	0xE
#define S3C_DMAC_CS_FAULTING		0xF



/* S3C_DMAC_INTEN : Interrupt Enable Register */
#define S3C_DMAC_INTEN_EVENT(x)		((x)<<0)
#define S3C_DMAC_INTEN_IRQ(x)		((x)<<1)

/* S3C_DMAC_INTCLR : Interrupt Clear Register */
#define S3C_DMAC_INTCLR_IRQ(x)		((x)<<1)

/* S3C DMA Channel control */
/* Source control */
#define S3C_DMACONTROL_SRC_INC		(1<<0)
#define S3C_DMACONTROL_SRC_FIXED	(0<<0)
#define S3C_DMACONTROL_SRC_WIDTH_BYTE	(0<<1)
#define S3C_DMACONTROL_SRC_WIDTH_HWORD	(1<<1)
#define S3C_DMACONTROL_SRC_WIDTH_WORD	(2<<1)
#define S3C_DMACONTROL_SRC_WIDTH_DWORD	(3<<1)
#define S3C_DMACONTROL_SBSIZE(x)	(((x-1)&0xF)<<4)
#define S3C_DMACONTROL_SP_SECURE	(0<<8)
#define S3C_DMACONTROL_SP_NON_SECURE	(2<<8)
#define S3C_DMACONTROL_SCACHE		(0<<11)

/* Destination control */
#define S3C_DMACONTROL_DEST_INC		(1<<14)
#define S3C_DMACONTROL_DEST_FIXED	(0<<14)
#define S3C_DMACONTROL_DEST_WIDTH_BYTE	(0<<15)
#define S3C_DMACONTROL_DEST_WIDTH_HWORD	(1<<15)
#define S3C_DMACONTROL_DEST_WIDTH_WORD	(2<<15)
#define S3C_DMACONTROL_DEST_WIDTH_DWORD	(3<<15)
#define S3C_DMACONTROL_DBSIZE(x)	(((x-1)&0xF)<<18)
#define S3C_DMACONTROL_DP_SECURE	(0<<22)
#define S3C_DMACONTROL_DP_NON_SECURE	(2<<22)
#define S3C_DMACONTROL_DCACHE		(0<<25)

#define S3C_DMACONTROL_ES_SIZE_8	(0<<28)
#define S3C_DMACONTROL_ES_SIZE_16	(1<<28)
#define S3C_DMACONTROL_ES_SIZE_32	(2<<28)
#define S3C_DMACONTROL_ES_SIZE_64	(3<<28)

#endif /* __ARM_MACH_DMA_PL330_H */

