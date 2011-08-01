/*
 * nt35580 TFT Panel Driver for the Samsung Universal board
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
#include <linux/nt35580.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>
#include <linux/earlysuspend.h>

#define NT35580_POWERON_DELAY	150

struct s5p_lcd {
	int ldi_enable;
	int bl;
	struct mutex lock;
	struct device *dev;
	struct spi_device *g_spi;
	struct s5p_tft_panel_data *data;
	struct backlight_device *bl_dev;
	struct early_suspend early_suspend;
};

static int nt35580_spi_write_driver(struct s5p_lcd *lcd, u16 reg)
{
	u16 buf;
	int ret;
	struct spi_message msg;

	struct spi_transfer xfer = {
		.len	= 2,
		.tx_buf	= &buf,
	};

	buf = reg;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(lcd->g_spi, &msg);

	if (ret < 0)
		pr_err("%s: error: spi_sync (%d)", __func__, ret);

	return ret;
}

static void nt35580_panel_send_sequence(struct s5p_lcd *lcd,
	const u16 *wbuf)
{
	int i = 0;

	while ((wbuf[i] & DEFMASK) != ENDDEF)
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC) {
			nt35580_spi_write_driver(lcd, wbuf[i]);
			i += 1;
		} else {
			msleep(wbuf[i+1]);
			i += 2;
		}
}

static void update_brightness(struct s5p_lcd *lcd, int level)
{
	struct s5p_tft_panel_data *pdata = lcd->data;

	pdata->brightness_set[pdata->pwm_reg_offset] = 0x100 | (level & 0xff);

	nt35580_panel_send_sequence(lcd, pdata->brightness_set);
}

static void nt35580_ldi_enable(struct s5p_lcd *lcd)
{
	struct s5p_tft_panel_data *pdata = lcd->data;

	mutex_lock(&lcd->lock);

	msleep(NT35580_POWERON_DELAY);

	nt35580_panel_send_sequence(lcd, pdata->seq_set);
	update_brightness(lcd, lcd->bl);
	nt35580_panel_send_sequence(lcd, pdata->display_on);

	lcd->ldi_enable = 1;

	mutex_unlock(&lcd->lock);
}

static void nt35580_ldi_disable(struct s5p_lcd *lcd)
{
	struct s5p_tft_panel_data *pdata = lcd->data;

	mutex_lock(&lcd->lock);

	lcd->ldi_enable = 0;
	nt35580_panel_send_sequence(lcd, pdata->display_off);
	nt35580_panel_send_sequence(lcd, pdata->sleep_in);

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
		update_brightness(lcd, bl);
	}

	mutex_unlock(&lcd->lock);

	return 0;
}

const struct backlight_ops s5p_bl_tft_ops = {
	.update_status = s5p_bl_update_status,
};

void nt35580_early_suspend(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);

	nt35580_ldi_disable(lcd);

	return;
}
void nt35580_late_resume(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);

	nt35580_ldi_enable(lcd);

	return;
}
static int __devinit nt35580_probe(struct spi_device *spi)
{
	struct s5p_lcd *lcd;
	int ret;

	lcd = kzalloc(sizeof(*lcd), GFP_KERNEL);
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
	lcd->data = (struct s5p_tft_panel_data *)spi->dev.platform_data;

	if (!lcd->data->seq_set || !lcd->data->display_on ||
		!lcd->data->display_off || !lcd->data->sleep_in ||
		!lcd->data->brightness_set || !lcd->data->pwm_reg_offset) {
		dev_err(lcd->dev, "Invalid platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev = backlight_device_register("s5p_bl",
			&spi->dev, lcd, &s5p_bl_tft_ops, NULL);
	if (!lcd->bl_dev) {
		dev_err(lcd->dev, "failed to register backlight\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev->props.max_brightness = 255;
	spi_set_drvdata(spi, lcd);

	lcd->ldi_enable = 1;
#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = nt35580_early_suspend;
	lcd->early_suspend.resume = nt35580_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif
	pr_info("%s successfully probed\n", __func__);

	return 0;

err_setup:
	mutex_destroy(&lcd->lock);
	kfree(lcd);

err_alloc:
	return ret;
}

static int __devexit nt35580_remove(struct spi_device *spi)
{
	struct s5p_lcd *lcd = spi_get_drvdata(spi);

	unregister_early_suspend(&lcd->early_suspend);

	backlight_device_unregister(lcd->bl_dev);

	nt35580_ldi_disable(lcd);

	kfree(lcd);

	return 0;
}

static struct spi_driver nt35580_driver = {
	.driver = {
		.name	= "nt35580",
		.owner	= THIS_MODULE,
	},
	.probe		= nt35580_probe,
	.remove		= __devexit_p(nt35580_remove),
};

static int __init nt35580_init(void)
{
	return spi_register_driver(&nt35580_driver);
}
static void __exit nt35580_exit(void)
{
	spi_unregister_driver(&nt35580_driver);
}

module_init(nt35580_init);
module_exit(nt35580_exit);
