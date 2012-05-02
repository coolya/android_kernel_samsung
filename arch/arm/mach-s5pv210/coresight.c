/* coresight.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/hardware/coresight.h>
#include <linux/amba/bus.h>
#include <linux/io.h>
#include <plat/pm.h>

#define CS_ETB_BASE	(0xE0D00000 + 0x1000)
#define CS_FUNNEL_BASE	(0xE0D00000 + 0x4000)
#define CS_ETM_BASE	(0xE0D00000 + 0x6000)
#define TZPC3_BASE	(0xE1C00000)

static void __iomem *cs_etb_regs;
static void __iomem *cs_funnel_regs;
static void __iomem *cs_etm_regs;
static void __iomem *tzpc3_regs;

static void cs_unlock(void *regs)
{
	writel(UNLOCK_MAGIC, regs + CSMR_LOCKACCESS);
	readl(regs + CSMR_LOCKSTATUS);
}

static void cs_relock(void *regs)
{
	writel(0, regs + CSMR_LOCKACCESS);
}

static void cs_init_static_regs(void)
{
	/* Enable NIDEN and SPNIDEN */
	writel(0x5, tzpc3_regs + 0x828);

	/* Set funnel control: Enable ATB port0 */
	cs_unlock(cs_funnel_regs);
	writel(0x301, cs_funnel_regs);
	cs_relock(cs_funnel_regs);
}

#ifdef CONFIG_PM
static void (*orig_pm_cpu_prep)(void);
static void (*orig_pm_cpu_restore)(void);

static u32 etm_state[64];
static int etm_state_count;
static u32 etb_ffcr;
static u32 etb_ctl;

static void cs_pm_prep(void)
{
	int save_count;
	orig_pm_cpu_prep();

	cs_unlock(cs_etb_regs);
	etb_ctl = readl(cs_etb_regs + ETBR_CTRL);
	etb_ffcr = readl(cs_etb_regs + ETBR_FORMATTERCTRL);
	cs_relock(cs_etb_regs);

	cs_unlock(cs_etm_regs);
	writel(UNLOCK_MAGIC, cs_etm_regs + ETMMR_OSLAR);
	save_count = readl(cs_etm_regs + ETMMR_OSSRR);
	pr_debug("%s: trace_pm_prep reg count %d\n", __func__, save_count);
	if (save_count > ARRAY_SIZE(etm_state))
		save_count = 0;
	etm_state_count = 0;
	while (save_count--)
		etm_state[etm_state_count++] = readl(cs_etm_regs + ETMMR_OSSRR);
	writel(0, cs_etm_regs + ETMMR_OSLAR);
	cs_relock(cs_etm_regs);
}

static void cs_pm_resume(void)
{
	int i;

	cs_init_static_regs();

	cs_unlock(cs_etb_regs);
	writel(etb_ffcr, cs_etb_regs + ETBR_FORMATTERCTRL);
	writel(etb_ctl, cs_etb_regs + ETBR_CTRL);
	cs_relock(cs_etb_regs);

	cs_unlock(cs_etm_regs);
	readl(cs_etm_regs + ETMMR_PDSR);
	writel(UNLOCK_MAGIC, cs_etm_regs + ETMMR_OSLAR);
	readl(cs_etm_regs + ETMMR_OSSRR);
	for (i = 0; i < etm_state_count; i++) {
		writel(etm_state[i], cs_etm_regs + ETMMR_OSSRR);
		pr_debug("%s: restore %d %08x\n", __func__, i, etm_state[i]);
	}
	writel(0, cs_etm_regs + ETMMR_OSLAR);
	cs_relock(cs_etm_regs);

	orig_pm_cpu_restore();
}

static void cs_pm_init(void)
{
	orig_pm_cpu_restore = pm_cpu_restore;
	orig_pm_cpu_prep = pm_cpu_prep;
	pm_cpu_restore = cs_pm_resume;
	pm_cpu_prep = cs_pm_prep;
}
#else
static inline void cs_pm_init(void) {}
#endif

static struct amba_device s5pv210_etb_device = {
	.dev		= {
		.init_name = "etb",
	},
	.res		= {
		.start	= CS_ETB_BASE,
		.end	= CS_ETB_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= 0x000bb907,
};

static struct amba_device s5pv210_etm_device = {
	.dev		= {
		.init_name = "etm",
	},
	.res		= {
		.start	= CS_ETM_BASE,
		.end	= CS_ETM_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	.periphid	= 0x005bb921,
};

static int trace_init(void)
{
	cs_etb_regs = ioremap(CS_ETB_BASE, SZ_4K);
	cs_funnel_regs = ioremap(CS_FUNNEL_BASE, SZ_4K);
	cs_etm_regs = ioremap(CS_ETM_BASE, SZ_4K);
	tzpc3_regs = ioremap(TZPC3_BASE, SZ_4K);

	cs_init_static_regs();

	cs_pm_init();

	amba_device_register(&s5pv210_etb_device, &iomem_resource);
	amba_device_register(&s5pv210_etm_device, &iomem_resource);
	return 0;
}
device_initcall(trace_init);
