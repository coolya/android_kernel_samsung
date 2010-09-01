/*
 * ams701ka AMOLED Panel Driver for the Samsung Universal board
 *
 * Derived from drivers/video/samsung/s3cfb_tl2796.c
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

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>

#include "s3cfb.h"

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

const unsigned short SEQ_DISPLAY_ON[] = {
        0x2e, 0x0607,
        SLEEPMSEC, 1,
        0x30, 0x230D,
        0x31, 0x161E,
        ENDDEF, 0x0000
};

const unsigned short SEQ_MANUAL_DISPLAY_ON[] = {
        0x12, 0x0001,
        SLEEPMSEC, 1,
        0x12, 0x0003,
        SLEEPMSEC, 1,
        0x12, 0x0007,
        SLEEPMSEC, 1,
        0x2e, 0x0605,
        SLEEPMSEC, 1,
        0x30, 0x2307,
        0x31, 0x161E,
        SLEEPMSEC, 5,
        0x12, 0x000f,
        SLEEPMSEC, 10,
        0x12, 0x001f,
        SLEEPMSEC, 10,

        0xf4, 0x0003,

        ENDDEF, 0x0000
};

const unsigned short SEQ_DISPLAY_OFF[] = {
        0x12, 0x000f,
        0x12, 0x0007,
        SLEEPMSEC, 1,   /* FIXME */
        0x12, 0x0001,
        0x12, 0x0000,
        SLEEPMSEC, 1,   /* FIXME */
        ENDDEF, 0x0000
};

const unsigned short SEQ_STANDBY_ON[] = {
        0x02, 0x2301,

        ENDDEF, 0x0000
};

const unsigned short SEQ_STANDBY_OFF[] = {
        0x02, 0x2300,

        ENDDEF, 0x0000
};

const unsigned short GAMMA_SETTING[] = {

#if 0
        /* Low Red Gamma */
        0x4c, 0xc209,
        0x4d, 0xdac8,
        0x4e, 0xc4bf,
        0x4f, 0x0093,

        /* Low Green Gamma */
        0x50, 0xad04,
        0x51, 0xd1c5,
        0x52, 0xbfc8,
        0x53, 0x009d,

        /* Low Blue Gamma */
        0x54, 0xbb09,
        0x55, 0xd2c9,
        0x56, 0xbcc4,
        0x57, 0x00b7,
#endif
#if 0 //SLP1
        /* High Red Gamma */
        0x40, 0xCA09, 0x41, 0xD9C9, 0x42, 0xC3B5, 0x43, 0x00A7,

        /* High Green Gamma */
        0x44, 0xBC04, 0x45, 0xD4C5, 0x46, 0xC0B1, 0x47, 0x00C8,

        /* High Blue Gamma */
        0x48, 0xC209, 0x49, 0xD3C1, 0x4a, 0xBCAC, 0x4b, 0x00D8,
#else //SLP2
        /* High Red Gamma */
        0x40, 0xC809, 0x41, 0xD9C9, 0x42, 0xC3B5, 0x43, 0x00A6,

        /* High Green Gamma */
        0x44, 0xBD04, 0x45, 0xD4C5, 0x46, 0xBEB0, 0x47, 0x00C7,

        /* High Blue Gamma */
        0x48, 0xBD09, 0x49, 0xD2C1, 0x4a, 0xBCAC, 0x4b, 0x00D7,
#endif

        0x3f, 0x0004,   /* gamma setting : ams701ka */
	0x32, 0x0002,
        ENDDEF, 0x0000
};

const unsigned short MANUAL_POWER_ON_SETTING[] = {
        0x06, 0x0000,

        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x0001,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x0005,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x0007,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x000f,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x001f,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x003f,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x007f,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x00ff,
        SLEEPMSEC, 30,  /* FIXME */
        0x06, 0x08ff,
        SLEEPMSEC, 30,  /* FIXME */

        0x03, 0x134A,   /* ETC Register setting */
        0x04, 0x86a4,   /* LTPS Power on setting VCIR=2.7V Display is not clean */

        0x14, 0x0808,   /* VFP, VBP Register setting */
        0x15, 0x3090,   /* HSW,HFP,HBP Register setting */
        ENDDEF, 0x0000

};

const unsigned short MANUAL_POWER_OFF_SETTING[] = {
	0x06, 0x8FF,
	SLEEPMSEC, 1,
	0x06, 0x3FF,
	SLEEPMSEC, 5,
	0x06, 0x07F,
	SLEEPMSEC, 5,
	0x06, 0x03F,
	SLEEPMSEC, 5,
	0x06, 0x01F,
	SLEEPMSEC, 5,
	0x06, 0x00F,
	SLEEPMSEC, 5,
	0x06, 0x007,
	SLEEPMSEC, 5,
	0x06, 0x003,
	SLEEPMSEC, 5,
	0x06, 0x001,
	SLEEPMSEC, 5,
	0x06, 0x000,
	SLEEPMSEC, 5,

	ENDDEF, 0x0000
};

const unsigned short SEQ_SLEEP_OUT[] = {
        0x02, 0x2300,           /* Sleep Out */
        SLEEPMSEC, 1,   /* FIXME */
        ENDDEF, 0x0000
};

const unsigned short ACL_ON_DISPLAY_SETTING[] = {
        0x5b, 0x0013,
        0x5c, 0x0000,
        0x5d, 0x03ff,
        0x5e, 0x0000,
        0x5f, 0x0257,
        ENDDEF, 0x0000
};

const unsigned short ACL_ON_WINDOW_SETTING[] = {
        0x5b, 0x0013,
        0x5c, 0x0200,
        0x5d, 0x03ff,
        0x5e, 0x0000,
        0x5f, 0x0257,
        ENDDEF, 0x0000
};

const unsigned short GAMMA22_SETTINGS[][30] = {
	// Luminance : 70nit
	{ 0x40, 0xD309, 0x41, 0xDDDD, 0x42, 0xD3C1, 0x43, 0x0061,	/* High Red Gamma */
	  0x44, 0x9804, 0x45, 0xDAD1, 0x46, 0xCEBD, 0x47, 0x0075,	/* High Green Gamma */
	  0x48, 0xC509, 0x49, 0xDBD3, 0x4a, 0xCCBC, 0x4b, 0x007F,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 82nit
	{ 0x40, 0xCF09, 0x41, 0xDDDC, 0x42, 0xD1C1, 0x43, 0x0067,	/* High Red Gamma */
	  0x44, 0x9A04, 0x45, 0xDCD0, 0x46, 0xCDBC, 0x47, 0x007C,	/* High Green Gamma */
	  0x48, 0xC109, 0x49, 0xDCD2, 0x4a, 0xCBBA, 0x4b, 0x0087,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 94nit
	{ 0x40, 0xCF09, 0x41, 0xDCDC, 0x42, 0xD2BF, 0x43, 0x006C,	/* High Red Gamma */
	  0x44, 0xA804, 0x45, 0xDAD2, 0x46, 0xCCBB, 0x47, 0x0083,	/* High Green Gamma */
	  0x48, 0xC309, 0x49, 0xDAD4, 0x4a, 0xCAB9, 0x4b, 0x008E,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 106nit
	{ 0x40, 0xCD09, 0x41, 0xDCDA, 0x42, 0xD0BE, 0x43, 0x0072,	/* High Red Gamma */
	  0x44, 0xA904, 0x45, 0xDAD0, 0x46, 0xCABA, 0x47, 0x008B,	/* High Green Gamma */
	  0x48, 0xC209, 0x49, 0xDAD1, 0x4a, 0xC8B8, 0x4b, 0x0096,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 118nit
	{ 0x40, 0xCF09, 0x41, 0xDBDA, 0x42, 0xCFBE, 0x43, 0x0077,	/* High Red Gamma */
	  0x44, 0xAD04, 0x45, 0xDAD1, 0x46, 0xCAB9, 0x47, 0x0090,	/* High Green Gamma */
	  0x48, 0xC309, 0x49, 0xD9D2, 0x4a, 0xC9B7, 0x4b, 0x009B,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 130nit
	{ 0x40, 0xCD09, 0x41, 0xDCD9, 0x42, 0xCDBD, 0x43, 0x007C,	/* High Red Gamma */
	  0x44, 0xAE04, 0x45, 0xDBD1, 0x46, 0xC7B8, 0x47, 0x0097,	/* High Green Gamma */
	  0x48, 0xC209, 0x49, 0xDAD1, 0x4a, 0xC6B6, 0x4b, 0x00A2,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 142nit
	{ 0x40, 0xCD09, 0x41, 0xDAD9, 0x42, 0xCEBB, 0x43, 0x0080,	/* High Red Gamma */
	  0x44, 0xB104, 0x45, 0xDAD1, 0x46, 0xC7B7, 0x47, 0x009C,	/* High Green Gamma */
	  0x48, 0xC209, 0x49, 0xDAD1, 0x4a, 0xC6B4, 0x4b, 0x00A8,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 154nit
	{ 0x40, 0xCB09, 0x41, 0xDAD9, 0x42, 0xCCBB, 0x43, 0x0085,	/* High Red Gamma */
	  0x44, 0xB104, 0x45, 0xD9D1, 0x46, 0xC7B6, 0x47, 0x00A1,	/* High Green Gamma */
	  0x48, 0xC109, 0x49, 0xD9D1, 0x4a, 0xC5B3, 0x4b, 0x00AE,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 166nit
	{ 0x40, 0xCD09, 0x41, 0xD9D9, 0x42, 0xCBBB, 0x43, 0x0089,	/* High Red Gamma */
	  0x44, 0xB704, 0x45, 0xD9D1, 0x46, 0xC6B6, 0x47, 0x00A6,	/* High Green Gamma */
	  0x48, 0xC309, 0x49, 0xD9D0, 0x4a, 0xC4B3, 0x4b, 0x00B3,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 178nit
	{ 0x40, 0xCB09, 0x41, 0xDAD8, 0x42, 0xCABA, 0x43, 0x008D,	/* High Red Gamma */
	  0x44, 0xB304, 0x45, 0xDAD0, 0x46, 0xC5B4, 0x47, 0x00AC,	/* High Green Gamma */
	  0x48, 0xC109, 0x49, 0xD9D0, 0x4a, 0xC3B2, 0x4b, 0x00B8,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 190nit
	{ 0x40, 0xCA09, 0x41, 0xDAD8, 0x42, 0xC9B9, 0x43, 0x0091,	/* High Red Gamma */
	  0x44, 0xB404, 0x45, 0xD8D0, 0x46, 0xC4B4, 0x47, 0x00B1,	/* High Green Gamma */
	  0x48, 0xC009, 0x49, 0xD8D0, 0x4a, 0xC3B1, 0x4b, 0x00BD,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 202nit
	{ 0x40, 0xCA09, 0x41, 0xD9D8, 0x42, 0xC8B9, 0x43, 0x0095,	/* High Red Gamma */
	  0x44, 0xB504, 0x45, 0xD9D1, 0x46, 0xC3B4, 0x47, 0x00B5,	/* High Green Gamma */
	  0x48, 0xC009, 0x49, 0xD8D0, 0x4a, 0xC2B1, 0x4b, 0x00C1,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 214nit
	{ 0x40, 0xCB09, 0x41, 0xDB07, 0x42, 0xC8B6, 0x43, 0x0099,	/* High Red Gamma */
	  0x44, 0xB704, 0x45, 0xD9D0, 0x46, 0xC2B2, 0x47, 0x00BA,	/* High Green Gamma */
	  0x48, 0xC209, 0x49, 0xD9CF, 0x4a, 0xC0AF, 0x4b, 0x00C8,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 226nit
	{ 0x40, 0xC909, 0x41, 0xD9D7, 0x42, 0xC7B7, 0x43, 0x009D,	/* High Red Gamma */
	  0x44, 0xB504, 0x45, 0xD7D0, 0x46, 0xC2B3, 0x47, 0x00BE,	/* High Green Gamma */
	  0x48, 0xBF09, 0x49, 0xD7CF, 0x4a, 0xC1B0, 0x4b, 0x00CB,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 238nit
	{ 0x40, 0xC209, 0x41, 0xD8D7, 0x42, 0xC3B5, 0x43, 0x00A0,	/* High Red Gamma */
	  0x44, 0xB604, 0x45, 0xD7D0, 0x46, 0xBFB1, 0x47, 0x00C5,	/* High Green Gamma */
	  0x48, 0xBF09, 0x49, 0xD7D0, 0x4a, 0xBBAE, 0x4b, 0x00D9,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
	// Luminance : 250nit
	{ 0x40, 0xC909, 0x41, 0xD7D8, 0x42, 0xC5B6, 0x43, 0x00A4,	/* High Red Gamma */
	  0x44, 0xB304, 0x45, 0xD7D1, 0x46, 0xBFB2, 0x47, 0x00C4,	/* High Green Gamma */
	  0x48, 0xBF09, 0x49, 0xD6D0, 0x4a, 0xBDAE, 0x4b, 0x00D7,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
};

const unsigned short GAMMA19_SETTINGS[][30] = {
	// Luminance : 250nit
	{ 0x40, 0xCA09, 0x41, 0xDBDA, 0x42, 0xCBBD, 0x43, 0x00A4,	/* High Red Gamma */
	  0x44, 0xC104, 0x45, 0xDAD4, 0x46, 0xC8B9, 0x47, 0x00C4,	/* High Green Gamma */
	  0x48, 0xC409, 0x49, 0xD9D3, 0x4a, 0xC3B6, 0x4b, 0x00D7,	/* High Blue Gamma */
	  0x3f, 0x0004,	0x32, 0x0002, ENDDEF, 0x0000				/* gamma setting : ams701ka */
	  },
};
	
#define MAX_BL_LEVEL	16

static int locked = 0;

typedef enum {
	LDI_ENABLE = 1,
	LDI_DISABLE = 0,
} ldi_enable_t;

static ldi_enable_t ldi_enable = LDI_DISABLE;

struct s5p_lcd{
	struct spi_device *g_spi;
	struct lcd_device *lcd_dev;
	struct backlight_device *bl_dev;
};

static struct s5p_lcd lcd;

static int ams701ka_spi_write_driver(unsigned char addr, unsigned short data)
{
        u32 buf[1];
        struct spi_message msg;

        struct spi_transfer xfer = {
                .len    = 4,
                .tx_buf = buf,
        };

        buf[0] = (addr << 16) | data;

        spi_message_init(&msg);
        spi_message_add_tail(&xfer, &msg);
	
	if(!(lcd.g_spi))
		return -EINVAL;

        return spi_sync(lcd.g_spi, &msg);
}

static void ams701ka_spi_write(unsigned char address, unsigned short command)
{
        if (address != DATA_ONLY)
                ams701ka_spi_write_driver(0x70, address);

        ams701ka_spi_write_driver(0x72, command);
}

static void ams701ka_panel_send_sequence(const unsigned short *wbuf)
{
        int i = 0;

        while ((wbuf[i] & DEFMASK) != ENDDEF) {
                if ((wbuf[i] & DEFMASK) != SLEEPMSEC)
                        ams701ka_spi_write(wbuf[i], wbuf[i+1]);
                else
                        mdelay(wbuf[i+1]);
                i += 2;
        }
}

static void ams701ka_ldi_init(void)
{
	printk("AMS701KA_LDI_INIT!!!!!!!!!!!!!!!\n");

        ams701ka_panel_send_sequence(GAMMA_SETTING);
        ams701ka_panel_send_sequence(SEQ_SLEEP_OUT);
        //ams701ka_panel_send_sequence(MANUAL_POWER_ON_SETTING);
        ams701ka_panel_send_sequence(SEQ_DISPLAY_ON);
        //ams701ka_panel_send_sequence(ACL_ON_DISPLAY_SETTING);

	ldi_enable = LDI_ENABLE;
}

static int s5p_lcd_set_power(struct lcd_device *ld)
{
	printk("\n+++lcd set power\n");

	return 0;
}

static struct lcd_ops s5p_lcd_ops = {
	.set_power = s5p_lcd_set_power,
};

static int gamma_level = 16;

static int s5p_bl_update_status(struct backlight_device* bd)
{

	int b = bd->props.brightness;
	int bl = 255 - b;	
	int level = 0;
	int i = 0;

	if ((!locked) && (ldi_enable == LDI_ENABLE))
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
		else	{
			printk("bl(%d) is wrong \n", bl);
			level = -1;
		}

		level = (b+1) / MAX_BL_LEVEL;
		
		if (level < 1)
			level = 1;
		if (level > MAX_BL_LEVEL)
			level = MAX_BL_LEVEL;

		printk("Brightness is updated to level %d(%d)\n", level, b);

		gamma_level = level;
		
		ams701ka_panel_send_sequence(GAMMA22_SETTINGS[level-1]);

		/*
		switch(level)
		{
			case  5:
			case  4:
			case  3:
			case  2:
			case  1: //dimming
				//s6e63m0_panel_send_sequence(s6e63m0_22gamma_50cd);
				break;
			case  6:
				//s6e63m0_panel_send_sequence(s6e63m0_22gamma_100cd);	
				break;
			case  7:
				//s6e63m0_panel_send_sequence(s6e63m0_22gamma_140cd);
				break;
			case  8:
				//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);	
				break;
			case  9:
				//s6e63m0_panel_send_sequence(s6e63m0_22gamma_260cd);	
				break;
			case  10:
				//s6e63m0_panel_send_sequence(s6e63m0_22gamma_300cd);
				break;
			default:
				break;
		}
		*/

		locked	= 0;
	}	else	{
		printk("Brightness updating is skip. locked: %d ~ ldi_enable: %d\n", locked, ldi_enable);
	}
	
	return 0;
}

void lightsensor_backlight_level_ctrl(int value)
{
	printk("Lightsensor_backlight_level_ctrl is called  : level(%d)\n",value);

	if(value > 21 || value < 1)
		return;

	switch(value){
		case  21: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  20: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  19: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  18: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  17: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  16: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  15: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  14: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  13: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  12: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  11: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  10: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  9: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  8: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  7: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  6: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  5: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  4: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  3: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  2: 
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		case  1:
			//s6e63m0_panel_send_sequence(s6e63m0_22gamma_180cd);
			break;
		default:
			break;
	}
}
EXPORT_SYMBOL(lightsensor_backlight_level_ctrl);


static int s5p_bl_get_brightness(struct backlilght_device* bd)
{
	/* TODO: Should implemented. */
	printk("Reading brightness\n");

	return 0;
}

static struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,	
};

static int __init ams701ka_probe(struct spi_device *spi)
{
	int ret = 0;

	spi->bits_per_word = 24;
	ret = spi_setup(spi);

	if (ret < 0)
		return ret;

	if (!spi)
		return -EINVAL;

	lcd.g_spi = spi;

	lcd.lcd_dev = lcd_device_register("s5p_lcd",&spi->dev,&lcd,&s5p_lcd_ops);

	lcd.bl_dev = backlight_device_register("s5p_bl",&spi->dev,&lcd,&s5p_bl_ops);
	
	lcd.bl_dev->props.max_brightness = 255;

	dev_set_drvdata(&spi->dev,&lcd);

	ams701ka_ldi_init();

	printk("AMS701KA_PROBE success\n");

	return ret;
}

#if 0 //def CONFIG_HAS_EARLYSUSPEND
static void ams701ka_suspend(struct early_suspend *h)
{
	printk("AMS701KA SUSPEND!!!!!!!!\n");

	ams701ka_panel_send_sequence(SEQ_STANDBY_ON);

	ldi_enable = LDI_DISABLE;
}

static void ams701ka_resume(struct early_suspend *h)
{
	mdelay(200);
	printk("AMS701KA RESUME START!!!!!!!!!!!\n");
#if 0
	ams701ka_panel_send_sequence(SEQ_STANDBY_OFF);
	mdelay(100);
#endif
        //ams701ka_panel_send_sequence(GAMMA_SETTING);
		ams701ka_panel_send_sequence(GAMMA22_SETTINGS[0]);

        ams701ka_panel_send_sequence(SEQ_DISPLAY_ON);
        ams701ka_panel_send_sequence(SEQ_SLEEP_OUT);
        ams701ka_panel_send_sequence(SEQ_DISPLAY_ON);

	printk("AMS701KA RESUME END!!!!!!!!!!!\n");

	ldi_enable = LDI_ENABLE;
}
#else
#ifdef CONFIG_PM
static int ams701ka_suspend(struct spi_device *spi, pm_message_t mesg)
{
	printk("AMS701KA SUSPEND!!!!!!!!\n");
	ams701ka_panel_send_sequence(SEQ_STANDBY_ON);

	ldi_enable = LDI_DISABLE;

	return 0;
}

static int ams701ka_resume(struct spi_device *spi)
{
	ams701ka_panel_send_sequence(SEQ_STANDBY_OFF);
	
	mdelay(100);

        ams701ka_panel_send_sequence(GAMMA_SETTING);
        ams701ka_panel_send_sequence(SEQ_SLEEP_OUT);
        ams701ka_panel_send_sequence(SEQ_DISPLAY_ON);

	ldi_enable = LDI_ENABLE;

	return 0;
}
#endif

#endif

static struct spi_driver ams701ka_driver = {
	.driver = {
		.name	= "ams701ka",
		.owner	= THIS_MODULE,
	},
	.probe		= ams701ka_probe,
	.remove		= __exit_p(ams701ka_remove),
#ifdef CONFIG_HAS_WAKELOCK
#if 0 //def CONFIG_HAS_EARLYSUSPEND
        .early_suspend = {
		.suspend = ams701ka_suspend,
		.resume = ams701ka_resume,
		.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING,
	},
#endif
#else
#ifdef CONFIG_PM
	.suspend	= ams701ka_suspend,
	.resume		= ams701ka_resume,
#endif
#endif
};

static int __init ams701ka_init(void)
{
	int ret;

	ret = spi_register_driver(&ams701ka_driver);

#if 0 //def CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&ams701ka_driver.early_suspend);
#endif 
	return ret;
}

static void __exit ams701ka_exit(void)
{
	spi_unregister_driver(&ams701ka_driver);
}


module_init(ams701ka_init);
module_exit(ams701ka_exit);

