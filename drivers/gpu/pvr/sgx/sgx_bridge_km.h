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

#if !defined(__SGX_BRIDGE_KM_H__)
#define __SGX_BRIDGE_KM_H__

#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "sgx_bridge.h"
#include "pvr_bridge.h"
#include "perproc.h"

#if defined (__cplusplus)
extern "C" {
#endif

IMG_IMPORT
PVRSRV_ERROR SGXSubmitTransferKM(IMG_HANDLE hDevHandle, PVRSRV_TRANSFER_SGX_KICK *psKick);

#if defined(SGX_FEATURE_2D_HARDWARE)
IMG_IMPORT
PVRSRV_ERROR SGXSubmit2DKM(IMG_HANDLE hDevHandle, PVRSRV_2D_SGX_KICK *psKick);
#endif

IMG_IMPORT
PVRSRV_ERROR SGXDoKickKM(IMG_HANDLE hDevHandle,
						 SGX_CCB_KICK *psCCBKick);

IMG_IMPORT
PVRSRV_ERROR SGXGetPhysPageAddrKM(IMG_HANDLE hDevMemHeap,
								  IMG_DEV_VIRTADDR sDevVAddr,
								  IMG_DEV_PHYADDR *pDevPAddr,
								  IMG_CPU_PHYADDR *pCpuPAddr);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV SGXGetMMUPDAddrKM(IMG_HANDLE		hDevCookie,
											IMG_HANDLE		hDevMemContext,
											IMG_DEV_PHYADDR	*psPDDevPAddr);

IMG_IMPORT
PVRSRV_ERROR SGXGetClientInfoKM(IMG_HANDLE				hDevCookie,
								SGX_CLIENT_INFO*	psClientInfo);

IMG_IMPORT
PVRSRV_ERROR SGXGetMiscInfoKM(PVRSRV_SGXDEV_INFO	*psDevInfo,
							  SGX_MISC_INFO			*psMiscInfo,
							  PVRSRV_DEVICE_NODE 	*psDeviceNode,
							  IMG_HANDLE 			 hDevMemContext);

IMG_IMPORT
PVRSRV_ERROR SGXReadHWPerfCBKM(IMG_HANDLE					hDevHandle,
							   IMG_UINT32					ui32ArraySize,
							   PVRSRV_SGX_HWPERF_CB_ENTRY	*psHWPerfCBData,
							   IMG_UINT32					*pui32DataCount,
							   IMG_UINT32					*pui32ClockSpeed,
							   IMG_UINT32					*pui32HostTimeStamp);

IMG_IMPORT
PVRSRV_ERROR SGX2DQueryBlitsCompleteKM(PVRSRV_SGXDEV_INFO		*psDevInfo,
									   PVRSRV_KERNEL_SYNC_INFO	*psSyncInfo,
									   IMG_BOOL bWaitForComplete);

IMG_IMPORT
PVRSRV_ERROR SGXGetInfoForSrvinitKM(IMG_HANDLE hDevHandle,
									SGX_BRIDGE_INFO_FOR_SRVINIT *psInitInfo);

IMG_IMPORT
PVRSRV_ERROR DevInitSGXPart2KM(PVRSRV_PER_PROCESS_DATA *psPerProc,
							   IMG_HANDLE hDevHandle,
							   SGX_BRIDGE_INIT_INFO *psInitInfo);

IMG_IMPORT PVRSRV_ERROR
SGXFindSharedPBDescKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
					  IMG_HANDLE				hDevCookie,
					  IMG_BOOL				bLockOnFailure,
					  IMG_UINT32				ui32TotalPBSize,
					  IMG_HANDLE				*phSharedPBDesc,
					  PVRSRV_KERNEL_MEM_INFO	**ppsSharedPBDescKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	**ppsHWPBDescKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	**ppsBlockKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	**ppsHWBlockKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	***pppsSharedPBDescSubKernelMemInfos,
					  IMG_UINT32				*ui32SharedPBDescSubKernelMemInfosCount);

IMG_IMPORT PVRSRV_ERROR
SGXUnrefSharedPBDescKM(IMG_HANDLE hSharedPBDesc);

IMG_IMPORT PVRSRV_ERROR
SGXAddSharedPBDescKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
					 IMG_HANDLE 				hDevCookie,
					 PVRSRV_KERNEL_MEM_INFO		*psSharedPBDescKernelMemInfo,
					 PVRSRV_KERNEL_MEM_INFO		*psHWPBDescKernelMemInfo,
					 PVRSRV_KERNEL_MEM_INFO		*psBlockKernelMemInfo,
					 PVRSRV_KERNEL_MEM_INFO		*psHWBlockKernelMemInfo,
					 IMG_UINT32					ui32TotalPBSize,
					 IMG_HANDLE					*phSharedPBDesc,
					 PVRSRV_KERNEL_MEM_INFO		**psSharedPBDescSubKernelMemInfos,
					 IMG_UINT32					ui32SharedPBDescSubKernelMemInfosCount);


IMG_IMPORT PVRSRV_ERROR
SGXGetInternalDevInfoKM(IMG_HANDLE hDevCookie,
						SGX_INTERNAL_DEVINFO *psSGXInternalDevInfo);

#if defined (__cplusplus)
}
#endif

#endif 

