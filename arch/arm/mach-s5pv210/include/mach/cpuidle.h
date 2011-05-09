/* arch/arm/mach-s5pv210/include/mach/cpuidle.h
 *
 * Copyright 2010 Samsung Electronics
 *	Jaecheol Lee <jc.lee@samsung>
 *
 * S5PV210 - CPUIDLE support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

extern int  s5pv210_didle_save(unsigned long *saveblk);
extern void s5pv210_didle_resume(void);
extern void i2sdma_getpos(dma_addr_t *src);
extern unsigned int get_rtc_cnt(void);
