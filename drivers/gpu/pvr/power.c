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
#include "pdump_km.h"

#include "lists.h"

#ifdef __linux__
#include <linux/delay.h>
#endif

static IMG_BOOL gbInitServerRunning = IMG_FALSE;
static IMG_BOOL gbInitServerRan = IMG_FALSE;
static IMG_BOOL gbInitSuccessful = IMG_FALSE;

IMG_EXPORT
PVRSRV_ERROR PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_STATE eInitServerState, IMG_BOOL bState)
{

	switch(eInitServerState)
	{
		case PVRSRV_INIT_SERVER_RUNNING:
			gbInitServerRunning	= bState;
			break;
		case PVRSRV_INIT_SERVER_RAN:
			gbInitServerRan	= bState;
			break;
		case PVRSRV_INIT_SERVER_SUCCESSFUL:
			gbInitSuccessful = bState;
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR,
				"PVRSRVSetInitServerState : Unknown state %x", eInitServerState));
			return PVRSRV_ERROR_UNKNOWN_INIT_SERVER_STATE;
	}

	return PVRSRV_OK;
}

IMG_EXPORT
IMG_BOOL PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_STATE eInitServerState)
{
	IMG_BOOL	bReturnVal;

	switch(eInitServerState)
	{
		case PVRSRV_INIT_SERVER_RUNNING:
			bReturnVal = gbInitServerRunning;
			break;
		case PVRSRV_INIT_SERVER_RAN:
			bReturnVal = gbInitServerRan;
			break;
		case PVRSRV_INIT_SERVER_SUCCESSFUL:
			bReturnVal = gbInitSuccessful;
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR,
				"PVRSRVGetInitServerState : Unknown state %x", eInitServerState));
			bReturnVal = IMG_FALSE;
	}

	return bReturnVal;
}

static IMG_BOOL _IsSystemStatePowered(PVRSRV_SYS_POWER_STATE eSystemPowerState)
{
	return (IMG_BOOL)(eSystemPowerState < PVRSRV_SYS_POWER_STATE_D2);
}


IMG_EXPORT
PVRSRV_ERROR PVRSRVPowerLock(IMG_UINT32	ui32CallerID,
							 IMG_BOOL	bSystemPowerEvent)
{
	PVRSRV_ERROR	eError;
	SYS_DATA		*psSysData;

#ifdef __linux__	
	IMG_UINT32		ui32Timeout = 1000;
#else
	IMG_UINT32		ui32Timeout = 1000000;
#endif

#if defined(SUPPORT_LMA)
	
	ui32Timeout *= 60;
#endif 

	SysAcquireData(&psSysData);

#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
	eError = SysPowerLockWrap(psSysData);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}
#endif
	do
	{
		eError = OSLockResource(&psSysData->sPowerStateChangeResource,
								ui32CallerID);
		if (eError == PVRSRV_OK)
		{
			break;
		}
		else if (ui32CallerID == ISR_ID)
		{
			

			eError = PVRSRV_ERROR_RETRY;
			break;
		}

#ifdef __linux__
		msleep(1);
#else
		OSWaitus(1);
#endif
		ui32Timeout--;
	} while (ui32Timeout > 0);

#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
	if (eError != PVRSRV_OK)
	{
		SysPowerLockUnwrap(psSysData);
	}
#endif
	 
	if ((eError == PVRSRV_OK) &&
		!bSystemPowerEvent &&
		!_IsSystemStatePowered(psSysData->eCurrentPowerState))
	{
		
		PVRSRVPowerUnlock(ui32CallerID);
		eError = PVRSRV_ERROR_RETRY;
	}

	return eError;
}


IMG_EXPORT
IMG_VOID PVRSRVPowerUnlock(IMG_UINT32	ui32CallerID)
{
	OSUnlockResource(&gpsSysData->sPowerStateChangeResource, ui32CallerID);
#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
	SysPowerLockUnwrap(gpsSysData);
#endif
}


static PVRSRV_ERROR PVRSRVDevicePrePowerStateKM_AnyVaCb(PVRSRV_POWER_DEV *psPowerDevice, va_list va)
{
	PVRSRV_DEV_POWER_STATE	eNewDevicePowerState;
	PVRSRV_ERROR			eError;

	
	IMG_BOOL				bAllDevices;
	IMG_UINT32				ui32DeviceIndex;
	PVRSRV_DEV_POWER_STATE	eNewPowerState;

	
	bAllDevices = va_arg(va, IMG_BOOL);
	ui32DeviceIndex = va_arg(va, IMG_UINT32);
	eNewPowerState = va_arg(va, PVRSRV_DEV_POWER_STATE);

	if (bAllDevices || (ui32DeviceIndex == psPowerDevice->ui32DeviceIndex))
	{
		eNewDevicePowerState = (eNewPowerState == PVRSRV_DEV_POWER_STATE_DEFAULT) ?
							psPowerDevice->eDefaultPowerState : eNewPowerState;

		if (psPowerDevice->eCurrentPowerState != eNewDevicePowerState)
		{
			if (psPowerDevice->pfnPrePower != IMG_NULL)
			{
				
				eError = psPowerDevice->pfnPrePower(psPowerDevice->hDevCookie,
													eNewDevicePowerState,
													psPowerDevice->eCurrentPowerState);
				if (eError != PVRSRV_OK)
				{
					return eError;
				}
			}

			
			eError = SysDevicePrePowerState(psPowerDevice->ui32DeviceIndex,
											eNewDevicePowerState,
											psPowerDevice->eCurrentPowerState);
			if (eError != PVRSRV_OK)
			{
				return eError;
			}
		}
	}

	return  PVRSRV_OK;
}

static
PVRSRV_ERROR PVRSRVDevicePrePowerStateKM(IMG_BOOL				bAllDevices,
										 IMG_UINT32				ui32DeviceIndex,
										 PVRSRV_DEV_POWER_STATE	eNewPowerState)
{
	PVRSRV_ERROR		eError;
	SYS_DATA			*psSysData;

	SysAcquireData(&psSysData);

	
	eError = List_PVRSRV_POWER_DEV_PVRSRV_ERROR_Any_va(psSysData->psPowerDeviceList,
														&PVRSRVDevicePrePowerStateKM_AnyVaCb,
														bAllDevices,
														ui32DeviceIndex,
														eNewPowerState);

	return eError;
}

static PVRSRV_ERROR PVRSRVDevicePostPowerStateKM_AnyVaCb(PVRSRV_POWER_DEV *psPowerDevice, va_list va)
{
	PVRSRV_DEV_POWER_STATE	eNewDevicePowerState;
	PVRSRV_ERROR			eError;

	
	IMG_BOOL				bAllDevices;
	IMG_UINT32				ui32DeviceIndex;
	PVRSRV_DEV_POWER_STATE	eNewPowerState;

	
	bAllDevices = va_arg(va, IMG_BOOL);
	ui32DeviceIndex = va_arg(va, IMG_UINT32);
	eNewPowerState = va_arg(va, PVRSRV_DEV_POWER_STATE);

	if (bAllDevices || (ui32DeviceIndex == psPowerDevice->ui32DeviceIndex))
	{
		eNewDevicePowerState = (eNewPowerState == PVRSRV_DEV_POWER_STATE_DEFAULT) ?
								psPowerDevice->eDefaultPowerState : eNewPowerState;

		if (psPowerDevice->eCurrentPowerState != eNewDevicePowerState)
		{
			
			eError = SysDevicePostPowerState(psPowerDevice->ui32DeviceIndex,
											 eNewDevicePowerState,
											 psPowerDevice->eCurrentPowerState);
			if (eError != PVRSRV_OK)
			{
				return eError;
			}

			if (psPowerDevice->pfnPostPower != IMG_NULL)
			{
				
				eError = psPowerDevice->pfnPostPower(psPowerDevice->hDevCookie,
													 eNewDevicePowerState,
													 psPowerDevice->eCurrentPowerState);
				if (eError != PVRSRV_OK)
				{
					return eError;
				}
			}

			psPowerDevice->eCurrentPowerState = eNewDevicePowerState;
		}
	}
	return PVRSRV_OK;
}

static
PVRSRV_ERROR PVRSRVDevicePostPowerStateKM(IMG_BOOL					bAllDevices,
										  IMG_UINT32				ui32DeviceIndex,
										  PVRSRV_DEV_POWER_STATE	eNewPowerState)
{
	PVRSRV_ERROR		eError;
	SYS_DATA			*psSysData;

	SysAcquireData(&psSysData);

	
	eError = List_PVRSRV_POWER_DEV_PVRSRV_ERROR_Any_va(psSysData->psPowerDeviceList,
														&PVRSRVDevicePostPowerStateKM_AnyVaCb,
														bAllDevices,
														ui32DeviceIndex,
														eNewPowerState);

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR PVRSRVSetDevicePowerStateKM(IMG_UINT32				ui32DeviceIndex,
										 PVRSRV_DEV_POWER_STATE	eNewPowerState,
										 IMG_UINT32				ui32CallerID,
										 IMG_BOOL				bRetainMutex)
{
	PVRSRV_ERROR	eError;
	SYS_DATA		*psSysData;

	SysAcquireData(&psSysData);

	eError = PVRSRVPowerLock(ui32CallerID, IMG_FALSE);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}

	#if defined(PDUMP)
	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_DEFAULT)
	{
		



		eError = PVRSRVDevicePrePowerStateKM(IMG_FALSE, ui32DeviceIndex, PVRSRV_DEV_POWER_STATE_ON);
		if(eError != PVRSRV_OK)
		{
			goto Exit;
		}

		eError = PVRSRVDevicePostPowerStateKM(IMG_FALSE, ui32DeviceIndex, PVRSRV_DEV_POWER_STATE_ON);

		if (eError != PVRSRV_OK)
		{
			goto Exit;
		}

		PDUMPSUSPEND();
	}
	#endif 

	eError = PVRSRVDevicePrePowerStateKM(IMG_FALSE, ui32DeviceIndex, eNewPowerState);
	if(eError != PVRSRV_OK)
	{
		if (eNewPowerState == PVRSRV_DEV_POWER_STATE_DEFAULT)
		{
			PDUMPRESUME();
		}
		goto Exit;
	}

	eError = PVRSRVDevicePostPowerStateKM(IMG_FALSE, ui32DeviceIndex, eNewPowerState);

	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_DEFAULT)
	{
		PDUMPRESUME();
	}

Exit:

	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"PVRSRVSetDevicePowerStateKM : Transition to %d FAILED 0x%x", eNewPowerState, eError));
	}

	if (!bRetainMutex || (eError != PVRSRV_OK))
	{
		PVRSRVPowerUnlock(ui32CallerID);
	}

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR PVRSRVSystemPrePowerStateKM(PVRSRV_SYS_POWER_STATE eNewSysPowerState)
{
	PVRSRV_ERROR			eError;
	SYS_DATA				*psSysData;
	PVRSRV_DEV_POWER_STATE	eNewDevicePowerState;

	SysAcquireData(&psSysData);

	
	eError = PVRSRVPowerLock(KERNEL_ID, IMG_TRUE);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}

	if (_IsSystemStatePowered(eNewSysPowerState) !=
		_IsSystemStatePowered(psSysData->eCurrentPowerState))
	{
		if (_IsSystemStatePowered(eNewSysPowerState))
		{
			
			eNewDevicePowerState = PVRSRV_DEV_POWER_STATE_DEFAULT;
		}
		else
		{
			eNewDevicePowerState = PVRSRV_DEV_POWER_STATE_OFF;
		}

		
		eError = PVRSRVDevicePrePowerStateKM(IMG_TRUE, 0, eNewDevicePowerState);
		if (eError != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}

	if (eNewSysPowerState != psSysData->eCurrentPowerState)
	{
		
		eError = SysSystemPrePowerState(eNewSysPowerState);
		if (eError != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}

	return eError;

ErrorExit:

	PVR_DPF((PVR_DBG_ERROR,
			"PVRSRVSystemPrePowerStateKM: Transition from %d to %d FAILED 0x%x",
			psSysData->eCurrentPowerState, eNewSysPowerState, eError));

	
	psSysData->eFailedPowerState = eNewSysPowerState;

	PVRSRVPowerUnlock(KERNEL_ID);

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR PVRSRVSystemPostPowerStateKM(PVRSRV_SYS_POWER_STATE eNewSysPowerState)
{
	PVRSRV_ERROR			eError = PVRSRV_OK;
	SYS_DATA				*psSysData;
	PVRSRV_DEV_POWER_STATE	eNewDevicePowerState;

	SysAcquireData(&psSysData);

	if (eNewSysPowerState != psSysData->eCurrentPowerState)
	{
		
		eError = SysSystemPostPowerState(eNewSysPowerState);
		if (eError != PVRSRV_OK)
		{
			goto Exit;
		}
	}

	if (_IsSystemStatePowered(eNewSysPowerState) !=
		_IsSystemStatePowered(psSysData->eCurrentPowerState))
	{
		if (_IsSystemStatePowered(eNewSysPowerState))
		{
			
			eNewDevicePowerState = PVRSRV_DEV_POWER_STATE_DEFAULT;
		}
		else
		{
			eNewDevicePowerState = PVRSRV_DEV_POWER_STATE_OFF;
		}

		
		eError = PVRSRVDevicePostPowerStateKM(IMG_TRUE, 0, eNewDevicePowerState);
		if (eError != PVRSRV_OK)
		{
			goto Exit;
		}
	}

	PVR_DPF((PVR_DBG_MESSAGE,
			"PVRSRVSystemPostPowerStateKM: System Power Transition from %d to %d OK",
			psSysData->eCurrentPowerState, eNewSysPowerState));

	psSysData->eCurrentPowerState = eNewSysPowerState;

Exit:

	PVRSRVPowerUnlock(KERNEL_ID);

	 
	if (_IsSystemStatePowered(eNewSysPowerState) &&
			PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL))
	{
		


		PVRSRVScheduleDeviceCallbacks();
	}

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR PVRSRVSetPowerStateKM(PVRSRV_SYS_POWER_STATE eNewSysPowerState)
{
	PVRSRV_ERROR	eError;
	SYS_DATA		*psSysData;

	SysAcquireData(&psSysData);

	eError = PVRSRVSystemPrePowerStateKM(eNewSysPowerState);
	if(eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	eError = PVRSRVSystemPostPowerStateKM(eNewSysPowerState);
	if(eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	
	psSysData->eFailedPowerState = PVRSRV_SYS_POWER_STATE_Unspecified;

	return PVRSRV_OK;

ErrorExit:

	PVR_DPF((PVR_DBG_ERROR,
			"PVRSRVSetPowerStateKM: Transition from %d to %d FAILED 0x%x",
			psSysData->eCurrentPowerState, eNewSysPowerState, eError));

	
	psSysData->eFailedPowerState = eNewSysPowerState;

	return eError;
}


PVRSRV_ERROR PVRSRVRegisterPowerDevice(IMG_UINT32					ui32DeviceIndex,
									   PFN_PRE_POWER				pfnPrePower,
									   PFN_POST_POWER				pfnPostPower,
									   PFN_PRE_CLOCKSPEED_CHANGE	pfnPreClockSpeedChange,
									   PFN_POST_CLOCKSPEED_CHANGE	pfnPostClockSpeedChange,
									   IMG_HANDLE					hDevCookie,
									   PVRSRV_DEV_POWER_STATE		eCurrentPowerState,
									   PVRSRV_DEV_POWER_STATE		eDefaultPowerState)
{
	PVRSRV_ERROR		eError;
	SYS_DATA			*psSysData;
	PVRSRV_POWER_DEV	*psPowerDevice;

	if (pfnPrePower == IMG_NULL &&
		pfnPostPower == IMG_NULL)
	{
		return PVRSRVRemovePowerDevice(ui32DeviceIndex);
	}

	SysAcquireData(&psSysData);

	eError = OSAllocMem( PVRSRV_OS_PAGEABLE_HEAP,
						 sizeof(PVRSRV_POWER_DEV),
						 (IMG_VOID **)&psPowerDevice, IMG_NULL,
						 "Power Device");
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVRegisterPowerDevice: Failed to alloc PVRSRV_POWER_DEV"));
		return eError;
	}

	
	psPowerDevice->pfnPrePower = pfnPrePower;
	psPowerDevice->pfnPostPower = pfnPostPower;
	psPowerDevice->pfnPreClockSpeedChange = pfnPreClockSpeedChange;
	psPowerDevice->pfnPostClockSpeedChange = pfnPostClockSpeedChange;
	psPowerDevice->hDevCookie = hDevCookie;
	psPowerDevice->ui32DeviceIndex = ui32DeviceIndex;
	psPowerDevice->eCurrentPowerState = eCurrentPowerState;
	psPowerDevice->eDefaultPowerState = eDefaultPowerState;

	
	List_PVRSRV_POWER_DEV_Insert(&(psSysData->psPowerDeviceList), psPowerDevice);

	return (PVRSRV_OK);
}


PVRSRV_ERROR PVRSRVRemovePowerDevice (IMG_UINT32 ui32DeviceIndex)
{
	SYS_DATA			*psSysData;
	PVRSRV_POWER_DEV	*psPowerDev;

	SysAcquireData(&psSysData);

	
	psPowerDev = (PVRSRV_POWER_DEV*)
					List_PVRSRV_POWER_DEV_Any_va(psSysData->psPowerDeviceList,
												 &MatchPowerDeviceIndex_AnyVaCb,
												 ui32DeviceIndex);

	if (psPowerDev)
	{
		List_PVRSRV_POWER_DEV_Remove(psPowerDev);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(PVRSRV_POWER_DEV), psPowerDev, IMG_NULL);
		
	}

	return (PVRSRV_OK);
}


IMG_EXPORT
IMG_BOOL PVRSRVIsDevicePowered(IMG_UINT32 ui32DeviceIndex)
{
	SYS_DATA			*psSysData;
	PVRSRV_POWER_DEV	*psPowerDevice;

	SysAcquireData(&psSysData);

	 
	if (OSIsResourceLocked(&psSysData->sPowerStateChangeResource, KERNEL_ID) ||
		OSIsResourceLocked(&psSysData->sPowerStateChangeResource, ISR_ID))
	{
		return IMG_FALSE;
	}

	psPowerDevice = (PVRSRV_POWER_DEV*)
					List_PVRSRV_POWER_DEV_Any_va(psSysData->psPowerDeviceList,
												 &MatchPowerDeviceIndex_AnyVaCb,
												 ui32DeviceIndex);
	return (psPowerDevice && (psPowerDevice->eCurrentPowerState == PVRSRV_DEV_POWER_STATE_ON))
			? IMG_TRUE : IMG_FALSE;
}


PVRSRV_ERROR PVRSRVDevicePreClockSpeedChange(IMG_UINT32	ui32DeviceIndex,
											 IMG_BOOL	bIdleDevice,
											 IMG_VOID	*pvInfo)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	SYS_DATA			*psSysData;
	PVRSRV_POWER_DEV	*psPowerDevice;

	PVR_UNREFERENCED_PARAMETER(pvInfo);

	SysAcquireData(&psSysData);

	if (bIdleDevice)
	{
		
		eError = PVRSRVPowerLock(KERNEL_ID, IMG_FALSE);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,	"PVRSRVDevicePreClockSpeedChange : failed to acquire lock, error:0x%x", eError));
			return eError;
		}
	}

	
	psPowerDevice = (PVRSRV_POWER_DEV*)
					List_PVRSRV_POWER_DEV_Any_va(psSysData->psPowerDeviceList,
												 &MatchPowerDeviceIndex_AnyVaCb,
												 ui32DeviceIndex);

	if (psPowerDevice && psPowerDevice->pfnPostClockSpeedChange)
	{
			eError = psPowerDevice->pfnPreClockSpeedChange(psPowerDevice->hDevCookie,
														   bIdleDevice,
														   psPowerDevice->eCurrentPowerState);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,
						"PVRSRVDevicePreClockSpeedChange : Device %u failed, error:0x%x",
						ui32DeviceIndex, eError));
			}
	}

	if (bIdleDevice && eError != PVRSRV_OK)
	{
		PVRSRVPowerUnlock(KERNEL_ID);
	}

	return eError;
}


IMG_VOID PVRSRVDevicePostClockSpeedChange(IMG_UINT32	ui32DeviceIndex,
										  IMG_BOOL		bIdleDevice,
										  IMG_VOID		*pvInfo)
{
	PVRSRV_ERROR		eError;
	SYS_DATA			*psSysData;
	PVRSRV_POWER_DEV	*psPowerDevice;

	PVR_UNREFERENCED_PARAMETER(pvInfo);

	SysAcquireData(&psSysData);

	
	psPowerDevice = (PVRSRV_POWER_DEV*)
					List_PVRSRV_POWER_DEV_Any_va(psSysData->psPowerDeviceList,
												 &MatchPowerDeviceIndex_AnyVaCb,
												 ui32DeviceIndex);

	if (psPowerDevice && psPowerDevice->pfnPostClockSpeedChange)
	{
		eError = psPowerDevice->pfnPostClockSpeedChange(psPowerDevice->hDevCookie,
														bIdleDevice,
														psPowerDevice->eCurrentPowerState);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"PVRSRVDevicePostClockSpeedChange : Device %u failed, error:0x%x",
					ui32DeviceIndex, eError));
		}
	}


	if (bIdleDevice)
	{
		
		PVRSRVPowerUnlock(KERNEL_ID);
	}
}

