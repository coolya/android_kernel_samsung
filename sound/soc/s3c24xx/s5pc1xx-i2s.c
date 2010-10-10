/* sound/soc/s3c24xx/s5pc1xx-i2s.c
 *
 * ALSA SoC Audio Layer - S3C64XX I2S driver
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>

#include <sound/soc.h>

#include <plat/regs-iis.h>
#include <plat/audio.h>

#include <mach/dma.h>

#include <mach/map.h>
#include <mach/regs-audss.h>
#include <mach/regs-clock.h>
#include <linux/wakelock.h>
#include "s3c-dma.h"
#include "s5pc1xx-i2s.h"

/*
 * The value should be set to maximum of the total number
 * of I2Sv3 controllers that any supported SoC has.
 */
#define MAX_I2SV3	2

static struct s3c2410_dma_client s3c64xx_dma_client_out = {
	.name		= "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c64xx_dma_client_in = {
	.name		= "I2S PCM Stereo in"
};

static struct snd_soc_dai_ops s3c64xx_i2s_dai_ops;
static struct s3c_dma_params s3c64xx_i2s_pcm_stereo_out[MAX_I2SV3];
static struct s3c_dma_params s3c64xx_i2s_pcm_stereo_in[MAX_I2SV3];
static struct s3c_i2sv2_info s3c64xx_i2s[MAX_I2SV3];

struct snd_soc_dai s3c64xx_i2s_dai[MAX_I2SV3];
bool audio_clk_gated;	/* At first, clock & i2s0_pd is enabled in probe() */
EXPORT_SYMBOL_GPL(s3c64xx_i2s_dai);

/* For I2S Clock/Power Gating */
static int tx_clk_enabled ;
static int rx_clk_enabled ;
static int reg_saved_ok ;

void dump_i2s(struct s3c_i2sv2_info *i2s)
{
	printk(KERN_INFO "IISMOD=0x%x..IISCON=0x%x..IISPSR=0x%x\
			IISAHB=0x%x..\n" , readl(i2s->regs + S3C2412_IISMOD),
			readl(i2s->regs + S3C2412_IISCON),
			readl(i2s->regs + S3C2412_IISPSR),
			readl(i2s->regs + S5P_IISAHB));
	printk(KERN_INFO "..AUDSSRC=0x%x..AUDSSDIV=0x%x..\
			AUDSSGATE=0x%x..\n" , readl(S5P_CLKSRC_AUDSS),
			readl(S5P_CLKDIV_AUDSS), readl(S5P_CLKGATE_AUDSS));
}

#define dump_reg(iis)

static inline struct s3c_i2sv2_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

struct clk *s3c64xx_i2s_get_clock(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	u32 iismod = readl(i2s->regs + S3C2412_IISMOD);

	if (iismod & S3C64XX_IISMOD_IMS_SYSMUX)
		return i2s->iis_clk;
	else
		return i2s->iis_ipclk;
}
EXPORT_SYMBOL_GPL(s3c64xx_i2s_get_clock);

void s5p_i2s_set_clk_enabled(struct snd_soc_dai *dai, bool state)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	pr_debug("..entering %s\n", __func__);

	if (state) {
		if (audio_clk_gated == 1)
			regulator_enable(i2s->regulator);

		if (dai->id == 0) {	/* I2S V5.1? */
			clk_enable(i2s->iis_ipclk);
			clk_enable(i2s->iis_clk);
			clk_enable(i2s->iis_busclk);
		}
		audio_clk_gated = 0;
	} else {
		if (dai->id == 0) {	/* I2S V5.1? */
			clk_disable(i2s->iis_busclk);
			clk_disable(i2s->iis_clk);
			clk_disable(i2s->iis_ipclk);
		}

		if (audio_clk_gated == 0)
			regulator_disable(i2s->regulator);

		audio_clk_gated = 1;
	}
}

static int s5p_i2s_wr_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		s5p_i2s_hw_params(substream, params, dai);
	else
		s3c2412_i2s_hw_params(substream, params, dai);

#else
	s3c2412_i2s_hw_params(substream, params, dai);
#endif
	return 0;
}
static int s5p_i2s_wr_trigger(struct snd_pcm_substream *substream,
		int cmd, struct snd_soc_dai *dai)
{
#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		s5p_i2s_trigger(substream, cmd, dai);
	else
		s3c2412_i2s_trigger(substream, cmd, dai);

#else
	s3c2412_i2s_trigger(substream, cmd, dai);
#endif
	return 0;
}

/*
 * Set S3C2412 I2S DAI format
 */
static int s5p_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 iismod;

	pr_debug("Entered %s\n", __func__);

	iismod = readl(i2s->regs + S3C2412_IISMOD);
	pr_debug("hw_params r: IISMOD: %x\n", iismod);

#if defined(CONFIG_PLAT_S3C64XX) || defined(CONFIG_PLAT_S5P)
	/* From Rev1.1 datasheet, we have two master and two slave modes:
	 * IMS[11:10]:
	 *	00 = master mode, fed from PCLK
	 *	01 = master mode, fed from CLKAUDIO
	 *	10 = slave mode, using PCLK
	 *	11 = slave mode, using I2SCLK
	 */
#define IISMOD_MASTER_MASK (1 << 11)
#define IISMOD_SLAVE (1 << 11)
#define IISMOD_MASTER (0 << 11)
#endif

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		i2s->master = 0;
		iismod &= ~IISMOD_MASTER_MASK;
		iismod |= IISMOD_SLAVE;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		i2s->master = 1;
		iismod &= ~IISMOD_MASTER_MASK;
		iismod |= IISMOD_MASTER;
		break;
	default:
		pr_err("unknwon master/slave format\n");
		return -EINVAL;
	}

	iismod &= ~S3C2412_IISMOD_SDF_MASK;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_RIGHT_J:
		iismod |= S3C2412_IISMOD_LR_RLOW;
		iismod |= S3C2412_IISMOD_SDF_MSB;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iismod |= S3C2412_IISMOD_LR_RLOW;
		iismod |= S3C2412_IISMOD_SDF_LSB;
		break;
	case SND_SOC_DAIFMT_I2S:
		iismod &= ~S3C2412_IISMOD_LR_RLOW;
		iismod |= S3C2412_IISMOD_SDF_IIS;
		break;
	default:
		pr_err("Unknown data format\n");
		return -EINVAL;
	}

	writel(iismod, i2s->regs + S3C2412_IISMOD);
	pr_debug("hw_params w: IISMOD: %x\n", iismod);
	return 0;
}

static int s5p_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
		int div_id, int div)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 reg;

	pr_debug("%s(%p, %d, %d)\n", __func__, cpu_dai, div_id, div);

	switch (div_id) {
	case S3C_I2SV2_DIV_BCLK:
		if (div > 3) {
			/* convert value to bit field */
			switch (div) {
			case 16:
				div = S3C2412_IISMOD_BCLK_16FS;
				break;
			case 32:
				div = S3C2412_IISMOD_BCLK_32FS;
				break;
			case 24:
				div = S3C2412_IISMOD_BCLK_24FS;
				break;
			case 48:
				div = S3C2412_IISMOD_BCLK_48FS;
				break;
			default:
				return -EINVAL;
			}
		}

		reg = readl(i2s->regs + S3C2412_IISMOD);
		reg &= ~S3C2412_IISMOD_BCLK_MASK;
		writel(reg | div, i2s->regs + S3C2412_IISMOD);
		pr_debug("%s: MOD=%08x\n", __func__,
				readl(i2s->regs + S3C2412_IISMOD));
		break;

	case S3C_I2SV2_DIV_RCLK:
		if (div > 3) {
			/* convert value to bit field */

			switch (div) {
			case 256:
				div = S3C2412_IISMOD_RCLK_256FS;
				break;
			case 384:
				div = S3C2412_IISMOD_RCLK_384FS;
				break;
			case 512:
				div = S3C2412_IISMOD_RCLK_512FS;
				break;
			case 768:
				div = S3C2412_IISMOD_RCLK_768FS;
				break;
			default:
				return -EINVAL;
			}
		}

		reg = readl(i2s->regs + S3C2412_IISMOD);
		reg &= ~S3C2412_IISMOD_RCLK_MASK;
		writel(reg | div, i2s->regs + S3C2412_IISMOD);
		pr_debug("%s: MOD=%08x\n", __func__,
				readl(i2s->regs + S3C2412_IISMOD));
		break;

	case S3C_I2SV2_DIV_PRESCALER:
		if (div >= 0)
			writel((div << 8) | S3C2412_IISPSR_PSREN,
					i2s->regs + S3C2412_IISPSR);
		else
			writel(0x0, i2s->regs + S3C2412_IISPSR);
			pr_debug("%s: PSR=%08x\n", __func__,
				readl(i2s->regs + S3C2412_IISPSR));
			break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int s5p_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct clk *clk;
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 iismod = readl(i2s->regs + S3C2412_IISMOD);

	switch (clk_id) {
	case S3C64XX_CLKSRC_PCLK:
		iismod &= ~S3C64XX_IISMOD_IMS_SYSMUX;
		break;
	case S3C64XX_CLKSRC_MUX:
		iismod |= S3C64XX_IISMOD_IMS_SYSMUX;
		break;

	case S3C64XX_CLKSRC_CDCLK:
		switch (dir) {
		case SND_SOC_CLOCK_IN:
			iismod |= S3C64XX_IISMOD_CDCLKCON;
			break;
		case SND_SOC_CLOCK_OUT:
			iismod &= ~S3C64XX_IISMOD_CDCLKCON;
			break;
		default:
			return -EINVAL;
		}
		break;
#ifdef USE_CLKAUDIO
		/* IIS-IP is Master and derives its clocks from I2SCLKD2 */
	case S3C_CLKSRC_CLKAUDIO:
		if (!i2s->master)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		clk = clk_get(NULL, "fout_epll");
		if (IS_ERR(clk)) {
			printk(KERN_ERR
					"failed to get %s\n", "fout_epll");
			return -EBUSY;
		}
		clk_disable(clk);
		switch (freq) {
		case 8000:
		case 16000:
		case 32000:
		case 48000:
		case 64000:
		case 96000:
			clk_set_rate(clk, 49152000);
			break;
		case 11025:
		case 22050:
		case 44100:
		case 88200:
		default:
			clk_set_rate(clk, 67738000);
			break;
		}
		clk_enable(clk);
		clk_put(clk);
		break;
#endif
		/* IIS-IP is Slave and derives its clocks from the Codec Chip */
	case S3C64XX_CLKSRC_I2SEXT:
		iismod &= ~S3C64XX_IISMOD_IMSMASK;
		iismod |= clk_id;
		/* Operation clock for I2S logic selected as Audio Bus Clock */
		iismod |= S3C64XX_IISMOD_OPPCLK;

		clk = clk_get(NULL, "fout_epll");
		if (IS_ERR(clk)) {
			printk(KERN_ERR
				"failed to get %s\n", "fout_epll");
				return -EBUSY;
		}
		clk_disable(clk);
		clk_set_rate(clk, 67738000);
		clk_enable(clk);
		clk_put(clk);
		break;

	case S3C64XX_CDCLKSRC_EXT:
		iismod |= S3C64XX_IISMOD_CDCLKCON;
		break;

	default:
		return -EINVAL;
	}

	writel(iismod, i2s->regs + S3C2412_IISMOD);

	return 0;
}

static int s5p_i2s_wr_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	u32 iiscon, iisfic;

	if (!tx_clk_enabled && !rx_clk_enabled) {
		s5p_i2s_set_clk_enabled(dai, 1);
		if (reg_saved_ok == true) {
			/* Is this dai for I2Sv5? (I2S0) */
			if (dai->id == 0) {
				writel(i2s->suspend_audss_clksrc,
						S5P_CLKSRC_AUDSS);
				writel(i2s->suspend_audss_clkdiv,
						S5P_CLKDIV_AUDSS);
				writel(i2s->suspend_audss_clkgate,
						S5P_CLKGATE_AUDSS);
			}
			writel(i2s->suspend_iismod, i2s->regs + S3C2412_IISMOD);
			writel(i2s->suspend_iiscon, i2s->regs + S3C2412_IISCON);
			writel(i2s->suspend_iispsr, i2s->regs + S3C2412_IISPSR);
			writel(i2s->suspend_iisahb, i2s->regs + S5P_IISAHB);
			reg_saved_ok = false;
			pr_debug("I2S Audio Clock enabled and \
					Registers restored...\n");
		}
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("Inside..%s..for playback stream\n" , __func__);
		tx_clk_enabled = 1;
	} else {
		pr_debug("Inside..%s..for capture stream\n" , __func__);
		rx_clk_enabled = 1;
		iiscon = readl(i2s->regs + S3C2412_IISCON);
		if (iiscon & S3C2412_IISCON_RXDMA_ACTIVE)
			return 0;

		iisfic = readl(i2s->regs + S3C2412_IISFIC);
		iisfic |= S3C2412_IISFIC_RXFLUSH;
		writel(iisfic, i2s->regs + S3C2412_IISFIC);

		do {
			cpu_relax();
		} while ((__raw_readl(i2s->regs + S3C2412_IISFIC) >> 0) & 0x7f);

		iisfic = readl(i2s->regs + S3C2412_IISFIC);
		iisfic &= ~S3C2412_IISFIC_RXFLUSH;
		writel(iisfic, i2s->regs + S3C2412_IISFIC);
	}

#ifdef CONFIG_S5P_INTERNAL_DMA
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		s5p_i2s_startup(dai);
#endif
	dump_reg(i2s);
	return 0;
}

static void s5p_i2s_wr_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("Inside %s for playback stream\n" , __func__);
		tx_clk_enabled = 0;
	} else {
		pr_debug("Inside..%s..for capture stream\n" , __func__);
		if (readl(i2s->regs + S3C2412_IISCON) & (1<<26)) {
			pr_debug("\n rx overflow int in %s\n" , __func__);
			/* clear rxfifo overflow interrupt */
			writel(readl(i2s->regs + S3C2412_IISCON) | (1<<26),
					i2s->regs + S3C2412_IISCON);
			/* flush rx */
			writel(readl(i2s->regs + S3C2412_IISFIC) | (1<<7) ,
					i2s->regs + S3C2412_IISFIC);
		}
		rx_clk_enabled = 0;
	}

	if (!tx_clk_enabled && !rx_clk_enabled) {
		i2s->suspend_iismod = readl(i2s->regs + S3C2412_IISMOD);
		i2s->suspend_iiscon = readl(i2s->regs + S3C2412_IISCON);
		i2s->suspend_iispsr = readl(i2s->regs + S3C2412_IISPSR);
		i2s->suspend_iisahb = readl(i2s->regs + S5P_IISAHB);
		/* Is this dai for I2Sv5? (I2S0) */
		if (dai->id == 0) {
			i2s->suspend_audss_clksrc = readl(S5P_CLKSRC_AUDSS);
			i2s->suspend_audss_clkdiv = readl(S5P_CLKDIV_AUDSS);
			i2s->suspend_audss_clkgate = readl(S5P_CLKGATE_AUDSS);
		}
		reg_saved_ok = true;
		s5p_i2s_set_clk_enabled(dai, 0);
		pr_debug("I2S Audio Clock disabled and Registers stored...\n");
		pr_debug("Inside %s CLkGATE_IP3=0x%x..\n",
				__func__ , __raw_readl(S5P_CLKGATE_IP3));
	}

	return;
}

#define S3C64XX_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	 SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	 SNDRV_PCM_RATE_KNOT)

#define S3C64XX_I2S_FMTS \
	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE |\
	 SNDRV_PCM_FMTBIT_S24_LE)

static void s3c64xx_iis_dai_init(struct snd_soc_dai *dai)
{
	dai->name = "s5pc1xx-i2s";
	dai->playback.channels_min = 2;
	dai->playback.channels_max = 2;
	dai->playback.rates = S3C64XX_I2S_RATES;
	dai->playback.formats = S3C64XX_I2S_FMTS;
	dai->capture.channels_min = 1;
	dai->capture.channels_max = 2;
	dai->capture.rates = S3C64XX_I2S_RATES;
	dai->capture.formats = S3C64XX_I2S_FMTS;
	dai->ops = &s3c64xx_i2s_dai_ops;
}

/* suspend/resume are not necessary due to Clock/Pwer gating scheme... */
#ifdef CONFIG_PM
static int s5p_i2s_suspend(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	if (reg_saved_ok != true) {
		dump_reg(i2s);
		i2s->suspend_iismod = readl(i2s->regs + S3C2412_IISMOD);
		i2s->suspend_iiscon = readl(i2s->regs + S3C2412_IISCON);
		i2s->suspend_iispsr = readl(i2s->regs + S3C2412_IISPSR);
		i2s->suspend_iisahb = readl(i2s->regs + S5P_IISAHB);
		if (dai->id == 0) {
			i2s->suspend_audss_clksrc = readl(S5P_CLKSRC_AUDSS);
			i2s->suspend_audss_clkdiv = readl(S5P_CLKDIV_AUDSS);
			i2s->suspend_audss_clkgate = readl(S5P_CLKGATE_AUDSS);
		}
	}
	return 0;
}

static int s5p_i2s_resume(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	pr_info("dai_active %d, IISMOD %08x, IISCON %08x\n",
			dai->active, i2s->suspend_iismod, i2s->suspend_iiscon);
	if (reg_saved_ok != true) {
		if (dai->id == 0) {
			writel(i2s->suspend_audss_clksrc, S5P_CLKSRC_AUDSS);
			writel(i2s->suspend_audss_clkdiv, S5P_CLKDIV_AUDSS);
			writel(i2s->suspend_audss_clkgate, S5P_CLKGATE_AUDSS);
			pr_info("Inside %s..@%d\n" , __func__ , __LINE__);
		}
		writel(i2s->suspend_iiscon, i2s->regs + S3C2412_IISCON);
		writel(i2s->suspend_iismod, i2s->regs + S3C2412_IISMOD);
		writel(i2s->suspend_iispsr, i2s->regs + S3C2412_IISPSR);
		writel(i2s->suspend_iisahb, i2s->regs + S5P_IISAHB);
	}
	return 0;
	/* Is this dai for I2Sv5? */
	if (dai->id == 0)
		writel(i2s->suspend_audss_clksrc, S5P_CLKSRC_AUDSS);

	writel(S3C2412_IISFIC_RXFLUSH | S3C2412_IISFIC_TXFLUSH,
			i2s->regs + S3C2412_IISFIC);

	ndelay(250);
	writel(0x0, i2s->regs + S3C2412_IISFIC);

	return 0;
}
#else
#define s3c2412_i2s_suspend NULL
#define s3c2412_i2s_resume  NULL
#endif	/* CONFIG_PM */

int s5p_i2sv5_register_dai(struct snd_soc_dai *dai)
{
	struct snd_soc_dai_ops *ops = dai->ops;

	ops->trigger = s5p_i2s_wr_trigger;
	ops->hw_params = s5p_i2s_wr_hw_params;
	ops->set_fmt = s5p_i2s_set_fmt;
	ops->set_clkdiv = s5p_i2s_set_clkdiv;
	ops->set_sysclk = s5p_i2s_set_sysclk;
	ops->startup   = s5p_i2s_wr_startup;
	ops->shutdown = s5p_i2s_wr_shutdown;
	/* suspend/resume are not necessary due to Clock/Pwer gating scheme */
	dai->suspend = s5p_i2s_suspend;
	dai->resume = s5p_i2s_resume;
	return snd_soc_register_dai(dai);
}

static __devinit int s3c64xx_iis_dev_probe(struct platform_device *pdev)
{
	struct s3c_audio_pdata *i2s_pdata;
	struct s3c_i2sv2_info *i2s;
	struct snd_soc_dai *dai;
	struct resource *res;
	struct clk *fout_epll, *mout_epll;
	struct clk *mout_audss = NULL;
	unsigned long base;
	unsigned int  iismod;
	int ret = 0;
	if (pdev->id >= MAX_I2SV3) {
		dev_err(&pdev->dev, "id %d out of range\n", pdev->id);
		return -EINVAL;
	}

	i2s = &s3c64xx_i2s[pdev->id];
	i2s->dev = &pdev->dev;
	dai = &s3c64xx_i2s_dai[pdev->id];
	dai->dev = &pdev->dev;
	dai->id = pdev->id;
	s3c64xx_iis_dai_init(dai);

	i2s->dma_capture = &s3c64xx_i2s_pcm_stereo_in[pdev->id];
	i2s->dma_playback = &s3c64xx_i2s_pcm_stereo_out[pdev->id];

	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-TX dma resource\n");
		return -ENXIO;
	}
	i2s->dma_playback->channel = res->start;

	res = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-RX dma resource\n");
		return -ENXIO;
	}
	i2s->dma_capture->channel = res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S SFR address\n");
		return -ENXIO;
	}

	if (!request_mem_region(res->start, resource_size(res),
				"s3c64xx-i2s")) {
		dev_err(&pdev->dev, "Unable to request SFR region\n");
		return -EBUSY;
	}

	i2s->dma_capture->dma_addr = res->start + S3C2412_IISRXD;
	i2s->dma_playback->dma_addr = res->start + S3C2412_IISTXD;

	i2s->dma_capture->client = &s3c64xx_dma_client_in;
	i2s->dma_capture->dma_size = 4;
	i2s->dma_playback->client = &s3c64xx_dma_client_out;
	i2s->dma_playback->dma_size = 4;

	i2s_pdata = pdev->dev.platform_data;

	dai->private_data = i2s;
	base = i2s->dma_playback->dma_addr - S3C2412_IISTXD;

	i2s->regs = ioremap(base, 0x100);
	if (i2s->regs == NULL) {
		dev_err(&pdev->dev, "cannot ioremap registers\n");
		return -ENXIO;
	}

	/* Configure the I2S pins if MUX'ed */
	if (i2s_pdata && i2s_pdata->cfg_gpio && i2s_pdata->cfg_gpio(pdev)) {
		dev_err(&pdev->dev, "Unable to configure gpio\n");
		return -EINVAL;
	}

	/* Get i2s power domain regulator */
	i2s->regulator = regulator_get(&pdev->dev, "pd");
	if (IS_ERR(i2s->regulator)) {
		dev_err(&pdev->dev, "%s: failed to get resource %s\n",
				__func__, "i2s");
		return PTR_ERR(i2s->regulator);
	}

	/* Enable Power domain */
	regulator_enable(i2s->regulator);

	/* Audio Clock
	 * fout_epll >> mout_epll >> sclk_audio
	 * fout_epll >> mout_audss >> audio-bus(iis_clk)
	 * fout_epll >> dout_audio_bus_clk_i2s(iis_busclk)
	 */
	fout_epll = clk_get(&pdev->dev, "fout_epll");
	if (IS_ERR(fout_epll)) {
		dev_err(&pdev->dev, "failed to get fout_epll\n");
		goto err;
	}

	mout_epll = clk_get(&pdev->dev, "mout_epll");
	if (IS_ERR(mout_epll)) {
		dev_err(&pdev->dev, "failed to get mout_epll\n");
		clk_put(fout_epll);
		goto err;
	}
	clk_set_parent(mout_epll, fout_epll);

	i2s->sclk_audio = clk_get(&pdev->dev, "sclk_audio");
	if (IS_ERR(i2s->sclk_audio)) {
		dev_err(&pdev->dev, "failed to get sclk_audio\n");
		ret = PTR_ERR(i2s->sclk_audio);
		clk_put(i2s->sclk_audio);
		goto err;
	}
	clk_set_parent(i2s->sclk_audio, mout_epll);
	/* Need not to enable in general */
	clk_enable(i2s->sclk_audio);

	/* When I2S V5.1 used, initialize audio subsystem clock */
	/* CLKMUX_ASS */
	if (pdev->id == 0) {
		mout_audss = clk_get(NULL, "mout_audss");
		if (IS_ERR(mout_audss)) {
			dev_err(&pdev->dev, "failed to get mout_audss\n");
			goto err1;
		}
		clk_set_parent(mout_audss, fout_epll);
		/*MUX-I2SA*/
		i2s->iis_clk = clk_get(&pdev->dev, "audio-bus");
		if (IS_ERR(i2s->iis_clk)) {
			dev_err(&pdev->dev, "failed to get audio-bus\n");
			clk_put(mout_audss);
			goto err2;
		}
		clk_set_parent(i2s->iis_clk, mout_audss);
		/*getting AUDIO BUS CLK*/
		i2s->iis_busclk = clk_get(NULL, "dout_audio_bus_clk_i2s");
		if (IS_ERR(i2s->iis_busclk)) {
			printk(KERN_ERR "failed to get audss_hclk\n");
			goto err3;
		}
		i2s->iis_ipclk = clk_get(&pdev->dev, "i2s_v50");
		if (IS_ERR(i2s->iis_ipclk)) {
			dev_err(&pdev->dev, "failed to get i2s_v50_clock\n");
			goto err4;
		}
	}

#if defined(CONFIG_PLAT_S5P)
	writel(((1<<0)|(1<<31)), i2s->regs + S3C2412_IISCON);
#endif

	/* Mark ourselves as in TXRX mode so we can run through our cleanup
	 * process without warnings. */
	iismod = readl(i2s->regs + S3C2412_IISMOD);
	iismod |= S3C2412_IISMOD_MODE_TXRX;
	writel(iismod, i2s->regs + S3C2412_IISMOD);

#ifdef CONFIG_S5P_INTERNAL_DMA
	s5p_i2s_sec_init(i2s->regs, base);
#endif

	ret = s5p_i2sv5_register_dai(dai);
	if (ret != 0)
		goto err_i2sv5;

	clk_put(i2s->iis_ipclk);
	clk_put(i2s->iis_busclk);
	clk_put(i2s->iis_clk);
	clk_put(mout_audss);
	clk_put(mout_epll);
	clk_put(fout_epll);
	return 0;
err4:
	clk_put(i2s->iis_busclk);
err3:
	clk_put(i2s->iis_clk);
err2:
	clk_put(mout_audss);
err1:
	clk_put(mout_epll);
	clk_put(fout_epll);
err_i2sv5:
	/* Not implemented for I2Sv5 core yet */
err:
	iounmap(i2s->regs);

	return ret;
}

static __devexit int s3c64xx_iis_dev_remove(struct platform_device *pdev)
{
	dev_err(&pdev->dev, "Device removal not yet supported\n");
	return 0;
}

static struct platform_driver s3c64xx_iis_driver = {
	.probe  = s3c64xx_iis_dev_probe,
	.remove = s3c64xx_iis_dev_remove,
	.driver = {
		.name = "s5pc1xx-iis",
		.owner = THIS_MODULE,
	},
};

static int __init s3c64xx_i2s_init(void)
{
	return platform_driver_register(&s3c64xx_iis_driver);
}
module_init(s3c64xx_i2s_init);

static void __exit s3c64xx_i2s_exit(void)
{
	platform_driver_unregister(&s3c64xx_iis_driver);
}
module_exit(s3c64xx_i2s_exit);

/* Module information */
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("S5PC1XX I2S SoC Interface");
MODULE_LICENSE("GPL");
