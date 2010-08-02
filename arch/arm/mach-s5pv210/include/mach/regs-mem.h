/* linux/arch/arm/mach-s5pv210/include/mach/regs-mem.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Memory Control register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_MEM_H
#define __ASM_ARCH_REGS_MEM_H __FILE__

#include <mach/map.h>

#define S5P_MEMREG(x)		(S5P_VA_SROMC + (x))

#define S5P_SROM_BW		S5P_MEMREG(0x00)
#define S5P_SROM_BC0		S5P_MEMREG(0x04)
#define S5P_SROM_BC1		S5P_MEMREG(0x08)
#define S5P_SROM_BC2		S5P_MEMREG(0x0C)
#define S5P_SROM_BC3		S5P_MEMREG(0x10)
#define S5P_SROM_BC4		S5P_MEMREG(0x14)
#define S5P_SROM_BC5		S5P_MEMREG(0x18)

#endif /* __ASM_ARCH_REGS_MEM_H */
