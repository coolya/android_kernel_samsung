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

#if !defined (__SGXINFO_H__)
#define __SGXINFO_H__

#include "sgxscript.h"
#include "servicesint.h"
#include "services.h"
#include "sgxapi_km.h"
#include "sgx_mkif_km.h"


#define SGX_MAX_DEV_DATA			24
#define	SGX_MAX_INIT_MEM_HANDLES	16


typedef struct _SGX_BRIDGE_INFO_FOR_SRVINIT
{
	IMG_DEV_PHYADDR sPDDevPAddr;
	PVRSRV_HEAP_INFO asHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];
} SGX_BRIDGE_INFO_FOR_SRVINIT;


typedef enum _SGXMKIF_CMD_TYPE_
{
	SGXMKIF_CMD_TA				= 0,
	SGXMKIF_CMD_TRANSFER		= 1,
	SGXMKIF_CMD_2D				= 2,
	SGXMKIF_CMD_POWER			= 3,
	SGXMKIF_CMD_CLEANUP			= 4,
	SGXMKIF_CMD_GETMISCINFO		= 5,
	SGXMKIF_CMD_PROCESS_QUEUES	= 6,
	SGXMKIF_CMD_DATABREAKPOINT	= 7,
	SGXMKIF_CMD_SETHWPERFSTATUS	= 8,
	SGXMKIF_CMD_MAX				= 9,

	SGXMKIF_CMD_FORCE_I32   	= -1,

} SGXMKIF_CMD_TYPE;


typedef struct _SGX_BRIDGE_INIT_INFO_
{
	IMG_HANDLE	hKernelCCBMemInfo;
	IMG_HANDLE	hKernelCCBCtlMemInfo;
	IMG_HANDLE	hKernelCCBEventKickerMemInfo;
	IMG_HANDLE	hKernelSGXHostCtlMemInfo;
	IMG_HANDLE	hKernelSGXTA3DCtlMemInfo;
	IMG_HANDLE	hKernelSGXMiscMemInfo;

	IMG_UINT32	aui32HostKickAddr[SGXMKIF_CMD_MAX];

	SGX_INIT_SCRIPTS sScripts;

	IMG_UINT32	ui32ClientBuildOptions;
	SGX_MISCINFO_STRUCT_SIZES	sSGXStructSizes;

#if defined(SGX_SUPPORT_HWPROFILING)
	IMG_HANDLE	hKernelHWProfilingMemInfo;
#endif
#if defined(SUPPORT_SGX_HWPERF)
	IMG_HANDLE	hKernelHWPerfCBMemInfo;
#endif
	IMG_HANDLE	hKernelTASigBufferMemInfo;
	IMG_HANDLE	hKernel3DSigBufferMemInfo;

#if defined(FIX_HW_BRN_29702)
	IMG_HANDLE	hKernelCFIMemInfo;
#endif
#if defined(FIX_HW_BRN_29823)
	IMG_HANDLE	hKernelDummyTermStreamMemInfo;
#endif
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	IMG_HANDLE	hKernelEDMStatusBufferMemInfo;
#endif
#if defined(SGX_FEATURE_OVERLAPPED_SPM)
	IMG_HANDLE hKernelTmpRgnHeaderMemInfo;
#endif
#if defined(SGX_FEATURE_SPM_MODE_0)
	IMG_HANDLE hKernelTmpDPMStateMemInfo;
#endif

	IMG_UINT32 ui32EDMTaskReg0;
	IMG_UINT32 ui32EDMTaskReg1;

	IMG_UINT32 ui32ClkGateStatusReg;
	IMG_UINT32 ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
	IMG_UINT32 ui32MasterClkGateStatusReg;
	IMG_UINT32 ui32MasterClkGateStatusMask;
	IMG_UINT32 ui32MasterClkGateStatus2Reg;
	IMG_UINT32 ui32MasterClkGateStatus2Mask;
#endif 

	IMG_UINT32 ui32CacheControl;

	IMG_UINT32	asInitDevData[SGX_MAX_DEV_DATA];
	IMG_HANDLE	asInitMemHandles[SGX_MAX_INIT_MEM_HANDLES];

} SGX_BRIDGE_INIT_INFO;


typedef struct _SGX_DEVICE_SYNC_LIST_
{
	PSGXMKIF_HWDEVICE_SYNC_LIST	psHWDeviceSyncList;

	IMG_HANDLE				hKernelHWSyncListMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psHWDeviceSyncListClientMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psAccessResourceClientMemInfo;

	volatile IMG_UINT32		*pui32Lock;

	struct _SGX_DEVICE_SYNC_LIST_	*psNext;

	
	IMG_UINT32			ui32NumSyncObjects;
	IMG_HANDLE			ahSyncHandles[1];
} SGX_DEVICE_SYNC_LIST, *PSGX_DEVICE_SYNC_LIST;


typedef struct _SGX_INTERNEL_STATUS_UPDATE_
{
	CTL_STATUS				sCtlStatus;
	IMG_HANDLE				hKernelMemInfo;
} SGX_INTERNEL_STATUS_UPDATE;


typedef struct _SGX_CCB_KICK_
{
	SGXMKIF_COMMAND		sCommand;
	IMG_HANDLE			hCCBKernelMemInfo;

	IMG_UINT32	ui32NumDstSyncObjects;
	IMG_HANDLE	hKernelHWSyncListMemInfo;

	
	IMG_HANDLE	*pahDstSyncHandles;

	IMG_UINT32	ui32NumTAStatusVals;
	IMG_UINT32	ui32Num3DStatusVals;

#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
	SGX_INTERNEL_STATUS_UPDATE	asTAStatusUpdate[SGX_MAX_TA_STATUS_VALS];
	SGX_INTERNEL_STATUS_UPDATE	as3DStatusUpdate[SGX_MAX_3D_STATUS_VALS];
#else
	IMG_HANDLE	ahTAStatusSyncInfo[SGX_MAX_TA_STATUS_VALS];
	IMG_HANDLE	ah3DStatusSyncInfo[SGX_MAX_3D_STATUS_VALS];
#endif

	IMG_BOOL	bFirstKickOrResume;
#if (defined(NO_HARDWARE) || defined(PDUMP))
	IMG_BOOL	bTerminateOrAbort;
#endif
	IMG_BOOL	bLastInScene;

	
	IMG_UINT32	ui32CCBOffset;

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	
	IMG_UINT32	ui32NumTASrcSyncs;
	IMG_HANDLE	ahTASrcKernelSyncInfo[SGX_MAX_TA_SRC_SYNCS];
	IMG_UINT32	ui32NumTADstSyncs;
	IMG_HANDLE	ahTADstKernelSyncInfo[SGX_MAX_TA_DST_SYNCS];
	IMG_UINT32	ui32Num3DSrcSyncs;
	IMG_HANDLE	ah3DSrcKernelSyncInfo[SGX_MAX_3D_SRC_SYNCS];
#else
	
	IMG_UINT32	ui32NumSrcSyncs;
	IMG_HANDLE	ahSrcKernelSyncInfo[SGX_MAX_SRC_SYNCS];
#endif

	
	IMG_BOOL	bTADependency;
	IMG_HANDLE	hTA3DSyncInfo;

	IMG_HANDLE	hTASyncInfo;
	IMG_HANDLE	h3DSyncInfo;
#if defined(PDUMP)
	IMG_UINT32	ui32CCBDumpWOff;
#endif
#if defined(NO_HARDWARE)
	IMG_UINT32	ui32WriteOpsPendingVal;
#endif
} SGX_CCB_KICK;


#define SGX_KERNEL_USE_CODE_BASE_INDEX		15


typedef struct _SGX_CLIENT_INFO_
{
	IMG_UINT32					ui32ProcessID;			
	IMG_VOID					*pvProcess;				
	PVRSRV_MISC_INFO			sMiscInfo;				

	IMG_UINT32					asDevData[SGX_MAX_DEV_DATA];

} SGX_CLIENT_INFO;

typedef struct _SGX_INTERNAL_DEVINFO_
{
	IMG_UINT32			ui32Flags;
	IMG_HANDLE			hHostCtlKernelMemInfoHandle;
	IMG_BOOL			bForcePTOff;
} SGX_INTERNAL_DEVINFO;


#if defined(TRANSFER_QUEUE)
typedef struct _PVRSRV_TRANSFER_SGX_KICK_
{
	IMG_HANDLE		hCCBMemInfo;
	IMG_UINT32		ui32SharedCmdCCBOffset;

	IMG_DEV_VIRTADDR 	sHWTransferContextDevVAddr;

	IMG_HANDLE		hTASyncInfo;
	IMG_HANDLE		h3DSyncInfo;

	IMG_UINT32		ui32NumSrcSync;
	IMG_HANDLE		ahSrcSyncInfo[SGX_MAX_TRANSFER_SYNC_OPS];

	IMG_UINT32		ui32NumDstSync;
	IMG_HANDLE		ahDstSyncInfo[SGX_MAX_TRANSFER_SYNC_OPS];

	IMG_UINT32		ui32Flags;

	IMG_UINT32		ui32PDumpFlags;
#if defined(PDUMP)
	IMG_UINT32		ui32CCBDumpWOff;
#endif
} PVRSRV_TRANSFER_SGX_KICK, *PPVRSRV_TRANSFER_SGX_KICK;

#if defined(SGX_FEATURE_2D_HARDWARE)
typedef struct _PVRSRV_2D_SGX_KICK_
{
	IMG_HANDLE		hCCBMemInfo;
	IMG_UINT32		ui32SharedCmdCCBOffset;

	IMG_DEV_VIRTADDR 	sHW2DContextDevVAddr;

	IMG_UINT32		ui32NumSrcSync;
	IMG_HANDLE		ahSrcSyncInfo[SGX_MAX_2D_SRC_SYNC_OPS];

	
	IMG_HANDLE 		hDstSyncInfo;

	
	IMG_HANDLE		hTASyncInfo;

	
	IMG_HANDLE		h3DSyncInfo;

	IMG_UINT32		ui32PDumpFlags;
#if defined(PDUMP)
	IMG_UINT32		ui32CCBDumpWOff;
#endif
} PVRSRV_2D_SGX_KICK, *PPVRSRV_2D_SGX_KICK;
#endif	
#endif	


#endif 
