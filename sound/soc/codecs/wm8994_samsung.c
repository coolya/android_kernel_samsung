/*
 * wm8994_samsung.c  --  WM8994 ALSA Soc Audio driver
 *
 * Copyright 2010 Wolfson Microelectronics PLC.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 * Notes:
 *  The WM8994 is a multichannel codec with S/PDIF support, featuring six
 *  DAC channels and two ADC channels.
 *
 *  Currently only the primary audio interface is supported - S/PDIF and
 *  the secondary audio interfaces are not.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <asm/div64.h>
#include <linux/io.h>
#include <plat/map-base.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-clock.h>
#include "wm8994_samsung.h"

#define WM8994_VERSION "0.1"
#define SUBJECT "wm8994_samsung.c"

#if defined(CONFIG_VIDEO_TV20) && defined(CONFIG_SND_S5P_WM8994_MASTER)
#define HDMI_USE_AUDIO
#endif

/*
 *Definitions of clock related.
*/

static struct {
	int ratio;
	int clk_sys_rate;
} clk_sys_rates[] = {
	{ 64,   0 },
	{ 128,  1 },
	{ 192,  2 },
	{ 256,  3 },
	{ 384,  4 },
	{ 512,  5 },
	{ 768,  6 },
	{ 1024, 7 },
	{ 1408, 8 },
	{ 1536, 9 },
};

static struct {
	int rate;
	int sample_rate;
} sample_rates[] = {
	{ 8000,  0  },
	{ 11025, 1  },
	{ 12000, 2  },
	{ 16000, 3  },
	{ 22050, 4  },
	{ 24000, 5  },
	{ 32000, 6  },
	{ 44100, 7  },
	{ 48000, 8  },
	{ 88200, 9  },
	{ 96000, 10  },
};

static struct {
	int div;
	int bclk_div;
} bclk_divs[] = {
	{ 1,   0  },
	{ 2,   1  },
	{ 4,   2  },
	{ 6,   3  },
	{ 8,   4  },
	{ 12,  5  },
	{ 16,  6  },
	{ 24,  7  },
	{ 32,  8  },
	{ 48,  9  },
};

struct snd_soc_dai wm8994_dai;
EXPORT_SYMBOL_GPL(wm8994_dai);

struct snd_soc_codec_device soc_codec_dev_pcm_wm8994;
EXPORT_SYMBOL_GPL(soc_codec_dev_pcm_wm8994);

struct snd_soc_codec_device soc_codec_dev_wm8994;
EXPORT_SYMBOL_GPL(soc_codec_dev_wm8994);

/*
 * Definitions of sound path
 */
select_route universal_wm8994_playback_paths[] = {
	wm8994_disable_path, wm8994_set_playback_receiver,
	wm8994_set_playback_speaker, wm8994_set_playback_headset,
	wm8994_set_playback_headset, wm8994_set_playback_bluetooth,
	wm8994_set_playback_speaker_headset
};

select_route universal_wm8994_voicecall_paths[] = {
	wm8994_disable_path, wm8994_set_voicecall_receiver,
	wm8994_set_voicecall_speaker, wm8994_set_voicecall_headset,
	wm8994_set_voicecall_headphone, wm8994_set_voicecall_bluetooth
};

select_mic_route universal_wm8994_mic_paths[] = {
	wm8994_record_main_mic,
	wm8994_record_headset_mic,
	wm8994_record_bluetooth,
};

select_clock_control universal_clock_controls = wm8994_configure_clock;

int gain_code;

/*
 * Implementation of I2C functions
 */
static unsigned int wm8994_read_hw(struct snd_soc_codec *codec, u16 reg)
{
	struct i2c_msg xfer[2];
	u16 data;
	int ret;
	struct i2c_client *i2c = codec->control_data;

	data = ((reg & 0xff00) >> 8) | ((reg & 0xff) << 8);

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = (void *)&data;

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = 2;
	xfer[1].buf = (u8 *)&data;
	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret != 2) {
		dev_err(codec->dev, "Failed to read 0x%x: %d\n", reg, ret);
		return 0;
	}

	return (data >> 8) | ((data & 0xff) << 8);
}

int wm8994_write(struct snd_soc_codec *codec, unsigned int reg,
		 unsigned int value)
{
	u8 data[4];
	int ret;

	/* data is
	 * D15..D9 WM8993 register offset
	 * D8...D0 register data
	 */
	data[0] = (reg & 0xff00) >> 8;
	data[1] = reg & 0x00ff;
	data[2] = value >> 8;
	data[3] = value & 0x00ff;
	ret = codec->hw_write(codec->control_data, data, 4);

	if (ret == 4)
		return 0;
	else {
		pr_err("i2c write problem occured\n");
		return ret;
	}
}

unsigned int wm8994_read(struct snd_soc_codec *codec, unsigned int reg)
{
	return wm8994_read_hw(codec, reg);
}

static int wm8994_ldo_control(struct wm8994_platform_data *pdata, int en)
{

	if (!pdata) {
		pr_err("failed to control wm8994 ldo\n");
		return -EINVAL;
	}

	gpio_set_value(pdata->ldo, en);

	if (en)
		msleep(10);
	else
		msleep(125);

	return 0;

}

/*
 * Functions related volume.
 */
static const DECLARE_TLV_DB_SCALE(dac_tlv, -12750, 50, 1);

static int wm899x_outpga_put_volsw_vu(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int ret;
	u16 val;

	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
	    (struct soc_mixer_control *)kcontrol->private_value;
	int reg = mc->reg;
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("");

	ret = snd_soc_put_volsw_2r(kcontrol, ucontrol);
	if (ret < 0)
		return ret;

	/* Volume changes in the headphone path mean we need to
	 * recallibrate DC servo */
	if (strcmp(kcontrol->id.name, "Playback Spkr Volume") == 0 ||
	    strcmp(kcontrol->id.name, "Playback Volume") == 0)
		memset(wm8994->dc_servo, 0, sizeof(wm8994->dc_servo));

	val = wm8994_read(codec, reg);

	return wm8994_write(codec, reg, val | 0x0100);
}

static int wm899x_inpga_put_volsw_vu(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
	    (struct soc_mixer_control *)kcontrol->private_value;
	int reg = mc->reg;
	int ret;
	u16 val;

	ret = snd_soc_put_volsw(kcontrol, ucontrol);

	if (ret < 0)
		return ret;

	val = wm8994_read(codec, reg);

	return wm8994_write(codec, reg, val | 0x0100);

}

/*
 * Implementation of sound path
 */
#define MAX_PLAYBACK_PATHS 10
#define MAX_VOICECALL_PATH 5
static const char *playback_path[] = {
	"OFF", "RCV", "SPK", "HP", "HP_NO_MIC", "BT", "SPK_HP",
	"RING_SPK", "RING_HP", "RING_NO_MIC", "RING_SPK_HP"
};
static const char *voicecall_path[] = { "OFF", "RCV", "SPK", "HP",
					"HP_NO_MIC", "BT" };
static const char *mic_path[] = { "Main Mic", "Hands Free Mic",
					"BT Sco Mic", "MIC OFF" };
static const char *input_source_state[] = { "Default", "Voice Recognition",
					"Camcorder" };

static int wm8994_get_mic_path(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	ucontrol->value.integer.value[0] = wm8994->rec_path;

	return 0;
}

static int wm8994_set_mic_path(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("");

	wm8994->codec_state |= CAPTURE_ACTIVE;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		wm8994->rec_path = MAIN;
		break;
	case 1:
		wm8994->rec_path = SUB;
		break;
	case 2:
		wm8994->rec_path = BT_REC;
		break;
	case 3:
		wm8994_disable_rec_path(codec);
		return 0;
	default:
		return -EINVAL;
	}

	wm8994->universal_mic_path[wm8994->rec_path] (codec);

	return 0;
}

static int wm8994_get_path(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	ucontrol->value.integer.value[0] = wm8994->cur_path;

	return 0;
}

static int wm8994_set_path(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;
	struct soc_enum *mc = (struct soc_enum *)kcontrol->private_value;
	int val;
	int path_num = ucontrol->value.integer.value[0];

	if (strcmp(mc->texts[path_num], playback_path[path_num])) {
		DEBUG_LOG_ERR("Unknown path %s\n", mc->texts[path_num]);
		return -ENODEV;
	}

	if (path_num > MAX_PLAYBACK_PATHS) {
		DEBUG_LOG_ERR("Unknown Path\n");
		return -ENODEV;
	}

	switch (path_num) {
	case OFF:
		DEBUG_LOG("Switching off output path\n");
		break;
	case RCV:
	case SPK:
	case HP:
	case HP_NO_MIC:
	case BT:
	case SPK_HP:
		DEBUG_LOG("routing to %s\n", mc->texts[path_num]);
		wm8994->ringtone_active = RING_OFF;
		break;
	case RING_SPK:
	case RING_HP:
	case RING_NO_MIC:
		DEBUG_LOG("routing to %s\n", mc->texts[path_num]);
		wm8994->ringtone_active = RING_ON;
		path_num -= 5;
		break;
	case RING_SPK_HP:
		DEBUG_LOG("routing to %s\n", mc->texts[path_num]);
		wm8994->ringtone_active = RING_ON;
		path_num -= 4;
		break;
	default:
		DEBUG_LOG_ERR("audio path[%d] does not exists!!\n", path_num);
		return -ENODEV;
		break;
	}

	wm8994->codec_state |= PLAYBACK_ACTIVE;

	if (wm8994->codec_state & CALL_ACTIVE) {
		wm8994->codec_state &= ~(CALL_ACTIVE);

		val = wm8994_read(codec, WM8994_CLOCKING_1);
		val &= ~(WM8994_DSP_FS2CLK_ENA_MASK | WM8994_SYSCLK_SRC_MASK);
		wm8994_write(codec, WM8994_CLOCKING_1, val);
	}

	wm8994->cur_path = path_num;
	wm8994->universal_playback_path[wm8994->cur_path] (codec);

	return 0;
}

static int wm8994_get_voice_path(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	ucontrol->value.integer.value[0] = wm8994->cur_path;

	return 0;
}

static int wm8994_set_voice_path(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;
	struct soc_enum *mc = (struct soc_enum *)kcontrol->private_value;

	int path_num = ucontrol->value.integer.value[0];

	if (strcmp(mc->texts[path_num], voicecall_path[path_num])) {
		DEBUG_LOG_ERR("Unknown path %s\n", mc->texts[path_num]);
		return -ENODEV;
	}

	switch (path_num) {
	case OFF:
		DEBUG_LOG("Switching off output path\n");
		break;
	case RCV:
	case SPK:
	case HP:
	case HP_NO_MIC:
	case BT:
		DEBUG_LOG("routing  voice path to %s\n", mc->texts[path_num]);
		break;
	default:
		DEBUG_LOG_ERR("path[%d] does not exists!\n", path_num);
		return -ENODEV;
		break;
	}

	if (wm8994->cur_path != path_num ||
			!(wm8994->codec_state & CALL_ACTIVE)) {
		wm8994->codec_state |= CALL_ACTIVE;
		wm8994->cur_path = path_num;
		wm8994->universal_voicecall_path[wm8994->cur_path] (codec);
	} else {
		int val;
		val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1);
		val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
		val |= (WM8994_AIF1DAC1_UNMUTE);
		wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, val);
	}

	return 0;
}

static int wm8994_get_input_source(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("input_source_state = [%d]", wm8994->input_source);

	return wm8994->input_source;
}

static int wm8994_set_input_source(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	int control_flag = ucontrol->value.integer.value[0];

	DEBUG_LOG("Changed input_source state [%d] => [%d]",
			wm8994->input_source, control_flag);

	wm8994->input_source = control_flag;

	return 0;
}

#define  SOC_WM899X_OUTPGA_DOUBLE_R_TLV(xname, reg_left, reg_right,\
		xshift, xmax, xinvert, tlv_array) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
		 SNDRV_CTL_ELEM_ACCESS_READWRITE,\
	.tlv.p = (tlv_array), \
	.info = snd_soc_info_volsw_2r, \
	.get = snd_soc_get_volsw_2r, .put = wm899x_outpga_put_volsw_vu, \
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = reg_left, .rreg = reg_right, .shift = xshift, \
		.max = xmax, .invert = xinvert} }

#define SOC_WM899X_OUTPGA_SINGLE_R_TLV(xname, reg, shift, max, invert,\
		tlv_array) {\
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
		.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
				SNDRV_CTL_ELEM_ACCESS_READWRITE,\
		.tlv.p = (tlv_array), \
		.info = snd_soc_info_volsw, \
		.get = snd_soc_get_volsw, .put = wm899x_inpga_put_volsw_vu, \
		.private_value = SOC_SINGLE_VALUE(reg, shift, max, invert) }

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7162, 37, 1);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_spkr, -5700, 600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_rcv, -5700, 600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_headphone, -5700, 600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_mic, -7162, 7162);

static const struct soc_enum path_control_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(playback_path), playback_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicecall_path), voicecall_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_path), mic_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(input_source_state), input_source_state),
};

static const struct snd_kcontrol_new wm8994_snd_controls[] = {
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Volume",
				       WM8994_LEFT_OPGA_VOLUME,
				       WM8994_RIGHT_OPGA_VOLUME, 0, 0x3F, 0,
				       digital_tlv_rcv),
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Spkr Volume",
				       WM8994_SPEAKER_VOLUME_LEFT,
				       WM8994_SPEAKER_VOLUME_RIGHT, 1, 0x3F, 0,
				       digital_tlv_spkr),
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Headset Volume",
				       WM8994_LEFT_OUTPUT_VOLUME,
				       WM8994_RIGHT_OUTPUT_VOLUME, 1, 0x3F, 0,
				       digital_tlv_headphone),
	SOC_WM899X_OUTPGA_SINGLE_R_TLV("Capture Volume",
				       WM8994_AIF1_ADC1_LEFT_VOLUME,
				       0, 0xEF, 0, digital_tlv_mic),
	/* Path Control */
	SOC_ENUM_EXT("Playback Path", path_control_enum[0],
		     wm8994_get_path, wm8994_set_path),

	SOC_ENUM_EXT("Voice Call Path", path_control_enum[1],
		     wm8994_get_voice_path, wm8994_set_voice_path),

	SOC_ENUM_EXT("Capture MIC Path", path_control_enum[2],
		     wm8994_get_mic_path, wm8994_set_mic_path),

#if defined USE_INFINIEON_EC_FOR_VT
	SOC_ENUM_EXT("Clock Control", clock_control_enum[0],
		     s3c_pcmdev_get_clock, s3c_pcmdev_set_clock),
#endif
	SOC_ENUM_EXT("Input Source", path_control_enum[3],
		     wm8994_get_input_source, wm8994_set_input_source),

};

/* Add non-DAPM controls */
static int wm8994_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(wm8994_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				  snd_soc_cnew(&wm8994_snd_controls[i],
					       codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}
static const struct snd_soc_dapm_widget wm8994_dapm_widgets[] = {
};

static const struct snd_soc_dapm_route audio_map[] = {
};

static int wm8994_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, wm8994_dapm_widgets,
			ARRAY_SIZE(wm8994_dapm_widgets));

	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}

static int configure_clock(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->drvdata;
	unsigned int reg;

	DEBUG_LOG("");

	if (wm8994->codec_state != DEACTIVE) {
		DEBUG_LOG("Codec is already actvied. Skip clock setting.");
		return 0;
	}

	reg = wm8994_read(codec, WM8994_AIF1_CLOCKING_1);
	reg &= ~WM8994_AIF1CLK_ENA;
	reg &= ~WM8994_AIF1CLK_SRC_MASK;
	wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

	switch (wm8994->sysclk_source) {
	case WM8994_SYSCLK_MCLK:
		dev_dbg(codec->dev, "Using %dHz MCLK\n", wm8994->mclk_rate);

		reg = wm8994_read(codec, WM8994_AIF1_CLOCKING_1);
		reg &= ~WM8994_AIF1CLK_ENA;
		wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

		reg = wm8994_read(codec, WM8994_AIF1_CLOCKING_1);
		reg &= 0x07;

		if (wm8994->mclk_rate > 13500000) {
			reg |= WM8994_AIF1CLK_DIV;
			wm8994->sysclk_rate = wm8994->mclk_rate / 2;
		} else {
			reg &= ~WM8994_AIF1CLK_DIV;
			wm8994->sysclk_rate = wm8994->mclk_rate;
		}
		reg |= WM8994_AIF1CLK_ENA;
		wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

		/* Enable clocks to the Audio core and sysclk of wm8994 */
		reg = wm8994_read(codec, WM8994_CLOCKING_1);
		reg &= ~(WM8994_SYSCLK_SRC_MASK | WM8994_DSP_FSINTCLK_ENA_MASK
				| WM8994_DSP_FS1CLK_ENA_MASK);
		reg |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
		wm8994_write(codec, WM8994_CLOCKING_1, reg);
		break;

	case WM8994_SYSCLK_FLL:
		switch (wm8994->fs) {
		case 8000:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x2F00);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 11025:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x1F00);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x86C2);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x00e0);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 12000:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x1F00);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 16000:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x1900);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0xE23E);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 22050:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0F00);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x86C2);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x00E0);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 24000:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0F00);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 32000:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0C00);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0xE23E);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 44100:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0700);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x86C2);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x00E0);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		case 48000:
			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0700);
			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
			wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
			wm8994_write(codec, WM8994_FLL1_CONTROL_1,
				WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
			break;

		default:
			DEBUG_LOG_ERR("Unsupported Frequency\n");
			break;
		}

		reg = wm8994_read(codec, WM8994_AIF1_CLOCKING_1);
		reg |= WM8994_AIF1CLK_ENA;
		reg |= WM8994_AIF1CLK_SRC_FLL1;
		wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

		/* Enable clocks to the Audio core and sysclk of wm8994*/
		reg = wm8994_read(codec, WM8994_CLOCKING_1);
		reg &= ~(WM8994_SYSCLK_SRC_MASK | WM8994_DSP_FSINTCLK_ENA_MASK |
				WM8994_DSP_FS1CLK_ENA_MASK);
		reg |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
		wm8994_write(codec, WM8994_CLOCKING_1, reg);
		break;

	default:
		dev_err(codec->dev, "System clock not configured\n");
		return -EINVAL;
	}

	dev_dbg(codec->dev, "CLK_SYS is %dHz\n", wm8994->sysclk_rate);

	return 0;
}

static int wm8994_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	DEBUG_LOG("");

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		/* VMID=2*40k */
		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
				    WM8994_VMID_SEL_MASK, 0x2);
		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_2,
				    WM8994_TSHUT_ENA, WM8994_TSHUT_ENA);
		break;

	case SND_SOC_BIAS_STANDBY:
		if (codec->bias_level == SND_SOC_BIAS_OFF) {
			/* Bring up VMID with fast soft start */
			snd_soc_update_bits(codec, WM8994_ANTIPOP_2,
					    WM8994_STARTUP_BIAS_ENA |
					    WM8994_VMID_BUF_ENA |
					    WM8994_VMID_RAMP_MASK |
					    WM8994_BIAS_SRC,
					    WM8994_STARTUP_BIAS_ENA |
					    WM8994_VMID_BUF_ENA |
					    WM8994_VMID_RAMP_MASK |
					    WM8994_BIAS_SRC);
			/* VMID=2*40k */
			snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
					    WM8994_VMID_SEL_MASK |
					    WM8994_BIAS_ENA,
					    WM8994_BIAS_ENA | 0x2);

			/* Switch to normal bias */
			snd_soc_update_bits(codec, WM8994_ANTIPOP_2,
					    WM8994_BIAS_SRC |
					    WM8994_STARTUP_BIAS_ENA, 0);
		}

		/* VMID=2*240k */
		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
				    WM8994_VMID_SEL_MASK, 0x4);

		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_2,
				    WM8994_TSHUT_ENA, 0);
		break;

	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, WM8994_ANTIPOP_1,
				    WM8994_LINEOUT_VMID_BUF_ENA, 0);

		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
				    WM8994_VMID_SEL_MASK | WM8994_BIAS_ENA, 0);
		break;
	}

	codec->bias_level = level;

	return 0;
}

static int wm8994_set_sysclk(struct snd_soc_dai *codec_dai,
			     int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("clk_id =%d ", clk_id);

	switch (clk_id) {
	case WM8994_SYSCLK_MCLK:
		wm8994->mclk_rate = freq;
		wm8994->sysclk_source = clk_id;
		break;
	case WM8994_SYSCLK_FLL:
		wm8994->sysclk_rate = freq;
		wm8994->sysclk_source = clk_id;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int wm8994_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	unsigned int aif1 = wm8994_read(codec, WM8994_AIF1_CONTROL_1);
	unsigned int aif2 = wm8994_read(codec, WM8994_AIF1_MASTER_SLAVE);

	DEBUG_LOG("");

	aif1 &= ~(WM8994_AIF1_LRCLK_INV | WM8994_AIF1_BCLK_INV |
			WM8994_AIF1_WL_MASK | WM8994_AIF1_FMT_MASK);

	aif2 &= ~(WM8994_AIF1_LRCLK_FRC_MASK |
			WM8994_AIF1_CLK_FRC | WM8994_AIF1_MSTR);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		wm8994->master = 0;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		aif2 |= (WM8994_AIF1_MSTR | WM8994_AIF1_LRCLK_FRC);
		wm8994->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		aif2 |= (WM8994_AIF1_MSTR | WM8994_AIF1_CLK_FRC);
		wm8994->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		aif2 |= (WM8994_AIF1_MSTR | WM8994_AIF1_CLK_FRC |
				WM8994_AIF1_LRCLK_FRC);
		wm8994->master = 1;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_B:
		aif1 |= WM8994_AIF1_LRCLK_INV;
	case SND_SOC_DAIFMT_DSP_A:
		aif1 |= 0x18;
		break;
	case SND_SOC_DAIFMT_I2S:
		aif1 |= 0x10;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		aif1 |= 0x8;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |= WM8994_AIF1_BCLK_INV;
			break;
		default:
			return -EINVAL;
		}
		break;

	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_IF:
			aif1 |= WM8994_AIF1_BCLK_INV | WM8994_AIF1_LRCLK_INV;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |= WM8994_AIF1_BCLK_INV;
			break;
		case SND_SOC_DAIFMT_NB_IF:
			aif1 |= WM8994_AIF1_LRCLK_INV;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	aif1 |= 0x4000;
	wm8994_write(codec, WM8994_AIF1_CONTROL_1, aif1);
	wm8994_write(codec, WM8994_AIF1_MASTER_SLAVE, aif2);
	wm8994_write(codec, WM8994_AIF1_CONTROL_2, 0x4000);

	return 0;
}

static int wm8994_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;
	int ret, i, best, best_val, cur_val;
	unsigned int clocking1, clocking3, aif1, aif4, aif5;

	DEBUG_LOG("");

	clocking1 = wm8994_read(codec, WM8994_AIF1_BCLK);
	clocking1 &= ~WM8994_AIF1_BCLK_DIV_MASK;

	clocking3 = wm8994_read(codec, WM8994_AIF1_RATE);
	clocking3 &= ~(WM8994_AIF1_SR_MASK | WM8994_AIF1CLK_RATE_MASK);

	aif1 = wm8994_read(codec, WM8994_AIF1_CONTROL_1);
	aif1 &= ~WM8994_AIF1_WL_MASK;
	aif4 = wm8994_read(codec, WM8994_AIF1ADC_LRCLK);
	aif4 &= ~WM8994_AIF1ADC_LRCLK_DIR;
	aif5 = wm8994_read(codec, WM8994_AIF1DAC_LRCLK);
	aif5 &= ~WM8994_AIF1DAC_LRCLK_DIR_MASK;

	wm8994->fs = params_rate(params);
	wm8994->bclk = 2 * wm8994->fs;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		wm8994->bclk *= 16;
		break;

	case SNDRV_PCM_FORMAT_S20_3LE:
		wm8994->bclk *= 20;
		aif1 |= (0x01 << WM8994_AIF1_WL_SHIFT);
		break;

	case SNDRV_PCM_FORMAT_S24_LE:
		wm8994->bclk *= 24;
		aif1 |= (0x10 << WM8994_AIF1_WL_SHIFT);
		break;

	case SNDRV_PCM_FORMAT_S32_LE:
		wm8994->bclk *= 32;
		aif1 |= (0x11 << WM8994_AIF1_WL_SHIFT);
		break;

	default:
		return -EINVAL;
	}

	ret = configure_clock(codec);
	if (ret != 0)
		return ret;

	dev_dbg(codec->dev, "Target BCLK is %dHz\n", wm8994->bclk);

	/* Select nearest CLK_SYS_RATE */
	if (wm8994->fs == 8000)
		best = 3;
	else {
		best = 0;
		best_val = abs((wm8994->sysclk_rate / clk_sys_rates[0].ratio)
				- wm8994->fs);

		for (i = 1; i < ARRAY_SIZE(clk_sys_rates); i++) {
			cur_val = abs((wm8994->sysclk_rate /
					clk_sys_rates[i].ratio)	- wm8994->fs);

			if (cur_val < best_val) {
				best = i;
				best_val = cur_val;
			}
		}
		dev_dbg(codec->dev, "Selected CLK_SYS_RATIO of %d\n",
				clk_sys_rates[best].ratio);
	}

	clocking3 |= (clk_sys_rates[best].clk_sys_rate
			<< WM8994_AIF1CLK_RATE_SHIFT);

	/* Sampling rate */
	best = 0;
	best_val = abs(wm8994->fs - sample_rates[0].rate);
	for (i = 1; i < ARRAY_SIZE(sample_rates); i++) {
		cur_val = abs(wm8994->fs - sample_rates[i].rate);
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	dev_dbg(codec->dev, "Selected SAMPLE_RATE of %dHz\n",
			sample_rates[best].rate);

	clocking3 |= (sample_rates[best].sample_rate << WM8994_AIF1_SR_SHIFT);

	/* BCLK_DIV */
	best = 0;
	best_val = INT_MAX;
	for (i = 0; i < ARRAY_SIZE(bclk_divs); i++) {
		cur_val = ((wm8994->sysclk_rate) / bclk_divs[i].div)
				  - wm8994->bclk;
		if (cur_val < 0)
			break;
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	wm8994->bclk = (wm8994->sysclk_rate) / bclk_divs[best].div;

	dev_dbg(codec->dev, "Selected BCLK_DIV of %d for %dHz BCLK\n",
			bclk_divs[best].div, wm8994->bclk);

	clocking1 |= bclk_divs[best].bclk_div << WM8994_AIF1_BCLK_DIV_SHIFT;

	/* LRCLK is a simple fraction of BCLK */
	dev_dbg(codec->dev, "LRCLK_RATE is %d\n", wm8994->bclk / wm8994->fs);

	aif4 |= wm8994->bclk / wm8994->fs;
	aif5 |= wm8994->bclk / wm8994->fs;

#ifdef HDMI_USE_AUDIO
	/* set bclk to 32fs for 44.1kHz 16 bit playback.*/
	if (wm8994->fs == 44100)
		wm8994_write(codec, WM8994_AIF1_BCLK, 0x70);
#endif

	wm8994_write(codec, WM8994_AIF1_RATE, clocking3);
	wm8994_write(codec, WM8994_AIF1_CONTROL_1, aif1);

	return 0;
}

static int wm8994_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int mute_reg;
	int reg;

	switch (codec_dai->id) {
	case 1:
		mute_reg = WM8994_AIF1_DAC1_FILTERS_1;
		break;
	case 2:
		mute_reg = WM8994_AIF2_DAC_FILTERS_1;
		break;
	default:
		return -EINVAL;
	}

	if (mute)
		reg = WM8994_AIF1DAC1_MUTE;
	else
		reg = 0;

	snd_soc_update_bits(codec, mute_reg, WM8994_AIF1DAC1_MUTE, reg);

	return 0;
}

static int wm8994_startup(struct snd_pcm_substream *substream,
			  struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		wm8994->stream_state |=  PCM_STREAM_PLAYBACK;
	else
		wm8994->stream_state |= PCM_STREAM_CAPTURE;


	if (wm8994->power_state == CODEC_OFF) {
		wm8994->power_state = CODEC_ON;
		DEBUG_LOG("Turn on codec!! Power state =[%d]",
				wm8994->power_state);

		/* For initialize codec */
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1,
				0x3 << WM8994_VMID_SEL_SHIFT | WM8994_BIAS_ENA);
		msleep(50);
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1,
				WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);
		wm8994_write(codec, WM8994_OVERSAMPLING, 0x0000);
	} else
		DEBUG_LOG("Already turned on codec!!");

	return 0;
}

static void wm8994_shutdown(struct snd_pcm_substream *substream,
			    struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("Stream_state = [0x%X],  Codec State = [0x%X]",
			wm8994->stream_state, wm8994->codec_state);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		wm8994->stream_state &=  ~(PCM_STREAM_CAPTURE);
		wm8994->codec_state &= ~(CAPTURE_ACTIVE);
	} else {
		wm8994->codec_state &= ~(PLAYBACK_ACTIVE);
		wm8994->stream_state &= ~(PCM_STREAM_PLAYBACK);
	}

	if ((wm8994->codec_state == DEACTIVE) &&
			(wm8994->stream_state == PCM_STREAM_DEACTIVE)) {
		DEBUG_LOG("Turn off Codec!!");
		wm8994->pdata->set_mic_bias(false);
		wm8994->power_state = CODEC_OFF;
		wm8994->cur_path = OFF;
		wm8994->rec_path = MIC_OFF;
		wm8994->ringtone_active = RING_OFF;
		wm8994_write(codec, WM8994_SOFTWARE_RESET, 0x0000);
		return;
	}

	DEBUG_LOG("Preserve codec state = [0x%X], Stream State = [0x%X]",
			wm8994->codec_state, wm8994->stream_state);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		wm8994_disable_rec_path(codec);
		wm8994->codec_state &= ~(CAPTURE_ACTIVE);
	} else {
		if (wm8994->codec_state & CALL_ACTIVE) {
			int val;

			val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1);
			val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
			val |= (WM8994_AIF1DAC1_MUTE);
			wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, val);
		} else
			wm8994_disable_path(codec);
	}
}

static struct snd_soc_device *wm8994_socdev;
static struct snd_soc_codec *wm8994_codec;

#define WM8994_RATES SNDRV_PCM_RATE_44100
#define WM8994_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
static struct snd_soc_dai_ops wm8994_ops = {
	.startup = wm8994_startup,
	.shutdown = wm8994_shutdown,
	.set_sysclk = wm8994_set_sysclk,
	.set_fmt = wm8994_set_dai_fmt,
	.hw_params = wm8994_hw_params,
	.digital_mute = NULL,
};

struct snd_soc_dai wm8994_dai = {

	.name = "WM8994 PAIFRX",
	.playback = {
		     .stream_name = "Playback",
		     .channels_min = 1,
		     .channels_max = 6,
		     .rates = WM8994_RATES,
		     .formats = WM8994_FORMATS,
		     },
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 2,
		    .rates = WM8994_RATES,
		    .formats = WM8994_FORMATS,
		    },

	.ops = &wm8994_ops,
};

static int __init gain_code_setup(char *str)
{

	gain_code = 0;

	if (!strcmp(str, "")) {
		pr_info("gain_code field is empty. use default value\n");
		return 0;
	}

	if (!strcmp(str, "1"))
		gain_code = 1;

	return 0;
}
__setup("gain_code=", gain_code_setup);

int gain_code_check(void)
{
	return gain_code;
}

static const struct {
	unsigned short readable;   /* Mask of readable bits */
	unsigned short writable;   /* Mask of writable bits */
} access_masks[] = {
	{ 0xFFFF, 0xFFFF }, /* R0     - Software Reset */
	{ 0x3B37, 0x3B37 }, /* R1     - Power Management (1) */
	{ 0x6BF0, 0x6BF0 }, /* R2     - Power Management (2) */
	{ 0x3FF0, 0x3FF0 }, /* R3     - Power Management (3) */
	{ 0x3F3F, 0x3F3F }, /* R4     - Power Management (4) */
	{ 0x3F0F, 0x3F0F }, /* R5     - Power Management (5) */
	{ 0x003F, 0x003F }, /* R6     - Power Management (6) */
	{ 0x0000, 0x0000 }, /* R7 */
	{ 0x0000, 0x0000 }, /* R8 */
	{ 0x0000, 0x0000 }, /* R9 */
	{ 0x0000, 0x0000 }, /* R10 */
	{ 0x0000, 0x0000 }, /* R11 */
	{ 0x0000, 0x0000 }, /* R12 */
	{ 0x0000, 0x0000 }, /* R13 */
	{ 0x0000, 0x0000 }, /* R14 */
	{ 0x0000, 0x0000 }, /* R15 */
	{ 0x0000, 0x0000 }, /* R16 */
	{ 0x0000, 0x0000 }, /* R17 */
	{ 0x0000, 0x0000 }, /* R18 */
	{ 0x0000, 0x0000 }, /* R19 */
	{ 0x0000, 0x0000 }, /* R20 */
	{ 0x01C0, 0x01C0 }, /* R21    - Input Mixer (1) */
	{ 0x0000, 0x0000 }, /* R22 */
	{ 0x0000, 0x0000 }, /* R23 */
	{ 0x00DF, 0x01DF }, /* R24    - Left Line Input 1&2 Volume */
	{ 0x00DF, 0x01DF }, /* R25    - Left Line Input 3&4 Volume */
	{ 0x00DF, 0x01DF }, /* R26    - Right Line Input 1&2 Volume */
	{ 0x00DF, 0x01DF }, /* R27    - Right Line Input 3&4 Volume */
	{ 0x00FF, 0x01FF }, /* R28    - Left Output Volume */
	{ 0x00FF, 0x01FF }, /* R29    - Right Output Volume */
	{ 0x0077, 0x0077 }, /* R30    - Line Outputs Volume */
	{ 0x0030, 0x0030 }, /* R31    - HPOUT2 Volume */
	{ 0x00FF, 0x01FF }, /* R32    - Left OPGA Volume */
	{ 0x00FF, 0x01FF }, /* R33    - Right OPGA Volume */
	{ 0x007F, 0x007F }, /* R34    - SPKMIXL Attenuation */
	{ 0x017F, 0x017F }, /* R35    - SPKMIXR Attenuation */
	{ 0x003F, 0x003F }, /* R36    - SPKOUT Mixers */
	{ 0x003F, 0x003F }, /* R37    - ClassD */
	{ 0x00FF, 0x01FF }, /* R38    - Speaker Volume Left */
	{ 0x00FF, 0x01FF }, /* R39    - Speaker Volume Right */
	{ 0x00FF, 0x00FF }, /* R40    - Input Mixer (2) */
	{ 0x01B7, 0x01B7 }, /* R41    - Input Mixer (3) */
	{ 0x01B7, 0x01B7 }, /* R42    - Input Mixer (4) */
	{ 0x01C7, 0x01C7 }, /* R43    - Input Mixer (5) */
	{ 0x01C7, 0x01C7 }, /* R44    - Input Mixer (6) */
	{ 0x01FF, 0x01FF }, /* R45    - Output Mixer (1) */
	{ 0x01FF, 0x01FF }, /* R46    - Output Mixer (2) */
	{ 0x0FFF, 0x0FFF }, /* R47    - Output Mixer (3) */
	{ 0x0FFF, 0x0FFF }, /* R48    - Output Mixer (4) */
	{ 0x0FFF, 0x0FFF }, /* R49    - Output Mixer (5) */
	{ 0x0FFF, 0x0FFF }, /* R50    - Output Mixer (6) */
	{ 0x0038, 0x0038 }, /* R51    - HPOUT2 Mixer */
	{ 0x0077, 0x0077 }, /* R52    - Line Mixer (1) */
	{ 0x0077, 0x0077 }, /* R53    - Line Mixer (2) */
	{ 0x03FF, 0x03FF }, /* R54    - Speaker Mixer */
	{ 0x00C1, 0x00C1 }, /* R55    - Additional Control */
	{ 0x00F0, 0x00F0 }, /* R56    - AntiPOP (1) */
	{ 0x01EF, 0x01EF }, /* R57    - AntiPOP (2) */
	{ 0x00FF, 0x00FF }, /* R58    - MICBIAS */
	{ 0x000F, 0x000F }, /* R59    - LDO 1 */
	{ 0x0007, 0x0007 }, /* R60    - LDO 2 */
	{ 0x0000, 0x0000 }, /* R61 */
	{ 0x0000, 0x0000 }, /* R62 */
	{ 0x0000, 0x0000 }, /* R63 */
	{ 0x0000, 0x0000 }, /* R64 */
	{ 0x0000, 0x0000 }, /* R65 */
	{ 0x0000, 0x0000 }, /* R66 */
	{ 0x0000, 0x0000 }, /* R67 */
	{ 0x0000, 0x0000 }, /* R68 */
	{ 0x0000, 0x0000 }, /* R69 */
	{ 0x0000, 0x0000 }, /* R70 */
	{ 0x0000, 0x0000 }, /* R71 */
	{ 0x0000, 0x0000 }, /* R72 */
	{ 0x0000, 0x0000 }, /* R73 */
	{ 0x0000, 0x0000 }, /* R74 */
	{ 0x0000, 0x0000 }, /* R75 */
	{ 0x8000, 0x8000 }, /* R76    - Charge Pump (1) */
	{ 0x0000, 0x0000 }, /* R77 */
	{ 0x0000, 0x0000 }, /* R78 */
	{ 0x0000, 0x0000 }, /* R79 */
	{ 0x0000, 0x0000 }, /* R80 */
	{ 0x0301, 0x0301 }, /* R81    - Class W (1) */
	{ 0x0000, 0x0000 }, /* R82 */
	{ 0x0000, 0x0000 }, /* R83 */
	{ 0x333F, 0x333F }, /* R84    - DC Servo (1) */
	{ 0x0FEF, 0x0FEF }, /* R85    - DC Servo (2) */
	{ 0x0000, 0x0000 }, /* R86 */
	{ 0xFFFF, 0xFFFF }, /* R87    - DC Servo (4) */
	{ 0x0333, 0x0000 }, /* R88    - DC Servo Readback */
	{ 0x0000, 0x0000 }, /* R89 */
	{ 0x0000, 0x0000 }, /* R90 */
	{ 0x0000, 0x0000 }, /* R91 */
	{ 0x0000, 0x0000 }, /* R92 */
	{ 0x0000, 0x0000 }, /* R93 */
	{ 0x0000, 0x0000 }, /* R94 */
	{ 0x0000, 0x0000 }, /* R95 */
	{ 0x00EE, 0x00EE }, /* R96    - Analogue HP (1) */
	{ 0x0000, 0x0000 }, /* R97 */
	{ 0x0000, 0x0000 }, /* R98 */
	{ 0x0000, 0x0000 }, /* R99 */
	{ 0x0000, 0x0000 }, /* R100 */
	{ 0x0000, 0x0000 }, /* R101 */
	{ 0x0000, 0x0000 }, /* R102 */
	{ 0x0000, 0x0000 }, /* R103 */
	{ 0x0000, 0x0000 }, /* R104 */
	{ 0x0000, 0x0000 }, /* R105 */
	{ 0x0000, 0x0000 }, /* R106 */
	{ 0x0000, 0x0000 }, /* R107 */
	{ 0x0000, 0x0000 }, /* R108 */
	{ 0x0000, 0x0000 }, /* R109 */
	{ 0x0000, 0x0000 }, /* R110 */
	{ 0x0000, 0x0000 }, /* R111 */
	{ 0x0000, 0x0000 }, /* R112 */
	{ 0x0000, 0x0000 }, /* R113 */
	{ 0x0000, 0x0000 }, /* R114 */
	{ 0x0000, 0x0000 }, /* R115 */
	{ 0x0000, 0x0000 }, /* R116 */
	{ 0x0000, 0x0000 }, /* R117 */
	{ 0x0000, 0x0000 }, /* R118 */
	{ 0x0000, 0x0000 }, /* R119 */
	{ 0x0000, 0x0000 }, /* R120 */
	{ 0x0000, 0x0000 }, /* R121 */
	{ 0x0000, 0x0000 }, /* R122 */
	{ 0x0000, 0x0000 }, /* R123 */
	{ 0x0000, 0x0000 }, /* R124 */
	{ 0x0000, 0x0000 }, /* R125 */
	{ 0x0000, 0x0000 }, /* R126 */
	{ 0x0000, 0x0000 }, /* R127 */
	{ 0x0000, 0x0000 }, /* R128 */
	{ 0x0000, 0x0000 }, /* R129 */
	{ 0x0000, 0x0000 }, /* R130 */
	{ 0x0000, 0x0000 }, /* R131 */
	{ 0x0000, 0x0000 }, /* R132 */
	{ 0x0000, 0x0000 }, /* R133 */
	{ 0x0000, 0x0000 }, /* R134 */
	{ 0x0000, 0x0000 }, /* R135 */
	{ 0x0000, 0x0000 }, /* R136 */
	{ 0x0000, 0x0000 }, /* R137 */
	{ 0x0000, 0x0000 }, /* R138 */
	{ 0x0000, 0x0000 }, /* R139 */
	{ 0x0000, 0x0000 }, /* R140 */
	{ 0x0000, 0x0000 }, /* R141 */
	{ 0x0000, 0x0000 }, /* R142 */
	{ 0x0000, 0x0000 }, /* R143 */
	{ 0x0000, 0x0000 }, /* R144 */
	{ 0x0000, 0x0000 }, /* R145 */
	{ 0x0000, 0x0000 }, /* R146 */
	{ 0x0000, 0x0000 }, /* R147 */
	{ 0x0000, 0x0000 }, /* R148 */
	{ 0x0000, 0x0000 }, /* R149 */
	{ 0x0000, 0x0000 }, /* R150 */
	{ 0x0000, 0x0000 }, /* R151 */
	{ 0x0000, 0x0000 }, /* R152 */
	{ 0x0000, 0x0000 }, /* R153 */
	{ 0x0000, 0x0000 }, /* R154 */
	{ 0x0000, 0x0000 }, /* R155 */
	{ 0x0000, 0x0000 }, /* R156 */
	{ 0x0000, 0x0000 }, /* R157 */
	{ 0x0000, 0x0000 }, /* R158 */
	{ 0x0000, 0x0000 }, /* R159 */
	{ 0x0000, 0x0000 }, /* R160 */
	{ 0x0000, 0x0000 }, /* R161 */
	{ 0x0000, 0x0000 }, /* R162 */
	{ 0x0000, 0x0000 }, /* R163 */
	{ 0x0000, 0x0000 }, /* R164 */
	{ 0x0000, 0x0000 }, /* R165 */
	{ 0x0000, 0x0000 }, /* R166 */
	{ 0x0000, 0x0000 }, /* R167 */
	{ 0x0000, 0x0000 }, /* R168 */
	{ 0x0000, 0x0000 }, /* R169 */
	{ 0x0000, 0x0000 }, /* R170 */
	{ 0x0000, 0x0000 }, /* R171 */
	{ 0x0000, 0x0000 }, /* R172 */
	{ 0x0000, 0x0000 }, /* R173 */
	{ 0x0000, 0x0000 }, /* R174 */
	{ 0x0000, 0x0000 }, /* R175 */
	{ 0x0000, 0x0000 }, /* R176 */
	{ 0x0000, 0x0000 }, /* R177 */
	{ 0x0000, 0x0000 }, /* R178 */
	{ 0x0000, 0x0000 }, /* R179 */
	{ 0x0000, 0x0000 }, /* R180 */
	{ 0x0000, 0x0000 }, /* R181 */
	{ 0x0000, 0x0000 }, /* R182 */
	{ 0x0000, 0x0000 }, /* R183 */
	{ 0x0000, 0x0000 }, /* R184 */
	{ 0x0000, 0x0000 }, /* R185 */
	{ 0x0000, 0x0000 }, /* R186 */
	{ 0x0000, 0x0000 }, /* R187 */
	{ 0x0000, 0x0000 }, /* R188 */
	{ 0x0000, 0x0000 }, /* R189 */
	{ 0x0000, 0x0000 }, /* R190 */
	{ 0x0000, 0x0000 }, /* R191 */
	{ 0x0000, 0x0000 }, /* R192 */
	{ 0x0000, 0x0000 }, /* R193 */
	{ 0x0000, 0x0000 }, /* R194 */
	{ 0x0000, 0x0000 }, /* R195 */
	{ 0x0000, 0x0000 }, /* R196 */
	{ 0x0000, 0x0000 }, /* R197 */
	{ 0x0000, 0x0000 }, /* R198 */
	{ 0x0000, 0x0000 }, /* R199 */
	{ 0x0000, 0x0000 }, /* R200 */
	{ 0x0000, 0x0000 }, /* R201 */
	{ 0x0000, 0x0000 }, /* R202 */
	{ 0x0000, 0x0000 }, /* R203 */
	{ 0x0000, 0x0000 }, /* R204 */
	{ 0x0000, 0x0000 }, /* R205 */
	{ 0x0000, 0x0000 }, /* R206 */
	{ 0x0000, 0x0000 }, /* R207 */
	{ 0x0000, 0x0000 }, /* R208 */
	{ 0x0000, 0x0000 }, /* R209 */
	{ 0x0000, 0x0000 }, /* R210 */
	{ 0x0000, 0x0000 }, /* R211 */
	{ 0x0000, 0x0000 }, /* R212 */
	{ 0x0000, 0x0000 }, /* R213 */
	{ 0x0000, 0x0000 }, /* R214 */
	{ 0x0000, 0x0000 }, /* R215 */
	{ 0x0000, 0x0000 }, /* R216 */
	{ 0x0000, 0x0000 }, /* R217 */
	{ 0x0000, 0x0000 }, /* R218 */
	{ 0x0000, 0x0000 }, /* R219 */
	{ 0x0000, 0x0000 }, /* R220 */
	{ 0x0000, 0x0000 }, /* R221 */
	{ 0x0000, 0x0000 }, /* R222 */
	{ 0x0000, 0x0000 }, /* R223 */
	{ 0x0000, 0x0000 }, /* R224 */
	{ 0x0000, 0x0000 }, /* R225 */
	{ 0x0000, 0x0000 }, /* R226 */
	{ 0x0000, 0x0000 }, /* R227 */
	{ 0x0000, 0x0000 }, /* R228 */
	{ 0x0000, 0x0000 }, /* R229 */
	{ 0x0000, 0x0000 }, /* R230 */
	{ 0x0000, 0x0000 }, /* R231 */
	{ 0x0000, 0x0000 }, /* R232 */
	{ 0x0000, 0x0000 }, /* R233 */
	{ 0x0000, 0x0000 }, /* R234 */
	{ 0x0000, 0x0000 }, /* R235 */
	{ 0x0000, 0x0000 }, /* R236 */
	{ 0x0000, 0x0000 }, /* R237 */
	{ 0x0000, 0x0000 }, /* R238 */
	{ 0x0000, 0x0000 }, /* R239 */
	{ 0x0000, 0x0000 }, /* R240 */
	{ 0x0000, 0x0000 }, /* R241 */
	{ 0x0000, 0x0000 }, /* R242 */
	{ 0x0000, 0x0000 }, /* R243 */
	{ 0x0000, 0x0000 }, /* R244 */
	{ 0x0000, 0x0000 }, /* R245 */
	{ 0x0000, 0x0000 }, /* R246 */
	{ 0x0000, 0x0000 }, /* R247 */
	{ 0x0000, 0x0000 }, /* R248 */
	{ 0x0000, 0x0000 }, /* R249 */
	{ 0x0000, 0x0000 }, /* R250 */
	{ 0x0000, 0x0000 }, /* R251 */
	{ 0x0000, 0x0000 }, /* R252 */
	{ 0x0000, 0x0000 }, /* R253 */
	{ 0x0000, 0x0000 }, /* R254 */
	{ 0x0000, 0x0000 }, /* R255 */
	{ 0x000F, 0x0000 }, /* R256   - Chip Revision */
	{ 0x0074, 0x0074 }, /* R257   - Control Interface */
	{ 0x0000, 0x0000 }, /* R258 */
	{ 0x0000, 0x0000 }, /* R259 */
	{ 0x0000, 0x0000 }, /* R260 */
	{ 0x0000, 0x0000 }, /* R261 */
	{ 0x0000, 0x0000 }, /* R262 */
	{ 0x0000, 0x0000 }, /* R263 */
	{ 0x0000, 0x0000 }, /* R264 */
	{ 0x0000, 0x0000 }, /* R265 */
	{ 0x0000, 0x0000 }, /* R266 */
	{ 0x0000, 0x0000 }, /* R267 */
	{ 0x0000, 0x0000 }, /* R268 */
	{ 0x0000, 0x0000 }, /* R269 */
	{ 0x0000, 0x0000 }, /* R270 */
	{ 0x0000, 0x0000 }, /* R271 */
	{ 0x807F, 0x837F }, /* R272   - Write Sequencer Ctrl (1) */
	{ 0x017F, 0x0000 }, /* R273   - Write Sequencer Ctrl (2) */
	{ 0x0000, 0x0000 }, /* R274 */
	{ 0x0000, 0x0000 }, /* R275 */
	{ 0x0000, 0x0000 }, /* R276 */
	{ 0x0000, 0x0000 }, /* R277 */
	{ 0x0000, 0x0000 }, /* R278 */
	{ 0x0000, 0x0000 }, /* R279 */
	{ 0x0000, 0x0000 }, /* R280 */
	{ 0x0000, 0x0000 }, /* R281 */
	{ 0x0000, 0x0000 }, /* R282 */
	{ 0x0000, 0x0000 }, /* R283 */
	{ 0x0000, 0x0000 }, /* R284 */
	{ 0x0000, 0x0000 }, /* R285 */
	{ 0x0000, 0x0000 }, /* R286 */
	{ 0x0000, 0x0000 }, /* R287 */
	{ 0x0000, 0x0000 }, /* R288 */
	{ 0x0000, 0x0000 }, /* R289 */
	{ 0x0000, 0x0000 }, /* R290 */
	{ 0x0000, 0x0000 }, /* R291 */
	{ 0x0000, 0x0000 }, /* R292 */
	{ 0x0000, 0x0000 }, /* R293 */
	{ 0x0000, 0x0000 }, /* R294 */
	{ 0x0000, 0x0000 }, /* R295 */
	{ 0x0000, 0x0000 }, /* R296 */
	{ 0x0000, 0x0000 }, /* R297 */
	{ 0x0000, 0x0000 }, /* R298 */
	{ 0x0000, 0x0000 }, /* R299 */
	{ 0x0000, 0x0000 }, /* R300 */
	{ 0x0000, 0x0000 }, /* R301 */
	{ 0x0000, 0x0000 }, /* R302 */
	{ 0x0000, 0x0000 }, /* R303 */
	{ 0x0000, 0x0000 }, /* R304 */
	{ 0x0000, 0x0000 }, /* R305 */
	{ 0x0000, 0x0000 }, /* R306 */
	{ 0x0000, 0x0000 }, /* R307 */
	{ 0x0000, 0x0000 }, /* R308 */
	{ 0x0000, 0x0000 }, /* R309 */
	{ 0x0000, 0x0000 }, /* R310 */
	{ 0x0000, 0x0000 }, /* R311 */
	{ 0x0000, 0x0000 }, /* R312 */
	{ 0x0000, 0x0000 }, /* R313 */
	{ 0x0000, 0x0000 }, /* R314 */
	{ 0x0000, 0x0000 }, /* R315 */
	{ 0x0000, 0x0000 }, /* R316 */
	{ 0x0000, 0x0000 }, /* R317 */
	{ 0x0000, 0x0000 }, /* R318 */
	{ 0x0000, 0x0000 }, /* R319 */
	{ 0x0000, 0x0000 }, /* R320 */
	{ 0x0000, 0x0000 }, /* R321 */
	{ 0x0000, 0x0000 }, /* R322 */
	{ 0x0000, 0x0000 }, /* R323 */
	{ 0x0000, 0x0000 }, /* R324 */
	{ 0x0000, 0x0000 }, /* R325 */
	{ 0x0000, 0x0000 }, /* R326 */
	{ 0x0000, 0x0000 }, /* R327 */
	{ 0x0000, 0x0000 }, /* R328 */
	{ 0x0000, 0x0000 }, /* R329 */
	{ 0x0000, 0x0000 }, /* R330 */
	{ 0x0000, 0x0000 }, /* R331 */
	{ 0x0000, 0x0000 }, /* R332 */
	{ 0x0000, 0x0000 }, /* R333 */
	{ 0x0000, 0x0000 }, /* R334 */
	{ 0x0000, 0x0000 }, /* R335 */
	{ 0x0000, 0x0000 }, /* R336 */
	{ 0x0000, 0x0000 }, /* R337 */
	{ 0x0000, 0x0000 }, /* R338 */
	{ 0x0000, 0x0000 }, /* R339 */
	{ 0x0000, 0x0000 }, /* R340 */
	{ 0x0000, 0x0000 }, /* R341 */
	{ 0x0000, 0x0000 }, /* R342 */
	{ 0x0000, 0x0000 }, /* R343 */
	{ 0x0000, 0x0000 }, /* R344 */
	{ 0x0000, 0x0000 }, /* R345 */
	{ 0x0000, 0x0000 }, /* R346 */
	{ 0x0000, 0x0000 }, /* R347 */
	{ 0x0000, 0x0000 }, /* R348 */
	{ 0x0000, 0x0000 }, /* R349 */
	{ 0x0000, 0x0000 }, /* R350 */
	{ 0x0000, 0x0000 }, /* R351 */
	{ 0x0000, 0x0000 }, /* R352 */
	{ 0x0000, 0x0000 }, /* R353 */
	{ 0x0000, 0x0000 }, /* R354 */
	{ 0x0000, 0x0000 }, /* R355 */
	{ 0x0000, 0x0000 }, /* R356 */
	{ 0x0000, 0x0000 }, /* R357 */
	{ 0x0000, 0x0000 }, /* R358 */
	{ 0x0000, 0x0000 }, /* R359 */
	{ 0x0000, 0x0000 }, /* R360 */
	{ 0x0000, 0x0000 }, /* R361 */
	{ 0x0000, 0x0000 }, /* R362 */
	{ 0x0000, 0x0000 }, /* R363 */
	{ 0x0000, 0x0000 }, /* R364 */
	{ 0x0000, 0x0000 }, /* R365 */
	{ 0x0000, 0x0000 }, /* R366 */
	{ 0x0000, 0x0000 }, /* R367 */
	{ 0x0000, 0x0000 }, /* R368 */
	{ 0x0000, 0x0000 }, /* R369 */
	{ 0x0000, 0x0000 }, /* R370 */
	{ 0x0000, 0x0000 }, /* R371 */
	{ 0x0000, 0x0000 }, /* R372 */
	{ 0x0000, 0x0000 }, /* R373 */
	{ 0x0000, 0x0000 }, /* R374 */
	{ 0x0000, 0x0000 }, /* R375 */
	{ 0x0000, 0x0000 }, /* R376 */
	{ 0x0000, 0x0000 }, /* R377 */
	{ 0x0000, 0x0000 }, /* R378 */
	{ 0x0000, 0x0000 }, /* R379 */
	{ 0x0000, 0x0000 }, /* R380 */
	{ 0x0000, 0x0000 }, /* R381 */
	{ 0x0000, 0x0000 }, /* R382 */
	{ 0x0000, 0x0000 }, /* R383 */
	{ 0x0000, 0x0000 }, /* R384 */
	{ 0x0000, 0x0000 }, /* R385 */
	{ 0x0000, 0x0000 }, /* R386 */
	{ 0x0000, 0x0000 }, /* R387 */
	{ 0x0000, 0x0000 }, /* R388 */
	{ 0x0000, 0x0000 }, /* R389 */
	{ 0x0000, 0x0000 }, /* R390 */
	{ 0x0000, 0x0000 }, /* R391 */
	{ 0x0000, 0x0000 }, /* R392 */
	{ 0x0000, 0x0000 }, /* R393 */
	{ 0x0000, 0x0000 }, /* R394 */
	{ 0x0000, 0x0000 }, /* R395 */
	{ 0x0000, 0x0000 }, /* R396 */
	{ 0x0000, 0x0000 }, /* R397 */
	{ 0x0000, 0x0000 }, /* R398 */
	{ 0x0000, 0x0000 }, /* R399 */
	{ 0x0000, 0x0000 }, /* R400 */
	{ 0x0000, 0x0000 }, /* R401 */
	{ 0x0000, 0x0000 }, /* R402 */
	{ 0x0000, 0x0000 }, /* R403 */
	{ 0x0000, 0x0000 }, /* R404 */
	{ 0x0000, 0x0000 }, /* R405 */
	{ 0x0000, 0x0000 }, /* R406 */
	{ 0x0000, 0x0000 }, /* R407 */
	{ 0x0000, 0x0000 }, /* R408 */
	{ 0x0000, 0x0000 }, /* R409 */
	{ 0x0000, 0x0000 }, /* R410 */
	{ 0x0000, 0x0000 }, /* R411 */
	{ 0x0000, 0x0000 }, /* R412 */
	{ 0x0000, 0x0000 }, /* R413 */
	{ 0x0000, 0x0000 }, /* R414 */
	{ 0x0000, 0x0000 }, /* R415 */
	{ 0x0000, 0x0000 }, /* R416 */
	{ 0x0000, 0x0000 }, /* R417 */
	{ 0x0000, 0x0000 }, /* R418 */
	{ 0x0000, 0x0000 }, /* R419 */
	{ 0x0000, 0x0000 }, /* R420 */
	{ 0x0000, 0x0000 }, /* R421 */
	{ 0x0000, 0x0000 }, /* R422 */
	{ 0x0000, 0x0000 }, /* R423 */
	{ 0x0000, 0x0000 }, /* R424 */
	{ 0x0000, 0x0000 }, /* R425 */
	{ 0x0000, 0x0000 }, /* R426 */
	{ 0x0000, 0x0000 }, /* R427 */
	{ 0x0000, 0x0000 }, /* R428 */
	{ 0x0000, 0x0000 }, /* R429 */
	{ 0x0000, 0x0000 }, /* R430 */
	{ 0x0000, 0x0000 }, /* R431 */
	{ 0x0000, 0x0000 }, /* R432 */
	{ 0x0000, 0x0000 }, /* R433 */
	{ 0x0000, 0x0000 }, /* R434 */
	{ 0x0000, 0x0000 }, /* R435 */
	{ 0x0000, 0x0000 }, /* R436 */
	{ 0x0000, 0x0000 }, /* R437 */
	{ 0x0000, 0x0000 }, /* R438 */
	{ 0x0000, 0x0000 }, /* R439 */
	{ 0x0000, 0x0000 }, /* R440 */
	{ 0x0000, 0x0000 }, /* R441 */
	{ 0x0000, 0x0000 }, /* R442 */
	{ 0x0000, 0x0000 }, /* R443 */
	{ 0x0000, 0x0000 }, /* R444 */
	{ 0x0000, 0x0000 }, /* R445 */
	{ 0x0000, 0x0000 }, /* R446 */
	{ 0x0000, 0x0000 }, /* R447 */
	{ 0x0000, 0x0000 }, /* R448 */
	{ 0x0000, 0x0000 }, /* R449 */
	{ 0x0000, 0x0000 }, /* R450 */
	{ 0x0000, 0x0000 }, /* R451 */
	{ 0x0000, 0x0000 }, /* R452 */
	{ 0x0000, 0x0000 }, /* R453 */
	{ 0x0000, 0x0000 }, /* R454 */
	{ 0x0000, 0x0000 }, /* R455 */
	{ 0x0000, 0x0000 }, /* R456 */
	{ 0x0000, 0x0000 }, /* R457 */
	{ 0x0000, 0x0000 }, /* R458 */
	{ 0x0000, 0x0000 }, /* R459 */
	{ 0x0000, 0x0000 }, /* R460 */
	{ 0x0000, 0x0000 }, /* R461 */
	{ 0x0000, 0x0000 }, /* R462 */
	{ 0x0000, 0x0000 }, /* R463 */
	{ 0x0000, 0x0000 }, /* R464 */
	{ 0x0000, 0x0000 }, /* R465 */
	{ 0x0000, 0x0000 }, /* R466 */
	{ 0x0000, 0x0000 }, /* R467 */
	{ 0x0000, 0x0000 }, /* R468 */
	{ 0x0000, 0x0000 }, /* R469 */
	{ 0x0000, 0x0000 }, /* R470 */
	{ 0x0000, 0x0000 }, /* R471 */
	{ 0x0000, 0x0000 }, /* R472 */
	{ 0x0000, 0x0000 }, /* R473 */
	{ 0x0000, 0x0000 }, /* R474 */
	{ 0x0000, 0x0000 }, /* R475 */
	{ 0x0000, 0x0000 }, /* R476 */
	{ 0x0000, 0x0000 }, /* R477 */
	{ 0x0000, 0x0000 }, /* R478 */
	{ 0x0000, 0x0000 }, /* R479 */
	{ 0x0000, 0x0000 }, /* R480 */
	{ 0x0000, 0x0000 }, /* R481 */
	{ 0x0000, 0x0000 }, /* R482 */
	{ 0x0000, 0x0000 }, /* R483 */
	{ 0x0000, 0x0000 }, /* R484 */
	{ 0x0000, 0x0000 }, /* R485 */
	{ 0x0000, 0x0000 }, /* R486 */
	{ 0x0000, 0x0000 }, /* R487 */
	{ 0x0000, 0x0000 }, /* R488 */
	{ 0x0000, 0x0000 }, /* R489 */
	{ 0x0000, 0x0000 }, /* R490 */
	{ 0x0000, 0x0000 }, /* R491 */
	{ 0x0000, 0x0000 }, /* R492 */
	{ 0x0000, 0x0000 }, /* R493 */
	{ 0x0000, 0x0000 }, /* R494 */
	{ 0x0000, 0x0000 }, /* R495 */
	{ 0x0000, 0x0000 }, /* R496 */
	{ 0x0000, 0x0000 }, /* R497 */
	{ 0x0000, 0x0000 }, /* R498 */
	{ 0x0000, 0x0000 }, /* R499 */
	{ 0x0000, 0x0000 }, /* R500 */
	{ 0x0000, 0x0000 }, /* R501 */
	{ 0x0000, 0x0000 }, /* R502 */
	{ 0x0000, 0x0000 }, /* R503 */
	{ 0x0000, 0x0000 }, /* R504 */
	{ 0x0000, 0x0000 }, /* R505 */
	{ 0x0000, 0x0000 }, /* R506 */
	{ 0x0000, 0x0000 }, /* R507 */
	{ 0x0000, 0x0000 }, /* R508 */
	{ 0x0000, 0x0000 }, /* R509 */
	{ 0x0000, 0x0000 }, /* R510 */
	{ 0x0000, 0x0000 }, /* R511 */
	{ 0x001F, 0x001F }, /* R512   - AIF1 Clocking (1) */
	{ 0x003F, 0x003F }, /* R513   - AIF1 Clocking (2) */
	{ 0x0000, 0x0000 }, /* R514 */
	{ 0x0000, 0x0000 }, /* R515 */
	{ 0x001F, 0x001F }, /* R516   - AIF2 Clocking (1) */
	{ 0x003F, 0x003F }, /* R517   - AIF2 Clocking (2) */
	{ 0x0000, 0x0000 }, /* R518 */
	{ 0x0000, 0x0000 }, /* R519 */
	{ 0x001F, 0x001F }, /* R520   - Clocking (1) */
	{ 0x0777, 0x0777 }, /* R521   - Clocking (2) */
	{ 0x0000, 0x0000 }, /* R522 */
	{ 0x0000, 0x0000 }, /* R523 */
	{ 0x0000, 0x0000 }, /* R524 */
	{ 0x0000, 0x0000 }, /* R525 */
	{ 0x0000, 0x0000 }, /* R526 */
	{ 0x0000, 0x0000 }, /* R527 */
	{ 0x00FF, 0x00FF }, /* R528   - AIF1 Rate */
	{ 0x00FF, 0x00FF }, /* R529   - AIF2 Rate */
	{ 0x000F, 0x0000 }, /* R530   - Rate Status */
	{ 0x0000, 0x0000 }, /* R531 */
	{ 0x0000, 0x0000 }, /* R532 */
	{ 0x0000, 0x0000 }, /* R533 */
	{ 0x0000, 0x0000 }, /* R534 */
	{ 0x0000, 0x0000 }, /* R535 */
	{ 0x0000, 0x0000 }, /* R536 */
	{ 0x0000, 0x0000 }, /* R537 */
	{ 0x0000, 0x0000 }, /* R538 */
	{ 0x0000, 0x0000 }, /* R539 */
	{ 0x0000, 0x0000 }, /* R540 */
	{ 0x0000, 0x0000 }, /* R541 */
	{ 0x0000, 0x0000 }, /* R542 */
	{ 0x0000, 0x0000 }, /* R543 */
	{ 0x0007, 0x0007 }, /* R544   - FLL1 Control (1) */
	{ 0x3F77, 0x3F77 }, /* R545   - FLL1 Control (2) */
	{ 0xFFFF, 0xFFFF }, /* R546   - FLL1 Control (3) */
	{ 0x7FEF, 0x7FEF }, /* R547   - FLL1 Control (4) */
	{ 0x1FDB, 0x1FDB }, /* R548   - FLL1 Control (5) */
	{ 0x0000, 0x0000 }, /* R549 */
	{ 0x0000, 0x0000 }, /* R550 */
	{ 0x0000, 0x0000 }, /* R551 */
	{ 0x0000, 0x0000 }, /* R552 */
	{ 0x0000, 0x0000 }, /* R553 */
	{ 0x0000, 0x0000 }, /* R554 */
	{ 0x0000, 0x0000 }, /* R555 */
	{ 0x0000, 0x0000 }, /* R556 */
	{ 0x0000, 0x0000 }, /* R557 */
	{ 0x0000, 0x0000 }, /* R558 */
	{ 0x0000, 0x0000 }, /* R559 */
	{ 0x0000, 0x0000 }, /* R560 */
	{ 0x0000, 0x0000 }, /* R561 */
	{ 0x0000, 0x0000 }, /* R562 */
	{ 0x0000, 0x0000 }, /* R563 */
	{ 0x0000, 0x0000 }, /* R564 */
	{ 0x0000, 0x0000 }, /* R565 */
	{ 0x0000, 0x0000 }, /* R566 */
	{ 0x0000, 0x0000 }, /* R567 */
	{ 0x0000, 0x0000 }, /* R568 */
	{ 0x0000, 0x0000 }, /* R569 */
	{ 0x0000, 0x0000 }, /* R570 */
	{ 0x0000, 0x0000 }, /* R571 */
	{ 0x0000, 0x0000 }, /* R572 */
	{ 0x0000, 0x0000 }, /* R573 */
	{ 0x0000, 0x0000 }, /* R574 */
	{ 0x0000, 0x0000 }, /* R575 */
	{ 0x0007, 0x0007 }, /* R576   - FLL2 Control (1) */
	{ 0x3F77, 0x3F77 }, /* R577   - FLL2 Control (2) */
	{ 0xFFFF, 0xFFFF }, /* R578   - FLL2 Control (3) */
	{ 0x7FEF, 0x7FEF }, /* R579   - FLL2 Control (4) */
	{ 0x1FDB, 0x1FDB }, /* R580   - FLL2 Control (5) */
	{ 0x0000, 0x0000 }, /* R581 */
	{ 0x0000, 0x0000 }, /* R582 */
	{ 0x0000, 0x0000 }, /* R583 */
	{ 0x0000, 0x0000 }, /* R584 */
	{ 0x0000, 0x0000 }, /* R585 */
	{ 0x0000, 0x0000 }, /* R586 */
	{ 0x0000, 0x0000 }, /* R587 */
	{ 0x0000, 0x0000 }, /* R588 */
	{ 0x0000, 0x0000 }, /* R589 */
	{ 0x0000, 0x0000 }, /* R590 */
	{ 0x0000, 0x0000 }, /* R591 */
	{ 0x0000, 0x0000 }, /* R592 */
	{ 0x0000, 0x0000 }, /* R593 */
	{ 0x0000, 0x0000 }, /* R594 */
	{ 0x0000, 0x0000 }, /* R595 */
	{ 0x0000, 0x0000 }, /* R596 */
	{ 0x0000, 0x0000 }, /* R597 */
	{ 0x0000, 0x0000 }, /* R598 */
	{ 0x0000, 0x0000 }, /* R599 */
	{ 0x0000, 0x0000 }, /* R600 */
	{ 0x0000, 0x0000 }, /* R601 */
	{ 0x0000, 0x0000 }, /* R602 */
	{ 0x0000, 0x0000 }, /* R603 */
	{ 0x0000, 0x0000 }, /* R604 */
	{ 0x0000, 0x0000 }, /* R605 */
	{ 0x0000, 0x0000 }, /* R606 */
	{ 0x0000, 0x0000 }, /* R607 */
	{ 0x0000, 0x0000 }, /* R608 */
	{ 0x0000, 0x0000 }, /* R609 */
	{ 0x0000, 0x0000 }, /* R610 */
	{ 0x0000, 0x0000 }, /* R611 */
	{ 0x0000, 0x0000 }, /* R612 */
	{ 0x0000, 0x0000 }, /* R613 */
	{ 0x0000, 0x0000 }, /* R614 */
	{ 0x0000, 0x0000 }, /* R615 */
	{ 0x0000, 0x0000 }, /* R616 */
	{ 0x0000, 0x0000 }, /* R617 */
	{ 0x0000, 0x0000 }, /* R618 */
	{ 0x0000, 0x0000 }, /* R619 */
	{ 0x0000, 0x0000 }, /* R620 */
	{ 0x0000, 0x0000 }, /* R621 */
	{ 0x0000, 0x0000 }, /* R622 */
	{ 0x0000, 0x0000 }, /* R623 */
	{ 0x0000, 0x0000 }, /* R624 */
	{ 0x0000, 0x0000 }, /* R625 */
	{ 0x0000, 0x0000 }, /* R626 */
	{ 0x0000, 0x0000 }, /* R627 */
	{ 0x0000, 0x0000 }, /* R628 */
	{ 0x0000, 0x0000 }, /* R629 */
	{ 0x0000, 0x0000 }, /* R630 */
	{ 0x0000, 0x0000 }, /* R631 */
	{ 0x0000, 0x0000 }, /* R632 */
	{ 0x0000, 0x0000 }, /* R633 */
	{ 0x0000, 0x0000 }, /* R634 */
	{ 0x0000, 0x0000 }, /* R635 */
	{ 0x0000, 0x0000 }, /* R636 */
	{ 0x0000, 0x0000 }, /* R637 */
	{ 0x0000, 0x0000 }, /* R638 */
	{ 0x0000, 0x0000 }, /* R639 */
	{ 0x0000, 0x0000 }, /* R640 */
	{ 0x0000, 0x0000 }, /* R641 */
	{ 0x0000, 0x0000 }, /* R642 */
	{ 0x0000, 0x0000 }, /* R643 */
	{ 0x0000, 0x0000 }, /* R644 */
	{ 0x0000, 0x0000 }, /* R645 */
	{ 0x0000, 0x0000 }, /* R646 */
	{ 0x0000, 0x0000 }, /* R647 */
	{ 0x0000, 0x0000 }, /* R648 */
	{ 0x0000, 0x0000 }, /* R649 */
	{ 0x0000, 0x0000 }, /* R650 */
	{ 0x0000, 0x0000 }, /* R651 */
	{ 0x0000, 0x0000 }, /* R652 */
	{ 0x0000, 0x0000 }, /* R653 */
	{ 0x0000, 0x0000 }, /* R654 */
	{ 0x0000, 0x0000 }, /* R655 */
	{ 0x0000, 0x0000 }, /* R656 */
	{ 0x0000, 0x0000 }, /* R657 */
	{ 0x0000, 0x0000 }, /* R658 */
	{ 0x0000, 0x0000 }, /* R659 */
	{ 0x0000, 0x0000 }, /* R660 */
	{ 0x0000, 0x0000 }, /* R661 */
	{ 0x0000, 0x0000 }, /* R662 */
	{ 0x0000, 0x0000 }, /* R663 */
	{ 0x0000, 0x0000 }, /* R664 */
	{ 0x0000, 0x0000 }, /* R665 */
	{ 0x0000, 0x0000 }, /* R666 */
	{ 0x0000, 0x0000 }, /* R667 */
	{ 0x0000, 0x0000 }, /* R668 */
	{ 0x0000, 0x0000 }, /* R669 */
	{ 0x0000, 0x0000 }, /* R670 */
	{ 0x0000, 0x0000 }, /* R671 */
	{ 0x0000, 0x0000 }, /* R672 */
	{ 0x0000, 0x0000 }, /* R673 */
	{ 0x0000, 0x0000 }, /* R674 */
	{ 0x0000, 0x0000 }, /* R675 */
	{ 0x0000, 0x0000 }, /* R676 */
	{ 0x0000, 0x0000 }, /* R677 */
	{ 0x0000, 0x0000 }, /* R678 */
	{ 0x0000, 0x0000 }, /* R679 */
	{ 0x0000, 0x0000 }, /* R680 */
	{ 0x0000, 0x0000 }, /* R681 */
	{ 0x0000, 0x0000 }, /* R682 */
	{ 0x0000, 0x0000 }, /* R683 */
	{ 0x0000, 0x0000 }, /* R684 */
	{ 0x0000, 0x0000 }, /* R685 */
	{ 0x0000, 0x0000 }, /* R686 */
	{ 0x0000, 0x0000 }, /* R687 */
	{ 0x0000, 0x0000 }, /* R688 */
	{ 0x0000, 0x0000 }, /* R689 */
	{ 0x0000, 0x0000 }, /* R690 */
	{ 0x0000, 0x0000 }, /* R691 */
	{ 0x0000, 0x0000 }, /* R692 */
	{ 0x0000, 0x0000 }, /* R693 */
	{ 0x0000, 0x0000 }, /* R694 */
	{ 0x0000, 0x0000 }, /* R695 */
	{ 0x0000, 0x0000 }, /* R696 */
	{ 0x0000, 0x0000 }, /* R697 */
	{ 0x0000, 0x0000 }, /* R698 */
	{ 0x0000, 0x0000 }, /* R699 */
	{ 0x0000, 0x0000 }, /* R700 */
	{ 0x0000, 0x0000 }, /* R701 */
	{ 0x0000, 0x0000 }, /* R702 */
	{ 0x0000, 0x0000 }, /* R703 */
	{ 0x0000, 0x0000 }, /* R704 */
	{ 0x0000, 0x0000 }, /* R705 */
	{ 0x0000, 0x0000 }, /* R706 */
	{ 0x0000, 0x0000 }, /* R707 */
	{ 0x0000, 0x0000 }, /* R708 */
	{ 0x0000, 0x0000 }, /* R709 */
	{ 0x0000, 0x0000 }, /* R710 */
	{ 0x0000, 0x0000 }, /* R711 */
	{ 0x0000, 0x0000 }, /* R712 */
	{ 0x0000, 0x0000 }, /* R713 */
	{ 0x0000, 0x0000 }, /* R714 */
	{ 0x0000, 0x0000 }, /* R715 */
	{ 0x0000, 0x0000 }, /* R716 */
	{ 0x0000, 0x0000 }, /* R717 */
	{ 0x0000, 0x0000 }, /* R718 */
	{ 0x0000, 0x0000 }, /* R719 */
	{ 0x0000, 0x0000 }, /* R720 */
	{ 0x0000, 0x0000 }, /* R721 */
	{ 0x0000, 0x0000 }, /* R722 */
	{ 0x0000, 0x0000 }, /* R723 */
	{ 0x0000, 0x0000 }, /* R724 */
	{ 0x0000, 0x0000 }, /* R725 */
	{ 0x0000, 0x0000 }, /* R726 */
	{ 0x0000, 0x0000 }, /* R727 */
	{ 0x0000, 0x0000 }, /* R728 */
	{ 0x0000, 0x0000 }, /* R729 */
	{ 0x0000, 0x0000 }, /* R730 */
	{ 0x0000, 0x0000 }, /* R731 */
	{ 0x0000, 0x0000 }, /* R732 */
	{ 0x0000, 0x0000 }, /* R733 */
	{ 0x0000, 0x0000 }, /* R734 */
	{ 0x0000, 0x0000 }, /* R735 */
	{ 0x0000, 0x0000 }, /* R736 */
	{ 0x0000, 0x0000 }, /* R737 */
	{ 0x0000, 0x0000 }, /* R738 */
	{ 0x0000, 0x0000 }, /* R739 */
	{ 0x0000, 0x0000 }, /* R740 */
	{ 0x0000, 0x0000 }, /* R741 */
	{ 0x0000, 0x0000 }, /* R742 */
	{ 0x0000, 0x0000 }, /* R743 */
	{ 0x0000, 0x0000 }, /* R744 */
	{ 0x0000, 0x0000 }, /* R745 */
	{ 0x0000, 0x0000 }, /* R746 */
	{ 0x0000, 0x0000 }, /* R747 */
	{ 0x0000, 0x0000 }, /* R748 */
	{ 0x0000, 0x0000 }, /* R749 */
	{ 0x0000, 0x0000 }, /* R750 */
	{ 0x0000, 0x0000 }, /* R751 */
	{ 0x0000, 0x0000 }, /* R752 */
	{ 0x0000, 0x0000 }, /* R753 */
	{ 0x0000, 0x0000 }, /* R754 */
	{ 0x0000, 0x0000 }, /* R755 */
	{ 0x0000, 0x0000 }, /* R756 */
	{ 0x0000, 0x0000 }, /* R757 */
	{ 0x0000, 0x0000 }, /* R758 */
	{ 0x0000, 0x0000 }, /* R759 */
	{ 0x0000, 0x0000 }, /* R760 */
	{ 0x0000, 0x0000 }, /* R761 */
	{ 0x0000, 0x0000 }, /* R762 */
	{ 0x0000, 0x0000 }, /* R763 */
	{ 0x0000, 0x0000 }, /* R764 */
	{ 0x0000, 0x0000 }, /* R765 */
	{ 0x0000, 0x0000 }, /* R766 */
	{ 0x0000, 0x0000 }, /* R767 */
	{ 0xE1F8, 0xE1F8 }, /* R768   - AIF1 Control (1) */
	{ 0xCD1F, 0xCD1F }, /* R769   - AIF1 Control (2) */
	{ 0xF000, 0xF000 }, /* R770   - AIF1 Master/Slave */
	{ 0x01F0, 0x01F0 }, /* R771   - AIF1 BCLK */
	{ 0x0FFF, 0x0FFF }, /* R772   - AIF1ADC LRCLK */
	{ 0x0FFF, 0x0FFF }, /* R773   - AIF1DAC LRCLK */
	{ 0x0003, 0x0003 }, /* R774   - AIF1DAC Data */
	{ 0x0003, 0x0003 }, /* R775   - AIF1ADC Data */
	{ 0x0000, 0x0000 }, /* R776 */
	{ 0x0000, 0x0000 }, /* R777 */
	{ 0x0000, 0x0000 }, /* R778 */
	{ 0x0000, 0x0000 }, /* R779 */
	{ 0x0000, 0x0000 }, /* R780 */
	{ 0x0000, 0x0000 }, /* R781 */
	{ 0x0000, 0x0000 }, /* R782 */
	{ 0x0000, 0x0000 }, /* R783 */
	{ 0xF1F8, 0xF1F8 }, /* R784   - AIF2 Control (1) */
	{ 0xFD1F, 0xFD1F }, /* R785   - AIF2 Control (2) */
	{ 0xF000, 0xF000 }, /* R786   - AIF2 Master/Slave */
	{ 0x01F0, 0x01F0 }, /* R787   - AIF2 BCLK */
	{ 0x0FFF, 0x0FFF }, /* R788   - AIF2ADC LRCLK */
	{ 0x0FFF, 0x0FFF }, /* R789   - AIF2DAC LRCLK */
	{ 0x0003, 0x0003 }, /* R790   - AIF2DAC Data */
	{ 0x0003, 0x0003 }, /* R791   - AIF2ADC Data */
	{ 0x0000, 0x0000 }, /* R792 */
	{ 0x0000, 0x0000 }, /* R793 */
	{ 0x0000, 0x0000 }, /* R794 */
	{ 0x0000, 0x0000 }, /* R795 */
	{ 0x0000, 0x0000 }, /* R796 */
	{ 0x0000, 0x0000 }, /* R797 */
	{ 0x0000, 0x0000 }, /* R798 */
	{ 0x0000, 0x0000 }, /* R799 */
	{ 0x0000, 0x0000 }, /* R800 */
	{ 0x0000, 0x0000 }, /* R801 */
	{ 0x0000, 0x0000 }, /* R802 */
	{ 0x0000, 0x0000 }, /* R803 */
	{ 0x0000, 0x0000 }, /* R804 */
	{ 0x0000, 0x0000 }, /* R805 */
	{ 0x0000, 0x0000 }, /* R806 */
	{ 0x0000, 0x0000 }, /* R807 */
	{ 0x0000, 0x0000 }, /* R808 */
	{ 0x0000, 0x0000 }, /* R809 */
	{ 0x0000, 0x0000 }, /* R810 */
	{ 0x0000, 0x0000 }, /* R811 */
	{ 0x0000, 0x0000 }, /* R812 */
	{ 0x0000, 0x0000 }, /* R813 */
	{ 0x0000, 0x0000 }, /* R814 */
	{ 0x0000, 0x0000 }, /* R815 */
	{ 0x0000, 0x0000 }, /* R816 */
	{ 0x0000, 0x0000 }, /* R817 */
	{ 0x0000, 0x0000 }, /* R818 */
	{ 0x0000, 0x0000 }, /* R819 */
	{ 0x0000, 0x0000 }, /* R820 */
	{ 0x0000, 0x0000 }, /* R821 */
	{ 0x0000, 0x0000 }, /* R822 */
	{ 0x0000, 0x0000 }, /* R823 */
	{ 0x0000, 0x0000 }, /* R824 */
	{ 0x0000, 0x0000 }, /* R825 */
	{ 0x0000, 0x0000 }, /* R826 */
	{ 0x0000, 0x0000 }, /* R827 */
	{ 0x0000, 0x0000 }, /* R828 */
	{ 0x0000, 0x0000 }, /* R829 */
	{ 0x0000, 0x0000 }, /* R830 */
	{ 0x0000, 0x0000 }, /* R831 */
	{ 0x0000, 0x0000 }, /* R832 */
	{ 0x0000, 0x0000 }, /* R833 */
	{ 0x0000, 0x0000 }, /* R834 */
	{ 0x0000, 0x0000 }, /* R835 */
	{ 0x0000, 0x0000 }, /* R836 */
	{ 0x0000, 0x0000 }, /* R837 */
	{ 0x0000, 0x0000 }, /* R838 */
	{ 0x0000, 0x0000 }, /* R839 */
	{ 0x0000, 0x0000 }, /* R840 */
	{ 0x0000, 0x0000 }, /* R841 */
	{ 0x0000, 0x0000 }, /* R842 */
	{ 0x0000, 0x0000 }, /* R843 */
	{ 0x0000, 0x0000 }, /* R844 */
	{ 0x0000, 0x0000 }, /* R845 */
	{ 0x0000, 0x0000 }, /* R846 */
	{ 0x0000, 0x0000 }, /* R847 */
	{ 0x0000, 0x0000 }, /* R848 */
	{ 0x0000, 0x0000 }, /* R849 */
	{ 0x0000, 0x0000 }, /* R850 */
	{ 0x0000, 0x0000 }, /* R851 */
	{ 0x0000, 0x0000 }, /* R852 */
	{ 0x0000, 0x0000 }, /* R853 */
	{ 0x0000, 0x0000 }, /* R854 */
	{ 0x0000, 0x0000 }, /* R855 */
	{ 0x0000, 0x0000 }, /* R856 */
	{ 0x0000, 0x0000 }, /* R857 */
	{ 0x0000, 0x0000 }, /* R858 */
	{ 0x0000, 0x0000 }, /* R859 */
	{ 0x0000, 0x0000 }, /* R860 */
	{ 0x0000, 0x0000 }, /* R861 */
	{ 0x0000, 0x0000 }, /* R862 */
	{ 0x0000, 0x0000 }, /* R863 */
	{ 0x0000, 0x0000 }, /* R864 */
	{ 0x0000, 0x0000 }, /* R865 */
	{ 0x0000, 0x0000 }, /* R866 */
	{ 0x0000, 0x0000 }, /* R867 */
	{ 0x0000, 0x0000 }, /* R868 */
	{ 0x0000, 0x0000 }, /* R869 */
	{ 0x0000, 0x0000 }, /* R870 */
	{ 0x0000, 0x0000 }, /* R871 */
	{ 0x0000, 0x0000 }, /* R872 */
	{ 0x0000, 0x0000 }, /* R873 */
	{ 0x0000, 0x0000 }, /* R874 */
	{ 0x0000, 0x0000 }, /* R875 */
	{ 0x0000, 0x0000 }, /* R876 */
	{ 0x0000, 0x0000 }, /* R877 */
	{ 0x0000, 0x0000 }, /* R878 */
	{ 0x0000, 0x0000 }, /* R879 */
	{ 0x0000, 0x0000 }, /* R880 */
	{ 0x0000, 0x0000 }, /* R881 */
	{ 0x0000, 0x0000 }, /* R882 */
	{ 0x0000, 0x0000 }, /* R883 */
	{ 0x0000, 0x0000 }, /* R884 */
	{ 0x0000, 0x0000 }, /* R885 */
	{ 0x0000, 0x0000 }, /* R886 */
	{ 0x0000, 0x0000 }, /* R887 */
	{ 0x0000, 0x0000 }, /* R888 */
	{ 0x0000, 0x0000 }, /* R889 */
	{ 0x0000, 0x0000 }, /* R890 */
	{ 0x0000, 0x0000 }, /* R891 */
	{ 0x0000, 0x0000 }, /* R892 */
	{ 0x0000, 0x0000 }, /* R893 */
	{ 0x0000, 0x0000 }, /* R894 */
	{ 0x0000, 0x0000 }, /* R895 */
	{ 0x0000, 0x0000 }, /* R896 */
	{ 0x0000, 0x0000 }, /* R897 */
	{ 0x0000, 0x0000 }, /* R898 */
	{ 0x0000, 0x0000 }, /* R899 */
	{ 0x0000, 0x0000 }, /* R900 */
	{ 0x0000, 0x0000 }, /* R901 */
	{ 0x0000, 0x0000 }, /* R902 */
	{ 0x0000, 0x0000 }, /* R903 */
	{ 0x0000, 0x0000 }, /* R904 */
	{ 0x0000, 0x0000 }, /* R905 */
	{ 0x0000, 0x0000 }, /* R906 */
	{ 0x0000, 0x0000 }, /* R907 */
	{ 0x0000, 0x0000 }, /* R908 */
	{ 0x0000, 0x0000 }, /* R909 */
	{ 0x0000, 0x0000 }, /* R910 */
	{ 0x0000, 0x0000 }, /* R911 */
	{ 0x0000, 0x0000 }, /* R912 */
	{ 0x0000, 0x0000 }, /* R913 */
	{ 0x0000, 0x0000 }, /* R914 */
	{ 0x0000, 0x0000 }, /* R915 */
	{ 0x0000, 0x0000 }, /* R916 */
	{ 0x0000, 0x0000 }, /* R917 */
	{ 0x0000, 0x0000 }, /* R918 */
	{ 0x0000, 0x0000 }, /* R919 */
	{ 0x0000, 0x0000 }, /* R920 */
	{ 0x0000, 0x0000 }, /* R921 */
	{ 0x0000, 0x0000 }, /* R922 */
	{ 0x0000, 0x0000 }, /* R923 */
	{ 0x0000, 0x0000 }, /* R924 */
	{ 0x0000, 0x0000 }, /* R925 */
	{ 0x0000, 0x0000 }, /* R926 */
	{ 0x0000, 0x0000 }, /* R927 */
	{ 0x0000, 0x0000 }, /* R928 */
	{ 0x0000, 0x0000 }, /* R929 */
	{ 0x0000, 0x0000 }, /* R930 */
	{ 0x0000, 0x0000 }, /* R931 */
	{ 0x0000, 0x0000 }, /* R932 */
	{ 0x0000, 0x0000 }, /* R933 */
	{ 0x0000, 0x0000 }, /* R934 */
	{ 0x0000, 0x0000 }, /* R935 */
	{ 0x0000, 0x0000 }, /* R936 */
	{ 0x0000, 0x0000 }, /* R937 */
	{ 0x0000, 0x0000 }, /* R938 */
	{ 0x0000, 0x0000 }, /* R939 */
	{ 0x0000, 0x0000 }, /* R940 */
	{ 0x0000, 0x0000 }, /* R941 */
	{ 0x0000, 0x0000 }, /* R942 */
	{ 0x0000, 0x0000 }, /* R943 */
	{ 0x0000, 0x0000 }, /* R944 */
	{ 0x0000, 0x0000 }, /* R945 */
	{ 0x0000, 0x0000 }, /* R946 */
	{ 0x0000, 0x0000 }, /* R947 */
	{ 0x0000, 0x0000 }, /* R948 */
	{ 0x0000, 0x0000 }, /* R949 */
	{ 0x0000, 0x0000 }, /* R950 */
	{ 0x0000, 0x0000 }, /* R951 */
	{ 0x0000, 0x0000 }, /* R952 */
	{ 0x0000, 0x0000 }, /* R953 */
	{ 0x0000, 0x0000 }, /* R954 */
	{ 0x0000, 0x0000 }, /* R955 */
	{ 0x0000, 0x0000 }, /* R956 */
	{ 0x0000, 0x0000 }, /* R957 */
	{ 0x0000, 0x0000 }, /* R958 */
	{ 0x0000, 0x0000 }, /* R959 */
	{ 0x0000, 0x0000 }, /* R960 */
	{ 0x0000, 0x0000 }, /* R961 */
	{ 0x0000, 0x0000 }, /* R962 */
	{ 0x0000, 0x0000 }, /* R963 */
	{ 0x0000, 0x0000 }, /* R964 */
	{ 0x0000, 0x0000 }, /* R965 */
	{ 0x0000, 0x0000 }, /* R966 */
	{ 0x0000, 0x0000 }, /* R967 */
	{ 0x0000, 0x0000 }, /* R968 */
	{ 0x0000, 0x0000 }, /* R969 */
	{ 0x0000, 0x0000 }, /* R970 */
	{ 0x0000, 0x0000 }, /* R971 */
	{ 0x0000, 0x0000 }, /* R972 */
	{ 0x0000, 0x0000 }, /* R973 */
	{ 0x0000, 0x0000 }, /* R974 */
	{ 0x0000, 0x0000 }, /* R975 */
	{ 0x0000, 0x0000 }, /* R976 */
	{ 0x0000, 0x0000 }, /* R977 */
	{ 0x0000, 0x0000 }, /* R978 */
	{ 0x0000, 0x0000 }, /* R979 */
	{ 0x0000, 0x0000 }, /* R980 */
	{ 0x0000, 0x0000 }, /* R981 */
	{ 0x0000, 0x0000 }, /* R982 */
	{ 0x0000, 0x0000 }, /* R983 */
	{ 0x0000, 0x0000 }, /* R984 */
	{ 0x0000, 0x0000 }, /* R985 */
	{ 0x0000, 0x0000 }, /* R986 */
	{ 0x0000, 0x0000 }, /* R987 */
	{ 0x0000, 0x0000 }, /* R988 */
	{ 0x0000, 0x0000 }, /* R989 */
	{ 0x0000, 0x0000 }, /* R990 */
	{ 0x0000, 0x0000 }, /* R991 */
	{ 0x0000, 0x0000 }, /* R992 */
	{ 0x0000, 0x0000 }, /* R993 */
	{ 0x0000, 0x0000 }, /* R994 */
	{ 0x0000, 0x0000 }, /* R995 */
	{ 0x0000, 0x0000 }, /* R996 */
	{ 0x0000, 0x0000 }, /* R997 */
	{ 0x0000, 0x0000 }, /* R998 */
	{ 0x0000, 0x0000 }, /* R999 */
	{ 0x0000, 0x0000 }, /* R1000 */
	{ 0x0000, 0x0000 }, /* R1001 */
	{ 0x0000, 0x0000 }, /* R1002 */
	{ 0x0000, 0x0000 }, /* R1003 */
	{ 0x0000, 0x0000 }, /* R1004 */
	{ 0x0000, 0x0000 }, /* R1005 */
	{ 0x0000, 0x0000 }, /* R1006 */
	{ 0x0000, 0x0000 }, /* R1007 */
	{ 0x0000, 0x0000 }, /* R1008 */
	{ 0x0000, 0x0000 }, /* R1009 */
	{ 0x0000, 0x0000 }, /* R1010 */
	{ 0x0000, 0x0000 }, /* R1011 */
	{ 0x0000, 0x0000 }, /* R1012 */
	{ 0x0000, 0x0000 }, /* R1013 */
	{ 0x0000, 0x0000 }, /* R1014 */
	{ 0x0000, 0x0000 }, /* R1015 */
	{ 0x0000, 0x0000 }, /* R1016 */
	{ 0x0000, 0x0000 }, /* R1017 */
	{ 0x0000, 0x0000 }, /* R1018 */
	{ 0x0000, 0x0000 }, /* R1019 */
	{ 0x0000, 0x0000 }, /* R1020 */
	{ 0x0000, 0x0000 }, /* R1021 */
	{ 0x0000, 0x0000 }, /* R1022 */
	{ 0x0000, 0x0000 }, /* R1023 */
	{ 0x00FF, 0x01FF }, /* R1024  - AIF1 ADC1 Left Volume */
	{ 0x00FF, 0x01FF }, /* R1025  - AIF1 ADC1 Right Volume */
	{ 0x00FF, 0x01FF }, /* R1026  - AIF1 DAC1 Left Volume */
	{ 0x00FF, 0x01FF }, /* R1027  - AIF1 DAC1 Right Volume */
	{ 0x00FF, 0x01FF }, /* R1028  - AIF1 ADC2 Left Volume */
	{ 0x00FF, 0x01FF }, /* R1029  - AIF1 ADC2 Right Volume */
	{ 0x00FF, 0x01FF }, /* R1030  - AIF1 DAC2 Left Volume */
	{ 0x00FF, 0x01FF }, /* R1031  - AIF1 DAC2 Right Volume */
	{ 0x0000, 0x0000 }, /* R1032 */
	{ 0x0000, 0x0000 }, /* R1033 */
	{ 0x0000, 0x0000 }, /* R1034 */
	{ 0x0000, 0x0000 }, /* R1035 */
	{ 0x0000, 0x0000 }, /* R1036 */
	{ 0x0000, 0x0000 }, /* R1037 */
	{ 0x0000, 0x0000 }, /* R1038 */
	{ 0x0000, 0x0000 }, /* R1039 */
	{ 0xF800, 0xF800 }, /* R1040  - AIF1 ADC1 Filters */
	{ 0x7800, 0x7800 }, /* R1041  - AIF1 ADC2 Filters */
	{ 0x0000, 0x0000 }, /* R1042 */
	{ 0x0000, 0x0000 }, /* R1043 */
	{ 0x0000, 0x0000 }, /* R1044 */
	{ 0x0000, 0x0000 }, /* R1045 */
	{ 0x0000, 0x0000 }, /* R1046 */
	{ 0x0000, 0x0000 }, /* R1047 */
	{ 0x0000, 0x0000 }, /* R1048 */
	{ 0x0000, 0x0000 }, /* R1049 */
	{ 0x0000, 0x0000 }, /* R1050 */
	{ 0x0000, 0x0000 }, /* R1051 */
	{ 0x0000, 0x0000 }, /* R1052 */
	{ 0x0000, 0x0000 }, /* R1053 */
	{ 0x0000, 0x0000 }, /* R1054 */
	{ 0x0000, 0x0000 }, /* R1055 */
	{ 0x02B6, 0x02B6 }, /* R1056  - AIF1 DAC1 Filters (1) */
	{ 0x3F00, 0x3F00 }, /* R1057  - AIF1 DAC1 Filters (2) */
	{ 0x02B6, 0x02B6 }, /* R1058  - AIF1 DAC2 Filters (1) */
	{ 0x3F00, 0x3F00 }, /* R1059  - AIF1 DAC2 Filters (2) */
	{ 0x0000, 0x0000 }, /* R1060 */
	{ 0x0000, 0x0000 }, /* R1061 */
	{ 0x0000, 0x0000 }, /* R1062 */
	{ 0x0000, 0x0000 }, /* R1063 */
	{ 0x0000, 0x0000 }, /* R1064 */
	{ 0x0000, 0x0000 }, /* R1065 */
	{ 0x0000, 0x0000 }, /* R1066 */
	{ 0x0000, 0x0000 }, /* R1067 */
	{ 0x0000, 0x0000 }, /* R1068 */
	{ 0x0000, 0x0000 }, /* R1069 */
	{ 0x0000, 0x0000 }, /* R1070 */
	{ 0x0000, 0x0000 }, /* R1071 */
	{ 0x0000, 0x0000 }, /* R1072 */
	{ 0x0000, 0x0000 }, /* R1073 */
	{ 0x0000, 0x0000 }, /* R1074 */
	{ 0x0000, 0x0000 }, /* R1075 */
	{ 0x0000, 0x0000 }, /* R1076 */
	{ 0x0000, 0x0000 }, /* R1077 */
	{ 0x0000, 0x0000 }, /* R1078 */
	{ 0x0000, 0x0000 }, /* R1079 */
	{ 0x0000, 0x0000 }, /* R1080 */
	{ 0x0000, 0x0000 }, /* R1081 */
	{ 0x0000, 0x0000 }, /* R1082 */
	{ 0x0000, 0x0000 }, /* R1083 */
	{ 0x0000, 0x0000 }, /* R1084 */
	{ 0x0000, 0x0000 }, /* R1085 */
	{ 0x0000, 0x0000 }, /* R1086 */
	{ 0x0000, 0x0000 }, /* R1087 */
	{ 0xFFFF, 0xFFFF }, /* R1088  - AIF1 DRC1 (1) */
	{ 0x1FFF, 0x1FFF }, /* R1089  - AIF1 DRC1 (2) */
	{ 0xFFFF, 0xFFFF }, /* R1090  - AIF1 DRC1 (3) */
	{ 0x07FF, 0x07FF }, /* R1091  - AIF1 DRC1 (4) */
	{ 0x03FF, 0x03FF }, /* R1092  - AIF1 DRC1 (5) */
	{ 0x0000, 0x0000 }, /* R1093 */
	{ 0x0000, 0x0000 }, /* R1094 */
	{ 0x0000, 0x0000 }, /* R1095 */
	{ 0x0000, 0x0000 }, /* R1096 */
	{ 0x0000, 0x0000 }, /* R1097 */
	{ 0x0000, 0x0000 }, /* R1098 */
	{ 0x0000, 0x0000 }, /* R1099 */
	{ 0x0000, 0x0000 }, /* R1100 */
	{ 0x0000, 0x0000 }, /* R1101 */
	{ 0x0000, 0x0000 }, /* R1102 */
	{ 0x0000, 0x0000 }, /* R1103 */
	{ 0xFFFF, 0xFFFF }, /* R1104  - AIF1 DRC2 (1) */
	{ 0x1FFF, 0x1FFF }, /* R1105  - AIF1 DRC2 (2) */
	{ 0xFFFF, 0xFFFF }, /* R1106  - AIF1 DRC2 (3) */
	{ 0x07FF, 0x07FF }, /* R1107  - AIF1 DRC2 (4) */
	{ 0x03FF, 0x03FF }, /* R1108  - AIF1 DRC2 (5) */
	{ 0x0000, 0x0000 }, /* R1109 */
	{ 0x0000, 0x0000 }, /* R1110 */
	{ 0x0000, 0x0000 }, /* R1111 */
	{ 0x0000, 0x0000 }, /* R1112 */
	{ 0x0000, 0x0000 }, /* R1113 */
	{ 0x0000, 0x0000 }, /* R1114 */
	{ 0x0000, 0x0000 }, /* R1115 */
	{ 0x0000, 0x0000 }, /* R1116 */
	{ 0x0000, 0x0000 }, /* R1117 */
	{ 0x0000, 0x0000 }, /* R1118 */
	{ 0x0000, 0x0000 }, /* R1119 */
	{ 0x0000, 0x0000 }, /* R1120 */
	{ 0x0000, 0x0000 }, /* R1121 */
	{ 0x0000, 0x0000 }, /* R1122 */
	{ 0x0000, 0x0000 }, /* R1123 */
	{ 0x0000, 0x0000 }, /* R1124 */
	{ 0x0000, 0x0000 }, /* R1125 */
	{ 0x0000, 0x0000 }, /* R1126 */
	{ 0x0000, 0x0000 }, /* R1127 */
	{ 0x0000, 0x0000 }, /* R1128 */
	{ 0x0000, 0x0000 }, /* R1129 */
	{ 0x0000, 0x0000 }, /* R1130 */
	{ 0x0000, 0x0000 }, /* R1131 */
	{ 0x0000, 0x0000 }, /* R1132 */
	{ 0x0000, 0x0000 }, /* R1133 */
	{ 0x0000, 0x0000 }, /* R1134 */
	{ 0x0000, 0x0000 }, /* R1135 */
	{ 0x0000, 0x0000 }, /* R1136 */
	{ 0x0000, 0x0000 }, /* R1137 */
	{ 0x0000, 0x0000 }, /* R1138 */
	{ 0x0000, 0x0000 }, /* R1139 */
	{ 0x0000, 0x0000 }, /* R1140 */
	{ 0x0000, 0x0000 }, /* R1141 */
	{ 0x0000, 0x0000 }, /* R1142 */
	{ 0x0000, 0x0000 }, /* R1143 */
	{ 0x0000, 0x0000 }, /* R1144 */
	{ 0x0000, 0x0000 }, /* R1145 */
	{ 0x0000, 0x0000 }, /* R1146 */
	{ 0x0000, 0x0000 }, /* R1147 */
	{ 0x0000, 0x0000 }, /* R1148 */
	{ 0x0000, 0x0000 }, /* R1149 */
	{ 0x0000, 0x0000 }, /* R1150 */
	{ 0x0000, 0x0000 }, /* R1151 */
	{ 0xFFFF, 0xFFFF }, /* R1152  - AIF1 DAC1 EQ Gains (1) */
	{ 0xFFC0, 0xFFC0 }, /* R1153  - AIF1 DAC1 EQ Gains (2) */
	{ 0xFFFF, 0xFFFF }, /* R1154  - AIF1 DAC1 EQ Band 1 A */
	{ 0xFFFF, 0xFFFF }, /* R1155  - AIF1 DAC1 EQ Band 1 B */
	{ 0xFFFF, 0xFFFF }, /* R1156  - AIF1 DAC1 EQ Band 1 PG */
	{ 0xFFFF, 0xFFFF }, /* R1157  - AIF1 DAC1 EQ Band 2 A */
	{ 0xFFFF, 0xFFFF }, /* R1158  - AIF1 DAC1 EQ Band 2 B */
	{ 0xFFFF, 0xFFFF }, /* R1159  - AIF1 DAC1 EQ Band 2 C */
	{ 0xFFFF, 0xFFFF }, /* R1160  - AIF1 DAC1 EQ Band 2 PG */
	{ 0xFFFF, 0xFFFF }, /* R1161  - AIF1 DAC1 EQ Band 3 A */
	{ 0xFFFF, 0xFFFF }, /* R1162  - AIF1 DAC1 EQ Band 3 B */
	{ 0xFFFF, 0xFFFF }, /* R1163  - AIF1 DAC1 EQ Band 3 C */
	{ 0xFFFF, 0xFFFF }, /* R1164  - AIF1 DAC1 EQ Band 3 PG */
	{ 0xFFFF, 0xFFFF }, /* R1165  - AIF1 DAC1 EQ Band 4 A */
	{ 0xFFFF, 0xFFFF }, /* R1166  - AIF1 DAC1 EQ Band 4 B */
	{ 0xFFFF, 0xFFFF }, /* R1167  - AIF1 DAC1 EQ Band 4 C */
	{ 0xFFFF, 0xFFFF }, /* R1168  - AIF1 DAC1 EQ Band 4 PG */
	{ 0xFFFF, 0xFFFF }, /* R1169  - AIF1 DAC1 EQ Band 5 A */
	{ 0xFFFF, 0xFFFF }, /* R1170  - AIF1 DAC1 EQ Band 5 B */
	{ 0xFFFF, 0xFFFF }, /* R1171  - AIF1 DAC1 EQ Band 5 PG */
	{ 0x0000, 0x0000 }, /* R1172 */
	{ 0x0000, 0x0000 }, /* R1173 */
	{ 0x0000, 0x0000 }, /* R1174 */
	{ 0x0000, 0x0000 }, /* R1175 */
	{ 0x0000, 0x0000 }, /* R1176 */
	{ 0x0000, 0x0000 }, /* R1177 */
	{ 0x0000, 0x0000 }, /* R1178 */
	{ 0x0000, 0x0000 }, /* R1179 */
	{ 0x0000, 0x0000 }, /* R1180 */
	{ 0x0000, 0x0000 }, /* R1181 */
	{ 0x0000, 0x0000 }, /* R1182 */
	{ 0x0000, 0x0000 }, /* R1183 */
	{ 0xFFFF, 0xFFFF }, /* R1184  - AIF1 DAC2 EQ Gains (1) */
	{ 0xFFC0, 0xFFC0 }, /* R1185  - AIF1 DAC2 EQ Gains (2) */
	{ 0xFFFF, 0xFFFF }, /* R1186  - AIF1 DAC2 EQ Band 1 A */
	{ 0xFFFF, 0xFFFF }, /* R1187  - AIF1 DAC2 EQ Band 1 B */
	{ 0xFFFF, 0xFFFF }, /* R1188  - AIF1 DAC2 EQ Band 1 PG */
	{ 0xFFFF, 0xFFFF }, /* R1189  - AIF1 DAC2 EQ Band 2 A */
	{ 0xFFFF, 0xFFFF }, /* R1190  - AIF1 DAC2 EQ Band 2 B */
	{ 0xFFFF, 0xFFFF }, /* R1191  - AIF1 DAC2 EQ Band 2 C */
	{ 0xFFFF, 0xFFFF }, /* R1192  - AIF1 DAC2 EQ Band 2 PG */
	{ 0xFFFF, 0xFFFF }, /* R1193  - AIF1 DAC2 EQ Band 3 A */
	{ 0xFFFF, 0xFFFF }, /* R1194  - AIF1 DAC2 EQ Band 3 B */
	{ 0xFFFF, 0xFFFF }, /* R1195  - AIF1 DAC2 EQ Band 3 C */
	{ 0xFFFF, 0xFFFF }, /* R1196  - AIF1 DAC2 EQ Band 3 PG */
	{ 0xFFFF, 0xFFFF }, /* R1197  - AIF1 DAC2 EQ Band 4 A */
	{ 0xFFFF, 0xFFFF }, /* R1198  - AIF1 DAC2 EQ Band 4 B */
	{ 0xFFFF, 0xFFFF }, /* R1199  - AIF1 DAC2 EQ Band 4 C */
	{ 0xFFFF, 0xFFFF }, /* R1200  - AIF1 DAC2 EQ Band 4 PG */
	{ 0xFFFF, 0xFFFF }, /* R1201  - AIF1 DAC2 EQ Band 5 A */
	{ 0xFFFF, 0xFFFF }, /* R1202  - AIF1 DAC2 EQ Band 5 B */
	{ 0xFFFF, 0xFFFF }, /* R1203  - AIF1 DAC2 EQ Band 5 PG */
	{ 0x0000, 0x0000 }, /* R1204 */
	{ 0x0000, 0x0000 }, /* R1205 */
	{ 0x0000, 0x0000 }, /* R1206 */
	{ 0x0000, 0x0000 }, /* R1207 */
	{ 0x0000, 0x0000 }, /* R1208 */
	{ 0x0000, 0x0000 }, /* R1209 */
	{ 0x0000, 0x0000 }, /* R1210 */
	{ 0x0000, 0x0000 }, /* R1211 */
	{ 0x0000, 0x0000 }, /* R1212 */
	{ 0x0000, 0x0000 }, /* R1213 */
	{ 0x0000, 0x0000 }, /* R1214 */
	{ 0x0000, 0x0000 }, /* R1215 */
	{ 0x0000, 0x0000 }, /* R1216 */
	{ 0x0000, 0x0000 }, /* R1217 */
	{ 0x0000, 0x0000 }, /* R1218 */
	{ 0x0000, 0x0000 }, /* R1219 */
	{ 0x0000, 0x0000 }, /* R1220 */
	{ 0x0000, 0x0000 }, /* R1221 */
	{ 0x0000, 0x0000 }, /* R1222 */
	{ 0x0000, 0x0000 }, /* R1223 */
	{ 0x0000, 0x0000 }, /* R1224 */
	{ 0x0000, 0x0000 }, /* R1225 */
	{ 0x0000, 0x0000 }, /* R1226 */
	{ 0x0000, 0x0000 }, /* R1227 */
	{ 0x0000, 0x0000 }, /* R1228 */
	{ 0x0000, 0x0000 }, /* R1229 */
	{ 0x0000, 0x0000 }, /* R1230 */
	{ 0x0000, 0x0000 }, /* R1231 */
	{ 0x0000, 0x0000 }, /* R1232 */
	{ 0x0000, 0x0000 }, /* R1233 */
	{ 0x0000, 0x0000 }, /* R1234 */
	{ 0x0000, 0x0000 }, /* R1235 */
	{ 0x0000, 0x0000 }, /* R1236 */
	{ 0x0000, 0x0000 }, /* R1237 */
	{ 0x0000, 0x0000 }, /* R1238 */
	{ 0x0000, 0x0000 }, /* R1239 */
	{ 0x0000, 0x0000 }, /* R1240 */
	{ 0x0000, 0x0000 }, /* R1241 */
	{ 0x0000, 0x0000 }, /* R1242 */
	{ 0x0000, 0x0000 }, /* R1243 */
	{ 0x0000, 0x0000 }, /* R1244 */
	{ 0x0000, 0x0000 }, /* R1245 */
	{ 0x0000, 0x0000 }, /* R1246 */
	{ 0x0000, 0x0000 }, /* R1247 */
	{ 0x0000, 0x0000 }, /* R1248 */
	{ 0x0000, 0x0000 }, /* R1249 */
	{ 0x0000, 0x0000 }, /* R1250 */
	{ 0x0000, 0x0000 }, /* R1251 */
	{ 0x0000, 0x0000 }, /* R1252 */
	{ 0x0000, 0x0000 }, /* R1253 */
	{ 0x0000, 0x0000 }, /* R1254 */
	{ 0x0000, 0x0000 }, /* R1255 */
	{ 0x0000, 0x0000 }, /* R1256 */
	{ 0x0000, 0x0000 }, /* R1257 */
	{ 0x0000, 0x0000 }, /* R1258 */
	{ 0x0000, 0x0000 }, /* R1259 */
	{ 0x0000, 0x0000 }, /* R1260 */
	{ 0x0000, 0x0000 }, /* R1261 */
	{ 0x0000, 0x0000 }, /* R1262 */
	{ 0x0000, 0x0000 }, /* R1263 */
	{ 0x0000, 0x0000 }, /* R1264 */
	{ 0x0000, 0x0000 }, /* R1265 */
	{ 0x0000, 0x0000 }, /* R1266 */
	{ 0x0000, 0x0000 }, /* R1267 */
	{ 0x0000, 0x0000 }, /* R1268 */
	{ 0x0000, 0x0000 }, /* R1269 */
	{ 0x0000, 0x0000 }, /* R1270 */
	{ 0x0000, 0x0000 }, /* R1271 */
	{ 0x0000, 0x0000 }, /* R1272 */
	{ 0x0000, 0x0000 }, /* R1273 */
	{ 0x0000, 0x0000 }, /* R1274 */
	{ 0x0000, 0x0000 }, /* R1275 */
	{ 0x0000, 0x0000 }, /* R1276 */
	{ 0x0000, 0x0000 }, /* R1277 */
	{ 0x0000, 0x0000 }, /* R1278 */
	{ 0x0000, 0x0000 }, /* R1279 */
	{ 0x00FF, 0x01FF }, /* R1280  - AIF2 ADC Left Volume */
	{ 0x00FF, 0x01FF }, /* R1281  - AIF2 ADC Right Volume */
	{ 0x00FF, 0x01FF }, /* R1282  - AIF2 DAC Left Volume */
	{ 0x00FF, 0x01FF }, /* R1283  - AIF2 DAC Right Volume */
	{ 0x0000, 0x0000 }, /* R1284 */
	{ 0x0000, 0x0000 }, /* R1285 */
	{ 0x0000, 0x0000 }, /* R1286 */
	{ 0x0000, 0x0000 }, /* R1287 */
	{ 0x0000, 0x0000 }, /* R1288 */
	{ 0x0000, 0x0000 }, /* R1289 */
	{ 0x0000, 0x0000 }, /* R1290 */
	{ 0x0000, 0x0000 }, /* R1291 */
	{ 0x0000, 0x0000 }, /* R1292 */
	{ 0x0000, 0x0000 }, /* R1293 */
	{ 0x0000, 0x0000 }, /* R1294 */
	{ 0x0000, 0x0000 }, /* R1295 */
	{ 0xF800, 0xF800 }, /* R1296  - AIF2 ADC Filters */
	{ 0x0000, 0x0000 }, /* R1297 */
	{ 0x0000, 0x0000 }, /* R1298 */
	{ 0x0000, 0x0000 }, /* R1299 */
	{ 0x0000, 0x0000 }, /* R1300 */
	{ 0x0000, 0x0000 }, /* R1301 */
	{ 0x0000, 0x0000 }, /* R1302 */
	{ 0x0000, 0x0000 }, /* R1303 */
	{ 0x0000, 0x0000 }, /* R1304 */
	{ 0x0000, 0x0000 }, /* R1305 */
	{ 0x0000, 0x0000 }, /* R1306 */
	{ 0x0000, 0x0000 }, /* R1307 */
	{ 0x0000, 0x0000 }, /* R1308 */
	{ 0x0000, 0x0000 }, /* R1309 */
	{ 0x0000, 0x0000 }, /* R1310 */
	{ 0x0000, 0x0000 }, /* R1311 */
	{ 0x02B6, 0x02B6 }, /* R1312  - AIF2 DAC Filters (1) */
	{ 0x3F00, 0x3F00 }, /* R1313  - AIF2 DAC Filters (2) */
	{ 0x0000, 0x0000 }, /* R1314 */
	{ 0x0000, 0x0000 }, /* R1315 */
	{ 0x0000, 0x0000 }, /* R1316 */
	{ 0x0000, 0x0000 }, /* R1317 */
	{ 0x0000, 0x0000 }, /* R1318 */
	{ 0x0000, 0x0000 }, /* R1319 */
	{ 0x0000, 0x0000 }, /* R1320 */
	{ 0x0000, 0x0000 }, /* R1321 */
	{ 0x0000, 0x0000 }, /* R1322 */
	{ 0x0000, 0x0000 }, /* R1323 */
	{ 0x0000, 0x0000 }, /* R1324 */
	{ 0x0000, 0x0000 }, /* R1325 */
	{ 0x0000, 0x0000 }, /* R1326 */
	{ 0x0000, 0x0000 }, /* R1327 */
	{ 0x0000, 0x0000 }, /* R1328 */
	{ 0x0000, 0x0000 }, /* R1329 */
	{ 0x0000, 0x0000 }, /* R1330 */
	{ 0x0000, 0x0000 }, /* R1331 */
	{ 0x0000, 0x0000 }, /* R1332 */
	{ 0x0000, 0x0000 }, /* R1333 */
	{ 0x0000, 0x0000 }, /* R1334 */
	{ 0x0000, 0x0000 }, /* R1335 */
	{ 0x0000, 0x0000 }, /* R1336 */
	{ 0x0000, 0x0000 }, /* R1337 */
	{ 0x0000, 0x0000 }, /* R1338 */
	{ 0x0000, 0x0000 }, /* R1339 */
	{ 0x0000, 0x0000 }, /* R1340 */
	{ 0x0000, 0x0000 }, /* R1341 */
	{ 0x0000, 0x0000 }, /* R1342 */
	{ 0x0000, 0x0000 }, /* R1343 */
	{ 0xFFFF, 0xFFFF }, /* R1344  - AIF2 DRC (1) */
	{ 0x1FFF, 0x1FFF }, /* R1345  - AIF2 DRC (2) */
	{ 0xFFFF, 0xFFFF }, /* R1346  - AIF2 DRC (3) */
	{ 0x07FF, 0x07FF }, /* R1347  - AIF2 DRC (4) */
	{ 0x03FF, 0x03FF }, /* R1348  - AIF2 DRC (5) */
	{ 0x0000, 0x0000 }, /* R1349 */
	{ 0x0000, 0x0000 }, /* R1350 */
	{ 0x0000, 0x0000 }, /* R1351 */
	{ 0x0000, 0x0000 }, /* R1352 */
	{ 0x0000, 0x0000 }, /* R1353 */
	{ 0x0000, 0x0000 }, /* R1354 */
	{ 0x0000, 0x0000 }, /* R1355 */
	{ 0x0000, 0x0000 }, /* R1356 */
	{ 0x0000, 0x0000 }, /* R1357 */
	{ 0x0000, 0x0000 }, /* R1358 */
	{ 0x0000, 0x0000 }, /* R1359 */
	{ 0x0000, 0x0000 }, /* R1360 */
	{ 0x0000, 0x0000 }, /* R1361 */
	{ 0x0000, 0x0000 }, /* R1362 */
	{ 0x0000, 0x0000 }, /* R1363 */
	{ 0x0000, 0x0000 }, /* R1364 */
	{ 0x0000, 0x0000 }, /* R1365 */
	{ 0x0000, 0x0000 }, /* R1366 */
	{ 0x0000, 0x0000 }, /* R1367 */
	{ 0x0000, 0x0000 }, /* R1368 */
	{ 0x0000, 0x0000 }, /* R1369 */
	{ 0x0000, 0x0000 }, /* R1370 */
	{ 0x0000, 0x0000 }, /* R1371 */
	{ 0x0000, 0x0000 }, /* R1372 */
	{ 0x0000, 0x0000 }, /* R1373 */
	{ 0x0000, 0x0000 }, /* R1374 */
	{ 0x0000, 0x0000 }, /* R1375 */
	{ 0x0000, 0x0000 }, /* R1376 */
	{ 0x0000, 0x0000 }, /* R1377 */
	{ 0x0000, 0x0000 }, /* R1378 */
	{ 0x0000, 0x0000 }, /* R1379 */
	{ 0x0000, 0x0000 }, /* R1380 */
	{ 0x0000, 0x0000 }, /* R1381 */
	{ 0x0000, 0x0000 }, /* R1382 */
	{ 0x0000, 0x0000 }, /* R1383 */
	{ 0x0000, 0x0000 }, /* R1384 */
	{ 0x0000, 0x0000 }, /* R1385 */
	{ 0x0000, 0x0000 }, /* R1386 */
	{ 0x0000, 0x0000 }, /* R1387 */
	{ 0x0000, 0x0000 }, /* R1388 */
	{ 0x0000, 0x0000 }, /* R1389 */
	{ 0x0000, 0x0000 }, /* R1390 */
	{ 0x0000, 0x0000 }, /* R1391 */
	{ 0x0000, 0x0000 }, /* R1392 */
	{ 0x0000, 0x0000 }, /* R1393 */
	{ 0x0000, 0x0000 }, /* R1394 */
	{ 0x0000, 0x0000 }, /* R1395 */
	{ 0x0000, 0x0000 }, /* R1396 */
	{ 0x0000, 0x0000 }, /* R1397 */
	{ 0x0000, 0x0000 }, /* R1398 */
	{ 0x0000, 0x0000 }, /* R1399 */
	{ 0x0000, 0x0000 }, /* R1400 */
	{ 0x0000, 0x0000 }, /* R1401 */
	{ 0x0000, 0x0000 }, /* R1402 */
	{ 0x0000, 0x0000 }, /* R1403 */
	{ 0x0000, 0x0000 }, /* R1404 */
	{ 0x0000, 0x0000 }, /* R1405 */
	{ 0x0000, 0x0000 }, /* R1406 */
	{ 0x0000, 0x0000 }, /* R1407 */
	{ 0xFFFF, 0xFFFF }, /* R1408  - AIF2 EQ Gains (1) */
	{ 0xFFC0, 0xFFC0 }, /* R1409  - AIF2 EQ Gains (2) */
	{ 0xFFFF, 0xFFFF }, /* R1410  - AIF2 EQ Band 1 A */
	{ 0xFFFF, 0xFFFF }, /* R1411  - AIF2 EQ Band 1 B */
	{ 0xFFFF, 0xFFFF }, /* R1412  - AIF2 EQ Band 1 PG */
	{ 0xFFFF, 0xFFFF }, /* R1413  - AIF2 EQ Band 2 A */
	{ 0xFFFF, 0xFFFF }, /* R1414  - AIF2 EQ Band 2 B */
	{ 0xFFFF, 0xFFFF }, /* R1415  - AIF2 EQ Band 2 C */
	{ 0xFFFF, 0xFFFF }, /* R1416  - AIF2 EQ Band 2 PG */
	{ 0xFFFF, 0xFFFF }, /* R1417  - AIF2 EQ Band 3 A */
	{ 0xFFFF, 0xFFFF }, /* R1418  - AIF2 EQ Band 3 B */
	{ 0xFFFF, 0xFFFF }, /* R1419  - AIF2 EQ Band 3 C */
	{ 0xFFFF, 0xFFFF }, /* R1420  - AIF2 EQ Band 3 PG */
	{ 0xFFFF, 0xFFFF }, /* R1421  - AIF2 EQ Band 4 A */
	{ 0xFFFF, 0xFFFF }, /* R1422  - AIF2 EQ Band 4 B */
	{ 0xFFFF, 0xFFFF }, /* R1423  - AIF2 EQ Band 4 C */
	{ 0xFFFF, 0xFFFF }, /* R1424  - AIF2 EQ Band 4 PG */
	{ 0xFFFF, 0xFFFF }, /* R1425  - AIF2 EQ Band 5 A */
	{ 0xFFFF, 0xFFFF }, /* R1426  - AIF2 EQ Band 5 B */
	{ 0xFFFF, 0xFFFF }, /* R1427  - AIF2 EQ Band 5 PG */
	{ 0x0000, 0x0000 }, /* R1428 */
	{ 0x0000, 0x0000 }, /* R1429 */
	{ 0x0000, 0x0000 }, /* R1430 */
	{ 0x0000, 0x0000 }, /* R1431 */
	{ 0x0000, 0x0000 }, /* R1432 */
	{ 0x0000, 0x0000 }, /* R1433 */
	{ 0x0000, 0x0000 }, /* R1434 */
	{ 0x0000, 0x0000 }, /* R1435 */
	{ 0x0000, 0x0000 }, /* R1436 */
	{ 0x0000, 0x0000 }, /* R1437 */
	{ 0x0000, 0x0000 }, /* R1438 */
	{ 0x0000, 0x0000 }, /* R1439 */
	{ 0x0000, 0x0000 }, /* R1440 */
	{ 0x0000, 0x0000 }, /* R1441 */
	{ 0x0000, 0x0000 }, /* R1442 */
	{ 0x0000, 0x0000 }, /* R1443 */
	{ 0x0000, 0x0000 }, /* R1444 */
	{ 0x0000, 0x0000 }, /* R1445 */
	{ 0x0000, 0x0000 }, /* R1446 */
	{ 0x0000, 0x0000 }, /* R1447 */
	{ 0x0000, 0x0000 }, /* R1448 */
	{ 0x0000, 0x0000 }, /* R1449 */
	{ 0x0000, 0x0000 }, /* R1450 */
	{ 0x0000, 0x0000 }, /* R1451 */
	{ 0x0000, 0x0000 }, /* R1452 */
	{ 0x0000, 0x0000 }, /* R1453 */
	{ 0x0000, 0x0000 }, /* R1454 */
	{ 0x0000, 0x0000 }, /* R1455 */
	{ 0x0000, 0x0000 }, /* R1456 */
	{ 0x0000, 0x0000 }, /* R1457 */
	{ 0x0000, 0x0000 }, /* R1458 */
	{ 0x0000, 0x0000 }, /* R1459 */
	{ 0x0000, 0x0000 }, /* R1460 */
	{ 0x0000, 0x0000 }, /* R1461 */
	{ 0x0000, 0x0000 }, /* R1462 */
	{ 0x0000, 0x0000 }, /* R1463 */
	{ 0x0000, 0x0000 }, /* R1464 */
	{ 0x0000, 0x0000 }, /* R1465 */
	{ 0x0000, 0x0000 }, /* R1466 */
	{ 0x0000, 0x0000 }, /* R1467 */
	{ 0x0000, 0x0000 }, /* R1468 */
	{ 0x0000, 0x0000 }, /* R1469 */
	{ 0x0000, 0x0000 }, /* R1470 */
	{ 0x0000, 0x0000 }, /* R1471 */
	{ 0x0000, 0x0000 }, /* R1472 */
	{ 0x0000, 0x0000 }, /* R1473 */
	{ 0x0000, 0x0000 }, /* R1474 */
	{ 0x0000, 0x0000 }, /* R1475 */
	{ 0x0000, 0x0000 }, /* R1476 */
	{ 0x0000, 0x0000 }, /* R1477 */
	{ 0x0000, 0x0000 }, /* R1478 */
	{ 0x0000, 0x0000 }, /* R1479 */
	{ 0x0000, 0x0000 }, /* R1480 */
	{ 0x0000, 0x0000 }, /* R1481 */
	{ 0x0000, 0x0000 }, /* R1482 */
	{ 0x0000, 0x0000 }, /* R1483 */
	{ 0x0000, 0x0000 }, /* R1484 */
	{ 0x0000, 0x0000 }, /* R1485 */
	{ 0x0000, 0x0000 }, /* R1486 */
	{ 0x0000, 0x0000 }, /* R1487 */
	{ 0x0000, 0x0000 }, /* R1488 */
	{ 0x0000, 0x0000 }, /* R1489 */
	{ 0x0000, 0x0000 }, /* R1490 */
	{ 0x0000, 0x0000 }, /* R1491 */
	{ 0x0000, 0x0000 }, /* R1492 */
	{ 0x0000, 0x0000 }, /* R1493 */
	{ 0x0000, 0x0000 }, /* R1494 */
	{ 0x0000, 0x0000 }, /* R1495 */
	{ 0x0000, 0x0000 }, /* R1496 */
	{ 0x0000, 0x0000 }, /* R1497 */
	{ 0x0000, 0x0000 }, /* R1498 */
	{ 0x0000, 0x0000 }, /* R1499 */
	{ 0x0000, 0x0000 }, /* R1500 */
	{ 0x0000, 0x0000 }, /* R1501 */
	{ 0x0000, 0x0000 }, /* R1502 */
	{ 0x0000, 0x0000 }, /* R1503 */
	{ 0x0000, 0x0000 }, /* R1504 */
	{ 0x0000, 0x0000 }, /* R1505 */
	{ 0x0000, 0x0000 }, /* R1506 */
	{ 0x0000, 0x0000 }, /* R1507 */
	{ 0x0000, 0x0000 }, /* R1508 */
	{ 0x0000, 0x0000 }, /* R1509 */
	{ 0x0000, 0x0000 }, /* R1510 */
	{ 0x0000, 0x0000 }, /* R1511 */
	{ 0x0000, 0x0000 }, /* R1512 */
	{ 0x0000, 0x0000 }, /* R1513 */
	{ 0x0000, 0x0000 }, /* R1514 */
	{ 0x0000, 0x0000 }, /* R1515 */
	{ 0x0000, 0x0000 }, /* R1516 */
	{ 0x0000, 0x0000 }, /* R1517 */
	{ 0x0000, 0x0000 }, /* R1518 */
	{ 0x0000, 0x0000 }, /* R1519 */
	{ 0x0000, 0x0000 }, /* R1520 */
	{ 0x0000, 0x0000 }, /* R1521 */
	{ 0x0000, 0x0000 }, /* R1522 */
	{ 0x0000, 0x0000 }, /* R1523 */
	{ 0x0000, 0x0000 }, /* R1524 */
	{ 0x0000, 0x0000 }, /* R1525 */
	{ 0x0000, 0x0000 }, /* R1526 */
	{ 0x0000, 0x0000 }, /* R1527 */
	{ 0x0000, 0x0000 }, /* R1528 */
	{ 0x0000, 0x0000 }, /* R1529 */
	{ 0x0000, 0x0000 }, /* R1530 */
	{ 0x0000, 0x0000 }, /* R1531 */
	{ 0x0000, 0x0000 }, /* R1532 */
	{ 0x0000, 0x0000 }, /* R1533 */
	{ 0x0000, 0x0000 }, /* R1534 */
	{ 0x0000, 0x0000 }, /* R1535 */
	{ 0x01EF, 0x01EF }, /* R1536  - DAC1 Mixer Volumes */
	{ 0x0037, 0x0037 }, /* R1537  - DAC1 Left Mixer Routing */
	{ 0x0037, 0x0037 }, /* R1538  - DAC1 Right Mixer Routing */
	{ 0x01EF, 0x01EF }, /* R1539  - DAC2 Mixer Volumes */
	{ 0x0037, 0x0037 }, /* R1540  - DAC2 Left Mixer Routing */
	{ 0x0037, 0x0037 }, /* R1541  - DAC2 Right Mixer Routing */
	{ 0x0003, 0x0003 }, /* R1542  - AIF1 ADC1 Left Mixer Routing */
	{ 0x0003, 0x0003 }, /* R1543  - AIF1 ADC1 Right Mixer Routing */
	{ 0x0003, 0x0003 }, /* R1544  - AIF1 ADC2 Left Mixer Routing */
	{ 0x0003, 0x0003 }, /* R1545  - AIF1 ADC2 Right mixer Routing */
	{ 0x0000, 0x0000 }, /* R1546 */
	{ 0x0000, 0x0000 }, /* R1547 */
	{ 0x0000, 0x0000 }, /* R1548 */
	{ 0x0000, 0x0000 }, /* R1549 */
	{ 0x0000, 0x0000 }, /* R1550 */
	{ 0x0000, 0x0000 }, /* R1551 */
	{ 0x02FF, 0x03FF }, /* R1552  - DAC1 Left Volume */
	{ 0x02FF, 0x03FF }, /* R1553  - DAC1 Right Volume */
	{ 0x02FF, 0x03FF }, /* R1554  - DAC2 Left Volume */
	{ 0x02FF, 0x03FF }, /* R1555  - DAC2 Right Volume */
	{ 0x0003, 0x0003 }, /* R1556  - DAC Softmute */
	{ 0x0000, 0x0000 }, /* R1557 */
	{ 0x0000, 0x0000 }, /* R1558 */
	{ 0x0000, 0x0000 }, /* R1559 */
	{ 0x0000, 0x0000 }, /* R1560 */
	{ 0x0000, 0x0000 }, /* R1561 */
	{ 0x0000, 0x0000 }, /* R1562 */
	{ 0x0000, 0x0000 }, /* R1563 */
	{ 0x0000, 0x0000 }, /* R1564 */
	{ 0x0000, 0x0000 }, /* R1565 */
	{ 0x0000, 0x0000 }, /* R1566 */
	{ 0x0000, 0x0000 }, /* R1567 */
	{ 0x0003, 0x0003 }, /* R1568  - Oversampling */
	{ 0x03C3, 0x03C3 }, /* R1569  - Sidetone */
};

static int wm8994_readable_register(unsigned int reg)
{
	switch (reg) {
	case WM8994_GPIO_1:
	case WM8994_GPIO_2:
	case WM8994_GPIO_3:
	case WM8994_GPIO_4:
	case WM8994_GPIO_5:
	case WM8994_GPIO_6:
	case WM8994_GPIO_7:
	case WM8994_GPIO_8:
	case WM8994_GPIO_9:
	case WM8994_GPIO_10:
	case WM8994_GPIO_11:
	case WM8994_INTERRUPT_STATUS_1:
	case WM8994_INTERRUPT_STATUS_2:
		return 1;
	default:
		break;
	}

	if (reg >= ARRAY_SIZE(access_masks))
		return 0;
	return access_masks[reg].readable != 0;
}

/*
 * initialise the WM8994 driver
 * register the mixer and dsp interfaces with the kernel
 */
static int wm8994_init(struct wm8994_priv *wm8994_private,
		       struct wm8994_platform_data *pdata)
{
	struct snd_soc_codec *codec = &wm8994_private->codec;
	struct wm8994_priv *wm8994;
	int ret = 0;
	DEBUG_LOG("");
	codec->drvdata = kzalloc(sizeof(struct wm8994_priv), GFP_KERNEL);
	if (codec->drvdata == NULL)
		return -ENOMEM;

	wm8994 = codec->drvdata;

	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);
	codec->name = "WM8994";
	codec->owner = THIS_MODULE;
	codec->read = wm8994_read;
	codec->write = wm8994_write;
	codec->readable_register = wm8994_readable_register;
	codec->reg_cache_size = WM8994_IRQ_POLARITY; /* Skip write sequencer */
	codec->set_bias_level = NULL;
	codec->dai = &wm8994_dai;
	codec->num_dai = 1;
	wm8994->universal_playback_path = universal_wm8994_playback_paths;
	wm8994->universal_voicecall_path = universal_wm8994_voicecall_paths;
	wm8994->universal_mic_path = universal_wm8994_mic_paths;
	wm8994->universal_clock_control = universal_clock_controls;
	wm8994->stream_state = PCM_STREAM_DEACTIVE;
	wm8994->cur_path = OFF;
	wm8994->rec_path = MIC_OFF;
	wm8994->power_state = CODEC_OFF;
	wm8994->input_source = DEFAULT;
	wm8994->ringtone_active = RING_OFF;
	wm8994->pdata = pdata;

	wm8994->gain_code = gain_code_check();

	wm8994->codec_clk = clk_get(NULL, "usb_osc");

	wm8994->universal_clock_control(codec, CODEC_ON);

	if (IS_ERR(wm8994->codec_clk)) {
		pr_err("failed to get MCLK clock from AP\n");
		ret = PTR_ERR(wm8994->codec_clk);
		goto card_err;
	}

	wm8994_write(codec, WM8994_SOFTWARE_RESET, 0x0000);

	wm8994->hw_version = wm8994_read(codec, 0x100);

	wm8994_socdev->card->codec = codec;
	wm8994_codec = codec;

	ret = snd_soc_new_pcms(wm8994_socdev, SNDRV_DEFAULT_IDX1,
			       SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		DEBUG_LOG_ERR("failed to create pcms\n");
		goto pcm_err;
	}

	wm8994_add_controls(codec);
	wm8994_add_widgets(codec);

	return ret;

card_err:
	snd_soc_free_pcms(wm8994_socdev);
	snd_soc_dapm_free(wm8994_socdev);
pcm_err:

	return ret;
}

/* If the i2c layer weren't so broken, we could pass this kind of data
   around */

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)


static void *control_data1;

static int wm8994_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct snd_soc_codec *codec;
	struct wm8994_priv *wm8994_priv;
	int ret = -ENODEV;
	struct wm8994_platform_data *pdata;

	DEBUG_LOG("");

	wm8994_priv = kzalloc(sizeof(struct wm8994_priv), GFP_KERNEL);
	if (wm8994_priv == NULL)
		return -ENOMEM;

	codec = &wm8994_priv->codec;
#ifdef PM_DEBUG
	pm_codec = codec;
#endif

	pdata = i2c->dev.platform_data;

	if (!pdata) {
		dev_err(&i2c->dev, "failed to initialize WM8994\n");
		goto err_bad_pdata;
	}

	if (!pdata->set_mic_bias) {
		dev_err(&i2c->dev, "bad pdata WM8994\n");
		goto err_bad_pdata;
	}

	/* CODEC LDO SETTING */
	if (gpio_is_valid(pdata->ldo)) {
		ret = gpio_request(pdata->ldo, "WM8994 LDO");
		if (ret) {
			pr_err("Failed to request CODEC_LDO_EN!\n");
			goto err_ldo;
		}
		gpio_direction_output(pdata->ldo, 0);
	}

	s3c_gpio_setpull(pdata->ldo, S3C_GPIO_PULL_NONE);

	/* For preserving output of codec related pins */
	s3c_gpio_slp_cfgpin(pdata->ldo, S3C_GPIO_SLP_PREV);
	s3c_gpio_slp_setpull_updown(pdata->ldo, S3C_GPIO_PULL_NONE);

	/* EAR_SEL SETTING(only crespo HW) */
	if (gpio_is_valid(pdata->ear_sel)) {
		ret = gpio_request(pdata->ear_sel, "EAR SEL");
		if (ret) {
			pr_err("Failed to request EAR_SEL!\n");
			goto err_earsel;
		}
		gpio_direction_output(pdata->ear_sel, 0);
	}
	s3c_gpio_setpull(pdata->ear_sel, S3C_GPIO_PULL_NONE);

	s3c_gpio_slp_cfgpin(pdata->ear_sel, S3C_GPIO_SLP_PREV);
	s3c_gpio_slp_setpull_updown(pdata->ear_sel, S3C_GPIO_PULL_NONE);

	wm8994_ldo_control(pdata, 1);

	codec->hw_write = (hw_write_t) i2c_master_send;
	i2c_set_clientdata(i2c, wm8994_priv);
	codec->control_data = i2c;
	codec->dev = &i2c->dev;
	control_data1 = i2c;

	ret = wm8994_init(wm8994_priv, pdata);
	if (ret) {
		dev_err(&i2c->dev, "failed to initialize WM8994\n");
		goto err_init;
	}

	return ret;

err_init:
	gpio_free(pdata->ear_sel);
err_earsel:
	gpio_free(pdata->ldo);
err_ldo:
err_bad_pdata:
	kfree(wm8994_priv);
	return ret;
}

static int wm8994_i2c_remove(struct i2c_client *client)
{
	struct wm8994_priv *wm8994_priv = i2c_get_clientdata(client);

	gpio_free(wm8994_priv->pdata->ldo);
	gpio_free(wm8994_priv->pdata->ear_sel);

	kfree(wm8994_priv);
	return 0;
}

static const struct i2c_device_id wm8994_i2c_id[] = {
	{"wm8994", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, wm8994_i2c_id);

static struct i2c_driver wm8994_i2c_driver = {
	.driver = {
		   .name = "WM8994 I2C Codec",
		   .owner = THIS_MODULE,
		   },
	.probe = wm8994_i2c_probe,
	.remove = wm8994_i2c_remove,
	.id_table = wm8994_i2c_id,
};

static int wm8994_add_i2c_device(struct platform_device *pdev,
				 const struct wm8994_setup_data *setup)
{
	int ret;

	ret = i2c_add_driver(&wm8994_i2c_driver);
	if (ret != 0) {
		dev_err(&pdev->dev, "can't add i2c driver\n");
		return ret;
	}

	return 0;

err_driver:
	i2c_del_driver(&wm8994_i2c_driver);
	return -ENODEV;
}
#endif

static int wm8994_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct wm8994_setup_data *setup;

	int ret = 0;

	pr_info("WM8994 Audio Codec %s\n", WM8994_VERSION);

	setup = socdev->codec_data;
	wm8994_socdev = socdev;
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	if (setup->i2c_address)
		ret = wm8994_add_i2c_device(pdev, setup);
#else
	/* Add other interfaces here */
#endif
	return ret;
}

/* power down chip */
static int wm8994_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = wm8994_codec;

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_unregister_device(codec->control_data);
	i2c_del_driver(&wm8994_i2c_driver);
#endif

	kfree(codec->drvdata);

	return 0;
}

#ifdef CONFIG_PM
static int wm8994_suspend(struct platform_device *pdev, pm_message_t msg)
{
	struct snd_soc_codec *codec = wm8994_codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("Codec State = [0x%X], Stream State = [0x%X]",
			wm8994->codec_state, wm8994->stream_state);

	if (wm8994->codec_state == DEACTIVE &&
		wm8994->stream_state == PCM_STREAM_DEACTIVE) {
		wm8994->power_state = CODEC_OFF;
		wm8994_write(codec, WM8994_SOFTWARE_RESET, 0x0000);
		wm8994_ldo_control(wm8994->pdata, 0);
		wm8994->universal_clock_control(codec, CODEC_OFF);
	}

	return 0;
}

static int wm8994_resume(struct platform_device *pdev)
{
	struct snd_soc_codec *codec = wm8994_codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("%s..", __func__);
	DEBUG_LOG_ERR("------WM8994 Revision = [%d]-------",
		      wm8994->hw_version);

	if (wm8994->power_state == CODEC_OFF) {
		/* Turn on sequence by recommend Wolfson.*/
		wm8994_ldo_control(wm8994->pdata, 1);
		wm8994->universal_clock_control(codec, CODEC_ON);
	}
	return 0;
}
#endif

struct snd_soc_codec_device soc_codec_dev_wm8994 = {
	.probe = wm8994_probe,
	.remove = wm8994_remove,
#ifdef CONFIG_PM
	.suspend = wm8994_suspend,
	.resume = wm8994_resume,
#endif
};

static int __init wm8994_modinit(void)
{
	int ret;
	ret = snd_soc_register_dai(&wm8994_dai);
	if (ret)
		pr_err("..dai registration failed..\n");

	return ret;
}

module_init(wm8994_modinit);

static void __exit wm8994_exit(void)
{
	snd_soc_unregister_dai(&wm8994_dai);
}

module_exit(wm8994_exit);

MODULE_DESCRIPTION("ASoC WM8994 driver");
MODULE_AUTHOR("Shaju Abraham shaju.abraham@samsung.com");
MODULE_LICENSE("GPL");
