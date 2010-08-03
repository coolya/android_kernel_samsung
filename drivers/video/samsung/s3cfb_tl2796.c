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

#include "s3cfb.h"

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

static int locked = 0;
static int ldi_enable = 0;


struct s5p_lcd{
	struct spi_device *g_spi;
	struct lcd_device *lcd_dev;
	struct backlight_device *bl_dev;

};

static struct s5p_lcd lcd;

const unsigned short s6e63m0_SEQ_DISPLAY_ON[] = {
	//insert

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_DISPLAY_OFF[] = {
	//insert

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_STANDBY_ON[] = {
	0x010,
	SLEEPMSEC, 120,
	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_STANDBY_OFF[] = {
	0x011,
	SLEEPMSEC, 	120,
	ENDDEF, 0x0000
};

const unsigned short s6e63m0_SEQ_SETTING[] = {
	SLEEPMSEC, 120,
	//panel setting
	0x0F8,
	0x101,	0x127,
	0x127,	0x107,
	0x107,	0x154,
	0x19F,	0x163,
	0x186,	0x11A,
	0x133,	0x10D,
	0x100,	0x100,
	
	//display condition set
	0x0F2,
	0x102,	0x103,
	0x11C,	0x110,
	0x110,
	
	0x0F7,
	0x103,	0x100,	
	0x100,
	
	//gamma set
	0x0FA,
	
	0x102,
	#if 0
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
	#else
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
	#endif


	//gamma update
	0x0FA,	
	0x103,

	
	//etc condition set
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
	
	//stand by off cmd
	0x011,

	SLEEPMSEC, 120,

	//Display on
	0x029,

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_300cd[] = {
	//gamma set
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


	//gamma update
	0x0FA,	
	0x103,

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_260cd[] = {
	//gamma set
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

	
	//gamma update
	0x0FA,	
	0x103,

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_220cd[] = {
	//gamma set
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

	
	//gamma update
	0x0FA,	
	0x103,

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_180cd[] = {
	//gamma set
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

	
	//gamma update
	0x0FA,	
	0x103,

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_140cd[] = {
	//gamma set
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

	
	//gamma update
	0x0FA,	
	0x103,

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_100cd[] = {
	//gamma set
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

	
	//gamma update
	0x0FA,	
	0x103,

	ENDDEF, 0x0000
};

const unsigned short s6e63m0_22gamma_50cd[] = {
	//gamma set
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

	
	//gamma update
	0x0FA,	
	0x103,

	ENDDEF, 0x0000
};

static struct s3cfb_lcd s6e63m0 = {
	.width = 480,
	.height = 800,
	.p_width = 52,
	.p_height = 86,
	.bpp = 24,
	.freq = 60,
	
	.timing = {
		.h_fp = 16,
		.h_bp = 16,
		.h_sw = 2,
		.v_fp = 28,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};


static int s6e63m0_spi_write_driver(int reg)
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

	//locked	= 1;
	ret = spi_sync(lcd.g_spi, &msg);
	//locked = 0;
	if(ret < 0)
	{
		//err("%s::%d -> spi_sync failed Err=%d\n",__func__,__LINE__,ret);
		printk("s6e63m0_spi_write_driver error\n");
	}
	return ret ;

}


static void s6e63m0_spi_write(unsigned short reg)
{
  	s6e63m0_spi_write_driver(reg);	
}

static void s6e63m0_panel_send_sequence(const unsigned short *wbuf)
{
	int i = 0;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC){
			s6e63m0_spi_write(wbuf[i]);
			i+=1;}
		else{
			msleep(wbuf[i+1]);
			i+=2;}
	}
}


void tl2796_ldi_init(void)
{
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_SETTING);
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_STANDBY_OFF);
	ldi_enable = 1;
	dev_dbg(&lcd.lcd_dev->dev,"%s::%d -> ldi initialized\n",__func__,__LINE__);	
}


void tl2796_ldi_enable(void)
{
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_SETTING);
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_STANDBY_OFF);
	ldi_enable = 1;
	dev_dbg(&lcd.lcd_dev->dev,"%s::%d -> ldi enabled\n",__func__,__LINE__);	
}

void tl2796_ldi_disable(void)
{
	s6e63m0_panel_send_sequence(s6e63m0_SEQ_STANDBY_ON);
	ldi_enable = 0;
	dev_dbg(&lcd.lcd_dev->dev,"%s::%d -> ldi disabled\n",__func__,__LINE__);	
}

void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	s6e63m0.init_ldi = NULL;
	ctrl->lcd = &s6e63m0;
}

//mkh:lcd operations and functions
static int s5p_lcd_set_power(struct lcd_device *ld,int power)
{
	//TODO: Implement
	return 0;//pkd@ to remove compile time warning message
}

static struct lcd_ops s5p_lcd_ops = {
	.set_power = s5p_lcd_set_power,
};

//mkh:backlight operations and functions
static int s5p_bl_update_status(struct backlight_device* bd)
{
	int bl = 255 - bd->props.brightness;	
	int level = 0;
//	printk("\nupdate status brightness %d \n",bd->props.brightness);

	if((!locked)&&(ldi_enable==1))
	{
		locked	= 1;
		
		if((bl <= 255) && (bl > 230))
			level = 1;
		else if((bl <= 230) && (bl > 200))
			level = 6;
		else if((bl <= 200) && (bl > 150))
			level = 7;
		else if((bl <= 150) && (bl > 100))
			level = 8;
		else if((bl <= 100) && (bl > 50))
			level = 9;
		else if((bl <= 50) && (bl >= 0))
			level = 10;
		else{
			printk(KERN_WARNING "\n bl(%d) is wrong \n", bl);
		}

//		printk("\n bl :%d level :%d \n", bl, level);

	 	if(level)
		{
			switch(level)
			{
				case  5:
				case  4:
				case  3:
				case  2:
				case  1: //dimming
				{	
					s6e63m0_panel_send_sequence(s6e63m0_22gamma_50cd);
					break;
				}
				case  6:
				{	
					s6e63m0_panel_send_sequence(s6e63m0_22gamma_100cd);	
					break;
				}
				case  7:
				{
					s6e63m0_panel_send_sequence(s6e63m0_22gamma_140cd);
					break;
				}
				case  8:
				{	
					s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);	
					break;
				}
				case  9:
				{	
					s6e63m0_panel_send_sequence(s6e63m0_22gamma_260cd);	
					break;
				}
				case  10:
				{
					s6e63m0_panel_send_sequence(s6e63m0_22gamma_300cd);
					break;
				}
			}
		}
		locked	= 0;
	}
//	else
//	{
//		printk("\n brightness is updating~~!!!!!!!!! skip~!!   locked : %d ~ ldi_enable : %d\n", locked,ldi_enable);
//	}
	
	return 0;
}

void lightsensor_backlight_level_ctrl(int value)
{
	printk("\n lightsensor_backlight_level_ctrl is called  : level(%d)\n",value);

	if(value > 21 || value < 1)
		return;

	switch(value){
		case  21: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
	
		case  20: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  19: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  18: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  17: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}		
		case  16: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  15: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  14: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  13: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  12: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  11: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  10: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  9: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}		
		case  8: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  7: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  6: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  5: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
	
		case  4: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  3: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  2: 
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
		case  1:
		{
			s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		}
	}
}

EXPORT_SYMBOL(lightsensor_backlight_level_ctrl);


static int s5p_bl_get_brightness(struct backlight_device* bd)
{
//	printk("\n reading brightness \n");
	return 0;
}

static struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,	
};

static int __init tl2796_probe(struct spi_device *spi)
{
	int ret;
	
	spi->bits_per_word = 9;
	ret = spi_setup(spi);
	lcd.g_spi = spi;
	lcd.lcd_dev = lcd_device_register("s5p_lcd",&spi->dev,&lcd,&s5p_lcd_ops);
	lcd.bl_dev = backlight_device_register("s5p_bl",&spi->dev,&lcd,&s5p_bl_ops, NULL);
	lcd.bl_dev->props.max_brightness = 255;
	dev_set_drvdata(&spi->dev,&lcd);
	
	tl2796_ldi_init();
	tl2796_ldi_enable();
	
	if (ret < 0){
		//err("%s::%d-> s6e63m0 probe failed Err=%d\n",__func__,__LINE__,ret);
		return 0;
	}
	printk("tl2796_probe successfully proved \n");
	//info("%s::%d->s6e63m0 probed successfuly\n",__func__,__LINE__);
	return ret;
}

#ifdef CONFIG_PM // add by ksoo (2009.09.07)
int tl2796_suspend(struct platform_device *pdev, pm_message_t state)
{
	//info("%s::%d->s6e63m0 suspend called\n",__func__,__LINE__);
	tl2796_ldi_disable();
	return 0;
}

int tl2796_resume(struct platform_device *pdev, pm_message_t state)
{
//	info("%s::%d -> s6e63m0 resume called\n",__func__,__LINE__);
	tl2796_ldi_init();
	tl2796_ldi_enable();

	return 0;
}
#endif

static struct spi_driver tl2796_driver = {
	.driver = {
		.name	= "tl2796",
		.owner	= THIS_MODULE,
	},
	.probe		= tl2796_probe,
	.remove		= __exit_p(tl2796_remove),
//#ifdef CONFIG_PM
//	.suspend		= tl2796_suspend,
//	.resume		= tl2796_resume,
//#else
	.suspend		= NULL,
	.resume		= NULL,
//#endif
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
