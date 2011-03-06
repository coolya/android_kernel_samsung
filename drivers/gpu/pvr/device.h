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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#if defined(__cplusplus)
extern "C" {
#endif
	
#include "ra.h"  		
#include "resman.h"		

typedef struct _BM_CONTEXT_ BM_CONTEXT;

typedef struct _MMU_HEAP_ MMU_HEAP;
typedef struct _MMU_CONTEXT_ MMU_CONTEXT;

#define PVRSRV_BACKINGSTORE_SYSMEM_CONTIG		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+0))
#define PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG	(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+1))
#define PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+2))
#define PVRSRV_BACKINGSTORE_LOCALMEM_NONCONTIG	(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+3))

typedef IMG_UINT32 DEVICE_MEMORY_HEAP_TYPE;
#define DEVICE_MEMORY_HEAP_PERCONTEXT		0
#define DEVICE_MEMORY_HEAP_KERNEL			1
#define DEVICE_MEMORY_HEAP_SHARED			2
#define DEVICE_MEMORY_HEAP_SHARED_EXPORTED	3

#define PVRSRV_DEVICE_NODE_FLAGS_PORT80DISPLAY	1
#define PVRSRV_DEVICE_NODE_FLAGS_MMU_OPT_INV	2	

typedef struct _DEVICE_MEMORY_HEAP_INFO_
{
	
	IMG_UINT32				ui32HeapID;

	
	IMG_CHAR				*pszName;

	
	IMG_CHAR				*pszBSName;
	
	
	IMG_DEV_VIRTADDR		sDevVAddrBase;

	
	IMG_UINT32				ui32HeapSize;

	
	IMG_UINT32				ui32Attribs;

	
	DEVICE_MEMORY_HEAP_TYPE	DevMemHeapType;
	
	
	IMG_HANDLE				hDevMemHeap;
	
	
	RA_ARENA				*psLocalDevMemArena;

	
	IMG_UINT32				ui32DataPageSize;

	IMG_UINT32				ui32XTileStride;

} DEVICE_MEMORY_HEAP_INFO;

typedef struct _DEVICE_MEMORY_INFO_
{
	
	IMG_UINT32				ui32AddressSpaceSizeLog2;

	


	IMG_UINT32				ui32Flags;

	
	IMG_UINT32				ui32HeapCount;
	
	
	IMG_UINT32				ui32SyncHeapID;
	
	
	IMG_UINT32				ui32MappingHeapID;

	
	DEVICE_MEMORY_HEAP_INFO	*psDeviceMemoryHeap;

	
    BM_CONTEXT				*pBMKernelContext;

	
    BM_CONTEXT				*pBMContext;

} DEVICE_MEMORY_INFO;


typedef struct DEV_ARENA_DESCRIPTOR_TAG
{
	IMG_UINT32				ui32HeapID;		

	IMG_CHAR				*pszName;		

	IMG_DEV_VIRTADDR		BaseDevVAddr;	

	IMG_UINT32 				ui32Size;		

	DEVICE_MEMORY_HEAP_TYPE	DevMemHeapType;

	
	IMG_UINT32				ui32DataPageSize;

	DEVICE_MEMORY_HEAP_INFO	*psDeviceMemoryHeapInfo;

} DEV_ARENA_DESCRIPTOR;


typedef struct _PDUMP_MMU_ATTRIB_
{
	PVRSRV_DEVICE_IDENTIFIER	sDevId;
	
	IMG_CHAR	*pszPDRegRegion;
	
	
	IMG_UINT32 ui32DataPageMask;

	
	IMG_UINT32 ui32PTEValid;
	IMG_UINT32 ui32PTSize;
	IMG_UINT32 ui32PTEAlignShift;

	
	IMG_UINT32 ui32PDEMask;
	IMG_UINT32 ui32PDEAlignShift;

} PDUMP_MMU_ATTRIB;

typedef struct _SYS_DATA_TAG_ *PSYS_DATA;

typedef struct _PVRSRV_DEVICE_NODE_
{
	PVRSRV_DEVICE_IDENTIFIER	sDevId;
	IMG_UINT32					ui32RefCount;

	

	
	PVRSRV_ERROR			(*pfnInitDevice) (IMG_VOID*);
	
	PVRSRV_ERROR			(*pfnDeInitDevice) (IMG_VOID*);

	
	PVRSRV_ERROR			(*pfnInitDeviceCompatCheck) (struct _PVRSRV_DEVICE_NODE_*);

	
	PVRSRV_ERROR			(*pfnMMUInitialise)(struct _PVRSRV_DEVICE_NODE_*, MMU_CONTEXT**, IMG_DEV_PHYADDR*);
	IMG_VOID				(*pfnMMUFinalise)(MMU_CONTEXT*);
	IMG_VOID				(*pfnMMUInsertHeap)(MMU_CONTEXT*, MMU_HEAP*);
	MMU_HEAP*				(*pfnMMUCreate)(MMU_CONTEXT*,DEV_ARENA_DESCRIPTOR*,RA_ARENA**,PDUMP_MMU_ATTRIB **ppsMMUAttrib);
	IMG_VOID				(*pfnMMUDelete)(MMU_HEAP*);
	IMG_BOOL				(*pfnMMUAlloc)(MMU_HEAP*pMMU,
										   IMG_SIZE_T uSize,
										   IMG_SIZE_T *pActualSize,
										   IMG_UINT32 uFlags,
										   IMG_UINT32 uDevVAddrAlignment,
										   IMG_DEV_VIRTADDR *pDevVAddr);
	IMG_VOID				(*pfnMMUFree)(MMU_HEAP*,IMG_DEV_VIRTADDR,IMG_UINT32);
	IMG_VOID 				(*pfnMMUEnable)(MMU_HEAP*);
	IMG_VOID				(*pfnMMUDisable)(MMU_HEAP*);
	IMG_VOID				(*pfnMMUMapPages)(MMU_HEAP *pMMU,
											  IMG_DEV_VIRTADDR devVAddr,
											  IMG_SYS_PHYADDR SysPAddr,
											  IMG_SIZE_T uSize,
											  IMG_UINT32 ui32MemFlags,
											  IMG_HANDLE hUniqueTag);
	IMG_VOID				(*pfnMMUMapShadow)(MMU_HEAP            *pMMU,
											   IMG_DEV_VIRTADDR    MapBaseDevVAddr,
											   IMG_SIZE_T          uSize, 
											   IMG_CPU_VIRTADDR    CpuVAddr,
											   IMG_HANDLE          hOSMemHandle,
											   IMG_DEV_VIRTADDR    *pDevVAddr,
											   IMG_UINT32 ui32MemFlags,
											   IMG_HANDLE hUniqueTag);
	IMG_VOID				(*pfnMMUUnmapPages)(MMU_HEAP *pMMU,
												IMG_DEV_VIRTADDR dev_vaddr,
												IMG_UINT32 ui32PageCount,
												IMG_HANDLE hUniqueTag);

	IMG_VOID				(*pfnMMUMapScatter)(MMU_HEAP *pMMU,
												IMG_DEV_VIRTADDR DevVAddr,
												IMG_SYS_PHYADDR *psSysAddr,
												IMG_SIZE_T uSize,
												IMG_UINT32 ui32MemFlags,
												IMG_HANDLE hUniqueTag);
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	IMG_BOOL				(*pfnMMUIsHeapShared)(MMU_HEAP *);
#endif
	IMG_DEV_PHYADDR			(*pfnMMUGetPhysPageAddr)(MMU_HEAP *pMMUHeap, IMG_DEV_VIRTADDR sDevVPageAddr);
	IMG_DEV_PHYADDR			(*pfnMMUGetPDDevPAddr)(MMU_CONTEXT *pMMUContext);

	
	PVRSRV_ERROR			(*pfnAllocMemTilingRange)(struct _PVRSRV_DEVICE_NODE_ *psDeviceNode,
														PVRSRV_KERNEL_MEM_INFO *psMemInfo,
														IMG_UINT32 ui32TilingStride,
														IMG_UINT32 *pui32RangeIndex);
	PVRSRV_ERROR			(*pfnFreeMemTilingRange)(struct _PVRSRV_DEVICE_NODE_ *psDeviceNode,
														IMG_UINT32 ui32RangeIndex);

	
	IMG_BOOL				(*pfnDeviceISR)(IMG_VOID*);
	
	IMG_VOID				*pvISRData;
	
	IMG_UINT32 				ui32SOCInterruptBit;
	
	IMG_VOID				(*pfnDeviceMISR)(IMG_VOID*);

	
	IMG_VOID				(*pfnDeviceCommandComplete)(struct _PVRSRV_DEVICE_NODE_ *psDeviceNode);
	
	IMG_BOOL				bReProcessDeviceCommandComplete;
	
	
	DEVICE_MEMORY_INFO		sDevMemoryInfo;

	
	IMG_VOID				*pvDevice;
	IMG_UINT32				ui32pvDeviceSize; 
		
	
	PRESMAN_CONTEXT			hResManContext;
	
	
	PSYS_DATA				psSysData;
	
	
	RA_ARENA				*psLocalDevMemArena;
	
	IMG_UINT32				ui32Flags;
	
	struct _PVRSRV_DEVICE_NODE_	*psNext;
	struct _PVRSRV_DEVICE_NODE_	**ppsThis;
	
#if defined(PDUMP)
	
	PVRSRV_ERROR			(*pfnPDumpInitDevice)(struct _PVRSRV_DEVICE_NODE_ *psDeviceNode);
	
	IMG_UINT32				(*pfnMMUGetContextID)(IMG_HANDLE hDevMemContext);
#endif
} PVRSRV_DEVICE_NODE;

PVRSRV_ERROR IMG_CALLCONV PVRSRVRegisterDevice(PSYS_DATA psSysData,
											  PVRSRV_ERROR (*pfnRegisterDevice)(PVRSRV_DEVICE_NODE*),
											  IMG_UINT32 ui32SOCInterruptBit,
			 								  IMG_UINT32 *pui32DeviceIndex );

PVRSRV_ERROR IMG_CALLCONV PVRSRVInitialiseDevice(IMG_UINT32 ui32DevIndex);
PVRSRV_ERROR IMG_CALLCONV PVRSRVFinaliseSystem(IMG_BOOL bInitSuccesful);

PVRSRV_ERROR IMG_CALLCONV PVRSRVDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode);

PVRSRV_ERROR IMG_CALLCONV PVRSRVDeinitialiseDevice(IMG_UINT32 ui32DevIndex);

#if !defined(USE_CODE)

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PollForValueKM(volatile IMG_UINT32*	pui32LinMemAddr,
													IMG_UINT32				ui32Value,
													IMG_UINT32				ui32Mask,
													IMG_UINT32				ui32Timeoutus,
													IMG_UINT32				ui32PollPeriodus,
													IMG_BOOL				bAllowPreemption);

#endif 


#if defined (USING_ISR_INTERRUPTS)
PVRSRV_ERROR IMG_CALLCONV PollForInterruptKM(IMG_UINT32 ui32Value,
								IMG_UINT32 ui32Mask,
								IMG_UINT32 ui32Waitus,
								IMG_UINT32 ui32Tries);
#endif 

PVRSRV_ERROR IMG_CALLCONV PVRSRVInit(PSYS_DATA psSysData);
IMG_VOID IMG_CALLCONV PVRSRVDeInit(PSYS_DATA psSysData);
IMG_BOOL IMG_CALLCONV PVRSRVDeviceLISR(PVRSRV_DEVICE_NODE *psDeviceNode);
IMG_BOOL IMG_CALLCONV PVRSRVSystemLISR(IMG_VOID *pvSysData);
IMG_VOID IMG_CALLCONV PVRSRVMISR(IMG_VOID *pvSysData);

#if defined(__cplusplus)
}
#endif
	
#endif 

