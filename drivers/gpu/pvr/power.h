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

#ifndef POWER_H
#define POWER_H

#if defined(__cplusplus)
extern "C" {
#endif


 
typedef struct _PVRSRV_POWER_DEV_TAG_
{
	PFN_PRE_POWER					pfnPrePower;
	PFN_POST_POWER					pfnPostPower;
	PFN_PRE_CLOCKSPEED_CHANGE		pfnPreClockSpeedChange;
	PFN_POST_CLOCKSPEED_CHANGE		pfnPostClockSpeedChange;
	IMG_HANDLE						hDevCookie;
	IMG_UINT32						ui32DeviceIndex;
	PVRSRV_DEV_POWER_STATE 			eDefaultPowerState;
	PVRSRV_DEV_POWER_STATE 			eCurrentPowerState;
	struct _PVRSRV_POWER_DEV_TAG_	*psNext;
	struct _PVRSRV_POWER_DEV_TAG_	**ppsThis;

} PVRSRV_POWER_DEV;

typedef enum _PVRSRV_INIT_SERVER_STATE_
{
	PVRSRV_INIT_SERVER_Unspecified		= -1,	
	PVRSRV_INIT_SERVER_RUNNING			= 0,	
	PVRSRV_INIT_SERVER_RAN				= 1,	
	PVRSRV_INIT_SERVER_SUCCESSFUL		= 2,	
	PVRSRV_INIT_SERVER_NUM				= 3,	
	PVRSRV_INIT_SERVER_FORCE_I32 = 0x7fffffff

} PVRSRV_INIT_SERVER_STATE, *PPVRSRV_INIT_SERVER_STATE;

IMG_IMPORT
IMG_BOOL PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_STATE	eInitServerState);

IMG_IMPORT
PVRSRV_ERROR PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_STATE	eInitServerState, IMG_BOOL bState);



IMG_IMPORT
PVRSRV_ERROR PVRSRVPowerLock(IMG_UINT32	ui32CallerID,
							 IMG_BOOL	bSystemPowerEvent);
IMG_IMPORT
IMG_VOID PVRSRVPowerUnlock(IMG_UINT32	ui32CallerID);

IMG_IMPORT
PVRSRV_ERROR PVRSRVSetDevicePowerStateKM(IMG_UINT32				ui32DeviceIndex,
										 PVRSRV_DEV_POWER_STATE	eNewPowerState,
										 IMG_UINT32				ui32CallerID,
										 IMG_BOOL				bRetainMutex);

IMG_IMPORT
PVRSRV_ERROR PVRSRVSystemPrePowerStateKM(PVRSRV_SYS_POWER_STATE eNewPowerState);
IMG_IMPORT
PVRSRV_ERROR PVRSRVSystemPostPowerStateKM(PVRSRV_SYS_POWER_STATE eNewPowerState);

IMG_IMPORT
PVRSRV_ERROR PVRSRVSetPowerStateKM (PVRSRV_SYS_POWER_STATE ePVRState);

IMG_IMPORT
PVRSRV_ERROR PVRSRVRegisterPowerDevice(IMG_UINT32					ui32DeviceIndex,
									   PFN_PRE_POWER				pfnPrePower,
									   PFN_POST_POWER				pfnPostPower,
									   PFN_PRE_CLOCKSPEED_CHANGE	pfnPreClockSpeedChange,
									   PFN_POST_CLOCKSPEED_CHANGE	pfnPostClockSpeedChange,
									   IMG_HANDLE					hDevCookie,
									   PVRSRV_DEV_POWER_STATE		eCurrentPowerState,
									   PVRSRV_DEV_POWER_STATE		eDefaultPowerState);

IMG_IMPORT
PVRSRV_ERROR PVRSRVRemovePowerDevice (IMG_UINT32 ui32DeviceIndex);

IMG_IMPORT
IMG_BOOL PVRSRVIsDevicePowered(IMG_UINT32 ui32DeviceIndex);

IMG_IMPORT
PVRSRV_ERROR PVRSRVDevicePreClockSpeedChange(IMG_UINT32	ui32DeviceIndex,
											 IMG_BOOL	bIdleDevice,
											 IMG_VOID	*pvInfo);

IMG_IMPORT
IMG_VOID PVRSRVDevicePostClockSpeedChange(IMG_UINT32	ui32DeviceIndex,
										  IMG_BOOL		bIdleDevice,
										  IMG_VOID		*pvInfo);

#if defined (__cplusplus)
}
#endif
#endif 

