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

#ifdef DEBUG_RELEASE_BUILD
#pragma optimize( "", off )
#define DEBUG		1
#endif

#ifndef __OSFUNC_H__
#define __OSFUNC_H__

#if defined (__cplusplus)
extern "C" {
#endif

#if defined(__linux__) && defined(__KERNEL__)
#include <linux/hardirq.h>
#include <linux/string.h>
#include <asm/system.h>
#if defined(__arm__)
#include <asm/memory.h>
#endif
#endif


	
	#define PVRSRV_PAGEABLE_SELECT		PVRSRV_OS_PAGEABLE_HEAP

#define KERNEL_ID			0xffffffffL
#define POWER_MANAGER_ID	0xfffffffeL
#define ISR_ID				0xfffffffdL
#define TIMER_ID			0xfffffffcL


#define HOST_PAGESIZE			OSGetPageSize
#define HOST_PAGEMASK			(HOST_PAGESIZE()-1)
#define HOST_PAGEALIGN(addr)	(((addr) + HOST_PAGEMASK) & ~HOST_PAGEMASK)

#define PVRSRV_OS_HEAP_MASK			0xf 
#define PVRSRV_OS_PAGEABLE_HEAP		0x1 
#define PVRSRV_OS_NON_PAGEABLE_HEAP	0x2 


IMG_UINT32 OSClockus(IMG_VOID);
IMG_SIZE_T OSGetPageSize(IMG_VOID);
PVRSRV_ERROR OSInstallDeviceLISR(IMG_VOID *pvSysData,
								 IMG_UINT32 ui32Irq,
								 IMG_CHAR *pszISRName,
								 IMG_VOID *pvDeviceNode);
PVRSRV_ERROR OSUninstallDeviceLISR(IMG_VOID *pvSysData);
PVRSRV_ERROR OSInstallSystemLISR(IMG_VOID *pvSysData, IMG_UINT32 ui32Irq);
PVRSRV_ERROR OSUninstallSystemLISR(IMG_VOID *pvSysData);
PVRSRV_ERROR OSInstallMISR(IMG_VOID *pvSysData);
PVRSRV_ERROR OSUninstallMISR(IMG_VOID *pvSysData);
IMG_CPU_PHYADDR OSMapLinToCPUPhys(IMG_HANDLE, IMG_VOID* pvLinAddr);
IMG_VOID OSMemCopy(IMG_VOID *pvDst, IMG_VOID *pvSrc, IMG_SIZE_T ui32Size);
IMG_VOID *OSMapPhysToLin(IMG_CPU_PHYADDR BasePAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_HANDLE *phOSMemHandle);
IMG_BOOL OSUnMapPhysToLin(IMG_VOID *pvLinAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_HANDLE hOSMemHandle);

PVRSRV_ERROR OSReservePhys(IMG_CPU_PHYADDR BasePAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_VOID **ppvCpuVAddr, IMG_HANDLE *phOSMemHandle);
PVRSRV_ERROR OSUnReservePhys(IMG_VOID *pvCpuVAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_HANDLE hOSMemHandle);

#if defined(__linux__) && defined(__KERNEL__)

IMG_VOID OSFlushCPUCacheKM(IMG_VOID);

IMG_VOID OSCleanCPUCacheKM(IMG_VOID);

IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length);
IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length);
IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32Length);

#else 

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSFlushCPUCacheKM)
#endif
static INLINE IMG_VOID OSFlushCPUCacheKM(IMG_VOID) {}

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSCleanCPUCacheKM)
#endif
static INLINE IMG_VOID OSCleanCPUCacheKM(IMG_VOID) {}

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSFlushCPUCacheRangeKM)
#endif
static INLINE IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
											  IMG_VOID *pvRangeAddrStart,
											  IMG_UINT32 ui32Length)
{
	PVR_UNREFERENCED_PARAMETER(hOSMemHandle);
	PVR_UNREFERENCED_PARAMETER(pvRangeAddrStart);
	PVR_UNREFERENCED_PARAMETER(ui32Length);
	return IMG_FALSE;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSCleanCPUCacheRangeKM)
#endif
static INLINE IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
											  IMG_VOID *pvRangeAddrStart,
											  IMG_UINT32 ui32Length)
{
	PVR_UNREFERENCED_PARAMETER(hOSMemHandle);
	PVR_UNREFERENCED_PARAMETER(pvRangeAddrStart);
	PVR_UNREFERENCED_PARAMETER(ui32Length);
	return IMG_FALSE;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSInvalidateCPUCacheRangeKM)
#endif
static INLINE IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
												   IMG_VOID *pvRangeAddrStart,
												   IMG_UINT32 ui32Length)
{
	PVR_UNREFERENCED_PARAMETER(hOSMemHandle);
	PVR_UNREFERENCED_PARAMETER(pvRangeAddrStart);
	PVR_UNREFERENCED_PARAMETER(ui32Length);
	return IMG_FALSE;
}

#endif 

#if defined(__linux__)
PVRSRV_ERROR OSRegisterDiscontigMem(IMG_SYS_PHYADDR *pBasePAddr,
									IMG_VOID *pvCpuVAddr, 
									IMG_SIZE_T ui32Bytes,
									IMG_UINT32 ui32Flags, 
									IMG_HANDLE *phOSMemHandle);
PVRSRV_ERROR OSUnRegisterDiscontigMem(IMG_VOID *pvCpuVAddr,
									IMG_SIZE_T ui32Bytes,
									IMG_UINT32 ui32Flags,
									IMG_HANDLE hOSMemHandle);
#else	
#ifdef INLINE_IS_PRAGMA
#pragma inline(OSRegisterDiscontigMem)
#endif
static INLINE PVRSRV_ERROR OSRegisterDiscontigMem(IMG_SYS_PHYADDR *pBasePAddr,
													IMG_VOID *pvCpuVAddr,
													IMG_SIZE_T ui32Bytes,
													IMG_UINT32 ui32Flags,
													IMG_HANDLE *phOSMemHandle)
{
	PVR_UNREFERENCED_PARAMETER(pBasePAddr);
	PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Bytes);
	PVR_UNREFERENCED_PARAMETER(ui32Flags);
	PVR_UNREFERENCED_PARAMETER(phOSMemHandle);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSUnRegisterDiscontigMem)
#endif
static INLINE PVRSRV_ERROR OSUnRegisterDiscontigMem(IMG_VOID *pvCpuVAddr,
													IMG_SIZE_T ui32Bytes,
													IMG_UINT32 ui32Flags,
													IMG_HANDLE hOSMemHandle)
{
	PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Bytes);
	PVR_UNREFERENCED_PARAMETER(ui32Flags);
	PVR_UNREFERENCED_PARAMETER(hOSMemHandle);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}
#endif	


#if defined(__linux__)
#ifdef INLINE_IS_PRAGMA
#pragma inline(OSReserveDiscontigPhys)
#endif
static INLINE PVRSRV_ERROR OSReserveDiscontigPhys(IMG_SYS_PHYADDR *pBasePAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_VOID **ppvCpuVAddr, IMG_HANDLE *phOSMemHandle)
{
#if defined(__linux__)	
	*ppvCpuVAddr = IMG_NULL;
	return OSRegisterDiscontigMem(pBasePAddr, *ppvCpuVAddr, ui32Bytes, ui32Flags, phOSMemHandle);	
#else
	extern IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr(IMG_SYS_PHYADDR SysPAddr);

	
	return OSReservePhys(SysSysPAddrToCpuPAddr(pBasePAddr[0]), ui32Bytes, ui32Flags, ppvCpuVAddr, phOSMemHandle);
#endif	
}

static INLINE PVRSRV_ERROR OSUnReserveDiscontigPhys(IMG_VOID *pvCpuVAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_HANDLE hOSMemHandle)
{
#if defined(__linux__)
	OSUnRegisterDiscontigMem(pvCpuVAddr, ui32Bytes, ui32Flags, hOSMemHandle);
#endif
	
	return PVRSRV_OK;
}
#else	


#ifdef INLINE_IS_PRAGMA
#pragma inline(OSReserveDiscontigPhys)
#endif
static INLINE PVRSRV_ERROR OSReserveDiscontigPhys(IMG_SYS_PHYADDR *pBasePAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_VOID **ppvCpuVAddr, IMG_HANDLE *phOSMemHandle)
{
	PVR_UNREFERENCED_PARAMETER(pBasePAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Bytes);
	PVR_UNREFERENCED_PARAMETER(ui32Flags);
	PVR_UNREFERENCED_PARAMETER(ppvCpuVAddr);
	PVR_UNREFERENCED_PARAMETER(phOSMemHandle);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSUnReserveDiscontigPhys)
#endif
static INLINE PVRSRV_ERROR OSUnReserveDiscontigPhys(IMG_VOID *pvCpuVAddr, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags, IMG_HANDLE hOSMemHandle)
{
	PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Bytes);
	PVR_UNREFERENCED_PARAMETER(ui32Flags);
	PVR_UNREFERENCED_PARAMETER(hOSMemHandle);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}
#endif	

PVRSRV_ERROR OSRegisterMem(IMG_CPU_PHYADDR BasePAddr,
							IMG_VOID *pvCpuVAddr,
							IMG_SIZE_T ui32Bytes,
							IMG_UINT32 ui32Flags,
							IMG_HANDLE *phOSMemHandle);
PVRSRV_ERROR OSUnRegisterMem(IMG_VOID *pvCpuVAddr,
							IMG_SIZE_T ui32Bytes,
							IMG_UINT32 ui32Flags,
							IMG_HANDLE hOSMemHandle);



#if defined(__linux__)
PVRSRV_ERROR OSGetSubMemHandle(IMG_HANDLE hOSMemHandle,
							   IMG_UINTPTR_T ui32ByteOffset,
							   IMG_SIZE_T ui32Bytes,
							   IMG_UINT32 ui32Flags,
							   IMG_HANDLE *phOSMemHandleRet);
PVRSRV_ERROR OSReleaseSubMemHandle(IMG_HANDLE hOSMemHandle, IMG_UINT32 ui32Flags);
#else
#ifdef INLINE_IS_PRAGMA
#pragma inline(OSGetSubMemHandle)
#endif
static INLINE PVRSRV_ERROR OSGetSubMemHandle(IMG_HANDLE hOSMemHandle,
											 IMG_UINTPTR_T ui32ByteOffset,
											 IMG_SIZE_T ui32Bytes,
											 IMG_UINT32 ui32Flags,
											 IMG_HANDLE *phOSMemHandleRet)
{
	PVR_UNREFERENCED_PARAMETER(ui32ByteOffset);
	PVR_UNREFERENCED_PARAMETER(ui32Bytes);
	PVR_UNREFERENCED_PARAMETER(ui32Flags);

	*phOSMemHandleRet = hOSMemHandle;
	return PVRSRV_OK;
}

static INLINE PVRSRV_ERROR OSReleaseSubMemHandle(IMG_HANDLE hOSMemHandle, IMG_UINT32 ui32Flags)
{
	PVR_UNREFERENCED_PARAMETER(hOSMemHandle);
	PVR_UNREFERENCED_PARAMETER(ui32Flags);
	return PVRSRV_OK;
}
#endif

IMG_UINT32 OSGetCurrentProcessIDKM(IMG_VOID);
IMG_UINTPTR_T OSGetCurrentThreadID( IMG_VOID );
IMG_VOID OSMemSet(IMG_VOID *pvDest, IMG_UINT8 ui8Value, IMG_SIZE_T ui32Size);

PVRSRV_ERROR OSAllocPages_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T ui32Size, IMG_UINT32 ui32PageSize, IMG_PVOID *ppvLinAddr, IMG_HANDLE *phPageAlloc);
PVRSRV_ERROR OSFreePages(IMG_UINT32 ui32Flags, IMG_SIZE_T ui32Size, IMG_PVOID pvLinAddr, IMG_HANDLE hPageAlloc);


#ifdef PVRSRV_LOG_MEMORY_ALLOCS
	#define OSAllocMem(flags, size, linAddr, blockAlloc, logStr) \
		(PVR_TRACE(("OSAllocMem(" #flags ", " #size ", " #linAddr ", " #blockAlloc "): " logStr " (size = 0x%lx)", size)), \
			OSAllocMem_Debug_Wrapper(flags, size, linAddr, blockAlloc, __FILE__, __LINE__))

	#define OSAllocPages(flags, size, pageSize, linAddr, pageAlloc) \
		(PVR_TRACE(("OSAllocPages(" #flags ", " #size ", " #pageSize ", " #linAddr ", " #pageAlloc "): (size = 0x%lx)", size)), \
			OSAllocPages_Impl(flags, size, pageSize, linAddr, pageAlloc))
		
	#define OSFreeMem(flags, size, linAddr, blockAlloc) \
		(PVR_TRACE(("OSFreeMem(" #flags ", " #size ", " #linAddr ", " #blockAlloc "): (pointer = 0x%X)", linAddr)), \
			OSFreeMem_Debug_Wrapper(flags, size, linAddr, blockAlloc, __FILE__, __LINE__))
#else
	#define OSAllocMem(flags, size, linAddr, blockAlloc, logString) \
		OSAllocMem_Debug_Wrapper(flags, size, linAddr, blockAlloc, __FILE__, __LINE__)
	
	#define OSAllocPages OSAllocPages_Impl
	
	#define OSFreeMem(flags, size, linAddr, blockAlloc) \
			OSFreeMem_Debug_Wrapper(flags, size, linAddr, blockAlloc, __FILE__, __LINE__)
#endif
 
#ifdef PVRSRV_DEBUG_OS_MEMORY

	PVRSRV_ERROR OSAllocMem_Debug_Wrapper(IMG_UINT32 ui32Flags,
										IMG_UINT32 ui32Size,
										IMG_PVOID *ppvCpuVAddr,
										IMG_HANDLE *phBlockAlloc,
										IMG_CHAR *pszFilename,
										IMG_UINT32 ui32Line);
	
	PVRSRV_ERROR OSFreeMem_Debug_Wrapper(IMG_UINT32 ui32Flags,
									 IMG_UINT32 ui32Size,
									 IMG_PVOID pvCpuVAddr,
									 IMG_HANDLE hBlockAlloc,
									 IMG_CHAR *pszFilename,
									 IMG_UINT32 ui32Line);


	typedef struct
	{	
		IMG_UINT8 sGuardRegionBefore[8];
		IMG_CHAR sFileName[128];
		IMG_UINT32 uLineNo;
		IMG_SIZE_T uSize;
		IMG_SIZE_T uSizeParityCheck;
		enum valid_tag
		{	isFree = 0x277260FF,
			isAllocated = 0x260511AA
		} eValid;
	} OSMEM_DEBUG_INFO;
	
	#define TEST_BUFFER_PADDING_STATUS (sizeof(OSMEM_DEBUG_INFO)) 
	#define TEST_BUFFER_PADDING_AFTER  (8) 
	#define TEST_BUFFER_PADDING (TEST_BUFFER_PADDING_STATUS + TEST_BUFFER_PADDING_AFTER)
#else
	#define OSAllocMem_Debug_Wrapper OSAllocMem_Debug_Linux_Memory_Allocations
	#define OSFreeMem_Debug_Wrapper OSFreeMem_Debug_Linux_Memory_Allocations
#endif
 
#if defined(__linux__) && defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	PVRSRV_ERROR OSAllocMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T ui32Size, IMG_PVOID *ppvLinAddr, IMG_HANDLE *phBlockAlloc, IMG_CHAR *pszFilename, IMG_UINT32 ui32Line);
	PVRSRV_ERROR OSFreeMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T ui32Size, IMG_PVOID pvLinAddr, IMG_HANDLE hBlockAlloc, IMG_CHAR *pszFilename, IMG_UINT32 ui32Line);
	
	#define OSAllocMem_Debug_Linux_Memory_Allocations OSAllocMem_Impl
	#define OSFreeMem_Debug_Linux_Memory_Allocations OSFreeMem_Impl
#else
	PVRSRV_ERROR OSAllocMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T ui32Size, IMG_PVOID *ppvLinAddr, IMG_HANDLE *phBlockAlloc);
	PVRSRV_ERROR OSFreeMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T ui32Size, IMG_PVOID pvLinAddr, IMG_HANDLE hBlockAlloc);
	
	#define OSAllocMem_Debug_Linux_Memory_Allocations(flags, size, addr, blockAlloc, file, line) \
		OSAllocMem_Impl(flags, size, addr, blockAlloc)
	#define OSFreeMem_Debug_Linux_Memory_Allocations(flags, size, addr, blockAlloc, file, line) \
		OSFreeMem_Impl(flags, size, addr, blockAlloc)
#endif


#if defined(__linux__)
IMG_CPU_PHYADDR OSMemHandleToCpuPAddr(IMG_VOID *hOSMemHandle, IMG_SIZE_T ui32ByteOffset);
#else
#ifdef INLINE_IS_PRAGMA
#pragma inline(OSMemHandleToCpuPAddr)
#endif
static INLINE IMG_CPU_PHYADDR OSMemHandleToCpuPAddr(IMG_HANDLE hOSMemHandle, IMG_SIZE_T ui32ByteOffset)
{
	IMG_CPU_PHYADDR sCpuPAddr;
	PVR_UNREFERENCED_PARAMETER(hOSMemHandle);
	PVR_UNREFERENCED_PARAMETER(ui32ByteOffset);
	sCpuPAddr.uiAddr = 0;
	return sCpuPAddr;
}
#endif
PVRSRV_ERROR OSInitEnvData(IMG_PVOID *ppvEnvSpecificData);
PVRSRV_ERROR OSDeInitEnvData(IMG_PVOID pvEnvSpecificData);
IMG_CHAR* OSStringCopy(IMG_CHAR *pszDest, const IMG_CHAR *pszSrc);
IMG_INT32 OSSNPrintf(IMG_CHAR *pStr, IMG_SIZE_T ui32Size, const IMG_CHAR *pszFormat, ...) IMG_FORMAT_PRINTF(3, 4);
#define OSStringLength(pszString) strlen(pszString)

PVRSRV_ERROR OSEventObjectCreate(const IMG_CHAR *pszName,
								 PVRSRV_EVENTOBJECT *psEventObject);
PVRSRV_ERROR OSEventObjectDestroy(PVRSRV_EVENTOBJECT *psEventObject);
PVRSRV_ERROR OSEventObjectSignal(IMG_HANDLE hOSEventKM);
PVRSRV_ERROR OSEventObjectWait(IMG_HANDLE hOSEventKM);
PVRSRV_ERROR OSEventObjectOpen(PVRSRV_EVENTOBJECT *psEventObject,
											IMG_HANDLE *phOSEvent);
PVRSRV_ERROR OSEventObjectClose(PVRSRV_EVENTOBJECT *psEventObject,
											IMG_HANDLE hOSEventKM);


PVRSRV_ERROR OSBaseAllocContigMemory(IMG_SIZE_T ui32Size, IMG_CPU_VIRTADDR *pLinAddr, IMG_CPU_PHYADDR *pPhysAddr);
PVRSRV_ERROR OSBaseFreeContigMemory(IMG_SIZE_T ui32Size, IMG_CPU_VIRTADDR LinAddr, IMG_CPU_PHYADDR PhysAddr);

IMG_PVOID MapUserFromKernel(IMG_PVOID pvLinAddrKM,IMG_SIZE_T ui32Size,IMG_HANDLE *phMemBlock);
IMG_PVOID OSMapHWRegsIntoUserSpace(IMG_HANDLE hDevCookie, IMG_SYS_PHYADDR sRegAddr, IMG_UINT32 ulSize, IMG_PVOID *ppvProcess);
IMG_VOID  OSUnmapHWRegsFromUserSpace(IMG_HANDLE hDevCookie, IMG_PVOID pvUserAddr, IMG_PVOID pvProcess);

IMG_VOID  UnmapUserFromKernel(IMG_PVOID pvLinAddrUM, IMG_SIZE_T ui32Size, IMG_HANDLE hMemBlock);

PVRSRV_ERROR OSMapPhysToUserSpace(IMG_HANDLE hDevCookie,
								  IMG_SYS_PHYADDR sCPUPhysAddr,
								  IMG_SIZE_T uiSizeInBytes,
								  IMG_UINT32 ui32CacheFlags,
								  IMG_PVOID *ppvUserAddr,
								  IMG_SIZE_T *puiActualSize,
								  IMG_HANDLE hMappingHandle);

PVRSRV_ERROR OSUnmapPhysToUserSpace(IMG_HANDLE hDevCookie,
									IMG_PVOID pvUserAddr,
									IMG_PVOID pvProcess);

PVRSRV_ERROR OSLockResource(PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID);
PVRSRV_ERROR OSUnlockResource(PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID);
IMG_BOOL OSIsResourceLocked(PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID);
PVRSRV_ERROR OSCreateResource(PVRSRV_RESOURCE *psResource);
PVRSRV_ERROR OSDestroyResource(PVRSRV_RESOURCE *psResource);
IMG_VOID OSBreakResourceLock(PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID);


 
IMG_VOID OSWaitus(IMG_UINT32 ui32Timeus);

 
IMG_VOID OSSleepms(IMG_UINT32 ui32Timems);


IMG_VOID OSReleaseThreadQuanta(IMG_VOID);
IMG_UINT32 OSPCIReadDword(IMG_UINT32 ui32Bus, IMG_UINT32 ui32Dev, IMG_UINT32 ui32Func, IMG_UINT32 ui32Reg);
IMG_VOID OSPCIWriteDword(IMG_UINT32 ui32Bus, IMG_UINT32 ui32Dev, IMG_UINT32 ui32Func, IMG_UINT32 ui32Reg, IMG_UINT32 ui32Value);

#ifndef OSReadHWReg
IMG_UINT32 OSReadHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset);
#endif
#ifndef OSWriteHWReg
IMG_VOID OSWriteHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset, IMG_UINT32 ui32Value);
#endif

typedef IMG_VOID (*PFN_TIMER_FUNC)(IMG_VOID*);
IMG_HANDLE OSAddTimer(PFN_TIMER_FUNC pfnTimerFunc, IMG_VOID *pvData, IMG_UINT32 ui32MsTimeout);
PVRSRV_ERROR OSRemoveTimer (IMG_HANDLE hTimer);
PVRSRV_ERROR OSEnableTimer (IMG_HANDLE hTimer);
PVRSRV_ERROR OSDisableTimer (IMG_HANDLE hTimer);

PVRSRV_ERROR OSGetSysMemSize(IMG_SIZE_T *pui32Bytes);

typedef enum _HOST_PCI_INIT_FLAGS_
{
	HOST_PCI_INIT_FLAG_BUS_MASTER	= 0x00000001,
	HOST_PCI_INIT_FLAG_MSI		= 0x00000002,
	HOST_PCI_INIT_FLAG_FORCE_I32 	= 0x7fffffff
} HOST_PCI_INIT_FLAGS;

struct _PVRSRV_PCI_DEV_OPAQUE_STRUCT_;
typedef struct _PVRSRV_PCI_DEV_OPAQUE_STRUCT_ *PVRSRV_PCI_DEV_HANDLE;

PVRSRV_PCI_DEV_HANDLE OSPCIAcquireDev(IMG_UINT16 ui16VendorID, IMG_UINT16 ui16DeviceID, HOST_PCI_INIT_FLAGS eFlags);
PVRSRV_PCI_DEV_HANDLE OSPCISetDev(IMG_VOID *pvPCICookie, HOST_PCI_INIT_FLAGS eFlags);
PVRSRV_ERROR OSPCIReleaseDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI);
PVRSRV_ERROR OSPCIIRQ(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 *pui32IRQ);
IMG_UINT32 OSPCIAddrRangeLen(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index);
IMG_UINT32 OSPCIAddrRangeStart(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index);
IMG_UINT32 OSPCIAddrRangeEnd(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index);
PVRSRV_ERROR OSPCIRequestAddrRange(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index);
PVRSRV_ERROR OSPCIReleaseAddrRange(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index);
PVRSRV_ERROR OSPCISuspendDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI);
PVRSRV_ERROR OSPCIResumeDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI);

PVRSRV_ERROR OSScheduleMISR(IMG_VOID *pvSysData);

IMG_VOID OSPanic(IMG_VOID);

IMG_BOOL OSProcHasPrivSrvInit(IMG_VOID);

typedef enum _img_verify_test
{
	PVR_VERIFY_WRITE = 0,
	PVR_VERIFY_READ
} IMG_VERIFY_TEST;

IMG_BOOL OSAccessOK(IMG_VERIFY_TEST eVerification, IMG_VOID *pvUserPtr, IMG_SIZE_T ui32Bytes);

PVRSRV_ERROR OSCopyToUser(IMG_PVOID pvProcess, IMG_VOID *pvDest, IMG_VOID *pvSrc, IMG_SIZE_T ui32Bytes);
PVRSRV_ERROR OSCopyFromUser(IMG_PVOID pvProcess, IMG_VOID *pvDest, IMG_VOID *pvSrc, IMG_SIZE_T ui32Bytes);

#if defined(__linux__)
PVRSRV_ERROR OSAcquirePhysPageAddr(IMG_VOID* pvCPUVAddr, 
									IMG_SIZE_T ui32Bytes, 
									IMG_SYS_PHYADDR *psSysPAddr,
									IMG_HANDLE *phOSWrapMem);
PVRSRV_ERROR OSReleasePhysPageAddr(IMG_HANDLE hOSWrapMem);
#else
#ifdef INLINE_IS_PRAGMA
#pragma inline(OSAcquirePhysPageAddr)
#endif
static INLINE PVRSRV_ERROR OSAcquirePhysPageAddr(IMG_VOID* pvCPUVAddr, 
												IMG_SIZE_T ui32Bytes, 
												IMG_SYS_PHYADDR *psSysPAddr,
												IMG_HANDLE *phOSWrapMem)
{
	PVR_UNREFERENCED_PARAMETER(pvCPUVAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Bytes);
	PVR_UNREFERENCED_PARAMETER(psSysPAddr);
	PVR_UNREFERENCED_PARAMETER(phOSWrapMem);
	return PVRSRV_OK;	
}
#ifdef INLINE_IS_PRAGMA
#pragma inline(OSReleasePhysPageAddr)
#endif
static INLINE PVRSRV_ERROR OSReleasePhysPageAddr(IMG_HANDLE hOSWrapMem)
{
	PVR_UNREFERENCED_PARAMETER(hOSWrapMem);
	return PVRSRV_OK;	
}
#endif
									
#if defined(__linux__) && defined(__KERNEL__)

#define	OS_SUPPORTS_IN_LISR

static inline IMG_BOOL OSInLISR(IMG_VOID unref__ *pvSysData)
{
	PVR_UNREFERENCED_PARAMETER(pvSysData);
	return (in_irq()) ? IMG_TRUE : IMG_FALSE;
}

static inline IMG_VOID OSWriteMemoryBarrier(IMG_VOID)
{
	wmb();
}

static inline IMG_VOID OSMemoryBarrier(IMG_VOID)
{
	mb();
}

#else 

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSWriteMemoryBarrier)
#endif
static INLINE IMG_VOID OSWriteMemoryBarrier(IMG_VOID) { }

#ifdef INLINE_IS_PRAGMA
#pragma inline(OSMemoryBarrier)
#endif
static INLINE IMG_VOID OSMemoryBarrier(IMG_VOID) { }

#endif 

#if defined (__cplusplus)
}
#endif

#endif 

