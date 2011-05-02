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

struct tl2796_gamma_adj_points {
	const u32 v0;
	const u32 v1;
	const u32 v19;
	const u32 v43;
	const u32 v87;
	const u32 v171;
	const u32 v255;
};

struct tl2796_color_adj {
	u32 mult[3];
	int rshift;
};

struct s5p_panel_data {
	const u16 *seq_display_set;
	const u16 *seq_etc_set;
	const u16 *standby_on;
	const u16 *standby_off;

	int gpio_dcx;
	int gpio_rdx;
	int gpio_csx;
	int gpio_wrx;
	int gpio_rst;
	int gpio_db[8];
	int (*configure_mtp_gpios)(struct s5p_panel_data *pdata, bool enable);
	u16 factory_v255_regs[3];
	struct tl2796_color_adj color_adj;

	const struct tl2796_gamma_adj_points *gamma_adj_points;
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

