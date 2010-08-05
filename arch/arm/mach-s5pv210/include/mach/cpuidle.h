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

#define NORMAL_MODE	0
#define LPAUDIO_MODE	1

extern int previous_idle_mode;
extern int idle2_lock_count;
extern int s5p_setup_lpaudio(unsigned int mode);
extern void s5p_set_lpaudio_lock(int flag);
extern int s5p_get_lpaudio_lock(void);
