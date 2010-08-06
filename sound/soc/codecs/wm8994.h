/*
 * wm8994.h  --  WM8994 Soc Audio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _WM8994_H
#define _WM8994_H

#include <sound/soc.h>

extern struct snd_soc_codec_device soc_codec_dev_wm8994;
// We don't use array	- DW Shim.
//extern struct snd_soc_dai wm8994_dai[];

/* Sources for AIF1/2 SYSCLK - use with set_dai_sysclk() */
#define WM8994_SYSCLK_MCLK1 1
#define WM8994_SYSCLK_MCLK2 2
#define WM8994_SYSCLK_FLL1  3
#define WM8994_SYSCLK_FLL2  4

#define WM8994_FLL1 1
#define WM8994_FLL2 2

//-----------------------------------------------------------
// Added belows codes by Samsung Electronics.

#include "wm8994_def.h"

extern struct snd_soc_dai wm8994_dai;

#define WM8994_SYSCLK_MCLK     1
#define WM8994_SYSCLK_FLL      2

#define AUDIO_COMMON_DEBUG  0 
//#define WM8994_REGISTER_DUMP
#if defined CONFIG_SND_SOC_WM8994_PCM
#define ATTACH_ADDITINAL_PCM_DRIVER	// for VT call.
#endif
//------------------------------------------------
// Definitions of enum type
//------------------------------------------------
enum audio_path	{ OFF, RCV, SPK, HP, BT, SPK_HP};
enum mic_path		{ MAIN, SUB, MIC_OFF};
enum fmradio_audio_path { FMR_OFF, FMR_SPK, FMR_HP, FMR_SPK_MIX, FMR_HP_MIX, FMR_SPK_HP_MIX};
enum call_state		{ DISCONNECT, CONNECT};
enum power_state	{ CODEC_OFF, CODEC_ON };
enum mic_state		{ MIC_NO_USE, MIC_USE};

typedef void (*select_route)(struct snd_soc_codec *);
typedef void (*select_mic_route)(struct snd_soc_codec *);

struct wm8994_setup_data {
	int i2c_bus;
	unsigned short i2c_address;
};

struct wm8994_priv {
	//u16 reg_cache[WM8994_REGISTER_COUNT];
	struct snd_soc_codec codec;
	int master;
	int sysclk_source;
	unsigned int mclk_rate;
	unsigned int sysclk_rate;
	unsigned int fs;
	unsigned int bclk;
	unsigned int hw_version;		// For wolfson H/W version. 1 = Rev B, 3 = Rev D
	enum audio_path cur_path;
	enum mic_path rec_path;
	enum fmradio_audio_path fmradio_path;
	enum call_state call_state;
	enum power_state power_state;
	enum mic_state mic_state;
	select_route *universal_playback_path;
	select_route *universal_voicecall_path;
	select_mic_route *universal_mic_path;
	int testmode_config_flag;	// for testmode.
};

#if AUDIO_COMMON_DEBUG
#define DEBUG_LOG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define DEBUG_LOG(format,...)
#endif

#define DEBUG_LOG_ERR(format,...)\
	printk (KERN_ERR "[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);

// Definitions of function prototype.
inline unsigned int wm8994_read(struct snd_soc_codec *codec,unsigned int reg);
int wm8994_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value);
int audio_init(void);
int audio_power(int en);
void audio_ctrl_mic_bias_gpio(int enable);
void wm8994_set_off(struct snd_soc_codec *codec);
void wm8994_disable_playback_path(struct snd_soc_codec *codec, enum audio_path path);
void wm8994_disable_fmradio_path(struct snd_soc_codec *codec, enum fmradio_audio_path path);
void wm8994_disable_rec_path(struct snd_soc_codec *codec,enum mic_path rec_path);
void  wm8994_record_main_mic( struct snd_soc_codec *codec);
void wm8994_record_headset_mic( struct snd_soc_codec *codec);
void wm8994_set_playback_receiver(struct snd_soc_codec *codec);
void wm8994_set_playback_headset(struct snd_soc_codec *codec);
void wm8994_set_playback_speaker(struct snd_soc_codec *codec);
void wm8994_set_playback_speaker_headset(struct snd_soc_codec *codec);
void wm8994_set_voicecall_receiver(struct snd_soc_codec *codec);
void wm8994_set_voicecall_headset(struct snd_soc_codec *codec);
void wm8994_set_voicecall_speaker(struct snd_soc_codec *codec);
void wm8994_set_voicecall_bluetooth(struct snd_soc_codec *codec);
void wm8994_set_fmradio_headset(struct snd_soc_codec *codec);
void wm8994_set_fmradio_speaker(struct snd_soc_codec *codec);
void wm8994_set_fmradio_headset_mix(struct snd_soc_codec *codec);
void wm8994_set_fmradio_speaker_mix(struct snd_soc_codec *codec);
void wm8994_set_fmradio_speaker_headset_mix(struct snd_soc_codec *codec);
#if defined WM8994_REGISTER_DUMP
void wm8994_register_dump(struct snd_soc_codec *codec);
#endif
#endif
