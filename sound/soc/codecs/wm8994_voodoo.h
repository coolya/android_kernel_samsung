/*
 * wm8994.h  --  WM8994 Soc Audio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define VOODOO_SOUND_VERSION 10

#if defined(CONFIG_MACH_HERRING) || defined (CONFIG_SAMSUNG_GALAXYS)	       \
	|| defined (CONFIG_SAMSUNG_GALAXYSB)				       \
	|| defined (CONFIG_SAMSUNG_CAPTIVATE)				       \
	|| defined (CONFIG_SAMSUNG_VIBRANT)				       \
	|| defined (CONFIG_SAMSUNG_FASCINATE)				       \
	|| defined (CONFIG_SAMSUNG_EPIC)
#define NEXUS_S
#endif

#if defined(CONFIG_FB_S3C_AMS701KA) || defined(CONFIG_KOR_MODEL_M180S)
#define GALAXY_TAB
#endif

#ifdef CONFIG_M110S
#define M110S
#endif

#ifdef CONFIG_MACH_SAMSUNG_VARIATION_TEGRA
#define GALAXY_TAB_TEGRA
#endif

#ifdef CONFIG_TDMB_T3700
#define M110S
#endif

enum debug_log { LOG_OFF, LOG_INFOS, LOG_VERBOSE };
bool debug_log(short unsigned int level);

enum unified_path { HEADPHONES, RADIO_HEADPHONES, SPEAKER, MAIN_MICROPHONE };

bool is_path(int unified_path);
bool is_path_media_or_fm_no_call_no_record(void);
unsigned int voodoo_hook_wm8994_write(struct snd_soc_codec *codec,
				      unsigned int reg, unsigned int value);
void voodoo_hook_fmradio_headset(void);
void voodoo_hook_wm8994_pcm_probe(struct snd_soc_codec *codec);
void voodoo_hook_wm8994_pcm_remove(void);
void voodoo_hook_record_main_mic(void);
void voodoo_hook_playback_speaker(void);

void load_current_eq_values(void);
void apply_saturation_prevention_drc(void);

void update_hpvol(bool with_fade);
void update_fm_radio_headset_restore_freqs(bool with_mute);
void update_fm_radio_headset_normalize_gain(bool with_mute);
void update_recording_preset(bool with_mute);
void update_full_bitwidth(bool with_mute);
void update_osr128(bool with_mute);
void update_fll_tuning(bool with_mute);
void update_mono_downmix(bool with_mute);
void update_dac_direct(bool with_mute);
void update_digital_gain(bool with_mute);
void update_stereo_expansion(bool with_mute);
void update_headphone_eq(bool update_bands);
void update_headphone_eq_bands(void);
void update_enable(void);
