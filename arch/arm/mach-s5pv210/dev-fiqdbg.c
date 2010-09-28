/* linux/arch/arm/mach-s5pv210/dev-fiqdbg.c
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/irq.h>
#include <linux/sysdev.h>

#include <asm/hardware/vic.h>
#include <asm/fiq_debugger.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <plat/regs-serial.h>

static void *s5pv210_fiqdbg_get_base(struct platform_device *pdev)
{
	return S5P_VA_UART0 + S3C_UART_OFFSET * pdev->id;
}

static unsigned int s5pv210_fiqdbg_tx_empty(void *base)
{
	unsigned long ufstat = readl(base + S3C2410_UFSTAT);

	return !(ufstat & (S5PV210_UFSTAT_TXMASK | S5PV210_UFSTAT_TXFULL));
}

static void s5pv210_fiqdbg_wait_for_tx_empty(void *base)
{
	unsigned int tmout = 10000;

	while (true) {
		if (s5pv210_fiqdbg_tx_empty(base))
			return;
		if (--tmout == 0)
			break;
		udelay(1);
	}
}

static int s5pv210_fiqdbg_uart_getc(struct platform_device *pdev)
{
	void *base = s5pv210_fiqdbg_get_base(pdev);
	unsigned int ufstat;

	if (readl(base + S3C2410_UERSTAT) & S3C2410_UERSTAT_BREAK)
		return FIQ_DEBUGGER_BREAK;

	ufstat = readl(base + S3C2410_UFSTAT);
	if (!(ufstat & (S5PV210_UFSTAT_RXMASK | S5PV210_UFSTAT_RXFULL)))
		return FIQ_DEBUGGER_NO_CHAR;
	return readb(base + S3C2410_URXH);
}

static void s5pv210_fiqdbg_uart_putc(struct platform_device *pdev,
					unsigned int c)
{
	void *base = s5pv210_fiqdbg_get_base(pdev);
	s5pv210_fiqdbg_wait_for_tx_empty(base);
	writeb(c, base + S3C2410_UTXH);
	if (c == 10) {
		s5pv210_fiqdbg_wait_for_tx_empty(base);
		writeb(13, base + S3C2410_UTXH);
	}
	s5pv210_fiqdbg_wait_for_tx_empty(base);
}

static void fiq_enable(struct platform_device *pdev,
			unsigned int fiq, bool enabled)
{
	struct irq_chip *chip = get_irq_chip(fiq);

	vic_set_fiq(fiq, enabled);
	if (enabled)
		chip->unmask(fiq);
	else
		chip->mask(fiq);
}

static void fiq_ack(struct platform_device *pdev, unsigned int fiq)
{
	void *base = s5pv210_fiqdbg_get_base(pdev);
	writel(0x3, base + S5P_UINTP);
}

static int s5pv210_fiqdbg_uart_init(struct platform_device *pdev)
{
	void *base = s5pv210_fiqdbg_get_base(pdev);

	writel(S3C2410_UCON_TXILEVEL |
		S3C2410_UCON_RXILEVEL |
		S3C2410_UCON_TXIRQMODE |
		S3C2410_UCON_RXIRQMODE |
		S3C2410_UCON_RXFIFO_TOI |
		S3C2443_UCON_RXERR_IRQEN, base + S3C2410_UCON);
	writel(S3C2410_LCON_CS8, base + S3C2410_ULCON);
	writel(S3C2410_UFCON_FIFOMODE |
		S5PV210_UFCON_TXTRIG4 |
		S5PV210_UFCON_RXTRIG4 |
		S3C2410_UFCON_RESETRX, base + S3C2410_UFCON);
	writel(0, base + S3C2410_UMCON);
	/* 115200 */
	writel(35, base + S3C2410_UBRDIV);
	writel(0x808, base + S3C2443_DIVSLOT);
	writel(0xc, base + S5P_UINTM);
	writel(0xf, base + S5P_UINTP);

	return 0;
}

static struct fiq_debugger_pdata s5pv210_fiqdbg_pdata = {
	.uart_init = s5pv210_fiqdbg_uart_init,
	.uart_resume = s5pv210_fiqdbg_uart_init,
	.uart_getc = s5pv210_fiqdbg_uart_getc,
	.uart_putc = s5pv210_fiqdbg_uart_putc,
	.fiq_enable = fiq_enable,
	.fiq_ack = fiq_ack,
};

#define DEFINE_FIQDBG_UART(uart) \
static struct resource s5pv210_fiqdbg_uart##uart##_resource[] = { \
	{ \
		.start = IRQ_UART##uart, \
		.end   = IRQ_UART##uart, \
		.name  = "fiq", \
		.flags = IORESOURCE_IRQ, \
	}, \
	{ \
		.start = IRQ_VIC_END-uart, \
		.end   = IRQ_VIC_END-uart, \
		.name  = "signal", \
		.flags = IORESOURCE_IRQ, \
	}, \
}; \
struct platform_device s5pv210_device_fiqdbg_uart##uart = { \
	.name		  = "fiq_debugger", \
	.id		  = uart, \
	.num_resources	  = ARRAY_SIZE(s5pv210_fiqdbg_uart##uart##_resource), \
	.resource	  = s5pv210_fiqdbg_uart##uart##_resource, \
	.dev = { \
		.platform_data = &s5pv210_fiqdbg_pdata, \
	}, \
}

DEFINE_FIQDBG_UART(0);
DEFINE_FIQDBG_UART(1);
DEFINE_FIQDBG_UART(2);
DEFINE_FIQDBG_UART(3);
