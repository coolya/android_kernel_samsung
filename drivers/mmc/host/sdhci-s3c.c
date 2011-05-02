/* linux/drivers/mmc/host/sdhci-s3c.c
 *
 * Copyright 2008 Openmoko Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * SDHCI (HSMMC) support for Samsung SoC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>

#include <plat/sdhci.h>
#include <plat/regs-sdhci.h>

#include "sdhci.h"

#define MAX_BUS_CLK	(4)

/**
 * struct sdhci_s3c - S3C SDHCI instance
 * @host: The SDHCI host created
 * @pdev: The platform device we where created from.
 * @ioarea: The resource created when we claimed the IO area.
 * @pdata: The platform data for this controller.
 * @cur_clk: The index of the current bus clock.
 * @clk_io: The clock for the internal bus interface.
 * @clk_bus: The clocks that are available for the SD/MMC bus clock.
 */
struct sdhci_s3c {
	struct sdhci_host	*host;
	struct platform_device	*pdev;
	struct resource		*ioarea;
	struct s3c_sdhci_platdata *pdata;
	unsigned int		cur_clk;

	struct clk		*clk_io;
	struct clk		*clk_bus[MAX_BUS_CLK];
};

static inline struct sdhci_s3c *to_s3c(struct sdhci_host *host)
{
	return sdhci_priv(host);
}

/**
 * get_curclk - convert ctrl2 register to clock source number
 * @ctrl2: Control2 register value.
 */
static u32 get_curclk(u32 ctrl2)
{
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 >>= S3C_SDHCI_CTRL2_SELBASECLK_SHIFT;

	return ctrl2;
}

static void sdhci_s3c_check_sclk(struct sdhci_host *host)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	u32 tmp = readl(host->ioaddr + S3C_SDHCI_CONTROL2);

	if (get_curclk(tmp) != ourhost->cur_clk) {
		dev_dbg(&ourhost->pdev->dev, "restored ctrl2 clock setting\n");

		tmp &= ~S3C_SDHCI_CTRL2_SELBASECLK_MASK;
		tmp |= ourhost->cur_clk << S3C_SDHCI_CTRL2_SELBASECLK_SHIFT;
		writel(tmp, host->ioaddr + S3C_SDHCI_CONTROL2);
	}
}

/**
 * sdhci_s3c_get_max_clk - callback to get maximum clock frequency.
 * @host: The SDHCI host instance.
 *
 * Callback to return the maximum clock rate acheivable by the controller.
*/
static unsigned int sdhci_s3c_get_max_clk(struct sdhci_host *host)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	struct clk *busclk;
	unsigned int rate, max;
	int clk;

	/* note, a reset will reset the clock source */

	sdhci_s3c_check_sclk(host);

	for (max = 0, clk = 0; clk < MAX_BUS_CLK; clk++) {
		busclk = ourhost->clk_bus[clk];
		if (!busclk)
			continue;

		rate = clk_get_rate(busclk);
		if (rate > max)
			max = rate;
	}

	return max;
}

static unsigned int sdhci_s3c_get_timeout_clk(struct sdhci_host *host)
{
	return sdhci_s3c_get_max_clk(host) / 1000000;
}

static void sdhci_s3c_set_ios(struct sdhci_host *host,
			      struct mmc_ios *ios)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	struct s3c_sdhci_platdata *pdata = ourhost->pdata;
	int width;
	u8 tmp;

	sdhci_s3c_check_sclk(host);

	if (ios->power_mode != MMC_POWER_OFF) {
		switch (ios->bus_width) {
		case MMC_BUS_WIDTH_8:
			width = 8;
			tmp = readb(host->ioaddr + SDHCI_HOST_CONTROL);
			writeb(tmp | SDHCI_S3C_CTRL_8BITBUS,
				host->ioaddr + SDHCI_HOST_CONTROL);
			break;
		case MMC_BUS_WIDTH_4:
			width = 4;
			break;
		case MMC_BUS_WIDTH_1:
			width = 1;
			break;
		default:
			BUG();
		}

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(ourhost->pdev, width);
	}

	if (pdata->cfg_card) {
		pdata->cfg_card(ourhost->pdev, host->ioaddr,
				ios, host->mmc->card);
		pdata->rx_cfg = 0;
		pdata->tx_cfg = 0;
	}
}

/**
 * sdhci_s3c_consider_clock - consider one the bus clocks for current setting
 * @ourhost: Our SDHCI instance.
 * @src: The source clock index.
 * @wanted: The clock frequency wanted.
 */
static unsigned int sdhci_s3c_consider_clock(struct sdhci_s3c *ourhost,
					     unsigned int src,
					     unsigned int wanted)
{
	unsigned long rate;
	struct clk *clksrc = ourhost->clk_bus[src];
	int div;

	if (!clksrc)
		return UINT_MAX;

	rate = clk_get_rate(clksrc);

	for (div = 1; div < 256; div *= 2) {
		if ((rate / div) <= wanted)
			break;
	}

	dev_dbg(&ourhost->pdev->dev, "clk %d: rate %ld, want %d, got %ld\n",
		src, rate, wanted, rate / div);

	return (wanted - (rate / div));
}

/**
 * sdhci_s3c_set_clock - callback on clock change
 * @host: The SDHCI host being changed
 * @clock: The clock rate being requested.
 *
 * When the card's clock is going to be changed, look at the new frequency
 * and find the best clock source to go with it.
*/
static void sdhci_s3c_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	unsigned int best = UINT_MAX;
	unsigned int delta;
	int best_src = 0;
	int src;
	u32 ctrl;

	/* don't bother if the clock is going off. */
	if (clock == 0)
		return;

	for (src = 0; src < MAX_BUS_CLK; src++) {
		delta = sdhci_s3c_consider_clock(ourhost, src, clock);
		if (delta < best) {
			best = delta;
			best_src = src;
		}
	}

	dev_dbg(&ourhost->pdev->dev,
		"selected source %d, clock %d, delta %d\n",
		 best_src, clock, best);

	/* select the new clock source */

	if (ourhost->cur_clk != best_src) {
		struct clk *clk = ourhost->clk_bus[best_src];

		/* turn clock off to card before changing clock source */
		writew(0, host->ioaddr + SDHCI_CLOCK_CONTROL);

		ourhost->cur_clk = best_src;
		host->max_clk = clk_get_rate(clk);
		host->timeout_clk = sdhci_s3c_get_timeout_clk(host);

		ctrl = readl(host->ioaddr + S3C_SDHCI_CONTROL2);
		ctrl &= ~S3C_SDHCI_CTRL2_SELBASECLK_MASK;
		ctrl |= best_src << S3C_SDHCI_CTRL2_SELBASECLK_SHIFT;
		writel(ctrl, host->ioaddr + S3C_SDHCI_CONTROL2);
	}
}

static int sdhci_s3c_get_ro(struct mmc_host *mmc)
{
	struct sdhci_host *host;
	struct sdhci_s3c *sc;

	host = mmc_priv(mmc);
	sc = sdhci_priv(host);

	if(sc->pdata->get_ro)
		return sc->pdata->get_ro(mmc);

	return 0;
}

static int sdhci_s3c_get_cd(struct sdhci_host *host)
{
	unsigned int detect = -ENOSYS;
	struct sdhci_s3c* sc = sdhci_priv(host);

	if(sc->pdata->detect_ext_cd)
		detect = sc->pdata->detect_ext_cd();

	return detect;
}

static void sdhci_s3c_adjust_cfg(struct sdhci_host *host, int rw)
{
	struct sdhci_s3c *ourhost = to_s3c(host);
	struct s3c_sdhci_platdata *pdata = ourhost->pdata;

	if(pdata->adjust_cfg_card)
		pdata->adjust_cfg_card(pdata, host->ioaddr, rw);
}

static struct sdhci_ops sdhci_s3c_ops = {
	.get_max_clock		= sdhci_s3c_get_max_clk,
	.get_timeout_clock	= sdhci_s3c_get_timeout_clk,
	.set_clock		= sdhci_s3c_set_clock,
	.set_ios		= sdhci_s3c_set_ios,
	.get_cd			= sdhci_s3c_get_cd,
	.adjust_cfg		= sdhci_s3c_adjust_cfg,
};

/*
 * call this when you need sd stack to recognize insertion or removal of card
 * that can't be told by SDHCI regs
 */
void sdhci_s3c_force_presence_change(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);

	printk(KERN_DEBUG "%s : Enter\n",__FUNCTION__);
	mmc_detect_change(host->mmc, msecs_to_jiffies(60));
}
EXPORT_SYMBOL_GPL(sdhci_s3c_force_presence_change);

irqreturn_t sdhci_irq_cd(int irq, void *dev_id)
{
	struct sdhci_s3c* sc = dev_id;
	uint detect;

	printk(KERN_DEBUG "sdhci: card interrupt.\n");

	detect = sc->pdata->detect_ext_cd();

	if (detect) {
		printk(KERN_DEBUG "sdhci: card inserted.\n");
		sc->host->flags |= SDHCI_DEVICE_ALIVE;
	} else {
		printk(KERN_DEBUG "sdhci: card removed.\n");
		sc->host->flags &= ~SDHCI_DEVICE_ALIVE;
	}
	tasklet_schedule(&sc->host->card_tasklet);

	return IRQ_HANDLED;
}

static int __devinit sdhci_s3c_probe(struct platform_device *pdev)
{
	struct s3c_sdhci_platdata *pdata = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	struct sdhci_host *host;
	struct sdhci_s3c *sc;
	struct resource *res;
	int ret, irq, ptr, clks;

	if (!pdata) {
		dev_err(dev, "no device data specified\n");
		return -ENOENT;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "no irq specified\n");
		return irq;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "no memory specified\n");
		return -ENOENT;
	}

	host = sdhci_alloc_host(dev, sizeof(struct sdhci_s3c));
	if (IS_ERR(host)) {
		dev_err(dev, "sdhci_alloc_host() failed\n");
		return PTR_ERR(host);
	}

	sc = sdhci_priv(host);

	sc->host = host;
	sc->pdev = pdev;
	sc->pdata = pdata;

	platform_set_drvdata(pdev, host);

	sc->clk_io = clk_get(dev, "hsmmc");
	if (IS_ERR(sc->clk_io)) {
		dev_err(dev, "failed to get io clock\n");
		ret = PTR_ERR(sc->clk_io);
		goto err_io_clk;
	}

	/* enable the local io clock and keep it running for the moment. */
	clk_enable(sc->clk_io);

	for (clks = 0, ptr = 0; ptr < MAX_BUS_CLK; ptr++) {
		struct clk *clk;
		char *name = pdata->clocks[ptr];

		if (name == NULL)
			continue;

		clk = clk_get(dev, name);
		if (IS_ERR(clk)) {
			dev_err(dev, "failed to get clock %s\n", name);
			continue;
		}

		clks++;
		sc->clk_bus[ptr] = clk;
		clk_enable(clk);

		dev_info(dev, "clock source %d: %s (%ld Hz)\n",
			 ptr, name, clk_get_rate(clk));
	}

	if (clks == 0) {
		dev_err(dev, "failed to find any bus clocks\n");
		ret = -ENOENT;
		goto err_no_busclks;
	}

	sc->ioarea = request_mem_region(res->start, resource_size(res),
					mmc_hostname(host->mmc));
	if (!sc->ioarea) {
		dev_err(dev, "failed to reserve register area\n");
		ret = -ENXIO;
		goto err_req_regs;
	}

	host->ioaddr = ioremap_nocache(res->start, resource_size(res));
	if (!host->ioaddr) {
		dev_err(dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_req_regs;
	}

	/* Ensure we have minimal gpio selected CMD/CLK/Detect */
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev, pdata->max_width);

	if (pdata->get_ro)
		sdhci_s3c_ops.get_ro = sdhci_s3c_get_ro;

	host->hw_name = "samsung-hsmmc";
	host->ops = &sdhci_s3c_ops;
	host->quirks = 0;
	host->irq = irq;

	/* Setup quirks for the controller */
	host->quirks |= SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC;
	host->quirks |= SDHCI_QUIRK_BROKEN_CARD_PRESENT_BIT;
	host->quirks |= SDHCI_QUIRK_BROKEN_TIMEOUT_VAL;
	if (pdata->must_maintain_clock)
		host->quirks |= SDHCI_QUIRK_MUST_MAINTAIN_CLOCK;

#ifndef CONFIG_MMC_SDHCI_S3C_DMA

	/* we currently see overruns on errors, so disable the SDMA
	 * support as well. */
	host->quirks |= SDHCI_QUIRK_BROKEN_DMA;

	/* PIO currently has problems with multi-block IO */
	host->quirks |= SDHCI_QUIRK_NO_MULTIBLOCK;

#endif /* CONFIG_MMC_SDHCI_S3C_DMA */

	/* It seems we do not get an DATA transfer complete on non-busy
	 * transfers, not sure if this is a problem with this specific
	 * SDHCI block, or a missing configuration that needs to be set. */
	host->quirks |= SDHCI_QUIRK_NO_BUSY_IRQ;

	host->quirks |= (SDHCI_QUIRK_32BIT_DMA_ADDR |
			 SDHCI_QUIRK_32BIT_DMA_SIZE);

	host->quirks |= SDHCI_QUIRK_NO_HISPD_BIT;

	if (pdata->host_caps)
		host->mmc->caps = pdata->host_caps;
	else
		host->mmc->caps = 0;

	/* Set pm_flags for built_in device */
	host->mmc->pm_caps = MMC_PM_KEEP_POWER | MMC_PM_IGNORE_PM_NOTIFY;
	if (pdata->built_in)
		host->mmc->pm_flags = MMC_PM_KEEP_POWER | MMC_PM_IGNORE_PM_NOTIFY;

	/* to add external irq as a card detect signal */
	if (pdata->cfg_ext_cd) {
		pdata->cfg_ext_cd();

		if (pdata->detect_ext_cd())
			host->flags |= SDHCI_DEVICE_ALIVE;
	}

	/* to configure gpio pin as a card write protection signal */
	if (pdata->cfg_wp)
		pdata->cfg_wp();

	ret = sdhci_add_host(host);
	if (ret) {
		dev_err(dev, "sdhci_add_host() failed\n");
		goto err_add_host;
	}

	/* register external irq here (after all init is done) */
	if (pdata->cfg_ext_cd) {
		ret = request_irq(pdata->ext_cd, sdhci_irq_cd,
				IRQF_SHARED, mmc_hostname(host->mmc), sc);
		if(ret)
			goto err_add_host;
	}

	return 0;

 err_add_host:
	release_resource(sc->ioarea);
	kfree(sc->ioarea);

 err_req_regs:
	for (ptr = 0; ptr < MAX_BUS_CLK; ptr++) {
		if (sc->clk_bus[ptr]) {
			clk_disable(sc->clk_bus[ptr]);
			clk_put(sc->clk_bus[ptr]);
		}
	}

 err_no_busclks:
	clk_disable(sc->clk_io);
	clk_put(sc->clk_io);

 err_io_clk:
	sdhci_free_host(host);

	return ret;
}

static int __devexit sdhci_s3c_remove(struct platform_device *pdev)
{
	struct sdhci_host *host =  platform_get_drvdata(pdev);
	struct sdhci_s3c *sc = sdhci_priv(host);
	int ptr, dead = 0;
	u32 scratch;

	scratch = readl(host->ioaddr + SDHCI_INT_STATUS);
	if (scratch == (u32)-1)
		dead = 1;

	if(sc->pdata && sc->pdata->cfg_ext_cd)
		free_irq(sc->pdata->ext_cd, sc);

	sdhci_remove_host(host, dead);

	for (ptr = 0; ptr < MAX_BUS_CLK; ptr++) {
		if (sc->clk_bus[ptr]) {
			clk_disable(sc->clk_bus[ptr]);
			clk_put(sc->clk_bus[ptr]);
		}
	}
	clk_disable(sc->clk_io);
	clk_put(sc->clk_io);

	iounmap(host->ioaddr);
	release_resource(sc->ioarea);
	kfree(sc->ioarea);

	sdhci_free_host(host);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM

static int sdhci_s3c_suspend(struct platform_device *dev, pm_message_t pm)
{
	struct sdhci_host *host = platform_get_drvdata(dev);
	struct s3c_sdhci_platdata *pdata = dev->dev.platform_data;

	struct mmc_host *mmc = host->mmc;

	if (mmc->card && (mmc->card->type == MMC_TYPE_SDIO))
		mmc->pm_flags |= MMC_PM_KEEP_POWER;

	sdhci_suspend_host(host, pm);

	if(pdata && pdata->cfg_ext_cd){
		free_irq(pdata->ext_cd, sdhci_priv(host));
	}
	return 0;
}

static int sdhci_s3c_resume(struct platform_device *dev)
{
	struct sdhci_host *host = platform_get_drvdata(dev);
	struct s3c_sdhci_platdata *pdata = dev->dev.platform_data;
	int ret;
	u32 ier;

	sdhci_resume_host(host);

	if (pdata->enable_intr_on_resume) {
		ier = sdhci_readl(host, SDHCI_INT_ENABLE);
		ier |= SDHCI_INT_CARD_INT;
		sdhci_writel(host, ier, SDHCI_INT_ENABLE);
		sdhci_writel(host, ier, SDHCI_SIGNAL_ENABLE);
	}

	if(pdata && pdata->cfg_ext_cd){
		ret = request_irq(pdata->ext_cd, sdhci_irq_cd, IRQF_SHARED, mmc_hostname(host->mmc), sdhci_priv(host));
		if(ret)
			return ret;
	}

	return 0;
}

#else
#define sdhci_s3c_suspend NULL
#define sdhci_s3c_resume NULL
#endif

static struct platform_driver sdhci_s3c_driver = {
	.probe		= sdhci_s3c_probe,
	.remove		= __devexit_p(sdhci_s3c_remove),
	.suspend	= sdhci_s3c_suspend,
	.resume	        = sdhci_s3c_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-sdhci",
	},
};

static int __init sdhci_s3c_init(void)
{
	return platform_driver_register(&sdhci_s3c_driver);
}

static void __exit sdhci_s3c_exit(void)
{
	platform_driver_unregister(&sdhci_s3c_driver);
}

module_init(sdhci_s3c_init);
module_exit(sdhci_s3c_exit);

MODULE_DESCRIPTION("Samsung SDHCI (HSMMC) glue");
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:s3c-sdhci");
