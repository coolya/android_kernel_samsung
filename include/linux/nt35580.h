/*include/linux/nt35580.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Header file for Sony LCD Panel(TFT) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/types.h>

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DEFMASK		0xFF00

struct s5p_tft_panel_data {
	const u16 *seq_set;
	const u16 *sleep_in;
	const u16 *display_on;
	const u16 *display_off;
	u16 *brightness_set;
	int pwm_reg_offset;
};


