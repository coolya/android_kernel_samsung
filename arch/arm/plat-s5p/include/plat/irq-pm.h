/* linux/arch/arm/plat-s5p/include/plat/irq-pm.h
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P Common IRQ support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __PLAT_S5P_IRQ_PM_H
#define __PLAT_S5P_IRQ_PM_H
int s3c_irq_wake(unsigned int irqno, unsigned int state);
#endif /* __PLAT_S5P_IRQ_PM_H */
