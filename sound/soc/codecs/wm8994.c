/*
 * wm8994.c  --  WM8994 ALSA Soc Audio driver
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
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <asm/div64.h>
#include <asm/io.h>
#include <plat/map-base.h>
#include <mach/regs-clock.h> 
#include "wm8994.h"

#define WM8994_VERSION "0.1"
#define SUBJECT "wm8994.c"

#if defined(CONFIG_VIDEO_TV20) && defined(CONFIG_SND_S5P_WM8994_MASTER) 
#define HDMI_USE_AUDIO
#endif


//------------------------------------------------
// Definitions of clock related.
//------------------------------------------------
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
	int div; /* *10 due to .5s */
	int bclk_div;
} bclk_divs[] = {
#if 0
	{ 10,  0  },
	{ 15,  1  },
	{ 20,  2  },
	{ 30,  3  },
	{ 40,  4  },
	{ 55,  5  },
	{ 60,  6  },
	{ 80,  7  },
	{ 110, 8  },
	{ 120, 9  },
	{ 160, 10 },
	{ 220, 11 },
	{ 240, 12 },
	{ 320, 13 },
	{ 440, 14 },
	{ 480, 15 },
#endif
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

extern flush_fifo(void);//I2S FIFO-flush function . Implemented as to match TN requirement to remove ringtone noise during call.

#if defined ATTACH_ADDITINAL_PCM_DRIVER
int vtCallActive = 0;
#endif
//------------------------------------------------
// Definitions of sound path
//------------------------------------------------
select_route universal_wm8994_playback_paths[] = 
	{wm8994_set_off, wm8994_set_playback_receiver,
	wm8994_set_playback_speaker, wm8994_set_playback_headset, wm8994_set_off, wm8994_set_playback_speaker_headset};

select_route universal_wm8994_voicecall_paths[] = 
	{wm8994_set_off, wm8994_set_voicecall_receiver, 
	wm8994_set_voicecall_speaker, wm8994_set_voicecall_headset, wm8994_set_voicecall_bluetooth}; 

select_mic_route universal_wm8994_mic_paths[] = {wm8994_record_main_mic,wm8994_record_headset_mic};


//------------------------------------------------
// Implementation of I2C functions
//------------------------------------------------
static unsigned int wm8994_read_hw(struct snd_soc_codec *codec, u16 reg)
{
	struct i2c_msg xfer[2];
	u16 data;
	int ret;
	struct i2c_client *i2c = codec->control_data;

	data = ((reg & 0xff00) >> 8) | ((reg & 0xff) << 8);
	
	/* Write register */
	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;  //1
	//xfer[0].buf = &reg;
	xfer[0].buf = (void *)&data;

	/* Read data */
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

int wm8994_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	u8 data[4];
	int ret;
	//BUG_ON(reg > WM8993_MAX_REGISTER);

	/* data is
	 *   D15..D9 WM8993 register offset
	 *   D8...D0 register data
	 */
	data[0] = (reg & 0xff00 ) >> 8;
	data[1] = reg & 0x00ff;
	data[2] = value >> 8;
	data[3] = value & 0x00ff;
	ret = codec->hw_write(codec->control_data, data, 4);

	if (ret == 4)
		return 0;
	//if (ret < 0)
	else{ 
		printk("i2c write problem occured\n");
		return ret;
	}
#if 0
	struct i2c_msg wmsg;
	unsigned char data[100];
	int ret,i;
	struct i2c_client *i2c_client = codec->control_data;
	
	wmsg.addr = i2c_client->addr;
	wmsg.flags = 0;
	wmsg.len = 4;
	wmsg.buf = data;
	
	data[0] = reg & 0x00ff;
	data[1] = (reg & 0xff00 ) >> 8;
	data[2] = value & 0x00ff;
	data[3] = value >> 8;
	
	ret = i2c_transfer(i2c_client->adapter,&wmsg,1);
	if(ret!=1)
	     printk("wm8994 i2c write problem occured\n");

	return ret;	

	return -EIO;
#endif
}

inline unsigned int wm8994_read(struct snd_soc_codec *codec, unsigned int reg)
{
	return wm8994_read_hw(codec, reg); 
}

//------------------------------------------------
// Functions related volume.
//------------------------------------------------
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

	DEBUG_LOG("");

        ret = snd_soc_put_volsw_2r(kcontrol, ucontrol);
        if (ret < 0)
                return ret;

        /* now hit the volume update bits (always bit 8) */
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

	/* now hit the volume update bits (always bit 8) */
        val = wm8994_read(codec, reg);

        return wm8994_write(codec, reg, val | 0x0100);

}


//------------------------------------------------
// Implementation of sound path
//------------------------------------------------
#define MAX_PLAYBACK_PATHS 5
#define MAX_VOICECALL_PATH 4
static const char *playback_path[] = { "OFF", "RCV", "SPK", "HP", "BT", "SPK_HP"};
static const char *voicecall_path[] = { "OFF", "RCV", "SPK", "HP", "BT", };
static const char *fmradio_path[] = { "FMR_OFF", "FMR_SPK", "FMR_HP", "FMR_SPK_MIX", "FMR_HP_MIX", "FMR_SPK_HP_MIX"};
static const char *mic_path[] = { "Main Mic", "Hands Free Mic", };
static const char *codec_tuning_control[] = {"OFF", "ON"};
static const char *mic_state[] = {"MIC_NO_USE", "MIC_USE"};

static int wm8994_get_mic_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	DEBUG_LOG("");
#if 0
	ucontrol->value.integer.value[0] = wm8994_mic_path;
	return 0;
#endif
	return 0;
}

static int wm8994_set_mic_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("");
			
	if (ucontrol->value.integer.value[0] == 0) // MAIN MIC
                wm8994->rec_path = MAIN;
	else if (ucontrol->value.integer.value[0] == 1) // SUB MIC
                wm8994->rec_path = SUB;
	else
		return -EINVAL;
	
	audio_ctrl_mic_bias_gpio(1);
	 wm8994->universal_mic_path[wm8994->rec_path ](codec);
	return 0;
}

static int wm8994_get_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	DEBUG_LOG("");
#if 0
	int i = 0;

	while(audio_path[i] != NULL) {
		if(!strcmp(audio_path[i], kcontrol->id.name) && ((wm8994_path >> 4) == i)) {
			ucontrol->value.integer.value[0] = wm8994_path & 0xf;
			break;
		}
		i++;
	}
#endif
	return 0;
}

static int wm8994_set_path(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;
	struct soc_enum *mc =
                (struct soc_enum *)kcontrol->private_value;

	// Get path value
	int path_num = ucontrol->value.integer.value[0];

	if(strcmp(mc->texts[path_num],playback_path[path_num]) )
	{		
		DEBUG_LOG_ERR("Unknown path %s\n", mc->texts[path_num] );		
		return -ENODEV;
	}

	if(path_num > MAX_PLAYBACK_PATHS )
	{
		DEBUG_LOG_ERR("Unknown Path\n");
		return -ENODEV;
	}

	//select the requested path from the array of function pointers
	switch(path_num)
	{
		case OFF:
			DEBUG_LOG("Switching off output path\n");
			break;

		case RCV:
		case SPK:
		case HP:	
		case SPK_HP :
			DEBUG_LOG("routing to %s \n", mc->texts[path_num] );
			break;			

		case BT:	
		default:
			DEBUG_LOG_ERR("The audio path[%d] does not exists!! \n", path_num);
			return -ENODEV;
			break;
	}

	wm8994->cur_path = path_num;
	wm8994->call_state = DISCONNECT;
	wm8994->universal_playback_path[wm8994->cur_path](codec);
	
	if(wm8994->mic_state == MIC_NO_USE)
		audio_ctrl_mic_bias_gpio(0);
	
	return 0;
}

static int wm8994_get_voice_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	DEBUG_LOG("");
#if 0
	while(audio_path[i] != NULL) {
		if(!strcmp(audio_path[i], kcontrol->id.name) && ((wm8994_path >> 4) == i)) {
			ucontrol->value.integer.value[0] = wm8994_path & 0xf;
			break;
		}
		i++;
	}
#endif
	return 0;
}

static int wm8994_set_voice_path(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;	
	struct soc_enum *mc =
		(struct soc_enum *)kcontrol->private_value;

	// Get path value
	int path_num = ucontrol->value.integer.value[0];	
	
	if(strcmp( mc->texts[path_num], voicecall_path[path_num]) )
	{		
		DEBUG_LOG_ERR("Unknown path %s\n", mc->texts[path_num] );
		return -ENODEV;
	}
	
	wm8994->cur_path = path_num;
	wm8994->call_state = CONNECT;
	
	switch(path_num)
	{
		case OFF :
			DEBUG_LOG("Switching off output path\n");
			break;
			
		case RCV :
		case SPK :
		case HP :
		case BT :
			DEBUG_LOG("routing  voice path to  %s \n", mc->texts[path_num] );
			break;
		
		default:
			DEBUG_LOG_ERR("The audio path[%d] does not exists!! \n", path_num);
			return -ENODEV;
			break;
	}

        wm8994->universal_voicecall_path[wm8994->cur_path](codec);

	return 0;
}


static int wm8994_get_fmradio_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if AUDIO_COMMON_DEBUG		// for removing warning on compliing.
	int pathnum = ucontrol->value.integer.value[0];
#endif

	DEBUG_LOG("wm8994_get_fmradio_path : %d", pathnum);

	return 0;
}

static int wm8994_set_fmradio_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct soc_enum *mc =
		(struct soc_enum *)kcontrol->private_value;
	struct wm8994_priv *wm8994 = codec->drvdata;
	
	int path_num = ucontrol->value.integer.value[0];	
	
	if(strcmp( mc->texts[path_num], fmradio_path[path_num]) )
	{		
		DEBUG_LOG_ERR("Unknown path %s\n", mc->texts[path_num] );		
	}
	
	if(path_num == wm8994->fmradio_path)
	{
		DEBUG_LOG("%s is already set. skip to set path.. \n", mc->texts[path_num]);
		return 0;
	}
		
	switch(path_num)
	{
		case FMR_OFF:
			DEBUG_LOG("Switching off output path\n");
			wm8994_disable_fmradio_path(codec, FMR_OFF);
			break;
			
		case FMR_SPK:
			DEBUG_LOG("routing  fmradio path to  %s \n", mc->texts[path_num] );
			wm8994_set_fmradio_speaker(codec);
			break;

		case FMR_HP:
			DEBUG_LOG("routing  fmradio path to  %s \n", mc->texts[path_num] );
			wm8994_set_fmradio_headset(codec);
			break;

		case FMR_SPK_MIX:
			DEBUG_LOG("routing  fmradio path to  %s \n", mc->texts[path_num]);
			wm8994_set_fmradio_speaker_mix(codec);
			break;

		case FMR_HP_MIX:
			DEBUG_LOG("routing  fmradio path to  %s \n", mc->texts[path_num]);
			wm8994_set_fmradio_headset_mix(codec);
			break;

		case FMR_SPK_HP_MIX :
			DEBUG_LOG("routing  fmradio path to  %s \n", mc->texts[path_num]);
			wm8994_set_fmradio_speaker_headset_mix(codec);
			break;			

		default:
			DEBUG_LOG_ERR("The audio path[%d] does not exists!! \n", path_num);
			return -ENODEV;
			break;
	}
	
	return 0;
}

static int wm8994_get_codec_tuning(struct snd_kcontrol *kcontrol, 
	struct snd_ctl_elem_value *ucontrol)
{	
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("testmode_config_flag = [%d]", wm8994->testmode_config_flag);

	return wm8994->testmode_config_flag;
}

static int wm8994_set_codec_tuning(struct snd_kcontrol *kcontrol, 
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	int control_flag = ucontrol->value.integer.value[0];	

	DEBUG_LOG("control flag =[%d]", control_flag); 

	wm8994->testmode_config_flag = control_flag;

	return 0;
}

static int wm8994_get_mic_status(struct snd_kcontrol *kcontrol, 
	struct snd_ctl_elem_value *ucontrol)
{	
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("mic_state = [%d]", wm8994->mic_state);

	return wm8994->testmode_config_flag;
}

static int wm8994_set_mic_status(struct snd_kcontrol *kcontrol, 
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->drvdata;

	int control_flag = ucontrol->value.integer.value[0];	

	DEBUG_LOG("Changed mic state [%d] => [%d]", wm8994->mic_state, control_flag); 

	wm8994->mic_state = control_flag;

	return 0;
}

void wm8994_set_off(struct snd_soc_codec *codec)
{
	DEBUG_LOG("");
	
	audio_power(0);
}

//#define USE_INFINIEON_EC_FOR_VT 	//VT call flag used in TN platform

#if defined USE_INFINIEON_EC_FOR_VT
extern int s3c_pcmdev_clock_control(int enable);

static const char *clock_control[] = { "OFF", "ON"};

static const struct soc_enum clock_control_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(clock_control),clock_control),
};

static int s3c_pcmdev_get_clock(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int s3c_pcmdev_set_clock(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
	// Get path value
	int enable = ucontrol->value.integer.value[0];

	s3c_pcmdev_clock_control(enable);

	return 0;
}
#endif

#define  SOC_WM899X_OUTPGA_DOUBLE_R_TLV(xname, reg_left, reg_right, xshift, xmax, xinvert, tlv_array) \
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

//these are all factors of .01dB
static const DECLARE_TLV_DB_SCALE(digital_tlv, -7162, 37, 1);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_spkr,-5700,600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_rcv,-5700,600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_headphone,-5700,600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_mic,-7162,7162);


static const struct soc_enum path_control_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(playback_path),playback_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicecall_path),voicecall_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_path),mic_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fmradio_path),fmradio_path), 
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(codec_tuning_control), codec_tuning_control), 
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_state), mic_state),
};


static const struct snd_kcontrol_new wm8994_snd_controls[] = {
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Volume",  WM8994_LEFT_OPGA_VOLUME ,
		  		 WM8994_RIGHT_OPGA_VOLUME , 0, 0x3F, 0, digital_tlv_rcv),
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Spkr Volume", WM8994_SPEAKER_VOLUME_LEFT  ,
				 WM8994_SPEAKER_VOLUME_RIGHT  , 1,0x3F, 0, digital_tlv_spkr),
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Headset Volume",WM8994_LEFT_OUTPUT_VOLUME  ,
				 WM8994_RIGHT_OUTPUT_VOLUME   , 1,0x3F, 0, digital_tlv_headphone),
	SOC_WM899X_OUTPGA_SINGLE_R_TLV("Capture Volume",  WM8994_AIF1_ADC1_LEFT_VOLUME ,
                                         0, 0xEF, 0, digital_tlv_mic),
	 /* Path Control */
        SOC_ENUM_EXT("Playback Path", path_control_enum[0],
                wm8994_get_path, wm8994_set_path),

	SOC_ENUM_EXT("Voice Call Path", path_control_enum[1],
                wm8994_get_voice_path, wm8994_set_voice_path),

	SOC_ENUM_EXT("MIC Path", path_control_enum[2],
                wm8994_get_mic_path, wm8994_set_mic_path),

	SOC_ENUM_EXT("FM Radio Path", path_control_enum[3],
                wm8994_get_fmradio_path, wm8994_set_fmradio_path),

	SOC_ENUM_EXT("Codec Tuning", path_control_enum[4],
					wm8994_get_codec_tuning, wm8994_set_codec_tuning),

#if defined USE_INFINIEON_EC_FOR_VT	
	SOC_ENUM_EXT("Clock Control", clock_control_enum[0],
			s3c_pcmdev_get_clock, s3c_pcmdev_set_clock),
#endif
	SOC_ENUM_EXT("Mic Status", path_control_enum[5],
				wm8994_get_mic_status, wm8994_set_mic_status),

} ;//snd_ctrls


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
//SND_SOC_DAPM_DAC("DAC1", "Playback", WM8580_PWRDN1, 2, 1),
//SND_SOC_DAPM_DAC("DAC2", "Playback", WM8580_PWRDN1, 3, 1),
//SND_SOC_DAPM_DAC("DAC3", "Playback", WM8580_PWRDN1, 4, 1),

//SND_SOC_DAPM_OUTPUT("VOUT1L"),
//SND_SOC_DAPM_OUTPUT("VOUT1R"),
//SND_SOC_DAPM_OUTPUT("VOUT2L"),
//SND_SOC_DAPM_OUTPUT("VOUT2R"),
//SND_SOC_DAPM_OUTPUT("VOUT3L"),
//SND_SOC_DAPM_OUTPUT("VOUT3R"),

//SND_SOC_DAPM_ADC("ADC", "Capture", WM8580_PWRDN1, 1, 1),

//SND_SOC_DAPM_INPUT("AINL"),
//SND_SOC_DAPM_INPUT("AINR"),
};

static const struct snd_soc_dapm_route audio_map[] = {
#if 0
	{ "VOUT1L", NULL, "DAC1" },
	{ "VOUT1R", NULL, "DAC1" },

	{ "VOUT2L", NULL, "DAC2" },
	{ "VOUT2R", NULL, "DAC2" },

	{ "VOUT3L", NULL, "DAC3" },
	{ "VOUT3R", NULL, "DAC3" },

	{ "ADC", NULL, "AINL" },
	{ "ADC", NULL, "AINR" },
#endif
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

	reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
        reg &= ~WM8994_AIF1CLK_ENA ; //disable the clock
	reg &= ~WM8994_AIF1CLK_SRC_MASK; 
        wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

	/* This should be done on init() for bypass paths */
	switch (wm8994->sysclk_source) {
		case WM8994_SYSCLK_MCLK:
			dev_dbg(codec->dev, "Using %dHz MCLK\n", wm8994->mclk_rate);

			reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
			reg &= ~WM8994_AIF1CLK_ENA ; //disable the clock 
			wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

			reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
			reg &= 0x07; //clear clksrc bits ..now it is for MCLK
			
			if(wm8994->mclk_rate > 13500000)
			{
				reg |= WM8994_AIF1CLK_DIV ; 
				wm8994->sysclk_rate = wm8994->mclk_rate / 2;
			}
			else
			{
				reg &= ~WM8994_AIF1CLK_DIV;
				wm8994->sysclk_rate = wm8994->mclk_rate;
			}
			reg |= WM8994_AIF1CLK_ENA ; //enable the clocks
			wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

			//Enable clocks to the Audio core and sysclk of wm8994
			reg = wm8994_read(codec, WM8994_CLOCKING_1 );		
			reg &= ~(WM8994_SYSCLK_SRC_MASK | WM8994_DSP_FSINTCLK_ENA_MASK|WM8994_DSP_FS1CLK_ENA_MASK);
			reg |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
			wm8994_write(codec,WM8994_CLOCKING_1 ,reg);
			break;

		case WM8994_SYSCLK_FLL:
			switch(wm8994->fs )
			{
				case  8000: 
					wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x2F00);
					wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
					wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
					wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  11025: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x1F00 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0x86C2 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x00e0 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  12000: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x1F00 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0x3126);
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x0100 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  16000: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x1900 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0xE23E );
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x0100 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  22050: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x0F00 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0x86C2 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x00E0 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  24000: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x0F00 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0x3126 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x0100 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  32000: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x0C00 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0xE23E );
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x0100 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  44100: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x0700 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0x86C2 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x00E0 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				case  48000: 
					wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x0700 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0x3126 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
					wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x0100 );
					wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);
					break;

				default:
					DEBUG_LOG_ERR("Unsupported Frequency\n");
					break;
			}

			reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
			reg |= WM8994_AIF1CLK_ENA ; //enable the clocks
			reg |= WM8994_AIF1CLK_SRC_FLL1;//selecting FLL1
			wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

			//Enable clocks to the Audio core and sysclk of wm8994	
			reg = wm8994_read(codec, WM8994_CLOCKING_1 );
			reg &= ~(WM8994_SYSCLK_SRC_MASK | WM8994_DSP_FSINTCLK_ENA_MASK|WM8994_DSP_FS1CLK_ENA_MASK  );
			reg |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA  );
			wm8994_write(codec,WM8994_CLOCKING_1 ,reg);		
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
	//struct wm8994_priv *wm8994 = codec->drvdata;
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
		#if 0
			/* If either line output is single ended we
			 * need the VMID buffer */
			if (!wm8993->pdata.lineout1_diff ||
			    !wm8993->pdata.lineout2_diff)
				snd_soc_update_bits(codec, WM8994_ANTIPOP1,
						 WM8994_LINEOUT_VMID_BUF_ENA,
						 WM8994_LINEOUT_VMID_BUF_ENA);
#endif //if 0 shaju
			/* VMID=2*40k */
			snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
					    WM8994_VMID_SEL_MASK |
					    WM8994_BIAS_ENA,
					    WM8994_BIAS_ENA | 0x2);
			//msleep(32);//commented as without sleep() also it behaves properly in SLSI platform

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
				    WM8994_VMID_SEL_MASK | WM8994_BIAS_ENA,
				    0);
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


static int wm8994_set_dai_fmt(struct snd_soc_dai *dai,
			      unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	unsigned int aif1 = wm8994_read(codec,WM8994_AIF1_CONTROL_1);
	unsigned int aif2 = wm8994_read(codec,WM8994_AIF1_MASTER_SLAVE );

	DEBUG_LOG("");

	aif1 &= ~(WM8994_AIF1_LRCLK_INV |WM8994_AIF1_BCLK_INV |
		   WM8994_AIF1_WL_MASK | WM8994_AIF1_FMT_MASK);

	aif2 &= ~( WM8994_AIF1_LRCLK_FRC_MASK| WM8994_AIF1_CLK_FRC| WM8994_AIF1_MSTR ) ; //to enable LRCLK and bclk in master mode

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		wm8994->master = 0;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		aif2 |= (WM8994_AIF1_MSTR|WM8994_AIF1_LRCLK_FRC);
		wm8994->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		aif2 |= (WM8994_AIF1_MSTR|WM8994_AIF1_CLK_FRC) ;
		wm8994->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		aif2 |= (WM8994_AIF1_MSTR|WM8994_AIF1_CLK_FRC| WM8994_AIF1_LRCLK_FRC);
		//aif2 |= (WM8994_AIF1_MSTR);
		wm8994->master = 1;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_B:
		aif1 |=WM8994_AIF1_LRCLK_INV;
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
		/* frame inversion not valid for DSP modes */
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |=  WM8994_AIF1_BCLK_INV;
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
			aif1 |= WM8994_AIF1_BCLK_INV |WM8994_AIF1_LRCLK_INV;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |=    WM8994_AIF1_BCLK_INV;
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
	wm8994_write(codec,WM8994_AIF1_CONTROL_1, aif1);
	wm8994_write(codec,WM8994_AIF1_MASTER_SLAVE, aif2);
	wm8994_write( codec,WM8994_AIF1_CONTROL_2, 0x4000);
	
	return 0;
}

static int wm8994_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;
	int ret, i, best, best_val, cur_val;
	unsigned int clocking1, clocking3, aif1, aif4,aif5;

	DEBUG_LOG("");

	clocking1 = wm8994_read(codec,WM8994_AIF1_BCLK);
	clocking1 &= ~ WM8994_AIF1_BCLK_DIV_MASK ;

	clocking3 = wm8994_read(codec, WM8994_AIF1_RATE);
	clocking3 &= ~(WM8994_AIF1_SR_MASK | WM8994_AIF1CLK_RATE_MASK);

	aif1 = wm8994_read(codec, WM8994_AIF1_CONTROL_1);
	aif1 &= ~WM8994_AIF1_WL_MASK;
	aif4 = wm8994_read(codec,WM8994_AIF1ADC_LRCLK);
	aif4 &= ~WM8994_AIF1ADC_LRCLK_DIR ;
	aif5 = wm8994_read(codec,WM8994_AIF1DAC_LRCLK);
	aif5 &= ~WM8994_AIF1DAC_LRCLK_DIR_MASK;

	/* What BCLK do we need? */
	wm8994->fs = params_rate(params);
	wm8994->bclk = 2 * wm8994->fs;

	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			wm8994->bclk *= 16;
			break;

		case SNDRV_PCM_FORMAT_S20_3LE:
			wm8994->bclk *= 20;
			aif1 |= (0x01<< WM8994_AIF1_WL_SHIFT);
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
	if(wm8994->fs == 8000)		// Force to select clck_sys_rate 192 on using 8KHz.
		best = 3;
	else
	{
		best = 0;
		best_val = abs((wm8994->sysclk_rate / clk_sys_rates[0].ratio) - wm8994->fs);

		for (i = 1; i < ARRAY_SIZE(clk_sys_rates); i++) {
			cur_val = abs((wm8994->sysclk_rate /clk_sys_rates[i].ratio) - wm8994->fs);

			if (cur_val < best_val) {
				best = i;
				best_val = cur_val;
				}
		}
		dev_dbg(codec->dev, "Selected CLK_SYS_RATIO of %d\n", clk_sys_rates[best].ratio);
	}

	clocking3 |= (clk_sys_rates[best].clk_sys_rate << WM8994_AIF1CLK_RATE_SHIFT);

	/* SAMPLE_RATE */
	best = 0;
	best_val = abs(wm8994->fs - sample_rates[0].rate);
	for (i = 1; i < ARRAY_SIZE(sample_rates); i++) {
		/* Closest match */
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
		cur_val = ((wm8994->sysclk_rate ) / bclk_divs[i].div)
			- wm8994->bclk;
		if (cur_val < 0) /* Table is sorted */
			break;
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	wm8994->bclk = (wm8994->sysclk_rate ) / bclk_divs[best].div;
	dev_dbg(codec->dev, "Selected BCLK_DIV of %d for %dHz BCLK\n",
		bclk_divs[best].div, wm8994->bclk);

	clocking1 |= bclk_divs[best].bclk_div << WM8994_AIF1_BCLK_DIV_SHIFT;

	/* LRCLK is a simple fraction of BCLK */
	dev_dbg(codec->dev, "LRCLK_RATE is %d\n", wm8994->bclk / wm8994->fs);

	aif4 |= wm8994->bclk / wm8994->fs;
	aif5 |= wm8994->bclk / wm8994->fs;

#ifdef HDMI_USE_AUDIO
	if(wm8994->fs == 44100)
		wm8994_write(codec,WM8994_AIF1_BCLK,0x70); //set bclk to 32fs for 44.1kHz 16 bit playback.
#endif

//TODO...we need to set proper BCLK & LRCLK to support different frequency songs..In modifying 
//BCLK & LRCLK , its giving noisy and improper frequency sound..this has to be checked
#ifndef CONFIG_SND_S5P_WM8994_MASTER 
	//wm8994_write(codec,WM8994_AIF1_BCLK, clocking1);
	//wm8994_write(codec,WM8994_AIF1ADC_LRCLK, aif4);
	//wm8994_write(codec,WM8994_AIF1DAC_LRCLK, aif5);
#endif	
	wm8994_write(codec,WM8994_AIF1_RATE, clocking3);
	wm8994_write(codec, WM8994_AIF1_CONTROL_1, aif1);
	
	return 0;
}



static int wm8994_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
//Implementation has to be tested properly.
	#if 0
	unsigned int reg;
	struct snd_soc_codec *codec = codec_dai->codec;
	reg = wm8994_read(codec, WM8994_DAC_SOFTMUTE);
	reg &= ~WM8994_DAC_SOFTMUTEMODE_MASK;

	if (mute)
		reg |= WM8994_DAC_SOFTMUTEMODE;
	else
		reg &= ~WM8994_DAC_SOFTMUTEMODE;

	wm8994_write(codec, WM8994_DAC_SOFTMUTEMODE_MASK, reg);
	#endif

#if AUDIO_COMMON_DEBUG		// for removing warning on compliing.
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;
#endif

	DEBUG_LOG("Mute =[%d], current Path = [%d]\n", mute, wm8994->cur_path);

	return 0;
}

static bool play_en_dis = 0;
static bool rec_en_dis  = 0;

static int wm8994_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;
#if 1 
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
	play_en_dis = 1;
	}
	else{
	rec_en_dis = 1;
	}
	
	
	if(wm8994->testmode_config_flag)
	{
		DEBUG_LOG_ERR("Testmode is activated!! Skip statup sequence!!");
		return 0;
	}

	if(wm8994->power_state == CODEC_OFF)
	{
		wm8994->power_state = CODEC_ON;
		DEBUG_LOG("Turn on codec!! Power state =[%d]", wm8994->power_state);

		// For initialize codec.	
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x3 << WM8994_VMID_SEL_SHIFT | WM8994_BIAS_ENA);
		msleep(10);
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);

		wm8994_write(codec,WM8994_OVERSAMPLING, 0x0000);
	}
	else
		DEBUG_LOG("Already turned on codec!!");
#endif
	return 0;
}

static void wm8994_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
	play_en_dis = 0;
	DEBUG_LOG("..inside..%s..for PLAYBACK_STREAM!!",__func__);
		if(wm8994->cur_path != OFF){
                                wm8994_disable_playback_path(codec, wm8994->cur_path);
                                wm8994->cur_path = OFF;
                }
	}
	else{
	rec_en_dis = 0;
	DEBUG_LOG("..inside..%s..for CAPTURE_STREAM!!",__func__);
		if(wm8994->mic_state == MIC_NO_USE && wm8994->rec_path != MIC_OFF)
                                wm8994_disable_rec_path(codec, wm8994->rec_path);
	}

	if(wm8994->call_state == DISCONNECT)
	{

		if(wm8994->testmode_config_flag)
		{
			DEBUG_LOG_ERR("Testmode is activated!! Don't shutdown(reset) sequence!!");
			return;
		}

		if(wm8994->fmradio_path != FMR_OFF)
                                wm8994_disable_fmradio_path(codec, FMR_OFF);

		if(!play_en_dis && !rec_en_dis){
			DEBUG_LOG("Turn off Codec!!");
                        wm8994->power_state = CODEC_OFF;
                        wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
#if defined ATTACH_ADDITINAL_PCM_DRIVER
			vtCallActive = 0;
#endif
                }
		

#if 0
		if(wm8994->mic_state == MIC_NO_USE)
		{
			DEBUG_LOG("Turn off Codec!!");
			wm8994->power_state = CODEC_OFF;
			wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
		}
		else
		{
			if(wm8994->cur_path != OFF)
			{
				wm8994_disable_playback_path(codec, wm8994->cur_path);
				wm8994->cur_path = OFF;
			}
			
			if(wm8994->fmradio_path != FMR_OFF)
				wm8994_disable_fmradio_path(codec, FMR_OFF);
			
			if(wm8994->mic_state == MIC_NO_USE && wm8994->rec_path != MIC_OFF)
				wm8994_disable_rec_path(codec, wm8994->rec_path);
		}

#endif


	}
	else
		DEBUG_LOG("Preserve codec state for call[%d].", wm8994->call_state);

		DEBUG_LOG("exiting ...%s...",__func__);
		
}

#if defined ATTACH_ADDITINAL_PCM_DRIVER
static int wm8994_pcm_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->drvdata;
 
	int reg;
 
	 if(vtCallActive == 0)
	 {
		 vtCallActive = 1;
		wm8994->cur_path = OFF;
		//wm8994->codec_state = DEACTIVE;
		wm8994->power_state = CODEC_OFF;
		
		DEBUG_LOG("Turn on codec!! Power state =[%d]", wm8994->power_state);

		// For initialize codec.	
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x3 << WM8994_VMID_SEL_SHIFT | WM8994_BIAS_ENA);
		msleep(50);
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);

		wm8994_write(codec,WM8994_OVERSAMPLING, 0x0000);

		wm8994_write( codec,WM8994_FLL1_CONTROL_2, 0x0700 );
		wm8994_write( codec,WM8994_FLL1_CONTROL_3, 0x86C2 );
		wm8994_write( codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
		wm8994_write( codec,WM8994_FLL1_CONTROL_4, 0x00E0 );
		wm8994_write(codec, WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA |WM8994_FLL1_ENA);

		reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
		reg |= (WM8994_AIF1CLK_SRC_FLL1 | WM8994_AIF1CLK_ENA); //enable the clocks
		wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);
	
		//Enable clocks to the Audio core and sysclk of wm8994	
		reg = wm8994_read(codec, WM8994_CLOCKING_1 );
		reg &= ~(WM8994_SYSCLK_SRC_MASK | WM8994_DSP_FSINTCLK_ENA_MASK|WM8994_DSP_FS1CLK_ENA_MASK);
		reg |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
		wm8994_write(codec,WM8994_CLOCKING_1 ,reg);		
	}
	else
		DEBUG_LOG("Already turned on codec!!");

	return 0;
}
#endif

static struct snd_soc_device *wm8994_socdev;
static struct snd_soc_codec  *wm8994_codec;

#define WM8994_RATES SNDRV_PCM_RATE_8000_96000
#define WM8994_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
static struct snd_soc_dai_ops wm8994_ops = {
			 .startup = wm8994_startup,
			 .shutdown = wm8994_shutdown,
			 .set_sysclk = wm8994_set_sysclk,
			 .set_fmt = wm8994_set_dai_fmt,
			 .hw_params = wm8994_hw_params,
			 .digital_mute = wm8994_digital_mute,
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
EXPORT_SYMBOL_GPL(wm8994_dai);

#if defined ATTACH_ADDITINAL_PCM_DRIVER
static struct snd_soc_dai_ops wm8994_pcm_ops = {
			 .startup = wm8994_pcm_startup,
};

struct snd_soc_dai wm8994_pcm_dai = {

                .name = "WM8994 PCM",
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
		.ops = &wm8994_pcm_ops,
};
EXPORT_SYMBOL_GPL(wm8994_pcm_dai);
#endif

/*
 * initialise the WM8994 driver
 * register the mixer and dsp interfaces with the kernel
 */
//static int wm8994_init(struct snd_soc_device *socdev)
static int wm8994_init(struct wm8994_priv *wm8994_private)
{
	struct snd_soc_codec *codec = &wm8994_private->codec;
	struct wm8994_priv *wm8994 ;
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
	codec->set_bias_level = wm8994_set_bias_level;
	codec->dai = &wm8994_dai;
	codec->num_dai = 1;//ARRAY_SIZE(wm8994_dai);
	wm8994->universal_playback_path = universal_wm8994_playback_paths;
	wm8994->universal_voicecall_path = universal_wm8994_voicecall_paths;
	wm8994->universal_mic_path = universal_wm8994_mic_paths;
	wm8994->cur_path = OFF;
	wm8994->rec_path = MIC_OFF;
	wm8994->call_state = DISCONNECT;
	wm8994->fmradio_path = FMR_OFF;
	wm8994->testmode_config_flag = 0;
	wm8994->power_state = CODEC_OFF;
	wm8994->mic_state = MIC_NO_USE;

	wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000);

	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x3 << WM8994_VMID_SEL_SHIFT | WM8994_BIAS_ENA);
	//msleep(10);//commented as sleep not required in SLSI platform
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);

	wm8994->hw_version = wm8994_read(codec, 0x100);	// Read Wm8994 version.

	wm8994_socdev->card->codec = codec;
	wm8994_codec = codec;
	/* register pcms */
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
	//kfree(codec->reg_cache);
	return ret;
}

#if defined ATTACH_ADDITINAL_PCM_DRIVER
static struct snd_soc_device *wm8994_pcm_socdev;
static struct snd_soc_codec  *wm8994_pcm_codec;

static int wm8994_pcm_init(struct wm8994_priv *wm8994_private)
{
	struct snd_soc_codec *codec = &wm8994_private->codec;
        struct wm8994_priv *wm8994;
        int ret = 0, val;

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
        codec->set_bias_level = wm8994_set_bias_level;
        codec->dai = &wm8994_pcm_dai;
        codec->num_dai = 1;//ARRAY_SIZE(wm8994_pcm_dai);
        wm8994->universal_voicecall_path = universal_wm8994_voicecall_paths;
        wm8994->cur_path = OFF;

        val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
        val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK);
        val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL);
        ret = wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

	wm8994->hw_version = wm8994_read(codec, 0x100);	// Read Wm8994 version.

	if(ret)
		printk("first wm8994_write failed in %s..\n",__func__);

	wm8994_pcm_socdev->card->codec = codec;
	wm8994_pcm_codec = codec;

#if 0
	ret = snd_soc_new_pcms(socdev, 1,"wm8994-pcm");
#else
	ret = snd_soc_new_pcms(wm8994_pcm_socdev,  1,"wm8994-pcm");
#endif
        if (ret < 0) {
                printk(KERN_ERR "wm8994: failed to create pcms\n");
                goto pcm_err;
        }

       wm8994_add_controls(codec);
       // wm8994_add_widgets(codec);

        ret = snd_soc_init_card(wm8994_pcm_socdev);
        if (ret < 0) {
                printk(KERN_ERR "wm8994: failed to register card\n");
                goto card_err;
        }
        return ret;

card_err:
        snd_soc_free_pcms(wm8994_pcm_socdev);
       // snd_soc_dapm_free(socdev);
pcm_err:
        kfree(codec->reg_cache);
	return ret;
}
#endif

/* If the i2c layer weren't so broken, we could pass this kind of data
   around */

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)

/*
 * WM8994 2 wire address is determined by GPIO5
 * state during powerup.
 *    low  = 0x1a
 *    high = 0x1b
 */
static void * control_data1;

static int wm8994_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct snd_soc_codec *codec ;
	struct wm8994_priv* wm8994_priv;
	int ret;

	DEBUG_LOG("");
#if 1
	 wm8994_priv = kzalloc(sizeof(struct wm8994_priv), GFP_KERNEL);
	 if (wm8994_priv == NULL)
	    return -ENOMEM;
#endif
	codec = &wm8994_priv->codec;
	#ifdef PM_DEBUG
	pm_codec = codec;
	#endif

	codec->hw_write = (hw_write_t)i2c_master_send; 
	i2c_set_clientdata(i2c, wm8994_priv);
	codec->control_data = i2c;
	codec->dev = &i2c->dev;
	control_data1 = i2c;
	ret = wm8994_init(wm8994_priv);
	if (ret < 0)
		dev_err(&i2c->dev, "failed to initialize WM8994\n");
	return ret;
}

static int wm8994_i2c_remove(struct i2c_client *client)
{
	struct wm8994_priv* wm8994_priv = i2c_get_clientdata(client);
	kfree(wm8994_priv);
	return 0;
}

static const struct i2c_device_id wm8994_i2c_id[] = {
	{ "wm8994", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, wm8994_i2c_id);

static struct i2c_driver wm8994_i2c_driver = {
	.driver = {
		.name = "WM8994 I2C Codec",
		.owner = THIS_MODULE,
	},
	.probe =    wm8994_i2c_probe,
	.remove =   wm8994_i2c_remove,
	.id_table = wm8994_i2c_id,
};

struct i2c_board_info info;
struct i2c_adapter *adapter;
struct i2c_client *client;

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
	struct snd_soc_codec *codec;
	struct wm8994_priv *wm8994;
	int ret = 0;

	pr_info("WM8994 Audio Codec %s\n", WM8994_VERSION);

	/* Board Specific Function */
	audio_init();
	audio_power(1);
	msleep(10);

	setup = socdev->codec_data;
	wm8994_socdev = socdev;
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
		ret = wm8994_add_i2c_device(pdev, setup);
	}
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
	//kfree(codec);

	return 0;
}

#if defined ATTACH_ADDITINAL_PCM_DRIVER
static int wm8994_pcm_probe(struct platform_device *pdev)
{
        struct snd_soc_device *socdev = platform_get_drvdata(pdev);
        struct wm8994_setup_data *setup;
        struct snd_soc_codec *codec;
        struct wm8994_priv *wm8994;
        int ret = 0;

        pr_info("WM8994 Audio Codec %s\n", WM8994_VERSION);

#if 1
        wm8994 = kzalloc(sizeof(struct wm8994_priv), GFP_KERNEL);
	 if (wm8994 == NULL)
                return -ENOMEM;
#endif
	//codec = &wm8994_priv->codec;
	codec = &wm8994->codec;

	setup = socdev->codec_data;
	wm8994_pcm_socdev = socdev;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
        if (setup->i2c_address) {
                   codec->hw_write = (hw_write_t)i2c_master_send;
		codec->control_data = control_data1;
		wm8994_pcm_init(wm8994);
                //ret = wm8994_add_i2c_device(pdev, setup);
        }
#else
                /* Add other interfaces here */
#endif
        return ret;
}

/* power down chip */
static int wm8994_pcm_remove(struct platform_device *pdev)
{
        struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = wm8994_pcm_codec;

        snd_soc_free_pcms(socdev);
        snd_soc_dapm_free(socdev);

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)	// It's executed by I2S
      //  i2c_unregister_device(codec->control_data);
      //  i2c_del_driver(&wm8994_i2c_driver);
#endif

        kfree(codec->drvdata);
        kfree(codec);

        return 0;
}
#endif

#ifdef CONFIG_PM
static int wm8994_suspend(struct platform_device *pdev,pm_message_t msg )
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
        struct snd_soc_codec *codec = wm8994_codec;
        struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("%s..",__func__);
	
	if(wm8994->testmode_config_flag)
	{
		DEBUG_LOG_ERR("Testmode is activated!! Skip suspend sequence!!");
		return 0;
	}

	if(wm8994->call_state == DISCONNECT && wm8994->cur_path == OFF )
	{
		wm8994->power_state = OFF;
		
		audio_power(0);		
	}
		
	return 0;
}

static int wm8994_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = wm8994_codec;
	struct wm8994_priv *wm8994 = codec->drvdata;

	DEBUG_LOG("%s..",__func__);
	DEBUG_LOG_ERR("------WM8994 Revision = [%d]-------", wm8994->hw_version);

	if(wm8994->testmode_config_flag)
	{
		DEBUG_LOG_ERR("Testmode is activated!! Skip resume sequence!!");
		return 0;
	}

	if(wm8994->call_state == DISCONNECT && wm8994->cur_path == OFF)
	{
		// Turn on sequence by recommend Wolfson.
		audio_power(1);
		wm8994->power_state = CODEC_ON;
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x3 << WM8994_VMID_SEL_SHIFT | WM8994_BIAS_ENA);
		//msleep(50);	// Wait to setup PLL. ////commented as without sleep() also it behaves properly in SLSI platform
		wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);

		wm8994_write(codec,WM8994_OVERSAMPLING, 0x0000);
	}
	return 0;
}
#endif

struct snd_soc_codec_device soc_codec_dev_wm8994 = {
	.probe = 	wm8994_probe,
	.remove = 	wm8994_remove,
#ifdef CONFIG_PM
	.suspend= wm8994_suspend,
	.resume= wm8994_resume,
#endif
};

EXPORT_SYMBOL_GPL(soc_codec_dev_wm8994);

#if defined ATTACH_ADDITINAL_PCM_DRIVER
struct snd_soc_codec_device soc_codec_dev_pcm_wm8994 = {
        .probe =        wm8994_pcm_probe,
        .remove =       wm8994_pcm_remove,
#ifdef CONFIG_PM
       // .suspend= wm8994_pcm_suspend,
       // .resume= wm8994_pcm_resume,
#endif
};

EXPORT_SYMBOL_GPL(soc_codec_dev_pcm_wm8994);
#endif

static int __init wm8994_modinit(void)
{
	int ret;
	ret = snd_soc_register_dai(&wm8994_dai);
	if(ret)
		printk(KERN_ERR "..dai registration failed..\n");

#if defined ATTACH_ADDITINAL_PCM_DRIVER
	ret = snd_soc_register_dai(&wm8994_pcm_dai);
	if(ret)
		printk(KERN_ERR "..pcm_dai registration failed..\n");
#endif

	return ret;
}
module_init(wm8994_modinit);

static void __exit wm8994_exit(void)
{
	snd_soc_unregister_dai(&wm8994_dai);
#if defined ATTACH_ADDITINAL_PCM_DRIVER
	snd_soc_unregister_dai(&wm8994_pcm_dai);
#endif
}
module_exit(wm8994_exit);

MODULE_DESCRIPTION("ASoC WM8994 driver");
MODULE_AUTHOR("Shaju Abraham shaju.abraham@samsung.com");
MODULE_LICENSE("GPL");
