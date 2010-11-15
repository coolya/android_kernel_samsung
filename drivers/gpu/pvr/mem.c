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

#include "services_headers.h"
#include "pvr_bridge_km.h"


static PVRSRV_ERROR
FreeSharedSysMemCallBack(IMG_PVOID	pvParam,
						 IMG_UINT32	ui32Param)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	OSFreePages(psKernelMemInfo->ui32Flags,
				psKernelMemInfo->ui32AllocSize,
				psKernelMemInfo->pvLinAddrKM,
				psKernelMemInfo->sMemBlk.hOSMemHandle);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(PVRSRV_KERNEL_MEM_INFO),
			  psKernelMemInfo,
			  IMG_NULL);
	

	return PVRSRV_OK;
}


IMG_EXPORT PVRSRV_ERROR
PVRSRVAllocSharedSysMemoryKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
							 IMG_UINT32					ui32Flags,
							 IMG_SIZE_T 				ui32Size,
							 PVRSRV_KERNEL_MEM_INFO 	**ppsKernelMemInfo)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	if(OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(PVRSRV_KERNEL_MEM_INFO),
				  (IMG_VOID **)&psKernelMemInfo, IMG_NULL,
				  "Kernel Memory Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVAllocSharedSysMemoryKM: Failed to alloc memory for meminfo"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	OSMemSet(psKernelMemInfo, 0, sizeof(*psKernelMemInfo));

	ui32Flags &= ~PVRSRV_HAP_MAPTYPE_MASK;
	ui32Flags |= PVRSRV_HAP_MULTI_PROCESS;
	psKernelMemInfo->ui32Flags = ui32Flags;
	psKernelMemInfo->ui32AllocSize = ui32Size;

	if(OSAllocPages(psKernelMemInfo->ui32Flags,
					psKernelMemInfo->ui32AllocSize,
					HOST_PAGESIZE(),
					&psKernelMemInfo->pvLinAddrKM,
					&psKernelMemInfo->sMemBlk.hOSMemHandle)
		!= PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSharedSysMemoryKM: Failed to alloc memory for block"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(PVRSRV_KERNEL_MEM_INFO),
				  psKernelMemInfo,
				  0);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	
	psKernelMemInfo->sMemBlk.hResItem =
				ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_SHARED_MEM_INFO,
								  psKernelMemInfo,
								  0,
								  &FreeSharedSysMemCallBack);

	*ppsKernelMemInfo = psKernelMemInfo;

	return PVRSRV_OK; 
}


IMG_EXPORT PVRSRV_ERROR
PVRSRVFreeSharedSysMemoryKM(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	PVRSRV_ERROR eError;

	if(psKernelMemInfo->sMemBlk.hResItem)
	{
		eError = ResManFreeResByPtr(psKernelMemInfo->sMemBlk.hResItem);
	}
	else
	{
		eError = FreeSharedSysMemCallBack(psKernelMemInfo, 0);
	}

	return eError;
}


IMG_EXPORT PVRSRV_ERROR
PVRSRVDissociateMemFromResmanKM(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if(!psKernelMemInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(psKernelMemInfo->sMemBlk.hResItem)
	{
		eError = ResManDissociateRes(psKernelMemInfo->sMemBlk.hResItem, IMG_NULL);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"PVRSRVDissociateMemFromResmanKM: ResManDissociateRes failed"));
			PVR_DBG_BREAK;
			return eError;
		}

		psKernelMemInfo->sMemBlk.hResItem = IMG_NULL;
	}

	return eError;
}

