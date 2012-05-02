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



#include <stddef.h>

#include "img_defs.h"
#include "services.h"
#include "pvr_bridge_km.h"
#include "pvr_debug.h"
#include "ra.h"
#include "pvr_bridge.h"
#if defined(SUPPORT_SGX)
#include "sgx_bridge.h"
#endif
#if defined(SUPPORT_VGX)
#include "vgx_bridge.h"
#endif
#if defined(SUPPORT_MSVDX)
#include "msvdx_bridge.h"
#endif
#include "perproc.h"
#include "device.h"
#include "buffer_manager.h"

#include "pdump_km.h"
#include "syscommon.h"

#include "bridged_pvr_bridge.h"
#if defined(SUPPORT_SGX)
#include "bridged_sgx_bridge.h"
#endif
#if defined(SUPPORT_VGX)
#include "bridged_vgx_bridge.h"
#endif
#if defined(SUPPORT_MSVDX)
#include "bridged_msvdx_bridge.h"
#endif

#include "env_data.h"

#if defined (__linux__)
#include "mmap.h"
#endif


#include "srvkm.h"

PVRSRV_BRIDGE_DISPATCH_TABLE_ENTRY g_BridgeDispatchTable[BRIDGE_DISPATCH_TABLE_ENTRY_COUNT];

#if defined(DEBUG_BRIDGE_KM)
PVRSRV_BRIDGE_GLOBAL_STATS g_BridgeGlobalStats;
#endif

#if defined(PVR_SECURE_HANDLES)
static IMG_BOOL abSharedDeviceMemHeap[PVRSRV_MAX_CLIENT_HEAPS];
static IMG_BOOL *pbSharedDeviceMemHeap = abSharedDeviceMemHeap;
#else
static IMG_BOOL *pbSharedDeviceMemHeap = (IMG_BOOL*)IMG_NULL;
#endif


#if defined(DEBUG_BRIDGE_KM)
PVRSRV_ERROR
CopyFromUserWrapper(PVRSRV_PER_PROCESS_DATA *pProcData,
					IMG_UINT32 ui32BridgeID,
					IMG_VOID *pvDest,
					IMG_VOID *pvSrc,
					IMG_UINT32 ui32Size)
{
	g_BridgeDispatchTable[ui32BridgeID].ui32CopyFromUserTotalBytes+=ui32Size;
	g_BridgeGlobalStats.ui32TotalCopyFromUserBytes+=ui32Size;
	return OSCopyFromUser(pProcData, pvDest, pvSrc, ui32Size);
}
PVRSRV_ERROR
CopyToUserWrapper(PVRSRV_PER_PROCESS_DATA *pProcData,
				  IMG_UINT32 ui32BridgeID,
				  IMG_VOID *pvDest,
				  IMG_VOID *pvSrc,
				  IMG_UINT32 ui32Size)
{
	g_BridgeDispatchTable[ui32BridgeID].ui32CopyToUserTotalBytes+=ui32Size;
	g_BridgeGlobalStats.ui32TotalCopyToUserBytes+=ui32Size;
	return OSCopyToUser(pProcData, pvDest, pvSrc, ui32Size);
}
#endif


static IMG_INT
PVRSRVEnumerateDevicesBW(IMG_UINT32 ui32BridgeID,
						 IMG_VOID *psBridgeIn,
						 PVRSRV_BRIDGE_OUT_ENUMDEVICE *psEnumDeviceOUT,
						 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ENUM_DEVICES);

	PVR_UNREFERENCED_PARAMETER(psPerProc);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	psEnumDeviceOUT->eError =
		PVRSRVEnumerateDevicesKM(&psEnumDeviceOUT->ui32NumDevices,
								 psEnumDeviceOUT->asDeviceIdentifier);

	return 0;
}

static IMG_INT
PVRSRVAcquireDeviceDataBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_ACQUIRE_DEVICEINFO *psAcquireDevInfoIN,
						  PVRSRV_BRIDGE_OUT_ACQUIRE_DEVICEINFO *psAcquireDevInfoOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ACQUIRE_DEVICEINFO);

	psAcquireDevInfoOUT->eError =
		PVRSRVAcquireDeviceDataKM(psAcquireDevInfoIN->uiDevIndex,
								  psAcquireDevInfoIN->eDeviceType,
								  &hDevCookieInt);
	if(psAcquireDevInfoOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
		psAcquireDevInfoOUT->eError =
		PVRSRVAllocHandle(psPerProc->psHandleBase,
						  &psAcquireDevInfoOUT->hDevCookie,
						  hDevCookieInt,
						  PVRSRV_HANDLE_TYPE_DEV_NODE,
						  PVRSRV_HANDLE_ALLOC_FLAG_SHARED);

	return 0;
}


static IMG_INT
PVRSRVCreateDeviceMemContextBW(IMG_UINT32 ui32BridgeID,
							   PVRSRV_BRIDGE_IN_CREATE_DEVMEMCONTEXT *psCreateDevMemContextIN,
							   PVRSRV_BRIDGE_OUT_CREATE_DEVMEMCONTEXT *psCreateDevMemContextOUT,
							   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	IMG_HANDLE hDevMemContextInt;
	IMG_UINT32 i;
	IMG_BOOL bCreated;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CREATE_DEVMEMCONTEXT);

	
	NEW_HANDLE_BATCH_OR_ERROR(psCreateDevMemContextOUT->eError, psPerProc, PVRSRV_MAX_CLIENT_HEAPS + 1)

	psCreateDevMemContextOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
						   psCreateDevMemContextIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	if(psCreateDevMemContextOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psCreateDevMemContextOUT->eError =
		PVRSRVCreateDeviceMemContextKM(hDevCookieInt,
									   psPerProc,
									   &hDevMemContextInt,
									   &psCreateDevMemContextOUT->ui32ClientHeapCount,
									   &psCreateDevMemContextOUT->sHeapInfo[0],
									   &bCreated,
									   pbSharedDeviceMemHeap);

	if(psCreateDevMemContextOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	if(bCreated)
	{
		PVRSRVAllocHandleNR(psPerProc->psHandleBase,
						  &psCreateDevMemContextOUT->hDevMemContext,
						  hDevMemContextInt,
						  PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT,
						  PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	}
	else
	{
		psCreateDevMemContextOUT->eError =
			PVRSRVFindHandle(psPerProc->psHandleBase,
							 &psCreateDevMemContextOUT->hDevMemContext,
							 hDevMemContextInt,
							 PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);
		if(psCreateDevMemContextOUT->eError != PVRSRV_OK)
		{
			return 0;
		}
	}

	for(i = 0; i < psCreateDevMemContextOUT->ui32ClientHeapCount; i++)
	{
		IMG_HANDLE hDevMemHeapExt;

#if defined(PVR_SECURE_HANDLES)
		if(abSharedDeviceMemHeap[i])
#endif
		{
			
			PVRSRVAllocHandleNR(psPerProc->psHandleBase, &hDevMemHeapExt,
							  psCreateDevMemContextOUT->sHeapInfo[i].hDevMemHeap,
							  PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
							  PVRSRV_HANDLE_ALLOC_FLAG_SHARED);
		}
#if defined(PVR_SECURE_HANDLES)
		else
		{
			
			if(bCreated)
			{
				PVRSRVAllocSubHandleNR(psPerProc->psHandleBase, &hDevMemHeapExt,
									 psCreateDevMemContextOUT->sHeapInfo[i].hDevMemHeap,
									 PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
									 PVRSRV_HANDLE_ALLOC_FLAG_NONE,
									 psCreateDevMemContextOUT->hDevMemContext);
			}
			else
			{
				psCreateDevMemContextOUT->eError =
					PVRSRVFindHandle(psPerProc->psHandleBase, &hDevMemHeapExt,
									 psCreateDevMemContextOUT->sHeapInfo[i].hDevMemHeap,
									 PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);
				if(psCreateDevMemContextOUT->eError != PVRSRV_OK)
				{
					return 0;
				}
			}
		}
#endif
		psCreateDevMemContextOUT->sHeapInfo[i].hDevMemHeap = hDevMemHeapExt;
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psCreateDevMemContextOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVDestroyDeviceMemContextBW(IMG_UINT32 ui32BridgeID,
								PVRSRV_BRIDGE_IN_DESTROY_DEVMEMCONTEXT *psDestroyDevMemContextIN,
								PVRSRV_BRIDGE_RETURN *psRetOUT,
								PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	IMG_HANDLE hDevMemContextInt;
	IMG_BOOL bDestroyed;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_DESTROY_DEVMEMCONTEXT);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
						   psDestroyDevMemContextIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
						   psDestroyDevMemContextIN->hDevMemContext,
						   PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVDestroyDeviceMemContextKM(hDevCookieInt, hDevMemContextInt, &bDestroyed);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	if(bDestroyed)
	{
		psRetOUT->eError =
			PVRSRVReleaseHandle(psPerProc->psHandleBase,
								psDestroyDevMemContextIN->hDevMemContext,
								PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);
	}

	return 0;
}


static IMG_INT
PVRSRVGetDeviceMemHeapInfoBW(IMG_UINT32 ui32BridgeID,
							   PVRSRV_BRIDGE_IN_GET_DEVMEM_HEAPINFO *psGetDevMemHeapInfoIN,
							   PVRSRV_BRIDGE_OUT_GET_DEVMEM_HEAPINFO *psGetDevMemHeapInfoOUT,
							   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	IMG_HANDLE hDevMemContextInt;
	IMG_UINT32 i;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_DEVMEM_HEAPINFO);

	NEW_HANDLE_BATCH_OR_ERROR(psGetDevMemHeapInfoOUT->eError, psPerProc, PVRSRV_MAX_CLIENT_HEAPS)

	psGetDevMemHeapInfoOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
						   psGetDevMemHeapInfoIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	if(psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetDevMemHeapInfoOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
						   psGetDevMemHeapInfoIN->hDevMemContext,
						   PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if(psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetDevMemHeapInfoOUT->eError =
		PVRSRVGetDeviceMemHeapInfoKM(hDevCookieInt,
									   hDevMemContextInt,
									   &psGetDevMemHeapInfoOUT->ui32ClientHeapCount,
									   &psGetDevMemHeapInfoOUT->sHeapInfo[0],
									   pbSharedDeviceMemHeap);

	if(psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	for(i = 0; i < psGetDevMemHeapInfoOUT->ui32ClientHeapCount; i++)
	{
		IMG_HANDLE hDevMemHeapExt;

#if defined(PVR_SECURE_HANDLES)
		if(abSharedDeviceMemHeap[i])
#endif
		{
			
			PVRSRVAllocHandleNR(psPerProc->psHandleBase, &hDevMemHeapExt,
							  psGetDevMemHeapInfoOUT->sHeapInfo[i].hDevMemHeap,
							  PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
							  PVRSRV_HANDLE_ALLOC_FLAG_SHARED);
		}
#if defined(PVR_SECURE_HANDLES)
		else
		{
			
			psGetDevMemHeapInfoOUT->eError =
				PVRSRVFindHandle(psPerProc->psHandleBase, &hDevMemHeapExt,
								 psGetDevMemHeapInfoOUT->sHeapInfo[i].hDevMemHeap,
								 PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);
			if(psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
			{
				return 0;
			}
		}
#endif
		psGetDevMemHeapInfoOUT->sHeapInfo[i].hDevMemHeap = hDevMemHeapExt;
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetDevMemHeapInfoOUT->eError, psPerProc)

	return 0;
}


#if defined(OS_PVRSRV_ALLOC_DEVICE_MEM_BW)
IMG_INT
PVRSRVAllocDeviceMemBW(IMG_UINT32 ui32BridgeID,
					   PVRSRV_BRIDGE_IN_ALLOCDEVICEMEM *psAllocDeviceMemIN,
					   PVRSRV_BRIDGE_OUT_ALLOCDEVICEMEM *psAllocDeviceMemOUT,
					   PVRSRV_PER_PROCESS_DATA *psPerProc);
#else
static IMG_INT
PVRSRVAllocDeviceMemBW(IMG_UINT32 ui32BridgeID,
					   PVRSRV_BRIDGE_IN_ALLOCDEVICEMEM *psAllocDeviceMemIN,
					   PVRSRV_BRIDGE_OUT_ALLOCDEVICEMEM *psAllocDeviceMemOUT,
					   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_KERNEL_MEM_INFO *psMemInfo;
	IMG_HANDLE hDevCookieInt;
	IMG_HANDLE hDevMemHeapInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ALLOC_DEVICEMEM);

	NEW_HANDLE_BATCH_OR_ERROR(psAllocDeviceMemOUT->eError, psPerProc, 2)

	psAllocDeviceMemOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
						   psAllocDeviceMemIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	if(psAllocDeviceMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psAllocDeviceMemOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemHeapInt,
						   psAllocDeviceMemIN->hDevMemHeap,
						   PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);

	if(psAllocDeviceMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psAllocDeviceMemOUT->eError =
		PVRSRVAllocDeviceMemKM(hDevCookieInt,
							   psPerProc,
							   hDevMemHeapInt,
							   psAllocDeviceMemIN->ui32Attribs,
							   psAllocDeviceMemIN->ui32Size,
							   psAllocDeviceMemIN->ui32Alignment,
							   &psMemInfo,
							   "" );

	if(psAllocDeviceMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	OSMemSet(&psAllocDeviceMemOUT->sClientMemInfo,
			 0,
			 sizeof(psAllocDeviceMemOUT->sClientMemInfo));

	psAllocDeviceMemOUT->sClientMemInfo.pvLinAddrKM =
			psMemInfo->pvLinAddrKM;

#if defined (__linux__)
	psAllocDeviceMemOUT->sClientMemInfo.pvLinAddr = 0;
#else
	psAllocDeviceMemOUT->sClientMemInfo.pvLinAddr = psMemInfo->pvLinAddrKM;
#endif
	psAllocDeviceMemOUT->sClientMemInfo.sDevVAddr = psMemInfo->sDevVAddr;
	psAllocDeviceMemOUT->sClientMemInfo.ui32Flags = psMemInfo->ui32Flags;
	psAllocDeviceMemOUT->sClientMemInfo.ui32AllocSize = psMemInfo->ui32AllocSize;
	psAllocDeviceMemOUT->sClientMemInfo.hMappingInfo = psMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					  &psAllocDeviceMemOUT->sClientMemInfo.hKernelMemInfo,
					  psMemInfo,
					  PVRSRV_HANDLE_TYPE_MEM_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	if(psAllocDeviceMemIN->ui32Attribs & PVRSRV_MEM_NO_SYNCOBJ)
	{
		
		OSMemSet(&psAllocDeviceMemOUT->sClientSyncInfo,
				 0,
				 sizeof (PVRSRV_CLIENT_SYNC_INFO));
		psAllocDeviceMemOUT->sClientMemInfo.psClientSyncInfo = IMG_NULL;
	}
	else
	{
		

		psAllocDeviceMemOUT->sClientSyncInfo.psSyncData =
			psMemInfo->psKernelSyncInfo->psSyncData;
		psAllocDeviceMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
			psMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
		psAllocDeviceMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
			psMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

		psAllocDeviceMemOUT->sClientSyncInfo.hMappingInfo =
			psMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->sMemBlk.hOSMemHandle;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
							 &psAllocDeviceMemOUT->sClientSyncInfo.hKernelSyncInfo,
							 psMemInfo->psKernelSyncInfo,
							 PVRSRV_HANDLE_TYPE_SYNC_INFO,
							 PVRSRV_HANDLE_ALLOC_FLAG_NONE,
							 psAllocDeviceMemOUT->sClientMemInfo.hKernelMemInfo);

		psAllocDeviceMemOUT->sClientMemInfo.psClientSyncInfo =
			&psAllocDeviceMemOUT->sClientSyncInfo;

	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psAllocDeviceMemOUT->eError, psPerProc)

	return 0;
}

#endif 

static IMG_INT
PVRSRVFreeDeviceMemBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_FREEDEVICEMEM *psFreeDeviceMemIN,
					  PVRSRV_BRIDGE_RETURN *psRetOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	IMG_VOID *pvKernelMemInfo;


	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_FREE_DEVICEMEM);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
						   psFreeDeviceMemIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &pvKernelMemInfo,
						   psFreeDeviceMemIN->psKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PVRSRVFreeDeviceMemKM(hDevCookieInt, pvKernelMemInfo);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVReleaseHandle(psPerProc->psHandleBase,
							psFreeDeviceMemIN->psKernelMemInfo,
							PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}


static IMG_INT
PVRSRVExportDeviceMemBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM *psExportDeviceMemIN,
					  PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM *psExportDeviceMemOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_EXPORT_DEVICEMEM);

	
	psExportDeviceMemOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
						   psExportDeviceMemIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	if(psExportDeviceMemOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMemBW: can't find devcookie"));
		return 0;
	}

	
	psExportDeviceMemOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, (IMG_PVOID *)&psKernelMemInfo,
						   psExportDeviceMemIN->psKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);

	if(psExportDeviceMemOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMemBW: can't find kernel meminfo"));
		return 0;
	}

	
	psExportDeviceMemOUT->eError =
		PVRSRVFindHandle(KERNEL_HANDLE_BASE,
							 &psExportDeviceMemOUT->hMemInfo,
							 psKernelMemInfo,
							 PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psExportDeviceMemOUT->eError == PVRSRV_OK)
	{
		
		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVExportDeviceMemBW: allocation is already exported"));
		return 0;
	}

	
	psExportDeviceMemOUT->eError = PVRSRVAllocHandle(KERNEL_HANDLE_BASE,
													&psExportDeviceMemOUT->hMemInfo,
													psKernelMemInfo,
													PVRSRV_HANDLE_TYPE_MEM_INFO,
													PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	if (psExportDeviceMemOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMemBW: failed to allocate handle from global handle list"));
		return 0;
	}

	
	psKernelMemInfo->ui32Flags |= PVRSRV_MEM_EXPORTED;

	return 0;
}


static IMG_INT
PVRSRVMapDeviceMemoryBW(IMG_UINT32 ui32BridgeID,
							 PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY *psMapDevMemIN,
							 PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY *psMapDevMemOUT,
							 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_KERNEL_MEM_INFO	*psSrcKernelMemInfo = IMG_NULL;
	PVRSRV_KERNEL_MEM_INFO	*psDstKernelMemInfo = IMG_NULL;
	IMG_HANDLE				hDstDevMemHeap = IMG_NULL;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MAP_DEV_MEMORY);

	NEW_HANDLE_BATCH_OR_ERROR(psMapDevMemOUT->eError, psPerProc, 2)

	
	psMapDevMemOUT->eError = PVRSRVLookupHandle(KERNEL_HANDLE_BASE,
												(IMG_VOID**)&psSrcKernelMemInfo,
												psMapDevMemIN->hKernelMemInfo,
												PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psMapDevMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	psMapDevMemOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
												&hDstDevMemHeap,
												psMapDevMemIN->hDstDevMemHeap,
												PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);
	if(psMapDevMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	psMapDevMemOUT->eError = PVRSRVMapDeviceMemoryKM(psPerProc,
												  psSrcKernelMemInfo,
												  hDstDevMemHeap,
												  &psDstKernelMemInfo);
	if(psMapDevMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	OSMemSet(&psMapDevMemOUT->sDstClientMemInfo,
			 0,
			 sizeof(psMapDevMemOUT->sDstClientMemInfo));
	OSMemSet(&psMapDevMemOUT->sDstClientSyncInfo,
			 0,
			 sizeof(psMapDevMemOUT->sDstClientSyncInfo));

	psMapDevMemOUT->sDstClientMemInfo.pvLinAddrKM =
			psDstKernelMemInfo->pvLinAddrKM;

	psMapDevMemOUT->sDstClientMemInfo.pvLinAddr = 0;
	psMapDevMemOUT->sDstClientMemInfo.sDevVAddr = psDstKernelMemInfo->sDevVAddr;
	psMapDevMemOUT->sDstClientMemInfo.ui32Flags = psDstKernelMemInfo->ui32Flags;
	psMapDevMemOUT->sDstClientMemInfo.ui32AllocSize = psDstKernelMemInfo->ui32AllocSize;
	psMapDevMemOUT->sDstClientMemInfo.hMappingInfo = psDstKernelMemInfo->sMemBlk.hOSMemHandle;

	
	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					  &psMapDevMemOUT->sDstClientMemInfo.hKernelMemInfo,
					  psDstKernelMemInfo,
					  PVRSRV_HANDLE_TYPE_MEM_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	psMapDevMemOUT->sDstClientSyncInfo.hKernelSyncInfo = IMG_NULL;

	
	if(psDstKernelMemInfo->psKernelSyncInfo)
	{
		psMapDevMemOUT->sDstClientSyncInfo.psSyncData =
			psDstKernelMemInfo->psKernelSyncInfo->psSyncData;
		psMapDevMemOUT->sDstClientSyncInfo.sWriteOpsCompleteDevVAddr =
			psDstKernelMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
		psMapDevMemOUT->sDstClientSyncInfo.sReadOpsCompleteDevVAddr =
			psDstKernelMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

		psMapDevMemOUT->sDstClientSyncInfo.hMappingInfo =
			psDstKernelMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->sMemBlk.hOSMemHandle;

		psMapDevMemOUT->sDstClientMemInfo.psClientSyncInfo = &psMapDevMemOUT->sDstClientSyncInfo;
		
		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
					  &psMapDevMemOUT->sDstClientSyncInfo.hKernelSyncInfo,
					  psDstKernelMemInfo->psKernelSyncInfo,
					  PVRSRV_HANDLE_TYPE_SYNC_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
					  psMapDevMemOUT->sDstClientMemInfo.hKernelMemInfo);
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psMapDevMemOUT->eError, psPerProc)

	return 0;
}


static IMG_INT
PVRSRVUnmapDeviceMemoryBW(IMG_UINT32 ui32BridgeID,
							 PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY *psUnmapDevMemIN,
							 PVRSRV_BRIDGE_RETURN *psRetOUT,
							 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo = IMG_NULL;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_UNMAP_DEV_MEMORY);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
											(IMG_VOID**)&psKernelMemInfo,
											psUnmapDevMemIN->psKernelMemInfo,
											PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PVRSRVUnmapDeviceMemoryKM(psKernelMemInfo);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
							psUnmapDevMemIN->psKernelMemInfo,
							PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}



static IMG_INT
PVRSRVMapDeviceClassMemoryBW(IMG_UINT32 ui32BridgeID,
							 PVRSRV_BRIDGE_IN_MAP_DEVICECLASS_MEMORY *psMapDevClassMemIN,
							 PVRSRV_BRIDGE_OUT_MAP_DEVICECLASS_MEMORY *psMapDevClassMemOUT,
							 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_KERNEL_MEM_INFO *psMemInfo;
	IMG_HANDLE hOSMapInfo;
	IMG_HANDLE hDeviceClassBufferInt;
	IMG_HANDLE hDevMemContextInt;
	PVRSRV_HANDLE_TYPE eHandleType;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MAP_DEVICECLASS_MEMORY);

	NEW_HANDLE_BATCH_OR_ERROR(psMapDevClassMemOUT->eError, psPerProc, 2)

	
	psMapDevClassMemOUT->eError =
		PVRSRVLookupHandleAnyType(psPerProc->psHandleBase, &hDeviceClassBufferInt,
								  &eHandleType,
								  psMapDevClassMemIN->hDeviceClassBuffer);

	if(psMapDevClassMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	psMapDevClassMemOUT->eError =
	PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
				   psMapDevClassMemIN->hDevMemContext,
				   PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if(psMapDevClassMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	switch(eHandleType)
	{
#if defined(PVR_SECURE_HANDLES)
		case PVRSRV_HANDLE_TYPE_DISP_BUFFER:
		case PVRSRV_HANDLE_TYPE_BUF_BUFFER:
#else
		case PVRSRV_HANDLE_TYPE_NONE:
#endif
			break;
		default:
			psMapDevClassMemOUT->eError = PVRSRV_ERROR_INVALID_HANDLE_TYPE;
			return 0;
	}

	psMapDevClassMemOUT->eError =
		PVRSRVMapDeviceClassMemoryKM(psPerProc,
									 hDevMemContextInt,
									 hDeviceClassBufferInt,
									 &psMemInfo,
									 &hOSMapInfo);
	if(psMapDevClassMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	OSMemSet(&psMapDevClassMemOUT->sClientMemInfo,
			 0,
			 sizeof(psMapDevClassMemOUT->sClientMemInfo));
	OSMemSet(&psMapDevClassMemOUT->sClientSyncInfo,
			 0,
			 sizeof(psMapDevClassMemOUT->sClientSyncInfo));

	psMapDevClassMemOUT->sClientMemInfo.pvLinAddrKM =
			psMemInfo->pvLinAddrKM;

	psMapDevClassMemOUT->sClientMemInfo.pvLinAddr = 0;
	psMapDevClassMemOUT->sClientMemInfo.sDevVAddr = psMemInfo->sDevVAddr;
	psMapDevClassMemOUT->sClientMemInfo.ui32Flags = psMemInfo->ui32Flags;
	psMapDevClassMemOUT->sClientMemInfo.ui32AllocSize = psMemInfo->ui32AllocSize;
	psMapDevClassMemOUT->sClientMemInfo.hMappingInfo = psMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
					  &psMapDevClassMemOUT->sClientMemInfo.hKernelMemInfo,
					  psMemInfo,
					  PVRSRV_HANDLE_TYPE_MEM_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE,
					  psMapDevClassMemIN->hDeviceClassBuffer);

	psMapDevClassMemOUT->sClientSyncInfo.hKernelSyncInfo = IMG_NULL;

	
	if(psMemInfo->psKernelSyncInfo)
	{
		psMapDevClassMemOUT->sClientSyncInfo.psSyncData =
			psMemInfo->psKernelSyncInfo->psSyncData;
		psMapDevClassMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
			psMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
		psMapDevClassMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
			psMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

		psMapDevClassMemOUT->sClientSyncInfo.hMappingInfo =
			psMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->sMemBlk.hOSMemHandle;

		psMapDevClassMemOUT->sClientMemInfo.psClientSyncInfo = &psMapDevClassMemOUT->sClientSyncInfo;
		
		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
						  &psMapDevClassMemOUT->sClientSyncInfo.hKernelSyncInfo,
						  psMemInfo->psKernelSyncInfo,
						  PVRSRV_HANDLE_TYPE_SYNC_INFO,
						  PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
						  psMapDevClassMemOUT->sClientMemInfo.hKernelMemInfo);
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psMapDevClassMemOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVUnmapDeviceClassMemoryBW(IMG_UINT32 ui32BridgeID,
							   PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY *psUnmapDevClassMemIN,
							   PVRSRV_BRIDGE_RETURN *psRetOUT,
							   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &pvKernelMemInfo,
						   psUnmapDevClassMemIN->psKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PVRSRVUnmapDeviceClassMemoryKM(pvKernelMemInfo);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVReleaseHandle(psPerProc->psHandleBase,
							psUnmapDevClassMemIN->psKernelMemInfo,
							PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}


#if defined(OS_PVRSRV_WRAP_EXT_MEM_BW)
IMG_INT
PVRSRVWrapExtMemoryBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_WRAP_EXT_MEMORY *psWrapExtMemIN,
					  PVRSRV_BRIDGE_OUT_WRAP_EXT_MEMORY *psWrapExtMemOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc);
#else 
static IMG_INT
PVRSRVWrapExtMemoryBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_WRAP_EXT_MEMORY *psWrapExtMemIN,
					  PVRSRV_BRIDGE_OUT_WRAP_EXT_MEMORY *psWrapExtMemOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	IMG_HANDLE hDevMemContextInt;
	PVRSRV_KERNEL_MEM_INFO *psMemInfo;
	IMG_SYS_PHYADDR *psSysPAddr = IMG_NULL;
        IMG_UINT32 ui32PageTableSize = 0;
	
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_WRAP_EXT_MEMORY);

	NEW_HANDLE_BATCH_OR_ERROR(psWrapExtMemOUT->eError, psPerProc, 2)

	
	psWrapExtMemOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
						   psWrapExtMemIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);
	if(psWrapExtMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	psWrapExtMemOUT->eError =
	PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
				   psWrapExtMemIN->hDevMemContext,
				   PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if(psWrapExtMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	if(psWrapExtMemIN->ui32NumPageTableEntries)
	{
		ui32PageTableSize = psWrapExtMemIN->ui32NumPageTableEntries
						* sizeof(IMG_SYS_PHYADDR);

		ASSIGN_AND_EXIT_ON_ERROR(psWrapExtMemOUT->eError,
				  OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				  ui32PageTableSize,
				  (IMG_VOID **)&psSysPAddr, 0,
				  "Page Table"));

		if(CopyFromUserWrapper(psPerProc,
							   ui32BridgeID,
							   psSysPAddr,
							   psWrapExtMemIN->psSysPAddr,
							   ui32PageTableSize) != PVRSRV_OK)
		{
			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, 	ui32PageTableSize, (IMG_VOID *)psSysPAddr, 0);
			
			return -EFAULT;
		}
	}
		
	psWrapExtMemOUT->eError =
		PVRSRVWrapExtMemoryKM(hDevCookieInt,
							  psPerProc,
							  hDevMemContextInt,
							  psWrapExtMemIN->ui32ByteSize,
							  psWrapExtMemIN->ui32PageOffset,
							  psWrapExtMemIN->bPhysContig,
							  psSysPAddr,
							  psWrapExtMemIN->pvLinAddr,
							  psWrapExtMemIN->ui32Flags,
							  &psMemInfo);

	if(psWrapExtMemIN->ui32NumPageTableEntries)
	{
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  ui32PageTableSize,
			  (IMG_VOID *)psSysPAddr, 0);
		
	}
	
	if(psWrapExtMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psWrapExtMemOUT->sClientMemInfo.pvLinAddrKM =
			psMemInfo->pvLinAddrKM;

	
	psWrapExtMemOUT->sClientMemInfo.pvLinAddr = 0;
	psWrapExtMemOUT->sClientMemInfo.sDevVAddr = psMemInfo->sDevVAddr;
	psWrapExtMemOUT->sClientMemInfo.ui32Flags = psMemInfo->ui32Flags;
	psWrapExtMemOUT->sClientMemInfo.ui32AllocSize = psMemInfo->ui32AllocSize;
	psWrapExtMemOUT->sClientMemInfo.hMappingInfo = psMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					  &psWrapExtMemOUT->sClientMemInfo.hKernelMemInfo,
					  psMemInfo,
					  PVRSRV_HANDLE_TYPE_MEM_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	
	psWrapExtMemOUT->sClientSyncInfo.psSyncData =
		psMemInfo->psKernelSyncInfo->psSyncData;
	psWrapExtMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
		psMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
	psWrapExtMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
		psMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

	psWrapExtMemOUT->sClientSyncInfo.hMappingInfo =
		psMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->sMemBlk.hOSMemHandle;

	psWrapExtMemOUT->sClientMemInfo.psClientSyncInfo = &psWrapExtMemOUT->sClientSyncInfo;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
					  &psWrapExtMemOUT->sClientSyncInfo.hKernelSyncInfo,
					  (IMG_HANDLE)psMemInfo->psKernelSyncInfo,
					  PVRSRV_HANDLE_TYPE_SYNC_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE,
					  psWrapExtMemOUT->sClientMemInfo.hKernelMemInfo);

	COMMIT_HANDLE_BATCH_OR_ERROR(psWrapExtMemOUT->eError, psPerProc)

	return 0;
}
#endif 

static IMG_INT
PVRSRVUnwrapExtMemoryBW(IMG_UINT32 ui32BridgeID,
						PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY *psUnwrapExtMemIN,
						PVRSRV_BRIDGE_RETURN *psRetOUT,
						PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvMemInfo,
						   psUnwrapExtMemIN->hKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVUnwrapExtMemoryKM((PVRSRV_KERNEL_MEM_INFO *)pvMemInfo);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVReleaseHandle(psPerProc->psHandleBase,
						   psUnwrapExtMemIN->hKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}

static IMG_INT
PVRSRVGetFreeDeviceMemBW(IMG_UINT32 ui32BridgeID,
						 PVRSRV_BRIDGE_IN_GETFREEDEVICEMEM *psGetFreeDeviceMemIN,
						 PVRSRV_BRIDGE_OUT_GETFREEDEVICEMEM *psGetFreeDeviceMemOUT,
						 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GETFREE_DEVICEMEM);

	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psGetFreeDeviceMemOUT->eError =
		PVRSRVGetFreeDeviceMemKM(psGetFreeDeviceMemIN->ui32Flags,
								 &psGetFreeDeviceMemOUT->ui32Total,
								 &psGetFreeDeviceMemOUT->ui32Free,
								 &psGetFreeDeviceMemOUT->ui32LargestBlock);

	return 0;
}

static IMG_INT
PVRMMapOSMemHandleToMMapDataBW(IMG_UINT32 ui32BridgeID,
								  PVRSRV_BRIDGE_IN_MHANDLE_TO_MMAP_DATA *psMMapDataIN,
								  PVRSRV_BRIDGE_OUT_MHANDLE_TO_MMAP_DATA *psMMapDataOUT,
								  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MHANDLE_TO_MMAP_DATA);

#if defined (__linux__)
	psMMapDataOUT->eError =
		PVRMMapOSMemHandleToMMapData(psPerProc,
										psMMapDataIN->hMHandle,
										&psMMapDataOUT->ui32MMapOffset,
										&psMMapDataOUT->ui32ByteOffset,
										&psMMapDataOUT->ui32RealByteSize,
										&psMMapDataOUT->ui32UserVAddr);
#else	
	PVR_UNREFERENCED_PARAMETER(psPerProc);
	PVR_UNREFERENCED_PARAMETER(psMMapDataIN);

	psMMapDataOUT->eError = PVRSRV_ERROR_NOT_SUPPORTED;
#endif	
	return 0;
}


static IMG_INT
PVRMMapReleaseMMapDataBW(IMG_UINT32 ui32BridgeID,
								  PVRSRV_BRIDGE_IN_RELEASE_MMAP_DATA *psMMapDataIN,
								  PVRSRV_BRIDGE_OUT_RELEASE_MMAP_DATA *psMMapDataOUT,
								  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_RELEASE_MMAP_DATA);

#if defined (__linux__)
	psMMapDataOUT->eError =
		PVRMMapReleaseMMapData(psPerProc,
										psMMapDataIN->hMHandle,
										&psMMapDataOUT->bMUnmap,
										&psMMapDataOUT->ui32RealByteSize,
										&psMMapDataOUT->ui32UserVAddr);
#else
	
	PVR_UNREFERENCED_PARAMETER(psPerProc);
	PVR_UNREFERENCED_PARAMETER(psMMapDataIN);

	psMMapDataOUT->eError = PVRSRV_ERROR_NOT_SUPPORTED;
#endif	
	return 0;
}


#ifdef PDUMP
static IMG_INT
PDumpIsCaptureFrameBW(IMG_UINT32 ui32BridgeID,
					  IMG_VOID *psBridgeIn,
					  PVRSRV_BRIDGE_OUT_PDUMP_ISCAPTURING *psPDumpIsCapturingOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_ISCAPTURING);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psPDumpIsCapturingOUT->bIsCapturing = PDumpIsCaptureFrameKM();
	psPDumpIsCapturingOUT->eError = PVRSRV_OK;

	return 0;
}

static IMG_INT
PDumpCommentBW(IMG_UINT32 ui32BridgeID,
			   PVRSRV_BRIDGE_IN_PDUMP_COMMENT *psPDumpCommentIN,
			   PVRSRV_BRIDGE_RETURN *psRetOUT,
			   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_COMMENT);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpCommentKM(&psPDumpCommentIN->szComment[0],
									  psPDumpCommentIN->ui32Flags);
	return 0;
}

static IMG_INT
PDumpSetFrameBW(IMG_UINT32 ui32BridgeID,
				PVRSRV_BRIDGE_IN_PDUMP_SETFRAME *psPDumpSetFrameIN,
				PVRSRV_BRIDGE_RETURN *psRetOUT,
				PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_SETFRAME);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpSetFrameKM(psPDumpSetFrameIN->ui32Frame);

	return 0;
}

static IMG_INT
PDumpRegWithFlagsBW(IMG_UINT32 ui32BridgeID,
					PVRSRV_BRIDGE_IN_PDUMP_DUMPREG *psPDumpRegDumpIN,
					PVRSRV_BRIDGE_RETURN *psRetOUT,
					PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_REG);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, 
						   (IMG_VOID **)&psDeviceNode, 
						   psPDumpRegDumpIN->hDevCookie, 
						   PVRSRV_HANDLE_TYPE_DEV_NODE);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PDumpRegWithFlagsKM (psPDumpRegDumpIN->szRegRegion,
											psPDumpRegDumpIN->sHWReg.ui32RegAddr,
											psPDumpRegDumpIN->sHWReg.ui32RegVal,
											psPDumpRegDumpIN->ui32Flags);

	return 0;
}

static IMG_INT
PDumpRegPolBW(IMG_UINT32 ui32BridgeID,
			  PVRSRV_BRIDGE_IN_PDUMP_REGPOL *psPDumpRegPolIN,
			  PVRSRV_BRIDGE_RETURN *psRetOUT,
			  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_REGPOL);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, 
						   (IMG_VOID **)&psDeviceNode, 
						   psPDumpRegPolIN->hDevCookie, 
						   PVRSRV_HANDLE_TYPE_DEV_NODE);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}


	psRetOUT->eError = 
		PDumpRegPolWithFlagsKM(psPDumpRegPolIN->szRegRegion,
							   psPDumpRegPolIN->sHWReg.ui32RegAddr,	
							   psPDumpRegPolIN->sHWReg.ui32RegVal,
							   psPDumpRegPolIN->ui32Mask,
							   psPDumpRegPolIN->ui32Flags);

	return 0;
}

static IMG_INT
PDumpMemPolBW(IMG_UINT32 ui32BridgeID,
			  PVRSRV_BRIDGE_IN_PDUMP_MEMPOL *psPDumpMemPolIN,
			  PVRSRV_BRIDGE_RETURN *psRetOUT,
			  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_MEMPOL);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvMemInfo,
						   psPDumpMemPolIN->psKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PDumpMemPolKM(((PVRSRV_KERNEL_MEM_INFO *)pvMemInfo),
					  psPDumpMemPolIN->ui32Offset,
					  psPDumpMemPolIN->ui32Value,
					  psPDumpMemPolIN->ui32Mask,
					  psPDumpMemPolIN->eOperator,
					  psPDumpMemPolIN->ui32Flags,
					  MAKEUNIQUETAG(pvMemInfo));

	return 0;
}

static IMG_INT
PDumpMemBW(IMG_UINT32 ui32BridgeID,
		   PVRSRV_BRIDGE_IN_PDUMP_DUMPMEM *psPDumpMemDumpIN,
		   PVRSRV_BRIDGE_RETURN *psRetOUT,
		   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DUMPMEM);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvMemInfo,
						   psPDumpMemDumpIN->psKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PDumpMemUM(psPerProc,
				   psPDumpMemDumpIN->pvAltLinAddr,
				   psPDumpMemDumpIN->pvLinAddr,
				   pvMemInfo,
				   psPDumpMemDumpIN->ui32Offset,
				   psPDumpMemDumpIN->ui32Bytes,
				   psPDumpMemDumpIN->ui32Flags,
				   MAKEUNIQUETAG(pvMemInfo));

	return 0;
}

static IMG_INT
PDumpBitmapBW(IMG_UINT32 ui32BridgeID,
			  PVRSRV_BRIDGE_IN_PDUMP_BITMAP *psPDumpBitmapIN,
			  PVRSRV_BRIDGE_RETURN *psRetOUT,
			  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_HANDLE hDevMemContextInt;
	
	PVR_UNREFERENCED_PARAMETER(ui32BridgeID);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, (IMG_VOID **)&psDeviceNode,
						   psPDumpBitmapIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	psRetOUT->eError =
		PVRSRVLookupHandle(	psPerProc->psHandleBase,
							&hDevMemContextInt,
							psPDumpBitmapIN->hDevMemContext,
							PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}
						
	psRetOUT->eError =
		PDumpBitmapKM(psDeviceNode,
					  &psPDumpBitmapIN->szFileName[0],
					  psPDumpBitmapIN->ui32FileOffset,
					  psPDumpBitmapIN->ui32Width,
					  psPDumpBitmapIN->ui32Height,
					  psPDumpBitmapIN->ui32StrideInBytes,
					  psPDumpBitmapIN->sDevBaseAddr,
					  hDevMemContextInt,
					  psPDumpBitmapIN->ui32Size,
					  psPDumpBitmapIN->ePixelFormat,
					  psPDumpBitmapIN->eMemFormat,
					  psPDumpBitmapIN->ui32Flags);

	return 0;
}

static IMG_INT
PDumpReadRegBW(IMG_UINT32 ui32BridgeID,
			   PVRSRV_BRIDGE_IN_PDUMP_READREG *psPDumpReadRegIN,
			   PVRSRV_BRIDGE_RETURN *psRetOUT,
			   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DUMPREADREG);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, (IMG_VOID **)&psDeviceNode,
						   psPDumpReadRegIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);

	psRetOUT->eError =
		PDumpReadRegKM(&psPDumpReadRegIN->szRegRegion[0],
					   &psPDumpReadRegIN->szFileName[0],
					   psPDumpReadRegIN->ui32FileOffset,
					   psPDumpReadRegIN->ui32Address,
					   psPDumpReadRegIN->ui32Size,
					   psPDumpReadRegIN->ui32Flags);

	return 0;
}

static IMG_INT
PDumpDriverInfoBW(IMG_UINT32 ui32BridgeID,
				  PVRSRV_BRIDGE_IN_PDUMP_DRIVERINFO *psPDumpDriverInfoIN,
				  PVRSRV_BRIDGE_RETURN *psRetOUT,
				  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_UINT32 ui32PDumpFlags;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DRIVERINFO);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	ui32PDumpFlags = 0;
	if(psPDumpDriverInfoIN->bContinuous)
	{
		ui32PDumpFlags |= PDUMP_FLAGS_CONTINUOUS;
	}
	psRetOUT->eError =
		PDumpDriverInfoKM(&psPDumpDriverInfoIN->szString[0],
						  ui32PDumpFlags);

	return 0;
}

static IMG_INT
PDumpSyncDumpBW(IMG_UINT32 ui32BridgeID,
				PVRSRV_BRIDGE_IN_PDUMP_DUMPSYNC *psPDumpSyncDumpIN,
				PVRSRV_BRIDGE_RETURN *psRetOUT,
				PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_UINT32 ui32Bytes = psPDumpSyncDumpIN->ui32Bytes;
	IMG_VOID *pvSyncInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DUMPSYNC);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &pvSyncInfo,
						   psPDumpSyncDumpIN->psKernelSyncInfo,
						   PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PDumpMemUM(psPerProc,
				   psPDumpSyncDumpIN->pvAltLinAddr,
				   IMG_NULL,
				   ((PVRSRV_KERNEL_SYNC_INFO *)pvSyncInfo)->psSyncDataMemInfoKM,
				   psPDumpSyncDumpIN->ui32Offset,
				   ui32Bytes,
				   0,
				   MAKEUNIQUETAG(((PVRSRV_KERNEL_SYNC_INFO *)pvSyncInfo)->psSyncDataMemInfoKM));

	return 0;
}

static IMG_INT
PDumpSyncPolBW(IMG_UINT32 ui32BridgeID,
			   PVRSRV_BRIDGE_IN_PDUMP_SYNCPOL *psPDumpSyncPolIN,
			   PVRSRV_BRIDGE_RETURN *psRetOUT,
			   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_UINT32 ui32Offset;
	IMG_VOID *pvSyncInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_SYNCPOL);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &pvSyncInfo,
						   psPDumpSyncPolIN->psKernelSyncInfo,
						   PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	if(psPDumpSyncPolIN->bIsRead)
	{
		ui32Offset = offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete);
	}
	else
	{
		ui32Offset = offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete);
	}

	psRetOUT->eError =
		PDumpMemPolKM(((PVRSRV_KERNEL_SYNC_INFO *)pvSyncInfo)->psSyncDataMemInfoKM,
					  ui32Offset,
					  psPDumpSyncPolIN->ui32Value,
					  psPDumpSyncPolIN->ui32Mask,
					  PDUMP_POLL_OPERATOR_EQUAL,
					  0,
					  MAKEUNIQUETAG(((PVRSRV_KERNEL_SYNC_INFO *)pvSyncInfo)->psSyncDataMemInfoKM));

	return 0;
}


static IMG_INT
PDumpCycleCountRegReadBW(IMG_UINT32 ui32BridgeID,
						 PVRSRV_BRIDGE_IN_PDUMP_CYCLE_COUNT_REG_READ *psPDumpCycleCountRegReadIN,
						 PVRSRV_BRIDGE_RETURN *psRetOUT,
						 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_CYCLE_COUNT_REG_READ);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, 
						   (IMG_VOID **)&psDeviceNode, 
						   psPDumpCycleCountRegReadIN->hDevCookie, 
						   PVRSRV_HANDLE_TYPE_DEV_NODE);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	PDumpCycleCountRegRead(&psDeviceNode->sDevId,
						   psPDumpCycleCountRegReadIN->ui32RegOffset,
						   psPDumpCycleCountRegReadIN->bLastFrame);

	psRetOUT->eError = PVRSRV_OK;

	return 0;
}

static IMG_INT
PDumpPDDevPAddrBW(IMG_UINT32 ui32BridgeID,
				  PVRSRV_BRIDGE_IN_PDUMP_DUMPPDDEVPADDR *psPDumpPDDevPAddrIN,
				  PVRSRV_BRIDGE_RETURN *psRetOUT,
				  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DUMPPDDEVPADDR);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &pvMemInfo,
						   psPDumpPDDevPAddrIN->hKernelMemInfo,
						   PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PDumpPDDevPAddrKM((PVRSRV_KERNEL_MEM_INFO *)pvMemInfo,
						  psPDumpPDDevPAddrIN->ui32Offset,
						  psPDumpPDDevPAddrIN->sPDDevPAddr,
						  MAKEUNIQUETAG(pvMemInfo),
						  PDUMP_PD_UNIQUETAG);
	return 0;
}

static IMG_INT
PDumpStartInitPhaseBW(IMG_UINT32 ui32BridgeID,
					  IMG_VOID *psBridgeIn,
					  PVRSRV_BRIDGE_RETURN *psRetOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_STARTINITPHASE);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpStartInitPhaseKM();

	return 0;
}

static IMG_INT
PDumpStopInitPhaseBW(IMG_UINT32 ui32BridgeID,
					  IMG_VOID *psBridgeIn,
					  PVRSRV_BRIDGE_RETURN *psRetOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_STOPINITPHASE);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpStopInitPhaseKM();

	return 0;
}

#endif 


static IMG_INT
PVRSRVGetMiscInfoBW(IMG_UINT32 ui32BridgeID,
					PVRSRV_BRIDGE_IN_GET_MISC_INFO *psGetMiscInfoIN,
					PVRSRV_BRIDGE_OUT_GET_MISC_INFO *psGetMiscInfoOUT,
					PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_ERROR eError;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_MISC_INFO);

	OSMemCopy(&psGetMiscInfoOUT->sMiscInfo,
	          &psGetMiscInfoIN->sMiscInfo,
	          sizeof(PVRSRV_MISC_INFO));

	if (((psGetMiscInfoIN->sMiscInfo.ui32StateRequest & PVRSRV_MISC_INFO_MEMSTATS_PRESENT) != 0) &&
	    ((psGetMiscInfoIN->sMiscInfo.ui32StateRequest & PVRSRV_MISC_INFO_DDKVERSION_PRESENT) != 0) &&
	    ((psGetMiscInfoIN->sMiscInfo.ui32StateRequest & PVRSRV_MISC_INFO_FREEMEM_PRESENT) != 0))
	{
		
		psGetMiscInfoOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}

	if (((psGetMiscInfoIN->sMiscInfo.ui32StateRequest & PVRSRV_MISC_INFO_MEMSTATS_PRESENT) != 0) ||
	    ((psGetMiscInfoIN->sMiscInfo.ui32StateRequest & PVRSRV_MISC_INFO_DDKVERSION_PRESENT) != 0) ||
	    ((psGetMiscInfoIN->sMiscInfo.ui32StateRequest & PVRSRV_MISC_INFO_FREEMEM_PRESENT) != 0))
	{
		
		ASSIGN_AND_EXIT_ON_ERROR(psGetMiscInfoOUT->eError,
				    OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		                    psGetMiscInfoOUT->sMiscInfo.ui32MemoryStrLen,
		                    (IMG_VOID **)&psGetMiscInfoOUT->sMiscInfo.pszMemoryStr, 0,
							"Output string buffer"));

		psGetMiscInfoOUT->eError = PVRSRVGetMiscInfoKM(&psGetMiscInfoOUT->sMiscInfo);

		
		eError = CopyToUserWrapper(psPerProc, ui32BridgeID,
		                           psGetMiscInfoIN->sMiscInfo.pszMemoryStr,
		                           psGetMiscInfoOUT->sMiscInfo.pszMemoryStr,
		                           psGetMiscInfoOUT->sMiscInfo.ui32MemoryStrLen);

		
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		          psGetMiscInfoOUT->sMiscInfo.ui32MemoryStrLen,
		         (IMG_VOID *)psGetMiscInfoOUT->sMiscInfo.pszMemoryStr, 0);
		psGetMiscInfoOUT->sMiscInfo.pszMemoryStr = IMG_NULL;

		
		psGetMiscInfoOUT->sMiscInfo.pszMemoryStr = psGetMiscInfoIN->sMiscInfo.pszMemoryStr;

		if(eError != PVRSRV_OK)
		{
			
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetMiscInfoBW Error copy to user"));
			return -EFAULT;
		}
	}
	else
	{
		psGetMiscInfoOUT->eError = PVRSRVGetMiscInfoKM(&psGetMiscInfoOUT->sMiscInfo);
	}

	
	if (psGetMiscInfoOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	if (psGetMiscInfoIN->sMiscInfo.ui32StateRequest & PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT)
	{
		psGetMiscInfoOUT->eError = PVRSRVAllocHandle(psPerProc->psHandleBase,
													&psGetMiscInfoOUT->sMiscInfo.sGlobalEventObject.hOSEventKM,
													psGetMiscInfoOUT->sMiscInfo.sGlobalEventObject.hOSEventKM,
													PVRSRV_HANDLE_TYPE_SHARED_EVENT_OBJECT,
													PVRSRV_HANDLE_ALLOC_FLAG_SHARED);

			if (psGetMiscInfoOUT->eError != PVRSRV_OK)
			{
				return 0;
			}
	}

	if (psGetMiscInfoOUT->sMiscInfo.hSOCTimerRegisterOSMemHandle)
	{
		
		psGetMiscInfoOUT->eError = PVRSRVAllocHandle(psPerProc->psHandleBase,
						  &psGetMiscInfoOUT->sMiscInfo.hSOCTimerRegisterOSMemHandle,
						  psGetMiscInfoOUT->sMiscInfo.hSOCTimerRegisterOSMemHandle,
						  PVRSRV_HANDLE_TYPE_SOC_TIMER,
						  PVRSRV_HANDLE_ALLOC_FLAG_SHARED);

		if (psGetMiscInfoOUT->eError != PVRSRV_OK)
		{
			return 0;
		}
	}

	return 0;
}

static IMG_INT
PVRSRVConnectBW(IMG_UINT32 ui32BridgeID,
				PVRSRV_BRIDGE_IN_CONNECT_SERVICES *psConnectServicesIN,
				PVRSRV_BRIDGE_OUT_CONNECT_SERVICES *psConnectServicesOUT,
				PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CONNECT_SERVICES);

#if defined(PDUMP)
	
	psPerProc->bPDumpPersistent |= ( (psConnectServicesIN->ui32Flags & SRV_FLAGS_PERSIST) != 0) ? IMG_TRUE : IMG_FALSE;

#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	
	psPerProc->bPDumpActive |= ( (psConnectServicesIN->ui32Flags & SRV_FLAGS_PDUMP_ACTIVE) != 0) ? IMG_TRUE : IMG_FALSE;
#endif 
#else
	PVR_UNREFERENCED_PARAMETER(psConnectServicesIN);
#endif
	psConnectServicesOUT->hKernelServices = psPerProc->hPerProcData;
	psConnectServicesOUT->eError = PVRSRV_OK;

	return 0;
}

static IMG_INT
PVRSRVDisconnectBW(IMG_UINT32 ui32BridgeID,
				   IMG_VOID *psBridgeIn,
				   PVRSRV_BRIDGE_RETURN *psRetOUT,
				   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psPerProc);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_DISCONNECT_SERVICES);

	
	psRetOUT->eError = PVRSRV_OK;

	return 0;
}

static IMG_INT
PVRSRVEnumerateDCBW(IMG_UINT32 ui32BridgeID,
					PVRSRV_BRIDGE_IN_ENUMCLASS *psEnumDispClassIN,
					PVRSRV_BRIDGE_OUT_ENUMCLASS *psEnumDispClassOUT,
					PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ENUM_CLASS);

	psEnumDispClassOUT->eError =
		PVRSRVEnumerateDCKM(psEnumDispClassIN->sDeviceClass,
							&psEnumDispClassOUT->ui32NumDevices,
							&psEnumDispClassOUT->ui32DevID[0]);

	return 0;
}

static IMG_INT
PVRSRVOpenDCDeviceBW(IMG_UINT32 ui32BridgeID,
					 PVRSRV_BRIDGE_IN_OPEN_DISPCLASS_DEVICE *psOpenDispClassDeviceIN,
					 PVRSRV_BRIDGE_OUT_OPEN_DISPCLASS_DEVICE *psOpenDispClassDeviceOUT,
					 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	IMG_HANDLE hDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_OPEN_DISPCLASS_DEVICE);

	NEW_HANDLE_BATCH_OR_ERROR(psOpenDispClassDeviceOUT->eError, psPerProc, 1)

	psOpenDispClassDeviceOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &hDevCookieInt,
						   psOpenDispClassDeviceIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);
	if(psOpenDispClassDeviceOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psOpenDispClassDeviceOUT->eError =
		PVRSRVOpenDCDeviceKM(psPerProc,
							 psOpenDispClassDeviceIN->ui32DeviceID,
							 hDevCookieInt,
							 &hDispClassInfoInt);

	if(psOpenDispClassDeviceOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					  &psOpenDispClassDeviceOUT->hDeviceKM,
					  hDispClassInfoInt,
					  PVRSRV_HANDLE_TYPE_DISP_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	COMMIT_HANDLE_BATCH_OR_ERROR(psOpenDispClassDeviceOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVCloseDCDeviceBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_CLOSE_DISPCLASS_DEVICE *psCloseDispClassDeviceIN,
					  PVRSRV_BRIDGE_RETURN *psRetOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CLOSE_DISPCLASS_DEVICE);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfoInt,
						   psCloseDispClassDeviceIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PVRSRVCloseDCDeviceKM(pvDispClassInfoInt, IMG_FALSE);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVReleaseHandle(psPerProc->psHandleBase,
							psCloseDispClassDeviceIN->hDeviceKM,
							PVRSRV_HANDLE_TYPE_DISP_INFO);
	return 0;
}

static IMG_INT
PVRSRVEnumDCFormatsBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_FORMATS *psEnumDispClassFormatsIN,
					  PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_FORMATS *psEnumDispClassFormatsOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ENUM_DISPCLASS_FORMATS);

	psEnumDispClassFormatsOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfoInt,
						   psEnumDispClassFormatsIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psEnumDispClassFormatsOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psEnumDispClassFormatsOUT->eError =
		PVRSRVEnumDCFormatsKM(pvDispClassInfoInt,
							  &psEnumDispClassFormatsOUT->ui32Count,
							  psEnumDispClassFormatsOUT->asFormat);

	return 0;
}

static IMG_INT
PVRSRVEnumDCDimsBW(IMG_UINT32 ui32BridgeID,
				   PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_DIMS *psEnumDispClassDimsIN,
				   PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_DIMS *psEnumDispClassDimsOUT,
				   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ENUM_DISPCLASS_DIMS);

	psEnumDispClassDimsOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfoInt,
						   psEnumDispClassDimsIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);

	if(psEnumDispClassDimsOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psEnumDispClassDimsOUT->eError =
		PVRSRVEnumDCDimsKM(pvDispClassInfoInt,
						   &psEnumDispClassDimsIN->sFormat,
						   &psEnumDispClassDimsOUT->ui32Count,
						   psEnumDispClassDimsOUT->asDim);

	return 0;
}

static IMG_INT
PVRSRVGetDCSystemBufferBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_GET_DISPCLASS_SYSBUFFER *psGetDispClassSysBufferIN,  
						  PVRSRV_BRIDGE_OUT_GET_DISPCLASS_SYSBUFFER *psGetDispClassSysBufferOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hBufferInt;
	IMG_VOID *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_DISPCLASS_SYSBUFFER);

	NEW_HANDLE_BATCH_OR_ERROR(psGetDispClassSysBufferOUT->eError, psPerProc, 1)

	psGetDispClassSysBufferOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfoInt,
						   psGetDispClassSysBufferIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psGetDispClassSysBufferOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetDispClassSysBufferOUT->eError =
		PVRSRVGetDCSystemBufferKM(pvDispClassInfoInt,
								  &hBufferInt);

	if(psGetDispClassSysBufferOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	 
	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
						 &psGetDispClassSysBufferOUT->hBuffer,
						 hBufferInt,
						 PVRSRV_HANDLE_TYPE_DISP_BUFFER,
						 (PVRSRV_HANDLE_ALLOC_FLAG)(PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE | PVRSRV_HANDLE_ALLOC_FLAG_SHARED),
						 psGetDispClassSysBufferIN->hDeviceKM);

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetDispClassSysBufferOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVGetDCInfoBW(IMG_UINT32 ui32BridgeID,
				  PVRSRV_BRIDGE_IN_GET_DISPCLASS_INFO *psGetDispClassInfoIN,
				  PVRSRV_BRIDGE_OUT_GET_DISPCLASS_INFO *psGetDispClassInfoOUT,
				  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_DISPCLASS_INFO);

	psGetDispClassInfoOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psGetDispClassInfoIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psGetDispClassInfoOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetDispClassInfoOUT->eError =
		PVRSRVGetDCInfoKM(pvDispClassInfo,
						  &psGetDispClassInfoOUT->sDisplayInfo);

	return 0;
}

static IMG_INT
PVRSRVCreateDCSwapChainBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_CREATE_DISPCLASS_SWAPCHAIN *psCreateDispClassSwapChainIN,
						  PVRSRV_BRIDGE_OUT_CREATE_DISPCLASS_SWAPCHAIN *psCreateDispClassSwapChainOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_HANDLE hSwapChainInt;
	IMG_UINT32	ui32SwapChainID;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CREATE_DISPCLASS_SWAPCHAIN);

	NEW_HANDLE_BATCH_OR_ERROR(psCreateDispClassSwapChainOUT->eError, psPerProc, 1)

	psCreateDispClassSwapChainOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psCreateDispClassSwapChainIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);

	if(psCreateDispClassSwapChainOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	ui32SwapChainID = psCreateDispClassSwapChainIN->ui32SwapChainID;

	psCreateDispClassSwapChainOUT->eError =
		PVRSRVCreateDCSwapChainKM(psPerProc, pvDispClassInfo,
								  psCreateDispClassSwapChainIN->ui32Flags,
								  &psCreateDispClassSwapChainIN->sDstSurfAttrib,
								  &psCreateDispClassSwapChainIN->sSrcSurfAttrib,
								  psCreateDispClassSwapChainIN->ui32BufferCount,
								  psCreateDispClassSwapChainIN->ui32OEMFlags,
								  &hSwapChainInt,
								  &ui32SwapChainID);

	if(psCreateDispClassSwapChainOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	
	psCreateDispClassSwapChainOUT->ui32SwapChainID = ui32SwapChainID;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
					  &psCreateDispClassSwapChainOUT->hSwapChain,
					  hSwapChainInt,
					  PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE,
					  psCreateDispClassSwapChainIN->hDeviceKM);

	COMMIT_HANDLE_BATCH_OR_ERROR(psCreateDispClassSwapChainOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVDestroyDCSwapChainBW(IMG_UINT32 ui32BridgeID,
						   PVRSRV_BRIDGE_IN_DESTROY_DISPCLASS_SWAPCHAIN *psDestroyDispClassSwapChainIN,
						   PVRSRV_BRIDGE_RETURN *psRetOUT,
						   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_DESTROY_DISPCLASS_SWAPCHAIN);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &pvSwapChain,
						   psDestroyDispClassSwapChainIN->hSwapChain,
						   PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVDestroyDCSwapChainKM(pvSwapChain);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVReleaseHandle(psPerProc->psHandleBase,
							psDestroyDispClassSwapChainIN->hSwapChain,
							PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);

	return 0;
}

static IMG_INT
PVRSRVSetDCDstRectBW(IMG_UINT32 ui32BridgeID,
					 PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT *psSetDispClassDstRectIN,
					 PVRSRV_BRIDGE_RETURN *psRetOUT,
					 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_VOID *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SET_DISPCLASS_DSTRECT);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psSetDispClassDstRectIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvSwapChain,
						   psSetDispClassDstRectIN->hSwapChain,
						   PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVSetDCDstRectKM(pvDispClassInfo,
							 pvSwapChain,
							 &psSetDispClassDstRectIN->sRect);

	return 0;
}

static IMG_INT
PVRSRVSetDCSrcRectBW(IMG_UINT32 ui32BridgeID,
					 PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT *psSetDispClassSrcRectIN,
					 PVRSRV_BRIDGE_RETURN *psRetOUT,
					 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_VOID *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SET_DISPCLASS_SRCRECT);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psSetDispClassSrcRectIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvSwapChain,
						   psSetDispClassSrcRectIN->hSwapChain,
						   PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVSetDCSrcRectKM(pvDispClassInfo,
							 pvSwapChain,
							 &psSetDispClassSrcRectIN->sRect);

	return 0;
}

static IMG_INT
PVRSRVSetDCDstColourKeyBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY *psSetDispClassColKeyIN,
						  PVRSRV_BRIDGE_RETURN *psRetOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_VOID *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SET_DISPCLASS_DSTCOLOURKEY);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psSetDispClassColKeyIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvSwapChain,
						   psSetDispClassColKeyIN->hSwapChain,
						   PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVSetDCDstColourKeyKM(pvDispClassInfo,
								  pvSwapChain,
								  psSetDispClassColKeyIN->ui32CKColour);

	return 0;
}

static IMG_INT
PVRSRVSetDCSrcColourKeyBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY *psSetDispClassColKeyIN,
						  PVRSRV_BRIDGE_RETURN *psRetOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_VOID *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SET_DISPCLASS_SRCCOLOURKEY);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psSetDispClassColKeyIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvSwapChain,
						   psSetDispClassColKeyIN->hSwapChain,
						   PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVSetDCSrcColourKeyKM(pvDispClassInfo,
								  pvSwapChain,
								  psSetDispClassColKeyIN->ui32CKColour);

	return 0;
}

static IMG_INT
PVRSRVGetDCBuffersBW(IMG_UINT32 ui32BridgeID,
					 PVRSRV_BRIDGE_IN_GET_DISPCLASS_BUFFERS *psGetDispClassBuffersIN,
					 PVRSRV_BRIDGE_OUT_GET_DISPCLASS_BUFFERS *psGetDispClassBuffersOUT,
					 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_VOID *pvSwapChain;
	IMG_UINT32 i;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_DISPCLASS_BUFFERS);

	NEW_HANDLE_BATCH_OR_ERROR(psGetDispClassBuffersOUT->eError, psPerProc, PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS)

	psGetDispClassBuffersOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psGetDispClassBuffersIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psGetDispClassBuffersOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetDispClassBuffersOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvSwapChain,
						   psGetDispClassBuffersIN->hSwapChain,
						   PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if(psGetDispClassBuffersOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetDispClassBuffersOUT->eError =
		PVRSRVGetDCBuffersKM(pvDispClassInfo,
							 pvSwapChain,
							 &psGetDispClassBuffersOUT->ui32BufferCount,
							 psGetDispClassBuffersOUT->ahBuffer);
	if (psGetDispClassBuffersOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	PVR_ASSERT(psGetDispClassBuffersOUT->ui32BufferCount <= PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS);

	for(i = 0; i < psGetDispClassBuffersOUT->ui32BufferCount; i++)
	{
		IMG_HANDLE hBufferExt;

		 
		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
							 &hBufferExt,
							 psGetDispClassBuffersOUT->ahBuffer[i],
							 PVRSRV_HANDLE_TYPE_DISP_BUFFER,
							 (PVRSRV_HANDLE_ALLOC_FLAG)(PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE | PVRSRV_HANDLE_ALLOC_FLAG_SHARED),
							 psGetDispClassBuffersIN->hSwapChain);

		psGetDispClassBuffersOUT->ahBuffer[i] = hBufferExt;
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetDispClassBuffersOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVSwapToDCBufferBW(IMG_UINT32 ui32BridgeID,
					   PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER *psSwapDispClassBufferIN,
					   PVRSRV_BRIDGE_RETURN *psRetOUT,
					   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_VOID *pvSwapChainBuf;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psSwapDispClassBufferIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupSubHandle(psPerProc->psHandleBase,
						   &pvSwapChainBuf,
						   psSwapDispClassBufferIN->hBuffer,
						   PVRSRV_HANDLE_TYPE_DISP_BUFFER,
						   psSwapDispClassBufferIN->hDeviceKM);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVSwapToDCBufferKM(pvDispClassInfo,
							   pvSwapChainBuf,
							   psSwapDispClassBufferIN->ui32SwapInterval,
							   psSwapDispClassBufferIN->hPrivateTag,
							   psSwapDispClassBufferIN->ui32ClipRectCount,
							   psSwapDispClassBufferIN->sClipRect);

	return 0;
}

static IMG_INT
PVRSRVSwapToDCSystemBW(IMG_UINT32 ui32BridgeID,
					   PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_SYSTEM *psSwapDispClassSystemIN,
					   PVRSRV_BRIDGE_RETURN *psRetOUT,
					   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvDispClassInfo;
	IMG_VOID *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_SYSTEM);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvDispClassInfo,
						   psSwapDispClassSystemIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_DISP_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVLookupSubHandle(psPerProc->psHandleBase,
						   &pvSwapChain,
						   psSwapDispClassSystemIN->hSwapChain,
						   PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN,
						   psSwapDispClassSystemIN->hDeviceKM);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}
	psRetOUT->eError =
		PVRSRVSwapToDCSystemKM(pvDispClassInfo,
							   pvSwapChain);

	return 0;
}

static IMG_INT
PVRSRVOpenBCDeviceBW(IMG_UINT32 ui32BridgeID,
					 PVRSRV_BRIDGE_IN_OPEN_BUFFERCLASS_DEVICE *psOpenBufferClassDeviceIN,
					 PVRSRV_BRIDGE_OUT_OPEN_BUFFERCLASS_DEVICE *psOpenBufferClassDeviceOUT,
					 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevCookieInt;
	IMG_HANDLE hBufClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_OPEN_BUFFERCLASS_DEVICE);

	NEW_HANDLE_BATCH_OR_ERROR(psOpenBufferClassDeviceOUT->eError, psPerProc, 1)

	psOpenBufferClassDeviceOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &hDevCookieInt,
						   psOpenBufferClassDeviceIN->hDevCookie,
						   PVRSRV_HANDLE_TYPE_DEV_NODE);
	if(psOpenBufferClassDeviceOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psOpenBufferClassDeviceOUT->eError =
		PVRSRVOpenBCDeviceKM(psPerProc,
							 psOpenBufferClassDeviceIN->ui32DeviceID,
							 hDevCookieInt,
							 &hBufClassInfo);
	if(psOpenBufferClassDeviceOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					  &psOpenBufferClassDeviceOUT->hDeviceKM,
					  hBufClassInfo,
					  PVRSRV_HANDLE_TYPE_BUF_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	COMMIT_HANDLE_BATCH_OR_ERROR(psOpenBufferClassDeviceOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVCloseBCDeviceBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_CLOSE_BUFFERCLASS_DEVICE *psCloseBufferClassDeviceIN,
					  PVRSRV_BRIDGE_RETURN *psRetOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvBufClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CLOSE_BUFFERCLASS_DEVICE);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvBufClassInfo,
						   psCloseBufferClassDeviceIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_BUF_INFO);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError =
		PVRSRVCloseBCDeviceKM(pvBufClassInfo, IMG_FALSE);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
										   psCloseBufferClassDeviceIN->hDeviceKM,
										   PVRSRV_HANDLE_TYPE_BUF_INFO);

	return 0;
}

static IMG_INT
PVRSRVGetBCInfoBW(IMG_UINT32 ui32BridgeID,
				  PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_INFO *psGetBufferClassInfoIN,
				  PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_INFO *psGetBufferClassInfoOUT,
				  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvBufClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_BUFFERCLASS_INFO);

	psGetBufferClassInfoOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvBufClassInfo,
						   psGetBufferClassInfoIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_BUF_INFO);
	if(psGetBufferClassInfoOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetBufferClassInfoOUT->eError =
		PVRSRVGetBCInfoKM(pvBufClassInfo,
						  &psGetBufferClassInfoOUT->sBufferInfo);
	return 0;
}

static IMG_INT
PVRSRVGetBCBufferBW(IMG_UINT32 ui32BridgeID,
					PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_BUFFER *psGetBufferClassBufferIN,
					PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_BUFFER *psGetBufferClassBufferOUT,
					PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_VOID *pvBufClassInfo;
	IMG_HANDLE hBufferInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_BUFFERCLASS_BUFFER);

	NEW_HANDLE_BATCH_OR_ERROR(psGetBufferClassBufferOUT->eError, psPerProc, 1)

	psGetBufferClassBufferOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &pvBufClassInfo,
						   psGetBufferClassBufferIN->hDeviceKM,
						   PVRSRV_HANDLE_TYPE_BUF_INFO);
	if(psGetBufferClassBufferOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetBufferClassBufferOUT->eError =
		PVRSRVGetBCBufferKM(pvBufClassInfo,
							psGetBufferClassBufferIN->ui32BufferIndex,
							&hBufferInt);

	if(psGetBufferClassBufferOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	 
	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
						 &psGetBufferClassBufferOUT->hBuffer,
						 hBufferInt,
						 PVRSRV_HANDLE_TYPE_BUF_BUFFER,
						 (PVRSRV_HANDLE_ALLOC_FLAG)(PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE |  PVRSRV_HANDLE_ALLOC_FLAG_SHARED),
						 psGetBufferClassBufferIN->hDeviceKM);

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetBufferClassBufferOUT->eError, psPerProc)

	return 0;
}


static IMG_INT
PVRSRVAllocSharedSysMemoryBW(IMG_UINT32 ui32BridgeID,
							 PVRSRV_BRIDGE_IN_ALLOC_SHARED_SYS_MEM *psAllocSharedSysMemIN,
							 PVRSRV_BRIDGE_OUT_ALLOC_SHARED_SYS_MEM *psAllocSharedSysMemOUT,
							 PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ALLOC_SHARED_SYS_MEM);

	NEW_HANDLE_BATCH_OR_ERROR(psAllocSharedSysMemOUT->eError, psPerProc, 1)

	psAllocSharedSysMemOUT->eError =
		PVRSRVAllocSharedSysMemoryKM(psPerProc,
									 psAllocSharedSysMemIN->ui32Flags,
									 psAllocSharedSysMemIN->ui32Size,
									 &psKernelMemInfo);
	if(psAllocSharedSysMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	OSMemSet(&psAllocSharedSysMemOUT->sClientMemInfo,
			 0,
			 sizeof(psAllocSharedSysMemOUT->sClientMemInfo));

	psAllocSharedSysMemOUT->sClientMemInfo.pvLinAddrKM =
			psKernelMemInfo->pvLinAddrKM;

	psAllocSharedSysMemOUT->sClientMemInfo.pvLinAddr = 0;
	psAllocSharedSysMemOUT->sClientMemInfo.ui32Flags =
		psKernelMemInfo->ui32Flags;
	psAllocSharedSysMemOUT->sClientMemInfo.ui32AllocSize =
		psKernelMemInfo->ui32AllocSize;
	psAllocSharedSysMemOUT->sClientMemInfo.hMappingInfo = psKernelMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					  &psAllocSharedSysMemOUT->sClientMemInfo.hKernelMemInfo,
					  psKernelMemInfo,
					  PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO,
					  PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	COMMIT_HANDLE_BATCH_OR_ERROR(psAllocSharedSysMemOUT->eError, psPerProc)

	return 0;
}

static IMG_INT
PVRSRVFreeSharedSysMemoryBW(IMG_UINT32 ui32BridgeID,
							PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM *psFreeSharedSysMemIN,
							PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM *psFreeSharedSysMemOUT,
							PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM);

	psFreeSharedSysMemOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   (IMG_VOID **)&psKernelMemInfo,
						   psFreeSharedSysMemIN->psKernelMemInfo,
																   PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);

	if(psFreeSharedSysMemOUT->eError != PVRSRV_OK)
		return 0;

	psFreeSharedSysMemOUT->eError =
		PVRSRVFreeSharedSysMemoryKM(psKernelMemInfo);
	if(psFreeSharedSysMemOUT->eError != PVRSRV_OK)
		return 0;

	psFreeSharedSysMemOUT->eError =
		PVRSRVReleaseHandle(psPerProc->psHandleBase,
							psFreeSharedSysMemIN->psKernelMemInfo,
							PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);
	return 0;
}

static IMG_INT
PVRSRVMapMemInfoMemBW(IMG_UINT32 ui32BridgeID,
					  PVRSRV_BRIDGE_IN_MAP_MEMINFO_MEM *psMapMemInfoMemIN,
					  PVRSRV_BRIDGE_OUT_MAP_MEMINFO_MEM *psMapMemInfoMemOUT,
					  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;
	PVRSRV_HANDLE_TYPE eHandleType;
	IMG_HANDLE	hParent;
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MAP_MEMINFO_MEM);

	NEW_HANDLE_BATCH_OR_ERROR(psMapMemInfoMemOUT->eError, psPerProc, 2)

	psMapMemInfoMemOUT->eError =
		PVRSRVLookupHandleAnyType(psPerProc->psHandleBase,
						   (IMG_VOID **)&psKernelMemInfo,
						   &eHandleType,
						   psMapMemInfoMemIN->hKernelMemInfo);
	if(psMapMemInfoMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	switch (eHandleType)
	{
#if defined(PVR_SECURE_HANDLES)
		case PVRSRV_HANDLE_TYPE_MEM_INFO:
		case PVRSRV_HANDLE_TYPE_MEM_INFO_REF:
		case PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO:
#else
		case PVRSRV_HANDLE_TYPE_NONE:
#endif
			break;
		default:
			psMapMemInfoMemOUT->eError = PVRSRV_ERROR_INVALID_HANDLE_TYPE;
			return 0;
	}

	
	psMapMemInfoMemOUT->eError =
		PVRSRVGetParentHandle(psPerProc->psHandleBase,
					&hParent,
					psMapMemInfoMemIN->hKernelMemInfo,
					eHandleType);
	if (psMapMemInfoMemOUT->eError != PVRSRV_OK)
	{
		return 0;
	}
	if (hParent == IMG_NULL)
	{
		hParent = psMapMemInfoMemIN->hKernelMemInfo;
	}

	OSMemSet(&psMapMemInfoMemOUT->sClientMemInfo,
			 0,
			 sizeof(psMapMemInfoMemOUT->sClientMemInfo));

	psMapMemInfoMemOUT->sClientMemInfo.pvLinAddrKM =
			psKernelMemInfo->pvLinAddrKM;

	psMapMemInfoMemOUT->sClientMemInfo.pvLinAddr = 0;
	psMapMemInfoMemOUT->sClientMemInfo.sDevVAddr =
		psKernelMemInfo->sDevVAddr;
	psMapMemInfoMemOUT->sClientMemInfo.ui32Flags =
		psKernelMemInfo->ui32Flags;
	psMapMemInfoMemOUT->sClientMemInfo.ui32AllocSize =
		psKernelMemInfo->ui32AllocSize;
	psMapMemInfoMemOUT->sClientMemInfo.hMappingInfo = psKernelMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
					  &psMapMemInfoMemOUT->sClientMemInfo.hKernelMemInfo,
					  psKernelMemInfo,
					  PVRSRV_HANDLE_TYPE_MEM_INFO_REF,
					  PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
					  hParent);

	if(psKernelMemInfo->ui32Flags & PVRSRV_MEM_NO_SYNCOBJ)
	{
		
		OSMemSet(&psMapMemInfoMemOUT->sClientSyncInfo,
				 0,
				 sizeof (PVRSRV_CLIENT_SYNC_INFO));
	}
	else
	{
		
		psMapMemInfoMemOUT->sClientSyncInfo.psSyncData =
			psKernelMemInfo->psKernelSyncInfo->psSyncData;
		psMapMemInfoMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
			psKernelMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
		psMapMemInfoMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
			psKernelMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

		psMapMemInfoMemOUT->sClientSyncInfo.hMappingInfo =
			psKernelMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->sMemBlk.hOSMemHandle;

		psMapMemInfoMemOUT->sClientMemInfo.psClientSyncInfo = &psMapMemInfoMemOUT->sClientSyncInfo;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
							 &psMapMemInfoMemOUT->sClientSyncInfo.hKernelSyncInfo,
							 psKernelMemInfo->psKernelSyncInfo,
							 PVRSRV_HANDLE_TYPE_SYNC_INFO,
							 PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
							 psMapMemInfoMemOUT->sClientMemInfo.hKernelMemInfo);
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psMapMemInfoMemOUT->eError, psPerProc)

	return 0;
}



static IMG_INT
MMU_GetPDDevPAddrBW(IMG_UINT32 ui32BridgeID,
					PVRSRV_BRIDGE_IN_GETMMU_PD_DEVPADDR *psGetMmuPDDevPAddrIN,
					PVRSRV_BRIDGE_OUT_GETMMU_PD_DEVPADDR *psGetMmuPDDevPAddrOUT,
					PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hDevMemContextInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GETMMU_PD_DEVPADDR);

	psGetMmuPDDevPAddrOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
						   psGetMmuPDDevPAddrIN->hDevMemContext,
						   PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);
	if(psGetMmuPDDevPAddrOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psGetMmuPDDevPAddrOUT->sPDDevPAddr =
		BM_GetDeviceNode(hDevMemContextInt)->pfnMMUGetPDDevPAddr(BM_GetMMUContextFromMemContext(hDevMemContextInt));
	if(psGetMmuPDDevPAddrOUT->sPDDevPAddr.uiAddr)
	{
		psGetMmuPDDevPAddrOUT->eError = PVRSRV_OK;
	}
	else
	{
		psGetMmuPDDevPAddrOUT->eError = PVRSRV_ERROR_INVALID_PHYS_ADDR;
	}
	return 0;
}



IMG_INT
DummyBW(IMG_UINT32 ui32BridgeID,
		IMG_VOID *psBridgeIn,
		IMG_VOID *psBridgeOut,
		PVRSRV_PER_PROCESS_DATA *psPerProc)
{
#if !defined(DEBUG)
	PVR_UNREFERENCED_PARAMETER(ui32BridgeID);
#endif
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psBridgeOut);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

#if defined(DEBUG_BRIDGE_KM)
	PVR_DPF((PVR_DBG_ERROR, "%s: BRIDGE ERROR: BridgeID %u (%s) mapped to "
			 "Dummy Wrapper (probably not what you want!)",
			 __FUNCTION__, ui32BridgeID, g_BridgeDispatchTable[ui32BridgeID].pszIOCName));
#else
	PVR_DPF((PVR_DBG_ERROR, "%s: BRIDGE ERROR: BridgeID %u mapped to "
			 "Dummy Wrapper (probably not what you want!)",
			 __FUNCTION__, ui32BridgeID));
#endif
	return -ENOTTY;
}


IMG_VOID
_SetDispatchTableEntry(IMG_UINT32 ui32Index,
					   const IMG_CHAR *pszIOCName,
					   BridgeWrapperFunction pfFunction,
					   const IMG_CHAR *pszFunctionName)
{
	static IMG_UINT32 ui32PrevIndex = ~0UL;		
#if !defined(DEBUG)
	PVR_UNREFERENCED_PARAMETER(pszIOCName);
#endif
#if !defined(DEBUG_BRIDGE_KM_DISPATCH_TABLE) && !defined(DEBUG_BRIDGE_KM)
	PVR_UNREFERENCED_PARAMETER(pszFunctionName);
#endif

#if defined(DEBUG_BRIDGE_KM_DISPATCH_TABLE)
	
	PVR_DPF((PVR_DBG_WARNING, "%s: %d %s %s", __FUNCTION__, ui32Index, pszIOCName, pszFunctionName));
#endif

	
	if(g_BridgeDispatchTable[ui32Index].pfFunction)
	{
#if defined(DEBUG_BRIDGE_KM)
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: BUG!: Adding dispatch table entry for %s clobbers an existing entry for %s",
				 __FUNCTION__, pszIOCName, g_BridgeDispatchTable[ui32Index].pszIOCName));
#else
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: BUG!: Adding dispatch table entry for %s clobbers an existing entry (index=%u)",
				 __FUNCTION__, pszIOCName, ui32Index));
#endif
		PVR_DPF((PVR_DBG_ERROR, "NOTE: Enabling DEBUG_BRIDGE_KM_DISPATCH_TABLE may help debug this issue."));
	}

	
	if((ui32PrevIndex != ~0UL) &&
	   ((ui32Index >= ui32PrevIndex + DISPATCH_TABLE_GAP_THRESHOLD) ||
		(ui32Index <= ui32PrevIndex)))
	{
#if defined(DEBUG_BRIDGE_KM)
		PVR_DPF((PVR_DBG_WARNING,
				 "%s: There is a gap in the dispatch table between indices %u (%s) and %u (%s)",
				 __FUNCTION__, ui32PrevIndex, g_BridgeDispatchTable[ui32PrevIndex].pszIOCName,
				 ui32Index, pszIOCName));
#else
		PVR_DPF((PVR_DBG_WARNING,
				 "%s: There is a gap in the dispatch table between indices %u and %u (%s)",
				 __FUNCTION__, (IMG_UINT)ui32PrevIndex, (IMG_UINT)ui32Index, pszIOCName));
#endif
		PVR_DPF((PVR_DBG_ERROR, "NOTE: Enabling DEBUG_BRIDGE_KM_DISPATCH_TABLE may help debug this issue."));
	}

	g_BridgeDispatchTable[ui32Index].pfFunction = pfFunction;
#if defined(DEBUG_BRIDGE_KM)
	g_BridgeDispatchTable[ui32Index].pszIOCName = pszIOCName;
	g_BridgeDispatchTable[ui32Index].pszFunctionName = pszFunctionName;
	g_BridgeDispatchTable[ui32Index].ui32CallCount = 0;
	g_BridgeDispatchTable[ui32Index].ui32CopyFromUserTotalBytes = 0;
#endif

	ui32PrevIndex = ui32Index;
}

static IMG_INT
PVRSRVInitSrvConnectBW(IMG_UINT32 ui32BridgeID,
					   IMG_VOID *psBridgeIn,
					   PVRSRV_BRIDGE_RETURN *psRetOUT,
					   PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_INITSRV_CONNECT);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	 
	if((OSProcHasPrivSrvInit() == IMG_FALSE) || PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RUNNING) || PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RAN))
	{
		psRetOUT->eError = PVRSRV_ERROR_SRV_CONNECT_FAILED;
		return 0;
	}

#if defined (__linux__)
	PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_RUNNING, IMG_TRUE);
#endif
	psPerProc->bInitProcess = IMG_TRUE;

	psRetOUT->eError = PVRSRV_OK;

	return 0;
}


static IMG_INT
PVRSRVInitSrvDisconnectBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_INITSRV_DISCONNECT *psInitSrvDisconnectIN,
						  PVRSRV_BRIDGE_RETURN *psRetOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_INITSRV_DISCONNECT);

	if(!psPerProc->bInitProcess)
	{
		psRetOUT->eError = PVRSRV_ERROR_SRV_DISCONNECT_FAILED;
		return 0;
	}

	psPerProc->bInitProcess = IMG_FALSE;

	PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_RUNNING, IMG_FALSE);
	PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_RAN, IMG_TRUE);

	psRetOUT->eError = PVRSRVFinaliseSystem(psInitSrvDisconnectIN->bInitSuccesful);

	PVRSRVSetInitServerState( PVRSRV_INIT_SERVER_SUCCESSFUL ,
				((psRetOUT->eError == PVRSRV_OK) && (psInitSrvDisconnectIN->bInitSuccesful))
				? IMG_TRUE : IMG_FALSE);

	return 0;
}


static IMG_INT
PVRSRVEventObjectWaitBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_EVENT_OBJECT_WAIT *psEventObjectWaitIN,
						  PVRSRV_BRIDGE_RETURN *psRetOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hOSEventKM;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_EVENT_OBJECT_WAIT);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &hOSEventKM,
						   psEventObjectWaitIN->hOSEventKM,
						   PVRSRV_HANDLE_TYPE_EVENT_OBJECT_CONNECT);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = OSEventObjectWait(hOSEventKM);

	return 0;
}


static IMG_INT
PVRSRVEventObjectOpenBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_EVENT_OBJECT_OPEN *psEventObjectOpenIN,
						  PVRSRV_BRIDGE_OUT_EVENT_OBJECT_OPEN *psEventObjectOpenOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_EVENT_OBJECT_OPEN);

	NEW_HANDLE_BATCH_OR_ERROR(psEventObjectOpenOUT->eError, psPerProc, 1)

	psEventObjectOpenOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &psEventObjectOpenIN->sEventObject.hOSEventKM,
						   psEventObjectOpenIN->sEventObject.hOSEventKM,
						   PVRSRV_HANDLE_TYPE_SHARED_EVENT_OBJECT);

	if(psEventObjectOpenOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psEventObjectOpenOUT->eError = OSEventObjectOpen(&psEventObjectOpenIN->sEventObject, &psEventObjectOpenOUT->hOSEvent);

	if(psEventObjectOpenOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					  &psEventObjectOpenOUT->hOSEvent,
					  psEventObjectOpenOUT->hOSEvent,
					  PVRSRV_HANDLE_TYPE_EVENT_OBJECT_CONNECT,
					  PVRSRV_HANDLE_ALLOC_FLAG_MULTI);

	COMMIT_HANDLE_BATCH_OR_ERROR(psEventObjectOpenOUT->eError, psPerProc)

	return 0;
}


static IMG_INT
PVRSRVEventObjectCloseBW(IMG_UINT32 ui32BridgeID,
						  PVRSRV_BRIDGE_IN_EVENT_OBJECT_CLOSE *psEventObjectCloseIN,
						  PVRSRV_BRIDGE_RETURN *psRetOUT,
						  PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	IMG_HANDLE hOSEventKM;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_EVENT_OBJECT_CLOSE);

	psRetOUT->eError =
		PVRSRVLookupHandle(psPerProc->psHandleBase,
						   &psEventObjectCloseIN->sEventObject.hOSEventKM,
						   psEventObjectCloseIN->sEventObject.hOSEventKM,
						   PVRSRV_HANDLE_TYPE_SHARED_EVENT_OBJECT);
	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
						   &hOSEventKM,
						   psEventObjectCloseIN->hOSEventKM,
						   PVRSRV_HANDLE_TYPE_EVENT_OBJECT_CONNECT);

	if(psRetOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psRetOUT->eError = OSEventObjectClose(&psEventObjectCloseIN->sEventObject, hOSEventKM);

	return 0;
}


typedef struct _MODIFY_SYNC_OP_INFO
{
	IMG_HANDLE  hResItem;
	PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;
	IMG_UINT32 	ui32ModifyFlags;
	IMG_UINT32	ui32ReadOpsPendingSnapShot;
	IMG_UINT32	ui32WriteOpsPendingSnapShot;
} MODIFY_SYNC_OP_INFO;


static PVRSRV_ERROR DoQuerySyncOpsSatisfied(MODIFY_SYNC_OP_INFO *psModSyncOpInfo)
{
	PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;

	psKernelSyncInfo = psModSyncOpInfo->psKernelSyncInfo;

	if (!psKernelSyncInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if((psModSyncOpInfo->ui32WriteOpsPendingSnapShot == psKernelSyncInfo->psSyncData->ui32WriteOpsComplete)
	   && (psModSyncOpInfo->ui32ReadOpsPendingSnapShot == psKernelSyncInfo->psSyncData->ui32ReadOpsComplete))
	{
#if defined(PDUMP)
		
		
		
		PDumpComment("Poll for read ops complete to reach value (%u)", psModSyncOpInfo->ui32ReadOpsPendingSnapShot);
		PDumpMemPolKM(psKernelSyncInfo->psSyncDataMemInfoKM,
						  offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete),
						  psModSyncOpInfo->ui32ReadOpsPendingSnapShot,
						  0xFFFFFFFF,
						  PDUMP_POLL_OPERATOR_EQUAL,
						  0,
						  MAKEUNIQUETAG(psKernelSyncInfo->psSyncDataMemInfoKM));
						  
		
		PDumpComment("Poll for write ops complete to reach value (%u)", psModSyncOpInfo->ui32WriteOpsPendingSnapShot);
		PDumpMemPolKM(psKernelSyncInfo->psSyncDataMemInfoKM,
						  offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete),
						  psModSyncOpInfo->ui32WriteOpsPendingSnapShot,
						  0xFFFFFFFF,
						  PDUMP_POLL_OPERATOR_EQUAL,
						  0,
						  MAKEUNIQUETAG(psKernelSyncInfo->psSyncDataMemInfoKM));
#endif
		return PVRSRV_OK;
	}
	else
	{
		return PVRSRV_ERROR_RETRY;
	}
}

static PVRSRV_ERROR DoModifyCompleteSyncOps(MODIFY_SYNC_OP_INFO *psModSyncOpInfo)
{
	PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;

	psKernelSyncInfo = psModSyncOpInfo->psKernelSyncInfo;

	if (!psKernelSyncInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	if((psModSyncOpInfo->ui32WriteOpsPendingSnapShot != psKernelSyncInfo->psSyncData->ui32WriteOpsComplete)
	   || (psModSyncOpInfo->ui32ReadOpsPendingSnapShot != psKernelSyncInfo->psSyncData->ui32ReadOpsComplete))
	{
		return PVRSRV_ERROR_BAD_SYNC_STATE;
	}
	
	
	if(psModSyncOpInfo->ui32ModifyFlags & PVRSRV_MODIFYSYNCOPS_FLAGS_WO_INC)
	{
		psKernelSyncInfo->psSyncData->ui32WriteOpsComplete++;
	}
	
	
	if(psModSyncOpInfo->ui32ModifyFlags & PVRSRV_MODIFYSYNCOPS_FLAGS_RO_INC)
	{
		psKernelSyncInfo->psSyncData->ui32ReadOpsComplete++;
	}

	return PVRSRV_OK;
}


static PVRSRV_ERROR ModifyCompleteSyncOpsCallBack(IMG_PVOID		pvParam,
													IMG_UINT32	ui32Param)
{
	MODIFY_SYNC_OP_INFO		*psModSyncOpInfo;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	if (!pvParam)
	{
		PVR_DPF((PVR_DBG_ERROR, "ModifyCompleteSyncOpsCallBack: invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psModSyncOpInfo = (MODIFY_SYNC_OP_INFO*)pvParam;

	if (psModSyncOpInfo->psKernelSyncInfo)
	{
		
		LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
		{
			if (DoQuerySyncOpsSatisfied(psModSyncOpInfo) == PVRSRV_OK)
			{
				goto OpFlushedComplete;
			}
			PVR_DPF((PVR_DBG_WARNING, "ModifyCompleteSyncOpsCallBack: waiting for current Ops to flush"));
			OSSleepms(1);
		} END_LOOP_UNTIL_TIMEOUT();
		
		PVR_DPF((PVR_DBG_ERROR, "ModifyCompleteSyncOpsCallBack: timeout whilst waiting for current Ops to flush."));
		PVR_DPF((PVR_DBG_ERROR, "  Write ops pending snapshot = %d, write ops complete = %d",
				 psModSyncOpInfo->ui32WriteOpsPendingSnapShot,
				 psModSyncOpInfo->psKernelSyncInfo->psSyncData->ui32WriteOpsComplete));
		PVR_DPF((PVR_DBG_ERROR, "  Read ops pending snapshot = %d, write ops complete = %d",
				 psModSyncOpInfo->ui32ReadOpsPendingSnapShot,
				 psModSyncOpInfo->psKernelSyncInfo->psSyncData->ui32ReadOpsComplete));
		
		return PVRSRV_ERROR_TIMEOUT;
		
	OpFlushedComplete:
	
		DoModifyCompleteSyncOps(psModSyncOpInfo);
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, 	sizeof(MODIFY_SYNC_OP_INFO), (IMG_VOID *)psModSyncOpInfo, 0);
	

	
	PVRSRVScheduleDeviceCallbacks();

	return PVRSRV_OK;
}


static IMG_INT
PVRSRVCreateSyncInfoModObjBW(IMG_UINT32                                         ui32BridgeID,
									 IMG_VOID                                           *psBridgeIn,
									 PVRSRV_BRIDGE_OUT_CREATE_SYNC_INFO_MOD_OBJ  *psCreateSyncInfoModObjOUT,
									 PVRSRV_PER_PROCESS_DATA		   		            *psPerProc)
{
	MODIFY_SYNC_OP_INFO		*psModSyncOpInfo;

	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CREATE_SYNC_INFO_MOD_OBJ);

	NEW_HANDLE_BATCH_OR_ERROR(psCreateSyncInfoModObjOUT->eError, psPerProc, 1)

	ASSIGN_AND_EXIT_ON_ERROR(psCreateSyncInfoModObjOUT->eError,
			  OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(MODIFY_SYNC_OP_INFO),
			  (IMG_VOID **)&psModSyncOpInfo, 0,
			  "ModSyncOpInfo (MODIFY_SYNC_OP_INFO)"));

	psModSyncOpInfo->psKernelSyncInfo = IMG_NULL; 

	psCreateSyncInfoModObjOUT->eError = PVRSRVAllocHandle(psPerProc->psHandleBase,
																  &psCreateSyncInfoModObjOUT->hKernelSyncInfoModObj,
																  psModSyncOpInfo,
																  PVRSRV_HANDLE_TYPE_SYNC_INFO_MOD_OBJ,
																  PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE);

	if (psCreateSyncInfoModObjOUT->eError != PVRSRV_OK)
	{
		return 0;
	}

	psModSyncOpInfo->hResItem = ResManRegisterRes(psPerProc->hResManContext,
												  RESMAN_TYPE_MODIFY_SYNC_OPS,
												  psModSyncOpInfo,
												  0,
												  &ModifyCompleteSyncOpsCallBack);

	COMMIT_HANDLE_BATCH_OR_ERROR(psCreateSyncInfoModObjOUT->eError, psPerProc)

	return 0;
}


static IMG_INT
PVRSRVDestroySyncInfoModObjBW(IMG_UINT32                                          ui32BridgeID,
							  PVRSRV_BRIDGE_IN_DESTROY_SYNC_INFO_MOD_OBJ          *psDestroySyncInfoModObjIN,
							  PVRSRV_BRIDGE_RETURN                                *psDestroySyncInfoModObjOUT,
							  PVRSRV_PER_PROCESS_DATA		   		              *psPerProc)
{
	MODIFY_SYNC_OP_INFO		*psModSyncOpInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_DESTROY_SYNC_INFO_MOD_OBJ);

	psDestroySyncInfoModObjOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
																	(IMG_VOID**)&psModSyncOpInfo,
																	psDestroySyncInfoModObjIN->hKernelSyncInfoModObj,
																	PVRSRV_HANDLE_TYPE_SYNC_INFO_MOD_OBJ);
	if (psDestroySyncInfoModObjOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroySyncInfoModObjBW: PVRSRVLookupHandle failed"));
		return 0;
	}

	if(psModSyncOpInfo->psKernelSyncInfo != IMG_NULL)
	{
		
		psDestroySyncInfoModObjOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}

	psDestroySyncInfoModObjOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
																	 psDestroySyncInfoModObjIN->hKernelSyncInfoModObj,
																	 PVRSRV_HANDLE_TYPE_SYNC_INFO_MOD_OBJ);

	if (psDestroySyncInfoModObjOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroySyncInfoModObjBW: PVRSRVReleaseHandle failed"));
		return 0;
	}

	psDestroySyncInfoModObjOUT->eError = ResManFreeResByPtr(psModSyncOpInfo->hResItem);
	if (psDestroySyncInfoModObjOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroySyncInfoModObjBW: ResManFreeResByPtr failed"));
		return 0;
	}

	return 0;
}
 

static IMG_INT
PVRSRVModifyPendingSyncOpsBW(IMG_UINT32									ui32BridgeID,
						      PVRSRV_BRIDGE_IN_MODIFY_PENDING_SYNC_OPS	*psModifySyncOpsIN,
							  PVRSRV_BRIDGE_OUT_MODIFY_PENDING_SYNC_OPS	*psModifySyncOpsOUT,
							  PVRSRV_PER_PROCESS_DATA					*psPerProc)
{
	PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;
	MODIFY_SYNC_OP_INFO		*psModSyncOpInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MODIFY_PENDING_SYNC_OPS);

	psModifySyncOpsOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
													(IMG_VOID**)&psModSyncOpInfo,
													psModifySyncOpsIN->hKernelSyncInfoModObj,
													PVRSRV_HANDLE_TYPE_SYNC_INFO_MOD_OBJ);
	if (psModifySyncOpsOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyPendingSyncOpsBW: PVRSRVLookupHandle failed"));
		return 0;
	}

	psModifySyncOpsOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
													(IMG_VOID**)&psKernelSyncInfo,
													psModifySyncOpsIN->hKernelSyncInfo,
													PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if (psModifySyncOpsOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyPendingSyncOpsBW: PVRSRVLookupHandle failed"));
		return 0;
	}

	if(psModSyncOpInfo->psKernelSyncInfo)
	{
		
		psModifySyncOpsOUT->eError = PVRSRV_ERROR_RETRY;
		PVR_DPF((PVR_DBG_VERBOSE, "PVRSRVModifyPendingSyncOpsBW: SyncInfo Modification object is not empty"));
		return 0;
	}

	
	psModSyncOpInfo->psKernelSyncInfo = psKernelSyncInfo;
	psModSyncOpInfo->ui32ModifyFlags = psModifySyncOpsIN->ui32ModifyFlags;
	psModSyncOpInfo->ui32ReadOpsPendingSnapShot = psKernelSyncInfo->psSyncData->ui32ReadOpsPending;
	psModSyncOpInfo->ui32WriteOpsPendingSnapShot = psKernelSyncInfo->psSyncData->ui32WriteOpsPending;

	

	psModifySyncOpsOUT->ui32ReadOpsPending = psKernelSyncInfo->psSyncData->ui32ReadOpsPending;
	psModifySyncOpsOUT->ui32WriteOpsPending = psKernelSyncInfo->psSyncData->ui32WriteOpsPending;

	if(psModifySyncOpsIN->ui32ModifyFlags & PVRSRV_MODIFYSYNCOPS_FLAGS_WO_INC)
	{
		psKernelSyncInfo->psSyncData->ui32WriteOpsPending++;
	}

	if(psModifySyncOpsIN->ui32ModifyFlags & PVRSRV_MODIFYSYNCOPS_FLAGS_RO_INC)
	{
		psKernelSyncInfo->psSyncData->ui32ReadOpsPending++;
	}

	
	psModifySyncOpsOUT->eError = ResManDissociateRes(psModSyncOpInfo->hResItem,
													 psPerProc->hResManContext);

	if (psModifySyncOpsOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyPendingSyncOpsBW: PVRSRVLookupHandle failed"));
		return 0;
	}

	return 0;
}


static IMG_INT
PVRSRVModifyCompleteSyncOpsBW(IMG_UINT32							ui32BridgeID,
				      PVRSRV_BRIDGE_IN_MODIFY_COMPLETE_SYNC_OPS		*psModifySyncOpsIN,
					  PVRSRV_BRIDGE_RETURN							*psModifySyncOpsOUT,
					  PVRSRV_PER_PROCESS_DATA						*psPerProc)
{
	MODIFY_SYNC_OP_INFO		*psModSyncOpInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MODIFY_COMPLETE_SYNC_OPS);

	psModifySyncOpsOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
													(IMG_VOID**)&psModSyncOpInfo,
													psModifySyncOpsIN->hKernelSyncInfoModObj,
													PVRSRV_HANDLE_TYPE_SYNC_INFO_MOD_OBJ);
	if (psModifySyncOpsOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyCompleteSyncOpsBW: PVRSRVLookupHandle failed"));
		return 0;
	}

	if(psModSyncOpInfo->psKernelSyncInfo == IMG_NULL)
	{
		
		psModifySyncOpsOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}

	psModifySyncOpsOUT->eError = DoModifyCompleteSyncOps(psModSyncOpInfo);

	if (psModifySyncOpsOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyCompleteSyncOpsBW: DoModifyCompleteSyncOps failed"));
		return 0;
	}

	psModSyncOpInfo->psKernelSyncInfo = IMG_NULL;

	
	PVRSRVScheduleDeviceCallbacks();

	return 0;
}


static IMG_INT
PVRSRVSyncOpsFlushToModObjBW(IMG_UINT32                                         ui32BridgeID,
							 PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_MOD_OBJ		    *psSyncOpsFlushToModObjIN,
							 PVRSRV_BRIDGE_RETURN						        *psSyncOpsFlushToModObjOUT,
							 PVRSRV_PER_PROCESS_DATA		   		            *psPerProc)
{
	MODIFY_SYNC_OP_INFO		*psModSyncOpInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SYNC_OPS_FLUSH_TO_MOD_OBJ);

	psSyncOpsFlushToModObjOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
														   (IMG_VOID**)&psModSyncOpInfo,
														   psSyncOpsFlushToModObjIN->hKernelSyncInfoModObj,
														   PVRSRV_HANDLE_TYPE_SYNC_INFO_MOD_OBJ);
	if (psSyncOpsFlushToModObjOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToModObjBW: PVRSRVLookupHandle failed"));
		return 0;
	}

	if(psModSyncOpInfo->psKernelSyncInfo == IMG_NULL)
	{
		
		psSyncOpsFlushToModObjOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}

	psSyncOpsFlushToModObjOUT->eError = DoQuerySyncOpsSatisfied(psModSyncOpInfo);

	if (psSyncOpsFlushToModObjOUT->eError != PVRSRV_OK && psSyncOpsFlushToModObjOUT->eError != PVRSRV_ERROR_RETRY)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToModObjBW: DoQuerySyncOpsSatisfied failed"));
		return 0;
	}

	return 0;
}


static IMG_INT
PVRSRVSyncOpsFlushToDeltaBW(IMG_UINT32                                         ui32BridgeID,
							PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_DELTA		   *psSyncOpsFlushToDeltaIN,
							PVRSRV_BRIDGE_RETURN						       *psSyncOpsFlushToDeltaOUT,
							PVRSRV_PER_PROCESS_DATA		   		               *psPerProc)
{
	PVRSRV_KERNEL_SYNC_INFO		*psSyncInfo;
	IMG_UINT32 ui32DeltaRead;
	IMG_UINT32 ui32DeltaWrite;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SYNC_OPS_FLUSH_TO_DELTA);

	psSyncOpsFlushToDeltaOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
														  (IMG_VOID**)&psSyncInfo,
														  psSyncOpsFlushToDeltaIN->hKernelSyncInfo,
														  PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if (psSyncOpsFlushToDeltaOUT->eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToDeltaBW: PVRSRVLookupHandle failed"));
		return 0;
	}

	ui32DeltaRead = psSyncInfo->psSyncData->ui32ReadOpsPending - psSyncInfo->psSyncData->ui32ReadOpsComplete;
	ui32DeltaWrite = psSyncInfo->psSyncData->ui32WriteOpsPending - psSyncInfo->psSyncData->ui32WriteOpsComplete;

	if (ui32DeltaRead <= psSyncOpsFlushToDeltaIN->ui32Delta && ui32DeltaWrite <= psSyncOpsFlushToDeltaIN->ui32Delta)
	{
#if defined(PDUMP)
		IMG_UINT32 ui32MinimumReadOpsComplete;

		ui32MinimumReadOpsComplete = psSyncInfo->psSyncData->ui32ReadOpsPending;
		if (ui32MinimumReadOpsComplete < psSyncOpsFlushToDeltaIN->ui32Delta)
		{
			ui32MinimumReadOpsComplete = 0;
		}
		else
		{
			ui32MinimumReadOpsComplete = ui32MinimumReadOpsComplete - psSyncOpsFlushToDeltaIN->ui32Delta;
		}

		
		PDumpComment("Poll for read ops complete to delta (%u)",
					 psSyncOpsFlushToDeltaIN->ui32Delta);
		psSyncOpsFlushToDeltaOUT->eError =
			PDumpMemPolKM(psSyncInfo->psSyncDataMemInfoKM,
						  offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete),
						  psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
						  0xFFFFFFFF,
						  PDUMP_POLL_OPERATOR_GREATEREQUAL,
						  0,
						  MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));

		
		PDumpComment("Poll for write ops complete to delta (%u)",
					 psSyncOpsFlushToDeltaIN->ui32Delta);
		psSyncOpsFlushToDeltaOUT->eError =
			PDumpMemPolKM(psSyncInfo->psSyncDataMemInfoKM,
						  offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete),
						  psSyncInfo->psSyncData->ui32LastOpDumpVal,
						  0xFFFFFFFF,
						  PDUMP_POLL_OPERATOR_GREATEREQUAL,
						  0,
						  MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
#endif

		psSyncOpsFlushToDeltaOUT->eError = PVRSRV_OK;
	}
	else
	{
		psSyncOpsFlushToDeltaOUT->eError = PVRSRV_ERROR_RETRY;
	}

	return 0;
}


static PVRSRV_ERROR
FreeSyncInfoCallback(IMG_PVOID	pvParam,
					 IMG_UINT32	ui32Param)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	PVRSRV_ERROR eError;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)pvParam;

	eError = PVRSRVFreeSyncInfoKM(psSyncInfo);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}
	
	return PVRSRV_OK;
}


static IMG_INT
PVRSRVAllocSyncInfoBW(IMG_UINT32                                         ui32BridgeID,
					  PVRSRV_BRIDGE_IN_ALLOC_SYNC_INFO                  *psAllocSyncInfoIN,
					  PVRSRV_BRIDGE_OUT_ALLOC_SYNC_INFO                 *psAllocSyncInfoOUT,
					  PVRSRV_PER_PROCESS_DATA		   		            *psPerProc)
{
	PVRSRV_KERNEL_SYNC_INFO		*psSyncInfo;
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_HANDLE hDevMemContext;
		
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ALLOC_SYNC_INFO);

	NEW_HANDLE_BATCH_OR_ERROR(psAllocSyncInfoOUT->eError, psPerProc, 1)

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
								(IMG_HANDLE *)&psDeviceNode,
								psAllocSyncInfoIN->hDevCookie,
								PVRSRV_HANDLE_TYPE_DEV_NODE);
	if(eError != PVRSRV_OK)
	{
		goto allocsyncinfo_errorexit;
	}

	hDevMemContext = psDeviceNode->sDevMemoryInfo.pBMKernelContext;
	
	eError = PVRSRVAllocSyncInfoKM(psDeviceNode,
								   hDevMemContext,
								   &psSyncInfo);
			
	if (eError != PVRSRV_OK)
	{
		goto allocsyncinfo_errorexit;
	}

	eError = PVRSRVAllocHandle(psPerProc->psHandleBase,
							   &psAllocSyncInfoOUT->hKernelSyncInfo,
							   psSyncInfo,
							   PVRSRV_HANDLE_TYPE_SYNC_INFO,
							   PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE);

	if(eError != PVRSRV_OK)
	{
		goto allocsyncinfo_errorexit_freesyncinfo;
	}

	psSyncInfo->hResItem = ResManRegisterRes(psPerProc->hResManContext,
											   RESMAN_TYPE_SYNC_INFO,
											   psSyncInfo,
											   0,
											   FreeSyncInfoCallback);

	
	goto allocsyncinfo_commit;
	
	
 allocsyncinfo_errorexit_freesyncinfo:
	PVRSRVFreeSyncInfoKM(psSyncInfo);

 allocsyncinfo_errorexit:

	
 allocsyncinfo_commit:
	psAllocSyncInfoOUT->eError = eError;
	COMMIT_HANDLE_BATCH_OR_ERROR(eError, psPerProc);
		
	return 0;
}


static IMG_INT
PVRSRVFreeSyncInfoBW(IMG_UINT32                                          ui32BridgeID,
					 PVRSRV_BRIDGE_IN_FREE_SYNC_INFO                     *psFreeSyncInfoIN,
					 PVRSRV_BRIDGE_RETURN                                *psFreeSyncInfoOUT,
					 PVRSRV_PER_PROCESS_DATA		   		             *psPerProc)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	PVRSRV_ERROR eError;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_FREE_SYNC_INFO);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
								(IMG_VOID**)&psSyncInfo,
								psFreeSyncInfoIN->hKernelSyncInfo,
								PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSyncInfoBW: PVRSRVLookupHandle failed"));
		psFreeSyncInfoOUT->eError = eError;
		return 0;
	}

	eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
								 psFreeSyncInfoIN->hKernelSyncInfo,
								 PVRSRV_HANDLE_TYPE_SYNC_INFO);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSyncInfoBW: PVRSRVReleaseHandle failed"));
		psFreeSyncInfoOUT->eError = eError;
		return 0;
	}

	eError = ResManFreeResByPtr(psSyncInfo->hResItem);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSyncInfoBW: ResManFreeResByPtr failed"));
		psFreeSyncInfoOUT->eError = eError;
		return 0;
	}

	return 0;
}


PVRSRV_ERROR
CommonBridgeInit(IMG_VOID)
{
	IMG_UINT32 i;

	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_DEVICES, PVRSRVEnumerateDevicesBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ACQUIRE_DEVICEINFO, PVRSRVAcquireDeviceDataBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RELEASE_DEVICEINFO, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CREATE_DEVMEMCONTEXT, PVRSRVCreateDeviceMemContextBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DESTROY_DEVMEMCONTEXT, PVRSRVDestroyDeviceMemContextBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DEVMEM_HEAPINFO, PVRSRVGetDeviceMemHeapInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ALLOC_DEVICEMEM, PVRSRVAllocDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_FREE_DEVICEMEM, PVRSRVFreeDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GETFREE_DEVICEMEM, PVRSRVGetFreeDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CREATE_COMMANDQUEUE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DESTROY_COMMANDQUEUE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MHANDLE_TO_MMAP_DATA, PVRMMapOSMemHandleToMMapDataBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CONNECT_SERVICES, PVRSRVConnectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DISCONNECT_SERVICES, PVRSRVDisconnectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_WRAP_DEVICE_MEM, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DEVICEMEMINFO, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RESERVE_DEV_VIRTMEM	, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_FREE_DEV_VIRTMEM, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_EXT_MEMORY, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_EXT_MEMORY, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_DEV_MEMORY, PVRSRVMapDeviceMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_DEV_MEMORY, PVRSRVUnmapDeviceMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_DEVICECLASS_MEMORY, PVRSRVMapDeviceClassMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY, PVRSRVUnmapDeviceClassMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_MEM_INFO_TO_USER, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_MEM_INFO_FROM_USER, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_EXPORT_DEVICEMEM, PVRSRVExportDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RELEASE_MMAP_DATA, PVRMMapReleaseMMapDataBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_PROCESS_SIMISR_EVENT, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_REGISTER_SIM_PROCESS, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNREGISTER_SIM_PROCESS, DummyBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAPPHYSTOUSERSPACE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAPPHYSTOUSERSPACE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GETPHYSTOUSERSPACEMAP, DummyBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_FB_STATS, DummyBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_MISC_INFO, PVRSRVGetMiscInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RELEASE_MISC_INFO, DummyBW);

	
#if defined (SUPPORT_OVERLAY_ROTATE_BLIT)
	SetDispatchTableEntry(PVRSRV_BRIDGE_INIT_3D_OVL_BLT_RES, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DEINIT_3D_OVL_BLT_RES, DummyBW);
#endif


	
#if defined(PDUMP)
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_INIT, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_MEMPOL, PDumpMemPolBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPMEM, PDumpMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_REG, PDumpRegWithFlagsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_REGPOL, PDumpRegPolBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_COMMENT, PDumpCommentBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_SETFRAME, PDumpSetFrameBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_ISCAPTURING, PDumpIsCaptureFrameBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPBITMAP, PDumpBitmapBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPREADREG, PDumpReadRegBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_SYNCPOL, PDumpSyncPolBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPSYNC, PDumpSyncDumpBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DRIVERINFO, PDumpDriverInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPPDDEVPADDR, PDumpPDDevPAddrBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_CYCLE_COUNT_REG_READ, PDumpCycleCountRegReadBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_STARTINITPHASE, PDumpStartInitPhaseBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_STOPINITPHASE, PDumpStopInitPhaseBW);
#endif 

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_OEMJTABLE, DummyBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_CLASS, PVRSRVEnumerateDCBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_OPEN_DISPCLASS_DEVICE, PVRSRVOpenDCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CLOSE_DISPCLASS_DEVICE, PVRSRVCloseDCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_DISPCLASS_FORMATS, PVRSRVEnumDCFormatsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_DISPCLASS_DIMS, PVRSRVEnumDCDimsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DISPCLASS_SYSBUFFER, PVRSRVGetDCSystemBufferBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DISPCLASS_INFO, PVRSRVGetDCInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CREATE_DISPCLASS_SWAPCHAIN, PVRSRVCreateDCSwapChainBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DESTROY_DISPCLASS_SWAPCHAIN, PVRSRVDestroyDCSwapChainBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_DSTRECT, PVRSRVSetDCDstRectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_SRCRECT, PVRSRVSetDCSrcRectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_DSTCOLOURKEY, PVRSRVSetDCDstColourKeyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_SRCCOLOURKEY, PVRSRVSetDCSrcColourKeyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DISPCLASS_BUFFERS, PVRSRVGetDCBuffersBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER, PVRSRVSwapToDCBufferBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_SYSTEM, PVRSRVSwapToDCSystemBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_OPEN_BUFFERCLASS_DEVICE, PVRSRVOpenBCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CLOSE_BUFFERCLASS_DEVICE, PVRSRVCloseBCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_BUFFERCLASS_INFO, PVRSRVGetBCInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_BUFFERCLASS_BUFFER, PVRSRVGetBCBufferBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_WRAP_EXT_MEMORY, PVRSRVWrapExtMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY, PVRSRVUnwrapExtMemoryBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_ALLOC_SHARED_SYS_MEM, PVRSRVAllocSharedSysMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM, PVRSRVFreeSharedSysMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_MEMINFO_MEM, PVRSRVMapMemInfoMemBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_GETMMU_PD_DEVPADDR, MMU_GetPDDevPAddrBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_INITSRV_CONNECT,	&PVRSRVInitSrvConnectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_INITSRV_DISCONNECT, &PVRSRVInitSrvDisconnectBW);

	
	SetDispatchTableEntry(PVRSRV_BRIDGE_EVENT_OBJECT_WAIT,	&PVRSRVEventObjectWaitBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_EVENT_OBJECT_OPEN,	&PVRSRVEventObjectOpenBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_EVENT_OBJECT_CLOSE, &PVRSRVEventObjectCloseBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_CREATE_SYNC_INFO_MOD_OBJ, PVRSRVCreateSyncInfoModObjBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DESTROY_SYNC_INFO_MOD_OBJ, PVRSRVDestroySyncInfoModObjBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MODIFY_PENDING_SYNC_OPS, PVRSRVModifyPendingSyncOpsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MODIFY_COMPLETE_SYNC_OPS, PVRSRVModifyCompleteSyncOpsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SYNC_OPS_FLUSH_TO_MOD_OBJ, PVRSRVSyncOpsFlushToModObjBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SYNC_OPS_FLUSH_TO_DELTA, PVRSRVSyncOpsFlushToDeltaBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ALLOC_SYNC_INFO, PVRSRVAllocSyncInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_FREE_SYNC_INFO, PVRSRVFreeSyncInfoBW);

#if defined (SUPPORT_SGX)
	SetSGXDispatchTableEntry();
#endif
#if defined (SUPPORT_VGX)
	SetVGXDispatchTableEntry();
#endif
#if defined (SUPPORT_MSVDX)
	SetMSVDXDispatchTableEntry();
#endif


	
	
	for(i=0;i<BRIDGE_DISPATCH_TABLE_ENTRY_COUNT;i++)
	{
		if(!g_BridgeDispatchTable[i].pfFunction)
		{
			g_BridgeDispatchTable[i].pfFunction = &DummyBW;
#if defined(DEBUG_BRIDGE_KM)
			g_BridgeDispatchTable[i].pszIOCName = "_PVRSRV_BRIDGE_DUMMY";
			g_BridgeDispatchTable[i].pszFunctionName = "DummyBW";
			g_BridgeDispatchTable[i].ui32CallCount = 0;
			g_BridgeDispatchTable[i].ui32CopyFromUserTotalBytes = 0;
			g_BridgeDispatchTable[i].ui32CopyToUserTotalBytes = 0;
#endif
		}
	}

	return PVRSRV_OK;
}

IMG_INT BridgedDispatchKM(PVRSRV_PER_PROCESS_DATA * psPerProc,
					  PVRSRV_BRIDGE_PACKAGE   * psBridgePackageKM)
{

	IMG_VOID   * psBridgeIn;
	IMG_VOID   * psBridgeOut;
	BridgeWrapperFunction pfBridgeHandler;
	IMG_UINT32   ui32BridgeID = psBridgePackageKM->ui32BridgeID;
	IMG_INT      err          = -EFAULT;

#if defined(DEBUG_TRACE_BRIDGE_KM)
	PVR_DPF((PVR_DBG_ERROR, "%s: %s",
			 __FUNCTION__,
			 g_BridgeDispatchTable[ui32BridgeID].pszIOCName));
#endif

#if defined(DEBUG_BRIDGE_KM)
	g_BridgeDispatchTable[ui32BridgeID].ui32CallCount++;
	g_BridgeGlobalStats.ui32IOCTLCount++;
#endif

	if(!psPerProc->bInitProcess)
	{
		if(PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RAN))
		{
			if(!PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL))
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Initialisation failed.  Driver unusable.",
						 __FUNCTION__));
				goto return_fault;
			}
		}
		else
		{
			if(PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RUNNING))
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Initialisation is in progress",
						 __FUNCTION__));
				goto return_fault;
			}
			else
			{
				
				switch(ui32BridgeID)
				{
					case PVRSRV_GET_BRIDGE_ID(PVRSRV_BRIDGE_CONNECT_SERVICES):
					case PVRSRV_GET_BRIDGE_ID(PVRSRV_BRIDGE_DISCONNECT_SERVICES):
					case PVRSRV_GET_BRIDGE_ID(PVRSRV_BRIDGE_INITSRV_CONNECT):
					case PVRSRV_GET_BRIDGE_ID(PVRSRV_BRIDGE_INITSRV_DISCONNECT):
						break;
					default:
						PVR_DPF((PVR_DBG_ERROR, "%s: Driver initialisation not completed yet.",
								 __FUNCTION__));
						goto return_fault;
				}
			}
		}
	}



#if defined(__linux__)
	{
		
		SYS_DATA *psSysData;

		SysAcquireData(&psSysData);

		
		psBridgeIn = ((ENV_DATA *)psSysData->pvEnvSpecificData)->pvBridgeData;
		psBridgeOut = (IMG_PVOID)((IMG_PBYTE)psBridgeIn + PVRSRV_MAX_BRIDGE_IN_SIZE);

		
#if defined(DEBUG)
		PVR_ASSERT(psBridgePackageKM->ui32InBufferSize < PVRSRV_MAX_BRIDGE_IN_SIZE);
		PVR_ASSERT(psBridgePackageKM->ui32OutBufferSize < PVRSRV_MAX_BRIDGE_OUT_SIZE);
#endif

		if(psBridgePackageKM->ui32InBufferSize > 0)
		{
			if(!OSAccessOK(PVR_VERIFY_READ,
							psBridgePackageKM->pvParamIn,
							psBridgePackageKM->ui32InBufferSize))
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Invalid pvParamIn pointer", __FUNCTION__));
			}

			if(CopyFromUserWrapper(psPerProc,
					               ui32BridgeID,
								   psBridgeIn,
								   psBridgePackageKM->pvParamIn,
								   psBridgePackageKM->ui32InBufferSize)
			  != PVRSRV_OK)
			{
				goto return_fault;
			}
		}
	}
#else
	psBridgeIn  = psBridgePackageKM->pvParamIn;
	psBridgeOut = psBridgePackageKM->pvParamOut;
#endif

	if(ui32BridgeID >= (BRIDGE_DISPATCH_TABLE_ENTRY_COUNT))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: ui32BridgeID = %d is out if range!",
				 __FUNCTION__, ui32BridgeID));
		goto return_fault;
	}
	pfBridgeHandler =
		(BridgeWrapperFunction)g_BridgeDispatchTable[ui32BridgeID].pfFunction;
	err = pfBridgeHandler(ui32BridgeID,
						  psBridgeIn,
						  psBridgeOut,
						  psPerProc);
	if(err < 0)
	{
		goto return_fault;
	}


#if defined(__linux__)
	
	if(CopyToUserWrapper(psPerProc,
						 ui32BridgeID,
						 psBridgePackageKM->pvParamOut,
						 psBridgeOut,
						 psBridgePackageKM->ui32OutBufferSize)
	   != PVRSRV_OK)
	{
		goto return_fault;
	}
#endif

	err = 0;
return_fault:
	ReleaseHandleBatch(psPerProc);
	return err;
}

