/* linux/arch/arm/mach-s5pv210/mach-herring.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/gpio.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/max8998.h>
#include <linux/i2c/qt602240_ts.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/pwm_backlight.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/gpio.h>
#include <mach/gpio-herring.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#include <plat/media.h>
#endif

#include <plat/regs-serial.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/fb.h>
#include <plat/iic.h>

#include <plat/fimc.h>
#include <plat/clock.h>

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);
        
void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

static void jupiter_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	
	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	
	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

};

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define S5PV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define S5PV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define S5PV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

extern void s5pv210_reserve_bootmem(void);

static struct s3c2410_uartcfg herring_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
};

#if defined(CONFIG_TOUCHSCREEN_QT602240)
static struct platform_device s3c_device_qtts = {
        .name = "qt602240-ts",
        .id = -1,
};
#endif

/* PMIC */
static struct regulator_consumer_supply dcdc1_consumers[] = {
        {
                .supply         = "vddarm",
        },
};

static struct regulator_init_data max8998_dcdc1_data = {
        .constraints    = {
                .name           = "VCC_ARM",
                .min_uV         =  750000,
                .max_uV         = 1500000,
                .always_on      = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
        .num_consumer_supplies  = ARRAY_SIZE(dcdc1_consumers),
        .consumer_supplies      = dcdc1_consumers,
};

static struct regulator_consumer_supply dcdc2_consumers[] = {
        {
                .supply         = "vddint",
        },
};

static struct regulator_init_data max8998_dcdc2_data = {
        .constraints    = {
                .name           = "VCC_INTERNAL",
                .min_uV         =  750000,
                .max_uV         = 1500000,
                .always_on      = 1,
//              .apply_uV       = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
        .num_consumer_supplies  = ARRAY_SIZE(dcdc2_consumers),
        .consumer_supplies      = dcdc2_consumers,
};
static struct regulator_init_data max8998_ldo4_data = {
        .constraints    = {
                .name           = "VCC_DAC",
                .min_uV         = 3300000,
                .max_uV         = 3300000,
                .always_on      = 1,
                .apply_uV       = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
};




static struct regulator_init_data max8998_ldo7_data = {
        .constraints    = {
                .name           = "VCC_LCD",
                .min_uV         = 1600000,
                .max_uV         = 3600000,
                .always_on      = 1,
                //.apply_uV     = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
};

static struct regulator_init_data max8998_ldo17_data = {
        .constraints    = {
                .name           = "PM_LVDS_VDD",
                .min_uV         = 1600000,
                .max_uV         = 3600000,
                .always_on      = 1,
                //.apply_uV     = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
        },
};
static struct max8998_subdev_data universal_regulators[] = {
        { MAX8998_DCDC1, &max8998_dcdc1_data },
        { MAX8998_DCDC2, &max8998_dcdc2_data },
//      { MAX8998_DCDC4, &max8998_dcdc4_data },
        { MAX8998_LDO4, &max8998_ldo4_data },
//      { MAX8998_LDO11, &max8998_ldo11_data },
//      { MAX8998_LDO12, &max8998_ldo12_data },
//      { MAX8998_LDO13, &max8998_ldo13_data },
//      { MAX8998_LDO14, &max8998_ldo14_data },
//      { MAX8998_LDO15, &max8998_ldo15_data },
        { MAX8998_LDO7, &max8998_ldo7_data },
        { MAX8998_LDO17, &max8998_ldo17_data },
};

static struct max8998_platform_data max8998_platform_data = {
        .num_regulators = ARRAY_SIZE(universal_regulators),
        .regulators     = universal_regulators,
};

#if 0
/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
        {
                /* The address is 0xCC used since SRAD = 0 */
                I2C_BOARD_INFO("max8998", (0xCC >> 1)),
                .platform_data = &max8998_platform_data,
        },
};
#endif
struct platform_device sec_device_dpram = {
        .name   = "dpram-device",
        .id             = -1,
};
struct platform_device s3c_device_8998consumer = {
        .name             = "max8998-consumer",
        .id               = 0,
        .dev = { .platform_data = &max8998_platform_data },
};


static void tl2796_cfg_gpio(struct platform_device *pdev)
{
        int i;


        /* Temporarry code for SLSI boot loader */
        //max8998_ldo_set_voltage_direct(MAX8998_LDO7,1800000, 1800000);
        //max8998_ldo_set_voltage_direct(MAX8998_LDO17,3000000,3000000);

        for (i = 0; i < 8; i++) {
                s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
        }

        for (i = 0; i < 8; i++) {
                s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
        }

        for (i = 0; i < 8; i++) {
                s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5PV210_GPF2(i), S3C_GPIO_PULL_NONE);
        }

        for (i = 0; i < 4; i++) {
                s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5PV210_GPF3(i), S3C_GPIO_PULL_NONE);
        }

        /* mDNIe SEL: why we shall write 0x2 ? */
#ifdef CONFIG_FB_S3C_MDNIE
        writel(0x1, S5P_MDNIE_SEL);
#else
        writel(0x2, S5P_MDNIE_SEL);
#endif
#if 0
	 /* drive strength to max */
        writel(0xffffffff, S5PC_VA_GPIO + 0x12c);
        writel(0xffffffff, S5PC_VA_GPIO + 0x14c);
        writel(0xffffffff, S5PC_VA_GPIO + 0x16c);
        writel(0x000000ff, S5PC_VA_GPIO + 0x18c);
#endif

        /* DISPLAY_CS */
        s3c_gpio_cfgpin(S5PV210_MP01(1), S3C_GPIO_SFN(1));
        /* DISPLAY_CLK */
        s3c_gpio_cfgpin(S5PV210_MP04(1), S3C_GPIO_SFN(1));
        /* DISPLAY_SO */
        s3c_gpio_cfgpin(S5PV210_MP04(2), S3C_GPIO_SFN(1));
        /* DISPLAY_SI */
        s3c_gpio_cfgpin(S5PV210_MP04(3), S3C_GPIO_SFN(1));

        /* DISPLAY_CS */
        s3c_gpio_setpull(S5PV210_MP01(1), S3C_GPIO_PULL_NONE);
        /* DISPLAY_CLK */
        s3c_gpio_setpull(S5PV210_MP04(1), S3C_GPIO_PULL_NONE);
        /* DISPLAY_SO */
        s3c_gpio_setpull(S5PV210_MP04(2), S3C_GPIO_PULL_NONE);
        /* DISPLAY_SI */
        s3c_gpio_setpull(S5PV210_MP04(3), S3C_GPIO_PULL_NONE);

        /*KGVS : configuring GPJ2(4) as FM interrupt */
        //s3c_gpio_cfgpin(S5PV210_GPJ2(4), S5PV210_GPJ2_4_GPIO_INT20_4);

}


void lcd_cfg_gpio_early_suspend(void)
{

        int i;
	printk("[%s]\n", __func__);

        for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_OUTPUT);
                s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF0(i), 0);
        }

        for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_OUTPUT);
                s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF1(i), 0);
        }

        for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_OUTPUT);
                s3c_gpio_setpull(S5PV210_GPF2(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF2(i), 0);
        }

        for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_OUTPUT);
                s3c_gpio_setpull(S5PV210_GPF3(i), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPF3(i), 0);
        }
	// drive strength to min 
	writel(0x00000000, S5P_VA_GPIO + 0x12c); 	// GPF0DRV
	writel(0x00000000, S5P_VA_GPIO + 0x14c);	// GPF1DRV
	writel(0x00000000, S5P_VA_GPIO + 0x16c);	// GPF2DRV
	writel(0x00000000, S5P_VA_GPIO + 0x18c);	// GPF3DRV

	// OLED_DET
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);	
	gpio_set_value(GPIO_OLED_DET, 0);	

	// LCD_RST
	s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MLCD_RST, S3C_GPIO_PULL_NONE);	
	gpio_set_value(GPIO_MLCD_RST, 0);

        /* DISPLAY_CS */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CS, S3C_GPIO_OUTPUT);
        s3c_gpio_setpull(GPIO_DISPLAY_CS, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_DISPLAY_CS, 0);

        /* DISPLAY_CLK */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CLK, S3C_GPIO_OUTPUT);
        s3c_gpio_setpull(GPIO_DISPLAY_CLK, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_DISPLAY_CLK, 0);
        /* DISPLAY_SO */
        //s3c_gpio_cfgpin(S5PV210_MP04(2), S3C_GPIO_INPUT);
        //s3c_gpio_setpull(S5PV210_MP04(2), S3C_GPIO_PULL_DOWN);
        /* DISPLAY_SI */
	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_OUTPUT);
        s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_DISPLAY_SI, 0);

	// OLED_ID
	s3c_gpio_cfgpin(GPIO_OLED_ID, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OLED_ID, S3C_GPIO_PULL_DOWN);	
//	gpio_set_value(GPIO_OLED_ID, 0);	

	// DIC_ID
	s3c_gpio_cfgpin(GPIO_DIC_ID, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_DIC_ID, S3C_GPIO_PULL_DOWN);	
//	gpio_set_value(GPIO_DIC_ID, 0);

}
EXPORT_SYMBOL(lcd_cfg_gpio_early_suspend);

void lcd_cfg_gpio_late_resume(void)
{
	printk("[%s]\n", __func__);

	// OLED_DET
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);		
	// OLED_ID
	s3c_gpio_cfgpin(GPIO_OLED_ID, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_ID, S3C_GPIO_PULL_NONE);	
//	gpio_set_value(GPIO_OLED_ID, 0);	
	// DIC_ID
	s3c_gpio_cfgpin(GPIO_DIC_ID, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DIC_ID, S3C_GPIO_PULL_NONE);	
//	gpio_set_value(GPIO_DIC_ID, 0);
}
EXPORT_SYMBOL(lcd_cfg_gpio_late_resume);




static int tl2796_reset_lcd(struct platform_device *pdev)
{
        int err;

//  Ver1 & Ver2 universal board kyoungheon
        err = gpio_request(S5PV210_MP05(5), "MLCD_RST");
        if (err) {
                printk(KERN_ERR "failed to request MP0(5) for "
                        "lcd reset control\n");
                return err;
        }

        gpio_direction_output(S5PV210_MP05(5), 1);
        msleep(10);

        gpio_set_value(S5PV210_MP05(5), 0);
        msleep(10);

        gpio_set_value(S5PV210_MP05(5), 1);
        msleep(10);

        gpio_free(S5PV210_MP05(5));

        return 0;
}

static int tl2796_backlight_on(struct platform_device *pdev)
{

}

static struct s3c_platform_fb tl2796_data __initdata = {
        .hw_ver = 0x62,
        .clk_name = "sclk_fimd",
        .nr_wins = 5,
        .default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
        .swap = FB_SWAP_HWORD | FB_SWAP_WORD,

        .cfg_gpio = tl2796_cfg_gpio,
        .backlight_on = tl2796_backlight_on,
        .reset_lcd = tl2796_reset_lcd,
};

#define LCD_BUS_NUM     3
#define DISPLAY_CS      S5PV210_MP01(1)
#define SUB_DISPLAY_CS  S5PV210_MP01(2)
#define DISPLAY_CLK     S5PV210_MP04(1)
#define DISPLAY_SI      S5PV210_MP04(3)


static struct spi_board_info spi_board_info[] __initdata = {
        {
                .modalias       = "tl2796",
                .platform_data  = NULL,
                .max_speed_hz   = 1200000,
                .bus_num        = LCD_BUS_NUM,
                .chip_select    = 0,
                .mode           = SPI_MODE_3,
                .controller_data = (void *)DISPLAY_CS,
        },
};

static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
        .sck    = DISPLAY_CLK,
        .mosi   = DISPLAY_SI,
        .miso   = 0,
        .num_chipselect = 2,
};

static struct platform_device s3c_device_spi_gpio = {
        .name   = "spi_gpio",
        .id     = LCD_BUS_NUM,
        .dev    = {
                .parent         = &s3c_device_fb.dev,
                .platform_data  = &tl2796_spi_gpio_data,
        },
};



static  struct  i2c_gpio_platform_data  i2c4_platdata = {
        .sda_pin                = GPIO_AP_SDA_18V,
        .scl_pin                = GPIO_AP_SCL_18V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
//      .scl_is_output_only     = 1,
      };

static struct platform_device s3c_device_i2c4 = {
        .name                           = "i2c-gpio",
        .id                                     = 4,
        .dev.platform_data      = &i2c4_platdata,
};

static  struct  i2c_gpio_platform_data  i2c5_platdata = {
        .sda_pin                = GPIO_AP_SDA_28V,
        .scl_pin                = GPIO_AP_SCL_28V,
        .udelay                 = 2,    /* 250KHz */
//      .udelay                 = 4,
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
//      .scl_is_output_only     = 1,
};

static struct platform_device s3c_device_i2c5 = {
        .name                           = "i2c-gpio",
        .id                                     = 5,
        .dev.platform_data      = &i2c5_platdata,
};


static  struct  i2c_gpio_platform_data  i2c6_platdata = {
        .sda_pin                = GPIO_AP_PMIC_SDA,
        .scl_pin                = GPIO_AP_PMIC_SCL,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c6 = {
        .name                           = "i2c-gpio",
        .id                                     = 6,
        .dev.platform_data      = &i2c6_platdata,
};

static  struct  i2c_gpio_platform_data  i2c7_platdata = {
        .sda_pin                = GPIO_USB_SDA_28V,
        .scl_pin                = GPIO_USB_SCL_28V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c7 = {
        .name                           = "i2c-gpio",
        .id                                     = 7,
        .dev.platform_data      = &i2c7_platdata,
};
// For FM radio
static  struct  i2c_gpio_platform_data  i2c8_platdata = {
        .sda_pin                = GPIO_FM_SDA_28V,
        .scl_pin                = GPIO_FM_SCL_28V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c8 = {
        .name                           = "i2c-gpio",
        .id                                     = 8,
        .dev.platform_data      = &i2c8_platdata,
};

static  struct  i2c_gpio_platform_data  i2c9_platdata = {
        .sda_pin                = FUEL_SDA_18V,
        .scl_pin                = FUEL_SCL_18V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c9 = {
        .name                           = "i2c-gpio",
        .id                                     = 9,
        .dev.platform_data      = &i2c9_platdata,
};

static  struct  i2c_gpio_platform_data  i2c10_platdata = {
        .sda_pin                = _3_TOUCH_SDA_28V,
        .scl_pin                = _3_TOUCH_SCL_28V,
        .udelay                 = 0,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};
static struct platform_device s3c_device_i2c10 = {
        .name                           = "i2c-gpio",
        .id                                     = 10,
        .dev.platform_data      = &i2c10_platdata,
};

static  struct  i2c_gpio_platform_data  i2c11_platdata = {
        .sda_pin                = GPIO_ALS_SDA_28V,
        .scl_pin                = GPIO_ALS_SCL_28V,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c11 = {
        .name                           = "i2c-gpio",
        .id                                     = 11,
        .dev.platform_data      = &i2c11_platdata,
};

static  struct  i2c_gpio_platform_data  i2c12_platdata = {
        .sda_pin                = GPIO_MSENSE_SDA_28V,
        .scl_pin                = GPIO_MSENSE_SCL_28V,
        .udelay                 = 0,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c12 = {
        .name                           = "i2c-gpio",
        .id                                     = 12,
        .dev.platform_data      = &i2c12_platdata,
};


#ifdef CONFIG_S5PV210_ADCTS
static struct s3c_adcts_plat_info s3c_adcts_cfgs __initdata = {
	.channel = {
		{ /* 0 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 1 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 2 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 3 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 4 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 5 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 6 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 7 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_S3C
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.adcts = {
		.delay = 0xFF,
	.presc                  = 49,
		.resol = S3C_ADCCON_RESSEL_12BIT,
	},
	.sampling_time = 18,
	.sampling_interval_ms = 20,
	.x_coor_min	= 180,
	.x_coor_max = 4000,
	.x_coor_fuzz = 32,
	.y_coor_min = 300,
	.y_coor_max = 3900,
	.y_coor_fuzz = 32,
	.use_tscal = false,
	.tscal = {0, 0, 0, 0, 0, 0, 0},
};
#endif

#if 0
#ifndef CONFIG_S5PV210_ADCTS 
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* s5pc110 support 12-bit resolution */
	.delay  = 10000,
	.presc  = 49,
	.resolution = 12,
};
#endif
#endif
#ifdef CONFIG_S5P_ADC 
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* s5pc110 support 12-bit resolution */
	.delay  = 10000,
	.presc  = 65,
	.resolution = 12,
};
#endif

#ifdef CONFIG_VIDEO_FIMC
/*
 * Guide for Camera Configuration for Aries
*/

#ifdef CONFIG_VIDEO_CE147
/*
 * Guide for Camera Configuration for Jupiter board
 * ITU CAM CH A: CE147
*/
static void ce147_ldo_en(bool onoff)
{
	int err;

	//For Emul Rev0.1
	// Because B4, B5 do not use this GPIO, this GPIO is enabled in all HW version
	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");
	if(err) {
		printk(KERN_ERR "failed to request GPB7 for camera control\n");
		return;
	}

	if(onoff == TRUE) { //power on 
		// Turn CAM_ISP_1.2V on
		Set_MAX8998_PM_OUTPUT_Voltage(BUCK4, VCC_1p300);

		Set_MAX8998_PM_REG(EN4, 1);

		mdelay(1);

		// Turn CAM_AF_2.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO11, VCC_2p800);

		Set_MAX8998_PM_REG(ELDO11, 1);

		// Turn CAM_SENSOR_1.2V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO12, VCC_1p200);

		Set_MAX8998_PM_REG(ELDO12, 1);

		// Turn CAM_SENSOR_A2.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);

		Set_MAX8998_PM_REG(ELDO13, 1);

		// Turn CAM_ISP_1.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);

		Set_MAX8998_PM_REG(ELDO14, 1);

		// Turn CAM_ISP_2.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);

		Set_MAX8998_PM_REG(ELDO15, 1);

		// Turn CAM_SENSOR_1.8V on
		Set_MAX8998_PM_OUTPUT_Voltage(LDO16, VCC_1p800);

		Set_MAX8998_PM_REG(ELDO16, 1);

		// Turn CAM_ISP_SYS_2.8V on
		gpio_direction_output(GPIO_GPB7, 0);

		gpio_set_value(GPIO_GPB7, 1);
	}
	
	else { // power off
		// Turn CAM_ISP_SYS_2.8V off
		gpio_direction_output(GPIO_GPB7, 1);
				
		gpio_set_value(GPIO_GPB7, 0);

		// Turn CAM_AF_2.8V off
		Set_MAX8998_PM_REG(ELDO11, 0);
		
		// Turn CAM_SENSOR_1.2V off
		Set_MAX8998_PM_REG(ELDO12, 0);
		
		// Turn CAM_SENSOR_A2.8V off
		Set_MAX8998_PM_REG(ELDO13, 0);
		
		// Turn CAM_ISP_1.8V off
		Set_MAX8998_PM_REG(ELDO14, 0);
		
		// Turn CAM_ISP_2.8V off
		Set_MAX8998_PM_REG(ELDO15, 0);
		
		// Turn CAM_SENSOR_1.8V off
		Set_MAX8998_PM_REG(ELDO16, 0);
		
		mdelay(1);
		
		// Turn CAM_ISP_1.2V off
		Set_MAX8998_PM_REG(EN4, 0);
	}

	gpio_free(GPIO_GPB7);
}

static int ce147_power_on(void)
{	
	int err;

	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPJ0");
	if(err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1");
	if(err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");
		return err;
	}
		
	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");
	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");
		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");
	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");
		return err;
	}
	
	ce147_ldo_en(TRUE);

	mdelay(1);

	// CAM_VGA_nSTBY  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PV210_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	// CAM_VGA_nRST  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);

	gpio_set_value(GPIO_CAM_VGA_nRST, 1);	

	mdelay(1);

	// CAM_VGA_nSTBY  LOW	
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	// CAM_MEGA_EN HIGH
	gpio_direction_output(GPIO_CAM_MEGA_EN, 0);

	gpio_set_value(GPIO_CAM_MEGA_EN, 1);

	mdelay(1);

	// CAM_MEGA_nRST HIGH
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);

	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);

	gpio_free(GPIO_CAM_MEGA_EN);

	gpio_free(GPIO_CAM_MEGA_nRST);

	gpio_free(GPIO_CAM_VGA_nSTBY);

	gpio_free(GPIO_CAM_VGA_nRST);	

	return 0;
}

static int ce147_power_off(void)
{
	int err;
	
	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPJ0");
	if(err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1");
	if(err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");
		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");
	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");
		return err;
	}

	// CAM_VGA_nRST  LOW		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);
	mdelay(1);

	// CAM_MEGA_nRST - GPJ1(5) LOW
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 1);
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);
	mdelay(1);

	// CAM_MEGA_EN - GPJ0(6) LOW
	gpio_direction_output(GPIO_CAM_MEGA_EN, 1);
	gpio_set_value(GPIO_CAM_MEGA_EN, 0);
	mdelay(1);

	ce147_ldo_en(FALSE);
	mdelay(1);
	
	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int ce147_power_en(int onoff)
{
	if(onoff == 1) {
		ce147_power_on();
	} else {
		ce147_power_off();
		s3c_i2c0_force_stop();
	}

	return 0;
}

/* External camera module setting */
static struct ce147_platform_data ce147_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  ce147_i2c_info = {
	I2C_BOARD_INFO("CE147", 0x78>>1),
	.platform_data = &ce147_plat,
};

static struct s3c_platform_camera ce147 = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &ce147_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= ce147_power_en,
};
#endif

/* External camera module setting */
#ifdef CONFIG_VIDEO_S5KA3DFX

static int s5ka3dfx_power_on(void)
{
	int err;

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");
	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");
		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");
	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");
		return err;
	}

	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");
	if(err) {
		printk(KERN_ERR "failed to request GPB7 for camera control\n");
		return err;
	}

	// Turn CAM_ISP_SYS_2.8V on
	gpio_direction_output(GPIO_GPB7, 0);
	gpio_set_value(GPIO_GPB7, 1);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_ISP_HOST_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO15, 1);

	mdelay(1);

	// Turn CAM_ISP_RAM_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);

	mdelay(1);
	
	gpio_free(GPIO_GPB7);	

	mdelay(1);

	// CAM_VGA_nSTBY  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PV210_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	// CAM_VGA_nRST  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	gpio_set_value(GPIO_CAM_VGA_nRST, 1);		

	mdelay(4);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);	

	return 0;
}

static int s5ka3dfx_power_off(void)
{
	int err;

	printk(KERN_ERR "s5ka3dfx_power_off\n");

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");
	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");
		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");
	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");
		return err;
	}


	// CAM_VGA_nRST  LOW		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	mdelay(1);

	// CAM_VGA_nSTBY  LOW		
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");
	if(err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");
		return err;
	}

	// Turn CAM_ISP_HOST_2.8V off
	Set_MAX8998_PM_REG(ELDO15, 0);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_ISP_RAM_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);

	// Turn CAM_ISP_SYS_2.8V off
	gpio_direction_output(GPIO_GPB7, 1);
	gpio_set_value(GPIO_GPB7, 0);
	
	gpio_free(GPIO_GPB7);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);	

	return 0;
}

static int s5ka3dfx_power_en(int onoff)
{
	if(onoff){
		s5ka3dfx_power_on();
	} else {
		s5ka3dfx_power_off();
		s3c_i2c0_force_stop();
	}

	return 0;
}

static struct s5ka3dfx_platform_data s5ka3dfx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5ka3dfx_i2c_info = {
	I2C_BOARD_INFO("S5KA3DFX", 0xc4>>1),
	.platform_data = &s5ka3dfx_plat,
};

static struct s3c_platform_camera s5ka3dfx = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5ka3dfx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 480,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= s5ka3dfx_power_en,
};
#endif


/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_fimc",
	.lclk_name	= "sclk_fimc_lclk",
	.clk_rate	= 166750000,
	.default_cam	= CAMERA_PAR_A,
	.camera		= {
#ifdef CONFIG_VIDEO_CE147
		&ce147,
#endif
#ifdef CONFIG_VIDEO_S5KA3DFX
		&s5ka3dfx,
#endif
	},
	.hw_ver		= 0x43,
};
#endif

#if defined(CONFIG_BACKLIGHT_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id  = 3,
	.max_brightness = 255,
	.dft_brightness = 255,
	.pwm_period_ns  = 78770,
};

static struct platform_device smdk_backlight_device = {
	.name      = "pwm-backlight",
	.id        = -1,
	.dev        = {
		.parent = &s3c_device_timer[3].dev,
		.platform_data = &smdk_backlight_data,
	},
};
static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);
	if (ret)
		printk(KERN_ERR "smdk: failed to register backlight device: %d\n", ret);
}
#endif

#if defined(CONFIG_BLK_DEV_IDE_S3C)
static struct s3c_ide_platdata smdkv210_ide_pdata __initdata = {
	.setup_gpio     = s3c_ide_setup_gpio,
};
#endif

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs4[] __initdata = {
#ifdef CONFIG_SND_SOC_WM8580
	{
		I2C_BOARD_INFO("wm8580", 0x1b),
	},
#endif
#ifdef CONFIG_SND_SOC_WM8994 
	{
		I2C_BOARD_INFO("wm8994", (0x34>>1)),
	},
#endif
};


/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
};

/* i2c board & device info. */
static struct qt602240_platform_data qt602240_p1_platform_data = {
        .x_line = 19,
        .y_line = 11,
        .x_size = 1024,
        .y_size = 1024,
        .blen = 0x41,
        .threshold = 0x30,
        .orient = QT602240_VERTICAL_FLIP,
};

/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
    {
        I2C_BOARD_INFO("qt602240_ts", 0x4a),
        .platform_data  = &qt602240_p1_platform_data,
    },
};



/* I2C2 */
static struct i2c_board_info i2c_devs10[] __initdata = {
    {
        I2C_BOARD_INFO("melfas_touchkey", 0x20),
       // .platform_data  = &qt602240_p1_platform_data,
    },
};

static struct i2c_board_info i2c_devs7[] __initdata = {
    {
        I2C_BOARD_INFO("fsa9480", 0x4A >> 1),
    },
};


static struct i2c_board_info i2c_devs6[] __initdata = {
#ifdef CONFIG_REGULATOR_MAX8998
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8998", (0xCC >> 1)),
		.platform_data = &max8998_platform_data,
	},
	{
		I2C_BOARD_INFO("rtc_max8998", (0x0D >> 1)),
	},
#endif
};


#ifdef CONFIG_DM9000
static void __init smdkv210_dm9000_set(void)
{
	unsigned int tmp;

	tmp = ((0<<28)|(0<<24)|(5<<16)|(0<<12)|(0<<8)|(0<<4)|(0<<0));
	__raw_writel(tmp, (S5P_SROM_BW+0x18));

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~(0xf << 20);

#ifdef CONFIG_DM9000_16BIT
	tmp |= (0x1 << 20);
#else
	tmp |= (0x2 << 20);
#endif
	__raw_writel(tmp, S5P_SROM_BW);

	tmp = __raw_readl(S5PV210_MP01CON);
	tmp &= ~(0xf << 20);
	tmp |= (2 << 20);

	__raw_writel(tmp, S5PV210_MP01CON);
}
#endif

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,
	.cached = 1,
	.start = 0, // will be set during proving pmem driver.
	.size = 0 // will be set during proving pmem driver.
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
   .name = "pmem_gpu1",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct android_pmem_platform_data pmem_adsp_pdata = {
   .name = "pmem_adsp",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct platform_device pmem_device = {
   .name = "android_pmem",
   .id = 0,
   .dev = { .platform_data = &pmem_pdata },
};

static struct platform_device pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};

static void __init android_pmem_set_platdata(void)
{
	pmem_pdata.start = (u32)s3c_get_media_memory_bank(S3C_MDEV_PMEM, 0);
	pmem_pdata.size = (u32)s3c_get_media_memsize_bank(S3C_MDEV_PMEM, 0);

	pmem_gpu1_pdata.start = (u32)s3c_get_media_memory_bank(S3C_MDEV_PMEM_GPU1, 0);
	pmem_gpu1_pdata.size = (u32)s3c_get_media_memsize_bank(S3C_MDEV_PMEM_GPU1, 0);

	pmem_adsp_pdata.start = (u32)s3c_get_media_memory_bank(S3C_MDEV_PMEM_ADSP, 0);
	pmem_adsp_pdata.size = (u32)s3c_get_media_memsize_bank(S3C_MDEV_PMEM_ADSP, 0);
}
#endif
struct platform_device sec_device_battery = {
	.name	= "sec-fake-battery",
	.id		= -1,
};

/*Adding gpio settings during bootup and sleep*/


#define S3C_GPIO_SETPIN_ZERO         0
#define S3C_GPIO_SETPIN_ONE          1
#define S3C_GPIO_SETPIN_NONE	     2  // dont set the data pin.

/*
 *
 * GPIO Initialization table. It has the following format
 * { pin number, pin configuration, pin value, pullup/down config,
 * 		driver strength, slew rate, sleep mode pin conf, sleep mode pullup/down config }
 * 
 * The table can be modified with the appropriate value for each pin. 
 */
static unsigned int jupiter_gpio_table[][8] = {
	/* Off part */	
	// GPA0 ~ GPA1 : is done by UART driver early, so not modifying.
#if 0	
	{S5PV210_GPA0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN, 
			S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	// uart2 rx and tx..  done by uboot. So not modifying
	//{S5PV210_GPA1(0), 2, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
	//		 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	//{S5PV210_GPA1(1), 2, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
	//		 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPA1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
#endif
	{S5PV210_GPB(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPC0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPC1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPD0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPD1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPE0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPE1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
#if 0 // trb
	{S5PV210_GPF0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPF1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF1(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPF2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPF3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
#endif	// trb		 
        {S5PV210_GPF3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPG0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPG1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG1(2), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPG2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPG3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPG3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	/* Alive part */
        {S5PV210_GPH0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 

        {S5PV210_GPH1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH1(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 

        {S5PV210_GPH2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 

        {S5PV210_GPH3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 
        {S5PV210_GPH3(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, 0, 0}, 


	/* Alive part ending and off part start*/
	{S5PV210_GPI(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPJ0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPJ1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPJ2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPJ2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPJ3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ3(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPJ4(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ4(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ4(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ4(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ4(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	/* memory part */
	{S5PV210_MP01(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
 
	{S5PV210_MP02(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP02(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP02(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP02(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_MP03(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_MP04(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_MP05(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_MP06(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_MP07(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE, S3C_GPIO_PULL_DOWN,
			 S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	/* Memory part ending and off part ending */

};


void s3c_config_gpio_table(int array_size, unsigned int (*gpio_table)[8])
{
        u32 i, gpio;
        for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		/* Off part */
		if((gpio <= S5PV210_GPG3(6)) ||
		   ((gpio <= S5PV210_GPJ4(7)) && (gpio >= S5PV210_GPI(0)))) {

        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);

                	//s3c_gpio_slp_cfgpin(gpio, gpio_table[i][6]);
                	//s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][7]);
			
		}
#if 1
		/* Alive part */
		else if((gpio <= S5PV210_GPH3(7)) && (gpio >= S5PV210_GPH0(0))) {
        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);
		}
#endif
#if 0
		/* Memory part */
		else if((gpio >  S5PV210_GPJ4(4)) && (gpio <= S5PV210_MP07(7))) {
        	        s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
                	s3c_gpio_setpull(gpio, gpio_table[i][3]);

                	if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
                    		gpio_set_value(gpio, gpio_table[i][2]);

                	s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
                	s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);

                	//s3c_gpio_slp_cfgpin(gpio, gpio_table[i][6]);
                	//s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][7]);

		}
#endif
	}

}






#define S5PV210_PS_HOLD_CONTROL_REG (S3C_VA_SYS+0xE81C)

static void herring_power_off (void) 
{

#if 1

	printk("herring_power_off\n");

	/* temporary power off code */
	/*PS_HOLD high  PS_HOLD_CONTROL, R/W, 0xE010_E81C*/
         writel(readl(S5PV210_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF, S5PV210_PS_HOLD_CONTROL_REG);


#else
	int	mode = REBOOT_MODE_NONE;
	//char reset_mode = 'r';
	//int cnt = 0;

	if (maxim_chg_status()) {	/* Reboot Charging */
		mode = REBOOT_MODE_CHARGING;
		if (sec_set_param_value)
			sec_set_param_value(__REBOOT_MODE, &mode);
		/* Watchdog Reset */
		printk(KERN_EMERG "%s: TA is connected, rebooting...\n", __func__);
#ifdef CONFIG_KERNEL_DEBUG_SEC // trb - rebasing error 
		kernel_sec_hw_reset(TRUE);
#endif
		printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
	}
	else {	/* Power Off or Reboot */

//		if (sec_set_param_value)
//			sec_set_param_value(__REBOOT_MODE, &mode);

#if 0 //if JIG is connected, reset
		if (get_usb_cable_state() & (JIG_UART_ON | JIG_UART_OFF | JIG_USB_OFF | JIG_USB_ON)) {
			/* Watchdog Reset */
			printk(KERN_EMERG "%s: JIG is connected, rebooting...\n", __func__);
			arch_reset(reset_mode);
			printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
		}
		else {
#endif			
			/* POWER_N -> Input */
			gpio_direction_input(GPIO_N_POWER);
			/* PHONE_ACTIVE -> Input */
			gpio_direction_input(GPIO_PHONE_ACTIVE);
			/* Check Power Off Condition */
			if (!gpio_get_value(GPIO_N_POWER) || gpio_get_value(GPIO_PHONE_ACTIVE)) {
				/* Wait Power Button Release */
				printk(KERN_EMERG "%s: waiting for GPIO_POWER_N high.\n", __func__);
				
#if 0 // add later for checking of nPower and Phone_active 
				while (!gpio_get_value(GPIO_N_POWER)); 

				/* Wait Phone Power Off */
				printk(KERN_EMERG "%s: waiting for GPIO_PHONE_ACTIVE low.\n", __func__);
				while (gpio_get_value(GPIO_PHONE_ACTIVE)) {
					if (cnt++ < 5) {
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE is high(%d)\n", __func__, cnt);
						mdelay(1000);
					} else {
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE TIMED OUT!!!\n", __func__);
						break;
					}
				}	
#endif
			}
			/* PS_HOLD -> Output Low */
			printk(KERN_EMERG "%s: setting GPIO_PDA_PS_HOLD low.\n", __func__);

			/*PS_HOLD high  PS_HOLD_CONTROL, R/W, 0xE010_E81C*/
			writel(readl(S5PV210_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF, S5PV210_PS_HOLD_CONTROL_REG);
			//gpio_direction_output(GPIO_AP_PS_HOLD, 1);
			//s3c_gpio_setpull(GPIO_AP_PS_HOLD, S3C_GPIO_PULL_NONE);
			//gpio_set_value(GPIO_AP_PS_HOLD, 0);

			printk(KERN_EMERG "%s: should not reach here!\n", __func__);
//		}
	}
#endif

	while (1);

}


/* this table only for B4 board */
 
static unsigned int jupiter_sleep_gpio_table[][3] = {
#if 0 //for herring

	{S5PV210_GPA0(0),
			 S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA0(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA0(2),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA0(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA0(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA0(5),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA0(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA0(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPA1(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPA1(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA1(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPA1(3),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPB(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPB(1),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPB(2),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPB(3),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPB(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPB(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPB(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPB(7),
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPC0(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPC0(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC0(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPC0(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPC0(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPC1(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPC1(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPC1(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPC1(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPC1(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPD0(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PV210_GPD0(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPD0(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPD0(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPD1(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPD1(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPD1(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPD1(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPD1(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPD1(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPE0(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE0(7),
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_GPE1(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE1(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE1(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPE1(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPE1(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
	{S5PV210_GPF0(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF0(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF0(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF0(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF0(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF0(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF0(6), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF0(7), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPF1(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF1(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF1(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF1(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF1(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF1(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF1(6), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF1(7), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPF2(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF2(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF2(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF2(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF2(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF2(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF2(6), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF2(7), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPF3(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF3(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF3(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPF3(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPF3(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPF3(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
			  

	{S5PV210_GPG0(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG0(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG0(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG0(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG0(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG0(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG0(6), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPG1(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG1(1), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG1(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
        {S5PV210_GPG1(3), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG1(4), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG1(5), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG1(6), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPG2(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG2(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG2(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG2(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG2(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG2(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG2(6), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPG3(0), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG3(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG3(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PV210_GPG3(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG3(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG3(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPG3(6), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 	  

	/* Alive part ending and off part start*/
#if 1
	{S5PV210_GPI(0), 
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPI(1), 
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPI(2), 
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPI(3), 
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPI(4), 
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPI(5), 
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPI(6), 
			  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
#else


	{S5PV210_GPI(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPI(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 


#endif
	{S5PV210_GPJ0(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ0(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ0(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ0(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ0(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ0(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ0(6), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ0(7), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPJ1(0), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ1(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ1(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_GPJ1(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ1(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ1(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPJ2(0), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ2(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ2(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ2(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ2(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ2(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPJ2(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ2(7), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 


	{S5PV210_GPJ3(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ3(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ3(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ3(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPJ3(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ3(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ3(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ3(7), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 

	{S5PV210_GPJ4(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ4(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
	{S5PV210_GPJ4(2), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ4(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_GPJ4(4), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 


	/* memory part */

	{S5PV210_MP01(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP01(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(4), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP01(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
        {S5PV210_MP01(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP01(7), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
 
	{S5PV210_MP02(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP02(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP02(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP02(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
			  
	{S5PV210_MP03(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(2), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE},
        {S5PV210_MP03(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP03(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP03(5), 
			  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP03(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP03(7), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
			  
	{S5PV210_MP04(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(1), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP04(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(3), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP04(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP04(6), 
			  //S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP04(7), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_MP05(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP05(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP05(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP05(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP05(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(5), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
        {S5PV210_MP05(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP05(7), 
			  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, 
			  
	{S5PV210_MP06(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP06(7), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	{S5PV210_MP07(0), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(1), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(2), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(3), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(4), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(5), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(6), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 
        {S5PV210_MP07(7), 
			  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, 

	/* Memory part ending and off part ending */
#endif //for herring
};

void s3c_config_sleep_gpio_table(int array_size, unsigned int (*gpio_table)[3])
{
        u32 i, gpio;
		
        for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
               	s3c_gpio_slp_cfgpin(gpio, gpio_table[i][1]);
               	s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][2]);
	}
}








// just for ref.. 	
//
void s3c_config_sleep_gpio(void)
{

	// setting the alive mode registers

	s3c_gpio_cfgpin(S5PV210_GPH0(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(1), S3C_GPIO_PULL_DOWN);
	//gpio_set_value(S5PV210_GPH0(1), 0);

	s3c_gpio_cfgpin(S5PV210_GPH0(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(2), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH0(2), 0);
	s3c_gpio_cfgpin(S5PV210_GPH0(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(3), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH0(3), 0);

	s3c_gpio_cfgpin(S5PV210_GPH0(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(4), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH0(4), 0);

	s3c_gpio_cfgpin(S5PV210_GPH0(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(5), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH0(5), 0);

	s3c_gpio_cfgpin(S5PV210_GPH0(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(6), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH0(6), 0);
	
	//s3c_gpio_cfgpin(S5PV210_GPH0(7), S3C_GPIO_INPUT);
	//s3c_gpio_setpull(S5PV210_GPH0(7), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH0(0), 0);

	s3c_gpio_cfgpin(S5PV210_GPH1(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(0), S3C_GPIO_PULL_DOWN);
	//gpio_set_value(S5PV210_GPH1(1), 0);

	s3c_gpio_cfgpin(S5PV210_GPH1(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(1), S3C_GPIO_PULL_DOWN);
	//gpio_set_value(S5PV210_GPH1(1), 0);
	//
	s3c_gpio_cfgpin(S5PV210_GPH1(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(2), S3C_GPIO_PULL_DOWN);
	//gpio_set_value(S5PV210_GPH1(2), 0);

#if 0	// kt.hur on 100104
	s3c_gpio_cfgpin(S5PV210_GPH1(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(3), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH1(3), 0);
#endif

	s3c_gpio_cfgpin(S5PV210_GPH1(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH1(4), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH1(4), 0);

	s3c_gpio_cfgpin(S5PV210_GPH1(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(5), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH1(5),0);

	s3c_gpio_cfgpin(S5PV210_GPH1(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(6), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH1(6), 0);

	s3c_gpio_cfgpin(S5PV210_GPH1(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(7), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH1(7), 0);


	s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(0), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH2(0), 0);
	
	s3c_gpio_cfgpin(S5PV210_GPH2(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(1), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH2(1), 0);

	s3c_gpio_cfgpin(S5PV210_GPH2(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(2), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH2(2), 0);

	s3c_gpio_cfgpin(S5PV210_GPH2(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(3), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH2(3), 0);

	s3c_gpio_cfgpin(S5PV210_GPH2(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH2(4), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH2(4), 0);

#if 0    // bluetooth
	s3c_gpio_cfgpin(S5PV210_GPH2(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH2(5), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH2(5), 0);
#endif

	//s3c_gpio_cfgpin(S5PV210_GPH2(6), S3C_GPIO_INPUT);
	//s3c_gpio_setpull(S5PV210_GPH2(6), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH2(6), 0);

	s3c_gpio_cfgpin(S5PV210_GPH2(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH2(7), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH2(7), 0);
	

#if 0 // keypad
	s3c_gpio_cfgpin(S5PV210_GPH3(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(0), S3C_GPIO_PULL_UP);
//	gpio_set_value(S5PV210_GPH3(0), 0);
	
	s3c_gpio_cfgpin(S5PV210_GPH3(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(1), S3C_GPIO_PULL_UP);
//	gpio_set_value(S5PV210_GPH3(1), 0);

	s3c_gpio_cfgpin(S5PV210_GPH3(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(2), S3C_GPIO_PULL_UP);
//	gpio_set_value(S5PV210_GPH3(2), 0);

	s3c_gpio_cfgpin(S5PV210_GPH3(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(3), S3C_GPIO_PULL_UP);
//	gpio_set_value(S5PV210_GPH3(3), 0);

#endif

	s3c_gpio_cfgpin(S5PV210_GPH3(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(3), S3C_GPIO_PULL_NONE);
	
	s3c_gpio_cfgpin(S5PV210_GPH3(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH3(4), 0);

	s3c_gpio_cfgpin(S5PV210_GPH3(5), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(5), S3C_GPIO_PULL_NONE);
	//gpio_set_value(S5PV210_GPH3(5), 0);
	
	s3c_gpio_cfgpin(S5PV210_GPH3(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(6), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH3(6), 0);

	s3c_gpio_cfgpin(S5PV210_GPH3(7), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH3(7), S3C_GPIO_PULL_UP);
	gpio_set_value(S5PV210_GPH3(7), 1);
	
    //s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table),
    //                jupiter_sleep_gpio_table);
}

EXPORT_SYMBOL(s3c_config_sleep_gpio);


static struct platform_device *herring_devices[] __initdata = {

	&s5pc110_device_onenand,
#ifdef CONFIG_RTC_DRV_S3C
	&s5p_device_rtc,
#endif
	&s5pv210_device_iis0,
	&s5pv210_device_ac97,
	&s3c_device_wdt,

#ifdef CONFIG_REGULATOR_MAX8998
	&s3c_device_8998consumer,
#endif
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif

#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
#endif

#ifdef CONFIG_FB_S3C_TL2796
	&s3c_device_spi_gpio,
#endif
	&s3c_device_i2c0,
#if defined(CONFIG_S3C_DEV_I2C1)
	&s3c_device_i2c1,
#endif

#if defined(CONFIG_S3C_DEV_I2C2)
	&s3c_device_i2c2,
#endif
        &s3c_device_i2c4,
	&s3c_device_i2c6,
//	&s3c_device_i2c10, /* For touchkey */
#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_adsp_device,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif
};

unsigned int HWREV=0;
EXPORT_SYMBOL(HWREV);

static int read_hwversion(void)
{
        int err;
        int hwver = -1;
        int hwver_0 = -1;
        int hwver_1 = -1;
        int hwver_2 = -1;

        err = gpio_request(S5PV210_GPJ0(2), "HWREV_MODE0");

        if (err) {
                printk(KERN_ERR "failed to request GPJ0(2) for "
                        "HWREV_MODE0\n");
                return err;
        }
        err = gpio_request(S5PV210_GPJ0(3), "HWREV_MODE1");

        if (err) {
                printk(KERN_ERR "failed to request GPJ0(3) for "
                        "HWREV_MODE1\n");
                return err;
        }
        err = gpio_request(S5PV210_GPJ0(4), "HWREV_MODE2");

        if (err) {
                printk(KERN_ERR "failed to request GPJ0(4) for "
                        "HWREV_MODE2\n");
                return err;
        }

        gpio_direction_input(S5PV210_GPJ0(2));
        gpio_direction_input(S5PV210_GPJ0(3));
        gpio_direction_input(S5PV210_GPJ0(4));

        hwver_0 = gpio_get_value(S5PV210_GPJ0(2));
        hwver_1 = gpio_get_value(S5PV210_GPJ0(3));
        hwver_2 = gpio_get_value(S5PV210_GPJ0(4));

        gpio_free(S5PV210_GPJ0(2));
        gpio_free(S5PV210_GPJ0(3));
        gpio_free(S5PV210_GPJ0(4));
	
	if((hwver_0 == 0)&&(hwver_1 == 1)&&(hwver_2 == 0)){
                hwver = 2;
                printk("+++++++++[I9000 Rev0.1 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }
        else if((hwver_0 == 1)&&(hwver_1 == 0)&&(hwver_2 == 1)){
                hwver = 2;
                printk("+++++++++[B5 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }
        else if((hwver_0 == 0)&&(hwver_1 == 1)&&(hwver_2 == 1)){
                hwver = 2;
                printk("+++++++++[ARIES B5 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }
        else{
                hwver = 0;
                //printk("+++++++++[B2, B3 board]++++++++ hwver_0: %d, hwver_1: %d, hwver_2: %d\n", hwver_0, hwver_1, hwver_2);
        }

        return hwver;
}

static void __init herring_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(herring_uartcfgs, ARRAY_SIZE(herring_uartcfgs));
	s5pv210_reserve_bootmem();

#ifdef CONFIG_MTD_ONENAND
	s5pc110_device_onenand.name = "s5pc110-onenand";
#endif
}

static void __init herring_fixup(struct machine_desc *desc,
                                       struct tag *tags, char **cmdline,
                                       struct meminfo *mi)
{

	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 80 * SZ_1M;
	mi->bank[0].node = 0;

	mi->bank[1].start = 0x40000000;
	mi->bank[1].size = 256 * SZ_1M;
	mi->bank[1].node = 1;

	mi->bank[2].start = 0x50000000;
	mi->bank[2].size = 128 * SZ_1M;
	mi->bank[2].node = 2;
	mi->nr_banks = 3;

}

#ifdef CONFIG_S3C_SAMSUNG_PMEM
static void __init s3c_pmem_set_platdata(void)
{
	pmem_pdata.start = s3c_get_media_memory_bank(S3C_MDEV_PMEM, 1);
	pmem_pdata.size = s3c_get_media_memsize_bank(S3C_MDEV_PMEM, 1);
}
#endif

#ifdef CONFIG_FB_S3C_LTE480WV
static struct s3c_platform_fb lte480wv_fb_data __initdata = {
	.hw_ver	= 0x62,
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,
};
#endif
/* this function are used to detect s5pc110 chip version temporally */

int s5pc110_version ;

void _hw_version_check(void)
{
	void __iomem * phy_address ;
	int temp; 

	phy_address = ioremap (0x40,1);

	temp = __raw_readl(phy_address);


	if (temp == 0xE59F010C)
	{
		s5pc110_version = 0;
	}
	else
	{
		s5pc110_version=1 ;
	}
	printk("S5PC110 Hardware version : EVT%d \n",s5pc110_version);
	
	iounmap(phy_address);
}

/* Temporally used
 * return value 0 -> EVT 0
 * value 1 -> evt 1
 */

int hw_version_check(void)
{
	return s5pc110_version ;
}
EXPORT_SYMBOL(hw_version_check);

/* touch screen device init */
static void __init qt_touch_init(void)
{
    int gpio, irq;

        /* qt602240 TSP */
    qt602240_p1_platform_data.blen = 0x1;
    qt602240_p1_platform_data.threshold = 0x13;
    qt602240_p1_platform_data.orient = QT602240_VERTICAL_FLIP;

    gpio = S5PV210_GPG3(6);                     /* XMMC3DATA_3 */
    gpio_request(gpio, "TOUCH_EN");
    s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
    gpio_direction_output(gpio, 1);
    gpio_free(gpio);

    gpio = S5PV210_GPJ0(5);                             /* XMSMADDR_5 */
    gpio_request(gpio, "TOUCH_INT");
    s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
    s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
    irq = gpio_to_irq(gpio);
    gpio_free(gpio);
    
    i2c_devs2[1].irq = irq;
}


extern void set_pmic_gpio(void);
static void jupiter_init_gpio(void)
{
        s3c_config_gpio_table(ARRAY_SIZE(jupiter_gpio_table),
                        jupiter_gpio_table);
        s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table),
                        jupiter_sleep_gpio_table);

	/*Adding pmic gpio(GPH3, GPH4, GPH5) initialisation*/
#if defined(CONFIG_CPU_FREQ)
	set_pmic_gpio();
#endif
}

static void __init herring_machine_init(void)
{
	platform_add_devices(herring_devices, ARRAY_SIZE(herring_devices));
	/* Find out S5PC110 chip version */
	_hw_version_check();

	pm_power_off = herring_power_off ; 


	s3c_gpio_cfgpin(GPIO_HWREV_MODE0, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE0, S3C_GPIO_PULL_NONE); 
	s3c_gpio_cfgpin(GPIO_HWREV_MODE1, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE1, S3C_GPIO_PULL_NONE);  
	s3c_gpio_cfgpin(GPIO_HWREV_MODE2, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE2, S3C_GPIO_PULL_NONE); 
	HWREV = gpio_get_value(GPIO_HWREV_MODE0);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE1) <<1);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE2) <<2);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE3, S3C_GPIO_INPUT);
	s3c_gpio_setpull( GPIO_HWREV_MODE3, S3C_GPIO_PULL_NONE); 
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE3) <<3);
	printk("HWREV is 0x%x\n", HWREV);

	/*initialise the gpio's*/
#if 0	// temporary removed for herring. jc.lee
	jupiter_init_gpio();
#endif

	/* OneNAND */
#ifdef CONFIG_MTD_ONENAND
	//s3c_device_onenand.dev.platform_data = &s5p_onenand_data;
#endif


	qt_touch_init();

#ifdef CONFIG_DM9000
	smdkv210_dm9000_set();
#endif

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif
	{
		int tint = GPIO_TOUCH_INT;
		s3c_gpio_cfgpin(tint, S3C_GPIO_INPUT);
		s3c_gpio_setpull(tint, S3C_GPIO_PULL_UP);
	}

	/* i2c */
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
	i2c_register_board_info(10, i2c_devs10, ARRAY_SIZE(i2c_devs10)); /* for touchkey */
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7)); /* for fsa9480 */

#ifdef CONFIG_FB_S3C_LTE480WV
	s3cfb_set_platdata(&lte480wv_fb_data);
#endif

#if defined(CONFIG_BLK_DEV_IDE_S3C)
	s3c_ide_set_platdata(&smdkv210_ide_pdata);
#endif

#if defined(CONFIG_TOUCHSCREEN_S3C)
	s3c_ts_set_platdata(&s3c_ts_platform);
#endif

#ifdef CONFIG_FB_S3C_TL2796
        spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
        s3cfb_set_platdata(&tl2796_data);
#endif

#if defined(CONFIG_S5PV210_ADCTS)
	s3c_adcts_set_platdata(&s3c_adcts_cfgs);
#endif

#if defined(CONFIG_S5P_ADC)
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif

#if 0 /* will be initialized at pm.c */
#if defined(CONFIG_PM)
	s3c_pm_init();
//	s5pc11x_pm_init();
#endif
#endif

#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
#endif

#ifdef CONFIG_VIDEO_MFC50
	/* mfc */
	s3c_mfc_set_platdata(NULL);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	s5pv210_default_sdhci0();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s5pv210_default_sdhci1();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s5pv210_default_sdhci2();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s5pv210_default_sdhci3();
#endif
#ifdef CONFIG_S5PV210_SETUP_SDHCI
	s3c_sdhci_set_platdata();
#endif
#ifdef CONFIG_BATTERY_S3C
	&sec_device_battery,
#endif

#if defined(CONFIG_BACKLIGHT_PWM)
	smdk_backlight_register();
#endif
	jupiter_switch_init();
}

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void)
{
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		|(0x1<<0), S5P_USB_PHY_CONTROL); /*USB PHY0 Enable */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x3<<3)&~(0x1<<0))|(0x1<<5), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x5<<2))|(0x3<<0), S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x3<<1))|(0x1<<0), S3C_USBOTG_RSTCON);
	udelay(10);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x7<<0), S3C_USBOTG_RSTCON);
	udelay(10);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(64)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x3<<3), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		&~(1<<0), S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "usbotg");
	clk_enable(otg_clk);

	if (readl(S5P_USB_PHY_CONTROL) & (0x1<<1))
		return;

	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		|(0x1<<1), S5P_USB_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x1<<7)&~(0x1<<6))|(0x1<<8)|(0x1<<5), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x1<<7))|(0x3<<0), S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON))
		|(0x1<<4)|(0x1<<3), S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x1<<4)&~(0x1<<3), S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL)
		&~(1<<1), S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(usb_host_phy_off);
#endif

#if defined(CONFIG_KEYPAD_S3C) || defined(CONFIG_KEYPAD_S3C_MODULE)
#if defined(CONFIG_KEYPAD_S3C_MSM)
void s3c_setup_keypad_cfg_gpio(void)
{
	unsigned int gpio;
	unsigned int end;

	/* gpio setting for KP_COL0 */
	s3c_gpio_cfgpin(S5PV210_GPJ1(5), S3C_GPIO_SFN(3));
	s3c_gpio_setpull(S5PV210_GPJ1(5), S3C_GPIO_PULL_NONE);

	/* gpio setting for KP_COL1 ~ KP_COL7 and KP_ROW0 */
	end = S5PV210_GPJ2(8);
	for (gpio = S5PV210_GPJ2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* gpio setting for KP_ROW1 ~ KP_ROW8 */
	end = S5PV210_GPJ3(8);
	for (gpio = S5PV210_GPJ3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* gpio setting for KP_ROW9 ~ KP_ROW13 */
	end = S5PV210_GPJ4(5);
	for (gpio = S5PV210_GPJ4(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}
#else
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = S5PV210_GPH3(rows);

	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = S5PV210_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}

	end = S5PV210_GPH2(columns);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S5PV210_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}
#endif /* if defined(CONFIG_KEYPAD_S3C_MSM)*/
EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif

MACHINE_START(SMDKC110, "SMDKC110")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.fixup		= herring_fixup,
	.init_irq	= s5pv210_init_irq,
	.map_io		= herring_map_io,
	.init_machine	= herring_machine_init,
#if	defined(CONFIG_S5P_HIGH_RES_TIMERS)
	.timer		= &s5p_systimer,
#else
	.timer		= &s3c24xx_timer,
#endif

MACHINE_END

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	switch(port)
	{
	case 0:
		s3c_gpio_cfgpin(GPIO_BT_RXD, S3C_GPIO_SFN(GPIO_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_TXD, S3C_GPIO_SFN(GPIO_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_CTS, S3C_GPIO_SFN(GPIO_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_RTS, S3C_GPIO_SFN(GPIO_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_TXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_CTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		break;
	case 1:
		s3c_gpio_cfgpin(GPIO_GPS_RXD, S3C_GPIO_SFN(GPIO_GPS_RXD_AF));
		s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_GPS_TXD, S3C_GPIO_SFN(GPIO_GPS_TXD_AF));
		s3c_gpio_setpull(GPIO_GPS_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(GPIO_GPS_CTS_AF));
		s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(GPIO_GPS_RTS_AF));
		s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_NONE);
		break;
	case 2:
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
		break;
	case 3:
		s3c_gpio_cfgpin(GPIO_FLM_RXD, S3C_GPIO_SFN(GPIO_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_FLM_TXD, S3C_GPIO_SFN(GPIO_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_FLM_TXD, S3C_GPIO_PULL_NONE);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);
