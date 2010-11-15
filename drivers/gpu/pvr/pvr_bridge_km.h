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

#ifndef __PVR_BRIDGE_KM_H_
#define __PVR_BRIDGE_KM_H_

#if defined (__cplusplus)
extern "C" {
#endif

#include "pvr_bridge.h"
#include "perproc.h"

#if defined(__linux__)
PVRSRV_ERROR LinuxBridgeInit(IMG_VOID);
IMG_VOID LinuxBridgeDeInit(IMG_VOID);
#endif

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVEnumerateDevicesKM(IMG_UINT32 *pui32NumDevices,
												   PVRSRV_DEVICE_IDENTIFIER *psDevIdList);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVAcquireDeviceDataKM(IMG_UINT32			uiDevIndex,
													PVRSRV_DEVICE_TYPE	eDeviceType,
													IMG_HANDLE			*phDevCookie);
							
IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateCommandQueueKM(IMG_SIZE_T ui32QueueSize,
													 PVRSRV_QUEUE_INFO **ppsQueueInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyCommandQueueKM(PVRSRV_QUEUE_INFO *psQueueInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetDeviceMemHeapsKM(IMG_HANDLE hDevCookie,
													PVRSRV_HEAP_INFO *psHeapInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateDeviceMemContextKM(IMG_HANDLE					hDevCookie,
														 PVRSRV_PER_PROCESS_DATA	*psPerProc,
														 IMG_HANDLE					*phDevMemContext,
														 IMG_UINT32					*pui32ClientHeapCount,
														 PVRSRV_HEAP_INFO			*psHeapInfo,
														 IMG_BOOL					*pbCreated,
														 IMG_BOOL					*pbShared);


IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyDeviceMemContextKM(IMG_HANDLE hDevCookie,
														  IMG_HANDLE hDevMemContext,
														  IMG_BOOL *pbDestroyed);


IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetDeviceMemHeapInfoKM(IMG_HANDLE				hDevCookie,
															IMG_HANDLE			hDevMemContext,
															IMG_UINT32			*pui32ClientHeapCount,
															PVRSRV_HEAP_INFO	*psHeapInfo,
															IMG_BOOL 			*pbShared
					);


IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV _PVRSRVAllocDeviceMemKM(IMG_HANDLE					hDevCookie,
												 PVRSRV_PER_PROCESS_DATA	*psPerProc,
												 IMG_HANDLE					hDevMemHeap,
												 IMG_UINT32					ui32Flags,
												 IMG_SIZE_T					ui32Size,
												 IMG_SIZE_T					ui32Alignment,
												 PVRSRV_KERNEL_MEM_INFO		**ppsMemInfo);


#if defined(PVRSRV_LOG_MEMORY_ALLOCS)
	#define PVRSRVAllocDeviceMemKM(devCookie, perProc, devMemHeap, flags, size, alignment, memInfo, logStr) \
		(PVR_TRACE(("PVRSRVAllocDeviceMemKM(" #devCookie ", " #perProc ", " #devMemHeap ", " #flags ", " #size \
			", " #alignment "," #memInfo "): " logStr " (size = 0x%;x)", size)),\
			_PVRSRVAllocDeviceMemKM(devCookie, perProc, devMemHeap, flags, size, alignment, memInfo))
#else
	#define PVRSRVAllocDeviceMemKM(devCookie, perProc, devMemHeap, flags, size, alignment, memInfo, logStr) \
			_PVRSRVAllocDeviceMemKM(devCookie, perProc, devMemHeap, flags, size, alignment, memInfo)
#endif


IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVFreeDeviceMemKM(IMG_HANDLE			hDevCookie,
												PVRSRV_KERNEL_MEM_INFO	*psMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDissociateDeviceMemKM(IMG_HANDLE			hDevCookie,
												PVRSRV_KERNEL_MEM_INFO	*psMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVReserveDeviceVirtualMemKM(IMG_HANDLE		hDevMemHeap,
														 IMG_DEV_VIRTADDR	*psDevVAddr,
														 IMG_SIZE_T			ui32Size,
														 IMG_SIZE_T			ui32Alignment,
														 PVRSRV_KERNEL_MEM_INFO	**ppsMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVFreeDeviceVirtualMemKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVMapDeviceMemoryKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
												  PVRSRV_KERNEL_MEM_INFO	*psSrcMemInfo,
												  IMG_HANDLE				hDstDevMemHeap,
												  PVRSRV_KERNEL_MEM_INFO	**ppsDstMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVUnmapDeviceMemoryKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVWrapExtMemoryKM(IMG_HANDLE				hDevCookie,
												PVRSRV_PER_PROCESS_DATA	*psPerProc,
												IMG_HANDLE				hDevMemContext,
												IMG_SIZE_T 				ui32ByteSize, 
												IMG_SIZE_T				ui32PageOffset,
												IMG_BOOL				bPhysContig,
												IMG_SYS_PHYADDR	 		*psSysAddr,
												IMG_VOID 				*pvLinAddr,
												IMG_UINT32				ui32Flags,
												PVRSRV_KERNEL_MEM_INFO **ppsMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVUnwrapExtMemoryKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

IMG_IMPORT
PVRSRV_ERROR PVRSRVEnumerateDCKM(PVRSRV_DEVICE_CLASS DeviceClass,
								 IMG_UINT32 *pui32DevCount,
								 IMG_UINT32 *pui32DevID );

IMG_IMPORT
PVRSRV_ERROR PVRSRVOpenDCDeviceKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
								  IMG_UINT32				ui32DeviceID,
								  IMG_HANDLE 				hDevCookie,
								  IMG_HANDLE 				*phDeviceKM);

IMG_IMPORT
PVRSRV_ERROR PVRSRVCloseDCDeviceKM(IMG_HANDLE hDeviceKM, IMG_BOOL bResManCallback);

IMG_IMPORT
PVRSRV_ERROR PVRSRVEnumDCFormatsKM(IMG_HANDLE hDeviceKM,
								   IMG_UINT32 *pui32Count,
								   DISPLAY_FORMAT *psFormat);

IMG_IMPORT
PVRSRV_ERROR PVRSRVEnumDCDimsKM(IMG_HANDLE hDeviceKM,
								DISPLAY_FORMAT *psFormat,
								IMG_UINT32 *pui32Count,
								DISPLAY_DIMS *psDim);

IMG_IMPORT
PVRSRV_ERROR PVRSRVGetDCSystemBufferKM(IMG_HANDLE hDeviceKM,
									   IMG_HANDLE *phBuffer);

IMG_IMPORT
PVRSRV_ERROR PVRSRVGetDCInfoKM(IMG_HANDLE hDeviceKM,
							   DISPLAY_INFO *psDisplayInfo);
IMG_IMPORT
PVRSRV_ERROR PVRSRVCreateDCSwapChainKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
									   IMG_HANDLE				hDeviceKM,
									   IMG_UINT32				ui32Flags,
									   DISPLAY_SURF_ATTRIBUTES	*psDstSurfAttrib,
									   DISPLAY_SURF_ATTRIBUTES	*psSrcSurfAttrib,
									   IMG_UINT32				ui32BufferCount,
									   IMG_UINT32				ui32OEMFlags,
									   IMG_HANDLE				*phSwapChain,
									   IMG_UINT32				*pui32SwapChainID);
IMG_IMPORT
PVRSRV_ERROR PVRSRVDestroyDCSwapChainKM(IMG_HANDLE	hSwapChain);
IMG_IMPORT
PVRSRV_ERROR PVRSRVSetDCDstRectKM(IMG_HANDLE	hDeviceKM,
								  IMG_HANDLE	hSwapChain,
								  IMG_RECT	*psRect);
IMG_IMPORT
PVRSRV_ERROR PVRSRVSetDCSrcRectKM(IMG_HANDLE	hDeviceKM,
								  IMG_HANDLE	hSwapChain,
								  IMG_RECT	*psRect);
IMG_IMPORT
PVRSRV_ERROR PVRSRVSetDCDstColourKeyKM(IMG_HANDLE	hDeviceKM,
									   IMG_HANDLE	hSwapChain,
									   IMG_UINT32	ui32CKColour);
IMG_IMPORT
PVRSRV_ERROR PVRSRVSetDCSrcColourKeyKM(IMG_HANDLE	hDeviceKM,
									IMG_HANDLE		hSwapChain,
									IMG_UINT32		ui32CKColour);
IMG_IMPORT
PVRSRV_ERROR PVRSRVGetDCBuffersKM(IMG_HANDLE	hDeviceKM,
								  IMG_HANDLE	hSwapChain,
								  IMG_UINT32	*pui32BufferCount,
								  IMG_HANDLE	*phBuffer);
IMG_IMPORT
PVRSRV_ERROR PVRSRVSwapToDCBufferKM(IMG_HANDLE	hDeviceKM,
									IMG_HANDLE	hBuffer,
									IMG_UINT32	ui32SwapInterval,
									IMG_HANDLE	hPrivateTag,
									IMG_UINT32	ui32ClipRectCount,
									IMG_RECT	*psClipRect);
IMG_IMPORT
PVRSRV_ERROR PVRSRVSwapToDCSystemKM(IMG_HANDLE	hDeviceKM,
									IMG_HANDLE	hSwapChain);

IMG_IMPORT
PVRSRV_ERROR PVRSRVOpenBCDeviceKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
								  IMG_UINT32				ui32DeviceID,
								  IMG_HANDLE				hDevCookie,
								  IMG_HANDLE				*phDeviceKM);
IMG_IMPORT
PVRSRV_ERROR PVRSRVCloseBCDeviceKM(IMG_HANDLE hDeviceKM, IMG_BOOL bResManCallback);

IMG_IMPORT
PVRSRV_ERROR PVRSRVGetBCInfoKM(IMG_HANDLE	hDeviceKM,
							   BUFFER_INFO	*psBufferInfo);
IMG_IMPORT
PVRSRV_ERROR PVRSRVGetBCBufferKM(IMG_HANDLE	hDeviceKM,
								 IMG_UINT32	ui32BufferIndex,
								 IMG_HANDLE	*phBuffer);


IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVMapDeviceClassMemoryKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
													   IMG_HANDLE				hDevMemContext,
													   IMG_HANDLE				hDeviceClassBuffer,
													   PVRSRV_KERNEL_MEM_INFO	**ppsMemInfo,
													   IMG_HANDLE				*phOSMapInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVUnmapDeviceClassMemoryKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetFreeDeviceMemKM(IMG_UINT32 ui32Flags,
												   IMG_SIZE_T *pui32Total,
												   IMG_SIZE_T *pui32Free,
												   IMG_SIZE_T *pui32LargestBlock);
IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVAllocSyncInfoKM(IMG_HANDLE					hDevCookie,
												IMG_HANDLE					hDevMemContext,
												PVRSRV_KERNEL_SYNC_INFO	**ppsKernelSyncInfo);
IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVFreeSyncInfoKM(PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetMiscInfoKM(PVRSRV_MISC_INFO *psMiscInfo);

IMG_IMPORT PVRSRV_ERROR
PVRSRVAllocSharedSysMemoryKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
							 IMG_UINT32 				ui32Flags,
							 IMG_SIZE_T 				ui32Size,
							 PVRSRV_KERNEL_MEM_INFO		**ppsKernelMemInfo);

IMG_IMPORT PVRSRV_ERROR
PVRSRVFreeSharedSysMemoryKM(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);

IMG_IMPORT PVRSRV_ERROR
PVRSRVDissociateMemFromResmanKM(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);

#if defined (__cplusplus)
}
#endif

#endif 

