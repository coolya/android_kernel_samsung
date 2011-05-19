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
#define DIM_BL	20
#define MIN_BL	30
#define MAX_BL	255
#define MAX_GAMMA_VALUE	24

extern struct class *sec_class;

/*********** for debug **********************************************************/
#if 0
#define gprintk(fmt, x... ) printk("%s(%d): " fmt, __FUNCTION__ , __LINE__ , ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/

#ifdef CONFIG_FB_S3C_MDNIE
extern void init_mdnie_class(void);
#endif

struct s5p_lcd {
	int ldi_enable;
	int bl;
	int acl_enable;
	int cur_acl;
	int on_19gamma;
	const struct tl2796_gamma_adj_points *gamma_adj_points;
	struct mutex	lock;
	struct device *dev;
	struct spi_device *g_spi;
	struct s5p_panel_data	*data;
	struct backlight_device *bl_dev;
	struct lcd_device *lcd_dev;
	struct class *acl_class;
	struct device *switch_aclset_dev;
	struct class *gammaset_class;
	struct device *switch_gammaset_dev;
	struct device *sec_lcdtype_dev;
	struct early_suspend    early_suspend;
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

static int get_gamma_value_from_bl(int bl)
{
	int gamma_value = 0;
	int gamma_val_x10 = 0;

	if (bl >= MIN_BL)		{
		gamma_val_x10 = 10*(MAX_GAMMA_VALUE-1)*bl/(MAX_BL-MIN_BL) + (10 - 10*(MAX_GAMMA_VALUE-1)*(MIN_BL)/(MAX_BL-MIN_BL)) ;
		gamma_value = (gamma_val_x10+5)/10;
	} else {
		gamma_value = 0;
	}

	return gamma_value;
}

#ifdef CONFIG_FB_S3C_TL2796_ACL
static void update_acl(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;
	int gamma_value;

	gamma_value = get_gamma_value_from_bl(lcd->bl);

	if (lcd->acl_enable) {
		if ((lcd->cur_acl == 0) && (gamma_value != 1)) {
			s6e63m0_panel_send_sequence(lcd, pdata->acl_init);
			msleep(20);
		}

		switch (gamma_value) {
		case 1:
			if (lcd->cur_acl != 0) {
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[0]);
				lcd->cur_acl = 0;
				gprintk(" ACL_cutoff_set Percentage : 0!!\n");
			}
			break;
		case 2 ... 12:
			if (lcd->cur_acl != 40)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[9]);
				lcd->cur_acl = 40;
				gprintk(" ACL_cutoff_set Percentage : 40!!\n");
			}
			break;
		case 13:
			if (lcd->cur_acl != 43)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[16]);
				lcd->cur_acl = 43;
				gprintk(" ACL_cutoff_set Percentage : 43!!\n");
			}
			break;
		case 14:
			if (lcd->cur_acl != 45)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[10]);
				lcd->cur_acl = 45;
				gprintk(" ACL_cutoff_set Percentage : 45!!\n");
			}
			break;
		case 15:
			if (lcd->cur_acl != 47)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[11]);
				lcd->cur_acl = 47;
				gprintk(" ACL_cutoff_set Percentage : 47!!\n");
			}
			break;
		case 16:
			if (lcd->cur_acl != 48)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[12]);
				lcd->cur_acl = 48;
				gprintk(" ACL_cutoff_set Percentage : 48!!\n");
			}
			break;
		default:
			if (lcd->cur_acl != 50)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[13]);
				lcd->cur_acl = 50;
				gprintk(" ACL_cutoff_set Percentage : 50!!\n");
			}
		}
	} else	{
		s6e63m0_panel_send_sequence(lcd, pdata->acl_table[0]);
		lcd->cur_acl  = 0;
		gprintk(" ACL_cutoff_set Percentage : 0!!\n");
	}

}
#endif

static void update_brightness(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;
	int gamma_value;

	gamma_value = get_gamma_value_from_bl(lcd->bl);

	gprintk("Update status brightness[0~255]:(%d) gamma_value:(%d) on_19gamma(%d)\n", lcd->bl, gamma_value, lcd->on_19gamma);

#ifdef CONFIG_FB_S3C_TL2796_ACL
	update_acl(lcd);
#endif
	if (lcd->on_19gamma)
		s6e63m0_panel_send_sequence(lcd, pdata->gamma19_table[gamma_value]);
	else
		s6e63m0_panel_send_sequence(lcd, pdata->gamma22_table[gamma_value]);

	s6e63m0_panel_send_sequence(lcd, pdata->gamma_update);
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


static int s5p_lcd_set_power(struct lcd_device *ld, int power)
{
	struct s5p_lcd *lcd = lcd_get_data(ld);
	struct s5p_panel_data *pdata = lcd->data;

	printk(KERN_DEBUG "s5p_lcd_set_power is called: %d", power);

	if (power)
		s6e63m0_panel_send_sequence(lcd, pdata->display_on);
	else
		s6e63m0_panel_send_sequence(lcd, pdata->display_off);

	return 0;
}

static int s5p_lcd_check_fb(struct lcd_device *lcddev, struct fb_info *fi)
{
	return 0;
}

struct lcd_ops s5p_lcd_ops = {
	.set_power = s5p_lcd_set_power,
	.check_fb = s5p_lcd_check_fb,
};

static ssize_t gammaset_file_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);

	gprintk("called %s\n", __func__);

	return sprintf(buf, "%u\n", lcd->bl);
}
static ssize_t gammaset_file_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);

	if ((lcd->ldi_enable) && ((value == 0) || (value == 1))) {
		printk("[gamma set] in gammaset_file_cmd_store, input value = %d\n", value);
		if (value != lcd->on_19gamma)	{
			lcd->on_19gamma = value;
			update_brightness(lcd);
		}
	}

	return size;
}

static DEVICE_ATTR(gammaset_file_cmd, 0664, gammaset_file_cmd_show, gammaset_file_cmd_store);

static ssize_t aclset_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);
	gprintk("called %s\n", __func__);

	return sprintf(buf, "%u\n", lcd->acl_enable);
}
static ssize_t aclset_file_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);

	if ((lcd->ldi_enable) && ((value == 0) || (value == 1))) {
		printk(KERN_INFO "[acl set] in aclset_file_cmd_store, input value = %d\n", value);
		if (lcd->acl_enable != value) {
			lcd->acl_enable = value;
#ifdef CONFIG_FB_S3C_TL2796_ACL
			update_acl(lcd);
#endif
		}
	}

	return size;
}

static DEVICE_ATTR(aclset_file_cmd, 0664, aclset_file_cmd_show, aclset_file_cmd_store);

static ssize_t lcdtype_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "SMD_AMS397GE03\n");
}

static ssize_t lcdtype_file_cmd_store(
        struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    return size;
}
static DEVICE_ATTR(lcdtype_file_cmd, 0664, lcdtype_file_cmd_show, lcdtype_file_cmd_store);

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


static int s5p_bl_get_brightness(struct backlight_device *bd)
{
	struct s5p_lcd *lcd = bl_get_data(bd);

	printk(KERN_DEBUG "\n reading brightness\n");

	return lcd->bl;
}

const struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,
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

	if (!lcd->data->gamma19_table || !lcd->data->gamma19_table ||
		!lcd->data->seq_display_set || !lcd->data->seq_etc_set ||
		!lcd->data->display_on || !lcd->data->display_off ||
		!lcd->data->standby_on || !lcd->data->standby_off ||
		!lcd->data->acl_init || !lcd->data->acl_table ||
		!lcd->data->gamma_update) {
		dev_err(lcd->dev, "Invalid platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev = backlight_device_register("s5p_bl",
			&spi->dev, lcd, &s5p_bl_ops, NULL);
	if (!lcd->bl_dev) {
		dev_err(lcd->dev, "failed to register backlight\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev->props.max_brightness = 255;

	lcd->lcd_dev = lcd_device_register("s5p_lcd",
			&spi->dev, lcd, &s5p_lcd_ops);
	if (!lcd->lcd_dev) {
		dev_err(lcd->dev, "failed to register lcd\n");
		ret = -EINVAL;
		goto err_setup_lcd;
	}

	lcd->gammaset_class = class_create(THIS_MODULE, "gammaset");
	if (IS_ERR(lcd->gammaset_class))
		pr_err("Failed to create class(gammaset_class)!\n");

	lcd->switch_gammaset_dev = device_create(lcd->gammaset_class, &spi->dev, 0, lcd, "switch_gammaset");
	if (!lcd->switch_gammaset_dev) {
		dev_err(lcd->dev, "failed to register switch_gammaset_dev\n");
		ret = -EINVAL;
		goto err_setup_gammaset;
	}

	if (device_create_file(lcd->switch_gammaset_dev, &dev_attr_gammaset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gammaset_file_cmd.attr.name);

	lcd->acl_enable = 0;
	lcd->cur_acl = 0;

	lcd->acl_class = class_create(THIS_MODULE, "aclset");
	if (IS_ERR(lcd->acl_class))
		pr_err("Failed to create class(acl_class)!\n");

	lcd->switch_aclset_dev = device_create(lcd->acl_class, &spi->dev, 0, lcd, "switch_aclset");
	if (IS_ERR(lcd->switch_aclset_dev))
		pr_err("Failed to create device(switch_aclset_dev)!\n");

	if (device_create_file(lcd->switch_aclset_dev, &dev_attr_aclset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_aclset_file_cmd.attr.name);

	 if (sec_class == NULL)
	 	sec_class = class_create(THIS_MODULE, "sec");
	 if (IS_ERR(sec_class))
                pr_err("Failed to create class(sec)!\n");

	 lcd->sec_lcdtype_dev = device_create(sec_class, NULL, 0, NULL, "sec_lcd");
	 if (IS_ERR(lcd->sec_lcdtype_dev))
	 	pr_err("Failed to create device(ts)!\n");

	 if (device_create_file(lcd->sec_lcdtype_dev, &dev_attr_lcdtype_file_cmd) < 0)
	 	pr_err("Failed to create device file(%s)!\n", dev_attr_lcdtype_file_cmd.attr.name);
	
#ifdef CONFIG_FB_S3C_MDNIE
	init_mdnie_class();  //set mDNIe UI mode, Outdoormode
#endif

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
err_setup_gammaset:
	lcd_device_unregister(lcd->lcd_dev);

err_setup_lcd:
	backlight_device_unregister(lcd->bl_dev);

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


MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("s6e63m0 LDI driver");
MODULE_LICENSE("GPL");
