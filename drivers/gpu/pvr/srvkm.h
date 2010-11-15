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

#ifndef SRVKM_H
#define SRVKM_H


#if defined(__cplusplus)
extern "C" {
#endif

	
	#ifdef PVR_DISABLE_LOGGING
	#define PVR_LOG(X)
	#else
	 
	#define PVR_LOG(X)			PVRSRVReleasePrintf X;
	#endif

	IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVReleasePrintf(const IMG_CHAR *pszFormat, ...) IMG_FORMAT_PRINTF(1, 2);

	IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVProcessConnect(IMG_UINT32	ui32PID, IMG_UINT32 ui32Flags);
	IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVProcessDisconnect(IMG_UINT32	ui32PID);

	IMG_IMPORT IMG_VOID PVRSRVScheduleDevicesKM(IMG_VOID);

	IMG_VOID IMG_CALLCONV PVRSRVSetDCState(IMG_UINT32 ui32State);

	PVRSRV_ERROR IMG_CALLCONV PVRSRVSaveRestoreLiveSegments(IMG_HANDLE hArena, IMG_PBYTE pbyBuffer, IMG_SIZE_T *puiBufSize, IMG_BOOL bSave);

	IMG_VOID PVRSRVScheduleDeviceCallbacks(IMG_VOID);


#if defined (__cplusplus)
}
#endif

 
#define LOOP_UNTIL_TIMEOUT(TIMEOUT) \
{\
	IMG_UINT32 uiOffset, uiStart, uiCurrent; \
	IMG_INT32 iNotLastLoop;					 \
	for(uiOffset = 0, uiStart = OSClockus(), uiCurrent = uiStart + 1, iNotLastLoop = 1;\
		((uiCurrent - uiStart + uiOffset) < (TIMEOUT)) || iNotLastLoop--;				\
		uiCurrent = OSClockus(),													\
		uiOffset = uiCurrent < uiStart ? IMG_UINT32_MAX - uiStart : uiOffset,		\
		uiStart = uiCurrent < uiStart ? 0 : uiStart)

#define END_LOOP_UNTIL_TIMEOUT() \
}

IMG_IMPORT
const IMG_CHAR *PVRSRVGetErrorStringKM(PVRSRV_ERROR eError);

#endif 
