/* linux/arch/arm/mach-s5pv210/include/mach/regs-irq.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - IRQ register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_IRQ_H
#define __ASM_ARCH_REGS_IRQ_H __FILE__

#include <asm/hardware/vic.h>
#include <mach/map.h>

/* interrupt controller */
#define S5P_VIC0REG(x)                  ((x) + S5P_VA_VIC0)
#define S5P_VIC1REG(x)                  ((x) + S5P_VA_VIC1)
#define S5P_VIC2REG(x)                  ((x) + S5P_VA_VIC2)
#define S5P_VIC3REG(x)                  ((x) + S5P_VA_VIC3)

#endif /* __ASM_ARCH_REGS_IRQ_H */
