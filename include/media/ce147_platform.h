/*
 * Driver for CE147 (5MP camera) from NEC
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

struct ce147_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in Hz */

	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;
	int (*power_en) (int);
};

