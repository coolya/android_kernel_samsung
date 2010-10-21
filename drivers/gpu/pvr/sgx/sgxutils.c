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

#include "sgxdefs.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "sgx_bridge_km.h"
#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgx_mkif_km.h"
#include "sysconfig.h"
#include "pdump_km.h"
#include "mmu.h"
#include "pvr_bridge_km.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "sgxutils.h"

#ifdef __linux__
#include <linux/kernel.h>	
#include <linux/string.h>	
#else
#include <stdio.h>
#endif


#if defined(SYS_CUSTOM_POWERDOWN)
PVRSRV_ERROR SysPowerDownMISR(PVRSRV_DEVICE_NODE	* psDeviceNode, IMG_UINT32 ui32CallerID);
#endif



static IMG_VOID SGXPostActivePowerEvent(PVRSRV_DEVICE_NODE	* psDeviceNode,
										IMG_UINT32           ui32CallerID)
{
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = psDevInfo->psSGXHostCtl;

	
	psSGXHostCtl->ui32NumActivePowerEvents++;

	if ((psSGXHostCtl->ui32PowerStatus & PVRSRV_USSE_EDM_POWMAN_POWEROFF_RESTART_IMMEDIATE) != 0)
	{
		


		if (ui32CallerID == ISR_ID)
		{
			psDeviceNode->bReProcessDeviceCommandComplete = IMG_TRUE;
		}
		else
		{
			SGXScheduleProcessQueuesKM(psDeviceNode);
		}
	}
}


IMG_VOID SGXTestActivePowerEvent (PVRSRV_DEVICE_NODE	*psDeviceNode,
								  IMG_UINT32			ui32CallerID)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = psDevInfo->psSGXHostCtl;

	if (((psSGXHostCtl->ui32InterruptFlags & PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER) != 0) &&
		((psSGXHostCtl->ui32InterruptClearFlags & PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER) == 0))
	{
		
		psSGXHostCtl->ui32InterruptClearFlags |= PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER;

		
		PDUMPSUSPEND();

#if defined(SYS_CUSTOM_POWERDOWN)
		


		eError = SysPowerDownMISR(psDeviceNode, ui32CallerID);
#else
		eError = PVRSRVSetDevicePowerStateKM(psDeviceNode->sDevId.ui32DeviceIndex,
											 PVRSRV_DEV_POWER_STATE_OFF,
											 ui32CallerID, IMG_FALSE);
		if (eError == PVRSRV_OK)
		{
			SGXPostActivePowerEvent(psDeviceNode, ui32CallerID);
		}
#endif
		if (eError == PVRSRV_ERROR_RETRY)
		{
			

			psSGXHostCtl->ui32InterruptClearFlags &= ~PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER;
			eError = PVRSRV_OK;
		}

		
		PDUMPRESUME();
	}

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXTestActivePowerEvent error:%u", eError));
	}
}


#ifdef INLINE_IS_PRAGMA
#pragma inline(SGXAcquireKernelCCBSlot)
#endif
static INLINE SGXMKIF_COMMAND * SGXAcquireKernelCCBSlot(PVRSRV_SGX_CCB_INFO *psCCB)
{
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		if(((*psCCB->pui32WriteOffset + 1) & 255) != *psCCB->pui32ReadOffset)
		{
			return &psCCB->psCommands[*psCCB->pui32WriteOffset];
		}

		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT();

	
	return IMG_NULL;
}

PVRSRV_ERROR SGXScheduleCCBCommand(PVRSRV_SGXDEV_INFO 	*psDevInfo,
								   SGXMKIF_CMD_TYPE		eCmdType,
								   SGXMKIF_COMMAND		*psCommandData,
								   IMG_UINT32			ui32CallerID,
								   IMG_UINT32			ui32PDumpFlags,
								   IMG_BOOL				bLastInScene)
{
	PVRSRV_SGX_CCB_INFO *psKernelCCB;
	PVRSRV_ERROR eError = PVRSRV_OK;
	SGXMKIF_COMMAND *psSGXCommand;
#if defined(PDUMP)
	IMG_VOID *pvDumpCommand;
	IMG_BOOL bPDumpIsSuspended = PDumpIsSuspended();
	IMG_BOOL bPersistentProcess = IMG_FALSE;
#else
	PVR_UNREFERENCED_PARAMETER(ui32CallerID);
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
#endif

#if defined(FIX_HW_BRN_28889)
	



	if ( (eCmdType != SGXMKIF_CMD_PROCESS_QUEUES) && 
		 ((psDevInfo->ui32CacheControl & SGXMKIF_CC_INVAL_DATA) != 0) &&
		 ((psDevInfo->ui32CacheControl & (SGXMKIF_CC_INVAL_BIF_PT | SGXMKIF_CC_INVAL_BIF_PD)) != 0))
	{
	#if defined(PDUMP)
		PVRSRV_KERNEL_MEM_INFO	*psSGXHostCtlMemInfo = psDevInfo->psKernelSGXHostCtlMemInfo;
	#endif
		SGXMKIF_HOST_CTL	*psSGXHostCtl = psDevInfo->psSGXHostCtl;
		SGXMKIF_COMMAND		sCacheCommand = {0};

		eError = SGXScheduleCCBCommand(psDevInfo,
									   SGXMKIF_CMD_PROCESS_QUEUES,
									   &sCacheCommand,
									   ui32CallerID,
									   ui32PDumpFlags,
									   bLastInScene);
		if (eError != PVRSRV_OK)
		{
			goto Exit;
		}
		
		
		#if !defined(NO_HARDWARE)
		if(PollForValueKM(&psSGXHostCtl->ui32InvalStatus,
						  PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
						  PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
						  2 * MAX_HW_TIME_US/WAIT_TRY_COUNT,
						  WAIT_TRY_COUNT) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXScheduleCCBCommand: Wait for uKernel to Invalidate BIF cache failed"));
			PVR_DBG_BREAK;
		}
		#endif
	
		#if defined(PDUMP)
		
		PDUMPCOMMENTWITHFLAGS(0, "Host Control - Poll for BIF cache invalidate request to complete");
		PDUMPMEMPOL(psSGXHostCtlMemInfo,
					offsetof(SGXMKIF_HOST_CTL, ui32InvalStatus),
					PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
					PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
					PDUMP_POLL_OPERATOR_EQUAL,
					0,
					MAKEUNIQUETAG(psSGXHostCtlMemInfo));
		#endif 
	
		psSGXHostCtl->ui32InvalStatus &= ~(PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE);
		PDUMPMEM(IMG_NULL, psSGXHostCtlMemInfo, offsetof(SGXMKIF_HOST_CTL, ui32CleanupStatus), sizeof(IMG_UINT32), 0, MAKEUNIQUETAG(psSGXHostCtlMemInfo));
	}
#endif
	
#if defined(PDUMP)
	
	{
		PVRSRV_PER_PROCESS_DATA* psPerProc = PVRSRVFindPerProcessData();
		if(psPerProc != IMG_NULL)
		{
			bPersistentProcess = psPerProc->bPDumpPersistent;
		}
	}
#endif 
	psKernelCCB = psDevInfo->psKernelCCBInfo;

	psSGXCommand = SGXAcquireKernelCCBSlot(psKernelCCB);

	
	if(!psSGXCommand)
	{
		eError = PVRSRV_ERROR_TIMEOUT;
		goto Exit;
	}

	
	psCommandData->ui32CacheControl = psDevInfo->ui32CacheControl;

#if defined(PDUMP)
	
	psDevInfo->sPDContext.ui32CacheControl |= psDevInfo->ui32CacheControl;
#endif

	
	psDevInfo->ui32CacheControl = 0;

	
	*psSGXCommand = *psCommandData;

	if (eCmdType >= SGXMKIF_CMD_MAX)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXScheduleCCBCommandKM: Unknown command type: %d", eCmdType)) ;
		eError = PVRSRV_ERROR_INVALID_CCB_COMMAND;
		goto Exit;
	}

	
	if((eCmdType == SGXMKIF_CMD_TA) && bLastInScene)
	{
		SYS_DATA *psSysData;

		SysAcquireData(&psSysData);

		if(psSysData->ePendingCacheOpType == PVRSRV_MISC_INFO_CPUCACHEOP_FLUSH)
		{
			OSFlushCPUCacheKM();
		}
		else if(psSysData->ePendingCacheOpType == PVRSRV_MISC_INFO_CPUCACHEOP_CLEAN)
		{
			OSCleanCPUCacheKM();
		}
	
		psSysData->ePendingCacheOpType = PVRSRV_MISC_INFO_CPUCACHEOP_NONE;
	}

	PVR_ASSERT(eCmdType < SGXMKIF_CMD_MAX);
	psSGXCommand->ui32ServiceAddress = psDevInfo->aui32HostKickAddr[eCmdType];	 

#if defined(PDUMP)
	if ((ui32CallerID != ISR_ID) && (bPDumpIsSuspended == IMG_FALSE) &&
		(bPersistentProcess == IMG_FALSE) )
	{
		
		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Poll for space in the Kernel CCB\r\n");
		PDUMPMEMPOL(psKernelCCB->psCCBCtlMemInfo,
					offsetof(PVRSRV_SGX_CCB_CTL, ui32ReadOffset),
					(psKernelCCB->ui32CCBDumpWOff + 1) & 0xff,
					0xff,
					PDUMP_POLL_OPERATOR_NOTEQUAL,
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));

		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kernel CCB command (type == %d)\r\n", eCmdType);
		pvDumpCommand = (IMG_VOID *)((IMG_UINT8 *)psKernelCCB->psCCBMemInfo->pvLinAddrKM + (*psKernelCCB->pui32WriteOffset * sizeof(SGXMKIF_COMMAND)));

		PDUMPMEM(pvDumpCommand,
					psKernelCCB->psCCBMemInfo,
					psKernelCCB->ui32CCBDumpWOff * sizeof(SGXMKIF_COMMAND),
					sizeof(SGXMKIF_COMMAND),
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBMemInfo));

		
		PDUMPMEM(&psDevInfo->sPDContext.ui32CacheControl,
					psKernelCCB->psCCBMemInfo,
					psKernelCCB->ui32CCBDumpWOff * sizeof(SGXMKIF_COMMAND) +
					offsetof(SGXMKIF_COMMAND, ui32CacheControl),
					sizeof(IMG_UINT32),
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBMemInfo));

		if (PDumpIsCaptureFrameKM()
		|| ((ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
		{
			
			psDevInfo->sPDContext.ui32CacheControl = 0;
		}
	}
#endif

#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
	
	eError = PollForValueKM (psKernelCCB->pui32ReadOffset,
								*psKernelCCB->pui32WriteOffset,
								0xFF,
								MAX_HW_TIME_US/WAIT_TRY_COUNT,
								WAIT_TRY_COUNT);
	if (eError != PVRSRV_OK)
	{
		eError = PVRSRV_ERROR_TIMEOUT;
		goto Exit;
	}
#endif

	

	*psKernelCCB->pui32WriteOffset = (*psKernelCCB->pui32WriteOffset + 1) & 255;

#if defined(PDUMP)
	if ((ui32CallerID != ISR_ID) && (bPDumpIsSuspended == IMG_FALSE) &&
		(bPersistentProcess == IMG_FALSE) )
	{
	#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Poll for previous Kernel CCB CMD to be read\r\n");
		PDUMPMEMPOL(psKernelCCB->psCCBCtlMemInfo,
					offsetof(PVRSRV_SGX_CCB_CTL, ui32ReadOffset),
					(psKernelCCB->ui32CCBDumpWOff),
					0xFF,
					PDUMP_POLL_OPERATOR_EQUAL,
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));
	#endif

		if (PDumpIsCaptureFrameKM()
		|| ((ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
		{
			psKernelCCB->ui32CCBDumpWOff = (psKernelCCB->ui32CCBDumpWOff + 1) & 0xFF;
			psDevInfo->ui32KernelCCBEventKickerDumpVal = (psDevInfo->ui32KernelCCBEventKickerDumpVal + 1) & 0xFF;
		}

		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kernel CCB write offset\r\n");
		PDUMPMEM(&psKernelCCB->ui32CCBDumpWOff,
				 psKernelCCB->psCCBCtlMemInfo,
				 offsetof(PVRSRV_SGX_CCB_CTL, ui32WriteOffset),
				 sizeof(IMG_UINT32),
				 ui32PDumpFlags,
				 MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));
		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kernel CCB event kicker\r\n");
		PDUMPMEM(&psDevInfo->ui32KernelCCBEventKickerDumpVal,
				 psDevInfo->psKernelCCBEventKickerMemInfo,
				 0,
				 sizeof(IMG_UINT32),
				 ui32PDumpFlags,
				 MAKEUNIQUETAG(psDevInfo->psKernelCCBEventKickerMemInfo));
		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kick the SGX microkernel\r\n");
	#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK2, 0), EUR_CR_EVENT_KICK2_NOW_MASK, ui32PDumpFlags);
	#else
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0), EUR_CR_EVENT_KICK_NOW_MASK, ui32PDumpFlags);
	#endif
	}
#endif

	*psDevInfo->pui32KernelCCBEventKicker = (*psDevInfo->pui32KernelCCBEventKicker + 1) & 0xFF;

	OSWriteMemoryBarrier();

#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
	OSWriteHWReg(psDevInfo->pvRegsBaseKM,
				SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK2, 0),
				EUR_CR_EVENT_KICK2_NOW_MASK);
#else
	OSWriteHWReg(psDevInfo->pvRegsBaseKM,
				SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0),
				EUR_CR_EVENT_KICK_NOW_MASK);
#endif

	OSMemoryBarrier();

#if defined(NO_HARDWARE)
	
	*psKernelCCB->pui32ReadOffset = (*psKernelCCB->pui32ReadOffset + 1) & 255;
#endif

Exit:
	return eError;
}


PVRSRV_ERROR SGXScheduleCCBCommandKM(PVRSRV_DEVICE_NODE		*psDeviceNode,
									 SGXMKIF_CMD_TYPE		eCmdType,
									 SGXMKIF_COMMAND		*psCommandData,
									 IMG_UINT32				ui32CallerID,
									 IMG_UINT32				ui32PDumpFlags,
									 IMG_BOOL				bLastInScene)
{
	PVRSRV_ERROR		eError;
	PVRSRV_SGXDEV_INFO 	*psDevInfo = psDeviceNode->pvDevice;

	
	PDUMPSUSPEND();

	
	eError = PVRSRVSetDevicePowerStateKM(psDeviceNode->sDevId.ui32DeviceIndex,
										 PVRSRV_DEV_POWER_STATE_ON,
										 ui32CallerID,
										 IMG_TRUE);

	PDUMPRESUME();

	if (eError == PVRSRV_OK)
	{
		psDeviceNode->bReProcessDeviceCommandComplete = IMG_FALSE;
	}
	else
	{
		if (eError == PVRSRV_ERROR_RETRY)
		{
			if (ui32CallerID == ISR_ID)
			{
				


				psDeviceNode->bReProcessDeviceCommandComplete = IMG_TRUE;
				eError = PVRSRV_OK;
			}
			else
			{
				

			}
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXScheduleCCBCommandKM failed to acquire lock - "
					 "ui32CallerID:%d eError:%u", ui32CallerID, eError));
		}

		return eError;
	}

	eError = SGXScheduleCCBCommand(psDevInfo, eCmdType, psCommandData, ui32CallerID, ui32PDumpFlags, bLastInScene);

	PVRSRVPowerUnlock(ui32CallerID);

	
	if (ui32CallerID != ISR_ID)
	{
		


		SGXTestActivePowerEvent(psDeviceNode, ui32CallerID);
	}

	return eError;
}


PVRSRV_ERROR SGXScheduleProcessQueuesKM(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR 		eError;
	PVRSRV_SGXDEV_INFO 	*psDevInfo = psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psHostCtl = psDevInfo->psKernelSGXHostCtlMemInfo->pvLinAddrKM;
	IMG_UINT32		ui32PowerStatus;
	SGXMKIF_COMMAND		sCommand = {0};

	ui32PowerStatus = psHostCtl->ui32PowerStatus;
	if ((ui32PowerStatus & PVRSRV_USSE_EDM_POWMAN_NO_WORK) != 0)
	{
		
		return PVRSRV_OK;
	}

	eError = SGXScheduleCCBCommandKM(psDeviceNode, SGXMKIF_CMD_PROCESS_QUEUES, &sCommand, ISR_ID, 0, IMG_FALSE);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXScheduleProcessQueuesKM failed to schedule CCB command: %u", eError));
		return eError;
	}

	return PVRSRV_OK;
}


IMG_BOOL SGXIsDevicePowered(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	return PVRSRVIsDevicePowered(psDeviceNode->sDevId.ui32DeviceIndex);
}

IMG_EXPORT
PVRSRV_ERROR SGXGetInternalDevInfoKM(IMG_HANDLE hDevCookie,
									SGX_INTERNAL_DEVINFO *psSGXInternalDevInfo)
{
	PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)((PVRSRV_DEVICE_NODE *)hDevCookie)->pvDevice;

	psSGXInternalDevInfo->ui32Flags = psDevInfo->ui32Flags;
	psSGXInternalDevInfo->bForcePTOff = (IMG_BOOL)psDevInfo->bForcePTOff;

	
	psSGXInternalDevInfo->hHostCtlKernelMemInfoHandle =
		(IMG_HANDLE)psDevInfo->psKernelSGXHostCtlMemInfo;

	return PVRSRV_OK;
}


IMG_VOID SGXCleanupRequest(PVRSRV_DEVICE_NODE	*psDeviceNode,
						   IMG_DEV_VIRTADDR		*psHWDataDevVAddr,
						   IMG_UINT32			ui32CleanupType)
{
	PVRSRV_ERROR			eError;
	PVRSRV_SGXDEV_INFO		*psSGXDevInfo = psDeviceNode->pvDevice;
	PVRSRV_KERNEL_MEM_INFO	*psSGXHostCtlMemInfo = psSGXDevInfo->psKernelSGXHostCtlMemInfo;
	SGXMKIF_HOST_CTL		*psSGXHostCtl = psSGXHostCtlMemInfo->pvLinAddrKM;

	if ((psSGXHostCtl->ui32PowerStatus & PVRSRV_USSE_EDM_POWMAN_NO_WORK) != 0)
	{
		
	}
	else
	{
		SGXMKIF_COMMAND		sCommand = {0};

		PDUMPCOMMENTWITHFLAGS(0, "Request ukernel resouce clean-up");
		sCommand.ui32Data[0] = ui32CleanupType;
		sCommand.ui32Data[1] = (psHWDataDevVAddr == IMG_NULL) ? 0 : psHWDataDevVAddr->uiAddr;

		eError = SGXScheduleCCBCommandKM(psDeviceNode, SGXMKIF_CMD_CLEANUP, &sCommand, KERNEL_ID, 0, IMG_FALSE);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXCleanupRequest: Failed to submit clean-up command"));
			PVR_DBG_BREAK;
		}

		
		#if !defined(NO_HARDWARE)
		if(PollForValueKM(&psSGXHostCtl->ui32CleanupStatus,
						  PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
						  PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
						  2 * MAX_HW_TIME_US/WAIT_TRY_COUNT,
						  WAIT_TRY_COUNT) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXCleanupRequest: Wait for uKernel to clean up (%u) failed", ui32CleanupType));
			PVR_DBG_BREAK;
		}
		#endif

		#if defined(PDUMP)
		
		PDUMPCOMMENTWITHFLAGS(0, "Host Control - Poll for clean-up request to complete");
		PDUMPMEMPOL(psSGXHostCtlMemInfo,
					offsetof(SGXMKIF_HOST_CTL, ui32CleanupStatus),
					PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
					PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
					PDUMP_POLL_OPERATOR_EQUAL,
					0,
					MAKEUNIQUETAG(psSGXHostCtlMemInfo));
		#endif 

		psSGXHostCtl->ui32CleanupStatus &= ~(PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE);
		PDUMPMEM(IMG_NULL, psSGXHostCtlMemInfo, offsetof(SGXMKIF_HOST_CTL, ui32CleanupStatus), sizeof(IMG_UINT32), 0, MAKEUNIQUETAG(psSGXHostCtlMemInfo));
		
		
	#if defined(SGX_FEATURE_SYSTEM_CACHE)
		psSGXDevInfo->ui32CacheControl |= (SGXMKIF_CC_INVAL_BIF_SL | SGXMKIF_CC_INVAL_DATA);
	#else
		psSGXDevInfo->ui32CacheControl |= SGXMKIF_CC_INVAL_DATA;
	#endif
	}
}


typedef struct _SGX_HW_RENDER_CONTEXT_CLEANUP_
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_DEV_VIRTADDR sHWRenderContextDevVAddr;
	IMG_HANDLE hBlockAlloc;
	PRESMAN_ITEM psResItem;
} SGX_HW_RENDER_CONTEXT_CLEANUP;


static PVRSRV_ERROR SGXCleanupHWRenderContextCallback(IMG_PVOID		pvParam,
													  IMG_UINT32	ui32Param)
{
	SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup = pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
					  &psCleanup->sHWRenderContextDevVAddr,
					  PVRSRV_CLEANUPCMD_RC);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(SGX_HW_RENDER_CONTEXT_CLEANUP),
			  psCleanup,
			  psCleanup->hBlockAlloc);
	

	return PVRSRV_OK;
}

typedef struct _SGX_HW_TRANSFER_CONTEXT_CLEANUP_
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_DEV_VIRTADDR sHWTransferContextDevVAddr;
	IMG_HANDLE hBlockAlloc;
	PRESMAN_ITEM psResItem;
} SGX_HW_TRANSFER_CONTEXT_CLEANUP;


static PVRSRV_ERROR SGXCleanupHWTransferContextCallback(IMG_PVOID	pvParam,
														IMG_UINT32	ui32Param)
{
	SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup = (SGX_HW_TRANSFER_CONTEXT_CLEANUP *)pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
					  &psCleanup->sHWTransferContextDevVAddr,
					  PVRSRV_CLEANUPCMD_TC);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(SGX_HW_TRANSFER_CONTEXT_CLEANUP),
			  psCleanup,
			  psCleanup->hBlockAlloc);
	

	return PVRSRV_OK;
}

IMG_EXPORT
IMG_HANDLE SGXRegisterHWRenderContextKM(IMG_HANDLE				psDeviceNode,
										IMG_DEV_VIRTADDR		*psHWRenderContextDevVAddr,
										PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup;
	PRESMAN_ITEM psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(SGX_HW_RENDER_CONTEXT_CLEANUP),
						(IMG_VOID **)&psCleanup,
						&hBlockAlloc,
						"SGX Hardware Render Context Cleanup");

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContextKM: Couldn't allocate memory for SGX_HW_RENDER_CONTEXT_CLEANUP structure"));
		return IMG_NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHWRenderContextDevVAddr = *psHWRenderContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_HW_RENDER_CONTEXT,
								  (IMG_VOID *)psCleanup,
								  0,
								  &SGXCleanupHWRenderContextCallback);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContextKM: ResManRegisterRes failed"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(SGX_HW_RENDER_CONTEXT_CLEANUP),
				  psCleanup,
				  psCleanup->hBlockAlloc);
		

		return IMG_NULL;
	}

	psCleanup->psResItem = psResItem;

	return (IMG_HANDLE)psCleanup;
}

IMG_EXPORT
PVRSRV_ERROR SGXUnregisterHWRenderContextKM(IMG_HANDLE hHWRenderContext)
{
	PVRSRV_ERROR eError;
	SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHWRenderContext != IMG_NULL);

	psCleanup = (SGX_HW_RENDER_CONTEXT_CLEANUP *)hHWRenderContext;

	if (psCleanup == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWRenderContextKM: invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = ResManFreeResByPtr(psCleanup->psResItem);

	return eError;
}


IMG_EXPORT
IMG_HANDLE SGXRegisterHWTransferContextKM(IMG_HANDLE				psDeviceNode,
										  IMG_DEV_VIRTADDR			*psHWTransferContextDevVAddr,
										  PVRSRV_PER_PROCESS_DATA	*psPerProc)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup;
	PRESMAN_ITEM psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(SGX_HW_TRANSFER_CONTEXT_CLEANUP),
						(IMG_VOID **)&psCleanup,
						&hBlockAlloc,
						"SGX Hardware Transfer Context Cleanup");

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWTransferContextKM: Couldn't allocate memory for SGX_HW_TRANSFER_CONTEXT_CLEANUP structure"));
		return IMG_NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHWTransferContextDevVAddr = *psHWTransferContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_HW_TRANSFER_CONTEXT,
								  psCleanup,
								  0,
								  &SGXCleanupHWTransferContextCallback);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWTransferContextKM: ResManRegisterRes failed"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(SGX_HW_TRANSFER_CONTEXT_CLEANUP),
				  psCleanup,
				  psCleanup->hBlockAlloc);
		

		return IMG_NULL;
	}

	psCleanup->psResItem = psResItem;

	return (IMG_HANDLE)psCleanup;
}

IMG_EXPORT
PVRSRV_ERROR SGXUnregisterHWTransferContextKM(IMG_HANDLE hHWTransferContext)
{
	PVRSRV_ERROR eError;
	SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHWTransferContext != IMG_NULL);

	psCleanup = (SGX_HW_TRANSFER_CONTEXT_CLEANUP *)hHWTransferContext;

	if (psCleanup == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWTransferContextKM: invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = ResManFreeResByPtr(psCleanup->psResItem);

	return eError;
}

#if defined(SGX_FEATURE_2D_HARDWARE)
typedef struct _SGX_HW_2D_CONTEXT_CLEANUP_
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_DEV_VIRTADDR sHW2DContextDevVAddr;
	IMG_HANDLE hBlockAlloc;
	PRESMAN_ITEM psResItem;
} SGX_HW_2D_CONTEXT_CLEANUP;

static PVRSRV_ERROR SGXCleanupHW2DContextCallback(IMG_PVOID pvParam, IMG_UINT32 ui32Param)
{
	SGX_HW_2D_CONTEXT_CLEANUP *psCleanup = (SGX_HW_2D_CONTEXT_CLEANUP *)pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
					  &psCleanup->sHW2DContextDevVAddr,
					  PVRSRV_CLEANUPCMD_2DC);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(SGX_HW_2D_CONTEXT_CLEANUP),
			  psCleanup,
			  psCleanup->hBlockAlloc);
	

	return PVRSRV_OK;
}

IMG_EXPORT
IMG_HANDLE SGXRegisterHW2DContextKM(IMG_HANDLE				psDeviceNode,
									IMG_DEV_VIRTADDR		*psHW2DContextDevVAddr,
									PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	SGX_HW_2D_CONTEXT_CLEANUP *psCleanup;
	PRESMAN_ITEM psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(SGX_HW_2D_CONTEXT_CLEANUP),
						(IMG_VOID **)&psCleanup,
						&hBlockAlloc,
						"SGX Hardware 2D Context Cleanup");

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHW2DContextKM: Couldn't allocate memory for SGX_HW_2D_CONTEXT_CLEANUP structure"));
		return IMG_NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHW2DContextDevVAddr = *psHW2DContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_HW_2D_CONTEXT,
								  psCleanup,
								  0,
								  &SGXCleanupHW2DContextCallback);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHW2DContextKM: ResManRegisterRes failed"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(SGX_HW_2D_CONTEXT_CLEANUP),
				  psCleanup,
				  psCleanup->hBlockAlloc);
		

		return IMG_NULL;
	}

	psCleanup->psResItem = psResItem;

	return (IMG_HANDLE)psCleanup;
}

IMG_EXPORT
PVRSRV_ERROR SGXUnregisterHW2DContextKM(IMG_HANDLE hHW2DContext)
{
	PVRSRV_ERROR eError;
	SGX_HW_2D_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHW2DContext != IMG_NULL);

	if (hHW2DContext == IMG_NULL)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psCleanup = (SGX_HW_2D_CONTEXT_CLEANUP *)hHW2DContext;

	eError = ResManFreeResByPtr(psCleanup->psResItem);

	return eError;
}
#endif 

#ifdef INLINE_IS_PRAGMA
#pragma inline(SGX2DQuerySyncOpsComplete)
#endif
static INLINE
IMG_BOOL SGX2DQuerySyncOpsComplete(PVRSRV_KERNEL_SYNC_INFO	*psSyncInfo,
								   IMG_UINT32				ui32ReadOpsPending,
								   IMG_UINT32				ui32WriteOpsPending)
{
	PVRSRV_SYNC_DATA *psSyncData = psSyncInfo->psSyncData;

	return (IMG_BOOL)(
					  (psSyncData->ui32ReadOpsComplete >= ui32ReadOpsPending) &&
					  (psSyncData->ui32WriteOpsComplete >= ui32WriteOpsPending)
					 );
}

IMG_EXPORT
PVRSRV_ERROR SGX2DQueryBlitsCompleteKM(PVRSRV_SGXDEV_INFO	*psDevInfo,
									   PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
									   IMG_BOOL bWaitForComplete)
{
	IMG_UINT32	ui32ReadOpsPending, ui32WriteOpsPending;

	PVR_UNREFERENCED_PARAMETER(psDevInfo);

	PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: Start"));

	ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOpsPending;
	ui32WriteOpsPending = psSyncInfo->psSyncData->ui32WriteOpsPending;

	if(SGX2DQuerySyncOpsComplete(psSyncInfo, ui32ReadOpsPending, ui32WriteOpsPending))
	{
		
		PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: No wait. Blits complete."));
		return PVRSRV_OK;
	}

	
	if (!bWaitForComplete)
	{
		
		PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: No wait. Ops pending."));
		return PVRSRV_ERROR_CMD_NOT_PROCESSED;
	}

	
	PVR_DPF((PVR_DBG_MESSAGE, "SGX2DQueryBlitsCompleteKM: Ops pending. Start polling."));

	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);

		if(SGX2DQuerySyncOpsComplete(psSyncInfo, ui32ReadOpsPending, ui32WriteOpsPending))
		{
			
			PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: Wait over.  Blits complete."));
			return PVRSRV_OK;
		}

		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT();

	
	PVR_DPF((PVR_DBG_ERROR,"SGX2DQueryBlitsCompleteKM: Timed out. Ops pending."));

#if defined(DEBUG)
	{
		PVRSRV_SYNC_DATA *psSyncData = psSyncInfo->psSyncData;

		PVR_TRACE(("SGX2DQueryBlitsCompleteKM: Syncinfo: 0x%x, Syncdata: 0x%x",
				(IMG_UINTPTR_T)psSyncInfo, (IMG_UINTPTR_T)psSyncData));

		PVR_TRACE(("SGX2DQueryBlitsCompleteKM: Read ops complete: %d, Read ops pending: %d", psSyncData->ui32ReadOpsComplete, psSyncData->ui32ReadOpsPending));
		PVR_TRACE(("SGX2DQueryBlitsCompleteKM: Write ops complete: %d, Write ops pending: %d", psSyncData->ui32WriteOpsComplete, psSyncData->ui32WriteOpsPending));

	}
#endif

	return PVRSRV_ERROR_TIMEOUT;
}


IMG_EXPORT
IMG_VOID SGXFlushHWRenderTargetKM(IMG_HANDLE psDeviceNode, IMG_DEV_VIRTADDR sHWRTDataSetDevVAddr)
{
	PVR_ASSERT(sHWRTDataSetDevVAddr.uiAddr != IMG_NULL);

	SGXCleanupRequest(psDeviceNode,
					  &sHWRTDataSetDevVAddr,
					  PVRSRV_CLEANUPCMD_RT);
}


IMG_UINT32 SGXConvertTimeStamp(PVRSRV_SGXDEV_INFO	*psDevInfo,
							   IMG_UINT32			ui32TimeWraps,
							   IMG_UINT32			ui32Time)
{
#if defined(EUR_CR_TIMER)
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32TimeWraps);
	return ui32Time;
#else
	IMG_UINT64	ui64Clocks;
	IMG_UINT32	ui32Clocksx16;

	ui64Clocks = ((IMG_UINT64)ui32TimeWraps * psDevInfo->ui32uKernelTimerClock) +
					(psDevInfo->ui32uKernelTimerClock - (ui32Time & EUR_CR_EVENT_TIMER_VALUE_MASK));
	ui32Clocksx16 = (IMG_UINT32)(ui64Clocks / 16);

	return ui32Clocksx16;
#endif 
}



