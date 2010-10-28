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
#include <linux/tl2796.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>
#include <linux/earlysuspend.h>

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DEFMASK		0xFF00

static const struct tl2796_gamma_adj_points default_gamma_adj_points = {
	.v0 = BV_0,
	.v1 = BV_1,
	.v19 = BV_19,
	.v43 = BV_43,
	.v87 = BV_87,
	.v171 = BV_171,
	.v255 = BV_255,
};

struct s5p_lcd{
	int ldi_enable;
	int bl;
	const struct tl2796_gamma_adj_points *gamma_adj_points;
	struct mutex	lock;
	struct device *dev;
	struct spi_device *g_spi;
	struct s5p_panel_data	*data;
	struct backlight_device *bl_dev;
	struct early_suspend    early_suspend;
};

static u32 gamma_lookup(struct s5p_lcd *lcd, u8 brightness, u32 val, int c)
{
	int i;
	u32 bl = 0;
	u32 bh = 0;
	u32 vl = 0;
	u32 vh;
	u32 b;
	u32 ret;
	u64 tmp;
	struct s5p_panel_data *pdata = lcd->data;
	const struct tl2796_gamma_adj_points *bv = lcd->gamma_adj_points;

	if (!val) {
		b = 0;
	} else {
		tmp = bv->v255 - bv->v0;
		tmp *= brightness;
		do_div(tmp, 255);

		tmp *= (val - bv->v0);
		do_div(tmp, bv->v255 - bv->v0);
		b = tmp + bv->v0;
	}

	for (i = 0; i < pdata->gamma_table_size; i++) {
		bl = bh;
		bh = pdata->gamma_table[i].brightness;
		if (bh >= b)
			break;
	}
	vh = pdata->gamma_table[i].v[c];
	if (i == 0 || (b - bl) == 0) {
		ret = vl = vh;
	} else {
		vl = pdata->gamma_table[i - 1].v[c];
		tmp = (u64)vh * (b - bl) + (u64)vl * (bh - b);
		do_div(tmp, bh - bl);
		ret = tmp;
	}

	pr_debug("%s: looking for %3d %08x c %d, %08x, "
		"found %08x:%08x, v %7d:%7d, ret %7d\n",
		__func__, brightness, val, c, b, bl, bh, vl, vh, ret);

	return ret;
}

static void setup_gamma_regs(struct s5p_lcd *lcd, u16 gamma_regs[])
{
	int c, i;
	u8 brightness = lcd->bl;
	const struct tl2796_gamma_adj_points *bv = lcd->gamma_adj_points;

	for (c = 0; c < 3; c++) {
		u32 adj;
		u32 v0 = gamma_lookup(lcd, brightness, BV_0, c);
		u32 vx[6];
		u32 v1;
		u32 v255;

		v1 = vx[0] = gamma_lookup(lcd, brightness, bv->v1, c);
		adj = 600 - 5 - DIV_ROUND_CLOSEST(600 * v1, v0);
		if (adj > 140) {
			pr_debug("%s: bad adj value %d, v0 %d, v1 %d, c %d\n",
				__func__, adj, v0, v1, c);
			if ((int)adj < 0)
				adj = 0;
			else
				adj = 140;
		}
		gamma_regs[c] = adj | 0x100;

		v255 = vx[5] = gamma_lookup(lcd, brightness, bv->v255, c);
		adj = 600 - 120 - DIV_ROUND_CLOSEST(600 * v255, v0);
		if (adj > 380) {
			pr_debug("%s: bad adj value %d, v0 %d, v255 %d, c %d\n",
				__func__, adj, v0, v255, c);
			if ((int)adj < 0)
				adj = 0;
			else
				adj = 380;
		}
		gamma_regs[3 * 5 + 2 * c] = adj >> 8 | 0x100;
		gamma_regs[3 * 5 + 2 * c + 1] = (adj & 0xff) | 0x100;

		vx[1] = gamma_lookup(lcd, brightness,  bv->v19, c);
		vx[2] = gamma_lookup(lcd, brightness,  bv->v43, c);
		vx[3] = gamma_lookup(lcd, brightness,  bv->v87, c);
		vx[4] = gamma_lookup(lcd, brightness, bv->v171, c);

		for (i = 4; i >= 1; i--) {
			if (v1 <= vx[i + 1])
				adj = -1;
			else
				adj = DIV_ROUND_CLOSEST(320 * (v1 - vx[i]),
							v1 - vx[i + 1]) - 65;
			if (adj > 255) {
				pr_debug("%s: bad adj value %d, "
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

	setup_gamma_regs(lcd, gamma_regs + 2);

	s6e63m0_panel_send_sequence(lcd, gamma_regs);
}

static void tl2796_ldi_enable(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;

	mutex_lock(&lcd->lock);

	s6e63m0_panel_send_sequence(lcd, pdata->seq_display_set);
	update_brightness(lcd);
	s6e63m0_panel_send_sequence(lcd, pdata->seq_etc_set);
	lcd->ldi_enable = 1;

	mutex_unlock(&lcd->lock);
}

static void tl2796_ldi_disable(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;

	mutex_lock(&lcd->lock);

	lcd->ldi_enable = 0;
	s6e63m0_panel_send_sequence(lcd, pdata->standby_on);

	mutex_unlock(&lcd->lock);
}

static int s5p_bl_update_status(struct backlight_device *bd)
{
	struct s5p_lcd *lcd = bl_get_data(bd);
	int bl = bd->props.brightness;

	pr_debug("\nupdate status brightness %d\n",
				bd->props.brightness);

	if (bl < 0 || bl > 255)
		return -EINVAL;

	mutex_lock(&lcd->lock);

	lcd->bl = bl;

	if (lcd->ldi_enable) {
		pr_debug("\n bl :%d\n", bl);
		update_brightness(lcd);
	}

	mutex_unlock(&lcd->lock);

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
	mutex_init(&lcd->lock);

	spi->bits_per_word = 9;
	if (spi_setup(spi)) {
		pr_err("failed to setup spi\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->g_spi = spi;
	lcd->dev = &spi->dev;
	lcd->bl = 255;

	if (!spi->dev.platform_data) {
		dev_err(lcd->dev, "failed to get platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}
	lcd->data = (struct s5p_panel_data *)spi->dev.platform_data;

	if (!lcd->data->gamma_table || !lcd->data->seq_display_set ||
		!lcd->data->seq_etc_set || !lcd->data->standby_on ||
		!lcd->data->standby_off) {
		dev_err(lcd->dev, "Invalid platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}
	lcd->gamma_adj_points =
		lcd->data->gamma_adj_points ?: &default_gamma_adj_points;

	lcd->bl_dev = backlight_device_register("s5p_bl",
			&spi->dev, lcd, &s5p_bl_ops, NULL);
	if (!lcd->bl_dev) {
		dev_err(lcd->dev, "failed to register backlight\n");
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
	mutex_destroy(&lcd->lock);
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
