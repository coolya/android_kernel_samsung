/* linux/arch/arm/mach-s5pv210/gpiolib.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - GPIOlib support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>
#include <mach/map.h>
#include <mach/regs-gpio.h>

static struct s3c_gpio_cfg gpio_cfg = {
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static struct s3c_gpio_cfg gpio_cfg_noint = {
	.set_config	= s3c_gpio_setcfg_s3c64xx_4bit,
	.set_pull	= s3c_gpio_setpull_updown,
	.get_pull	= s3c_gpio_getpull_updown,
};

static int s5p_gpiolib_eint_to_irq(struct gpio_chip *chip, unsigned int offset)
{
	struct s3c_gpio_chip *s3c_chip = to_s3c_gpio(chip);

	return s3c_chip->eint_offset + offset;
}

/* be called from gpio_to_irq() for gpio interrupts */
static int s5p_gpiolib_gpioint_to_irq(struct gpio_chip *chip, unsigned int offset)
{
	return S5P_IRQ_GPIOINT(chip->base + offset);
}

/* GPIO bank's base address given the index of the bank in the
 * list of all gpio banks.
 */
#define S5PV210_BANK_BASE(bank_nr)	(S5P_VA_GPIO + ((bank_nr) * 0x20))

/*
 * Following are the gpio banks in v210.
 *
 * The 'config' member when left to NULL, is initialized to the default
 * structure gpio_cfg in the init function below.
 *
 * The 'base' member is also initialized in the init function below.
 * Note: The initialization of 'base' member of s3c_gpio_chip structure
 * uses the above macro and depends on the banks being listed in order here.
 */
static struct s3c_gpio_chip s5pv210_gpio_4bit[] = {
	{
		.chip	= {
			.base	= S5PV210_GPA0(0),
			.ngpio	= S5PV210_GPIO_A0_NR,
			.label	= "GPA0",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPA1(0),
			.ngpio	= S5PV210_GPIO_A1_NR,
			.label	= "GPA1",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPB(0),
			.ngpio	= S5PV210_GPIO_B_NR,
			.label	= "GPB",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPC0(0),
			.ngpio	= S5PV210_GPIO_C0_NR,
			.label	= "GPC0",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPC1(0),
			.ngpio	= S5PV210_GPIO_C1_NR,
			.label	= "GPC1",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPD0(0),
			.ngpio	= S5PV210_GPIO_D0_NR,
			.label	= "GPD0",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPD1(0),
			.ngpio	= S5PV210_GPIO_D1_NR,
			.label	= "GPD1",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPE0(0),
			.ngpio	= S5PV210_GPIO_E0_NR,
			.label	= "GPE0",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPE1(0),
			.ngpio	= S5PV210_GPIO_E1_NR,
			.label	= "GPE1",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPF0(0),
			.ngpio	= S5PV210_GPIO_F0_NR,
			.label	= "GPF0",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPF1(0),
			.ngpio	= S5PV210_GPIO_F1_NR,
			.label	= "GPF1",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPF2(0),
			.ngpio	= S5PV210_GPIO_F2_NR,
			.label	= "GPF2",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPF3(0),
			.ngpio	= S5PV210_GPIO_F3_NR,
			.label	= "GPF3",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPG0(0),
			.ngpio	= S5PV210_GPIO_G0_NR,
			.label	= "GPG0",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPG1(0),
			.ngpio	= S5PV210_GPIO_G1_NR,
			.label	= "GPG1",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPG2(0),
			.ngpio	= S5PV210_GPIO_G2_NR,
			.label	= "GPG2",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPG3(0),
			.ngpio	= S5PV210_GPIO_G3_NR,
			.label	= "GPG3",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.config = &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_GPI(0),
			.ngpio	= S5PV210_GPIO_I_NR,
			.label	= "GPI",
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPJ0(0),
			.ngpio	= S5PV210_GPIO_J0_NR,
			.label	= "GPJ0",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPJ1(0),
			.ngpio	= S5PV210_GPIO_J1_NR,
			.label	= "GPJ1",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPJ2(0),
			.ngpio	= S5PV210_GPIO_J2_NR,
			.label	= "GPJ2",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPJ3(0),
			.ngpio	= S5PV210_GPIO_J3_NR,
			.label	= "GPJ3",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.chip	= {
			.base	= S5PV210_GPJ4(0),
			.ngpio	= S5PV210_GPIO_J4_NR,
			.label	= "GPJ4",
			.to_irq = s5p_gpiolib_gpioint_to_irq,
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP01(0),
			.ngpio	= S5PV210_GPIO_MP01_NR,
			.label	= "MP01",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP02(0),
			.ngpio	= S5PV210_GPIO_MP02_NR,
			.label	= "MP02",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP03(0),
			.ngpio	= S5PV210_GPIO_MP03_NR,
			.label	= "MP03",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP04(0),
			.ngpio	= S5PV210_GPIO_MP04_NR,
			.label	= "MP04",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP05(0),
			.ngpio	= S5PV210_GPIO_MP05_NR,
			.label	= "MP05",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP06(0),
			.ngpio	= S5PV210_GPIO_MP06_NR,
			.label	= "MP06",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP07(0),
			.ngpio	= S5PV210_GPIO_MP07_NR,
			.label	= "MP07",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP10(0),
			.ngpio	= S5PV210_GPIO_MP10_NR,
			.label	= "MP10",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP11(0),
			.ngpio	= S5PV210_GPIO_MP11_NR,
			.label	= "MP11",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP12(0),
			.ngpio	= S5PV210_GPIO_MP12_NR,
			.label	= "MP12",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP13(0),
			.ngpio	= S5PV210_GPIO_MP13_NR,
			.label	= "MP13",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP14(0),
			.ngpio	= S5PV210_GPIO_MP14_NR,
			.label	= "MP14",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP15(0),
			.ngpio	= S5PV210_GPIO_MP15_NR,
			.label	= "MP15",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP16(0),
			.ngpio	= S5PV210_GPIO_MP16_NR,
			.label	= "MP16",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP17(0),
			.ngpio	= S5PV210_GPIO_MP17_NR,
			.label	= "MP17",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP18(0),
			.ngpio	= S5PV210_GPIO_MP18_NR,
			.label	= "MP18",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP20(0),
			.ngpio	= S5PV210_GPIO_MP20_NR,
			.label	= "MP20",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP21(0),
			.ngpio	= S5PV210_GPIO_MP21_NR,
			.label	= "MP21",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP22(0),
			.ngpio	= S5PV210_GPIO_MP22_NR,
			.label	= "MP22",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP23(0),
			.ngpio	= S5PV210_GPIO_MP23_NR,
			.label	= "MP23",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP24(0),
			.ngpio	= S5PV210_GPIO_MP24_NR,
			.label	= "MP24",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP25(0),
			.ngpio	= S5PV210_GPIO_MP25_NR,
			.label	= "MP25",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP26(0),
			.ngpio	= S5PV210_GPIO_MP26_NR,
			.label	= "MP26",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP27(0),
			.ngpio	= S5PV210_GPIO_MP27_NR,
			.label	= "MP27",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_MP28(0),
			.ngpio	= S5PV210_GPIO_MP28_NR,
			.label	= "MP28",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_ETC0(0),
			.ngpio	= S5PV210_GPIO_ETC0_NR,
			.label	= "ETC0",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_ETC1(0),
			.ngpio	= S5PV210_GPIO_ETC1_NR,
			.label	= "ETC1",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_ETC2(0),
			.ngpio	= S5PV210_GPIO_ETC2_NR,
			.label	= "ETC2",
		},
	}, {
		.config	= &gpio_cfg_noint,
		.chip	= {
			.base	= S5PV210_ETC4(0),
			.ngpio	= S5PV210_GPIO_ETC4_NR,
			.label	= "ETC4",
		},
	}, {
		.base	= (S5P_VA_GPIO + 0xC00),
		.config	= &gpio_cfg_noint,
		.eint_offset = IRQ_EINT(0),
		.chip	= {
			.base	= S5PV210_GPH0(0),
			.ngpio	= S5PV210_GPIO_H0_NR,
			.label	= "GPH0",
			.to_irq = s5p_gpiolib_eint_to_irq,
		},
	}, {
		.base	= (S5P_VA_GPIO + 0xC20),
		.config	= &gpio_cfg_noint,
		.eint_offset = IRQ_EINT(8),
		.chip	= {
			.base	= S5PV210_GPH1(0),
			.ngpio	= S5PV210_GPIO_H1_NR,
			.label	= "GPH1",
			.to_irq = s5p_gpiolib_eint_to_irq,
		},
	}, {
		.base	= (S5P_VA_GPIO + 0xC40),
		.config	= &gpio_cfg_noint,
		.eint_offset = IRQ_EINT(16),
		.chip	= {
			.base	= S5PV210_GPH2(0),
			.ngpio	= S5PV210_GPIO_H2_NR,
			.label	= "GPH2",
			.to_irq = s5p_gpiolib_eint_to_irq,
		},
	}, {
		.base	= (S5P_VA_GPIO + 0xC60),
		.config	= &gpio_cfg_noint,
		.eint_offset = IRQ_EINT(24),
		.chip	= {
			.base	= S5PV210_GPH3(0),
			.ngpio	= S5PV210_GPIO_H3_NR,
			.label	= "GPH3",
			.to_irq = s5p_gpiolib_eint_to_irq,
		},
	},
};

/* S5PV210 machine dependent GPIO help function */
int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin <= S5PV210_GPH3(7)) && (pin >= S5PV210_GPH0(0)))
		return -EINVAL;

	if (config > S3C_GPIO_SLP_PREV)
		return -EINVAL;

	reg = chip->base + 0x10;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con &= ~(3 << shift);
	con |= config << shift;
	__raw_writel(con, reg);

	local_irq_restore(flags);
	return 0;
}

s3c_gpio_pull_t s3c_gpio_get_slp_cfgpin(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin <= S5PV210_GPH3(7)) && (pin >= S5PV210_GPH0(0)))
		return -EINVAL;

	reg = chip->base + 0x10;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con >>= shift;
	con &= 0x3;

	local_irq_restore(flags);

	return (__force s3c_gpio_pull_t)con;
}

int s3c_gpio_slp_setpull_updown(unsigned int pin, unsigned int config)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if ((pin <= S5PV210_GPH3(7)) && (pin >= S5PV210_GPH0(0)))
		return -EINVAL;

	if (config > S3C_GPIO_PULL_UP)
		return -EINVAL;
	reg = chip->base + 0x14;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con &= ~(3 << shift);
	con |= config << shift;
	__raw_writel(con, reg);

	local_irq_restore(flags);

	return 0;
}
EXPORT_SYMBOL(s3c_gpio_slp_setpull_updown);

int s3c_gpio_set_drvstrength(unsigned int pin, unsigned int config)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if (config  > S3C_GPIO_DRVSTR_4X)
		return -EINVAL;

	reg = chip->base + 0x0c;

	offset = pin - chip->chip.base;
	shift = offset * 2;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con &= ~(3 << shift);
	con |= config << shift;

	__raw_writel(con, reg);
#ifdef S5PC11X_ALIVEGPIO_STORE
	con = __raw_readl(reg);
#endif

	local_irq_restore(flags);

	return 0;
}

int s3c_gpio_set_slewrate(unsigned int pin, unsigned int config)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	void __iomem *reg;
	unsigned long flags;
	int offset;
	u32 con;
	int shift;

	if (!chip)
		return -EINVAL;

	if (config > S3C_GPIO_SLEWRATE_SLOW)
		return -EINVAL;

	reg = chip->base + 0x0c;

	offset = pin - chip->chip.base;
	shift = offset;

	local_irq_save(flags);

	con = __raw_readl(reg);
	con &= ~(1 << shift);
	con |= config << shift;

	__raw_writel(con, reg);
#ifdef S5PC11X_ALIVEGPIO_STORE
	con = __raw_readl(reg);
#endif

	local_irq_restore(flags);

	return 0;
}

__init int s5pv210_gpiolib_init(void)
{
	struct s3c_gpio_chip *chip = s5pv210_gpio_4bit;
	int nr_chips = ARRAY_SIZE(s5pv210_gpio_4bit);
	int i = 0;

	for (i = 0; i < nr_chips; i++, chip++) {
		if (chip->config == NULL)
			chip->config = &gpio_cfg;
		if (chip->base == NULL)
			chip->base = S5PV210_BANK_BASE(i);
	}

	samsung_gpiolib_add_4bit_chips(s5pv210_gpio_4bit, nr_chips);

	return 0;
}
