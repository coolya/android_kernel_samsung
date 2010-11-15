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

#if !defined (__SGX_MKIF_KM_H__)
#define __SGX_MKIF_KM_H__

#include "img_types.h"
#include "servicesint.h"
#include "sgxapi_km.h"


#if defined(SGX_FEATURE_MP)
	#define SGX_REG_BANK_SHIFT 			(12)
	#define SGX_REG_BANK_SIZE 			(0x4000)
	#if defined(SGX541)
		#define SGX_REG_BANK_BASE_INDEX		(1)
		#define	SGX_REG_BANK_MASTER_INDEX	(SGX_REG_BANK_BASE_INDEX + SGX_FEATURE_MP_CORE_COUNT)
	#else
		#define SGX_REG_BANK_BASE_INDEX		(2)
		#define	SGX_REG_BANK_MASTER_INDEX	(1)
	#endif 
	#define SGX_MP_CORE_SELECT(x,i) 	(x + ((i + SGX_REG_BANK_BASE_INDEX) * SGX_REG_BANK_SIZE))
	#define SGX_MP_MASTER_SELECT(x) 	(x + (SGX_REG_BANK_MASTER_INDEX * SGX_REG_BANK_SIZE))
#else
	#define SGX_MP_CORE_SELECT(x,i) 	(x)
#endif 


typedef struct _SGXMKIF_COMMAND_
{
	IMG_UINT32				ui32ServiceAddress;		
	IMG_UINT32				ui32CacheControl;		
	IMG_UINT32				ui32Data[4];			
	IMG_UINT32				ui32Padding[2];			
} SGXMKIF_COMMAND;


typedef struct _PVRSRV_SGX_KERNEL_CCB_
{
	SGXMKIF_COMMAND		asCommands[256];		
} PVRSRV_SGX_KERNEL_CCB;


typedef struct _PVRSRV_SGX_CCB_CTL_
{
	IMG_UINT32				ui32WriteOffset;		
	IMG_UINT32				ui32ReadOffset;			
} PVRSRV_SGX_CCB_CTL;


typedef struct _SGXMKIF_HOST_CTL_
{
#if defined(PVRSRV_USSE_EDM_BREAKPOINTS)
	IMG_UINT32				ui32BreakpointDisable;
	IMG_UINT32				ui32Continue;
#endif

	volatile IMG_UINT32		ui32InitStatus;				
	volatile IMG_UINT32		ui32PowerStatus;			
	volatile IMG_UINT32		ui32CleanupStatus;			
#if defined(FIX_HW_BRN_28889)
	volatile IMG_UINT32		ui32InvalStatus;			
#endif
#if defined(SUPPORT_HW_RECOVERY)
	IMG_UINT32				ui32uKernelDetectedLockups;	
	IMG_UINT32				ui32HostDetectedLockups;	
	IMG_UINT32				ui32HWRecoverySampleRate;	
#endif 
	IMG_UINT32				ui32uKernelTimerClock;		
	IMG_UINT32				ui32ActivePowManSampleRate;	
	IMG_UINT32				ui32InterruptFlags; 		
	IMG_UINT32				ui32InterruptClearFlags; 	
	IMG_UINT32				ui32BPSetClearSignal;		

	IMG_UINT32				ui32NumActivePowerEvents;	

	IMG_UINT32				ui32TimeWraps;				
	IMG_UINT32				ui32HostClock;				

#if defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
	IMG_UINT32				aui32PerfGroup[PVRSRV_SGX_HWPERF_NUM_COUNTERS];	
	IMG_UINT32				aui32PerfBit[PVRSRV_SGX_HWPERF_NUM_COUNTERS];	
#else
	IMG_UINT32				ui32PerfGroup;									
#endif 
} SGXMKIF_HOST_CTL;

#define	SGXMKIF_CMDTA_CTRLFLAGS_READY			0x00000001
typedef struct _SGXMKIF_CMDTA_SHARED_
{
	IMG_UINT32			ui32CtrlFlags;
	
	IMG_UINT32			ui32NumTAStatusVals;
	IMG_UINT32			ui32Num3DStatusVals;

	
	IMG_UINT32			ui32TATQSyncWriteOpsPendingVal;
	IMG_DEV_VIRTADDR	sTATQSyncWriteOpsCompleteDevVAddr;
	IMG_UINT32			ui32TATQSyncReadOpsPendingVal;
	IMG_DEV_VIRTADDR	sTATQSyncReadOpsCompleteDevVAddr;

	
	IMG_UINT32			ui323DTQSyncWriteOpsPendingVal;
	IMG_DEV_VIRTADDR	s3DTQSyncWriteOpsCompleteDevVAddr;
	IMG_UINT32			ui323DTQSyncReadOpsPendingVal;
	IMG_DEV_VIRTADDR	s3DTQSyncReadOpsCompleteDevVAddr;


#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	
	IMG_UINT32					ui32NumTASrcSyncs;
	PVRSRV_DEVICE_SYNC_OBJECT	asTASrcSyncs[SGX_MAX_TA_SRC_SYNCS];
	IMG_UINT32					ui32NumTADstSyncs;
	PVRSRV_DEVICE_SYNC_OBJECT	asTADstSyncs[SGX_MAX_TA_DST_SYNCS];
	IMG_UINT32					ui32Num3DSrcSyncs;
	PVRSRV_DEVICE_SYNC_OBJECT	as3DSrcSyncs[SGX_MAX_3D_SRC_SYNCS];
#else
	
	IMG_UINT32			ui32NumSrcSyncs;
	PVRSRV_DEVICE_SYNC_OBJECT	asSrcSyncs[SGX_MAX_SRC_SYNCS];
#endif

	
	PVRSRV_DEVICE_SYNC_OBJECT	sTA3DDependency;

	CTL_STATUS			sCtlTAStatusInfo[SGX_MAX_TA_STATUS_VALS];
	CTL_STATUS			sCtl3DStatusInfo[SGX_MAX_3D_STATUS_VALS];

} SGXMKIF_CMDTA_SHARED;

#define SGXTQ_MAX_STATUS						SGX_MAX_TRANSFER_STATUS_VALS + 2

#define SGXMKIF_TQFLAGS_NOSYNCUPDATE			0x00000001
#define SGXMKIF_TQFLAGS_KEEPPENDING				0x00000002
#define SGXMKIF_TQFLAGS_TATQ_SYNC				0x00000004
#define SGXMKIF_TQFLAGS_3DTQ_SYNC				0x00000008
#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
#define SGXMKIF_TQFLAGS_CTXSWITCH				0x00000010
#endif
#define SGXMKIF_TQFLAGS_DUMMYTRANSFER			0x00000020

typedef struct _SGXMKIF_TRANSFERCMD_SHARED_
{
	

 	IMG_UINT32			ui32NumSrcSyncs;
 	PVRSRV_DEVICE_SYNC_OBJECT	asSrcSyncs[SGX_MAX_SRC_SYNCS];
	

 	IMG_UINT32			ui32NumDstSyncs;
 	PVRSRV_DEVICE_SYNC_OBJECT	asDstSyncs[SGX_MAX_DST_SYNCS];	
	
	IMG_UINT32		ui32DstReadOpPendingVal;
	IMG_DEV_VIRTADDR	sDstReadOpsCompleteDevAddr;
	
	IMG_UINT32		ui32DstWriteOpPendingVal;
	IMG_DEV_VIRTADDR	sDstWriteOpsCompleteDevAddr;

	
	IMG_UINT32		ui32TASyncWriteOpsPendingVal;
	IMG_DEV_VIRTADDR	sTASyncWriteOpsCompleteDevVAddr;
	IMG_UINT32		ui32TASyncReadOpsPendingVal;
	IMG_DEV_VIRTADDR	sTASyncReadOpsCompleteDevVAddr;

	
	IMG_UINT32		ui323DSyncWriteOpsPendingVal;
	IMG_DEV_VIRTADDR	s3DSyncWriteOpsCompleteDevVAddr;
	IMG_UINT32		ui323DSyncReadOpsPendingVal;
	IMG_DEV_VIRTADDR	s3DSyncReadOpsCompleteDevVAddr;

	IMG_UINT32 		ui32NumStatusVals;
	CTL_STATUS  	sCtlStatusInfo[SGXTQ_MAX_STATUS];
} SGXMKIF_TRANSFERCMD_SHARED, *PSGXMKIF_TRANSFERCMD_SHARED;


#if defined(SGX_FEATURE_2D_HARDWARE)
typedef struct _SGXMKIF_2DCMD_SHARED_ {
	
	IMG_UINT32			ui32NumSrcSync;
	PVRSRV_DEVICE_SYNC_OBJECT	sSrcSyncData[SGX_MAX_2D_SRC_SYNC_OPS];

	
	PVRSRV_DEVICE_SYNC_OBJECT	sDstSyncData;

	
	PVRSRV_DEVICE_SYNC_OBJECT	sTASyncData;

	
	PVRSRV_DEVICE_SYNC_OBJECT	s3DSyncData;
} SGXMKIF_2DCMD_SHARED, *PSGXMKIF_2DCMD_SHARED;
#endif 


typedef struct _SGXMKIF_HWDEVICE_SYNC_LIST_
{
	IMG_DEV_VIRTADDR	sAccessDevAddr;
	IMG_UINT32			ui32NumSyncObjects;
	
	PVRSRV_DEVICE_SYNC_OBJECT	asSyncData[1];
} SGXMKIF_HWDEVICE_SYNC_LIST, *PSGXMKIF_HWDEVICE_SYNC_LIST;


#define PVRSRV_USSE_EDM_INIT_COMPLETE			(1UL << 0)	

#define PVRSRV_USSE_EDM_POWMAN_IDLE_COMPLETE				(1UL << 2)	
#define PVRSRV_USSE_EDM_POWMAN_POWEROFF_COMPLETE			(1UL << 3)	
#define PVRSRV_USSE_EDM_POWMAN_POWEROFF_RESTART_IMMEDIATE	(1UL << 4)	
#define PVRSRV_USSE_EDM_POWMAN_NO_WORK						(1UL << 5)	

#define PVRSRV_USSE_EDM_INTERRUPT_HWR			(1UL << 0)	
#define PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER	(1UL << 1)	

#define PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE 	(1UL << 0)	

#if defined(FIX_HW_BRN_28889)
#define PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE 		(1UL << 0)	
#endif

#define PVRSRV_USSE_MISCINFO_READY		0x1UL
#define PVRSRV_USSE_MISCINFO_GET_STRUCT_SIZES	0x2UL	
#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
#define PVRSRV_USSE_MISCINFO_MEMREAD			0x4UL	
#define PVRSRV_USSE_MISCINFO_MEMWRITE			0x8UL	
#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
#define PVRSRV_USSE_MISCINFO_MEMREAD_FAIL		0x1UL << 31	
#endif
#endif


#define	PVRSRV_CLEANUPCMD_RT		0x1
#define	PVRSRV_CLEANUPCMD_RC		0x2
#define	PVRSRV_CLEANUPCMD_TC		0x3
#define	PVRSRV_CLEANUPCMD_2DC		0x4
#define	PVRSRV_CLEANUPCMD_PB		0x5

#define PVRSRV_POWERCMD_POWEROFF	0x1
#define PVRSRV_POWERCMD_IDLE		0x2
#define PVRSRV_POWERCMD_RESUME		0x3


#if defined(SGX_FEATURE_BIF_NUM_DIRLISTS)
#define SGX_BIF_DIR_LIST_INDEX_EDM	(SGX_FEATURE_BIF_NUM_DIRLISTS - 1)
#else
#define SGX_BIF_DIR_LIST_INDEX_EDM	(0)
#endif

#define	SGXMKIF_CC_INVAL_BIF_PT	0x1
#define	SGXMKIF_CC_INVAL_BIF_PD	0x2
#define SGXMKIF_CC_INVAL_BIF_SL	0x4
#define SGXMKIF_CC_INVAL_DATA	0x8


typedef struct _SGX_MISCINFO_STRUCT_SIZES_
{
#if defined (SGX_FEATURE_2D_HARDWARE)
	IMG_UINT32	ui32Sizeof_2DCMD;
	IMG_UINT32	ui32Sizeof_2DCMD_SHARED;
#endif
	IMG_UINT32	ui32Sizeof_CMDTA;
	IMG_UINT32	ui32Sizeof_CMDTA_SHARED;
	IMG_UINT32	ui32Sizeof_TRANSFERCMD;
	IMG_UINT32	ui32Sizeof_TRANSFERCMD_SHARED;
	IMG_UINT32	ui32Sizeof_3DREGISTERS;
	IMG_UINT32	ui32Sizeof_HWPBDESC;
	IMG_UINT32	ui32Sizeof_HWRENDERCONTEXT;
	IMG_UINT32	ui32Sizeof_HWRENDERDETAILS;
	IMG_UINT32	ui32Sizeof_HWRTDATA;
	IMG_UINT32	ui32Sizeof_HWRTDATASET;
	IMG_UINT32	ui32Sizeof_HWTRANSFERCONTEXT;
	IMG_UINT32	ui32Sizeof_HOST_CTL;
	IMG_UINT32	ui32Sizeof_COMMAND;
} SGX_MISCINFO_STRUCT_SIZES;


#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
typedef struct _PVRSRV_SGX_MISCINFO_MEMACCESS
{
	IMG_DEV_VIRTADDR	sDevVAddr;		
	IMG_DEV_PHYADDR		sPDDevPAddr;	
} PVRSRV_SGX_MISCINFO_MEMACCESS;
#endif

typedef struct _PVRSRV_SGX_MISCINFO_INFO
{
	IMG_UINT32						ui32MiscInfoFlags;
	PVRSRV_SGX_MISCINFO_FEATURES	sSGXFeatures;		
	SGX_MISCINFO_STRUCT_SIZES		sSGXStructSizes;	
#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
	PVRSRV_SGX_MISCINFO_MEMACCESS	sSGXMemAccessSrc;	
	PVRSRV_SGX_MISCINFO_MEMACCESS	sSGXMemAccessDest;	
#endif
} PVRSRV_SGX_MISCINFO_INFO;

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
#define SGXMK_TRACE_BUFFER_SIZE 512
#endif 

#define SGXMKIF_HWPERF_CB_SIZE					0x100	

typedef struct _SGXMKIF_HWPERF_CB_ENTRY_
{
	IMG_UINT32	ui32FrameNo;
	IMG_UINT32	ui32Type;
	IMG_UINT32	ui32Ordinal;
	IMG_UINT32	ui32Info;
	IMG_UINT32	ui32TimeWraps;
	IMG_UINT32	ui32Time;
	IMG_UINT32	ui32Counters[SGX_FEATURE_MP_CORE_COUNT][PVRSRV_SGX_HWPERF_NUM_COUNTERS];
} SGXMKIF_HWPERF_CB_ENTRY;

typedef struct _SGXMKIF_HWPERF_CB_
{
	IMG_UINT32				ui32Woff;
	IMG_UINT32				ui32Roff;
	IMG_UINT32				ui32Ordinal;
	SGXMKIF_HWPERF_CB_ENTRY psHWPerfCBData[SGXMKIF_HWPERF_CB_SIZE];
} SGXMKIF_HWPERF_CB;


#endif 

