/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/tl2796.h>

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000

static const u16 s6e63m0_SEQ_STANDBY_ON[] = {
	0x010,	/* Stand-by On Command */
	SLEEPMSEC, 160,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_STANDBY_OFF[] = {
	0x011,	/* Stand-by Off Command */
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_DISPLAY_SETTING[] = {
	SLEEPMSEC, 10,
	0x0F8,	/* Panel Condition Set Command*/
	0x101,	/* DOCT */
	0x127,	/* CLWEA */
	0x127,	/* CLWEB*/
	0x107,	/* CLTE */
	0x107,	/* SHE */
	0x154,	/* FLTE */
	0x19F,	/* FLWE */
	0x163,	/* SCTE */
	0x186,	/* SCWE */
	0x11A,	/* INTE */
	0x133,	/* INWE */
	0x10D,	/* EMPS */
	0x100,	/* E_INTE */
	0x100,	/* E_INWE */
	0x0F2,	/* Display Condition Set Command*/
	0x102,	/* Number of Line */
	0x103,	/* VBP */
	0x11C,	/* VFP */
	0x110,	/* HBP */
	0x110,	/* HFP */
	0x0F7,	/* Command */
	0x103,	/* GTCON */
	0x100,	/* Display Mode */
	0x100,	/* Vsync/Hsync, DOCCLK, RGB mode */
	ENDDEF, 0x0000
};

static const u16 s6e63m0_SEQ_ETC_SETTING[] = {
	/* ETC Condition Set Command  */
	0x0F6,
	0x100,	0x18E,
	0x107,
	0x0B3,
	0x16C,
	0x0B5,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0B6,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x0B7,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0B8,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x0B9,
	0x12C,	0x112,
	0x10C,	0x10A,
	0x110,	0x10E,
	0x117,	0x113,
	0x11F,	0x11A,
	0x12A,	0x124,
	0x11F,	0x11B,
	0x11A,	0x117,
	0x12B,	0x126,
	0x122,	0x120,
	0x13A,	0x134,
	0x130,	0x12C,
	0x129,	0x126,
	0x125,	0x123,
	0x121,	0x120,
	0x11E,	0x11E,
	0x0BA,
	0x100,	0x100,
	0x111,	0x122,
	0x133,	0x144,
	0x144,	0x144,
	0x155,	0x155,
	0x166,	0x166,
	0x166,	0x166,
	0x166,	0x166,
	0x011,
	SLEEPMSEC, 120,
	0x029,
	ENDDEF, 0x0000
};

static const struct tl2796_gamma_adj_points gamma_adj_points = {
	.v0 = 1,
	.v1 = 1,
	.v19 = BV_1,
	.v43 = 0x4595438,
	.v87 = BV_87,
	.v171 = BV_171,
	.v255 = BV_255,
};

static const struct gamma_entry gamma_table[] = {
	{       BV_0, { 4200000, 4200000, 4200000, }, },
	{          1, { 3994200, 4107600, 3910200, }, },
	{          2, { 3467167, 3789169, 3488171, }, },
	{    7000000, { 3310578, 3407998, 3280696, }, },
	{   14000000, { 3234532, 3291183, 3181487, }, },
	{   22000000, { 3195339, 3222358, 3125699, }, },
	{   37000000, { 3149722, 3164112, 3057860, }, },
	{   55000000, { 3108336, 3118714, 3006053, }, },
	{   90000000, { 3039792, 3045000, 2931600, }, },
	{  150000000, { 2985326, 2989132, 2848234, }, },
	{  200000000, { 2926000, 2935800, 2772000, }, },
	{ 0x11111111, { 2849000, 2856000, 2674000, }, },
	{ 0x22222222, { 2723000, 2730000, 2506000, }, },
	{ 0x44444444, { 2555000, 2562000, 2282000, }, },
	{ 0x7FFFFFFF, { 2359000, 2373000, 2002000, }, },
	{     BV_255, { 2065000, 2072000, 1596000, }, },
};

struct s5p_panel_data herring_panel_data = {
	.seq_display_set = s6e63m0_SEQ_DISPLAY_SETTING,
	.seq_etc_set = s6e63m0_SEQ_ETC_SETTING,
	.standby_on = s6e63m0_SEQ_STANDBY_ON,
	.standby_off = s6e63m0_SEQ_STANDBY_OFF,
	.gamma_adj_points = &gamma_adj_points,
	.gamma_table = gamma_table,
	.gamma_table_size = ARRAY_SIZE(gamma_table),
};
