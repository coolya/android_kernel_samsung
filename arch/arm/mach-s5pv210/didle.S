/* linux/arch/arm/mach-s5pv210/didle.S
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <asm/asm-offsets.h>
#include <asm/memory.h>
#include <asm/system.h>

/*
 *	v7_flush_l1_dcache()
 *
 *	Flush the L1 D-cache.
 */
ENTRY(v7_flush_l1_dcache)
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	ands	r3, r0, #0x7000000		@ extract loc from clidr
	mov	r3, r3, lsr #23			@ left align loc bit field
	beq	finished			@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
loop1:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	finished			@ finish if no cache, or just i-cache
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
loop2:
	mov	r9, r4				@ create working copy of max way size
loop3:
	orr	r11, r10, r9, lsl r5		@ factor way and cache number into r11
	orr	r11, r11, r7, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, c14, 2		@ clean & invalidate by set/way
	subs	r9, r9, #1			@ decrement the way
	bge	loop3
	subs	r7, r7, #1			@ decrement the index
	bge	loop2
finished:
	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb
	isb
	mov	pc, lr
ENDPROC(v7_flush_l1_dcache)

ENTRY(v7_flush_cache_for_didle)
	stmfd	sp!, {r4-r5, r7, r9-r11, lr}
	bl	v7_flush_l1_dcache
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}
	mov	pc, lr
ENDPROC(v7_flush_cache_for_didle)

ENTRY(s5pv210_didle)
	stmfd	sp!, {r4-r5, r7, r9-r11, lr}

	bl	v7_flush_cache_for_didle

	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}
	dmb
	dsb
	wfi

	b	.

	.text

	/* s5pv210_didle_save
	 *
	 * entry:
	 *	r0 = save address (virtual addr of s3c_sleep_save_phys)
	*/

ENTRY(s5pv210_didle_save)

	stmfd	sp!, { r3 - r12, lr }

	mrc	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mrc	p15, 0, r5, c3, c0, 0	@ Domain ID
	mrc	p15, 0, r6, c2, c0, 0	@ Translation Table BASE0
	mrc	p15, 0, r7, c2, c0, 1	@ Translation Table BASE1
	mrc	p15, 0, r8, c2, c0, 2	@ Translation Table Control
	mrc	p15, 0, r9, c1, c0, 0	@ Control register
	mrc	p15, 0, r10, c1, c0, 1	@ Auxiliary control register
	mrc	p15, 0, r11, c1, c0, 2	@ Co-processor access controls
	mrc	p15, 0, r12, c10, c2, 0	@ Read PRRR
	mrc	p15, 0, r3, c10, c2, 1	@ READ NMRR

	/* Save CP15 registers */
	stmia	r0, { r3 - r13 }

	bl s5pv210_didle

	@@ return to the caller, after having the MMU
	@@ turned on, this restores the last bits from the
	@@ stack
resume_with_mmu:
	mrc     p15, 0, r0, c1, c0, 1	@enable L2 cache
	orr     r0, r0, #(1<<1)
	mcr     p15, 0, r0, c1, c0, 1

	mov	r0, #1
	/* delete added mmu table list */
	ldr	r9 , =(PAGE_OFFSET - PHYS_OFFSET)
	add	r4, r4, r9
	str	r12, [r4]

	ldmfd	sp!, { r3 - r12, pc }

	.ltorg

	/* s5pv210_didle_resume
	 *
	 * resume code entry for bootloader to call
	 *
	 * we must put this code here in the data segment as we have no
	 * other way of restoring the stack pointer after sleep, and we
	 * must not write to the code segment (code is read-only)
	*/

ENTRY(s5pv210_didle_resume)
	mov	r0, #PSR_I_BIT | PSR_F_BIT | SVC_MODE
	msr	cpsr_c, r0

	@@ load UART to allow us to print the two characters for
	@@ resume debug

	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0		@@ invalidate TLBs
	mcr	p15, 0, r1, c7, c5, 0		@@ invalidate I Cache

	ldr	r1, =0xe010f008		@ Read INFORM2 register
	ldr	r0, [r1]		@ Load phy_regs_save value
	ldmia	r0, { r3 - r13 }

	mcr	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mcr	p15, 0, r5, c3, c0, 0	@ Domain ID

	mcr	p15, 0, r8, c2, c0, 2	@ Translation Table Control
	mcr	p15, 0, r7, c2, c0, 1	@ Translation Table BASE1
	mcr	p15, 0, r6, c2, c0, 0	@ Translation Table BASE0

	bic	r10, r10, #(1<<1)	@ disable L2cache
	mcr	p15, 0, r10, c1, c0, 1	@ Auxiliary control register

	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	@ Invalidate I & D TLB

	mov	r0, #0			@ restore copro access controls
	mcr	p15, 0, r11, c1, c0, 2	@ Co-processor access controls
	mcr 	p15, 0, r0, c7, c5, 4

	mcr	p15, 0, r12, c10, c2, 0	@ write PRRR
	mcr	p15, 0, r3, c10, c2, 1	@ write NMRR

	/* calculate first section address into r8 */
	mov	r4, r6
	ldr	r5, =0x3fff
	bic	r4, r4, r5
	ldr	r11, =0xe010f000
	ldr	r10, [r11, #0]
	mov	r10, r10 ,LSR #18
	bic	r10, r10, #0x3
	orr	r4, r4, r10

	/* calculate mmu list value into r9 */
	mov 	r10, r10, LSL #18
	ldr	r5, =0x40e
	orr	r10, r10, r5

	/* back up originally data */
	ldr	r12, [r4]

	/* Added list about mmu */
	str	r10, [r4]

	ldr	r2, =resume_with_mmu
	mcr	p15, 0, r9, c1, c0, 0		@ turn on MMU, etc

        nop
        nop
        nop
        nop
        nop					@ second-to-last before mmu

	mov	pc, r2				@ go back to virtual address

	.ltorg
