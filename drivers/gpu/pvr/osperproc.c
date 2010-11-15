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
#include "osperproc.h"

#include "env_perproc.h"
#include "proc.h"

extern IMG_UINT32 gui32ReleasePID;

PVRSRV_ERROR OSPerProcessPrivateDataInit(IMG_HANDLE *phOsPrivateData)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc;

	eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				sizeof(PVRSRV_ENV_PER_PROCESS_DATA),
				phOsPrivateData,
				&hBlockAlloc,
				"Environment per Process Data");

	if (eError != PVRSRV_OK)
	{
		*phOsPrivateData = IMG_NULL;

		PVR_DPF((PVR_DBG_ERROR, "%s: OSAllocMem failed (%d)", __FUNCTION__, eError));
		return eError;
	}

	psEnvPerProc = (PVRSRV_ENV_PER_PROCESS_DATA *)*phOsPrivateData;
	OSMemSet(psEnvPerProc, 0, sizeof(*psEnvPerProc));

	psEnvPerProc->hBlockAlloc = hBlockAlloc;

	
	LinuxMMapPerProcessConnect(psEnvPerProc);

#if defined(SUPPORT_DRI_DRM) && defined(PVR_SECURE_DRM_AUTH_EXPORT)
	
	INIT_LIST_HEAD(&psEnvPerProc->sDRMAuthListHead);
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR OSPerProcessPrivateDataDeInit(IMG_HANDLE hOsPrivateData)
{
	PVRSRV_ERROR eError;
	PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc;

	if (hOsPrivateData == IMG_NULL)
	{
		return PVRSRV_OK;
	}

	psEnvPerProc = (PVRSRV_ENV_PER_PROCESS_DATA *)hOsPrivateData;

	
	LinuxMMapPerProcessDisconnect(psEnvPerProc);

	
	RemovePerProcessProcDir(psEnvPerProc);

	eError = OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				sizeof(PVRSRV_ENV_PER_PROCESS_DATA),
				hOsPrivateData,
				psEnvPerProc->hBlockAlloc);
	

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: OSFreeMem failed (%d)", __FUNCTION__, eError));
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR OSPerProcessSetHandleOptions(PVRSRV_HANDLE_BASE *psHandleBase)
{
	return LinuxMMapPerProcessHandleOptions(psHandleBase);
}

IMG_HANDLE LinuxTerminatingProcessPrivateData(IMG_VOID)
{
	if(!gui32ReleasePID)
		return NULL;
	return PVRSRVPerProcessPrivateData(gui32ReleasePID);
}
