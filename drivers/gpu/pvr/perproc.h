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

#ifndef __PERPROC_H__
#define __PERPROC_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include "img_types.h"
#include "resman.h"

#include "handle.h"

typedef struct _PVRSRV_PER_PROCESS_DATA_
{
	IMG_UINT32		ui32PID;
	IMG_HANDLE		hBlockAlloc;
	PRESMAN_CONTEXT 	hResManContext;
	IMG_HANDLE		hPerProcData;
	PVRSRV_HANDLE_BASE 	*psHandleBase;
#if defined (PVR_SECURE_HANDLES)
	
	IMG_BOOL		bHandlesBatched;
#endif  
	IMG_UINT32		ui32RefCount;

	
	IMG_BOOL		bInitProcess;
#if defined(PDUMP)
	
	IMG_BOOL		bPDumpPersistent;
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	
	IMG_BOOL		bPDumpActive;
#endif 
#endif
	
	IMG_HANDLE		hOsPrivateData;
} PVRSRV_PER_PROCESS_DATA;

PVRSRV_PER_PROCESS_DATA *PVRSRVPerProcessData(IMG_UINT32 ui32PID);

PVRSRV_ERROR PVRSRVPerProcessDataConnect(IMG_UINT32	ui32PID, IMG_UINT32 ui32Flags);
IMG_VOID PVRSRVPerProcessDataDisconnect(IMG_UINT32	ui32PID);

PVRSRV_ERROR PVRSRVPerProcessDataInit(IMG_VOID);
PVRSRV_ERROR PVRSRVPerProcessDataDeInit(IMG_VOID);

#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVFindPerProcessData)
#endif
static INLINE
PVRSRV_PER_PROCESS_DATA *PVRSRVFindPerProcessData(IMG_VOID)
{
	return PVRSRVPerProcessData(OSGetCurrentProcessIDKM());
}


#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVProcessPrivateData)
#endif
static INLINE
IMG_HANDLE PVRSRVProcessPrivateData(PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	return (psPerProc != IMG_NULL) ? psPerProc->hOsPrivateData : IMG_NULL;
}


#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVPerProcessPrivateData)
#endif
static INLINE
IMG_HANDLE PVRSRVPerProcessPrivateData(IMG_UINT32 ui32PID)
{
	return PVRSRVProcessPrivateData(PVRSRVPerProcessData(ui32PID));
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVFindPerProcessPrivateData)
#endif
static INLINE
IMG_HANDLE PVRSRVFindPerProcessPrivateData(IMG_VOID)
{
	return PVRSRVProcessPrivateData(PVRSRVFindPerProcessData());
}

#if defined (__cplusplus)
}
#endif

#endif 

