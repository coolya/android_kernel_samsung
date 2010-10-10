/*
 * s5p-i2s_sec.c  --  Secondary Fifo driver for I2S_v5
 *
 * (c) 2009 Samsung Electronics Co. Ltd
 *   - Jaswinder Singh Brar <jassi.brar@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <plat/regs-iis.h>

#include <mach/regs-audss.h>
#include <mach/dma.h>
#include "s3c-dma.h"
#include "s3c-i2s-v2.h"

#define S3C64XX_DIV_BCLK	S3C_I2SV2_DIV_BCLK
#define S3C64XX_DIV_RCLK	S3C_I2SV2_DIV_RCLK
#define S3C64XX_DIV_PRESCALER	S3C_I2SV2_DIV_PRESCALER

#define S3C64XX_CLKSRC_PCLK	(0)
#define S3C64XX_CLKSRC_MUX	(1)
#define S3C64XX_CLKSRC_CDCLK    (2)

static void __iomem *s5p_i2s0_regs;

static struct s3c2410_dma_client s5p_dma_client_outs = {
	.name		= "I2S_Sec PCM Stereo out"
};

static struct s3c_dma_params s5p_i2s_sec_pcm_out = {
	.channel	= DMACH_I2S0S_TX,
	.client		= &s5p_dma_client_outs,
	.dma_size	= 4,
};

static inline struct s3c_i2sv2_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

static void s5p_snd_rxctrl(int on)
{
	u32 fic, con, mod;

	pr_debug("%s(%d)\n", __func__, on);

	fic = readl(s5p_i2s0_regs + S3C2412_IISFIC);
	con = readl(s5p_i2s0_regs + S3C2412_IISCON);
	mod = readl(s5p_i2s0_regs + S3C2412_IISMOD);

	pr_debug("%s: On=%d..IIS: CON=%x MOD=%x FIC=%x\n",
				__func__, on, con, mod, fic);

	if (on) {
		con |= S3C2412_IISCON_RXDMA_ACTIVE | S3C2412_IISCON_IIS_ACTIVE;
		con &= ~S3C2412_IISCON_RXDMA_PAUSE;
		con &= ~S3C2412_IISCON_RXCH_PAUSE;

		switch (mod & S3C2412_IISMOD_MODE_MASK) {
		case S3C2412_IISMOD_MODE_TXRX:
		case S3C2412_IISMOD_MODE_RXONLY:
			/* do nothing, we are in the right mode */
			break;

		case S3C2412_IISMOD_MODE_TXONLY:
			mod &= ~S3C2412_IISMOD_MODE_MASK;
			mod |= S3C2412_IISMOD_MODE_TXRX;
			break;

		default:
			printk(KERN_WARNING
				"RXEN: Invalid MODE %x in IISMOD\n",
				mod & S3C2412_IISMOD_MODE_MASK);
		}

		writel(mod, s5p_i2s0_regs + S3C2412_IISMOD);
		writel(con, s5p_i2s0_regs + S3C2412_IISCON);
	} else {
		/* See txctrl notes on FIFOs. */

		con &= ~S3C2412_IISCON_RXDMA_ACTIVE;
		con |=  S3C2412_IISCON_RXDMA_PAUSE;
		con |=  S3C2412_IISCON_RXCH_PAUSE;

		switch (mod & S3C2412_IISMOD_MODE_MASK) {
		case S3C2412_IISMOD_MODE_RXONLY:
			con &= ~S3C2412_IISCON_IIS_ACTIVE;
			mod &= ~S3C2412_IISMOD_MODE_MASK;
			break;

		case S3C2412_IISMOD_MODE_TXRX:
			mod &= ~S3C2412_IISMOD_MODE_MASK;
			mod |= S3C2412_IISMOD_MODE_TXONLY;
			break;

		default:
			printk(KERN_WARNING
				"RXDIS: Invalid MODE %x in IISMOD\n",
				mod & S3C2412_IISMOD_MODE_MASK);
		}

		writel(con, s5p_i2s0_regs + S3C2412_IISCON);
		writel(mod, s5p_i2s0_regs + S3C2412_IISMOD);
	}

	fic = readl(s5p_i2s0_regs + S3C2412_IISFIC);
	pr_debug("%s: IIS: CON=%x MOD=%x FIC=%x\n", __func__, con, mod, fic);
}

static void s5p_snd_txctrl(int on)
{
	u32 iiscon, iismod;
	iiscon = readl(s5p_i2s0_regs + S3C2412_IISCON);
	iismod = readl(s5p_i2s0_regs + S3C2412_IISMOD);
	pr_debug("%s: On=%d . IIS: CON=%x MOD=%x\n",
			__func__, on, iiscon, iismod);

	if (on) {
		iiscon |= S3C2412_IISCON_IIS_ACTIVE;
		iiscon &= ~S3C2412_IISCON_TXCH_PAUSE;
		iiscon &= ~S5P_IISCON_TXSDMAPAUSE;
		iiscon |= S5P_IISCON_TXSDMACTIVE;

		switch (iismod & S3C2412_IISMOD_MODE_MASK) {
		case S3C2412_IISMOD_MODE_TXONLY:
		case S3C2412_IISMOD_MODE_TXRX:
			/* do nothing, we are in the right mode */
			break;

		case S3C2412_IISMOD_MODE_RXONLY:
			iismod &= ~S3C2412_IISMOD_MODE_MASK;
			iismod |= S3C2412_IISMOD_MODE_TXRX;
			break;

		default:
			printk(KERN_WARNING
				"TXEN: Invalid MODE %x in IISMOD\n",
				iismod & S3C2412_IISMOD_MODE_MASK);
			break;
		}

		writel(iiscon, s5p_i2s0_regs + S3C2412_IISCON);
		writel(iismod, s5p_i2s0_regs + S3C2412_IISMOD);
	} else {
		iiscon |= S5P_IISCON_TXSDMAPAUSE;
		iiscon &= ~S5P_IISCON_TXSDMACTIVE;

		/* return if primary is active */
		if (iiscon & S3C2412_IISCON_TXDMA_ACTIVE) {
			writel(iiscon, s5p_i2s0_regs + S3C2412_IISCON);
			return;
		}

		iiscon |= S3C2412_IISCON_TXCH_PAUSE;

		switch (iismod & S3C2412_IISMOD_MODE_MASK) {
		case S3C2412_IISMOD_MODE_TXRX:
			iismod &= ~S3C2412_IISMOD_MODE_MASK;
			iismod |= S3C2412_IISMOD_MODE_RXONLY;
			break;

		case S3C2412_IISMOD_MODE_TXONLY:
			iismod &= ~S3C2412_IISMOD_MODE_MASK;
			iiscon &= ~S3C2412_IISCON_IIS_ACTIVE;
			break;

		default:
			printk(KERN_WARNING
				"TXDIS: Invalid MODE %x in IISMOD\n",
				iismod & S3C2412_IISMOD_MODE_MASK);
			break;
		}


		writel(iismod, s5p_i2s0_regs + S3C2412_IISMOD);
		writel(iiscon, s5p_i2s0_regs + S3C2412_IISCON);
	}
}

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s5p_snd_lrsync(void)
{
	u32 iiscon;
	unsigned long loops = msecs_to_loops(1);

	pr_debug("Entered %s\n", __func__);

	while (--loops) {
		iiscon = readl(s5p_i2s0_regs + S3C2412_IISCON);
		if (iiscon & S3C2412_IISCON_LRINDEX)
			break;

		cpu_relax();
	}

	if (!loops) {
		pr_debug("%s: timeout\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

int s5p_i2s_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	u32 iismod;

	snd_soc_dai_set_dma_data(rtd->dai->cpu_dai,
				substream, &s5p_i2s_sec_pcm_out);

	iismod = readl(s5p_i2s0_regs + S3C2412_IISMOD);

	/* Copy the same bps as Primary */
	iismod &= ~S5P_IISMOD_BLCSMASK;
	iismod |= ((iismod & S5P_IISMOD_BLCPMASK) << 2);

	writel(iismod, s5p_i2s0_regs + S3C2412_IISMOD);

	return 0;
}
EXPORT_SYMBOL_GPL(s5p_i2s_hw_params);

int s5p_i2s_startup(struct snd_soc_dai *dai)
{
	u32 iiscon, iisfic;
	u32 iismod, iisahb;

	iiscon = readl(s5p_i2s0_regs + S3C2412_IISCON);
	iismod = readl(s5p_i2s0_regs + S3C2412_IISMOD);
	iisahb = readl(s5p_i2s0_regs + S5P_IISAHB);

	iisahb |= (S5P_IISAHB_DMARLD | S5P_IISAHB_DISRLDINT);
	iismod |= S5P_IISMOD_TXSLP;

	writel(iisahb, s5p_i2s0_regs + S5P_IISAHB);
	writel(iismod, s5p_i2s0_regs + S3C2412_IISMOD);

	s5p_snd_txctrl(0);
	/* 
	* Don't turn-off RX setting as recording may be
	* active during playback startup
	* s5p_snd_rxctrl(0);
	*/

	/* FIFOs must be flushed before enabling PSR
	*  and other MOD bits, so we do it here. */
	if (iiscon & S5P_IISCON_TXSDMACTIVE)
		return 0;

	iisfic = readl(s5p_i2s0_regs + S5P_IISFICS);
	iisfic |= S3C2412_IISFIC_TXFLUSH;
	writel(iisfic, s5p_i2s0_regs + S5P_IISFICS);

	do {
		cpu_relax();
	} while ((__raw_readl(s5p_i2s0_regs + S5P_IISFICS) >> 8) & 0x7f);

	iisfic = readl(s5p_i2s0_regs + S5P_IISFICS);
	iisfic &= ~S3C2412_IISFIC_TXFLUSH;
	writel(iisfic, s5p_i2s0_regs + S5P_IISFICS);

	return 0;
}
EXPORT_SYMBOL_GPL(s5p_i2s_startup);

int i2s_trigger_stop ;
EXPORT_SYMBOL_GPL(i2s_trigger_stop);
int s5p_i2s_trigger(struct snd_pcm_substream *substream,
		int cmd, struct snd_soc_dai *dai)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* We don't configure clocks from this Sec i/f.
		 * So, we simply wait enough time for LRSYNC to
		 * get synced and not check return 'error'
		 */
		s5p_snd_lrsync();
		s5p_snd_txctrl(1);
		i2s_trigger_stop = 0;
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		i2s_trigger_stop = 1;
		s5p_snd_txctrl(0);
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(s5p_i2s_trigger);

struct snd_soc_dai i2s_sec_fifo_dai = {
	.name = "i2s-sec-fifo",
	.id = 0,
};
EXPORT_SYMBOL_GPL(i2s_sec_fifo_dai);


void s5p_i2s_sec_init(void *regs, dma_addr_t phys_base)
{
	u32 val;
#ifdef CONFIG_ARCH_S5PV210
	/* We use I2SCLK for rate generation, so set EPLLout as
	 * the parent of I2SCLK.
	 */
	val = readl(S5P_CLKSRC_AUDSS);
	val &= ~(0x3<<2);
	val |= (1<<0);
	writel(val, S5P_CLKSRC_AUDSS);

	val = readl(S5P_CLKGATE_AUDSS);
	val |= (0x7f<<0);
	writel(val, S5P_CLKGATE_AUDSS);
#else
	#error INITIALIZE HERE!
#endif

	s5p_i2s0_regs = regs;
	s5p_i2s_sec_pcm_out.dma_addr = phys_base + S5P_IISTXDS;

	s5p_snd_rxctrl(0);
	s5p_snd_txctrl(0);
	s5p_idma_init(regs);
}

/* Module information */
MODULE_AUTHOR("Jaswinder Singh <jassi.brar@samsung.com>");
MODULE_DESCRIPTION("S5P I2S-SecFifo SoC Interface");
MODULE_LICENSE("GPL");
