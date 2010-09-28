/*
 * crespo_wm8994.c
 *
 * Copyright (C) 2010, Samsung Elect. Ltd. -
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <mach/regs-clock.h>
#include <plat/regs-iis.h>
#include "../codecs/wm8994.h"
#include "s3c-dma.h"
#include "s5pc1xx-i2s.h"
#include "s3c-i2s-v2.h"

#include <linux/io.h>

#define I2S_NUM 0
#define SRC_CLK 66738000

/* #define CONFIG_SND_DEBUG */
#ifdef CONFIG_SND_DEBUG
#define debug_msg(x...) printk(x)
#else
#define debug_msg(x...)
#endif

/*  BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2)*/
/*  BFS --> RFS(must be a multiple of BFS)                                  */
/*  RFS & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)           */
int smdkc110_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int bfs, rfs, ret;
	u32 ap_codec_clk;
#ifndef CONFIG_SND_S5P_WM8994_MASTER
	struct clk    *clk_out, *clk_epll;
	int psr;
#endif
	debug_msg("%s\n", __func__);

	/* Choose BFS and RFS values combination that is supported by
	 * both the WM8994 codec as well as the S5P AP
	 *
	 */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
	/* Can take any RFS value for AP */
			bfs = 16;
			rfs = 256;
			break;
	case SNDRV_PCM_FORMAT_S16_LE:
	/* Can take any RFS value for AP */
			bfs = 32;
			rfs = 256;
			break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
			bfs = 48;
			rfs = 512;
			break;
	/* Impossible, as the AP doesn't support 64fs or more BFS */
	case SNDRV_PCM_FORMAT_S32_LE:
	default:
			return -EINVAL;
	}

#ifdef CONFIG_SND_S5P_WM8994_MASTER
	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);

	if (ret < 0) {
		printk(KERN_ERR "smdkc110_wm8994_hw_params :\
				 Codec DAI configuration error!\n");
		return ret;
	}

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);

	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params :\
				AP DAI configuration error!\n");
		return ret;
	}

	/* Select the AP Sysclk */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CDCLKSRC_EXT,
					params_rate(params), SND_SOC_CLOCK_IN);

	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params :\
			AP sys clock INT setting error!\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_I2SEXT,
					params_rate(params), SND_SOC_CLOCK_IN);
	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params :\
			AP sys clock I2SEXT setting error!\n");
		return ret;
	}

	switch (params_rate(params)) {

	case 8000:
		ap_codec_clk = 4096000;
		break;
	case 11025:
		ap_codec_clk = 2822400;
		break;
	case 12000:
		ap_codec_clk = 6144000;
		break;
	case 16000:
		ap_codec_clk = 4096000;
		break;
	case 22050:
		ap_codec_clk = 6144000;
		break;
	case 24000:
		ap_codec_clk = 6144000;
		break;
	case 32000:
		ap_codec_clk = 8192000;
		break;
	case 44100:
		ap_codec_clk = 11289600;
		break;
	case 48000:
		ap_codec_clk = 12288000;
		break;
	default:
		ap_codec_clk = 11289600;
		break;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL,
					ap_codec_clk, 0);
	if (ret < 0)
		return ret;
#else
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);

	if (ret < 0)
		return ret;
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);

	if (ret < 0)

		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_CDCLK,
					params_rate(params), SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;
#ifdef USE_CLKAUDIO
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_CLKAUDIO,
					params_rate(params), SND_SOC_CLOCK_OUT);

	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params : \
			AP sys clock setting error!\n");
		return ret;
	}
#endif
	clk_out = clk_get(NULL, "clk_out");
	if (IS_ERR(clk_out)) {
			printk(KERN_ERR
				"failed to get CLK_OUT\n");
			return -EBUSY;
	}

	clk_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(clk_epll)) {
		printk(KERN_ERR
			"failed to get fout_epll\n");
		clk_put(clk_out);
		return -EBUSY;
	}

	if (clk_set_parent(clk_out, clk_epll)) {
		printk(KERN_ERR
			"failed to set CLK_EPLL as parent of CLK_OUT\n");
		clk_put(clk_out);
		clk_put(clk_epll);
		return -EBUSY;
	}


	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		clk_set_rate(clk_out, 12288000);
		ap_codec_clk = SRC_CLK/4;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
	default:
		clk_set_rate(clk_out, 11289600);
		ap_codec_clk = SRC_CLK/6;
		break;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_MCLK,
							ap_codec_clk, 0);
	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params : \
				Codec sys clock setting error!\n");
		return ret;
	}

	/* Calculate Prescalare/PLL values for supported Rates */
	psr = SRC_CLK / rfs / params_rate(params);
	ret = SRC_CLK / rfs - psr * params_rate(params);
	/* round off */
	if (ret >= params_rate(params)/2)
		psr += 1;

	psr -= 1;
	printk(KERN_INFO
		"SRC_CLK=%d PSR=%d RFS=%d BFS=%d\n", SRC_CLK, psr, rfs, bfs);

	/* Set the AP Prescalar/Pll */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_PRESCALER, psr);

	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params :\
				AP prescalar setting error!\n");
		return ret;
	}

	/* Set the AP RFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_RCLK, rfs);
	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params : AP RFS setting error!\n");
		return ret;
	}

	/* Set the AP BFS */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_BCLK, bfs);

	if (ret < 0) {
		printk(KERN_ERR
			"smdkc110_wm8994_hw_params : AP BCLK setting error!\n");
		return ret;
	}

	clk_put(clk_epll);
	clk_put(clk_out);
#endif
	return 0;

}

/* machine stream operations */
static struct snd_soc_ops smdkc110_ops = {
	.hw_params = smdkc110_hw_params,
};

/* digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link smdkc1xx_dai = {
	.name = "WM8994",
	.stream_name = "WM8994 HiFi Playback",
	.cpu_dai = &s3c64xx_i2s_dai[I2S_NUM],
	.codec_dai = &wm8994_dai,
	.ops = &smdkc110_ops,
};

static struct snd_soc_card smdkc100 = {
	.name = "smdkc110",
	.platform = &s3c_dma_wrapper,
	.dai_link = &smdkc1xx_dai,
	.num_links = 1,
};

static struct wm8994_setup_data smdkc110_wm8994_setup = {
	/*
		The I2C address of the WM89940 is 0x34. To the I2C driver
		the address is a 7-bit number hence the right shift .
	*/
	.i2c_address = 0x34,
	.i2c_bus = 4,
};

/* audio subsystem */
static struct snd_soc_device smdkc1xx_snd_devdata = {
	.card = &smdkc100,
	.codec_dev = &soc_codec_dev_wm8994,
	.codec_data = &smdkc110_wm8994_setup,
};

static struct platform_device *smdkc1xx_snd_device;
static int __init smdkc110_audio_init(void)
{
	int ret;

	debug_msg("%s\n", __func__);

	smdkc1xx_snd_device = platform_device_alloc("soc-audio", 0);
	if (!smdkc1xx_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdkc1xx_snd_device, &smdkc1xx_snd_devdata);
	smdkc1xx_snd_devdata.dev = &smdkc1xx_snd_device->dev;
	ret = platform_device_add(smdkc1xx_snd_device);

	if (ret)
		platform_device_put(smdkc1xx_snd_device);

	return ret;
}

static void __exit smdkc110_audio_exit(void)
{
	debug_msg("%s\n", __func__);

	platform_device_unregister(smdkc1xx_snd_device);
}

module_init(smdkc110_audio_init);
module_exit(smdkc110_audio_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC SMDKC110 WM8994");
MODULE_LICENSE("GPL");
