/* linux/arch/arm/mach-s5pv210/irq-eint-group.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P - IRQ EINT Group support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <plat/regs-irqtype.h>

#include <mach/map.h>
#include <plat/cpu.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/irq-eint-group.h>
#include <mach/regs-gpio.h>

#define S5PV210_EINT_MAX_SOURCES	8

static DEFINE_SPINLOCK(eint_group_lock);

struct s5pv210_eint_group_t {
	int		sources;
	int		base;
	void __iomem	*cont_reg;
	void __iomem	*mask_reg;
	void __iomem	*pend_reg;
	int		mask_ofs;
	int		pend_ofs;
	unsigned int	int_con;
	unsigned int	int_mask;

	/* start offset in control register for each source */
	int		cont_map[S5PV210_EINT_MAX_SOURCES];
};

static struct s5pv210_eint_group_t eint_groups[] = {
	[0] = {
		.sources	= 0,
		.base		= 0,
		.cont_reg	= 0x0,
		.mask_reg	= 0x0,
		.pend_reg	= 0x0,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			-1, -1, -1, -1, -1, -1, -1, -1,
		},
	},
	[1] = {
		.sources	= IRQ_EINT_GROUP1_NR,
		.base		= IRQ_EINT_GROUP1_BASE,
		.cont_reg	= S5PV210_GPA0_INT_CON,
		.mask_reg	= S5PV210_GPA0_INT_MASK,
		.pend_reg	= S5PV210_GPA0_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[2] = {
		.sources	= IRQ_EINT_GROUP2_NR,
		.base		= IRQ_EINT_GROUP2_BASE,
		.cont_reg	= S5PV210_GPA1_INT_CON,
		.mask_reg	= S5PV210_GPA1_INT_MASK,
		.pend_reg	= S5PV210_GPA1_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, -1, -1, -1, -1,
		},
	},
	[3] = {
		.sources	= IRQ_EINT_GROUP3_NR,
		.base		= IRQ_EINT_GROUP3_BASE,
		.cont_reg	= S5PV210_GPB_INT_CON,
		.mask_reg	= S5PV210_GPB_INT_MASK,
		.pend_reg	= S5PV210_GPB_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[4] = {
		.sources	= IRQ_EINT_GROUP4_NR,
		.base		= IRQ_EINT_GROUP4_BASE,
		.cont_reg	= S5PV210_GPC0_INT_CON,
		.mask_reg	= S5PV210_GPC0_INT_MASK,
		.pend_reg	= S5PV210_GPC0_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, -1, -1, -1,
		},
	},
	[5] = {
		.sources	= IRQ_EINT_GROUP5_NR,
		.base		= IRQ_EINT_GROUP5_BASE,
		.cont_reg	= S5PV210_GPC1_INT_CON,
		.mask_reg	= S5PV210_GPC1_INT_MASK,
		.pend_reg	= S5PV210_GPC1_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, -1, -1, -1,
		},
	},
	[6] = {
		.sources	= IRQ_EINT_GROUP6_NR,
		.base		= IRQ_EINT_GROUP6_BASE,
		.cont_reg	= S5PV210_GPD0_INT_CON,
		.mask_reg	= S5PV210_GPD0_INT_MASK,
		.pend_reg	= S5PV210_GPD0_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, -1, -1, -1, -1,
		},
	},
	[7] = {
		.sources	= IRQ_EINT_GROUP7_NR,
		.base		= IRQ_EINT_GROUP7_BASE,
		.cont_reg	= S5PV210_GPD1_INT_CON,
		.mask_reg	= S5PV210_GPD1_INT_MASK,
		.pend_reg	= S5PV210_GPD1_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, -1, -1,
		},
	},
	[8] = {
		.sources	= IRQ_EINT_GROUP8_NR,
		.base		= IRQ_EINT_GROUP8_BASE,
		.cont_reg	= S5PV210_GPE0_INT_CON,
		.mask_reg	= S5PV210_GPE0_INT_MASK,
		.pend_reg	= S5PV210_GPE0_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[9] = {
		.sources	= IRQ_EINT_GROUP9_NR,
		.base		= IRQ_EINT_GROUP9_BASE,
		.cont_reg	= S5PV210_GPE1_INT_CON,
		.mask_reg	= S5PV210_GPE1_INT_MASK,
		.pend_reg	= S5PV210_GPE1_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, -1, -1, -1,
		},
	},
	[10] = {
		.sources	= IRQ_EINT_GROUP10_NR,
		.base		= IRQ_EINT_GROUP10_BASE,
		.cont_reg	= S5PV210_GPF0_INT_CON,
		.mask_reg	= S5PV210_GPF0_INT_MASK,
		.pend_reg	= S5PV210_GPF0_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[11] = {
		.sources	= IRQ_EINT_GROUP11_NR,
		.base		= IRQ_EINT_GROUP11_BASE,
		.cont_reg	= S5PV210_GPF1_INT_CON,
		.mask_reg	= S5PV210_GPF1_INT_MASK,
		.pend_reg	= S5PV210_GPF1_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[12] = {
		.sources	= IRQ_EINT_GROUP12_NR,
		.base		= IRQ_EINT_GROUP12_BASE,
		.cont_reg	= S5PV210_GPF2_INT_CON,
		.mask_reg	= S5PV210_GPF2_INT_MASK,
		.pend_reg	= S5PV210_GPF2_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[13] = {
		.sources	= IRQ_EINT_GROUP13_NR,
		.base		= IRQ_EINT_GROUP13_BASE,
		.cont_reg	= S5PV210_GPF3_INT_CON,
		.mask_reg	= S5PV210_GPF3_INT_MASK,
		.pend_reg	= S5PV210_GPF3_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, -1, -1,
		},
	},
	[14] = {
		.sources	= IRQ_EINT_GROUP14_NR,
		.base		= IRQ_EINT_GROUP14_BASE,
		.cont_reg	= S5PV210_GPG0_INT_CON,
		.mask_reg	= S5PV210_GPG0_INT_MASK,
		.pend_reg	= S5PV210_GPG0_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, -1,
		},
	},
	[15] = {
		.sources	= IRQ_EINT_GROUP15_NR,
		.base		= IRQ_EINT_GROUP15_BASE,
		.cont_reg	= S5PV210_GPG1_INT_CON,
		.mask_reg	= S5PV210_GPG1_INT_MASK,
		.pend_reg	= S5PV210_GPG1_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, -1,
		},
	},
	[16] = {
		.sources	= IRQ_EINT_GROUP16_NR,
		.base		= IRQ_EINT_GROUP16_BASE,
		.cont_reg	= S5PV210_GPG2_INT_CON,
		.mask_reg	= S5PV210_GPG2_INT_MASK,
		.pend_reg	= S5PV210_GPG2_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, -1,
		},
	},
	[17] = {
		.sources	= IRQ_EINT_GROUP17_NR,
		.base		= IRQ_EINT_GROUP17_BASE,
		.cont_reg	= S5PV210_GPG3_INT_CON,
		.mask_reg	= S5PV210_GPG3_INT_MASK,
		.pend_reg	= S5PV210_GPG3_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, -1,
		},
	},
	[18] = {
		.sources	= IRQ_EINT_GROUP18_NR,
		.base		= IRQ_EINT_GROUP18_BASE,
		.cont_reg	= S5PV210_GPJ0_INT_CON,
		.mask_reg	= S5PV210_GPJ0_INT_MASK,
		.pend_reg	= S5PV210_GPJ0_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[19] = {
		.sources	= IRQ_EINT_GROUP19_NR,
		.base		= IRQ_EINT_GROUP19_BASE,
		.cont_reg	= S5PV210_GPJ1_INT_CON,
		.mask_reg	= S5PV210_GPJ1_INT_MASK,
		.pend_reg	= S5PV210_GPJ1_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, -1, -1,
		},
	},
	[20] = {
		.sources	= IRQ_EINT_GROUP20_NR,
		.base		= IRQ_EINT_GROUP20_BASE,
		.cont_reg	= S5PV210_GPJ2_INT_CON,
		.mask_reg	= S5PV210_GPJ2_INT_MASK,
		.pend_reg	= S5PV210_GPJ2_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[21] = {
		.sources	= IRQ_EINT_GROUP21_NR,
		.base		= IRQ_EINT_GROUP21_BASE,
		.cont_reg	= S5PV210_GPJ3_INT_CON,
		.mask_reg	= S5PV210_GPJ3_INT_MASK,
		.pend_reg	= S5PV210_GPJ3_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, 20, 24, 28,
		},
	},
	[22] = {
		.sources	= IRQ_EINT_GROUP22_NR,
		.base		= IRQ_EINT_GROUP22_BASE,
		.cont_reg	= S5PV210_GPJ4_INT_CON,
		.mask_reg	= S5PV210_GPJ4_INT_MASK,
		.pend_reg	= S5PV210_GPJ4_INT_PEND,
		.mask_ofs	= 0,
		.pend_ofs	= 0,
		.int_con	= 0x0,
		.int_mask	= 0xff,
		.cont_map	= {
			0, 4, 8, 12, 16, -1, -1, -1,
		},
	},
};

#define S5PV210_EINT_GROUPS	(sizeof(eint_groups) / sizeof(eint_groups[0]))

static int to_group_number(unsigned int irq)
{
	int grp, found;

	for (grp = 1; grp < S5PV210_EINT_GROUPS; grp++) {
		if (irq >= eint_groups[grp].base + eint_groups[grp].sources)
			continue;
		else {
			found = 1;
			break;
		}
	}

	if (!found) {
		printk(KERN_ERR "failed to find out the eint group number\n");
		grp = 0;
	}

	return grp;
}

static inline int to_irq_number(int grp, unsigned int irq)
{
	return irq - eint_groups[grp].base;
}

static inline int to_bit_offset(int grp, unsigned int irq)
{
	int offset;

	offset = eint_groups[grp].cont_map[to_irq_number(grp, irq)];

	if (offset == -1) {
		printk(KERN_ERR "invalid bit offset\n");
		offset = 0;
	}

	return offset;
}

static inline void s5pv210_irq_eint_group_mask(unsigned int irq)
{
	struct s5pv210_eint_group_t *group;
	unsigned long flags;
	int grp;

	grp = to_group_number(irq);
	group = &eint_groups[grp];
	spin_lock_irqsave(&eint_group_lock, flags);
	eint_groups[grp].int_mask |=
		(1 << (group->mask_ofs + to_irq_number(grp, irq)));

	writel(eint_groups[grp].int_mask, group->mask_reg);
	spin_unlock_irqrestore(&eint_group_lock, flags);
}

static inline void s5pv210_irq_eint_group_ack(unsigned int irq)
{
	struct s5pv210_eint_group_t *group;
	unsigned long flags;
	int grp;
	u32 pend;

	grp = to_group_number(irq);
	group = &eint_groups[grp];

	spin_lock_irqsave(&eint_group_lock, flags);
	pend = (1 << (group->pend_ofs + to_irq_number(grp, irq)));

	writel(pend, group->pend_reg);
	spin_unlock_irqrestore(&eint_group_lock, flags);
}

static void s5pv210_irq_eint_group_unmask(unsigned int irq)
{
	struct s5pv210_eint_group_t *group;
	unsigned long flags;
	int grp;

	grp = to_group_number(irq);
	group = &eint_groups[grp];

	/* for level triggered interrupts, masking doesn't prevent
	 * the interrupt from becoming pending again.  by the time
	 * the handler (either irq or thread) can do its thing to clear
	 * the interrupt, it's too late because it could be pending
	 * already.  we have to ack it here, after the handler runs,
	 * or else we get a false interrupt.
	 */
	if (irq_to_desc(irq)->status & IRQ_LEVEL)
		s5pv210_irq_eint_group_ack(irq);

	spin_lock_irqsave(&eint_group_lock, flags);
	eint_groups[grp].int_mask &=
		~(1 << (group->mask_ofs + to_irq_number(grp, irq)));

	writel(eint_groups[grp].int_mask, group->mask_reg);
	spin_unlock_irqrestore(&eint_group_lock, flags);
}

static void s5pv210_irq_eint_group_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s5pv210_irq_eint_group_mask(irq);
	s5pv210_irq_eint_group_ack(irq);
}

static int s5pv210_irq_eint_group_set_type(unsigned int irq, unsigned int type)
{
	struct s5pv210_eint_group_t *group;
	unsigned long flags;
	int grp, shift;
	u32 mask, newvalue = 0;

	grp = to_group_number(irq);
	group = &eint_groups[grp];

	switch (type) {
	case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No edge setting!\n");
		break;

	case IRQ_TYPE_EDGE_RISING:
		newvalue = S5P_EXTINT_RISEEDGE;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		newvalue = S5P_EXTINT_FALLEDGE;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		newvalue = S5P_EXTINT_BOTHEDGE;
		break;

	case IRQ_TYPE_LEVEL_LOW:
		newvalue = S5P_EXTINT_LOWLEV;
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		newvalue = S5P_EXTINT_HILEV;
		break;

	default:
		printk(KERN_ERR "No such irq type %d", type);
		return -1;
	}

	shift = to_bit_offset(grp, irq);
	mask = 0x7 << shift;

	spin_lock_irqsave(&eint_group_lock, flags);
	eint_groups[grp].int_con &= ~mask;
	eint_groups[grp].int_con |= newvalue << shift;
	writel(eint_groups[grp].int_con, group->cont_reg);
	spin_unlock_irqrestore(&eint_group_lock, flags);

	return 0;
}

static struct irq_chip s5pv210_irq_eint_group = {
	.name		= "s5pv210-eint-group",
	.mask		= s5pv210_irq_eint_group_mask,
	.unmask		= s5pv210_irq_eint_group_unmask,
	.mask_ack	= s5pv210_irq_eint_group_maskack,
	.ack		= s5pv210_irq_eint_group_ack,
	.set_type	= s5pv210_irq_eint_group_set_type,
};

/*
 * s5p_irq_demux_eint_group
*/
static inline void s5pv210_irq_demux_eint_group(unsigned int irq,
					    struct irq_desc *desc)
{
	struct s5pv210_eint_group_t *group;
	u32 status, newirq;
	int grp, src;

	for (grp = 1; grp < S5PV210_EINT_GROUPS; grp++) {
		group = &eint_groups[grp];
		status = __raw_readl(group->pend_reg);

		status &= ~eint_groups[grp].int_mask;
		status >>= group->pend_ofs;
		status &= 0xff;			/* MAX IRQ in a group is 8 */

		if (!status)
			continue;

		for (src = 0; src < S5PV210_EINT_MAX_SOURCES; src++) {
			if (status & 1) {
				newirq = group->base + src;
				generic_handle_irq(newirq);
			}

			status >>= 1;
		}
	}
}

void s5pv210_restore_eint_group(void)
{
	struct s5pv210_eint_group_t *group;
	unsigned long flags;
	int grp;

	spin_lock_irqsave(&eint_group_lock, flags);
	for (grp = 1; grp < S5PV210_EINT_GROUPS; grp++) {
		group = &eint_groups[grp];
		writel(eint_groups[grp].int_con, group->cont_reg);
		writel(eint_groups[grp].int_mask, group->mask_reg);
	}
	spin_unlock_irqrestore(&eint_group_lock, flags);
}

int __init s5pv210_init_irq_eint_group(void)
{
	int irq;

	for (irq = IRQ_EINT_GROUP_BASE; irq < NR_IRQS; irq++) {
		set_irq_chip(irq, &s5pv210_irq_eint_group);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler(IRQ_GPIOINT, s5pv210_irq_demux_eint_group);

	return 0;
}

arch_initcall(s5pv210_init_irq_eint_group);
