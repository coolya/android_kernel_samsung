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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>

#include <mach/map.h>
#include <mach/dma.h>

#include <plat/cpu.h>

#ifdef CONFIG_OLD_DMA_PL330

#include <plat/dma-s5p.h>

/* M2M DMAC */
#define MAP0(x) { \
		[0]	= (x) | DMA_CH_VALID,	\
		[1]	= (x) | DMA_CH_VALID,	\
		[2]	= (x) | DMA_CH_VALID,	\
		[3]	= (x) | DMA_CH_VALID,	\
		[4]	= (x) | DMA_CH_VALID,	\
		[5]     = (x) | DMA_CH_VALID,	\
		[6]	= (x) | DMA_CH_VALID,	\
		[7]     = (x) | DMA_CH_VALID,	\
		[8]	= (x),	\
		[9]	= (x),	\
		[10]	= (x),	\
		[11]	= (x),	\
		[12]	= (x),	\
		[13]    = (x),	\
		[14]	= (x),	\
		[15]    = (x),	\
		[16]	= (x),	\
		[17]	= (x),	\
		[18]	= (x),	\
		[19]	= (x),	\
		[20]	= (x),	\
		[21]    = (x),	\
		[22]	= (x),	\
		[23]    = (x),	\
	}

/* Peri-DMAC 0 */
#define MAP1(x) { \
		[0]	= (x),	\
		[1]	= (x),	\
		[2]	= (x),	\
		[3]	= (x),	\
		[4]	= (x),	\
		[5]     = (x),	\
		[6]	= (x),	\
		[7]     = (x),	\
		[8]	= (x) | DMA_CH_VALID,	\
		[9]	= (x) | DMA_CH_VALID,	\
		[10]	= (x) | DMA_CH_VALID,	\
		[11]	= (x) | DMA_CH_VALID,	\
		[12]	= (x) | DMA_CH_VALID,	\
		[13]    = (x) | DMA_CH_VALID,	\
		[14]	= (x) | DMA_CH_VALID,	\
		[15]    = (x) | DMA_CH_VALID,	\
		[16]	= (x),	\
		[17]	= (x),	\
		[18]	= (x),	\
		[19]	= (x),	\
		[20]	= (x),	\
		[21]    = (x),	\
		[22]	= (x),	\
		[23]    = (x),	\
	}

/* Peri-DMAC 1 */
#define MAP2(x) { \
		[0]	= (x),	\
		[1]	= (x),	\
		[2]	= (x),	\
		[3]	= (x),	\
		[4]	= (x),	\
		[5]     = (x),	\
		[6]	= (x),	\
		[7]     = (x),	\
		[8]	= (x),	\
		[9]	= (x),	\
		[10]	= (x),	\
		[11]	= (x),	\
		[12]	= (x),	\
		[13]    = (x),	\
		[14]	= (x),	\
		[15]    = (x),	\
		[16]	= (x) | DMA_CH_VALID,	\
		[17]	= (x) | DMA_CH_VALID,	\
		[18]	= (x) | DMA_CH_VALID,	\
		[19]	= (x) | DMA_CH_VALID,	\
		[20]	= (x) | DMA_CH_VALID,	\
		[21]    = (x) | DMA_CH_VALID,	\
		[22]	= (x) | DMA_CH_VALID,	\
		[23]    = (x) | DMA_CH_VALID,	\
	}

/* DMA request sources of Peri-DMAC 1 */
#define S3C_PDMA1_UART0_RX	0
#define S3C_PDMA1_UART0_TX	1
#define S3C_PDMA1_UART1_RX	2
#define S3C_PDMA1_UART1_TX	3
#define S3C_PDMA1_UART2_RX	4
#define S3C_PDMA1_UART2_TX	5
#define S3C_PDMA1_UART3_RX	6
#define S3C_PDMA1_UART3_TX	7
#define S3C_PDMA1_Reserved	8
#define S3C_PDMA1_I2S0_RX	9
#define S3C_PDMA1_I2S0_TX	10
#define S3C_PDMA1_I2S0S_TX	11
#define S3C_PDMA1_I2S1_RX	12
#define S3C_PDMA1_I2S1_TX	13
#define S3C_PDMA1_I2S2_RX	14
#define S3C_PDMA1_I2S2_TX	15
#define S3C_PDMA1_SPI0_RX	16
#define S3C_PDMA1_SPI0_TX	17
#define S3C_PDMA1_SPI1_RX	18
#define S3C_PDMA1_SPI1_TX	19
#define S3C_PDMA1_SPI2_RX	20
#define S3C_PDMA1_SPI2_TX	21
#define S3C_PDMA1_PCM0_RX	22
#define S3C_PDMA1_PCM0_TX	23
#define S3C_PDMA1_PCM1_RX	24
#define S3C_PDMA1_PCM1_TX	25
#define S3C_PDMA1_MSM_REQ0	26
#define S3C_PDMA1_MSM_REQ1	27
#define S3C_PDMA1_MSM_REQ2	28
#define S3C_PDMA1_MSM_REQ3	29
#define S3C_PDMA1_PCM2_RX	30
#define S3C_PDMA1_PCM2_TX	31

/* DMA request sources of Peri-DMAC 0 */
#define S3C_PDMA0_UART0_RX	0
#define S3C_PDMA0_UART0_TX	1
#define S3C_PDMA0_UART1_RX	2
#define S3C_PDMA0_UART1_TX	3
#define S3C_PDMA0_UART2_RX	4
#define S3C_PDMA0_UART2_TX	5
#define S3C_PDMA0_UART3_RX	6
#define S3C_PDMA0_UART3_TX	7
#define S3C_PDMA0_Reserved_7	8
#define S3C_PDMA0_I2S0_RX	9
#define S3C_PDMA0_I2S0_TX	10
#define S3C_PDMA0_I2S0S_TX	11
#define S3C_PDMA0_I2S1_RX	12
#define S3C_PDMA0_I2S1_TX	13
#define S3C_PDMA0_Reserved_6	14
#define S3C_PDMA0_Reserved_5	15
#define S3C_PDMA0_SPI0_RX	16
#define S3C_PDMA0_SPI0_TX	17
#define S3C_PDMA0_SPI1_RX	18
#define S3C_PDMA0_SPI1_TX	19
#define S3C_PDMA0_SPI2_RX	20
#define S3C_PDMA0_SPI2_TX	21
#define S3C_PDMA0_AC_MICIN	22
#define S3C_PDMA0_AC_PCMIN	23
#define S3C_PDMA0_AC_PCMOUT	24
#define S3C_PDMA0_Reserved_4	25
#define S3C_PDMA0_PWM		26
#define S3C_PDMA0_SPDIF		27
#define S3C_PDMA0_Reserved_3	28
#define S3C_PDMA0_Reserved_2	29
#define S3C_PDMA0_Reserved_1	30
#define S3C_PDMA0_Reserved	31

/* DMA request sources of M2M-DMAC */
#define S3C_DMA_M2M		0

static struct s5p_dma_map __initdata s5pv210_dma_mappings[] = {
	[DMACH_UART0_TX] = {
		.name	= "uart0-dma-tx",
		.channels	= MAP1(S3C_PDMA0_UART0_TX),
		.hw_addr.to	= S3C_PDMA0_UART0_TX,
	},
	[DMACH_UART0_RX] = {
		.name	= "uart0-dma-rx",
		.channels	= MAP1(S3C_PDMA0_UART0_RX),
		.hw_addr.from	= S3C_PDMA0_UART0_RX,
	},
	[DMACH_UART1_TX] = {
		.name		= "uart1-dma-tx",
		.channels	= MAP1(S3C_PDMA0_UART1_TX),
		.hw_addr.to	= S3C_PDMA0_UART1_TX,
	},
	[DMACH_UART1_RX] = {
		.name	= "uart1-dma-rx",
		.channels	= MAP1(S3C_PDMA0_UART1_RX),
		.hw_addr.from	= S3C_PDMA0_UART1_RX,
	},
	[DMACH_UART2_TX] = {
		.name		= "uart2-dma-tx",
		.channels	= MAP1(S3C_PDMA0_UART2_TX),
		.hw_addr.to	= S3C_PDMA0_UART2_TX,
	},
	[DMACH_UART2_RX] = {
		.name	= "uart2-dma-rx",
		.channels	= MAP1(S3C_PDMA0_UART2_RX),
		.hw_addr.from	= S3C_PDMA0_UART2_RX,
	},
	[DMACH_UART3_TX] = {
		.name		= "uart3-dma-tx",
		.channels	= MAP1(S3C_PDMA0_UART3_TX),
		.hw_addr.to	= S3C_PDMA0_UART3_TX,
	},
	[DMACH_UART3_RX] = {
		.name	= "uart3-dma-rx",
		.channels	= MAP1(S3C_PDMA0_UART3_RX),
		.hw_addr.from	= S3C_PDMA0_UART3_RX,
	},
	[DMACH_I2S0_RX] = {
		.name	= "i2s0-in",
		.channels	= MAP1(S3C_PDMA1_I2S0_RX),
		.hw_addr.from	= S3C_PDMA1_I2S0_RX,
	},
	[DMACH_I2S0_TX] = {
		.name	= "i2s0-out",
		.channels	= MAP1(S3C_PDMA1_I2S0_TX),
		.hw_addr.to	= S3C_PDMA1_I2S0_TX,
	},
	[DMACH_I2S0S_TX] = {
		.name		= "i2s-v50-sec_out",
		.channels	= MAP2(S3C_PDMA1_I2S0S_TX),
		.hw_addr.to	= S3C_PDMA1_I2S0S_TX,
	},
	[DMACH_I2S1_RX] = {
		.name		= "i2s1-in",
		.channels	= MAP2(S3C_PDMA1_I2S1_RX),
		.hw_addr.from	= S3C_PDMA1_I2S1_RX,
	},
	[DMACH_I2S1_TX] = {
		.name		= "i2s1-out",
		.channels	= MAP2(S3C_PDMA1_I2S1_TX),
		.hw_addr.to	= S3C_PDMA1_I2S1_TX,
	},
	[DMACH_I2S2_RX] = {
		.name		= "i2s2-in",
		.channels	= MAP1(S3C_PDMA1_I2S2_RX),
		.hw_addr.from	= S3C_PDMA1_I2S2_RX,
	},
	[DMACH_I2S2_TX] = {
		.name		= "i2s2-out",
		.channels	= MAP1(S3C_PDMA1_I2S2_TX),
		.hw_addr.to	= S3C_PDMA1_I2S2_TX,
	},
	[DMACH_SPI0_RX] = {
		.name		= "spi0-in",
		.channels	= MAP1(S3C_PDMA0_SPI0_RX),
		.hw_addr.from	= S3C_PDMA0_SPI0_RX,
	},
	[DMACH_SPI0_TX] = {
		.name		= "spi0-out",
		.channels	= MAP1(S3C_PDMA0_SPI0_TX),
		.hw_addr.to	= S3C_PDMA0_SPI0_TX,
	},
	[DMACH_SPI1_RX] = {
		.name		= "spi1-in",
		.channels	= MAP2(S3C_PDMA1_SPI1_RX),
		.hw_addr.from	= S3C_PDMA1_SPI1_RX,
	},
	[DMACH_SPI1_TX] = {
		.name		= "spi1-out",
		.channels	= MAP2(S3C_PDMA1_SPI1_TX),
		.hw_addr.to	= S3C_PDMA1_SPI1_TX,
	},
	[DMACH_SPI2_RX] = {
		.name	= "spi2-in",
		.channels	= MAP1(S3C_PDMA1_SPI2_RX),
		.hw_addr.from	= S3C_PDMA1_SPI2_RX,
	},
	[DMACH_SPI2_TX] = {
		.name	= "spi2-out",
		.channels	= MAP1(S3C_PDMA1_SPI2_TX),
		.hw_addr.to	= S3C_PDMA1_SPI2_TX,
	},
	[DMACH_PCM0_RX] = {
		.name		= "pcm0-in",
		.channels	= MAP2(S3C_PDMA1_PCM0_RX),
		.hw_addr.from	= S3C_PDMA1_PCM0_RX,
	},
	[DMACH_PCM0_TX] = {
		.name		= "pcm0-out",
		.channels	= MAP2(S3C_PDMA1_PCM0_TX),
		.hw_addr.to	= S3C_PDMA1_PCM0_TX,
	},
	[DMACH_PCM1_RX] = {
		.name		= "pcm1-in",
		.channels	= MAP2(S3C_PDMA1_PCM1_RX),
		.hw_addr.from	= S3C_PDMA1_PCM1_RX,
	},
	[DMACH_PCM1_TX] = {
		.name		= "pcm1-out",
		.channels	= MAP2(S3C_PDMA1_PCM1_TX),
		.hw_addr.to	= S3C_PDMA1_PCM1_TX,
	},
	[DMACH_PCM2_RX] = {
		.name		= "pcm2-in",
		.channels	= MAP2(S3C_PDMA1_PCM2_RX),
		.hw_addr.from	= S3C_PDMA1_PCM2_RX,
	},
	[DMACH_PCM2_TX] = {
		.name		= "pcm2-out",
		.channels	= MAP2(S3C_PDMA1_PCM2_TX),
		.hw_addr.to	= S3C_PDMA1_PCM2_TX,
	},
	[DMACH_AC97_PCMOUT] = {
		.name		= "ac97-pcm-out",
		.channels	= MAP1(S3C_PDMA0_AC_PCMOUT),
		.hw_addr.to	= S3C_PDMA0_AC_PCMOUT,
	},
	[DMACH_AC97_PCMIN] = {
		.name		= "ac97-pcm-in",
		.channels	= MAP1(S3C_PDMA0_AC_PCMIN),
		.hw_addr.from	= S3C_PDMA0_AC_PCMIN,
	},
	[DMACH_AC97_MICIN] = {
		.name		= "ac97-mic-in",
		.channels	= MAP1(S3C_PDMA0_AC_MICIN),
		.hw_addr.from	= S3C_PDMA0_AC_MICIN,
	},
	[DMACH_ONENAND_IN] = {
		.name		= "onenand-in",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M0] = {
		.name		= "3D-M2M0",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M1] = {
		.name		= "3D-M2M1",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M2] = {
		.name		= "3D-M2M2",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M3] = {
		.name		= "3D-M2M3",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M4] = {
		.name		= "3D-M2M4",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M5] = {
		.name		= "3D-M2M5",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M6] = {
		.name		= "3D-M2M6",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_3D_M2M7] = {
		.name		= "3D-M2M7",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_SPDIF_OUT] = {
		.name           = "spdif-out",
		.channels       = MAP1(S3C_PDMA0_SPDIF),
		.hw_addr.to     = S3C_PDMA0_SPDIF,
	},
};

static void s5pv210_dma_select(struct s3c2410_dma_chan *chan,
			       struct s5p_dma_map *map)
{
	chan->map = map;
}

static struct s5p_dma_selection __initdata s5pv210_dma_sel = {
	.select		= s5pv210_dma_select,
	.dcon_mask	= 0,
	.map		= s5pv210_dma_mappings,
	.map_size	= ARRAY_SIZE(s5pv210_dma_mappings),
};

static int __init s5pv210_dma_add(struct sys_device *sysdev)
{
	s5p_dma_init(S3C_DMA_CHANNELS, IRQ_MDMA, 0x20);
	return s5p_dma_init_map(&s5pv210_dma_sel);
}

static struct sysdev_driver s5pv210_dma_driver = {
	.add	= s5pv210_dma_add,
};


#else

#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <plat/devs.h>
#include <plat/irqs.h>
#include <plat/s3c-pl330-pdata.h>

#include <mach/irqs.h>


static u64 dma_dmamask = DMA_BIT_MASK(32);

static struct resource s5pv210_pdma0_resource[] = {
	[0] = {
		.start  = S5PV210_PA_PDMA0,
		.end    = S5PV210_PA_PDMA0 + SZ_4K,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_PDMA0,
		.end	= IRQ_PDMA0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct s3c_pl330_platdata s5pv210_pdma0_pdata = {
	.peri = {
		[0] = DMACH_UART0_RX,
		[1] = DMACH_UART0_TX,
		[2] = DMACH_UART1_RX,
		[3] = DMACH_UART1_TX,
		[4] = DMACH_UART2_RX,
		[5] = DMACH_UART2_TX,
		[6] = DMACH_UART3_RX,
		[7] = DMACH_UART3_TX,
		[8] = DMACH_MAX,
		[9] = DMACH_I2S0_RX,
		[10] = DMACH_I2S0_TX,
		[11] = DMACH_I2S0S_TX,
		[12] = DMACH_I2S1_RX,
		[13] = DMACH_I2S1_TX,
		[14] = DMACH_MAX,
		[15] = DMACH_MAX,
		[16] = DMACH_SPI0_RX,
		[17] = DMACH_SPI0_TX,
		[18] = DMACH_SPI1_RX,
		[19] = DMACH_SPI1_TX,
		[20] = DMACH_MAX,
		[21] = DMACH_MAX,
		[22] = DMACH_AC97_MICIN,
		[23] = DMACH_AC97_PCMIN,
		[24] = DMACH_AC97_PCMOUT,
		[25] = DMACH_MAX,
		[26] = DMACH_PWM,
		[27] = DMACH_SPDIF,
		[28] = DMACH_MAX,
		[29] = DMACH_MAX,
		[30] = DMACH_MAX,
		[31] = DMACH_MAX,
	},
};

static struct platform_device s5pv210_device_pdma0 = {
	.name		= "s3c-pl330",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s5pv210_pdma0_resource),
	.resource	= s5pv210_pdma0_resource,
	.dev		= {
		.dma_mask = &dma_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &s5pv210_pdma0_pdata,
	},
};

static struct resource s5pv210_pdma1_resource[] = {
	[0] = {
		.start  = S5PV210_PA_PDMA1,
		.end    = S5PV210_PA_PDMA1 + SZ_4K,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_PDMA1,
		.end	= IRQ_PDMA1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct s3c_pl330_platdata s5pv210_pdma1_pdata = {
	.peri = {
		[0] = DMACH_UART0_RX,
		[1] = DMACH_UART0_TX,
		[2] = DMACH_UART1_RX,
		[3] = DMACH_UART1_TX,
		[4] = DMACH_UART2_RX,
		[5] = DMACH_UART2_TX,
		[6] = DMACH_UART3_RX,
		[7] = DMACH_UART3_TX,
		[8] = DMACH_MAX,
		[9] = DMACH_I2S0_RX,
		[10] = DMACH_I2S0_TX,
		[11] = DMACH_I2S0S_TX,
		[12] = DMACH_I2S1_RX,
		[13] = DMACH_I2S1_TX,
		[14] = DMACH_I2S2_RX,
		[15] = DMACH_I2S2_TX,
		[16] = DMACH_SPI0_RX,
		[17] = DMACH_SPI0_TX,
		[18] = DMACH_SPI1_RX,
		[19] = DMACH_SPI1_TX,
		[20] = DMACH_MAX,
		[21] = DMACH_MAX,
		[22] = DMACH_PCM0_RX,
		[23] = DMACH_PCM0_TX,
		[24] = DMACH_PCM1_RX,
		[25] = DMACH_PCM1_TX,
		[26] = DMACH_MSM_REQ0,
		[27] = DMACH_MSM_REQ1,
		[28] = DMACH_MSM_REQ2,
		[29] = DMACH_MSM_REQ3,
		[30] = DMACH_PCM2_RX,
		[31] = DMACH_PCM2_TX,
	},
};

static struct platform_device s5pv210_device_pdma1 = {
	.name		= "s3c-pl330",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s5pv210_pdma1_resource),
	.resource	= s5pv210_pdma1_resource,
	.dev		= {
		.dma_mask = &dma_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &s5pv210_pdma1_pdata,
	},
};

static struct platform_device *s5pv210_dmacs[] __initdata = {
	&s5pv210_device_pdma0,
	&s5pv210_device_pdma1,
};
#endif

static int __init s5pv210_dma_init(void)
{
#ifdef CONFIG_OLD_DMA_PL330
	return sysdev_driver_register(&s5pv210_sysclass, &s5pv210_dma_driver);
#else
	platform_add_devices(s5pv210_dmacs, ARRAY_SIZE(s5pv210_dmacs));
	return 0;
#endif
}
arch_initcall(s5pv210_dma_init);
