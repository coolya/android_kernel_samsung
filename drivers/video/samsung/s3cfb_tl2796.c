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

const u16 s6e63m0_SEQ_SETTING[] = {
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
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x15F,
	0x150,
	0x12D,
	0x1B6,
	0x1B9,
	0x1A7,
	0x1AD,
	0x1B1,
	0x19F,
	0x1BE,
	0x1C0,
	0x1B5,
	0x100,
	0x1A0,
	0x100,
	0x1A4,
	0x100,
	0x1DB,
	0x0FA,
	0x103,
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

const u16 s6e63m0_22gamma_300cd[] = {
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x150,
	0x14E,
	0x12B,
	0x1B0,
	0x1B7,
	0x1A5,
	0x1AB,
	0x1B1,
	0x1A0,
	0x1C0,
	0x1C2,
	0x1B7,
	0x100,
	0x1CD,
	0x100,
	0x1C5,
	0x101,
	0x10C,
	0x0FA,
	0x103,
	ENDDEF, 0x0000
};

const u16 s6e63m0_22gamma_260cd[] = {
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x151,
	0x14E,
	0x12B,
	0x1B0,
	0x1B9,
	0x1A6,
	0x1AD,
	0x1B4,
	0x1A1,
	0x1C2,
	0x1C4,
	0x1B8,
	0x100,
	0x1C3,
	0x100,
	0x1B9,
	0x101,
	0x100,
	0x0FA,
	0x103,
	ENDDEF, 0x0000
};

const u16 s6e63m0_22gamma_220cd[] = {
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x151,
	0x14E,
	0x12D,
	0x1B4,
	0x1B9,
	0x1A8,
	0x1AF,
	0x1B5,
	0x1A4,
	0x1C2,
	0x1C8,
	0x1B9,
	0x100,
	0x1B7,
	0x100,
	0x1AC,
	0x100,
	0x1EF,
	0x0FA,
	0x103,
	ENDDEF, 0x0000
};

const u16 s6e63m0_22gamma_180cd[] = {
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x154,
	0x14B,
	0x130,
	0x1B4,
	0x1BB,
	0x1A9,
	0x1B1,
	0x1B7,
	0x1A6,
	0x1C4,
	0x1CA,
	0x1BB,
	0x100,
	0x1AA,
	0x100,
	0x19F,
	0x100,
	0x1DD,
	0x0FA,
	0x103,
	ENDDEF, 0x0000
};

const u16 s6e63m0_22gamma_140cd[] = {
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x15A,
	0x148,
	0x133,
	0x1B7,
	0x1BC,
	0x1AC,
	0x1B2,
	0x1B9,
	0x1A8,
	0x1C8,
	0x1CD,
	0x1BF,
	0x100,
	0x19A,
	0x100,
	0x190,
	0x100,
	0x1C7,
	0x0FA,
	0x103,
	ENDDEF, 0x0000
};

const u16 s6e63m0_22gamma_100cd[] = {
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x161,
	0x140,
	0x13A,
	0x1B9,
	0x1BE,
	0x1AD,
	0x1B5,
	0x1BB,
	0x1AB,
	0x1CB,
	0x1D1,
	0x1C3,
	0x100,
	0x188,
	0x100,
	0x17F,
	0x100,
	0x1B0,
	0x0FA,
	0x103,
	ENDDEF, 0x0000
};

const u16 s6e63m0_22gamma_50cd[] = {
	0x0FA,
	0x102,
	0x118,
	0x108,
	0x124,
	0x16E,
	0x11A,
	0x13A,
	0x1BD,
	0x1BE,
	0x1B2,
	0x1BB,
	0x1C0,
	0x1B1,
	0x1CF,
	0x1D5,
	0x1C8,
	0x100,
	0x16A,
	0x100,
	0x163,
	0x100,
	0x189,
	0x0FA,
	0x103,
	ENDDEF, 0x0000
};

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
static void tl2796_ldi_enable(struct s5p_lcd *lcd)
{
	s6e63m0_panel_send_sequence(lcd, s6e63m0_SEQ_SETTING);
	s6e63m0_panel_send_sequence(lcd, s6e63m0_SEQ_STANDBY_OFF);
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
	int bl = 255 - bd->props.brightness;

	pr_debug("\nupdate status brightness %d\n",
				bd->props.brightness);
	if (!lcd->ldi_enable)
		return 0;

	pr_debug("\n bl :%d\n", bl);

	switch (bl) {
	case  0 ... 50:
		s6e63m0_panel_send_sequence
				(lcd, s6e63m0_22gamma_300cd);
		break;
	case  51 ... 100:
		s6e63m0_panel_send_sequence
				(lcd, s6e63m0_22gamma_260cd);
		break;
	case  101 ... 150:
		s6e63m0_panel_send_sequence
				(lcd, s6e63m0_22gamma_180cd);
		break;
	case  151 ... 200:
		s6e63m0_panel_send_sequence
				(lcd, s6e63m0_22gamma_140cd);
		break;
	case  201 ... 230:
		s6e63m0_panel_send_sequence
				(lcd, s6e63m0_22gamma_100cd);
		break;
	case  231 ... 255:
		s6e63m0_panel_send_sequence
				(lcd, s6e63m0_22gamma_50cd);
		break;
	default:
		return -EINVAL;
	}
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
