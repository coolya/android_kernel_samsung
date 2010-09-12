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
#include <linux/gpio_event.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max8998.h>
#include <linux/i2c/qt602240_ts.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/usb/ch9.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/clk.h>
#include <linux/usb/ch9.h>
#include <linux/input/cypress-touchkey.h>
#include <linux/input.h>
#include <linux/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/gpio.h>
#include <mach/gpio-herring.h>
#include <mach/fsa9480_i2c.h>
#include <mach/adc.h>
#include <mach/param.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/wlan_plat.h>
#include <linux/mfd/wm8994/wm8994_pdata.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#include <plat/media.h>
#include <mach/media.h>
#endif

#ifdef CONFIG_S5PV210_POWER_DOMAIN
#include <mach/power-domain.h>
#endif

#include <plat/regs-serial.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/fb.h>
#include <plat/mfc.h>
#include <plat/iic.h>
#include <plat/pm.h>

#include <plat/sdhci.h>
#include <plat/fimc.h>
#include <plat/clock.h>
#include <plat/regs-otg.h>
#ifdef CONFIG_CPU_FREQ
#include <mach/cpu-freq-v210.h>
#endif

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

#define KERNEL_REBOOT_MASK      0xFFFFFFFF
#define REBOOT_MODE_FAST_BOOT		7

static int herring_notifier_call(struct notifier_block *this,
					unsigned long code, void *_cmd)
{
	int mode = REBOOT_MODE_NONE;
	unsigned int temp;

	if ((code == SYS_RESTART) && _cmd) {
		if (!strcmp((char *)_cmd, "recovery"))
			mode = REBOOT_MODE_RECOVERY;
		else if (!strcmp((char *)_cmd, "bootloader"))
			mode = REBOOT_MODE_FAST_BOOT;
		else
			mode = REBOOT_MODE_NONE;
	}
	temp = __raw_readl(S5P_INFORM6);
	temp |= KERNEL_REBOOT_MASK;
	temp &= mode;
	__raw_writel(temp, S5P_INFORM6);

	return NOTIFY_DONE;
}

static struct notifier_block herring_reboot_notifier = {
	.notifier_call = herring_notifier_call,
};

static void gps_gpio_init(void)
{
	struct device *gps_dev;

	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	if (IS_ERR(gps_dev)) {
		pr_err("Failed to create device(gps)!\n");
		goto err;
	}

	gpio_request(GPIO_GPS_nRST, "GPS_nRST");	/* XMMC3CLK */
	s3c_gpio_setpull(GPIO_GPS_nRST, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_nRST, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_nRST, 1);

	gpio_request(GPIO_GPS_PWR_EN, "GPS_PWR_EN");	/* XMMC3CLK */
	s3c_gpio_setpull(GPIO_GPS_PWR_EN, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_PWR_EN, 0);

	s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
	gpio_export(GPIO_GPS_nRST, 1);
	gpio_export(GPIO_GPS_PWR_EN, 1);

	gpio_export_link(gps_dev, "GPS_nRST", GPIO_GPS_nRST);
	gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN);

 err:
	return;
}

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

#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0 (6144 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1 (9900 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2 (6144 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0 (36864 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1 (36864 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD (4800 * SZ_1K)
#define  S5PV210_ANDROID_PMEM_MEMSIZE_PMEM (1024 * SZ_1K)
#define  S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 (8192 * SZ_1K)
#define  S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_ADSP (3600 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG (8192 * SZ_1K)

static struct s5p_media_device herring_media_devs[] = {
	[0] = {
		.id = S5P_MDEV_MFC,
		.name = "mfc",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0,
		.paddr = 0,
	},
	[1] = {
		.id = S5P_MDEV_MFC,
		.name = "mfc",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1,
		.paddr = 0,
	},
	[2] = {
		.id = S5P_MDEV_FIMC0,
		.name = "fimc0",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0,
		.paddr = 0,
	},
	[3] = {
		.id = S5P_MDEV_FIMC1,
		.name = "fimc1",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1,
		.paddr = 0,
	},
	[4] = {
		.id = S5P_MDEV_FIMC2,
		.name = "fimc2",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2,
		.paddr = 0,
	},
	[5] = {
		.id = S5P_MDEV_PMEM,
		.name = "pmem",
		.memsize = S5PV210_ANDROID_PMEM_MEMSIZE_PMEM,
		.paddr = 0,
		.bank = 0, /* OneDRAM */
	},
	[6] = {
		.id = S5P_MDEV_PMEM_GPU1,
		.name = "pmem_gpu1",
		.memsize = S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_GPU1,
		.paddr = 0,
		.bank = 0, /* OneDRAM */
	},
	[7] = {
		.id = S5P_MDEV_PMEM_ADSP,
		.name = "pmem_adsp",
		.memsize = S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_ADSP,
		.paddr = 0,
		.bank = 0, /* OneDRAM */
	},
	[8] = {
		.id = S5P_MDEV_JPEG,
		.name = "jpeg",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG,
		.paddr = 0,
	},
	[9] = {
		.id = S5P_MDEV_FIMD,
		.name = "fimd",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD,
		.paddr = 0,
	},
};

#if defined(CONFIG_TOUCHSCREEN_QT602240)
static struct platform_device s3c_device_qtts = {
	.name	= "qt602240-ts",
	.id	= -1,
};
#endif

static struct regulator_consumer_supply ldo3_consumer[] = {
	{	.supply	= "vdd_otg_d", },
};

static struct regulator_consumer_supply ldo7_consumer[] = {
	{	.supply	= "vlcd", },
};

static struct regulator_consumer_supply ldo8_consumer[] = {
	{	.supply	= "vdd_otg_a", },
};

static struct regulator_consumer_supply ldo11_consumer[] = {
	{	.supply	= "cam_af", },
};

static struct regulator_consumer_supply ldo12_consumer[] = {
	{	.supply	= "cam_sensor", },
};

static struct regulator_consumer_supply ldo13_consumer[] = {
	{	.supply	= "vga_vddio", },
};

static struct regulator_consumer_supply ldo14_consumer[] = {
	{	.supply	= "vga_dvdd", },
};

static struct regulator_consumer_supply ldo15_consumer[] = {
	{	.supply	= "cam_isp_host", },
};

static struct regulator_consumer_supply ldo16_consumer[] = {
	{	.supply	= "vga_avdd", },
};

static struct regulator_consumer_supply ldo17_consumer[] = {
	{	.supply	= "vcc_lcd", },
};

static struct regulator_consumer_supply buck1_consumer[] = {
	{	.supply	= "vddarm", },
};

static struct regulator_consumer_supply buck2_consumer[] = {
	{	.supply	= "vddint", },
};

static struct regulator_consumer_supply buck4_consumer[] = {
	{	.supply	= "cam_isp_core", },
};

static struct regulator_init_data herring_ldo2_data = {
	.constraints	= {
		.name		= "VALIVE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled = 1,
		},
	},
};

static struct regulator_init_data herring_ldo3_data = {
	.constraints	= {
		.name		= "VUSB_1.1V",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo3_consumer),
	.consumer_supplies	= ldo3_consumer,
};

static struct regulator_init_data herring_ldo4_data = {
	.constraints	= {
		.name		= "VADC_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
};

static struct regulator_init_data herring_ldo7_data = {
	.constraints	= {
		.name		= "VLCD_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo7_consumer),
	.consumer_supplies	= ldo7_consumer,
};

static struct regulator_init_data herring_ldo8_data = {
	.constraints	= {
		.name		= "VUSB_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo8_consumer),
	.consumer_supplies	= ldo8_consumer,
};

static struct regulator_init_data herring_ldo9_data = {
	.constraints	= {
		.name		= "VCC_2.8V_PDA",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.always_on	= 1,
	},
};

static struct regulator_init_data herring_ldo11_data = {
	.constraints	= {
		.name		= "CAM_AF_3.0V",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo11_consumer),
	.consumer_supplies	= ldo11_consumer,
};

static struct regulator_init_data herring_ldo12_data = {
	.constraints	= {
		.name		= "CAM_SENSOR_CORE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo12_consumer),
	.consumer_supplies	= ldo12_consumer,
};

static struct regulator_init_data herring_ldo13_data = {
	.constraints	= {
		.name		= "VGA_VDDIO_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo13_consumer),
	.consumer_supplies	= ldo13_consumer,
};

static struct regulator_init_data herring_ldo14_data = {
	.constraints	= {
		.name		= "VGA_DVDD_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo14_consumer),
	.consumer_supplies	= ldo14_consumer,
};

static struct regulator_init_data herring_ldo15_data = {
	.constraints	= {
		.name		= "CAM_ISP_HOST_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo15_consumer),
	.consumer_supplies	= ldo15_consumer,
};

static struct regulator_init_data herring_ldo16_data = {
	.constraints	= {
		.name		= "VGA_AVDD_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo16_consumer),
	.consumer_supplies	= ldo16_consumer,
};

static struct regulator_init_data herring_ldo17_data = {
	.constraints	= {
		.name		= "VCC_3.0V_LCD",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo17_consumer),
	.consumer_supplies	= ldo17_consumer,
};

static struct regulator_init_data herring_buck1_data = {
	.constraints	= {
		.name		= "VDD_ARM",
		.min_uV		= 750000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV	= 1250000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck1_consumer),
	.consumer_supplies	= buck1_consumer,
};


static struct regulator_init_data herring_buck2_data = {
	.constraints	= {
		.name		= "VDD_INT",
		.min_uV		= 750000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV	= 1100000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_consumer),
	.consumer_supplies	= buck2_consumer,
};

static struct regulator_init_data herring_buck3_data = {
	.constraints	= {
		.name		= "VCC_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
	},
};

static struct regulator_init_data herring_buck4_data = {
	.constraints	= {
		.name		= "CAM_ISP_CORE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck4_consumer),
	.consumer_supplies	= buck4_consumer,
};

static struct max8998_regulator_data herring_regulators[] = {
	{ MAX8998_LDO2,  &herring_ldo2_data },
	{ MAX8998_LDO3,  &herring_ldo3_data },
	{ MAX8998_LDO4,  &herring_ldo4_data },
	{ MAX8998_LDO7,  &herring_ldo7_data },
	{ MAX8998_LDO8,  &herring_ldo8_data },
	{ MAX8998_LDO9,  &herring_ldo9_data },
	{ MAX8998_LDO11, &herring_ldo11_data },
	{ MAX8998_LDO12, &herring_ldo12_data },
	{ MAX8998_LDO13, &herring_ldo13_data },
	{ MAX8998_LDO14, &herring_ldo14_data },
	{ MAX8998_LDO15, &herring_ldo15_data },
	{ MAX8998_LDO16, &herring_ldo16_data },
	{ MAX8998_LDO17, &herring_ldo17_data },
	{ MAX8998_BUCK1, &herring_buck1_data },
	{ MAX8998_BUCK2, &herring_buck2_data },
	{ MAX8998_BUCK3, &herring_buck3_data },
	{ MAX8998_BUCK4, &herring_buck4_data },
};

static struct max8998_platform_data herring_platform_data = {
	.num_regulators = ARRAY_SIZE(herring_regulators),
	.regulators     = herring_regulators,
};

struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id	= -1,
};

static void tl2796_cfg_gpio(struct platform_device *pdev)
{
	int i;

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
}

void lcd_cfg_gpio_early_suspend(void)
{
	int i;

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
	/* drive strength to min */
	writel(0x00000000, S5P_VA_GPIO + 0x12c); /* GPF0DRV */
	writel(0x00000000, S5P_VA_GPIO + 0x14c); /* GPF1DRV */
	writel(0x00000000, S5P_VA_GPIO + 0x16c); /* GPF2DRV */
	writel(0x00000000, S5P_VA_GPIO + 0x18c); /* GPF3DRV */

	/* OLED_DET */
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_OLED_DET, 0);

	/* LCD_RST */
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
	/*
	s3c_gpio_cfgpin(S5PV210_MP04(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_MP04(2), S3C_GPIO_PULL_DOWN);
	*/

	/* DISPLAY_SI */
	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_DISPLAY_SI, 0);

	/* OLED_ID */
	s3c_gpio_cfgpin(GPIO_OLED_ID, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OLED_ID, S3C_GPIO_PULL_DOWN);
	/* gpio_set_value(GPIO_OLED_ID, 0); */

	/* DIC_ID */
	s3c_gpio_cfgpin(GPIO_DIC_ID, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_DIC_ID, S3C_GPIO_PULL_DOWN);
	/* gpio_set_value(GPIO_DIC_ID, 0); */
}
EXPORT_SYMBOL(lcd_cfg_gpio_early_suspend);

void lcd_cfg_gpio_late_resume(void)
{
	/* OLED_DET */
	s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
	/* OLED_ID */
	s3c_gpio_cfgpin(GPIO_OLED_ID, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_OLED_ID, S3C_GPIO_PULL_NONE);
	/* gpio_set_value(GPIO_OLED_ID, 0); */
	/* DIC_ID */
	s3c_gpio_cfgpin(GPIO_DIC_ID, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_DIC_ID, S3C_GPIO_PULL_NONE);
	/* gpio_set_value(GPIO_DIC_ID, 0); */
}
EXPORT_SYMBOL(lcd_cfg_gpio_late_resume);

static int tl2796_reset_lcd(struct platform_device *pdev)
{
	int err;

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
	return 0;
}

static struct s3c_platform_fb tl2796_data __initdata = {
	.hw_ver		= 0x62,
	.clk_name	= "sclk_fimd",
	.nr_wins	= 5,
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,

	.cfg_gpio	= tl2796_cfg_gpio,
	.backlight_on	= tl2796_backlight_on,
	.reset_lcd	= tl2796_reset_lcd,
};

#define LCD_BUS_NUM     3
#define DISPLAY_CS      S5PV210_MP01(1)
#define SUB_DISPLAY_CS  S5PV210_MP01(2)
#define DISPLAY_CLK     S5PV210_MP04(1)
#define DISPLAY_SI      S5PV210_MP04(3)

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "tl2796",
		.platform_data	= NULL,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect = 2,
};

static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};

static  struct  i2c_gpio_platform_data  i2c4_platdata = {
	.sda_pin		= GPIO_AP_SDA_18V,
	.scl_pin		= GPIO_AP_SCL_18V,
	.udelay			= 2,    /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c4 = {
	.name			= "i2c-gpio",
	.id			= 4,
	.dev.platform_data	= &i2c4_platdata,
};

static  struct  i2c_gpio_platform_data  i2c5_platdata = {
	.sda_pin		= GPIO_AP_SDA_28V,
	.scl_pin		= GPIO_AP_SCL_28V,
	.udelay			= 2,    /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c5 = {
	.name			= "i2c-gpio",
	.id			= 5,
	.dev.platform_data	= &i2c5_platdata,
};

static struct i2c_gpio_platform_data i2c6_platdata = {
	.sda_pin                = GPIO_AP_PMIC_SDA,
	.scl_pin                = GPIO_AP_PMIC_SCL,
	.udelay                 = 2,    /* 250KHz */
	.sda_is_open_drain      = 0,
	.scl_is_open_drain      = 0,
	.scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c6 = {
	.name			= "i2c-gpio",
	.id			= 6,
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
	.name			= "i2c-gpio",
	.id			= 7,
	.dev.platform_data      = &i2c7_platdata,
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
	.name			= "i2c-gpio",
	.id			= 9,
	.dev.platform_data	= &i2c9_platdata,
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
	.name			= "i2c-gpio",
	.id			= 10,
	.dev.platform_data	= &i2c10_platdata,
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
	.name			= "i2c-gpio",
	.id			= 11,
	.dev.platform_data	= &i2c11_platdata,
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
	.name			= "i2c-gpio",
	.id			= 12,
	.dev.platform_data	= &i2c12_platdata,
};

static void touch_keypad_gpio_init(void)
{
	int ret = 0;

	ret = gpio_request(_3_GPIO_TOUCH_EN, "TOUCH_EN");
	if (ret)
		printk(KERN_ERR "Failed to request gpio touch_en.\n");
}

static void touch_keypad_onoff(int onoff)
{
	gpio_direction_output(_3_GPIO_TOUCH_EN, onoff);

	if (onoff == TOUCHKEY_OFF)
		msleep(30);
}

static const int touch_keypad_code[5] = {
	0,
	KEY_MENU,
	KEY_HOME,
	KEY_BACK,
	KEY_SEARCH
};

static struct touchkey_platform_data touchkey_data = {
	.keycode_cnt = 5,
	.keycode = &touch_keypad_code,
	.touchkey_onoff = &touch_keypad_onoff,
};

#ifdef CONFIG_S5PV210_ADCTS
static struct s3c_adcts_plat_info s3c_adcts_cfgs __initdata = {
	.channel = {
		{ /* 0 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		}, { /* 1 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		}, { /* 2 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		}, { /* 3 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		}, { /* 4 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		}, { /* 5 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		}, { /* 6 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		}, { /* 7 */
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

static struct gpio_event_direct_entry herring_keypad_key_map[] = {
	{
		.gpio	= S5PV210_GPH2(6),
		.code	= KEY_POWER,
	},
	{
		.gpio	= S5PV210_GPH3(1),
		.code	= KEY_VOLUMEDOWN,
	},
	{
		.gpio	= S5PV210_GPH3(2),
		.code	= KEY_VOLUMEUP,
	}
};

static struct gpio_event_input_info herring_keypad_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = false,
	.type = EV_KEY,
	.keymap = herring_keypad_key_map,
	.keymap_size = ARRAY_SIZE(herring_keypad_key_map)
};

static struct gpio_event_info *herring_input_info[] = {
	&herring_keypad_key_info.info,
};


static struct gpio_event_platform_data herring_input_data = {
	.names = {
		"herring-keypad",
		NULL,
	},
	.info = herring_input_info,
	.info_count = ARRAY_SIZE(herring_input_info),
};

static struct platform_device herring_input_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &herring_input_data,
	},
};

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

static struct wm8994_platform_data wm8994_pdata = {
	.ldo = GPIO_CODEC_LDO_EN,
	.ear_sel = GPIO_EAR_SEL,
	.micbias = GPIO_MICBIAS_EN,
};

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
	if (err) {
		printk(KERN_ERR "failed to request GPB7 for camera control\n");
		return err;
	}

	/* Turn CAM_ISP_SYS_2.8V on */
	gpio_direction_output(GPIO_GPB7, 0);
	gpio_set_value(GPIO_GPB7, 1);

	mdelay(1);

	/* Turn CAM_SENSOR_A2.8V on */
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	/* Turn CAM_ISP_HOST_2.8V on */
	Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO15, 1);

	mdelay(1);

	/* Turn CAM_ISP_RAM_1.8V on */
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);

	mdelay(1);

	gpio_free(GPIO_GPB7);

	mdelay(1);

	/* CAM_VGA_nSTBY  HIGH */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	/* Mclk enable */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PV210_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	/* CAM_VGA_nRST  HIGH */
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

	/* CAM_VGA_nSTBY - GPB(0) */
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

	/* CAM_VGA_nRST  LOW */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	/* Mclk disable */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	mdelay(1);

	/* CAM_VGA_nSTBY  LOW */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");
	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");
		return err;
	}

	/* Turn CAM_ISP_HOST_2.8V off */
	Set_MAX8998_PM_REG(ELDO15, 0);

	mdelay(1);

	/* Turn CAM_SENSOR_A2.8V off */
	Set_MAX8998_PM_REG(ELDO13, 0);

	/* Turn CAM_ISP_RAM_1.8V off */
	Set_MAX8998_PM_REG(ELDO14, 0);

	/* Turn CAM_ISP_SYS_2.8V off */
	gpio_direction_output(GPIO_GPB7, 1);
	gpio_set_value(GPIO_GPB7, 0);

	gpio_free(GPIO_GPB7);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5ka3dfx_power_en(int onoff)
{
	if (onoff) {
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
	.clk_name	= "sclk_cam",
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
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
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
	.name	= "pwm-backlight",
	.id	= -1,
	.dev	= {
		.parent = &s3c_device_timer[3].dev,
		.platform_data = &smdk_backlight_data,
	},
};

static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);

	if (ret)
		printk(KERN_ERR
			"smdk: failed to register backlight device: %d\n", ret);
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
	{
		I2C_BOARD_INFO("wm8994", (0x34>>1)),
		.platform_data = &wm8994_pdata,
	},
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
		I2C_BOARD_INFO("cypress_touchkey", 0x20),
		.platform_data  = &touchkey_data,
		.irq = (IRQ_EINT_GROUP22_BASE + 1),
	},
};

static struct i2c_board_info i2c_devs5[] __initdata = {
	{
		I2C_BOARD_INFO("kr3dm", 0x09),
	},
};

static struct fsa9480_i2c_platform_data fsa9480_pdata = {
	.usb_sel = GPIO_USB_SEL,
	.uart_sel = GPIO_UART_SEL,
	.jack_nint = GPIO_JACK_nINT,
};

static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("fsa9480", 0x4A >> 1),
		.platform_data = &fsa9480_pdata,
	},
};

static struct i2c_board_info i2c_devs6[] __initdata = {
#ifdef CONFIG_REGULATOR_MAX8998
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8998", (0xCC >> 1)),
		.platform_data = &herring_platform_data,
	}, {
		I2C_BOARD_INFO("rtc_max8998", (0x0D >> 1)),
	},
#endif
};

static struct i2c_board_info i2c_devs9[] __initdata = {
	{
		I2C_BOARD_INFO("max1704x", (0x6D >> 1)),
	},
};

static struct i2c_board_info i2c_devs11[] __initdata = {
	{
		I2C_BOARD_INFO("gp2a", (0x88 >> 1)),
	},
};

static struct i2c_board_info i2c_devs12[] __initdata = {
	{
		I2C_BOARD_INFO("ak8973b", 0x1c),
	},
};

static struct resource ram_console_resource[] = {
	{
		.flags = IORESOURCE_MEM,
	}
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources = ARRAY_SIZE(ram_console_resource),
	.resource = ram_console_resource,
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
	.start = 0,
	.size = 0,
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
	pmem_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM, 0);
	pmem_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM, 0);

	pmem_gpu1_pdata.start =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_GPU1, 0);
	pmem_gpu1_pdata.size =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_GPU1, 0);

	pmem_adsp_pdata.start =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_ADSP, 0);
	pmem_adsp_pdata.size =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_ADSP, 0);
}
#endif

struct platform_device sec_device_battery = {
	.name	= "sec-battery",
	.id	= -1,
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
};

static struct platform_device sec_device_rfkill = {
	.name	= "bt_rfkill",
	.id	= -1,
};

static struct platform_device sec_device_btsleep = {
	.name	= "bt_sleep",
	.id	= -1,
};

/*Adding gpio settings during bootup and sleep*/
#define S3C_GPIO_SETPIN_ZERO         0
#define S3C_GPIO_SETPIN_ONE          1
#define S3C_GPIO_SETPIN_NONE	     2

/*
 * GPIO Initialization table. It has the following format
 * { pin number, pin configuration, pin value,
 *   pullup/down config, driver strength, slew rate,
 *   sleep mode pin conf, sleep mode pullup/down config }
 *
 * The table can be modified with the appropriate value for each pin.
 */
static unsigned int jupiter_gpio_table[][8] = {
	/* Off part */
	{ S5PV210_GPB(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPB(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPB(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPB(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPB(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPB(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPB(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPB(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPC0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPC1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPC1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPD0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPD1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPD1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPE0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPE1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPE1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPF3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPF3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPG0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPG1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG1(2), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPG2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPG3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPG3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	/* Alive part */
	{ S5PV210_GPH0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH0(3), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO,
	  S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH0(4), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO,
	  S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH0(5), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO,
	  S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH0(7), S3C_GPIO_SFN(0xF), S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },

	{ S5PV210_GPH1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH1(1), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH1(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH1(7), S3C_GPIO_SFN(0xF), S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },

	{ S5PV210_GPH2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH2(1), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH2(6), S3C_GPIO_SFN(0xf), S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_UP, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },

	{ S5PV210_GPH3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH3(1), S3C_GPIO_SFN(0xf), S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_UP, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH3(2), S3C_GPIO_SFN(0xf), S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_UP, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },
	{ S5PV210_GPH3(7), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  0, 0 },

	/* Alive part ending and off part start*/
	{ S5PV210_GPI(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPI(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPI(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPI(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPI(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPI(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPI(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPJ0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ0(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ0(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPJ1(0), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S5PV210_GPJ1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ1(2), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S5PV210_GPJ1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPJ2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPJ3(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ3(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_GPJ4(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ4(1), S3C_GPIO_SFN(0xff), S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_NONE, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ4(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ4(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_GPJ4(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	/* memory part */
	{ S5PV210_MP01(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP01(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP01(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP01(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP01(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP01(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP01(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP01(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_MP02(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP02(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP02(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP02(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_MP03(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP03(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP03(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP03(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP03(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP03(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP03(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP03(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_MP04(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP04(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP04(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP04(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP04(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP04(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP04(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP04(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_MP05(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP05(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP05(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP05(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP05(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP05(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP05(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP05(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_MP06(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP06(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP06(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP06(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP06(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP06(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP06(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP06(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

	{ S5PV210_MP07(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP07(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP07(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP07(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP07(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP07(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP07(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ S5PV210_MP07(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
	  S3C_GPIO_PULL_DOWN, S3C_GPIO_DRVSTR_1X, S3C_GPIO_SLEWRATE_FAST,
	  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* Memory part ending and off part ending */
};

void s3c_config_gpio_table(int array_size, unsigned int (*gpio_table)[8])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		/* Off part */
		if ((gpio <= S5PV210_GPG3(6)) ||
		    ((gpio <= S5PV210_GPJ4(7)) && (gpio >= S5PV210_GPI(0)))) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);

			if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);

			s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
			s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);
		} else if ((gpio <= S5PV210_GPH3(7)) &&
			   (gpio >= S5PV210_GPH0(0))) {
			/* Alive part */
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);

			if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);

			s3c_gpio_set_drvstrength(gpio, gpio_table[i][4]);
			s3c_gpio_set_slewrate(gpio, gpio_table[i][5]);
		}
	}
}

#define S5PV210_PS_HOLD_CONTROL_REG (S3C_VA_SYS+0xE81C)
static void herring_power_off(void)
{
	printk(KERN_DEBUG "herring_power_off\n");

	/* power off code */
	/* PS_HOLD high  PS_HOLD_CONTROL, R/W, 0xE010_E81C */
	writel(readl(S5PV210_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF,
		     S5PV210_PS_HOLD_CONTROL_REG);

	while (1)
		;
}

/* this table only for B4 board */
static unsigned int jupiter_sleep_gpio_table[][3] = {
	{ S5PV210_GPA0(0), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPA0(1), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPA0(2), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPA0(3), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPA0(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPA0(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPA0(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPA0(7), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPA1(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPA1(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPA1(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPA1(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPB(0),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPB(1),  S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPB(2),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPB(3),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPB(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPB(5),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPB(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPB(7),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPC0(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPC0(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPC0(2), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPC0(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPC0(4), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPC1(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPC1(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPC1(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPC1(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPC1(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPD0(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPD0(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPD0(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPD0(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPD1(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPD1(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPD1(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPD1(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPD1(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPD1(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPE0(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE0(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE0(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE0(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE0(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE0(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE0(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE0(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPE1(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE1(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE1(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPE1(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPE1(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPF0(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF0(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF0(2), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF0(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF0(4), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF0(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF0(6), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF0(7), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPF1(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF1(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF1(2), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF1(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF1(4), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF1(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF1(6), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF1(7), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPF2(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF2(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF2(2), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF2(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF2(4), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF2(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF2(6), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF2(7), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPF3(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF3(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF3(2), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF3(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF3(4), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPF3(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPG0(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG0(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG0(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG0(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG0(4), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG0(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG0(6), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPG1(0), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG1(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG1(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG1(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG1(4), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG1(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG1(6), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPG2(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG2(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG2(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG2(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG2(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG2(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPG2(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPG3(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG3(1), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG3(2), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG3(3), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG3(4), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG3(5), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPG3(6), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},

	/* Alive part ending and off part start*/
	{ S5PV210_GPI(0),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPI(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPI(2),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPI(3),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPI(4),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPI(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPI(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_GPJ0(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ0(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ0(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ0(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ0(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ0(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ0(6), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ0(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPJ1(0), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ1(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ1(2), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ1(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ1(4), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ1(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPJ2(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ2(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ2(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ2(3), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ2(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ2(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ2(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ2(7), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPJ3(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ3(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ3(2), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ3(3), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ3(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ3(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ3(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ3(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},

	{ S5PV210_GPJ4(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ4(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_GPJ4(2), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ4(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_GPJ4(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	/* memory part */
	{ S5PV210_MP01(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP01(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP01(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP01(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP01(4), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP01(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP01(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP01(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_MP02(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP02(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP02(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP02(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_MP03(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP03(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP03(2), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP03(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP03(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP03(5), S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP03(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP03(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_MP04(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP04(1), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP04(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP04(3), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP04(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP04(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP04(6), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP04(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_MP05(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP05(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP05(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP05(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP05(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP05(5), S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},
	{ S5PV210_MP05(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP05(7), S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},

	{ S5PV210_MP06(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP06(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP06(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP06(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP06(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP06(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP06(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP06(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	{ S5PV210_MP07(0), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP07(1), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP07(2), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP07(3), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP07(4), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP07(5), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP07(6), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
	{ S5PV210_MP07(7), S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

	/* Memory part ending and off part ending */
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

void s3c_config_sleep_gpio(void)
{
	/* setting the alive mode registers */
	s3c_gpio_cfgpin(S5PV210_GPH0(1), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(1), S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(S5PV210_GPH0(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(3), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH0(3), 0);

	s3c_gpio_cfgpin(S5PV210_GPH0(4), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(4), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH0(4), 0);

	s3c_gpio_cfgpin(S5PV210_GPH0(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH0(5), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH0(5), 0);

	s3c_gpio_cfgpin(S5PV210_GPH1(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(0), S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(S5PV210_GPH1(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH1(1), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH1(1), 0);

	s3c_gpio_cfgpin(S5PV210_GPH1(2), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(2), S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(S5PV210_GPH1(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(4), S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(S5PV210_GPH1(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH1(5), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH1(5), 0);

	s3c_gpio_cfgpin(S5PV210_GPH1(6), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(6), S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(S5PV210_GPH1(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH1(7), S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(S5PV210_GPH2(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH2(0), S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(S5PV210_GPH2(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(2), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH2(2), 0);

	s3c_gpio_cfgpin(S5PV210_GPH2(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPH2(3), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPH2(3), 0);

	s3c_gpio_cfgpin(S5PV210_GPH3(0), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(0), S3C_GPIO_PULL_UP);

	s3c_gpio_cfgpin(S5PV210_GPH3(3), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(3), S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(S5PV210_GPH3(4), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_DOWN);
}
EXPORT_SYMBOL(s3c_config_sleep_gpio);

static unsigned int wlan_sdio_on_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, GPIO_WLAN_SDIO_D0_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, GPIO_WLAN_SDIO_D1_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, GPIO_WLAN_SDIO_D2_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, GPIO_WLAN_SDIO_D3_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_off_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static int wlan_power_en(int onoff)
{
	u32 i;
	u32 sdio;

	if (onoff) {
		s3c_gpio_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_WLAN_BT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);

		s3c_gpio_cfgpin(GPIO_WLAN_nRST,
				S3C_GPIO_SFN(GPIO_WLAN_nRST_AF));
		s3c_gpio_setpull(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);

		s3c_gpio_cfgpin(GPIO_WLAN_HOST_WAKE,
				S3C_GPIO_SFN(GPIO_WLAN_HOST_WAKE_AF));
		s3c_gpio_setpull(GPIO_WLAN_HOST_WAKE, S3C_GPIO_PULL_DOWN);

		s3c_gpio_cfgpin(GPIO_WLAN_WAKE,
				S3C_GPIO_SFN(GPIO_WLAN_WAKE_AF));
		s3c_gpio_setpull(GPIO_WLAN_WAKE, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_WAKE, GPIO_LEVEL_LOW);

		for (i = 0; i < ARRAY_SIZE(wlan_sdio_on_table); i++) {
			sdio = wlan_sdio_on_table[i][0];
			s3c_gpio_cfgpin(sdio,
					S3C_GPIO_SFN(wlan_sdio_on_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_on_table[i][3]);
			if (wlan_sdio_on_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_on_table[i][2]);
		}
		udelay(5);

		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);
	} else {
		if (gpio_get_value(GPIO_BT_nRST) == 0) {
			gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);
			s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
		}
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);

		for (i = 0; i < ARRAY_SIZE(wlan_sdio_off_table); i++) {
			sdio = wlan_sdio_off_table[i][0];
			s3c_gpio_cfgpin(sdio,
				S3C_GPIO_SFN(wlan_sdio_off_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_off_table[i][3]);
			if (wlan_sdio_off_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_off_table[i][2]);
		}
	}
	return 0;
}

static int wlan_reset_en(int onoff)
{
	gpio_set_value(GPIO_WLAN_nRST,
			onoff ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	return 0;
}

static int wlan_carddetect_en(int val)
{
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc3);
	return 0;
}

static struct resource wifi_resources[] = {
	[0] = {
		.name	= "bcm4329_wlan_irq",
		.start	= IRQ_EINT(20),
		.end	= IRQ_EINT(20),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct wifi_platform_data wifi_pdata = {
	.set_power		= wlan_power_en,
	.set_reset		= wlan_reset_en,
	.set_carddetect		= wlan_carddetect_en,
};

static struct platform_device sec_device_wifi = {
	.name			= "bcm4329_wlan",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(wifi_resources),
	.resource		= wifi_resources,
	.dev			= {
		.platform_data = &wifi_pdata,
	},
};

static struct platform_device *herring_devices[] __initdata = {
	&s5pc110_device_onenand,
#ifdef CONFIG_RTC_DRV_S3C
	&s5p_device_rtc,
#endif
	&herring_input_device,

	&s5pv210_device_iis0,
	&s5pv210_device_ac97,
	&s3c_device_wdt,

#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif

#ifdef CONFIG_VIDEO_MFC50
	&s3c_device_mfc,
#endif
#ifdef	CONFIG_S5P_ADC
	&s3c_device_adc,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
#endif

#ifdef CONFIG_VIDEO_JPEG_V2
	&s3c_device_jpeg,
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
	&s3c_device_i2c5,  /* accel sensor */
	&s3c_device_i2c6,
	&s3c_device_i2c7,
	&s3c_device_i2c9,  /* max1704x:fuel_guage */
	&s3c_device_i2c11, /* optical sensor */
	&s3c_device_i2c12, /* magnetic sensor */
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID
	&s3c_device_android_usb,
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	&s3c_device_usb_mass_storage,
#endif
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif

	&sec_device_battery,
	&s3c_device_i2c10,

#ifdef CONFIG_S5PV210_POWER_DOMAIN
	&s5pv210_pd_audio,
	&s5pv210_pd_cam,
	&s5pv210_pd_tv,
	&s5pv210_pd_lcd,
	&s5pv210_pd_mfc,
#endif

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
#ifdef CONFIG_TOUCHSCREEN_QT602240
	&s3c_device_qtts,
#endif
	&opt_gp2a,
	&sec_device_rfkill,
	&sec_device_btsleep,
	&ram_console_device,
	&sec_device_wifi,
};

unsigned int HWREV;
EXPORT_SYMBOL(HWREV);

static void __init herring_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s5pv210_gpiolib_init();
	s3c24xx_init_uarts(herring_uartcfgs, ARRAY_SIZE(herring_uartcfgs));
	s5p_reserve_bootmem(herring_media_devs, ARRAY_SIZE(herring_media_devs));
#ifdef CONFIG_MTD_ONENAND
	s5pc110_device_onenand.name = "s5pc110-onenand";
#endif
}

static unsigned int ram_console_start;
static unsigned int ram_console_size;

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
	/* 1M for ram_console buffer */
	mi->bank[2].size = 127 * SZ_1M;
	mi->bank[2].node = 2;
	mi->nr_banks = 3;

	ram_console_start = mi->bank[2].start + mi->bank[2].size;
	ram_console_size = SZ_1M;
}

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
	void __iomem *phy_address ;
	int temp;

	phy_address = ioremap(0x40, 1);

	temp = __raw_readl(phy_address);

	if (temp == 0xE59F010C)
		s5pc110_version = 0;
	else
		s5pc110_version = 1;

	printk(KERN_INFO "S5PC110 Hardware version : EVT%d\n",
				s5pc110_version);

	iounmap(phy_address);
}

/*
 * Temporally used
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

	gpio = S5PV210_GPG3(6);	/* XMMC3DATA_3 */
	gpio_request(gpio, "TOUCH_EN");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
	gpio_direction_output(gpio, 1);
	gpio_free(gpio);

	gpio = S5PV210_GPJ0(5);	/* XMSMADDR_5 */
	gpio_request(gpio, "TOUCH_INT");
	s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	irq = gpio_to_irq(gpio);
	gpio_free(gpio);

	i2c_devs2[1].irq = irq;
}

static void jupiter_init_gpio(void)
{
	s3c_config_gpio_table(ARRAY_SIZE(jupiter_gpio_table),
			jupiter_gpio_table);
	s3c_config_sleep_gpio_table(ARRAY_SIZE(jupiter_sleep_gpio_table),
			jupiter_sleep_gpio_table);
}

static void __init fsa9480_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_USB_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_USB_SEL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_JACK_nINT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_JACK_nINT, S3C_GPIO_PULL_NONE);
}

static void __init setup_ram_console_mem(void)
{
	ram_console_resource[0].start = ram_console_start;
	ram_console_resource[0].end = ram_console_start + ram_console_size - 1;
}

static void __init sound_init(void)
{
	u32 reg;

	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x3 << 8);
	reg |= 3 << 8;
	__raw_writel(reg, S5P_OTHERS);

	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~(0x1f << 12);
	reg |= 19 << 12;
	__raw_writel(reg, S5P_CLK_OUT);

	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~0x1;
	reg |= 0x1;
	__raw_writel(reg, S5P_CLK_OUT);

}
static void __init herring_machine_init(void)
{
	setup_ram_console_mem();
	s3c_usb_set_serial();
	platform_add_devices(herring_devices, ARRAY_SIZE(herring_devices));

	/* Find out S5PC110 chip version */
	_hw_version_check();

	pm_power_off = herring_power_off ;

	s3c_gpio_cfgpin(GPIO_HWREV_MODE0, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE0, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE1, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE1, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE2, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE2, S3C_GPIO_PULL_NONE);
	HWREV = gpio_get_value(GPIO_HWREV_MODE0);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE1) << 1);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE2) << 2);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE3, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE3, S3C_GPIO_PULL_NONE);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE3) << 3);
	printk(KERN_INFO "HWREV is 0x%x\n", HWREV);

	/*initialise the gpio's*/
	jupiter_init_gpio();

	/* OneNAND */
#ifdef CONFIG_MTD_ONENAND
	/* s3c_device_onenand.dev.platform_data = &s5p_onenand_data; */
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
#ifdef CONFIG_S3C_DEV_I2C1
	s3c_i2c1_set_platdata(NULL);
#endif

#ifdef CONFIG_S3C_DEV_I2C2
	s3c_i2c2_set_platdata(NULL);
#endif
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));

	/* wm8994 codec */
	sound_init();
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));

	/* accel sensor */
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
	/* Touch Key */
	touch_keypad_gpio_init();
	i2c_register_board_info(10, i2c_devs10, ARRAY_SIZE(i2c_devs10));
	/* FSA9480 */
	fsa9480_gpio_init();
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));
	i2c_register_board_info(9, i2c_devs9, ARRAY_SIZE(i2c_devs9));
	/* optical sensor */
	i2c_register_board_info(11, i2c_devs11, ARRAY_SIZE(i2c_devs11));
	/* magnetic sensor */
	i2c_register_board_info(12, i2c_devs12, ARRAY_SIZE(i2c_devs12));

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

#if defined(CONFIG_PM)
	s3c_pm_init();
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

#if defined(CONFIG_BACKLIGHT_PWM)
	smdk_backlight_register();
#endif
	register_reboot_notifier(&herring_reboot_notifier);

	jupiter_switch_init();

	gps_gpio_init();
}

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void)
{
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) | (0x1<<0),
			S5P_USB_PHY_CONTROL); /* USB PHY0 Enable */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
			& ~(0x3<<3) & ~(0x1<<0)) | (0x1<<5),
			S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) & ~(0x5<<2)) | (0x3<<0),
			S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON) & ~(0x3<<1)) | (0x1<<0),
			S3C_USBOTG_RSTCON);
	udelay(10);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON) & ~(0x7<<0),
			S3C_USBOTG_RSTCON);
	udelay(10);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(64)));

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR) | (0x3<<3),
			S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) & ~(1<<0),
			S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "otg");
	clk_enable(otg_clk);

	if (readl(S5P_USB_PHY_CONTROL) & (0x1<<1))
		return;

	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) | (0x1<<1),
			S5P_USB_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
			& ~(0x1<<7) & ~(0x1<<6)) | (0x1<<8) | (0x1<<5),
			S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) & ~(0x1<<7)) | (0x3<<0),
			S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON)) | (0x1<<4) | (0x1<<3),
			S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON) & ~(0x1<<4) & ~(0x1<<3),
			S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR) | (0x1<<7)|(0x1<<6),
			S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) & ~(1<<1),
			S5P_USB_PHY_CONTROL);
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
	switch (port) {
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
