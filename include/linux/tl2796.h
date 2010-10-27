/*inclue/linux/tl2796.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Header file for Samsung Display Panel(AMOLED) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/types.h>

struct gamma_entry {
	u32 brightness;
	u32 v[3];
};

struct s5p_panel_data {
	const u16 *seq_display_set;
	const u16 *seq_etc_set;
	const u16 *standby_on;
	const u16 *standby_off;
	const struct gamma_entry *gamma_table;
	int gamma_table_size;
};

enum {
	BV_0   =          0,
	BV_1   =     0x552D,
	BV_19  =   0xD8722A,
	BV_43  =  0x51955E1,
	BV_87  = 0x18083FB0,
	BV_171 = 0x6A472534,
	BV_255 = 0xFFFFFFFF,
};

