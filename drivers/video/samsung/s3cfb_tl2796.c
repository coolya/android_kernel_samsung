/*
 * s6e63m0 AMOLED Panel Driver for the Samsung Universal board
 *
 * Derived from drivers/video/omap/lcd-apollon.c
 *
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

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>
#include <linux/earlysuspend.h>

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DEFMASK		0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

struct s5p_lcd{
	int ldi_enable;
	int bl;
	struct device *dev;
	struct spi_device *g_spi;
	struct backlight_device *bl_dev;
	struct early_suspend    early_suspend;
};

const u16 s6e63m0_SEQ_DISPLAY_ON[] = {
	ENDDEF, 0x0000
};

const u16 s6e63m0_SEQ_DISPLAY_OFF[] = {
	ENDDEF, 0x0000
};

const u16 s6e63m0_SEQ_STANDBY_ON[] = {
	0x010,
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

const u16 s6e63m0_SEQ_STANDBY_OFF[] = {
	0x011,
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

const u16 s6e63m0_SEQ_DISPLAY_SETTING[] = {
	SLEEPMSEC, 120,
	0x0F8,
	0x101,	0x127,
	0x127,	0x107,
	0x107,	0x154,
	0x19F,	0x163,
	0x186,	0x11A,
	0x133,	0x10D,
	0x100,	0x100,
	0x0F2,
	0x102,	0x103,
	0x11C,	0x110,
	0x110,
	0x0F7,
	0x103,	0x100,
	0x100,
	ENDDEF, 0x0000
};

const u16 s6e63m0_SEQ_ETC_SETTING[] = {
	0x0F6,
	0x100,	0x18C,
	0x107,
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

enum {
	BV_0   =          0,
	BV_1   =     0x552D,
	BV_19  =   0xD8722A,
	BV_43  =  0x51955E1,
	BV_87  = 0x18083FB0,
	BV_171 = 0x6A472534,
	BV_255 = 0xFFFFFFFF,
};

static struct gamma_entry {
	u32 brightness;
	u32 v[3];
} gamma_table[] = {
	{       BV_0, { 1000000, 1000000, 1000000, }, },
	{       BV_1, {  951000,  978000,  931000, }, },
	{    0x73701, {  825516,  902183,  830517, }, },
	{   0x418937, {  788233,  811428,  781118, }, },
	{   0xF04C75, {  760795,  767228,  744214, }, },
	{  0x1B4E81B, {  749934,  753360,  728062, }, },
	{  0x2673991, {  740080,  742551,  715727, }, },
	{  0x46508DF, {  723760,  725000,  698000, }, },
	{  0x71529A4, {  710792,  711698,  678151, }, },
	{  0xA3D70A3, {  696667,  699000,  660000, }, },
	{ 0x11111111, {  678333,  680000,  636667, }, },
	{ 0x22222222, {  648333,  650000,  596667, }, },
	{ 0x44444444, {  608333,  610000,  543333, }, },
	{ 0x7FFFFFFF, {  561667,  565000,  476667, }, },
	{     BV_255, {  491667,  493333,  380000, }, },
};

static u32 gamma_lookup(u8 brightness, u32 val, int c)
{
	int i;
	u32 bl = 0;
	u32 bh = 0;
	u32 vl = 0, vh;
	u32 b;
	u32 ret;
	u64 tmp;

	if (!val) {
		b = 0;
	} else {
		tmp = BV_255 - BV_1;
		tmp *= brightness;
		do_div(tmp, 255);

		tmp *= (val - BV_1);
		do_div(tmp, BV_255 - BV_1);
		b = tmp + BV_1;
	}

	for (i = 0; i < ARRAY_SIZE(gamma_table); i++) {
		bl = bh;
		bh = gamma_table[i].brightness;
		if (bh >= b)
			break;
	}
	vh = gamma_table[i].v[c];
	if (i == 0 || (b - bl) == 0) {
		ret = vl = vh;
	} else {
		vl = gamma_table[i - 1].v[c];
		tmp = (u64)vh * (b - bl) + (u64)vl * (bh - b);
		do_div(tmp, bh - bl);
		ret = tmp;
	}

	pr_debug("%s: looking for %3d %08x c %d, %08x, "
		"found %08x:%08x, v %7d:%7d, ret %7d\n",
		__func__, brightness, val, c, b, bl, bh, vl, vh, ret);

	return ret;
}

static void setup_gamma_regs(u16 gamma_regs[], u8 brightness)
{
	int c, i;
	for (c = 0; c < 3; c++) {
		u32 adj;
		u32 v0 = gamma_lookup(brightness, BV_0, c);
		u32 vx[6];
		u32 v1, v255;

		v1 = vx[0] = gamma_lookup(brightness, BV_1, c);
		adj = 600 - 5 - DIV_ROUND_CLOSEST(600 * v1, v0);
		if (adj > 140) {
			pr_err("%s: bad adj value %d, v0 %d, v1 %d, c %d\n",
				__func__, adj, v0, v1, c);
			if ((int)adj < 0)
				adj = 0;
			else
				adj = 140;
		}
		gamma_regs[c] = adj | 0x100;

		v255 = vx[5] = gamma_lookup(brightness, BV_255, c);
		adj = 600 - 120 - DIV_ROUND_CLOSEST(600 * v255, v0);
		if (adj > 380) {
			pr_err("%s: bad adj value %d, v0 %d, v255 %d, c %d\n",
				__func__, adj, v0, v255, c);
			if ((int)adj < 0)
				adj = 0;
			else
				adj = 380;
		}
		gamma_regs[3 * 5 + 2 * c] = adj >> 8 | 0x100;
		gamma_regs[3 * 5 + 2 * c + 1] = (adj & 0xff) | 0x100;

		vx[1] = gamma_lookup(brightness,  BV_19, c);
		vx[2] = gamma_lookup(brightness,  BV_43, c);
		vx[3] = gamma_lookup(brightness,  BV_87, c);
		vx[4] = gamma_lookup(brightness, BV_171, c);

		for (i = 4; i >= 1; i--) {
			if (v1 <= vx[i + 1])
				adj = -1;
			else
				adj = DIV_ROUND_CLOSEST(320 * (v1 - vx[i]),
							v1 - vx[i + 1]) - 65;
			if (adj > 255) {
				pr_err("%s: bad adj value %d, "
					"vh %d, v %d, c %d\n",
					__func__, adj, vx[i + 1], vx[i], c);
				if ((int)adj < 0)
					adj = 0;
				else
					adj = 255;
			}
			gamma_regs[3 * i + c] = adj | 0x100;
		}
	}
}

static int s6e63m0_spi_write_driver(struct s5p_lcd *lcd, u16 reg)
{
	u16 buf[1];
	int ret;
	struct spi_message msg;

	struct spi_transfer xfer = {
		.len	= 2,
		.tx_buf	= buf,
	};

	buf[0] = reg;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);


	ret = spi_sync(lcd->g_spi, &msg);

	if (ret < 0)
		pr_err("%s error\n", __func__);

	return ret ;

}

static void s6e63m0_panel_send_sequence(struct s5p_lcd *lcd,
	const u16 *wbuf)
{
	int i = 0;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC) {
			s6e63m0_spi_write_driver(lcd, wbuf[i]);
			i += 1;
		} else {
			msleep(wbuf[i+1]);
			i += 2;
		}
	}
}

static void update_brightness(struct s5p_lcd *lcd)
{
	u16 gamma_regs[27];

	gamma_regs[0] = 0x0FA;
	gamma_regs[1] = 0x102;
	gamma_regs[23] = 0x0FA;
	gamma_regs[24] = 0x103;
	gamma_regs[25] = ENDDEF;
	gamma_regs[26] = 0x0000;

	setup_gamma_regs(gamma_regs + 2, lcd->bl);
	s6e63m0_panel_send_sequence(lcd, gamma_regs);
}

static void tl2796_ldi_enable(struct s5p_lcd *lcd)
{
	s6e63m0_panel_send_sequence(lcd, s6e63m0_SEQ_DISPLAY_SETTING);
	update_brightness(lcd);
	s6e63m0_panel_send_sequence(lcd, s6e63m0_SEQ_ETC_SETTING);

	lcd->ldi_enable = 1;
}

static void tl2796_ldi_disable(struct s5p_lcd *lcd)
{
	s6e63m0_panel_send_sequence(lcd, s6e63m0_SEQ_STANDBY_ON);
	lcd->ldi_enable = 0;
}

static int s5p_bl_update_status(struct backlight_device *bd)
{

	struct s5p_lcd *lcd = bl_get_data(bd);
	int bl = bd->props.brightness;

	pr_debug("\nupdate status brightness %d\n",
				bd->props.brightness);

	if (bl < 0 || bl > 255)
		return -EINVAL;

	lcd->bl = bl;

	if (!lcd->ldi_enable)
		return 0;

	pr_debug("\n bl :%d\n", bl);

	update_brightness(lcd);

	return 0;
}

const struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
};

void tl2796_early_suspend(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);

	tl2796_ldi_disable(lcd);

	return ;
}
void tl2796_late_resume(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);

	tl2796_ldi_enable(lcd);

	return ;
}
static int __devinit tl2796_probe(struct spi_device *spi)
{
	struct s5p_lcd *lcd;
	int ret;

	lcd = kzalloc(sizeof(struct s5p_lcd), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	spi->bits_per_word = 9;
	if (spi_setup(spi)) {
		pr_err("failed to setup spi\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->g_spi = spi;
	lcd->dev = &spi->dev;
	lcd->bl = 255;
	lcd->bl_dev = backlight_device_register("s5p_bl",
			&spi->dev, lcd, &s5p_bl_ops, NULL);
	if (!lcd->bl_dev) {
		pr_err("failed to register backlight\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev->props.max_brightness = 255;
	spi_set_drvdata(spi, lcd);

	tl2796_ldi_enable(lcd);
#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = tl2796_early_suspend;
	lcd->early_suspend.resume = tl2796_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif
	pr_info("tl2796_probe successfully proved\n");

	return 0;

err_setup:
	kfree(lcd);

err_alloc:
	return ret;
}

static int __devexit tl2796_remove(struct spi_device *spi)
{
	struct s5p_lcd *lcd = spi_get_drvdata(spi);

	unregister_early_suspend(&lcd->early_suspend);

	backlight_device_unregister(lcd->bl_dev);

	tl2796_ldi_disable(lcd);

	kfree(lcd);

	return 0;
}

static struct spi_driver tl2796_driver = {
	.driver = {
		.name	= "tl2796",
		.owner	= THIS_MODULE,
	},
	.probe		= tl2796_probe,
	.remove		= __devexit_p(tl2796_remove),
};

static int __init tl2796_init(void)
{
	return spi_register_driver(&tl2796_driver);
}
static void __exit tl2796_exit(void)
{
	spi_unregister_driver(&tl2796_driver);
}

module_init(tl2796_init);
module_exit(tl2796_exit);
