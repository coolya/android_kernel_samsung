/* linux/arch/arm/plat-s5p/bootmem.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Bootmem helper functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>
#include <asm/setup.h>
#include <linux/io.h>
#include <mach/memory.h>
#include <plat/media.h>
#include <mach/media.h>

static struct s5p_media_device *media_devs;
static int nr_media_devs;

static struct s5p_media_device *s5p_get_media_device(int dev_id, int bank)
{
	struct s5p_media_device *mdev = NULL;
	int i = 0, found = 0;

	if (dev_id < 0)
		return NULL;

	while (!found && (i < nr_media_devs)) {
		mdev = &media_devs[i];
		if (mdev->id == dev_id && mdev->bank == bank)
			found = 1;
		else
			i++;
	}

	if (!found)
		mdev = NULL;

	return mdev;
}

dma_addr_t s5p_get_media_memory_bank(int dev_id, int bank)
{
	struct s5p_media_device *mdev;

	mdev = s5p_get_media_device(dev_id, bank);
	if (!mdev) {
		printk(KERN_ERR "invalid media device\n");
		return 0;
	}

	if (!mdev->paddr) {
		printk(KERN_ERR "no memory for %s\n", mdev->name);
		return 0;
	}

	return mdev->paddr;
}
EXPORT_SYMBOL(s5p_get_media_memory_bank);

size_t s5p_get_media_memsize_bank(int dev_id, int bank)
{
	struct s5p_media_device *mdev;

	mdev = s5p_get_media_device(dev_id, bank);
	if (!mdev) {
		printk(KERN_ERR "invalid media device\n");
		return 0;
	}

	return mdev->memsize;
}
EXPORT_SYMBOL(s5p_get_media_memsize_bank);

void s5p_reserve_bootmem(struct s5p_media_device *mdevs, int nr_mdevs)
{
	struct s5p_media_device *mdev;
	int i;

	media_devs = mdevs;
	nr_media_devs = nr_mdevs;

	for (i = 0; i < nr_media_devs; i++) {
		mdev = &media_devs[i];
		if (mdev->memsize <= 0)
			continue;

		if (mdev->paddr)
			mdev->paddr = virt_to_phys(__alloc_bootmem(
				mdev->memsize,
				PAGE_SIZE,
				mdev->paddr));
		else
			mdev->paddr = virt_to_phys(__alloc_bootmem(
				mdev->memsize,
				PAGE_SIZE,
				meminfo.bank[mdev->bank].start));

		printk(KERN_INFO "s5pv210: %lu bytes system memory reserved "
			"for %s at 0x%08x\n", (unsigned long) mdev->memsize,
			mdev->name, mdev->paddr);
	}
}

/* FIXME: temporary implementation to avoid compile error */
int dma_needs_bounce(struct device *dev, dma_addr_t addr, size_t size)
{
	return 0;
}


