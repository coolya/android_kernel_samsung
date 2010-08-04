/*
 * Copyright (C) 2010 Samsung Electronics Co. Ltd.
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifdef CONFIG_OLD_DMA_PL330

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H __FILE__

#include <linux/sysdev.h>

#include <plat/dma.h>
#include <plat/dma-pl330.h>

/* We use `virtual` dma channels to hide the fact we have only a limited
 * number of DMA channels, and not of all of them (dependant on the device)
 * can be attached to any DMA source. We therefore let the DMA core handle
 * the allocation of hardware channels to clients.
*/

enum dma_ch {
	DMACH_XD0,
	DMACH_XD1,
	DMACH_SDI,
	DMACH_SPI0,
	DMACH_SPI1,
	DMACH_UART0,
	DMACH_UART1,
	DMACH_UART2,
	DMACH_TIMER,
	DMACH_I2S0_IN,
	DMACH_I2S0_OUT,
	DMACH_I2S0_OUT_S,
	DMACH_PCM0_RX,
	DMACH_PCM0_TX,
	DMACH_PCM1_RX,
	DMACH_PCM1_TX,
	DMACH_PCM2_RX,
	DMACH_PCM2_TX,
	DMACH_MIC_IN,
	DMACH_USB_EP1,
	DMACH_USB_EP2,
	DMACH_USB_EP3,
	DMACH_USB_EP4,
	DMACH_UART0_SRC2,	/* s3c2412 second uart sources */
	DMACH_UART1_SRC2,
	DMACH_UART2_SRC2,
	DMACH_UART3,		/* s3c2443 has extra uart */
	DMACH_UART3_SRC2,
	DMACH_I2S1_IN,		/* S3C6400 */
	DMACH_I2S1_OUT,
	DMACH_SPI0_RX,
	DMACH_SPI0_TX,
	DMACH_SPI1_RX,
	DMACH_SPI1_TX,
	DMACH_SPI2_RX,
	DMACH_SPI2_TX,
	DMACH_I2S2_IN,
	DMACH_I2S2_OUT,
	DMACH_AC97_PCMOUT,
	DMACH_AC97_PCMIN,
	DMACH_AC97_MICIN,
	DMACH_ONENAND_IN,
	DMACH_SPDIF_OUT,
	DMACH_UART0_RX,
	DMACH_UART0_TX,
	DMACH_UART1_RX,
	DMACH_UART1_TX,
	DMACH_UART2_RX,
	DMACH_UART2_TX,
	DMACH_UART3_RX,
	DMACH_UART3_TX,
	DMACH_IRDA,
	DMACH_MSM_REQ0,
	DMACH_MSM_REQ1,
	DMACH_MSM_REQ2,
	DMACH_MSM_REQ3,
	DMACH_EXTGPIO,
	DMACH_PWM,
	DMACH_HSI_RX,
	DMACH_HSI_TX,
	DMACH_I2S0_TX,
	DMACH_I2S0_RX,
	DMACH_I2S1_TX,
	DMACH_I2S1_RX,
	DMACH_I2S2_TX,
	DMACH_I2S2_RX,
	/* Keep these M->M channels contiguous and at the end */
	DMACH_MTOM_0,
	DMACH_MTOM_1,
	DMACH_MTOM_2,
	DMACH_MTOM_3,
	DMACH_MTOM_4,
	DMACH_MTOM_5,
	DMACH_MTOM_6,
	DMACH_MTOM_7,
	DMACH_MTOM_8,
	DMACH_MTOM_9,
	DMACH_MTOM_10,
	DMACH_MTOM_11,
	DMACH_MTOM_12,
	DMACH_MTOM_13,
	DMACH_MTOM_14,
	DMACH_MTOM_15,
	DMACH_MTOM_16,
	DMACH_MTOM_17,
	DMACH_MTOM_18,
	DMACH_MAX,		/* the end entry */
};

#define DMACH_3D_M2M	DMACH_MTOM_0

/*
 * There's nothing 3D about Mem->Mem transfers.
 * Still keep them for backward compatibility.
 */
#define DMACH_3D_M2M0	DMACH_MTOM_0
#define DMACH_3D_M2M1	DMACH_MTOM_1
#define DMACH_3D_M2M2	DMACH_MTOM_2
#define DMACH_3D_M2M3	DMACH_MTOM_3
#define DMACH_3D_M2M4	DMACH_MTOM_4
#define DMACH_3D_M2M5	DMACH_MTOM_5
#define DMACH_3D_M2M6	DMACH_MTOM_6
#define DMACH_3D_M2M7	DMACH_MTOM_7
#define S3C_DMA_CONTROLLERS        	(3)
#define S3C_CHANNELS_PER_DMA       	(8)
#define S3C_CANDIDATE_CHANNELS_PER_DMA  (32)
#define S3C_DMA_CHANNELS		(S3C_DMA_CONTROLLERS*S3C_CHANNELS_PER_DMA)

/* types */

enum s5pv210_dma_state {
	S5P_DMA_IDLE,
	S5P_DMA_RUNNING,
	S5P_DMA_PAUSED
};

/* enum s5pv210_dma_loadst
 *
 * This represents the state of the DMA engine, wrt to the loaded / running
 * transfers. Since we don't have any way of knowing exactly the state of
 * the DMA transfers, we need to know the state to make decisions on wether
 * we can
 *
 * S5PV210_DMA_NONE
 *
 * There are no buffers loaded (the channel should be inactive)
 *
 * S5PV210_DMA_1LOADED
 *
 * There is one buffer loaded, however it has not been confirmed to be
 * loaded by the DMA engine. This may be because the channel is not
 * yet running, or the DMA driver decided that it was too costly to
 * sit and wait for it to happen.
 *
 * S5PV210_DMA_1RUNNING
 *
 * The buffer has been confirmed running, and not finisged
 *
 * S5PV210_DMA_1LOADED_1RUNNING
 *
 * There is a buffer waiting to be loaded by the DMA engine, and one
 * currently running.
*/

enum s5pv210_dma_loadst {
	S5P_DMALOAD_NONE,
	S5P_DMALOAD_1LOADED,
	S5P_DMALOAD_1RUNNING,
	S5P_DMALOAD_1LOADED_1RUNNING,
};

/* dma buffer */

struct s5p_dma_buf;

/* s5p_dma_buf
 *
 * internally used buffer structure to describe a queued or running
 * buffer.
*/

struct s5p_dma_buf {
	struct s5p_dma_buf	*next;
	int			magic;		/* magic */
	int			size;		/* buffer size in bytes */
	dma_addr_t		data;		/* start of DMA data */
	dma_addr_t		ptr;		/* where the DMA got to [1] */
	void			*id;		/* client's id */
	dma_addr_t		mcptr;		/* physical pointer to a set of micro codes */
	unsigned long		*mcptr_cpu;	/* virtual pointer to a set of micro codes */
};

/* [1] is this updated for both recv/send modes? */

struct s5p_dma_stats {
	unsigned long		loads;
	unsigned long		timeout_longest;
	unsigned long		timeout_shortest;
	unsigned long		timeout_avg;
	unsigned long		timeout_failed;
};

struct s3c2410_dma_map;

/* struct s3c2410_dma_chan
 *
 * full state information for each DMA channel
*/

struct s3c2410_dma_chan;

typedef struct s3c2410_dma_chan s3c_dma_controller_t;

struct s3c2410_dma_chan {
	/* channel state flags and information */
	unsigned char		number;      /* number of this dma channel */
	unsigned char		in_use;      /* channel allocated */
	unsigned char		irq_claimed; /* irq claimed for channel */
	unsigned char		irq_enabled; /* irq enabled for channel */
	unsigned char		xfer_unit;   /* size of an transfer */
	unsigned char 		running;	/* cgannel bitmap for channels in RUMMING state*/

	/* channel state */
	enum s5pv210_dma_state	state;
	enum s5pv210_dma_loadst	load_state;
	struct s3c2410_dma_client *client;

	/* channel configuration */
	enum s3c2410_dmasrc	source;
	unsigned long		dev_addr;
	unsigned long		load_timeout;
	unsigned int		flags;		/* channel flags */

	struct s5p_dma_map	*map;		/* channel hw maps */

	/* channel's hardware position and configuration */
	void __iomem		*regs;		/* channels registers */
	void __iomem		*addr_reg;	/* data address register */
	unsigned int		irq;		/* channel irq */
	unsigned long		dcon;		/* default value of DCON */

	/* channel's clock*/
	struct clk		*clk;		/*clock for this controller*/

	/* driver handles */
	s3c2410_dma_cbfn_t	callback_fn;	/* buffer done callback */
	s3c2410_dma_opfn_t	op_fn;		/* channel op callback */

	/* stats gathering */
	struct s5p_dma_stats	*stats;
	struct s5p_dma_stats	stats_store;

	/* buffer list and information */
	struct s5p_dma_buf	*curr;		/* current dma buffer */
	struct s5p_dma_buf	*next;		/* next buffer to load */
	struct s5p_dma_buf	*end;		/* end of queue */

	/* system device */
	struct sys_device	dev;

	unsigned int		index;        	/* channel index */
	unsigned int		config_flags;	/* channel flags */
	unsigned int		control_flags;	/* channel flags */
	s3c_dma_controller_t	*dma_con;
};

typedef unsigned long dma_device_t;

static bool __inline__ s3c_dma_has_circular(void)
{
#ifdef CONFIG_NEW_DMA_PL330
	return true;
#else
	return false;
#endif
}

#endif /* __ASM_ARCH_DMA_H */

#else	/* CONFIG_OLD_DMA_PL330 */

#ifndef __MACH_DMA_H
#define __MACH_DMA_H

/* This platform uses the common S3C DMA API driver for PL330 */
#include <plat/s3c-dma-pl330.h>

#endif /* __MACH_DMA_H */
#endif /* End of CONFIG_OLD_DMA_PL330 */
