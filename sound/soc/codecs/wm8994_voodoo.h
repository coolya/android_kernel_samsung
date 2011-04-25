/*
 * wm8994.h  --  WM8994 Soc Audio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if defined(CONFIG_MACH_HERRING) || defined (CONFIG_SAMSUNG_GALAXYS)	       \
	|| defined (CONFIG_SAMSUNG_GALAXYSB)				       \
	|| defined (CONFIG_SAMSUNG_CAPTIVATE)				       \
	|| defined (CONFIG_SAMSUNG_VIBRANT)				       \
	|| defined (CONFIG_SAMSUNG_FASCINATE)				       \
	|| defined (CONFIG_SAMSUNG_EPIC)
#define NEXUS_S
#endif

#ifdef CONFIG_FB_S3C_AMS701KA
#define GALAXY_TAB
#endif

#ifdef CONFIG_M110S
#define M110S
#endif

enum unified_path { HEADPHONES, RADIO_HEADPHONES, SPEAKER, MAIN_MICROPHONE };

bool is_path(int unified_path);
unsigned short tune_fll_value(unsigned short val);
unsigned int voodoo_hook_wm8994_write(struct snd_soc_codec *codec,
				      unsigned int reg, unsigned int value);
void voodoo_hook_fmradio_headset(void);
void voodoo_hook_wm8994_pcm_probe(struct snd_soc_codec *codec);
void voodoo_hook_wm8994_pcm_remove(void);
void voodoo_hook_record_main_mic(void);
void voodoo_hook_playback_speaker(void);

void update_hpvol(void);
void update_fm_radio_headset_restore_freqs(bool with_mute);
void update_fm_radio_headset_normalize_gain(bool with_mute);
void update_recording_preset(bool with_mute);
void update_full_bitwidth(bool with_mute);
void update_osr128(bool with_mute);
void update_fll_tuning(bool with_mute);
void update_mono_downmix(bool with_mute);
void update_dac_direct(bool with_mute);
void update_enable(void);
