/* linux/arch/arm/plat-s3c/pm.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2004-2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C common power management (suspend to ram) support.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-serial.h>
#include <mach/regs-clock.h>
#include <mach/regs-irq.h>
#include <asm/fiq_glue.h>
#include <asm/irq.h>

#include <plat/pm.h>
#include <plat/irq-eint-group.h>
#include <mach/pm-core.h>

/* for external use */

unsigned long s3c_pm_flags;

/* ---------------------------------------------- */
extern unsigned int pm_debug_scratchpad;
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/module.h>

#define PMSTATS_MAGIC "*PM*DEBUG*STATS*"

struct pmstats {
	char magic[16];
	unsigned sleep_count;
	unsigned wake_count;
	unsigned sleep_freq;
	unsigned wake_freq;
};

static struct pmstats *pmstats;
static struct pmstats *pmstats_last;

static ssize_t pmstats_read(struct file *file, char __user *buf,
			    size_t len, loff_t *offset)
{
	if (*offset != 0)
		return 0;
	if (len > 4096)
		len = 4096;

	if (copy_to_user(buf, file->private_data, len))
		return -EFAULT;

	*offset += len;
	return len;
}

static int pmstats_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations pmstats_ops = {
	.owner = THIS_MODULE,
	.read = pmstats_read,
	.open = pmstats_open,
};

void __init pmstats_init(void)
{
	pr_info("pmstats at %08x\n", pm_debug_scratchpad);
	if (pm_debug_scratchpad)
		pmstats = ioremap(pm_debug_scratchpad, 4096);
	else
		pmstats = kzalloc(4096, GFP_ATOMIC);

	if (!memcmp(pmstats->magic, PMSTATS_MAGIC, 16)) {
		pmstats_last = kzalloc(4096, GFP_ATOMIC);
		if (pmstats_last)
			memcpy(pmstats_last, pmstats, 4096);
	}

	memset(pmstats, 0, 4096);
	memcpy(pmstats->magic, PMSTATS_MAGIC, 16);

	debugfs_create_file("pmstats", 0444, NULL, pmstats, &pmstats_ops);
	if (pmstats_last)
		debugfs_create_file("pmstats_last", 0444, NULL, pmstats_last, &pmstats_ops);
}
/* ---------------------------------------------- */

/* Debug code:
 *
 * This code supports debug output to the low level UARTs for use on
 * resume before the console layer is available.
*/

#ifdef CONFIG_SAMSUNG_PM_DEBUG
extern void printascii(const char *);

void s3c_pm_dbg(const char *fmt, ...)
{
	va_list va;
	char buff[256];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	printascii(buff);
}

static inline void s3c_pm_debug_init(void)
{
	/* restart uart clocks so we can use them to output */
	s3c_pm_debug_init_uart();
}

#else
#define s3c_pm_debug_init() do { } while(0)

#endif /* CONFIG_SAMSUNG_PM_DEBUG */

/* Save the UART configurations if we are configured for debug. */

unsigned char pm_uart_udivslot;

#ifdef CONFIG_SAMSUNG_PM_DEBUG

struct pm_uart_save uart_save[CONFIG_SERIAL_SAMSUNG_UARTS];

static void s3c_pm_save_uart(unsigned int uart, struct pm_uart_save *save)
{
	void __iomem *regs = S3C_VA_UARTx(uart);

	save->ulcon = __raw_readl(regs + S3C2410_ULCON);
	save->ucon = __raw_readl(regs + S3C2410_UCON);
	save->ufcon = __raw_readl(regs + S3C2410_UFCON);
	save->umcon = __raw_readl(regs + S3C2410_UMCON);
	save->ubrdiv = __raw_readl(regs + S3C2410_UBRDIV);

	if (pm_uart_udivslot)
		save->udivslot = __raw_readl(regs + S3C2443_DIVSLOT);

	S3C_PMDBG("UART[%d]: ULCON=%04x, UCON=%04x, UFCON=%04x, UBRDIV=%04x\n",
		  uart, save->ulcon, save->ucon, save->ufcon, save->ubrdiv);
}

static void s3c_pm_save_uarts(void)
{
	struct pm_uart_save *save = uart_save;
	unsigned int uart;

	for (uart = 0; uart < CONFIG_SERIAL_SAMSUNG_UARTS; uart++, save++)
		s3c_pm_save_uart(uart, save);
}

static void s3c_pm_restore_uart(unsigned int uart, struct pm_uart_save *save)
{
	void __iomem *regs = S3C_VA_UARTx(uart);

	s3c_pm_arch_update_uart(regs, save);

	__raw_writel(save->ulcon, regs + S3C2410_ULCON);
	__raw_writel(save->ucon,  regs + S3C2410_UCON);
	__raw_writel(save->ufcon, regs + S3C2410_UFCON);
	__raw_writel(save->umcon, regs + S3C2410_UMCON);
	__raw_writel(save->ubrdiv, regs + S3C2410_UBRDIV);

	if (pm_uart_udivslot)
		__raw_writel(save->udivslot, regs + S3C2443_DIVSLOT);
}

static void s3c_pm_restore_uarts(void)
{
	struct pm_uart_save *save = uart_save;
	unsigned int uart;

	for (uart = 0; uart < CONFIG_SERIAL_SAMSUNG_UARTS; uart++, save++)
		s3c_pm_restore_uart(uart, save);
}
#else
static void s3c_pm_save_uarts(void) { }
static void s3c_pm_restore_uarts(void) { }
#endif

/* The IRQ ext-int code goes here, it is too small to currently bother
 * with its own file. */

unsigned long s3c_irqwake_intmask	= 0xffffffffL;
unsigned long s3c_irqwake_eintmask	= 0xffffffffL;

int s3c_irqext_wake(unsigned int irqno, unsigned int state)
{
	unsigned long bit = 1L << IRQ_EINT_BIT(irqno);

	if (!(s3c_irqwake_eintallow & bit))
		return -ENOENT;

	printk(KERN_INFO "wake %s for irq %d\n",
	       state ? "enabled" : "disabled", irqno);

	if (!state)
		s3c_irqwake_eintmask |= bit;
	else
		s3c_irqwake_eintmask &= ~bit;

	return 0;
}

/* helper functions to save and restore register state */

/**
 * s3c_pm_do_save() - save a set of registers for restoration on resume.
 * @ptr: Pointer to an array of registers.
 * @count: Size of the ptr array.
 *
 * Run through the list of registers given, saving their contents in the
 * array for later restoration when we wakeup.
 */
void s3c_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		S3C_PMDBG("saved %p value %08lx\n", ptr->reg, ptr->val);
	}
}

/**
 * s3c_pm_do_restore() - restore register values from the save list.
 * @ptr: Pointer to an array of registers.
 * @count: Size of the ptr array.
 *
 * Restore the register values saved from s3c_pm_do_save().
 *
 * Note, we do not use S3C_PMDBG() in here, as the system may not have
 * restore the UARTs state yet
*/

void s3c_pm_do_restore(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++)
		__raw_writel(ptr->val, ptr->reg);
}

/**
 * s3c_pm_do_restore_core() - early restore register values from save list.
 *
 * This is similar to s3c_pm_do_restore() except we try and minimise the
 * side effects of the function in case registers that hardware might need
 * to work has been restored.
 *
 * WARNING: Do not put any debug in here that may effect memory or use
 * peripherals, as things may be changing!
*/

void s3c_pm_do_restore_core(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++)
		__raw_writel(ptr->val, ptr->reg);
}

/* s3c2410_pm_show_resume_irqs
 *
 * print any IRQs asserted at resume time (ie, we woke from)
*/
static void s3c_pm_show_resume_irqs(int start, unsigned long which,
				    unsigned long mask)
{
	int i;

	which &= ~mask;

	for (i = 0; i <= 31; i++) {
		if (which & (1L<<i)) {
			S3C_PMDBG("IRQ %d asserted at resume\n", start+i);
		}
	}
}


void (*pm_cpu_prep)(void);
void (*pm_cpu_sleep)(void);
void (*pm_cpu_restore)(void);

#define any_allowed(mask, allow) (((mask) & (allow)) != (allow))

/* s3c_pm_enter
 *
 * central control for sleep/resume process
*/

static int s3c_pm_enter(suspend_state_t state)
{
	static unsigned long regs_save[16];

	/* ensure the debug is initialised (if enabled) */

	s3c_pm_debug_init();

	S3C_PMDBG("%s(%d)\n", __func__, state);

	if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
		printk(KERN_ERR "%s: error: no cpu sleep function\n", __func__);
		return -EINVAL;
	}

	/* check if we have anything to wake-up with... bad things seem
	 * to happen if you suspend with no wakeup (system will often
	 * require a full power-cycle)
	*/

	if (!any_allowed(s3c_irqwake_intmask, s3c_irqwake_intallow) &&
	    !any_allowed(s3c_irqwake_eintmask, s3c_irqwake_eintallow)) {
		printk(KERN_ERR "%s: No wake-up sources!\n", __func__);
		printk(KERN_ERR "%s: Aborting sleep\n", __func__);
		return -EINVAL;
	}

	/* store the physical address of the register recovery block */

	s3c_sleep_save_phys = virt_to_phys(regs_save);

	S3C_PMDBG("s3c_sleep_save_phys=0x%08lx\n", s3c_sleep_save_phys);

	/* save all necessary core registers not covered by the drivers */

	s3c_pm_save_gpios();
	s3c_pm_save_uarts();
	s3c_pm_save_core();


	/* set the irq configuration for wake */

	s3c_pm_configure_extint();

	S3C_PMDBG("sleep: irq wakeup masks: %08lx,%08lx\n",
	    s3c_irqwake_intmask, s3c_irqwake_eintmask);

	s3c_pm_arch_prepare_irqs();

	/* call cpu specific preparation */

	pm_cpu_prep();

	/* flush cache back to ram */

	flush_cache_all();

	s3c_pm_check_store();

	/* clear wakeup_stat register for next wakeup reason */
	__raw_writel(__raw_readl(S5P_WAKEUP_STAT), S5P_WAKEUP_STAT);

	/* send the cpu to sleep... */

	s3c_pm_arch_stop_clocks();

	/* s3c_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state and restores it
	 * during the resume.  */

	pmstats->sleep_count++;
	pmstats->sleep_freq = __raw_readl(S5P_CLK_DIV0);
	s3c_cpu_save(regs_save);
	pmstats->wake_count++;
	pmstats->wake_freq = __raw_readl(S5P_CLK_DIV0);

	/* restore the cpu state using the kernel's cpu init code. */

	cpu_init();

	fiq_glue_resume();
	local_fiq_enable();

	s3c_pm_restore_core();
	s3c_pm_restore_uarts();
	s3c_pm_restore_gpios();
	s5pv210_restore_eint_group();

	s3c_pm_debug_init();

        /* restore the system state */

	if (pm_cpu_restore)
		pm_cpu_restore();

	/* check what irq (if any) restored the system */

	s3c_pm_arch_show_resume_irqs();

	S3C_PMDBG("%s: post sleep, preparing to return\n", __func__);

	/* LEDs should now be 1110 */
	s3c_pm_debug_smdkled(1 << 1, 0);

	s3c_pm_check_restore();

	/* ok, let's return from sleep */

	S3C_PMDBG("S3C PM Resume (post-restore)\n");
	return 0;
}

/* callback from assembly code */
void s3c_pm_cb_flushcache(void)
{
	flush_cache_all();
}

static int s3c_pm_prepare(void)
{
	/* prepare check area if configured */

	s3c_pm_check_prepare();
	return 0;
}

static void s3c_pm_finish(void)
{
	s3c_pm_check_cleanup();
}

static struct platform_suspend_ops s3c_pm_ops = {
	.enter		= s3c_pm_enter,
	.prepare	= s3c_pm_prepare,
	.finish		= s3c_pm_finish,
	.valid		= suspend_valid_only_mem,
};

/* s3c_pm_init
 *
 * Attach the power management functions. This should be called
 * from the board specific initialisation if the board supports
 * it.
*/

int __init s3c_pm_init(void)
{
	printk("S3C Power Management, Copyright 2004 Simtec Electronics\n");
	pmstats_init();

	suspend_set_ops(&s3c_pm_ops);
	return 0;
}
