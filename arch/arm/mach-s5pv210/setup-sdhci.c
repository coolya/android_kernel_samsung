/* linux/arch/arm/mach-s5pv210/setup-sdhci.c
 *
 * Copyright (c) 2009-2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Helper functions for settign up SDHCI device(s) (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <plat/regs-sdhci.h>
#include <plat/sdhci.h>

#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <asm/mach-types.h>

/* clock sources for the mmc bus clock, order as for the ctrl2[5..4] */
char *s5pv210_hsmmc_clksrcs[4] = {
	[0] = "hsmmc",		/* HCLK */
	[1] = "hsmmc",		/* HCLK */
	[2] = "sclk_mmc",	/* mmc_bus */
	[3] = NULL,		/*reserved */
};

void s5pv210_setup_sdhci0_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;

	switch (width) {
	/* Channel 0 supports 4 and 8-bit bus width */
	case 8:
		/* Set all the necessary GPIO function and pull up/down */
		for (gpio = S5PV210_GPG1(3); gpio <= S5PV210_GPG1(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
			s3c_gpio_set_drvstrength(gpio, S3C_GPIO_DRVSTR_2X);
		}

	case 0:
	case 1:
	case 4:
		/* Set all the necessary GPIO function and pull up/down */
		for (gpio = S5PV210_GPG0(0); gpio <= S5PV210_GPG0(6); gpio++) {
			if (gpio != S5PV210_GPG0(2)) {
				s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
				s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
			}
			s3c_gpio_set_drvstrength(gpio, S3C_GPIO_DRVSTR_2X);
		}
		break;
	default:
		printk(KERN_ERR "Wrong SD/MMC bus width : %d\n", width);
	}

	if (machine_is_herring()) {
		s3c_gpio_cfgpin(S5PV210_GPJ2(7), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV210_GPJ2(7), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPJ2(7), 1);
	}
        if (machine_is_aries()) {
		s3c_gpio_cfgpin(S5PV210_GPJ2(7), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV210_GPJ2(7), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV210_GPJ2(7), 1);
	}
}

void s5pv210_setup_sdhci1_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;

	switch (width) {
	/* Channel 1 supports 4-bit bus width */
	case 0:
	case 1:
	case 4:
		/* Set all the necessary GPIO function and pull up/down */
		for (gpio = S5PV210_GPG1(0); gpio <= S5PV210_GPG1(6); gpio++) {
			if (gpio != S5PV210_GPG1(2)) {
				s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
				s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
			}
			s3c_gpio_set_drvstrength(gpio, S3C_GPIO_DRVSTR_2X);
		}
		break;
	default:
		printk(KERN_ERR "Wrong SD/MMC bus width : %d\n", width);
	}
}

void s5pv210_setup_sdhci2_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;

	switch (width) {
	/* Channel 2 supports 4 and 8-bit bus width */
	case 8:
		/* Set all the necessary GPIO function and pull up/down */
		for (gpio = S5PV210_GPG3(3); gpio <= S5PV210_GPG3(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
			s3c_gpio_set_drvstrength(gpio, S3C_GPIO_DRVSTR_2X);
		}

	case 0:
	case 1:
	case 4:
		/* Set all the necessary GPIO function and pull up/down */
		for (gpio = S5PV210_GPG2(0); gpio <= S5PV210_GPG2(6); gpio++) {
			if (gpio != S5PV210_GPG2(2)) {
				s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
				s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
			}
			s3c_gpio_set_drvstrength(gpio, S3C_GPIO_DRVSTR_2X);
		}
		break;
	default:
		printk(KERN_ERR "Wrong SD/MMC bus width : %d\n", width);
	}
}

void s5pv210_setup_sdhci3_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio;

	switch (width) {
	/* Channel 3 supports 4-bit bus width */
	case 0:
	case 1:
	case 4:
		/* Set all the necessary GPIO function and pull up/down */
		for (gpio = S5PV210_GPG3(0); gpio <= S5PV210_GPG3(6); gpio++) {
			if (gpio != S5PV210_GPG3(2)) {
				s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
				s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
			}
			s3c_gpio_set_drvstrength(gpio, S3C_GPIO_DRVSTR_2X);
		}
		break;
	default:
		printk(KERN_ERR "Wrong SD/MMC bus width : %d\n", width);
	}
}

#define S3C_SDHCI_CTRL3_FCSELTX_INVERT  (0)
#define S3C_SDHCI_CTRL3_FCSELTX_BASIC \
	(S3C_SDHCI_CTRL3_FCSEL3 | S3C_SDHCI_CTRL3_FCSEL2)
#define S3C_SDHCI_CTRL3_FCSELRX_INVERT  (0)
#define S3C_SDHCI_CTRL3_FCSELRX_BASIC \
	(S3C_SDHCI_CTRL3_FCSEL1 | S3C_SDHCI_CTRL3_FCSEL0)

void s5pv210_setup_sdhci_cfg_card(struct platform_device *dev,
				    void __iomem *r,
				    struct mmc_ios *ios,
				    struct mmc_card *card)
{
	u32 ctrl2;
	u32 ctrl3;

	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		  S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
		  S3C_SDHCI_CTRL2_DFCNT_NONE |
		  S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	if (ios->clock <= (400 * 1000)) {
		ctrl2 &= ~(S3C_SDHCI_CTRL2_ENFBCLKTX |
			   S3C_SDHCI_CTRL2_ENFBCLKRX);
		ctrl3 = 0;
	} else {
		u32 range_start;
		u32 range_end;

		ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKTX |
			 S3C_SDHCI_CTRL2_ENFBCLKRX;

		if (card->type == MMC_TYPE_MMC)  /* MMC */
			range_start = 20 * 1000 * 1000;
		else    /* SD, SDIO */
			range_start = 25 * 1000 * 1000;

		range_end = 37 * 1000 * 1000;

		if ((ios->clock > range_start) && (ios->clock < range_end))
			ctrl3 = S3C_SDHCI_CTRL3_FCSELTX_BASIC |
				S3C_SDHCI_CTRL3_FCSELRX_BASIC;
		else
			ctrl3 = S3C_SDHCI_CTRL3_FCSELTX_BASIC |
				S3C_SDHCI_CTRL3_FCSELRX_INVERT;
	}

	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

void s5pv210_adjust_sdhci_cfg_card(struct s3c_sdhci_platdata *pdata,
				   void __iomem *r, int rw)
{
	u32 ctrl2, ctrl3;

	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl3 = readl(r + S3C_SDHCI_CONTROL3);

	if (rw == 0) {
		pdata->rx_cfg++;
		if (pdata->rx_cfg == 1) {
			ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKRX;
			ctrl3 |= S3C_SDHCI_CTRL3_FCSELRX_BASIC;
		} else if (pdata->rx_cfg == 2) {
			ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKRX;
			ctrl3 &= ~S3C_SDHCI_CTRL3_FCSELRX_BASIC;
		} else if (pdata->rx_cfg == 3) {
			ctrl2 &= ~(S3C_SDHCI_CTRL2_ENFBCLKTX |
				   S3C_SDHCI_CTRL2_ENFBCLKRX);
			pdata->rx_cfg = 0;
		}
	} else if (rw == 1) {
		pdata->tx_cfg++;
		if (pdata->tx_cfg == 1) {
			if (ctrl2 & S3C_SDHCI_CTRL2_ENFBCLKRX) {
				ctrl2 |= S3C_SDHCI_CTRL2_ENFBCLKTX;
				ctrl3 |= S3C_SDHCI_CTRL3_FCSELTX_BASIC;
			} else {
				ctrl2 &= ~S3C_SDHCI_CTRL2_ENFBCLKTX;
			}
		} else if (pdata->tx_cfg == 2) {
			ctrl2 &= ~S3C_SDHCI_CTRL2_ENFBCLKTX;
			pdata->tx_cfg = 0;
		}
	} else {
		printk(KERN_ERR "%s, unknown value rw:%d\n", __func__, rw);
		return;
	}

	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

#if defined(CONFIG_MACH_SMDKV210)
static void setup_sdhci0_gpio_wp(void)
{
	s3c_gpio_cfgpin(S5PV210_GPH0(7), S3C_GPIO_INPUT);
	s3c_gpio_setpull(S5PV210_GPH0(7), S3C_GPIO_PULL_DOWN);
}

static int sdhci0_get_ro(struct mmc_host *mmc)
{
	return !!(readl(S5PV210_GPH0DAT) & 0x80);
}
#endif

unsigned int universal_sdhci2_detect_ext_cd(void)
{
	unsigned int card_status = 0;

#ifdef CONFIG_MMC_DEBUG
	printk(KERN_DEBUG "Universal :SD Detect function\n");
	printk(KERN_DEBUG "eint conf %x  eint filter conf %x",
		readl(S5P_EINT_CON(3)), readl(S5P_EINT_FLTCON(3, 1)));
	printk(KERN_DEBUG "eint pend %x  eint mask %x",
		readl(S5P_EINT_PEND(3)), readl(S5P_EINT_MASK(3)));
#endif
	card_status = gpio_get_value(S5PV210_GPH3(4));
	printk(KERN_DEBUG "Universal : Card status %d\n", card_status ? 0 : 1);
	return card_status ? 0 : 1;

}

void universal_sdhci2_cfg_ext_cd(void)
{
	printk(KERN_DEBUG "Universal :SD Detect configuration\n");
#if defined(CONFIG_SAMSUNG_CAPTIVATE) || defined(CONFIG_SAMSUNG_VIBRANT)
  	s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_UP);
#else
  	s3c_gpio_setpull(S5PV210_GPH3(4), S3C_GPIO_PULL_NONE);
#endif
	set_irq_type(IRQ_EINT(28), IRQ_TYPE_EDGE_BOTH);
}

static struct s3c_sdhci_platdata hsmmc0_platdata = {
#if defined(CONFIG_S5PV210_SD_CH0_8BIT)
	.max_width	= 8,
	.host_caps	= MMC_CAP_8_BIT_DATA,
#endif
#if defined(CONFIG_MACH_SMDKV210)
	.cfg_wp         = setup_sdhci0_gpio_wp,
	.get_ro         = sdhci0_get_ro,
#endif
};

#if defined(CONFIG_S3C_DEV_HSMMC1)
static struct s3c_sdhci_platdata hsmmc1_platdata = { 0 };
#endif

#if defined(CONFIG_S3C_DEV_HSMMC2)
static struct s3c_sdhci_platdata hsmmc2_platdata = {
#if defined(CONFIG_S5PV210_SD_CH2_8BIT)
	.max_width	= 8,
	.host_caps	= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#if defined(CONFIG_S3C_DEV_HSMMC3)
static struct s3c_sdhci_platdata hsmmc3_platdata = { 0 };
#endif

void s3c_sdhci_set_platdata(void)
{
#if defined(CONFIG_S3C_DEV_HSMMC)
	s3c_sdhci0_set_platdata(&hsmmc0_platdata);
#endif
#if defined(CONFIG_S3C_DEV_HSMMC1)
	if (machine_is_aries())
		hsmmc1_platdata.built_in = 1;
	s3c_sdhci1_set_platdata(&hsmmc1_platdata);
#endif
#if defined(CONFIG_S3C_DEV_HSMMC2)
	if (machine_is_herring()) {
		hsmmc2_platdata.ext_cd = IRQ_EINT(28);
		hsmmc2_platdata.cfg_ext_cd = universal_sdhci2_cfg_ext_cd;
		hsmmc2_platdata.detect_ext_cd = universal_sdhci2_detect_ext_cd;
	}
        if (machine_is_aries()) {
		hsmmc2_platdata.ext_cd = IRQ_EINT(28);
		hsmmc2_platdata.cfg_ext_cd = universal_sdhci2_cfg_ext_cd;
		hsmmc2_platdata.detect_ext_cd = universal_sdhci2_detect_ext_cd;
	}

	s3c_sdhci2_set_platdata(&hsmmc2_platdata);
#endif
#if defined(CONFIG_S3C_DEV_HSMMC3)
	if (machine_is_herring())
		hsmmc3_platdata.built_in = 1;
	s3c_sdhci3_set_platdata(&hsmmc3_platdata);
        if (machine_is_aries())
		hsmmc3_platdata.built_in = 1;
	s3c_sdhci3_set_platdata(&hsmmc3_platdata);
#endif
};
