/*
 * drivers/media/video/samsung/mfc50/mfc_intr.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code (Key Young, Park)
 *   2009.09.21 - Implement clock & power gating.
 *                including suspend & resume fuction. (Key Young, Park)
 *   2009.10.09 - Add error handling rountine (Key Young, Park)
 *   2009.10.13 - Change wait_for_done (Key Young, Park)
 *   2009.11.04 - remove mfc_common.[ch]
 *                seperate buffer alloc & set (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/io.h>

#include <plat/regs-mfc.h>

#include "mfc_logmsg.h"
#include "mfc_opr.h"
#include "mfc_memory.h"
#include "mfc_intr.h"


#define MFC_WAIT_1_TIME	100
#define MFC_WAIT_2_TIME	200
#define MFC_WAIT_3_TIME	400
#define MFC_WAIT_4_TIME	600
#define MFC_WAIT_5_TIME	800
#define MFC_WAIT_6_TIME	1000

#define ENABLE_MFC_INTERRUPT_DEBUG	0	/* 0: Disable	1: Enable */

#if defined(MFC_REQUEST_TIME)
struct timeval mfc_wakeup_before;
struct timeval mfc_wakeup_after;
#endif

static unsigned int mfc_int_type;
static unsigned int mfc_disp_err_status;
static unsigned int mfc_dec_err_status;
static bool irq_sync;
static DECLARE_WAIT_QUEUE_HEAD(mfc_wait_queue);
static DEFINE_SPINLOCK(mfc_irq_lock);

#if !defined(MFC_POLLING)
irqreturn_t mfc_irq(int irq, void *dev_id)
{
	unsigned int int_reason;
	unsigned int err_status;

	int_reason = READL(MFC_RISC2HOST_COMMAND) & 0x1FFFF;
	err_status = READL(MFC_RISC2HOST_ARG2);

	mfc_disp_err_status = err_status >> 16;
	mfc_dec_err_status = err_status & 0xFFFF;

	mfc_debug_L0("mfc_irq() : Interrupt !! : %d\n", int_reason);

	if (((int_reason & R2H_CMD_FRAME_DONE_RET)     == R2H_CMD_FRAME_DONE_RET)      ||
		((int_reason & R2H_CMD_SEQ_DONE_RET)       == R2H_CMD_SEQ_DONE_RET)        ||
		((int_reason & R2H_CMD_SYS_INIT_RET)       == R2H_CMD_SYS_INIT_RET)        ||
		((int_reason & R2H_CMD_OPEN_INSTANCE_RET)  == R2H_CMD_OPEN_INSTANCE_RET)   ||
		((int_reason & R2H_CMD_CLOSE_INSTANCE_RET) == R2H_CMD_CLOSE_INSTANCE_RET)  ||
		((int_reason & R2H_CMD_INIT_BUFFERS_RET) == R2H_CMD_INIT_BUFFERS_RET)      ||
		((int_reason & R2H_CMD_DECODE_ERR_RET)   == R2H_CMD_DECODE_ERR_RET)        ||
		((int_reason & R2H_CMD_SLICE_DONE_RET)   == R2H_CMD_SLICE_DONE_RET)        ||
		((int_reason & R2H_CMD_ERROR_RET) == R2H_CMD_ERROR_RET)) {
		mfc_int_type = int_reason;
		irq_sync = true;
		wake_up(&mfc_wait_queue);
	} else {
		irq_sync = false;
		mfc_info("Strange Interrupt !! : %d\n", int_reason);
	}


	WRITEL(0, MFC_RISC_HOST_INT);
	WRITEL(0, MFC_RISC2HOST_COMMAND);
	WRITEL(0xffff, MFC_SI_RTN_CHID);

	return IRQ_HANDLED;
}
#endif

void mfc_interrupt_debug(int nCnt)
{
	int nn = 0;
	for (nn = 0; nn < nCnt; nn++) {
		mdelay(100);
		mfc_err("[%d] Timeout (0x64: 0x%08x) (0xF4: 0x%08x)\n", nn, READL(0x64), READL(0xF4));
	}
}

int mfc_wait_for_done(enum mfc_wait_done_type command)
{
	unsigned int nwait_time = 100;
	unsigned int ret_val = 1;
	unsigned long flags;

	if ((command == R2H_CMD_CLOSE_INSTANCE_RET) ||
	   (command == R2H_CMD_OPEN_INSTANCE_RET) ||
	   (command == R2H_CMD_SYS_INIT_RET) ||
	   (command == R2H_CMD_FW_STATUS_RET))
		nwait_time = MFC_WAIT_6_TIME;
	else
		nwait_time = MFC_WAIT_2_TIME;


#if defined(MFC_REQUEST_TIME)
	long sec, msec;
#endif

#if defined(MFC_POLLING)
	unsigned long timeo = jiffies;
	timeo += 20;    /* waiting for 100ms */
#endif

#if defined(MFC_REQUEST_TIME)
	do_gettimeofday(&mfc_wakeup_before);
	if (mfc_wakeup_before.tv_usec - mfc_wakeup_after.tv_usec < 0) {
		msec = 1000000 + mfc_wakeup_before.tv_usec - mfc_wakeup_after.tv_usec;
		sec = mfc_wakeup_before.tv_sec - mfc_wakeup_after.tv_sec - 1;
	} else {
		msec = mfc_wakeup_before.tv_usec - mfc_wakeup_after.tv_usec;
		sec = mfc_wakeup_before.tv_sec - mfc_wakeup_after.tv_sec;
	}
#endif

#if defined(MFC_POLLING)
	while (time_before(jiffies, timeo))
		ret_val = READL(MFC_RISC2HOST_COMMAND) & 0x1ffff;
		if (ret_val != 0) {
			WRITEL(0, MFC_RISC_HOST_INT);
			WRITEL(0, MFC_RISC2HOST_COMMAND);
			WRITEL(0xffff, MFC_SI_RTN_CHID);
			mfc_int_type = ret_val;
			break;
		}
		msleep_interruptible(2);
	}

	if (ret_val == 0)
		printk(KERN_INFO "MFC timeouted!\n");
#else
	if (wait_event_timeout(mfc_wait_queue, irq_sync, nwait_time) == 0) {
		ret_val = 0;
		mfc_err("Interrupt Time Out(Cmd: %d)	(Ver: 0x%08x) (0x64: 0x%08x) (0xF4: 0x%08x) (0x80: 0x%08x)\n", command, READL(0x58), READL(0x64), READL(0xF4), READL(0x80));

#if ENABLE_MFC_INTERRUPT_DEBUG	/* For MFC Interrupt Debugging. */
		mfc_interrupt_debug(10);
#endif

		mfc_int_type = 0;
		return ret_val;
	} else if (mfc_int_type == R2H_CMD_DECODE_ERR_RET) {
		mfc_err("Decode Error Returned Disp Error Status(%d), Dec Error Status(%d)\n", mfc_disp_err_status, mfc_dec_err_status);
	} else if (command != mfc_int_type) {
		mfc_err("Interrupt Error Returned (%d) waiting for (%d)\n", mfc_int_type, command);
	}
#endif
	spin_lock_irqsave(&mfc_irq_lock, flags);
	irq_sync = false;
	spin_unlock_irqrestore(&mfc_irq_lock, flags);

#if defined(MFC_REQUEST_TIME)
	do_gettimeofday(&mfc_wakeup_after);
	if (mfc_wakeup_after.tv_usec - mfc_wakeup_before.tv_usec < 0) {
		msec = 1000000 + mfc_wakeup_after.tv_usec - mfc_wakeup_before.tv_usec;
		sec = mfc_wakeup_after.tv_sec - mfc_wakeup_before.tv_sec - 1;
	} else {
		msec = mfc_wakeup_after.tv_usec - mfc_wakeup_before.tv_usec;
		sec = mfc_wakeup_after.tv_sec - mfc_wakeup_before.tv_sec;
	}

	mfc_info("mfc_wait_for_done: mfc request interval time is %ld(sec), %ld(msec)\n", sec, msec);
#endif

	ret_val = mfc_int_type;
	mfc_int_type = 0;

	return ret_val;
}


int mfc_return_code(void)
{
	return mfc_dec_err_status;
}
