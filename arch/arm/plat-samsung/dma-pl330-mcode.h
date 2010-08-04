/* linux/arch/arm/plat-s5p/dma-pl330-mcode.h
 *
 * Copyright 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P - DMA PL330 microcode
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define	PL330_DMA_DEBUG
#undef	PL330_DMA_DEBUG

#ifdef PL330_DMA_DEBUG
#define dma_debug(fmt...) 		printk(fmt)
#else
#define dma_debug(fmt...)
#endif

#define print_warning(fmt...) 		printk(fmt)

#define PL330_P2M_DMA			0
#define PL330_M2P_DMA			1
#define PL330_M2M_DMA			2
#define PL330_P2P_DMA			3

#define MSM_REQ0	26
#define MSM_REQ1	27
#define MSM_REQ2	28
#define MSM_REQ3	29

#define	PL330_MAX_ITERATION_NUM		256
#define	PL330_MAX_JUMPBACK_NUM		256
#define	PL330_MAX_EVENT_NUM		32
#define	PL330_MAX_PERIPHERAL_NUM	32
#define	PL330_MAX_CHANNEL_NUM		8

#define DMA_DBGSTATUS			0x0
#define DMA_DBGCMD			0x1
#define DMA_DBGINST0			0x2
#define DMA_DBGINST1			0x3

#define memOutp8(addr, data) 	(*(volatile u8 *)(addr) = (data))
#define Outp32(addr, data)	(*(volatile u32 *)(addr) = (data))
#define Inp32(addr)		(*(volatile u32 *)(addr))

/* Parameter set for Channel Control Register */
typedef struct DMA_control {
	unsigned uSI:1;		/* [0] Transfer size not count		*/
	unsigned uSBSize:3;	/* [3:1] Source 1 transfer size		*/
	unsigned uSBLength:4;	/* [7:4] Sourse burst len		*/
	unsigned uSProt:3;	/* [10:8] Source Protection set 101b=5	*/
	unsigned uSCache:3;	/* [13:11] Source Cache control		*/
	unsigned uDI:1;		/* [14] Destination increment		*/
	unsigned uDBSize:3;	/* [17:15] Destination 1 transfer size	*/
	unsigned uDBLength:4;	/* [21:18] Destination burst len	*/
	unsigned uDProt:3;	/* [24:22] Source Protection set 101b=5	*/
	unsigned uDCache:3;	/* [27:25] Source Cache control		*/
	unsigned uESSize:4;	/* [31:28] endian_swap_size		*/
} pl330_DMA_control_t;


/* Parameter list for a DMA operation */
typedef struct DMA_parameters {
	unsigned long			mDirection;	/* DMA direction */
	unsigned long			mPeriNum;	/* DMA Peripheral number */
	unsigned long			mSrcAddr;	/* DMA source address */
	unsigned long			mDstAddr;	/* DMA destination address */
	unsigned long			mTrSize;	/* DMA Transfer size */
	pl330_DMA_control_t		mControl;	/* DMA control */
	unsigned long			mIrqEnable;	/* DMA Send IRQ */
	unsigned long			mLoop;		/* DMA Infinite Loop - 0(off) */
	unsigned long			mBwJump;	/* DMA backward relative offset */
	unsigned long			mLastReq;	/* The last DMA Req.  */
} pl330_DMA_parameters_t;


static void print_dma_param_info(pl330_DMA_parameters_t dma_param)
{
	/* Parameter list for a DMA operation */
	dma_debug("	mDirection = %lu\n", dma_param.mDirection);
	dma_debug("	mPeriNum = %lu\n", dma_param.mPeriNum);
	dma_debug("	mSrcAddr = 0x%x\n", dma_param.mSrcAddr);
	dma_debug("	mDstAddr = 0x%x\n", dma_param.mDstAddr);
	dma_debug("	mTrSize = %lu\n", dma_param.mTrSize);
	dma_debug("	mControl = 0x%x\n", dma_param.mControl);
	dma_debug("	mIrqEnable = %lu\n", dma_param.mIrqEnable);
	dma_debug("	mLoop = %lu\n", dma_param.mLoop);
	dma_debug("	mBwJump = %lu\n", dma_param.mBwJump);
	dma_debug("	mLastReq = %lu\n", dma_param.mLastReq);
}

/*---------------------- Primitive functions -------------*/
/* When the DMAC is operating in real-time then you can only issue a limited
 * subset of instructions as follows:
 * DMAGO starts a DMA transaction using a DMA channel that you specify.
 * DMASEV signals the occurrence of an event, or interrupt, using an
 * event number that you specify.
 * DMAKILL terminates a thread.

 * Prior to issuing DMAGO, you must ensure that the system memory contains
 * a suitable program for the DMAC to execute, starting at the address that
 * the DMAGO specifies.
 */

/* DMAMOV CCR, ...  */
static int encodeDmaMoveChCtrl(u8 *mcode_ptr, u32 dmacon)
{
	u8 uInsBytes[6];
	u32 i;

	uInsBytes[0] = (u8)(0xbc);
	uInsBytes[1] = (u8)(0x1);
	uInsBytes[2] = (u8)((dmacon>>0)&0xff);
	uInsBytes[3] = (u8)((dmacon>>8)&0xff);
	uInsBytes[4] = (u8)((dmacon>>16)&0xff);
	uInsBytes[5] = (u8)((dmacon>>24)&0xff);

	for (i = 0; i < 6; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 6;
}

/* DMAMOV SAR, uStAddr
 * DMAMOV DAR, uStAddr   */
static int encodeDmaMove(u8 *mcode_ptr, u8 uDir, u32 uStAddr)
{
	u8 uInsBytes[6];
	u32 i;

	uInsBytes[0] = (u8)(0xbc);
	uInsBytes[1] = (u8)(0x0|uDir);
	uInsBytes[2] = (u8)((uStAddr>>0)&0xff);
	uInsBytes[3] = (u8)((uStAddr>>8)&0xff);
	uInsBytes[4] = (u8)((uStAddr>>16)&0xff);
	uInsBytes[5] = (u8)((uStAddr>>24)&0xff);

	for (i = 0; i < 6; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 6;
}

/* DMALD, DMALDS, DMALDB  */
static int encodeDmaLoad(u8 *mcode_ptr)
{
	u8 bs = 0;
	u8 x = 0;
	u8 uInsBytes[1];
	u32 i;

	uInsBytes[0] = (u8)(0x04|(bs<<1)|(x<<0));

	for (i = 0; i < 1; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 1;
}

/* DMALDPS, DMALDPB (Load Peripheral)  */
static int encodeDmaLoadPeri(u8 *mcode_ptr, u8 mPeriNum, u8 m_uBurstSz)
{
	u8 bs;
	u8 uInsBytes[2];
	u32 i;

	if (mPeriNum > PL330_MAX_PERIPHERAL_NUM) {
		print_warning("[%s] The peripheral number is too big ! : %d\n",
				__func__, mPeriNum);
		return 0;
	}

	bs = (m_uBurstSz == 1) ? 0 : 1; /* single -> 0, burst -> 1 */

	uInsBytes[0] = (u8)(0x25|(bs<<1));
	uInsBytes[1] = (u8)(0x00|((mPeriNum&0x1f)<<3));

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 2;
}

/* DMAST, DMASTS, DMASTB  */
static int encodeDmaStore(u8 *mcode_ptr)
{
	u8 bs = 0;
	u8 x = 0;
	u8 uInsBytes[1];
	u32 i;

	uInsBytes[0] = (u8)(0x08|(bs<<1)|(x<<0));

	for (i = 0; i < 1; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 1;
}

/* DMASTPS, DMASTPB (Store and notify Peripheral)  */
static int encodeDmaStorePeri(u8 *mcode_ptr, u8 mPeriNum, u8 m_uBurstSz)
{
	u8 bs;
	u8 uInsBytes[2];
	u32 i;

	if (mPeriNum > PL330_MAX_PERIPHERAL_NUM) {
		print_warning("[%s] The peripheral number is too big ! : %d\n",
				__func__, mPeriNum);
		return 0;
	}

	bs = (m_uBurstSz == 1) ? 0 : 1; /* single:0, burst:1 */

	uInsBytes[0] = (u8)(0x29|(bs<<1));
	uInsBytes[1] = (u8)(0x00|((mPeriNum&0x1f)<<3));

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 2;
}

#ifdef PL330_WILL_BE_USE
/* DMASTZ  */
static int encodeDmaStoreZero(u8 *mcode_ptr)
{
	u8 uInsBytes[1];
	u32 i;

	uInsBytes[0] = (u8)(0x0c);

	for (i = 0; i < 1; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 1;
}
#endif

/* DMALP  */
static int encodeDmaLoop(u8 *mcode_ptr, u8 uLoopCnt, u8 uIteration)
{
	u8 uInsBytes[2];
	u32 i;

	uInsBytes[0] = (u8)(0x20|(uLoopCnt<<1));
	uInsBytes[1] = (u8)(uIteration);

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 2;

}

/* DMALPFE  */
static int encodeDmaLoopForever(u8 *mcode_ptr, u8 uBwJump)
{
	u8 bs = 0;
	u8 x = 0;
	u8 uInsBytes[2];
	u32 i;

	uInsBytes[0] = (u8)(0x28|(0<<4)|(0<<2)|(bs<<1)|x);
	uInsBytes[1] = (u8)(uBwJump);

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr + i, uInsBytes[i]);

	return 2;
}

/* DMALPEND, DMALPENDS, DMALPENDB  */
static int encodeDmaLoopEnd(u8 *mcode_ptr, u8 uLoopCnt, u8 uBwJump)
{
	u8 bs = 0;
	u8 x = 0;
	u8 uInsBytes[2];
	u32 i;

	uInsBytes[0] = (u8)(0x38|(1<<4)|(uLoopCnt<<2)|(bs<<1)|x);
	uInsBytes[1] = (u8)(uBwJump);

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr + i, uInsBytes[i]);

	return 2;
}

/*  DMAWFP, DMAWFPS, DMAWFPB (Wait For Peripheral) */
static int encodeDmaWaitForPeri(u8 *mcode_ptr, u8 mPeriNum)
{
	u8 bs = 0;
	u8 p = 0;
	u8 uInsBytes[2];
	u32 i;

	if (mPeriNum > PL330_MAX_PERIPHERAL_NUM) {
		print_warning("[%s] The peripheral number is too big ! : %d\n",
				__func__, mPeriNum);
		return 0;
	}

	uInsBytes[0] = (u8)(0x30|(bs<<1)|p);
	uInsBytes[1] = (u8)(0x00|((mPeriNum&0x1f)<<3));

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 2;
}

#ifdef PL330_WILL_BE_USE
/* DMAWFE (Wait For Event) : 0 ~ 31 */
static int encodeDmaWaitForEvent(u8 *mcode_ptr, u8 uEventNum)
{
	u8 uInsBytes[2];
	u32 i;

	if (uEventNum > PL330_MAX_EVENT_NUM) {
		print_warning("[%s] The uEventNum number is too big ! : %d\n",
				__func__, uEventNum);
		return 0;
	}

	uInsBytes[0] = (u8)(0x36);
	uInsBytes[1] = (u8)((uEventNum<<3)|0x2); /* for cache coherency, invalid is issued.  */

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 2;
}
#endif

/*  DMAFLUSHP (Flush and notify Peripheral) */
static int encodeDmaFlushPeri(u8 *mcode_ptr, u8 mPeriNum)
{
	u8 uInsBytes[2];
	u32 i;

	if (mPeriNum > PL330_MAX_PERIPHERAL_NUM) {
		print_warning("[%s] The peripheral number is too big ! : %d\n",
				__func__, mPeriNum);
		return 0;
	}

	uInsBytes[0] = (u8)(0x35);
	uInsBytes[1] = (u8)(0x00|((mPeriNum&0x1f)<<3));

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 2;
}

/* DMAEND */
static int encodeDmaEnd(u8 *mcode_ptr)
{
	memOutp8(mcode_ptr, 0x00);

	return 1;
}

#ifdef PL330_WILL_BE_USE
/* DMAADDH (Add Halfword) */
static int encodeDmaAddHalfword(u8 *mcode_ptr, bool bSrcDir, u16 uStAddr)
{
	u8 uDir = (bSrcDir) ? 0 : 1; /* src addr = 0, dst addr=1 */
	u8 uInsBytes[3];
	u32 i;

	uInsBytes[0] = (u8)(0x54|(uDir<<1));
	uInsBytes[1] = (u8)((uStAddr>>0)&0xff);
	uInsBytes[2] = (u8)((uStAddr>>8)&0xff);

	for (i = 0; i < 3; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 3;
}
#endif

#ifdef PL330_WILL_BE_USE
/* DMAKILL (Kill) */
static int encodeDmaKill(u8 *mcode_ptr)
{
	u8 uInsBytes[1];
	u32 i;
	uInsBytes[0] = (u8)(0x01);

	for (i = 0; i < 1; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 1;
}
#endif

#ifdef PL330_WILL_BE_USE
/* DMANOP (No operation) */
static int encodeDmaNop(u8 *mcode_ptr)
{
	u8 uInsBytes[1];
	u32 i;
	uInsBytes[0] = (u8)(0x18);

	for (i = 0; i < 1; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 1;
}
#endif

/* DMARMB (Read Memory Barrier) */
static int encodeDmaReadMemBarrier(u8 *mcode_ptr)
{
	u8 uInsBytes[1];
	u32 i;
	uInsBytes[0] = (u8)(0x12);

	for (i = 0; i < 1; i++)
		memOutp8(mcode_ptr+i, uInsBytes[i]);

	return 1;
}

/* DMASEV (Send Event) : 0 ~ 31 */
static int encodeDmaSendEvent(u8 *mcode_ptr, u8 uEventNum)
{
	u8 uInsBytes[2];
	u32 i;
	uInsBytes[0] = (u8)(0x34);
	uInsBytes[1] = (u8)((uEventNum<<3)|0x0);

	if (uEventNum > PL330_MAX_EVENT_NUM) {
		print_warning("[%s] Event number is too big ! : %d\n",
				__func__, uEventNum);
		return 0;
	}

	for (i = 0; i < 2; i++)
		memOutp8(mcode_ptr + i, uInsBytes[i]);

	return 2;
}

/* DMAWMB (Write Memory Barrier) */
static int encodeDmaWriteMemBarrier(u8 *mcode_ptr)
{
	u8 uInsBytes[1];
	u32 i;
	uInsBytes[0] = (u8)(0x13);

	for (i = 0; i < 1; i++)
		memOutp8(mcode_ptr + i, uInsBytes[i]);

	return 1;
}

/* DMAGO over DBGINST[0:1] registers */
static void encodeDmaGoOverDBGINST(u32 *mcode_ptr, u8 chanNum,
				   u32 mbufAddr, u8 m_secureBit)
{
	u32 x;
	u8 uDmaGo;		/* DMAGO instruction */

	if (chanNum > PL330_MAX_CHANNEL_NUM) {
		print_warning("[%s] Channel number is too big ! : %d\n",
				__func__, chanNum);
		return;
	}

	do {
		x = Inp32(mcode_ptr+DMA_DBGSTATUS);
	} while ((x&0x1) == 0x1);

	uDmaGo = (m_secureBit == 0) ?
		(0xa0|(0<<1)) : 	/* secure mode : M2M DMA only   */
		(0xa0|(1<<1));  	/* non-secure mode : M2P/P2M DMA only */

	Outp32(mcode_ptr+DMA_DBGINST0, (chanNum<<24)|(uDmaGo<<16)|(chanNum<<8)|(0<<0));
	Outp32(mcode_ptr+DMA_DBGINST1, mbufAddr);
	Outp32(mcode_ptr+DMA_DBGCMD, 0);/* 0 : execute the instruction that the DBGINST0,1 registers contain */
}

/* DMAKILL over DBGINST[0:1] registers - Stop a DMA channel */
static void encodeDmaKillChannelOverDBGINST(u32 *mcode_ptr, u8 chanNum)
{
	u32 x;

	if (chanNum > PL330_MAX_CHANNEL_NUM) {
		print_warning("[%s] Channel number is too big ! : %d\n",
				__func__, chanNum);
		return;
	}

	do {
		x = Inp32(mcode_ptr+DMA_DBGSTATUS);
	} while ((x&0x1) == 0x1);

	Outp32(mcode_ptr+DMA_DBGINST0, (0<<24)|(1<<16)|(chanNum<<8)|(1<<0));	/* issue instruction by channel thread */
	Outp32(mcode_ptr+DMA_DBGINST1, 0);
	Outp32(mcode_ptr+DMA_DBGCMD, 0); 	/* 0 : execute the instruction that the DBGINST0,1 registers contain */

	do {
		x = Inp32(mcode_ptr+DMA_DBGSTATUS);
	} while ((x&0x1) == 0x1);
}

/* DMAKILL over DBGINST[0:1] registers - Stop a DMA controller (stop all of the channels) */
static void encodeDmaKillDMACOverDBGINST(u32 *mcode_ptr)
{
	u32 x;

	do {
		x = Inp32(mcode_ptr+DMA_DBGSTATUS);
	} while ((x&0x1) == 0x1);

	Outp32(mcode_ptr+DMA_DBGINST0, (0<<24)|(1<<16)|(0<<8)|(0<<0));	/* issue instruction by manager thread */
	Outp32(mcode_ptr+DMA_DBGINST1, 0);
	Outp32(mcode_ptr+DMA_DBGCMD, 0); 	/* 0 : execute the instruction that the DBGINST0,1 registers contain */

	do {
		x = Inp32(mcode_ptr+DMA_DBGSTATUS);
	} while ((x&0x1) == 0x1);
}

/*----------------------------------------------------------*/
/*                      Wrapper functions                   */
/*----------------------------------------------------------*/

/* config_DMA_Go_command
 * - make DMA GO command into the Debug Instruction register 0/1
 *
 *	mcode_ptr	the buffer for PL330 DMAGO micro code to be stored into
 *	chanNum		the DMA channel number to be started
 *	mbufAddr	the start address of the buffer containing PL330 DMA micro codes
 */
static void config_DMA_GO_command(u32 *mcode_ptr, int chanNum, u32 mbufAddr, int secureMode)
{
	dma_debug("%s entered - channel Num=%d\n", __func__, chanNum);
	dma_debug("mcode_ptr=0x%p, mbufAddr=0x%x, secureMode=%d\n\n", mcode_ptr, mbufAddr, secureMode);
	encodeDmaGoOverDBGINST(mcode_ptr, (u8)chanNum, mbufAddr, (u8)secureMode);
}

/* config_DMA_stop_channel
 * - stop the DMA channel working on
 *	mcode_ptr	the buffer for PL330 DMAKILL micro code to be stored into
 *	chanNum		the DMA channel number to be stopped
 */
static void config_DMA_stop_channel(u32 *mcode_ptr, int chanNum)
{
	dma_debug("%s entered - channel Num=%d\n", __func__, chanNum);
	encodeDmaKillChannelOverDBGINST(mcode_ptr, (u8)chanNum);
}

/* config_DMA_stop_controller
 * - stop the DMA controller
 *	mcode_ptr	the buffer for PL330 DMAKILL micro code to be stored into
 */
static void config_DMA_stop_controller(u32 *mcode_ptr)
{
	dma_debug("%s entered - mcode_ptr=0x%p\n", __func__, mcode_ptr);
	encodeDmaKillDMACOverDBGINST(mcode_ptr);
}

/* config_DMA_start_address
 * - set the DMA start address
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMAMOVE micro code to be stored into
 *	uStAddr		the DMA start address
 */
static int config_DMA_start_address(u8 *mcode_ptr, int uStAddr)
{
	dma_debug("%s entered - start addr=0x%x\n", __func__, uStAddr);
	return encodeDmaMove(mcode_ptr, 0, (u32)uStAddr);
}

/* config_DMA_destination_address
 * - set the DMA destination address
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMAMOVE micro code to be stored into
 *	uStAddr		the DMA destination address
 */
static int config_DMA_destination_address(u8 *mcode_ptr, int uStAddr)
{
	dma_debug("%s entered - destination addr=0x%x\n", __func__, uStAddr);
	return encodeDmaMove(mcode_ptr, 2, (u32)uStAddr);
}

/* config_DMA_control
 * - set the burst length, burst size, source and destination increment/fixed field
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMAMOVE micro code to be stored into
 *	dmacon		the value for the DMA channel control register
 */
static int config_DMA_control(u8 *mcode_ptr, pl330_DMA_control_t dmacon)
{
	dma_debug("%s entered - dmacon : 0x%p\n", __func__, &dmacon);
	return encodeDmaMoveChCtrl(mcode_ptr, *(u32 *)&dmacon);
}

/* config_DMA_transfer_remainder
 * - set the transfer size of the remainder
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMA micro code to be stored into
 *	lcRemainder	the remainder except for the LC-aligned transfers
 *	dma_param	the parameter set for a DMA operation
 */
static int config_DMA_transfer_remainder(u8 *mcode_ptr, int lcRemainder, pl330_DMA_parameters_t dma_param)
{
	int mcode_size = 0, msize = 0;
	int lc0 = 0, lcSize = 0, mLoopStart0 = 0, dmaSent = 0;

	dma_debug("%s entered - lcRemainder=%d\n", __func__, lcRemainder);

	dmaSent = dma_param.mTrSize - lcRemainder;

	msize = config_DMA_start_address(mcode_ptr+mcode_size, dma_param.mSrcAddr+dmaSent);
	mcode_size += msize;

	msize = config_DMA_destination_address(mcode_ptr+mcode_size, dma_param.mDstAddr+dmaSent);
	mcode_size += msize;

	dma_param.mControl.uSBSize = 0x2;	/* 4 bytes    */
	dma_param.mControl.uSBLength = 0x0;	/* 1 transfer */
	dma_param.mControl.uDBSize = 0x2;	/* 4 bytes    */
	dma_param.mControl.uDBLength = 0x0;	/* 1 transfer */

	msize = config_DMA_control(mcode_ptr+mcode_size, dma_param.mControl);
	mcode_size += msize;

	lcSize = (dma_param.mControl.uSBLength+1)*(1<<dma_param.mControl.uSBSize);
	lc0 = lcRemainder/lcSize;

	msize = encodeDmaLoop(mcode_ptr+mcode_size, 0, lc0-1);
	mcode_size += msize;
	mLoopStart0 = mcode_size;

	switch (dma_param.mDirection) {
	case PL330_M2M_DMA:
		msize = encodeDmaLoad(mcode_ptr+mcode_size);
		mcode_size += msize;
		msize = encodeDmaReadMemBarrier(mcode_ptr+mcode_size);
		mcode_size += msize;
		msize = encodeDmaStore(mcode_ptr+mcode_size);
		mcode_size += msize;
		msize = encodeDmaWriteMemBarrier(mcode_ptr+mcode_size);
		mcode_size += msize;
		break;

	case PL330_M2P_DMA:
		msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
		mcode_size += msize;
		if ((u8)dma_param.mPeriNum == MSM_REQ0 || (u8)dma_param.mPeriNum == MSM_REQ1) {
			msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
			msize = encodeDmaLoad(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaStorePeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum, 1);
			mcode_size += msize;
		} else {
			msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
			msize = encodeDmaLoad(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaStore(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
		}
		break;

	case PL330_P2M_DMA:
		msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
		mcode_size += msize;
		if ((u8)dma_param.mPeriNum == MSM_REQ2 || (u8)dma_param.mPeriNum == MSM_REQ3) {
			msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
			msize = encodeDmaLoad(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaStore(mcode_ptr+mcode_size);
			mcode_size += msize;
		} else {
			msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
			msize = encodeDmaLoadPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum, 1);
			mcode_size += msize;
			msize = encodeDmaStore(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
		}
		break;

	case PL330_P2P_DMA:
		print_warning("[%s] P2P DMA selected !\n", __func__);
		break;

	default:
		print_warning("[%s] Invaild DMA direction selected !\n",
				__func__);
		break;
	}

	msize = encodeDmaLoopEnd(mcode_ptr+mcode_size, 0, (u8)(mcode_size-mLoopStart0));
	mcode_size += msize;

	if ((u8)dma_param.mPeriNum == MSM_REQ0 || \
			(u8)dma_param.mPeriNum == MSM_REQ1 || \
			(u8)dma_param.mPeriNum == MSM_REQ2 || \
			(u8)dma_param.mPeriNum == MSM_REQ3) {
			msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
			}

	return mcode_size;
}

/* config_DMA_transfer_size
 * - set the transfer size
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMA micro code to be stored into
 *	dma_param	the parameter set for a DMA operation
 */
static int config_DMA_transfer_size(u8 *mcode_ptr, pl330_DMA_parameters_t dma_param)
{
	int mcode_size = 0, msize = 0;
	int lc0 = 0, lc1 = 0, lcRemainder = 0, lcSize = 0;
	int mLoopStart0 = 0, mLoopStart1 = 0;

	dma_debug("%s entered \n", __func__);

	switch (dma_param.mDirection) {
	case PL330_M2M_DMA:
		if (dma_param.mTrSize > (8*1024*1024)) {
			print_warning("[%s] The chunk size is too big !: %lu\n",
					__func__, dma_param.mTrSize);
			return 0;
		}
		break;

	case PL330_M2P_DMA:
	case PL330_P2M_DMA:
		if (dma_param.mTrSize > (2*1024*1024)) {
			print_warning("[%s] The chunk size is too big !: %lu\n",
					__func__, dma_param.mTrSize);
			return 0;
		}
		break;

	case PL330_P2P_DMA:
		print_warning("[%s] P2P DMA selected !\n", __func__);
		break;

	default:
		print_warning("[%s] Invaild DMA direction entered !\n", __func__);
		break;
	}

	lcSize = (dma_param.mControl.uSBLength+1)*(1<<dma_param.mControl.uSBSize);
	lc0 = dma_param.mTrSize/lcSize;
	lcRemainder = dma_param.mTrSize - (lc0*lcSize);
	dma_debug("lcSize=%d,  lc0=%d,  lcRemainder=%d\n", lcSize, lc0, lcRemainder);

	if (lc0 > PL330_MAX_ITERATION_NUM) {
		lc1 = lc0/PL330_MAX_ITERATION_NUM;
		dma_debug("  Inner loop : lc1=%d\n", lc1);

		if (lc1 <= PL330_MAX_ITERATION_NUM) {
			msize = encodeDmaLoop(mcode_ptr+mcode_size, 1, lc1-1);
			mcode_size += msize;
			mLoopStart1 = mcode_size;

			msize = encodeDmaLoop(mcode_ptr+mcode_size, 0, PL330_MAX_ITERATION_NUM-1);
			mcode_size += msize;
			mLoopStart0 = mcode_size;

			switch (dma_param.mDirection) {
			case PL330_M2M_DMA:
				msize = encodeDmaLoad(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaReadMemBarrier(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaStore(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaWriteMemBarrier(mcode_ptr+mcode_size);
				mcode_size += msize;
				break;

			case PL330_M2P_DMA:
				msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
				if ((u8)dma_param.mPeriNum == MSM_REQ0 || (u8)dma_param.mPeriNum == MSM_REQ1) {
					msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
					mcode_size += msize;
					msize = encodeDmaLoad(mcode_ptr+mcode_size);
					mcode_size += msize;
					msize = encodeDmaStorePeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum, 1);
					mcode_size += msize;
				} else {
					msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
					mcode_size += msize;
					msize = encodeDmaLoad(mcode_ptr+mcode_size);
					mcode_size += msize;
					msize = encodeDmaStore(mcode_ptr+mcode_size);
					mcode_size += msize;
					msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
					mcode_size += msize;
				}
				break;

			case PL330_P2M_DMA:
				msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
				if ((u8)dma_param.mPeriNum == MSM_REQ2 || (u8)dma_param.mPeriNum == MSM_REQ3) {
					msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
					mcode_size += msize;
					msize = encodeDmaLoad(mcode_ptr+mcode_size);
					mcode_size += msize;
					msize = encodeDmaStore(mcode_ptr+mcode_size);
					mcode_size += msize;
				} else {
					msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
					mcode_size += msize;
					msize = encodeDmaLoadPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum, 1);
					mcode_size += msize;
					msize = encodeDmaStore(mcode_ptr+mcode_size);
					mcode_size += msize;
					msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
					mcode_size += msize;
				}
				break;

			case PL330_P2P_DMA:
				print_warning("[%s] P2P DMA selected !\n",
						__func__);
				break;

			default:
				print_warning("[%s] Invaild DMA direction selected !\n",
						__func__);
				break;
			}

			msize = encodeDmaLoopEnd(mcode_ptr+mcode_size, 0, (u8)(mcode_size-mLoopStart0));
			mcode_size += msize;

			msize = encodeDmaLoopEnd(mcode_ptr+mcode_size, 1, (u8)(mcode_size-mLoopStart1));
			mcode_size += msize;

			if ((u8)dma_param.mPeriNum == MSM_REQ0 || \
					(u8)dma_param.mPeriNum == MSM_REQ1 || \
					(u8)dma_param.mPeriNum == MSM_REQ2 || \
					(u8)dma_param.mPeriNum == MSM_REQ3) {
					msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
					mcode_size += msize;
				}

			lc0 = lc0 - (lc1*PL330_MAX_ITERATION_NUM);
		} else {
			print_warning("[%s] The transfer size is over the limit (lc1=%d)\n",
					__func__, lc1);
		}
	}

	if (lc0 > 0) {
		dma_debug("Single loop : lc0=%d\n", lc0);
		msize = encodeDmaLoop(mcode_ptr+mcode_size, 0, lc0-1);
		mcode_size += msize;
		mLoopStart0 = mcode_size;

		switch (dma_param.mDirection) {
		case PL330_M2M_DMA:
			msize = encodeDmaLoad(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaReadMemBarrier(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaStore(mcode_ptr+mcode_size);
			mcode_size += msize;
			msize = encodeDmaWriteMemBarrier(mcode_ptr+mcode_size);
			mcode_size += msize;
			break;

		case PL330_M2P_DMA:
			msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
			if ((u8)dma_param.mPeriNum == MSM_REQ0 || (u8)dma_param.mPeriNum == MSM_REQ1) {
				msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
				msize = encodeDmaLoad(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaStorePeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum, 1);
				mcode_size += msize;
			} else {
				msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
				msize = encodeDmaLoad(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaStore(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
			}
			break;

		case PL330_P2M_DMA:
			msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
			mcode_size += msize;
			if ((u8)dma_param.mPeriNum == MSM_REQ2 || (u8)dma_param.mPeriNum == MSM_REQ3) {
				msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
				msize = encodeDmaLoad(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaStore(mcode_ptr+mcode_size);
				mcode_size += msize;
			} else {
				msize = encodeDmaWaitForPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
				msize = encodeDmaLoadPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum, 1);
				mcode_size += msize;
				msize = encodeDmaStore(mcode_ptr+mcode_size);
				mcode_size += msize;
				msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
			}
			break;

		case PL330_P2P_DMA:
			print_warning("[%s] P2P DMA selected !\n", __func__);
			break;

		default:
			break;
		}

		msize = encodeDmaLoopEnd(mcode_ptr+mcode_size, 0, (u8)(mcode_size-mLoopStart0));
		mcode_size += msize;

		if ((u8)dma_param.mPeriNum == MSM_REQ0 || \
				(u8)dma_param.mPeriNum == MSM_REQ1 || \
				(u8)dma_param.mPeriNum == MSM_REQ2 || \
				(u8)dma_param.mPeriNum == MSM_REQ3) {
				msize = encodeDmaFlushPeri(mcode_ptr+mcode_size, (u8)dma_param.mPeriNum);
				mcode_size += msize;
				}
	}

	if (lcRemainder != 0) {
		msize = config_DMA_transfer_remainder(mcode_ptr+mcode_size, lcRemainder, dma_param);
		mcode_size += msize;
	}

	return mcode_size;
}

/* config_DMA_transfer_size_for_oneNAND
 * - set the transfer size
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMA micro code to be stored into
 *	dma_param	the parameter set for a DMA operation
 */
#define ONENAND_PAGE_SIZE	2048
#define ONENAND_OOB_SIZE	64
#define ONENAND_PAGE_WITH_OOB	(ONENAND_PAGE_SIZE+ONENAND_OOB_SIZE)
#define MAX_ONENAND_PAGE_CNT	64

static int config_DMA_transfer_size_for_oneNAND(u8 *mcode_ptr, pl330_DMA_parameters_t dma_param)
{
	int i = 0, pageCnt = 0;
	int mcode_size = 0, msize = 0;
	int lc0 = 0, lcSize = 0;
	int mLoopStart = 0;

	dma_debug("%s entered\n", __func__);

	if (dma_param.mTrSize > (MAX_ONENAND_PAGE_CNT*ONENAND_PAGE_WITH_OOB)) {
		print_warning("[%s] The chunk size is too big !: %lu\n",
				__func__, dma_param.mTrSize);
		return 0;
	}

	/* Buffer address on SDRAM */
	switch (dma_param.mDirection) {
	case PL330_M2P_DMA:		/* Write into oneNAND */
		msize = config_DMA_start_address(mcode_ptr+mcode_size, dma_param.mSrcAddr);
		mcode_size += msize;
		break;

	case PL330_P2M_DMA:		/* Read from oneNAND */
		msize = config_DMA_destination_address(mcode_ptr+mcode_size, dma_param.mDstAddr);
		mcode_size += msize;
		break;

	case PL330_P2P_DMA:
	case PL330_M2M_DMA:
	default:
		print_warning("[%s] Invaild DMA direction selected !\n",
				__func__);
		break;
	}

	lcSize = (dma_param.mControl.uSBLength+1)*(1<<dma_param.mControl.uSBSize);	/* 16 bursts * 4 bytes = 64 bytes */
	lc0 = ONENAND_PAGE_WITH_OOB/lcSize;						/* 2114(1 page+oob)/64 	 	  */
	pageCnt = dma_param.mTrSize/ONENAND_PAGE_WITH_OOB;				/* mTrsize/2112	 	  	  */
	dma_debug("lcSize=%d,  lc0=%d, No. of pages=%d\n", lcSize, lc0, pageCnt);

	for (i = 0; i < pageCnt; i++) {

		msize = encodeDmaLoop(mcode_ptr+mcode_size, 0, lc0-1);
		mcode_size += msize;
		mLoopStart = mcode_size;

		/* OneNAND address */
		switch (dma_param.mDirection) {
		case PL330_M2P_DMA:		/* Write into oneNAND */
			msize = config_DMA_destination_address(mcode_ptr+mcode_size, dma_param.mDstAddr+(i*0x80));
			mcode_size += msize;
			break;

		case PL330_P2M_DMA:		/* Read from oneNAND */
			msize = config_DMA_start_address(mcode_ptr+mcode_size, dma_param.mSrcAddr+(i*0x80));
			mcode_size += msize;
			break;

		case PL330_P2P_DMA:
		case PL330_M2M_DMA:
		default:
			print_warning("[%s] Invaild DMA direction selected !\n",
					__func__);
			break;
		}

		msize = encodeDmaLoad(mcode_ptr+mcode_size);
		mcode_size += msize;
		msize = encodeDmaStore(mcode_ptr+mcode_size);
		mcode_size += msize;

		msize = encodeDmaLoopEnd(mcode_ptr+mcode_size, 0, (u8)(mcode_size-mLoopStart));
		mcode_size += msize;

	}

	return mcode_size;
}

/* register_irq_to_DMA_channel
 * - register an event with the DMA channel
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMASEV micro code to be stored into
 *	uEventNum	the event number to be assigned to this DMA channel
 */
static int register_irq_to_DMA_channel(u8 *mcode_ptr, int uEventNum)
{
	dma_debug("%s entered - Event num : %d\n",
			__func__, uEventNum);
	return encodeDmaSendEvent(mcode_ptr, (u8)uEventNum);
}

/* config_DMA_set_infinite_loop
 * - set an infinite loop
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMAPLPEND micro code to be stored into
 *	bBwJump		the relative location of the first instruction in the program loop
 */
static int config_DMA_set_infinite_loop(u8 *mcode_ptr, int uBwJump)
{
	dma_debug("%s entered - Backward jump offset : %d\n",
			__func__, uBwJump);
	return encodeDmaLoopForever(mcode_ptr, (u8)uBwJump);
}

/* config_DMA_mark_end
 * - mark the end of the DMA request
 *
 *	mcode_ptr	the pointer to the buffer for PL330 DMAEND micro code to be stored into
 */
static int config_DMA_mark_end(u8 *mcode_ptr)
{
	dma_debug("%s entered \n\n", __func__);
	return encodeDmaEnd(mcode_ptr);
}

/*----------------------------------------------------------*/
/*                       DMA Feature Functions              */
/*----------------------------------------------------------*/

/* start_DMA_controller
 * - start the DMA controller
 */
void start_DMA_controller(u32 *mbuf)
{
	dma_debug("%s entered - mbuf=0x%p\n", __func__, mbuf);
	return;
}

/* stop_DMA_controller
 * - stop the DMA controller
 *
 *	mbuf		the address of the buffer for DMAKILL micro code to be stored at
 */
void stop_DMA_controller(u32 *mbuf)
{
	dma_debug("%s entered - mbuf=0x%p\n", __func__, mbuf);
	config_DMA_stop_controller(mbuf);
}

/* setup_DMA_channel
 * - set up a DMA channel for the DMA operation
 *
 *	mbuf		the address of the buffer that will contain PL330 DMA micro codes
 *	dma_param	the parameter set for a DMA operation
 *	chanNum		the DMA channel number to be started
 */
int setup_DMA_channel(u8 *mbuf, pl330_DMA_parameters_t dma_param, int chanNum)
{
	int mcode_size = 0, msize = 0;
	dma_debug("%s entered : Channel Num=%d\n", __func__, chanNum);
	print_dma_param_info(dma_param);

	msize = config_DMA_start_address(mbuf+mcode_size, dma_param.mSrcAddr);
	mcode_size += msize;

	msize = config_DMA_destination_address(mbuf+mcode_size, dma_param.mDstAddr);
	mcode_size += msize;

	msize = config_DMA_control(mbuf+mcode_size, dma_param.mControl);
	mcode_size += msize;

	msize = config_DMA_transfer_size(mbuf+mcode_size, dma_param);
	mcode_size += msize;

	if (dma_param.mIrqEnable) {
		msize = register_irq_to_DMA_channel(mbuf+mcode_size, chanNum);
		mcode_size += msize;
	}

	if (dma_param.mLoop) {
		msize = config_DMA_set_infinite_loop(mbuf+mcode_size, dma_param.mBwJump+mcode_size);
		mcode_size += msize;
	}

	if (dma_param.mLastReq) {
		msize = config_DMA_mark_end(mbuf+mcode_size);
		mcode_size += msize;
	}

	return mcode_size;
}

/* setup_DMA_channel for oneNAND
 * - set up a DMA channel for the oneNAND DMA operation
 *
 *	mbuf		the address of the buffer that will contain PL330 DMA micro codes
 *	dma_param	the parameter set for a DMA operation
 *	chanNum		the DMA channel number to be started
 */
int setup_DMA_channel_for_oneNAND(u8 *mbuf, pl330_DMA_parameters_t dma_param, int chanNum)
{
	int mcode_size = 0, msize = 0;
	dma_debug("%s entered : Channel Num=%d\n", __func__, chanNum);
	print_dma_param_info(dma_param);

	msize = config_DMA_control(mbuf+mcode_size, dma_param.mControl);
	mcode_size += msize;

	msize = config_DMA_transfer_size_for_oneNAND(mbuf+mcode_size, dma_param);
	mcode_size += msize;

	if (dma_param.mIrqEnable) {
		msize = register_irq_to_DMA_channel(mbuf+mcode_size, chanNum);
		mcode_size += msize;
	}

	if (dma_param.mLastReq) {
		msize = config_DMA_mark_end(mbuf+mcode_size);
		mcode_size += msize;
	}

	return mcode_size;
}

/* start_DMA_channel
 * - get the DMA channel started
 *
 *	mbuf		the address of the buffer for DMAGO micro code to be stored at
 *	chanNum		the DMA channel number to be started
 *	mbufAddr	the start address of the buffer containing PL330 DMA micro codes
 */
void start_DMA_channel(u32 *mbuf, int chanNum, u32 mbufAddr, int secureMode)
{
	dma_debug("%s entered - channel Num=%d\n", __func__, chanNum);
	config_DMA_GO_command(mbuf, chanNum, mbufAddr, secureMode);
}

/* stop_DMA_channel
 * - get the DMA channel stopped
 *	mbuf		the address of the buffer for DMAKILL micro code to be stored at
 *	chanNum		the DMA channel number to be stopped
 */
void stop_DMA_channel(u32 *mbuf, int chanNum)
{
	dma_debug("%s entered - channel Num=%d\n", __func__, chanNum);
	config_DMA_stop_channel(mbuf, chanNum);
}
