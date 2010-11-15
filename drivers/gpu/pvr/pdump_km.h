/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#ifndef _PDUMP_KM_H_
#define _PDUMP_KM_H_


#include "pdump_osfunc.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "pdump.h"

#define PDUMP_PD_UNIQUETAG			(IMG_HANDLE)0
#define PDUMP_PT_UNIQUETAG			(IMG_HANDLE)0

#define PDUMP_STREAM_PARAM2			0
#define PDUMP_STREAM_SCRIPT2		1
#define PDUMP_STREAM_DRIVERINFO		2
#define PDUMP_NUM_STREAMS			3

#if defined(PDUMP_DEBUG_OUTFILES)
extern IMG_UINT32 g_ui32EveryLineCounter;
#endif

#ifndef PDUMP
#define MAKEUNIQUETAG(hMemInfo)	(0)
#endif

#ifdef PDUMP

#define MAKEUNIQUETAG(hMemInfo)	(((BM_BUF *)(((PVRSRV_KERNEL_MEM_INFO *)(hMemInfo))->sMemBlk.hBuffer))->pMapping)

	IMG_IMPORT PVRSRV_ERROR PDumpMemPolKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo,
										  IMG_UINT32			ui32Offset,
										  IMG_UINT32			ui32Value,
										  IMG_UINT32			ui32Mask,
										  PDUMP_POLL_OPERATOR	eOperator,
										  IMG_UINT32			ui32Flags,
										  IMG_HANDLE			hUniqueTag);

	IMG_IMPORT PVRSRV_ERROR PDumpMemUM(PVRSRV_PER_PROCESS_DATA *psProcData,
									   IMG_PVOID			pvAltLinAddr,
									   IMG_PVOID			pvLinAddr,
									   PVRSRV_KERNEL_MEM_INFO	*psMemInfo,
									   IMG_UINT32			ui32Offset,
									   IMG_UINT32			ui32Bytes,
									   IMG_UINT32			ui32Flags,
									   IMG_HANDLE			hUniqueTag);

	IMG_IMPORT PVRSRV_ERROR PDumpMemKM(IMG_PVOID			pvAltLinAddr,
									   PVRSRV_KERNEL_MEM_INFO	*psMemInfo,
									   IMG_UINT32			ui32Offset,
									   IMG_UINT32			ui32Bytes,
									   IMG_UINT32			ui32Flags,
									   IMG_HANDLE			hUniqueTag);
	PVRSRV_ERROR PDumpMemPagesKM(PVRSRV_DEVICE_TYPE	eDeviceType,
								 IMG_DEV_PHYADDR		*pPages,
								 IMG_UINT32			ui32NumPages,
								 IMG_DEV_VIRTADDR	sDevAddr,
								 IMG_UINT32			ui32Start,
								 IMG_UINT32			ui32Length,
								 IMG_UINT32			ui32Flags,
								 IMG_HANDLE			hUniqueTag);

	PVRSRV_ERROR PDumpMemPDEntriesKM(PDUMP_MMU_ATTRIB *psMMUAttrib,
									 IMG_HANDLE hOSMemHandle,
							 		 IMG_CPU_VIRTADDR pvLinAddr,
							 		 IMG_UINT32 ui32Bytes,
							 		 IMG_UINT32 ui32Flags,
							 		 IMG_BOOL bInitialisePages,
							 		 IMG_HANDLE hUniqueTag1,
							 		 IMG_HANDLE hUniqueTag2);
							 
	PVRSRV_ERROR PDumpMemPTEntriesKM(PDUMP_MMU_ATTRIB *psMMUAttrib,
									 IMG_HANDLE         hOSMemHandle,
									 IMG_CPU_VIRTADDR	pvLinAddr,
									 IMG_UINT32			ui32Bytes,
									 IMG_UINT32			ui32Flags,
									 IMG_BOOL			bInitialisePages,
									 IMG_HANDLE			hUniqueTag1,
									 IMG_HANDLE			hUniqueTag2);
	IMG_VOID PDumpInitCommon(IMG_VOID);
	IMG_VOID PDumpDeInitCommon(IMG_VOID);
	IMG_VOID PDumpInit(IMG_VOID);
	IMG_VOID PDumpDeInit(IMG_VOID);
	IMG_BOOL PDumpIsSuspended(IMG_VOID);
	PVRSRV_ERROR PDumpStartInitPhaseKM(IMG_VOID);
	PVRSRV_ERROR PDumpStopInitPhaseKM(IMG_VOID);
	IMG_IMPORT PVRSRV_ERROR PDumpSetFrameKM(IMG_UINT32 ui32Frame);
	IMG_IMPORT PVRSRV_ERROR PDumpCommentKM(IMG_CHAR *pszComment, IMG_UINT32 ui32Flags);
	IMG_IMPORT PVRSRV_ERROR PDumpDriverInfoKM(IMG_CHAR *pszString, IMG_UINT32 ui32Flags);

	PVRSRV_ERROR PDumpRegWithFlagsKM(IMG_CHAR *pszPDumpRegName,
									 IMG_UINT32 ui32RegAddr,
									 IMG_UINT32 ui32RegValue,
									 IMG_UINT32 ui32Flags);
	PVRSRV_ERROR PDumpRegPolWithFlagsKM(IMG_CHAR *pszPDumpRegName,
										IMG_UINT32 ui32RegAddr,
										IMG_UINT32 ui32RegValue,
										IMG_UINT32 ui32Mask,
										IMG_UINT32 ui32Flags);
	PVRSRV_ERROR PDumpRegPolKM(IMG_CHAR *pszPDumpRegName,
								IMG_UINT32 ui32RegAddr,
								IMG_UINT32 ui32RegValue,
								IMG_UINT32 ui32Mask);

	IMG_IMPORT PVRSRV_ERROR PDumpBitmapKM(PVRSRV_DEVICE_NODE *psDeviceNode,
										  IMG_CHAR *pszFileName,
										  IMG_UINT32 ui32FileOffset,
										  IMG_UINT32 ui32Width,
										  IMG_UINT32 ui32Height,
										  IMG_UINT32 ui32StrideInBytes,
										  IMG_DEV_VIRTADDR sDevBaseAddr,
										  IMG_HANDLE hDevMemContext,
										  IMG_UINT32 ui32Size,
										  PDUMP_PIXEL_FORMAT ePixelFormat,
										  PDUMP_MEM_FORMAT eMemFormat,
										  IMG_UINT32 ui32PDumpFlags);
	IMG_IMPORT PVRSRV_ERROR PDumpReadRegKM(IMG_CHAR *pszPDumpRegName,
										   IMG_CHAR *pszFileName,
										   IMG_UINT32 ui32FileOffset,
										   IMG_UINT32 ui32Address,
										   IMG_UINT32 ui32Size,
										   IMG_UINT32 ui32PDumpFlags);

	PVRSRV_ERROR PDumpRegKM(IMG_CHAR* pszPDumpRegName,
							IMG_UINT32 dwReg,
							IMG_UINT32 dwData);

	PVRSRV_ERROR PDumpComment(IMG_CHAR* pszFormat, ...) IMG_FORMAT_PRINTF(1, 2);
	PVRSRV_ERROR PDumpCommentWithFlags(IMG_UINT32	ui32Flags,
									   IMG_CHAR*	pszFormat,
									   ...) IMG_FORMAT_PRINTF(2, 3);

	PVRSRV_ERROR PDumpPDReg(PDUMP_MMU_ATTRIB *psMMUAttrib,
							IMG_UINT32	ui32Reg,
							IMG_UINT32	ui32dwData,
							IMG_HANDLE	hUniqueTag);
	PVRSRV_ERROR PDumpPDRegWithFlags(PDUMP_MMU_ATTRIB *psMMUAttrib,
									 IMG_UINT32		ui32Reg,
									 IMG_UINT32		ui32Data,
									 IMG_UINT32		ui32Flags,
									 IMG_HANDLE		hUniqueTag);

	IMG_BOOL PDumpIsLastCaptureFrameKM(IMG_VOID);
	IMG_IMPORT IMG_BOOL PDumpIsCaptureFrameKM(IMG_VOID);

	IMG_VOID PDumpMallocPagesPhys(PVRSRV_DEVICE_TYPE	eDeviceType,
								  IMG_UINT32			ui32DevVAddr,
								  IMG_PUINT32			pui32PhysPages,
								  IMG_UINT32			ui32NumPages,
								  IMG_HANDLE			hUniqueTag);
	PVRSRV_ERROR PDumpSetMMUContext(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_CHAR *pszMemSpace,
									IMG_UINT32 *pui32MMUContextID,
									IMG_UINT32 ui32MMUType,
									IMG_HANDLE hUniqueTag1,
									IMG_HANDLE hOSMemHandle,
									IMG_VOID *pvPDCPUAddr);
	PVRSRV_ERROR PDumpClearMMUContext(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_CHAR *pszMemSpace,
									IMG_UINT32 ui32MMUContextID,
									IMG_UINT32 ui32MMUType);

	PVRSRV_ERROR PDumpPDDevPAddrKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo,
								   IMG_UINT32 ui32Offset,
								   IMG_DEV_PHYADDR sPDDevPAddr,
								   IMG_HANDLE hUniqueTag1,
								   IMG_HANDLE hUniqueTag2);

	IMG_BOOL PDumpTestNextFrame(IMG_UINT32 ui32CurrentFrame);

	PVRSRV_ERROR PDumpSaveMemKM (PVRSRV_DEVICE_IDENTIFIER *psDevId,
							 	 IMG_CHAR			*pszFileName,
								 IMG_UINT32			ui32FileOffset,
								 IMG_DEV_VIRTADDR	sDevBaseAddr,
								 IMG_UINT32 		ui32Size,
								 IMG_UINT32 		ui32DataMaster,
								 IMG_UINT32 		ui32PDumpFlags);

	PVRSRV_ERROR PDumpTASignatureRegisters(PVRSRV_DEVICE_IDENTIFIER *psDevId,
								   IMG_UINT32	ui32DumpFrameNum,
								   IMG_UINT32	ui32TAKickCount,
								   IMG_BOOL		bLastFrame,
								   IMG_UINT32 *pui32Registers,
								   IMG_UINT32 ui32NumRegisters);

	PVRSRV_ERROR PDump3DSignatureRegisters(PVRSRV_DEVICE_IDENTIFIER *psDevId,
											IMG_UINT32 ui32DumpFrameNum,
											IMG_BOOL bLastFrame,
											IMG_UINT32 *pui32Registers,
											IMG_UINT32 ui32NumRegisters);

	PVRSRV_ERROR PDumpCounterRegisters(PVRSRV_DEVICE_IDENTIFIER *psDevId,
					IMG_UINT32 ui32DumpFrameNum,
					IMG_BOOL		bLastFrame,
					IMG_UINT32 *pui32Registers,
					IMG_UINT32 ui32NumRegisters);

	PVRSRV_ERROR PDumpRegRead(IMG_CHAR *pszPDumpRegName,
								const IMG_UINT32 dwRegOffset,
								IMG_UINT32	ui32Flags);

	PVRSRV_ERROR PDumpCycleCountRegRead(PVRSRV_DEVICE_IDENTIFIER *psDevId,
										const IMG_UINT32 dwRegOffset,
										IMG_BOOL bLastFrame);

	PVRSRV_ERROR PDumpIDLWithFlags(IMG_UINT32 ui32Clocks, IMG_UINT32 ui32Flags);
	PVRSRV_ERROR PDumpIDL(IMG_UINT32 ui32Clocks);

	PVRSRV_ERROR PDumpMallocPages(PVRSRV_DEVICE_IDENTIFIER	*psDevID,
								  IMG_UINT32				ui32DevVAddr,
								  IMG_CPU_VIRTADDR			pvLinAddr,
								  IMG_HANDLE				hOSMemHandle,
								  IMG_UINT32				ui32NumBytes,
								  IMG_UINT32				ui32PageSize,
		                          IMG_BOOL			  		bShared,
								  IMG_HANDLE				hUniqueTag);
	PVRSRV_ERROR PDumpMallocPageTable(PVRSRV_DEVICE_IDENTIFIER	*psDevId,
									  IMG_HANDLE            hOSMemHandle,
									  IMG_UINT32            ui32Offset,
									  IMG_CPU_VIRTADDR		pvLinAddr,
									  IMG_UINT32			ui32NumBytes,
									  IMG_UINT32			ui32Flags,
									  IMG_HANDLE			hUniqueTag);
	PVRSRV_ERROR PDumpFreePages(struct _BM_HEAP_	*psBMHeap,
							IMG_DEV_VIRTADDR	sDevVAddr,
							IMG_UINT32			ui32NumBytes,
							IMG_UINT32			ui32PageSize,
							IMG_HANDLE      	hUniqueTag,
							IMG_BOOL			bInterleaved);
	PVRSRV_ERROR PDumpFreePageTable(PVRSRV_DEVICE_IDENTIFIER *psDevID,
									IMG_HANDLE          hOSMemHandle,
									IMG_CPU_VIRTADDR	pvLinAddr,
									IMG_UINT32			ui32NumBytes,
									IMG_UINT32			ui32Flags,
									IMG_HANDLE			hUniqueTag);

	IMG_IMPORT PVRSRV_ERROR PDumpHWPerfCBKM(PVRSRV_DEVICE_IDENTIFIER *psDevId,
										IMG_CHAR			*pszFileName,
										IMG_UINT32			ui32FileOffset,
										IMG_DEV_VIRTADDR	sDevBaseAddr,
										IMG_UINT32 			ui32Size,
										IMG_UINT32 			ui32PDumpFlags);

	PVRSRV_ERROR PDumpSignatureBuffer(PVRSRV_DEVICE_IDENTIFIER *psDevId,
									  IMG_CHAR			*pszFileName,
									  IMG_CHAR			*pszBufferType,
									  IMG_UINT32		ui32FileOffset,
									  IMG_DEV_VIRTADDR	sDevBaseAddr,
									  IMG_UINT32 		ui32Size,
									  IMG_UINT32 		ui32PDumpFlags);

	PVRSRV_ERROR PDumpCBP(PPVRSRV_KERNEL_MEM_INFO	psROffMemInfo,
				  IMG_UINT32				ui32ROffOffset,
				  IMG_UINT32				ui32WPosVal,
				  IMG_UINT32				ui32PacketSize,
				  IMG_UINT32				ui32BufferSize,
				  IMG_UINT32				ui32Flags,
				  IMG_HANDLE				hUniqueTag);

	PVRSRV_ERROR PDumpRegBasedCBP(IMG_CHAR		*pszPDumpRegName,
								  IMG_UINT32	ui32RegOffset,
								  IMG_UINT32	ui32WPosVal,
								  IMG_UINT32	ui32PacketSize,
								  IMG_UINT32	ui32BufferSize,
								  IMG_UINT32	ui32Flags);

	IMG_VOID PDumpVGXMemToFile(IMG_CHAR *pszFileName,
							   IMG_UINT32 ui32FileOffset, 
							   PVRSRV_KERNEL_MEM_INFO *psMemInfo,
							   IMG_UINT32 uiAddr, 
							   IMG_UINT32 ui32Size,
							   IMG_UINT32 ui32PDumpFlags,
							   IMG_HANDLE hUniqueTag);

	IMG_VOID PDumpSuspendKM(IMG_VOID);
	IMG_VOID PDumpResumeKM(IMG_VOID);

	
	PVRSRV_ERROR PDumpStoreMemToFile(PDUMP_MMU_ATTRIB *psMMUAttrib,
							         IMG_CHAR *pszFileName,
									 IMG_UINT32 ui32FileOffset, 
									 PVRSRV_KERNEL_MEM_INFO *psMemInfo,
									 IMG_UINT32 uiAddr, 
									 IMG_UINT32 ui32Size,
									 IMG_UINT32 ui32PDumpFlags,
									 IMG_HANDLE hUniqueTag);

	#define PDUMPMEMPOL				PDumpMemPolKM
	#define PDUMPMEM				PDumpMemKM
	#define PDUMPMEMPTENTRIES		PDumpMemPTEntriesKM
	#define PDUMPPDENTRIES			PDumpMemPDEntriesKM
	#define PDUMPMEMUM				PDumpMemUM
	#define PDUMPINIT				PDumpInitCommon
	#define PDUMPDEINIT				PDumpDeInitCommon
	#define PDUMPISLASTFRAME		PDumpIsLastCaptureFrameKM
	#define PDUMPTESTFRAME			PDumpIsCaptureFrameKM
	#define PDUMPTESTNEXTFRAME		PDumpTestNextFrame
	#define PDUMPREGWITHFLAGS		PDumpRegWithFlagsKM
	#define PDUMPREG				PDumpRegKM
	#define PDUMPCOMMENT			PDumpComment
	#define PDUMPCOMMENTWITHFLAGS	PDumpCommentWithFlags
	#define PDUMPREGPOL				PDumpRegPolKM
	#define PDUMPREGPOLWITHFLAGS	PDumpRegPolWithFlagsKM
	#define PDUMPMALLOCPAGES		PDumpMallocPages
	#define PDUMPMALLOCPAGETABLE	PDumpMallocPageTable
	#define PDUMPSETMMUCONTEXT		PDumpSetMMUContext
	#define PDUMPCLEARMMUCONTEXT	PDumpClearMMUContext
	#define PDUMPFREEPAGES			PDumpFreePages
	#define PDUMPFREEPAGETABLE		PDumpFreePageTable
	#define PDUMPPDREG				PDumpPDReg
	#define PDUMPPDREGWITHFLAGS		PDumpPDRegWithFlags
	#define PDUMPCBP				PDumpCBP
	#define PDUMPREGBASEDCBP		PDumpRegBasedCBP
	#define PDUMPMALLOCPAGESPHYS	PDumpMallocPagesPhys
	#define PDUMPENDINITPHASE		PDumpStopInitPhaseKM
	#define PDUMPBITMAPKM			PDumpBitmapKM
	#define PDUMPDRIVERINFO			PDumpDriverInfoKM
	#define PDUMPIDLWITHFLAGS		PDumpIDLWithFlags
	#define PDUMPIDL				PDumpIDL
	#define PDUMPSUSPEND			PDumpSuspendKM
	#define PDUMPRESUME				PDumpResumeKM

#else
		#if ((defined(LINUX) || defined(GCC_IA32)) || defined(GCC_ARM))
			#define PDUMPMEMPOL(args...)
			#define PDUMPMEM(args...)
			#define PDUMPMEMPTENTRIES(args...)
			#define PDUMPPDENTRIES(args...)
			#define PDUMPMEMUM(args...)
			#define PDUMPINIT(args...)
			#define PDUMPDEINIT(args...)
			#define PDUMPISLASTFRAME(args...)
			#define PDUMPTESTFRAME(args...)
			#define PDUMPTESTNEXTFRAME(args...)
			#define PDUMPREGWITHFLAGS(args...)
			#define PDUMPREG(args...)
			#define PDUMPCOMMENT(args...)
			#define PDUMPREGPOL(args...)
			#define PDUMPREGPOLWITHFLAGS(args...)
			#define PDUMPMALLOCPAGES(args...)
			#define PDUMPMALLOCPAGETABLE(args...)
			#define PDUMPSETMMUCONTEXT(args...)
			#define PDUMPCLEARMMUCONTEXT(args...)
			#define PDUMPFREEPAGES(args...)
			#define PDUMPFREEPAGETABLE(args...)
			#define PDUMPPDREG(args...)
			#define PDUMPPDREGWITHFLAGS(args...)
			#define PDUMPSYNC(args...)
			#define PDUMPCOPYTOMEM(args...)
			#define PDUMPWRITE(args...)
			#define PDUMPCBP(args...)
			#define PDUMPREGBASEDCBP(args...)
			#define PDUMPCOMMENTWITHFLAGS(args...)
			#define PDUMPMALLOCPAGESPHYS(args...)
			#define PDUMPENDINITPHASE(args...)
			#define PDUMPMSVDXREG(args...)
			#define PDUMPMSVDXREGWRITE(args...)
			#define PDUMPMSVDXREGREAD(args...)
			#define PDUMPMSVDXPOLEQ(args...)
			#define PDUMPMSVDXPOL(args...)
			#define PDUMPBITMAPKM(args...)
			#define PDUMPDRIVERINFO(args...)
			#define PDUMPIDLWITHFLAGS(args...)
			#define PDUMPIDL(args...)
			#define PDUMPSUSPEND(args...)
			#define PDUMPRESUME(args...)
			#define PDUMPMSVDXWRITEREF(args...)
		#else
			#error Compiler not specified
		#endif
#endif

#if defined (__cplusplus)
}
#endif

#endif 

