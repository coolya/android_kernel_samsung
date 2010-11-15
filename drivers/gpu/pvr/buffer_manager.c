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

#include "sysconfig.h"
#include "hash.h"
#include "ra.h"
#include "pdump_km.h"
#include "lists.h"

static IMG_BOOL
ZeroBuf(BM_BUF *pBuf, BM_MAPPING *pMapping, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags);
static IMG_VOID
BM_FreeMemory (IMG_VOID *pH, IMG_UINTPTR_T base, BM_MAPPING *psMapping);
static IMG_BOOL
BM_ImportMemory(IMG_VOID *pH, IMG_SIZE_T uSize,
					IMG_SIZE_T *pActualSize, BM_MAPPING **ppsMapping,
					IMG_UINT32 uFlags, IMG_UINTPTR_T *pBase);

static IMG_BOOL
DevMemoryAlloc (BM_CONTEXT *pBMContext,
				BM_MAPPING *pMapping,
				IMG_SIZE_T *pActualSize,
				IMG_UINT32 uFlags,
				IMG_UINT32 dev_vaddr_alignment,
				IMG_DEV_VIRTADDR *pDevVAddr);
static IMG_VOID
DevMemoryFree (BM_MAPPING *pMapping);

static IMG_BOOL
AllocMemory (BM_CONTEXT				*pBMContext,
				BM_HEAP				*psBMHeap,
				IMG_DEV_VIRTADDR	*psDevVAddr,
				IMG_SIZE_T			uSize,
				IMG_UINT32			uFlags,
				IMG_UINT32			uDevVAddrAlignment,
				BM_BUF				*pBuf)
{
	BM_MAPPING			*pMapping;
	IMG_UINTPTR_T		uOffset;
	RA_ARENA			*pArena = IMG_NULL;

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "AllocMemory (uSize=0x%x, uFlags=0x%x, align=0x%x)",
			  uSize, uFlags, uDevVAddrAlignment));

	


	if(uFlags & PVRSRV_MEM_RAM_BACKED_ALLOCATION)
	{
		if(uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR)
		{
			
			PVR_DPF ((PVR_DBG_ERROR, "AllocMemory: combination of DevVAddr management and RAM backing mode unsupported"));
			return IMG_FALSE;
		}

		

		
		if(psBMHeap->ui32Attribs
		   &	(PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG
		   |PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG))
		{
			
			pArena = psBMHeap->pImportArena;
			PVR_ASSERT(psBMHeap->sDevArena.psDeviceMemoryHeapInfo->ui32Attribs & PVRSRV_MEM_RAM_BACKED_ALLOCATION);
		}
		else
		{
			PVR_DPF ((PVR_DBG_ERROR, "AllocMemory: backing store type doesn't match heap"));
			return IMG_FALSE;
		}

		
		if (!RA_Alloc(pArena,
					  uSize,
					  IMG_NULL,
					  (IMG_VOID*) &pMapping,
					  uFlags,
					  uDevVAddrAlignment,
					  0,
					  (IMG_UINTPTR_T *)&(pBuf->DevVAddr.uiAddr)))
		{
			PVR_DPF((PVR_DBG_ERROR, "AllocMemory: RA_Alloc(0x%x) FAILED", uSize));
			return IMG_FALSE;
		}

		uOffset = pBuf->DevVAddr.uiAddr - pMapping->DevVAddr.uiAddr;
		if(pMapping->CpuVAddr)
		{
			pBuf->CpuVAddr = (IMG_VOID*) ((IMG_UINTPTR_T)pMapping->CpuVAddr + uOffset);
		}
		else
		{
			pBuf->CpuVAddr = IMG_NULL;
		}

		if(uSize == pMapping->uSize)
		{
			pBuf->hOSMemHandle = pMapping->hOSMemHandle;
		}
		else
		{
			if(OSGetSubMemHandle(pMapping->hOSMemHandle,
								 uOffset,
								 uSize,
								 psBMHeap->ui32Attribs,
								 &pBuf->hOSMemHandle)!=PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "AllocMemory: OSGetSubMemHandle FAILED"));
				return IMG_FALSE;
			}
		}

		
		pBuf->CpuPAddr.uiAddr = pMapping->CpuPAddr.uiAddr + uOffset;

		if(uFlags & PVRSRV_MEM_ZERO)
		{
			if(!ZeroBuf(pBuf, pMapping, uSize, psBMHeap->ui32Attribs | uFlags))
			{
				return IMG_FALSE;
			}
		}
	}
	else
	{
		if(uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR)
		{
			
			PVR_ASSERT(psDevVAddr != IMG_NULL);

			if (psDevVAddr == IMG_NULL)
			{
				PVR_DPF((PVR_DBG_ERROR, "AllocMemory: invalid parameter - psDevVAddr"));
				return IMG_FALSE;
			}

			
			pBMContext->psDeviceNode->pfnMMUAlloc (psBMHeap->pMMUHeap,
													uSize,
													IMG_NULL,
													PVRSRV_MEM_USER_SUPPLIED_DEVVADDR,
													uDevVAddrAlignment,
													psDevVAddr);

			
			pBuf->DevVAddr = *psDevVAddr;
		}
		else
		{
			

			
			pBMContext->psDeviceNode->pfnMMUAlloc (psBMHeap->pMMUHeap,
													uSize,
													IMG_NULL,
													0,
													uDevVAddrAlignment,
													&pBuf->DevVAddr);
		}

		
		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
							sizeof (struct _BM_MAPPING_),
							(IMG_PVOID *)&pMapping, IMG_NULL,
							"Buffer Manager Mapping") != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "AllocMemory: OSAllocMem(0x%x) FAILED", sizeof(*pMapping)));
			return IMG_FALSE;
		}

		
		pBuf->CpuVAddr = IMG_NULL;
		pBuf->hOSMemHandle = 0;
		pBuf->CpuPAddr.uiAddr = 0;

		
		pMapping->CpuVAddr = IMG_NULL;
		pMapping->CpuPAddr.uiAddr = 0;
		pMapping->DevVAddr = pBuf->DevVAddr;
		pMapping->psSysAddr = IMG_NULL;
		pMapping->uSize = uSize;
		pMapping->hOSMemHandle = 0;
	}

	
	pMapping->pArena = pArena;

	
	pMapping->pBMHeap = psBMHeap;
	pBuf->pMapping = pMapping;

	
	PVR_DPF ((PVR_DBG_MESSAGE,
				"AllocMemory: pMapping=%08x: DevV=%08X CpuV=%08x CpuP=%08X uSize=0x%x",
				(IMG_UINTPTR_T)pMapping,
				pMapping->DevVAddr.uiAddr,
				(IMG_UINTPTR_T)pMapping->CpuVAddr,
				pMapping->CpuPAddr.uiAddr,
				pMapping->uSize));

	PVR_DPF ((PVR_DBG_MESSAGE,
				"AllocMemory: pBuf=%08x: DevV=%08X CpuV=%08x CpuP=%08X uSize=0x%x",
				(IMG_UINTPTR_T)pBuf,
				pBuf->DevVAddr.uiAddr,
				(IMG_UINTPTR_T)pBuf->CpuVAddr,
				pBuf->CpuPAddr.uiAddr,
				uSize));

	
	PVR_ASSERT(((pBuf->DevVAddr.uiAddr) & (uDevVAddrAlignment - 1)) == 0);

	return IMG_TRUE;
}


static IMG_BOOL
WrapMemory (BM_HEAP *psBMHeap,
			IMG_SIZE_T uSize,
			IMG_SIZE_T ui32BaseOffset,
			IMG_BOOL bPhysContig,
			IMG_SYS_PHYADDR *psAddr,
			IMG_VOID *pvCPUVAddr,
			IMG_UINT32 uFlags,
			BM_BUF *pBuf)
{
	IMG_DEV_VIRTADDR DevVAddr = {0};
	BM_MAPPING *pMapping;
	IMG_BOOL bResult;
	IMG_SIZE_T const ui32PageSize = HOST_PAGESIZE();

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "WrapMemory(psBMHeap=%08X, size=0x%x, offset=0x%x, bPhysContig=0x%x, pvCPUVAddr = 0x%08x, flags=0x%x)",
			  (IMG_UINTPTR_T)psBMHeap, uSize, ui32BaseOffset, bPhysContig, (IMG_UINTPTR_T)pvCPUVAddr, uFlags));

	PVR_ASSERT((psAddr->uiAddr & (ui32PageSize - 1)) == 0);
	
	PVR_ASSERT(((IMG_UINTPTR_T)pvCPUVAddr & (ui32PageSize - 1)) == 0);

	uSize += ui32BaseOffset;
	uSize = HOST_PAGEALIGN (uSize);

	
	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(*pMapping),
						(IMG_PVOID *)&pMapping, IMG_NULL,
						"Mocked-up mapping") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "WrapMemory: OSAllocMem(0x%x) FAILED",sizeof(*pMapping)));
		return IMG_FALSE;
	}

	OSMemSet(pMapping, 0, sizeof (*pMapping));

	pMapping->uSize = uSize;
	pMapping->pBMHeap = psBMHeap;

	if(pvCPUVAddr)
	{
		pMapping->CpuVAddr = pvCPUVAddr;

		if (bPhysContig)
		{
			pMapping->eCpuMemoryOrigin = hm_wrapped_virtaddr;
			pMapping->CpuPAddr = SysSysPAddrToCpuPAddr(psAddr[0]);

			if(OSRegisterMem(pMapping->CpuPAddr,
							pMapping->CpuVAddr,
							pMapping->uSize,
							uFlags,
							&pMapping->hOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,	"WrapMemory: OSRegisterMem Phys=0x%08X, Size=%d) failed",
					pMapping->CpuPAddr.uiAddr, pMapping->uSize));
				goto fail_cleanup;
			}
		}
		else
		{
			pMapping->eCpuMemoryOrigin = hm_wrapped_scatter_virtaddr;
			pMapping->psSysAddr = psAddr;

			if(OSRegisterDiscontigMem(pMapping->psSysAddr,
							pMapping->CpuVAddr,
							pMapping->uSize,
							uFlags,
							&pMapping->hOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,	"WrapMemory: OSRegisterDiscontigMem Size=%d) failed",
					pMapping->uSize));
				goto fail_cleanup;
			}
		}
	}
	else
	{
		if (bPhysContig)
		{
			pMapping->eCpuMemoryOrigin = hm_wrapped;
			pMapping->CpuPAddr = SysSysPAddrToCpuPAddr(psAddr[0]);

			if(OSReservePhys(pMapping->CpuPAddr,
							 pMapping->uSize,
							 uFlags,
							 &pMapping->CpuVAddr,
							 &pMapping->hOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,	"WrapMemory: OSReservePhys Phys=0x%08X, Size=%d) failed",
					pMapping->CpuPAddr.uiAddr, pMapping->uSize));
				goto fail_cleanup;
			}
		}
		else
		{
			pMapping->eCpuMemoryOrigin = hm_wrapped_scatter;
			pMapping->psSysAddr = psAddr;

			if(OSReserveDiscontigPhys(pMapping->psSysAddr,
							 pMapping->uSize,
							 uFlags,
							 &pMapping->CpuVAddr,
							 &pMapping->hOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,	"WrapMemory: OSReserveDiscontigPhys Size=%d) failed",
					pMapping->uSize));
				goto fail_cleanup;
			}
		}
	}

	
	bResult = DevMemoryAlloc(psBMHeap->pBMContext,
							 pMapping,
							 IMG_NULL,
							 uFlags | PVRSRV_MEM_READ | PVRSRV_MEM_WRITE,
							 IMG_CAST_TO_DEVVADDR_UINT(ui32PageSize),
							 &DevVAddr);
	if (!bResult)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"WrapMemory: DevMemoryAlloc(0x%x) failed",
				pMapping->uSize));
		goto fail_cleanup;
	}

	
	pBuf->CpuPAddr.uiAddr = pMapping->CpuPAddr.uiAddr + ui32BaseOffset;
	if(!ui32BaseOffset)
	{
		pBuf->hOSMemHandle = pMapping->hOSMemHandle;
	}
	else
	{
		if(OSGetSubMemHandle(pMapping->hOSMemHandle,
							 ui32BaseOffset,
							 (pMapping->uSize-ui32BaseOffset),
							 uFlags,
							 &pBuf->hOSMemHandle)!=PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "WrapMemory: OSGetSubMemHandle failed"));
			goto fail_cleanup;
		}
	}
	if(pMapping->CpuVAddr)
	{
		pBuf->CpuVAddr = (IMG_VOID*) ((IMG_UINTPTR_T)pMapping->CpuVAddr + ui32BaseOffset);
	}
	pBuf->DevVAddr.uiAddr = pMapping->DevVAddr.uiAddr + IMG_CAST_TO_DEVVADDR_UINT(ui32BaseOffset);

	if(uFlags & PVRSRV_MEM_ZERO)
	{
		if(!ZeroBuf(pBuf, pMapping, uSize, uFlags))
		{
			return IMG_FALSE;
		}
	}

	PVR_DPF ((PVR_DBG_MESSAGE, "DevVaddr.uiAddr=%08X", DevVAddr.uiAddr));
	PVR_DPF ((PVR_DBG_MESSAGE,
				"WrapMemory: DevV=%08X CpuP=%08X uSize=0x%x",
				pMapping->DevVAddr.uiAddr, pMapping->CpuPAddr.uiAddr, pMapping->uSize));
	PVR_DPF ((PVR_DBG_MESSAGE,
				"WrapMemory: DevV=%08X CpuP=%08X uSize=0x%x",
				pBuf->DevVAddr.uiAddr, pBuf->CpuPAddr.uiAddr, uSize));

	pBuf->pMapping = pMapping;
	return IMG_TRUE;

fail_cleanup:
	if(ui32BaseOffset && pBuf->hOSMemHandle)
	{
		OSReleaseSubMemHandle(pBuf->hOSMemHandle, uFlags);
	}

	if(pMapping && (pMapping->CpuVAddr || pMapping->hOSMemHandle))
	{
		switch(pMapping->eCpuMemoryOrigin)
		{
			case hm_wrapped:
				OSUnReservePhys(pMapping->CpuVAddr, pMapping->uSize, uFlags, pMapping->hOSMemHandle);
				break;
			case hm_wrapped_virtaddr:
				OSUnRegisterMem(pMapping->CpuVAddr, pMapping->uSize, uFlags, pMapping->hOSMemHandle);
				break;
			case hm_wrapped_scatter:
				OSUnReserveDiscontigPhys(pMapping->CpuVAddr, pMapping->uSize, uFlags, pMapping->hOSMemHandle);
				break;
			case hm_wrapped_scatter_virtaddr:
				OSUnRegisterDiscontigMem(pMapping->CpuVAddr, pMapping->uSize, uFlags, pMapping->hOSMemHandle);
				break;
			default:
				break;
		}

	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_MAPPING), pMapping, IMG_NULL);
	

	return IMG_FALSE;
}


static IMG_BOOL
ZeroBuf(BM_BUF *pBuf, BM_MAPPING *pMapping, IMG_SIZE_T ui32Bytes, IMG_UINT32 ui32Flags)
{
	IMG_VOID *pvCpuVAddr;

	if(pBuf->CpuVAddr)
	{
		OSMemSet(pBuf->CpuVAddr, 0, ui32Bytes);
	}
	else if(pMapping->eCpuMemoryOrigin == hm_contiguous
			|| pMapping->eCpuMemoryOrigin == hm_wrapped)
	{
		pvCpuVAddr = OSMapPhysToLin(pBuf->CpuPAddr,
									ui32Bytes,
									PVRSRV_HAP_KERNEL_ONLY
									| (ui32Flags & PVRSRV_HAP_CACHETYPE_MASK),
									IMG_NULL);
		if(!pvCpuVAddr)
		{
			PVR_DPF((PVR_DBG_ERROR, "ZeroBuf: OSMapPhysToLin for contiguous buffer failed"));
			return IMG_FALSE;
		}
		OSMemSet(pvCpuVAddr, 0, ui32Bytes);
		OSUnMapPhysToLin(pvCpuVAddr,
						 ui32Bytes,
						 PVRSRV_HAP_KERNEL_ONLY
						 | (ui32Flags & PVRSRV_HAP_CACHETYPE_MASK),
						 IMG_NULL);
	}
	else
	{
		IMG_SIZE_T ui32BytesRemaining = ui32Bytes;
		IMG_SIZE_T ui32CurrentOffset = 0;
		IMG_CPU_PHYADDR CpuPAddr;

		
		PVR_ASSERT(pBuf->hOSMemHandle);

		while(ui32BytesRemaining > 0)
		{
			IMG_SIZE_T ui32BlockBytes = MIN(ui32BytesRemaining, HOST_PAGESIZE());
			CpuPAddr = OSMemHandleToCpuPAddr(pBuf->hOSMemHandle, ui32CurrentOffset);
			
			if(CpuPAddr.uiAddr & (HOST_PAGESIZE() -1))
			{
				ui32BlockBytes =
					MIN(ui32BytesRemaining, (IMG_UINT32)(HOST_PAGEALIGN(CpuPAddr.uiAddr) - CpuPAddr.uiAddr));
			}

			pvCpuVAddr = OSMapPhysToLin(CpuPAddr,
										ui32BlockBytes,
										PVRSRV_HAP_KERNEL_ONLY
										| (ui32Flags & PVRSRV_HAP_CACHETYPE_MASK),
										IMG_NULL);
			if(!pvCpuVAddr)
			{
				PVR_DPF((PVR_DBG_ERROR, "ZeroBuf: OSMapPhysToLin while zeroing non-contiguous memory FAILED"));
				return IMG_FALSE;
			}
			OSMemSet(pvCpuVAddr, 0, ui32BlockBytes);
			OSUnMapPhysToLin(pvCpuVAddr,
							 ui32BlockBytes,
							 PVRSRV_HAP_KERNEL_ONLY
							 | (ui32Flags & PVRSRV_HAP_CACHETYPE_MASK),
							 IMG_NULL);

			ui32BytesRemaining -= ui32BlockBytes;
			ui32CurrentOffset += ui32BlockBytes;
		}
	}

	return IMG_TRUE;
}

static IMG_VOID
FreeBuf (BM_BUF *pBuf, IMG_UINT32 ui32Flags, IMG_BOOL bFromAllocator)
{
	BM_MAPPING *pMapping;

	PVR_DPF ((PVR_DBG_MESSAGE,
			"FreeBuf: pBuf=0x%x: DevVAddr=%08X CpuVAddr=0x%x CpuPAddr=%08X",
			(IMG_UINTPTR_T)pBuf, pBuf->DevVAddr.uiAddr,
			(IMG_UINTPTR_T)pBuf->CpuVAddr, pBuf->CpuPAddr.uiAddr));

	
	pMapping = pBuf->pMapping;

	if(ui32Flags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR)
	{
		
		if ((pBuf->ui32ExportCount == 0) && (pBuf->ui32RefCount == 0))
		{
			
			if(ui32Flags & PVRSRV_MEM_RAM_BACKED_ALLOCATION)
			{
				
				PVR_DPF ((PVR_DBG_ERROR, "FreeBuf: combination of DevVAddr management and RAM backing mode unsupported"));
			}
			else
			{
				
				OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_MAPPING), pMapping, IMG_NULL);
				pBuf->pMapping = IMG_NULL; 
			}
		}
	}
	else
	{
		
		if(pBuf->hOSMemHandle != pMapping->hOSMemHandle)
		{
            
			if ((pBuf->ui32ExportCount == 0) && (pBuf->ui32RefCount == 0))
			{
				
				OSReleaseSubMemHandle(pBuf->hOSMemHandle, ui32Flags);
			}
		}
		if(ui32Flags & PVRSRV_MEM_RAM_BACKED_ALLOCATION)
		{
			
            
			if ((pBuf->ui32ExportCount == 0) && (pBuf->ui32RefCount == 0))
			{
				


				PVR_ASSERT(pBuf->ui32ExportCount == 0)
				RA_Free (pBuf->pMapping->pArena, pBuf->DevVAddr.uiAddr, IMG_FALSE);
			}
		}
		else
		{
			if ((pBuf->ui32ExportCount == 0) && (pBuf->ui32RefCount == 0))
			{
				switch (pMapping->eCpuMemoryOrigin)
				{
					case hm_wrapped:
						OSUnReservePhys(pMapping->CpuVAddr, pMapping->uSize, ui32Flags, pMapping->hOSMemHandle);
						break;
					case hm_wrapped_virtaddr:
						OSUnRegisterMem(pMapping->CpuVAddr, pMapping->uSize, ui32Flags, pMapping->hOSMemHandle);
						break;
					case hm_wrapped_scatter:
						OSUnReserveDiscontigPhys(pMapping->CpuVAddr, pMapping->uSize, ui32Flags, pMapping->hOSMemHandle);
						break;
					case hm_wrapped_scatter_virtaddr:
						OSUnRegisterDiscontigMem(pMapping->CpuVAddr, pMapping->uSize, ui32Flags, pMapping->hOSMemHandle);
						break;
					default:
						break;
				}
			}
			if (bFromAllocator)
				DevMemoryFree (pMapping);

			if ((pBuf->ui32ExportCount == 0) && (pBuf->ui32RefCount == 0))
			{
				
				OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_MAPPING), pMapping, IMG_NULL);
				pBuf->pMapping = IMG_NULL; 
			}
		}
	}


	if ((pBuf->ui32ExportCount == 0) && (pBuf->ui32RefCount == 0))
	{
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_BUF), pBuf, IMG_NULL);
		
	}
}

static PVRSRV_ERROR BM_DestroyContext_AnyCb(BM_HEAP *psBMHeap)
{
	if(psBMHeap->ui32Attribs
	& 	(PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG
		|PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG))
	{
		if (psBMHeap->pImportArena)
		{
			IMG_BOOL bTestDelete = RA_TestDelete(psBMHeap->pImportArena);
			if (!bTestDelete)
			{
				PVR_DPF ((PVR_DBG_ERROR, "BM_DestroyContext_AnyCb: RA_TestDelete failed"));
				return PVRSRV_ERROR_UNABLE_TO_DESTROY_BM_HEAP;
			}
		}
	}
	return PVRSRV_OK;
}


PVRSRV_ERROR
BM_DestroyContext(IMG_HANDLE	hBMContext,
				  IMG_BOOL		*pbDestroyed)
{
	PVRSRV_ERROR eError;
	BM_CONTEXT *pBMContext = (BM_CONTEXT*)hBMContext;

	PVR_DPF ((PVR_DBG_MESSAGE, "BM_DestroyContext"));

	if (pbDestroyed != IMG_NULL)
	{
		*pbDestroyed = IMG_FALSE;
	}

	

	if (pBMContext == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR, "BM_DestroyContext: Invalid handle"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	pBMContext->ui32RefCount--;

	if (pBMContext->ui32RefCount > 0)
	{
		
		return PVRSRV_OK;
	}

	


	eError = List_BM_HEAP_PVRSRV_ERROR_Any(pBMContext->psBMHeap, &BM_DestroyContext_AnyCb);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF ((PVR_DBG_ERROR, "BM_DestroyContext: List_BM_HEAP_PVRSRV_ERROR_Any failed"));
#if 0
		
		
		
		
		PVR_DPF ((PVR_DBG_ERROR, "BM_DestroyContext: Cleaning up with ResManFreeSpecial"));
		if(ResManFreeSpecial() != PVRSRV_OK)
		{
			PVR_DPF ((PVR_DBG_ERROR, "BM_DestroyContext: ResManFreeSpecial failed %d",eError));
		}
		
#endif
		return eError;
	}
	else
	{
		
		eError = ResManFreeResByPtr(pBMContext->hResItem);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF ((PVR_DBG_ERROR, "BM_DestroyContext: ResManFreeResByPtr failed %d",eError));
			return eError;
		}

		
		if (pbDestroyed != IMG_NULL)
		{
			*pbDestroyed = IMG_TRUE;
		}
	}

	return PVRSRV_OK;
}


static PVRSRV_ERROR BM_DestroyContextCallBack_AnyVaCb(BM_HEAP *psBMHeap, va_list va)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	psDeviceNode = va_arg(va, PVRSRV_DEVICE_NODE*);

	
	if(psBMHeap->ui32Attribs
	& 	(PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG
		|PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG))
	{
		if (psBMHeap->pImportArena)
		{
			RA_Delete (psBMHeap->pImportArena);
		}
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_DestroyContext: backing store type unsupported"));
		return PVRSRV_ERROR_UNSUPPORTED_BACKING_STORE;
	}

	
	psDeviceNode->pfnMMUDelete(psBMHeap->pMMUHeap);

	
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_HEAP), psBMHeap, IMG_NULL);
	

	return PVRSRV_OK;
}


static PVRSRV_ERROR BM_DestroyContextCallBack(IMG_PVOID		pvParam,
											  IMG_UINT32	ui32Param)
{
	BM_CONTEXT *pBMContext = pvParam;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	PVRSRV_ERROR eError;
	PVR_UNREFERENCED_PARAMETER(ui32Param);

	

	psDeviceNode = pBMContext->psDeviceNode;

	

	eError = List_BM_HEAP_PVRSRV_ERROR_Any_va(pBMContext->psBMHeap,
										&BM_DestroyContextCallBack_AnyVaCb,
										psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}
	

	if (pBMContext->psMMUContext)
	{
		psDeviceNode->pfnMMUFinalise(pBMContext->psMMUContext);
	}

	

	if (pBMContext->pBufferHash)
	{
		HASH_Delete(pBMContext->pBufferHash);
	}

	if (pBMContext == psDeviceNode->sDevMemoryInfo.pBMKernelContext)
	{
		
		psDeviceNode->sDevMemoryInfo.pBMKernelContext = IMG_NULL;
	}
	else
	{
		
		List_BM_CONTEXT_Remove(pBMContext);
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_CONTEXT), pBMContext, IMG_NULL);
	

	return PVRSRV_OK;
}


static IMG_HANDLE BM_CreateContext_IncRefCount_AnyVaCb(BM_CONTEXT *pBMContext, va_list va)
{
	PRESMAN_CONTEXT	hResManContext;
	hResManContext = va_arg(va, PRESMAN_CONTEXT);
	if(ResManFindResourceByPtr(hResManContext, pBMContext->hResItem) == PVRSRV_OK)
	{
		
		pBMContext->ui32RefCount++;
		return pBMContext;
	}
	return IMG_NULL;
}

static IMG_VOID BM_CreateContext_InsertHeap_ForEachVaCb(BM_HEAP *psBMHeap, va_list va)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	BM_CONTEXT *pBMContext;
	psDeviceNode = va_arg(va, PVRSRV_DEVICE_NODE*);
	pBMContext = va_arg(va, BM_CONTEXT*);
	switch(psBMHeap->sDevArena.DevMemHeapType)
	{
		case DEVICE_MEMORY_HEAP_SHARED:
		case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
		{
			
			psDeviceNode->pfnMMUInsertHeap(pBMContext->psMMUContext, psBMHeap->pMMUHeap);
			break;
		}
	}
}

IMG_HANDLE
BM_CreateContext(PVRSRV_DEVICE_NODE			*psDeviceNode,
				 IMG_DEV_PHYADDR			*psPDDevPAddr,
				 PVRSRV_PER_PROCESS_DATA	*psPerProc,
				 IMG_BOOL					*pbCreated)
{
	BM_CONTEXT			*pBMContext;
	DEVICE_MEMORY_INFO	*psDevMemoryInfo;
	IMG_BOOL			bKernelContext;
	PRESMAN_CONTEXT		hResManContext;

	PVR_DPF((PVR_DBG_MESSAGE, "BM_CreateContext"));

	if (psPerProc == IMG_NULL)
	{
		bKernelContext = IMG_TRUE;
		hResManContext = psDeviceNode->hResManContext;
	}
	else
	{
		bKernelContext = IMG_FALSE;
		hResManContext = psPerProc->hResManContext;
	}

	if (pbCreated != IMG_NULL)
	{
		*pbCreated = IMG_FALSE;
	}

	
	psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;

	if (bKernelContext == IMG_FALSE)
	{
		IMG_HANDLE res = (IMG_HANDLE) List_BM_CONTEXT_Any_va(psDevMemoryInfo->pBMContext,
															&BM_CreateContext_IncRefCount_AnyVaCb,
															hResManContext);
		if (res)
		{
			return res;
		}
	}

	
	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
					 sizeof (struct _BM_CONTEXT_),
					 (IMG_PVOID *)&pBMContext, IMG_NULL,
					 "Buffer Manager Context") != PVRSRV_OK)
	{
		PVR_DPF ((PVR_DBG_ERROR, "BM_CreateContext: Alloc failed"));
		return IMG_NULL;
	}
	OSMemSet(pBMContext, 0, sizeof (BM_CONTEXT));

	
	pBMContext->psDeviceNode = psDeviceNode;

	
	
	pBMContext->pBufferHash = HASH_Create(32);
	if (pBMContext->pBufferHash==IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR, "BM_CreateContext: HASH_Create failed"));
		goto cleanup;
	}

	if(psDeviceNode->pfnMMUInitialise(psDeviceNode,
										&pBMContext->psMMUContext,
										psPDDevPAddr) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_CreateContext: MMUInitialise failed"));
		goto cleanup;
	}

	if(bKernelContext)
	{
		
		PVR_ASSERT(psDevMemoryInfo->pBMKernelContext == IMG_NULL);
		psDevMemoryInfo->pBMKernelContext = pBMContext;
	}
	else
	{
		




		PVR_ASSERT(psDevMemoryInfo->pBMKernelContext);

		if (psDevMemoryInfo->pBMKernelContext == IMG_NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "BM_CreateContext: psDevMemoryInfo->pBMKernelContext invalid"));
			goto cleanup;
		}

		PVR_ASSERT(psDevMemoryInfo->pBMKernelContext->psBMHeap);

		



		pBMContext->psBMSharedHeap = psDevMemoryInfo->pBMKernelContext->psBMHeap;

		


		List_BM_HEAP_ForEach_va(pBMContext->psBMSharedHeap,
								&BM_CreateContext_InsertHeap_ForEachVaCb,
								psDeviceNode,
								pBMContext);

		
		List_BM_CONTEXT_Insert(&psDevMemoryInfo->pBMContext, pBMContext);
	}

	
	pBMContext->ui32RefCount++;

	
	pBMContext->hResItem = ResManRegisterRes(hResManContext,
											RESMAN_TYPE_DEVICEMEM_CONTEXT,
											pBMContext,
											0,
											&BM_DestroyContextCallBack);
	if (pBMContext->hResItem == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR, "BM_CreateContext: ResManRegisterRes failed"));
		goto cleanup;
	}

	if (pbCreated != IMG_NULL)
	{
		*pbCreated = IMG_TRUE;
	}
	return (IMG_HANDLE)pBMContext;

cleanup:
	(IMG_VOID)BM_DestroyContextCallBack(pBMContext, 0);

	return IMG_NULL;
}


static IMG_VOID *BM_CreateHeap_AnyVaCb(BM_HEAP *psBMHeap, va_list va)
{
	DEVICE_MEMORY_HEAP_INFO *psDevMemHeapInfo;
	psDevMemHeapInfo = va_arg(va, DEVICE_MEMORY_HEAP_INFO*);
	if (psBMHeap->sDevArena.ui32HeapID ==  psDevMemHeapInfo->ui32HeapID)
	{
		
		return psBMHeap;
	}
	else
	{
		return IMG_NULL;
	}
}

IMG_HANDLE
BM_CreateHeap (IMG_HANDLE hBMContext,
			   DEVICE_MEMORY_HEAP_INFO *psDevMemHeapInfo)
{
	BM_CONTEXT *pBMContext = (BM_CONTEXT*)hBMContext;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	BM_HEAP *psBMHeap;

	PVR_DPF((PVR_DBG_MESSAGE, "BM_CreateHeap"));

	if(!pBMContext)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_CreateHeap: BM_CONTEXT null"));
		return IMG_NULL;
	}

	psDeviceNode = pBMContext->psDeviceNode;

	




	if(pBMContext->ui32RefCount > 0)
	{
		psBMHeap = (BM_HEAP*)List_BM_HEAP_Any_va(pBMContext->psBMHeap,
												 &BM_CreateHeap_AnyVaCb,
												 psDevMemHeapInfo);

		if (psBMHeap)
		{
			return psBMHeap;
		}
	}


	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof (BM_HEAP),
						(IMG_PVOID *)&psBMHeap, IMG_NULL,
						"Buffer Manager Heap") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_CreateHeap: Alloc failed"));
		return IMG_NULL;
	}

	OSMemSet (psBMHeap, 0, sizeof (BM_HEAP));

	psBMHeap->sDevArena.ui32HeapID = psDevMemHeapInfo->ui32HeapID;
	psBMHeap->sDevArena.pszName = psDevMemHeapInfo->pszName;
	psBMHeap->sDevArena.BaseDevVAddr = psDevMemHeapInfo->sDevVAddrBase;
	psBMHeap->sDevArena.ui32Size = psDevMemHeapInfo->ui32HeapSize;
	psBMHeap->sDevArena.DevMemHeapType = psDevMemHeapInfo->DevMemHeapType;
	psBMHeap->sDevArena.ui32DataPageSize = psDevMemHeapInfo->ui32DataPageSize;
	psBMHeap->sDevArena.psDeviceMemoryHeapInfo = psDevMemHeapInfo;
	psBMHeap->ui32Attribs = psDevMemHeapInfo->ui32Attribs;

	
	psBMHeap->pBMContext = pBMContext;

	psBMHeap->pMMUHeap = psDeviceNode->pfnMMUCreate (pBMContext->psMMUContext,
													&psBMHeap->sDevArena,
													&psBMHeap->pVMArena,
													&psBMHeap->psMMUAttrib);
	if (!psBMHeap->pMMUHeap)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_CreateHeap: MMUCreate failed"));
		goto ErrorExit;
	}

	
	psBMHeap->pImportArena = RA_Create (psDevMemHeapInfo->pszBSName,
										0, 0, IMG_NULL,
										MAX(HOST_PAGESIZE(), psBMHeap->sDevArena.ui32DataPageSize),
										&BM_ImportMemory,
										&BM_FreeMemory,
										IMG_NULL,
										psBMHeap);
	if(psBMHeap->pImportArena == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_CreateHeap: RA_Create failed"));
		goto ErrorExit;
	}

	if(psBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG)
	{
		



		psBMHeap->pLocalDevMemArena = psDevMemHeapInfo->psLocalDevMemArena;
		if(psBMHeap->pLocalDevMemArena == IMG_NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "BM_CreateHeap: LocalDevMemArena null"));
			goto ErrorExit;
		}
	}

	
	List_BM_HEAP_Insert(&pBMContext->psBMHeap, psBMHeap);

	return (IMG_HANDLE)psBMHeap;

	
ErrorExit:

	
	if (psBMHeap->pMMUHeap != IMG_NULL)
	{
		psDeviceNode->pfnMMUDelete (psBMHeap->pMMUHeap);
		psDeviceNode->pfnMMUFinalise (pBMContext->psMMUContext);
	}

	
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_HEAP), psBMHeap, IMG_NULL);
	

	return IMG_NULL;
}

IMG_VOID
BM_DestroyHeap (IMG_HANDLE hDevMemHeap)
{
	BM_HEAP* psBMHeap = (BM_HEAP*)hDevMemHeap;
	PVRSRV_DEVICE_NODE *psDeviceNode = psBMHeap->pBMContext->psDeviceNode;

	PVR_DPF((PVR_DBG_MESSAGE, "BM_DestroyHeap"));

	if(psBMHeap)
	{
		
		if(psBMHeap->ui32Attribs
		&	(PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG
			|PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG))
		{
			if (psBMHeap->pImportArena)
			{
				RA_Delete (psBMHeap->pImportArena);
			}
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "BM_DestroyHeap: backing store type unsupported"));
			return;
		}

		
		psDeviceNode->pfnMMUDelete (psBMHeap->pMMUHeap);

		
		List_BM_HEAP_Remove(psBMHeap);
		
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_HEAP), psBMHeap, IMG_NULL);
		
	}
	else
	{
		PVR_DPF ((PVR_DBG_ERROR, "BM_DestroyHeap: invalid heap handle"));
	}
}


IMG_BOOL
BM_Reinitialise (PVRSRV_DEVICE_NODE *psDeviceNode)
{

	PVR_DPF((PVR_DBG_MESSAGE, "BM_Reinitialise"));
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);


	return IMG_TRUE;
}

IMG_BOOL
BM_Alloc (  IMG_HANDLE			hDevMemHeap,
			IMG_DEV_VIRTADDR	*psDevVAddr,
			IMG_SIZE_T			uSize,
			IMG_UINT32			*pui32Flags,
			IMG_UINT32			uDevVAddrAlignment,
			BM_HANDLE			*phBuf)
{
	BM_BUF *pBuf;
	BM_CONTEXT *pBMContext;
	BM_HEAP *psBMHeap;
	SYS_DATA *psSysData;
	IMG_UINT32 uFlags;

	if (pui32Flags == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_Alloc: invalid parameter"));
		PVR_DBG_BREAK;
		return IMG_FALSE;
	}

	uFlags = *pui32Flags;

	PVR_DPF ((PVR_DBG_MESSAGE,
		  "BM_Alloc (uSize=0x%x, uFlags=0x%x, uDevVAddrAlignment=0x%x)",
			uSize, uFlags, uDevVAddrAlignment));

	SysAcquireData(&psSysData);

	psBMHeap = (BM_HEAP*)hDevMemHeap;
	pBMContext = psBMHeap->pBMContext;

	if(uDevVAddrAlignment == 0)
	{
		uDevVAddrAlignment = 1;
	}

	
	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				   sizeof (BM_BUF),
				   (IMG_PVOID *)&pBuf, IMG_NULL,
				   "Buffer Manager buffer") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_Alloc: BM_Buf alloc FAILED"));
		return IMG_FALSE;
	}
	OSMemSet(pBuf, 0, sizeof (BM_BUF));

	
	if (AllocMemory(pBMContext,
					psBMHeap,
					psDevVAddr,
					uSize,
					uFlags,
					uDevVAddrAlignment,
					pBuf) != IMG_TRUE)
	{
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof (BM_BUF), pBuf, IMG_NULL);
		
		PVR_DPF((PVR_DBG_ERROR, "BM_Alloc: AllocMemory FAILED"));
		return IMG_FALSE;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
		  "BM_Alloc (uSize=0x%x, uFlags=0x%x)",
		  uSize, uFlags));

	
	pBuf->ui32RefCount = 1;
	*phBuf = (BM_HANDLE)pBuf;
	*pui32Flags = uFlags | psBMHeap->ui32Attribs;

	
	if(uFlags & PVRSRV_HAP_CACHETYPE_MASK)
	{
		*pui32Flags &= ~PVRSRV_HAP_CACHETYPE_MASK;
		*pui32Flags |= (uFlags & PVRSRV_HAP_CACHETYPE_MASK);
	}

	return IMG_TRUE;
}



#if defined(PVR_LMA)
static IMG_BOOL
ValidSysPAddrArrayForDev(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_SYS_PHYADDR *psSysPAddr, IMG_UINT32 ui32PageCount, IMG_SIZE_T ui32PageSize)
{
	IMG_UINT32 i;

	for (i = 0; i < ui32PageCount; i++)
	{
		IMG_SYS_PHYADDR sStartSysPAddr = psSysPAddr[i];
		IMG_SYS_PHYADDR sEndSysPAddr;

		if (!SysVerifySysPAddrToDevPAddr(psDeviceNode->sDevId.eDeviceType, sStartSysPAddr))
		{
			return IMG_FALSE;
		}

		sEndSysPAddr.uiAddr = sStartSysPAddr.uiAddr + ui32PageSize;

		if (!SysVerifySysPAddrToDevPAddr(psDeviceNode->sDevId.eDeviceType, sEndSysPAddr))
		{
			return IMG_FALSE;
		}
	}

	return IMG_TRUE;
}

static IMG_BOOL
ValidSysPAddrRangeForDev(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_SYS_PHYADDR sStartSysPAddr, IMG_SIZE_T ui32Range)
{
	IMG_SYS_PHYADDR sEndSysPAddr;

	if (!SysVerifySysPAddrToDevPAddr(psDeviceNode->sDevId.eDeviceType, sStartSysPAddr))
	{
		return IMG_FALSE;
	}

	sEndSysPAddr.uiAddr = sStartSysPAddr.uiAddr + ui32Range;

	if (!SysVerifySysPAddrToDevPAddr(psDeviceNode->sDevId.eDeviceType, sEndSysPAddr))
	{
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

#define	WRAP_MAPPING_SIZE(ui32ByteSize, ui32PageOffset) HOST_PAGEALIGN((ui32ByteSize) + (ui32PageOffset))

#define	WRAP_PAGE_COUNT(ui32ByteSize, ui32PageOffset, ui32HostPageSize)	(WRAP_MAPPING_SIZE(ui32ByteSize, ui32PageOffset) / (ui32HostPageSize))

#endif


IMG_BOOL
BM_Wrap (	IMG_HANDLE hDevMemHeap,
			IMG_SIZE_T ui32Size,
			IMG_SIZE_T ui32Offset,
			IMG_BOOL bPhysContig,
			IMG_SYS_PHYADDR *psSysAddr,
			IMG_VOID *pvCPUVAddr,
			IMG_UINT32 *pui32Flags,
			BM_HANDLE *phBuf)
{
	BM_BUF *pBuf;
	BM_CONTEXT *psBMContext;
	BM_HEAP *psBMHeap;
	SYS_DATA *psSysData;
	IMG_SYS_PHYADDR sHashAddress;
	IMG_UINT32 uFlags;

	psBMHeap = (BM_HEAP*)hDevMemHeap;
	psBMContext = psBMHeap->pBMContext;

	uFlags = psBMHeap->ui32Attribs & (PVRSRV_HAP_CACHETYPE_MASK | PVRSRV_HAP_MAPTYPE_MASK);

	if ((pui32Flags != IMG_NULL) && ((*pui32Flags & PVRSRV_HAP_CACHETYPE_MASK) != 0))
	{
		uFlags &= ~PVRSRV_HAP_CACHETYPE_MASK;
		uFlags |= *pui32Flags & PVRSRV_HAP_CACHETYPE_MASK;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
		  "BM_Wrap (uSize=0x%x, uOffset=0x%x, bPhysContig=0x%x, pvCPUVAddr=0x%x, uFlags=0x%x)",
			ui32Size, ui32Offset, bPhysContig, (IMG_UINTPTR_T)pvCPUVAddr, uFlags));

	SysAcquireData(&psSysData);

#if defined(PVR_LMA)
	if (bPhysContig)
	{
		if (!ValidSysPAddrRangeForDev(psBMContext->psDeviceNode, *psSysAddr, WRAP_MAPPING_SIZE(ui32Size, ui32Offset)))
		{
			PVR_DPF((PVR_DBG_ERROR, "BM_Wrap: System address range invalid for device"));
			return IMG_FALSE;
		}
	}
	else
	{
		IMG_SIZE_T ui32HostPageSize = HOST_PAGESIZE();

		if (!ValidSysPAddrArrayForDev(psBMContext->psDeviceNode, psSysAddr, WRAP_PAGE_COUNT(ui32Size, ui32Offset, ui32HostPageSize), ui32HostPageSize))
		{
			PVR_DPF((PVR_DBG_ERROR, "BM_Wrap: Array of system addresses invalid for device"));
			return IMG_FALSE;
		}
	}
#endif
	
	sHashAddress = psSysAddr[0];

	
	sHashAddress.uiAddr += ui32Offset;

	
	pBuf = (BM_BUF *)HASH_Retrieve(psBMContext->pBufferHash, sHashAddress.uiAddr);

	if(pBuf)
	{
		IMG_SIZE_T ui32MappingSize = HOST_PAGEALIGN (ui32Size + ui32Offset);

		
		if(pBuf->pMapping->uSize == ui32MappingSize && (pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped ||
														pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped_virtaddr))
		{
			PVR_DPF((PVR_DBG_MESSAGE,
					"BM_Wrap (Matched previous Wrap! uSize=0x%x, uOffset=0x%x, SysAddr=%08X)",
					ui32Size, ui32Offset, sHashAddress.uiAddr));

			pBuf->ui32RefCount++;
			*phBuf = (BM_HANDLE)pBuf;
			if(pui32Flags)
				*pui32Flags = uFlags;

			return IMG_TRUE;
		}
	}

	
	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof (BM_BUF),
						(IMG_PVOID *)&pBuf, IMG_NULL,
						"Buffer Manager buffer") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_Wrap: BM_Buf alloc FAILED"));
		return IMG_FALSE;
	}
	OSMemSet(pBuf, 0, sizeof (BM_BUF));

	
	if (WrapMemory (psBMHeap, ui32Size, ui32Offset, bPhysContig, psSysAddr, pvCPUVAddr, uFlags, pBuf) != IMG_TRUE)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_Wrap: WrapMemory FAILED"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof (BM_BUF), pBuf, IMG_NULL);
		
		return IMG_FALSE;
	}

	
	if(pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped || pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped_virtaddr)
	{
		
		PVR_ASSERT(SysSysPAddrToCpuPAddr(sHashAddress).uiAddr == pBuf->CpuPAddr.uiAddr);

		if (!HASH_Insert (psBMContext->pBufferHash, sHashAddress.uiAddr, (IMG_UINTPTR_T)pBuf))
		{
			FreeBuf (pBuf, uFlags, IMG_TRUE);
			PVR_DPF((PVR_DBG_ERROR, "BM_Wrap: HASH_Insert FAILED"));
			return IMG_FALSE;
		}
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
			"BM_Wrap (uSize=0x%x, uFlags=0x%x, devVAddr=%08X)",
			ui32Size, uFlags, pBuf->DevVAddr.uiAddr));

	
	pBuf->ui32RefCount = 1;
	*phBuf = (BM_HANDLE)pBuf;
	if(pui32Flags)
	{
		
		*pui32Flags = (uFlags & ~PVRSRV_HAP_MAPTYPE_MASK) | PVRSRV_HAP_MULTI_PROCESS;
	}

	return IMG_TRUE;
}

IMG_VOID
BM_Export (BM_HANDLE hBuf)
{
	BM_BUF *pBuf = (BM_BUF *)hBuf;

	pBuf->ui32ExportCount++;
}

IMG_VOID
BM_FreeExport(BM_HANDLE hBuf,
		IMG_UINT32 ui32Flags)
{
	BM_BUF *pBuf = (BM_BUF *)hBuf;

	pBuf->ui32ExportCount--;
	FreeBuf (pBuf, ui32Flags, IMG_FALSE);
}

IMG_VOID
BM_Free (BM_HANDLE hBuf,
		IMG_UINT32 ui32Flags)
{
	BM_BUF *pBuf = (BM_BUF *)hBuf;
	SYS_DATA *psSysData;
	IMG_SYS_PHYADDR sHashAddr;

	PVR_DPF ((PVR_DBG_MESSAGE, "BM_Free (h=0x%x)", (IMG_UINTPTR_T)hBuf));
	PVR_ASSERT (pBuf!=IMG_NULL);

	if (pBuf == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_Free: invalid parameter"));
		return;
	}

	SysAcquireData(&psSysData);

	pBuf->ui32RefCount--;

	if(pBuf->ui32RefCount == 0)
	{
		if(pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped || pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped_virtaddr)
		{
			sHashAddr = SysCpuPAddrToSysPAddr(pBuf->CpuPAddr);

			HASH_Remove (pBuf->pMapping->pBMHeap->pBMContext->pBufferHash,	(IMG_UINTPTR_T)sHashAddr.uiAddr);
		}
		FreeBuf (pBuf, ui32Flags, IMG_TRUE);
	}
}


IMG_CPU_VIRTADDR
BM_HandleToCpuVaddr (BM_HANDLE hBuf)
{
	BM_BUF *pBuf = (BM_BUF *)hBuf;

	PVR_ASSERT (pBuf != IMG_NULL);
	if (pBuf == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_HandleToCpuVaddr: invalid parameter"));
		return IMG_NULL;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
				"BM_HandleToCpuVaddr(h=0x%x)=0x%x",
				(IMG_UINTPTR_T)hBuf, (IMG_UINTPTR_T)pBuf->CpuVAddr));
	return pBuf->CpuVAddr;
}


IMG_DEV_VIRTADDR
BM_HandleToDevVaddr (BM_HANDLE hBuf)
{
	BM_BUF *pBuf = (BM_BUF *)hBuf;

	PVR_ASSERT (pBuf != IMG_NULL);
	if (pBuf == IMG_NULL)
	{
		IMG_DEV_VIRTADDR	DevVAddr = {0};
		PVR_DPF((PVR_DBG_ERROR, "BM_HandleToDevVaddr: invalid parameter"));
		return DevVAddr;
	}

	PVR_DPF ((PVR_DBG_MESSAGE, "BM_HandleToDevVaddr(h=0x%x)=%08X", (IMG_UINTPTR_T)hBuf, pBuf->DevVAddr.uiAddr));
	return pBuf->DevVAddr;
}


IMG_SYS_PHYADDR
BM_HandleToSysPaddr (BM_HANDLE hBuf)
{
	BM_BUF *pBuf = (BM_BUF *)hBuf;

	PVR_ASSERT (pBuf != IMG_NULL);

	if (pBuf == IMG_NULL)
	{
		IMG_SYS_PHYADDR	PhysAddr = {0};
		PVR_DPF((PVR_DBG_ERROR, "BM_HandleToSysPaddr: invalid parameter"));
		return PhysAddr;
	}

	PVR_DPF ((PVR_DBG_MESSAGE, "BM_HandleToSysPaddr(h=0x%x)=%08X", (IMG_UINTPTR_T)hBuf, pBuf->CpuPAddr.uiAddr));
	return SysCpuPAddrToSysPAddr (pBuf->CpuPAddr);
}

IMG_HANDLE
BM_HandleToOSMemHandle(BM_HANDLE hBuf)
{
	BM_BUF *pBuf = (BM_BUF *)hBuf;

	PVR_ASSERT (pBuf != IMG_NULL);

	if (pBuf == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_HandleToOSMemHandle: invalid parameter"));
		return IMG_NULL;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
				"BM_HandleToOSMemHandle(h=0x%x)=0x%x",
				(IMG_UINTPTR_T)hBuf, (IMG_UINTPTR_T)pBuf->hOSMemHandle));
	return pBuf->hOSMemHandle;
}

static IMG_BOOL
DevMemoryAlloc (BM_CONTEXT *pBMContext,
				BM_MAPPING *pMapping,
				IMG_SIZE_T *pActualSize,
				IMG_UINT32 uFlags,
				IMG_UINT32 dev_vaddr_alignment,
				IMG_DEV_VIRTADDR *pDevVAddr)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
#ifdef PDUMP
	IMG_UINT32 ui32PDumpSize = pMapping->uSize;
#endif

	psDeviceNode = pBMContext->psDeviceNode;

	if(uFlags & PVRSRV_MEM_INTERLEAVED)
	{
		
		pMapping->uSize *= 2;
	}

#ifdef PDUMP
	if(uFlags & PVRSRV_MEM_DUMMY)
	{
		
		ui32PDumpSize = pMapping->pBMHeap->sDevArena.ui32DataPageSize;
	}
#endif

	
	if (!psDeviceNode->pfnMMUAlloc (pMapping->pBMHeap->pMMUHeap,
									pMapping->uSize,
									pActualSize,
									0,
									dev_vaddr_alignment,
									&(pMapping->DevVAddr)))
	{
		PVR_DPF((PVR_DBG_ERROR, "DevMemoryAlloc ERROR MMU_Alloc"));
		return IMG_FALSE;
	}

#ifdef SUPPORT_SGX_MMU_BYPASS
	EnableHostAccess(pBMContext->psMMUContext);
#endif

#if defined(PDUMP)
	
	PDUMPMALLOCPAGES(&psDeviceNode->sDevId,
					 pMapping->DevVAddr.uiAddr,
					 pMapping->CpuVAddr,
					 pMapping->hOSMemHandle,
					 ui32PDumpSize,
					 pMapping->pBMHeap->sDevArena.ui32DataPageSize,
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
					 psDeviceNode->pfnMMUIsHeapShared(pMapping->pBMHeap->pMMUHeap),
#else
					 IMG_FALSE, 
#endif 
					 (IMG_HANDLE)pMapping);
#endif

	switch (pMapping->eCpuMemoryOrigin)
	{
		case hm_wrapped:
		case hm_wrapped_virtaddr:
		case hm_contiguous:
		{
			psDeviceNode->pfnMMUMapPages (	pMapping->pBMHeap->pMMUHeap,
							pMapping->DevVAddr,
							SysCpuPAddrToSysPAddr (pMapping->CpuPAddr),
							pMapping->uSize,
							uFlags,
							(IMG_HANDLE)pMapping);

			*pDevVAddr = pMapping->DevVAddr;
			break;
		}
		case hm_env:
		{
			psDeviceNode->pfnMMUMapShadow (	pMapping->pBMHeap->pMMUHeap,
							pMapping->DevVAddr,
							pMapping->uSize,
							pMapping->CpuVAddr,
							pMapping->hOSMemHandle,
							pDevVAddr,
							uFlags,
							(IMG_HANDLE)pMapping);
			break;
		}
		case hm_wrapped_scatter:
		case hm_wrapped_scatter_virtaddr:
		{
			psDeviceNode->pfnMMUMapScatter (pMapping->pBMHeap->pMMUHeap,
							pMapping->DevVAddr,
							pMapping->psSysAddr,
							pMapping->uSize,
							uFlags,
							(IMG_HANDLE)pMapping);

			*pDevVAddr = pMapping->DevVAddr;
			break;
		}
		default:
			PVR_DPF((PVR_DBG_ERROR,
				"Illegal value %d for pMapping->eCpuMemoryOrigin",
				pMapping->eCpuMemoryOrigin));
			return IMG_FALSE;
	}

#ifdef SUPPORT_SGX_MMU_BYPASS
	DisableHostAccess(pBMContext->psMMUContext);
#endif

	return IMG_TRUE;
}

static IMG_VOID
DevMemoryFree (BM_MAPPING *pMapping)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
#ifdef PDUMP
	IMG_UINT32 ui32PSize;
#endif

#ifdef PDUMP
	
	if(pMapping->ui32Flags & PVRSRV_MEM_DUMMY)
	{
		
		ui32PSize = pMapping->pBMHeap->sDevArena.ui32DataPageSize;
	}
	else
	{
		ui32PSize = pMapping->uSize;
	}

	PDUMPFREEPAGES(pMapping->pBMHeap,
                    pMapping->DevVAddr,
                    ui32PSize,
                    pMapping->pBMHeap->sDevArena.ui32DataPageSize,
                    (IMG_HANDLE)pMapping,
                    (pMapping->ui32Flags & PVRSRV_MEM_INTERLEAVED) ? IMG_TRUE : IMG_FALSE);
#endif

	psDeviceNode = pMapping->pBMHeap->pBMContext->psDeviceNode;

	psDeviceNode->pfnMMUFree (pMapping->pBMHeap->pMMUHeap, pMapping->DevVAddr, IMG_CAST_TO_DEVVADDR_UINT(pMapping->uSize));
}

static IMG_BOOL
BM_ImportMemory (IMG_VOID *pH,
			  IMG_SIZE_T uRequestSize,
			  IMG_SIZE_T *pActualSize,
			  BM_MAPPING **ppsMapping,
			  IMG_UINT32 uFlags,
			  IMG_UINTPTR_T *pBase)
{
	BM_MAPPING *pMapping;
	BM_HEAP *pBMHeap = pH;
	BM_CONTEXT *pBMContext = pBMHeap->pBMContext;
	IMG_BOOL bResult;
	IMG_SIZE_T uSize;
	IMG_SIZE_T uPSize;
	IMG_UINT32 uDevVAddrAlignment = 0;

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "BM_ImportMemory (pBMContext=0x%x, uRequestSize=0x%x, uFlags=0x%x, uAlign=0x%x)",
			  (IMG_UINTPTR_T)pBMContext, uRequestSize, uFlags, uDevVAddrAlignment));

	PVR_ASSERT (ppsMapping != IMG_NULL);
	PVR_ASSERT (pBMContext != IMG_NULL);

	if (ppsMapping == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_ImportMemory: invalid parameter"));
		goto fail_exit;
	}

	uSize = HOST_PAGEALIGN (uRequestSize);
	PVR_ASSERT (uSize >= uRequestSize);

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof (BM_MAPPING),
						(IMG_PVOID *)&pMapping, IMG_NULL,
						"Buffer Manager Mapping") != PVRSRV_OK)
	{
		PVR_DPF ((PVR_DBG_ERROR, "BM_ImportMemory: failed BM_MAPPING alloc"));
		goto fail_exit;
	}

	pMapping->hOSMemHandle = 0;
	pMapping->CpuVAddr = 0;
	pMapping->DevVAddr.uiAddr = 0;
	pMapping->CpuPAddr.uiAddr = 0;
	pMapping->uSize = uSize;
	pMapping->pBMHeap = pBMHeap;
	pMapping->ui32Flags = uFlags;

	
	if (pActualSize)
	{
		*pActualSize = uSize;
	}

	
	if(pMapping->ui32Flags & PVRSRV_MEM_DUMMY)
	{
		uPSize = pBMHeap->sDevArena.ui32DataPageSize;
	}
	else
	{
		uPSize = pMapping->uSize;
	}

	

	if(pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG)
	{
		IMG_UINT32 ui32Attribs = pBMHeap->ui32Attribs;

		
		if (pMapping->ui32Flags & PVRSRV_HAP_CACHETYPE_MASK)
		{
			ui32Attribs &= ~PVRSRV_HAP_CACHETYPE_MASK;
			ui32Attribs |= (pMapping->ui32Flags & PVRSRV_HAP_CACHETYPE_MASK);
		}

		
		if (OSAllocPages(ui32Attribs,
						 uPSize,
						 pBMHeap->sDevArena.ui32DataPageSize,
						 (IMG_VOID **)&pMapping->CpuVAddr,
						 &pMapping->hOSMemHandle) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"BM_ImportMemory: OSAllocPages(0x%x) failed",
					uPSize));
			goto fail_mapping_alloc;
		}

		
		pMapping->eCpuMemoryOrigin = hm_env;
	}
	else if(pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG)
	{
		IMG_SYS_PHYADDR sSysPAddr;
		IMG_UINT32 ui32Attribs = pBMHeap->ui32Attribs;

		
		PVR_ASSERT(pBMHeap->pLocalDevMemArena != IMG_NULL);

		
		if (pMapping->ui32Flags & PVRSRV_HAP_CACHETYPE_MASK)
		{
			ui32Attribs &= ~PVRSRV_HAP_CACHETYPE_MASK;
			ui32Attribs |= (pMapping->ui32Flags & PVRSRV_HAP_CACHETYPE_MASK);
		}

		if (!RA_Alloc (pBMHeap->pLocalDevMemArena,
					   uPSize,
					   IMG_NULL,
					   IMG_NULL,
					   0,
					   pBMHeap->sDevArena.ui32DataPageSize,
					   0,
					   (IMG_UINTPTR_T *)&sSysPAddr.uiAddr))
		{
			PVR_DPF((PVR_DBG_ERROR, "BM_ImportMemory: RA_Alloc(0x%x) FAILED", uPSize));
			goto fail_mapping_alloc;
		}

		
		pMapping->CpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
		if(OSReservePhys(pMapping->CpuPAddr,
						 uPSize,
						 ui32Attribs,
						 &pMapping->CpuVAddr,
						 &pMapping->hOSMemHandle) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,	"BM_ImportMemory: OSReservePhys failed"));
			goto fail_dev_mem_alloc;
		}

		
		pMapping->eCpuMemoryOrigin = hm_contiguous;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR,	"BM_ImportMemory: Invalid backing store type"));
		goto fail_mapping_alloc;
	}

	
	bResult = DevMemoryAlloc (pBMContext,
								pMapping,
								IMG_NULL,
								uFlags,
								uDevVAddrAlignment,
								&pMapping->DevVAddr);
	if (!bResult)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"BM_ImportMemory: DevMemoryAlloc(0x%x) failed",
				pMapping->uSize));
		goto fail_dev_mem_alloc;
	}

	
	
	PVR_ASSERT (uDevVAddrAlignment>1?(pMapping->DevVAddr.uiAddr%uDevVAddrAlignment)==0:1);

	*pBase = pMapping->DevVAddr.uiAddr;
	*ppsMapping = pMapping;

	PVR_DPF ((PVR_DBG_MESSAGE, "BM_ImportMemory: IMG_TRUE"));
	return IMG_TRUE;

fail_dev_mem_alloc:
	if (pMapping && (pMapping->CpuVAddr || pMapping->hOSMemHandle))
	{
		
		if(pMapping->ui32Flags & PVRSRV_MEM_INTERLEAVED)
		{
			pMapping->uSize /= 2;
		}

		if(pMapping->ui32Flags & PVRSRV_MEM_DUMMY)
		{
			uPSize = pBMHeap->sDevArena.ui32DataPageSize;
		}
		else
		{
			uPSize = pMapping->uSize;
		}

		if(pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG)
		{
			OSFreePages(pBMHeap->ui32Attribs,
						  uPSize,
						  (IMG_VOID *)pMapping->CpuVAddr,
						  pMapping->hOSMemHandle);
		}
		else
		{
			IMG_SYS_PHYADDR sSysPAddr;

			if(pMapping->CpuVAddr)
			{
				OSUnReservePhys(pMapping->CpuVAddr,
								uPSize,
								pBMHeap->ui32Attribs,
								pMapping->hOSMemHandle);
			}
			sSysPAddr = SysCpuPAddrToSysPAddr(pMapping->CpuPAddr);
			RA_Free (pBMHeap->pLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);
		}
	}
fail_mapping_alloc:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_MAPPING), pMapping, IMG_NULL);
	
fail_exit:
	return IMG_FALSE;
}


static IMG_VOID
BM_FreeMemory (IMG_VOID *h, IMG_UINTPTR_T _base, BM_MAPPING *psMapping)
{
	BM_HEAP *pBMHeap = h;
	IMG_SIZE_T uPSize;

	PVR_UNREFERENCED_PARAMETER (_base);

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "BM_FreeMemory (h=0x%x, base=0x%x, psMapping=0x%x)",
			  (IMG_UINTPTR_T)h, _base, (IMG_UINTPTR_T)psMapping));

	PVR_ASSERT (psMapping != IMG_NULL);

	if (psMapping == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "BM_FreeMemory: invalid parameter"));
		return;
	}

	DevMemoryFree (psMapping);

	
	if((psMapping->ui32Flags & PVRSRV_MEM_INTERLEAVED) != 0)
	{
		psMapping->uSize /= 2;
	}

	if(psMapping->ui32Flags & PVRSRV_MEM_DUMMY)
	{
		uPSize = psMapping->pBMHeap->sDevArena.ui32DataPageSize;
	}
	else
	{
		uPSize = psMapping->uSize;
	}

	if(pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG)
	{
		OSFreePages(pBMHeap->ui32Attribs,
						uPSize,
						(IMG_VOID *) psMapping->CpuVAddr,
						psMapping->hOSMemHandle);
	}
	else if(pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG)
	{
		IMG_SYS_PHYADDR sSysPAddr;

		OSUnReservePhys(psMapping->CpuVAddr, uPSize, pBMHeap->ui32Attribs, psMapping->hOSMemHandle);

		sSysPAddr = SysCpuPAddrToSysPAddr(psMapping->CpuPAddr);

		RA_Free (pBMHeap->pLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR,	"BM_FreeMemory: Invalid backing store type"));
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BM_MAPPING), psMapping, IMG_NULL);
	

	PVR_DPF((PVR_DBG_MESSAGE,
			"..BM_FreeMemory (h=0x%x, base=0x%x)",
			(IMG_UINTPTR_T)h, _base));
}

IMG_VOID BM_GetPhysPageAddr(PVRSRV_KERNEL_MEM_INFO *psMemInfo,
								IMG_DEV_VIRTADDR sDevVPageAddr,
								IMG_DEV_PHYADDR *psDevPAddr)
{
	PVRSRV_DEVICE_NODE *psDeviceNode;

	PVR_DPF((PVR_DBG_MESSAGE, "BM_GetPhysPageAddr"));

	PVR_ASSERT (psMemInfo && psDevPAddr)

	
	PVR_ASSERT((sDevVPageAddr.uiAddr & 0xFFF) == 0);

	psDeviceNode = ((BM_BUF*)psMemInfo->sMemBlk.hBuffer)->pMapping->pBMHeap->pBMContext->psDeviceNode;

	*psDevPAddr = psDeviceNode->pfnMMUGetPhysPageAddr(((BM_BUF*)psMemInfo->sMemBlk.hBuffer)->pMapping->pBMHeap->pMMUHeap,
												sDevVPageAddr);
}


MMU_CONTEXT* BM_GetMMUContext(IMG_HANDLE hDevMemHeap)
{
	BM_HEAP *pBMHeap = (BM_HEAP*)hDevMemHeap;

	PVR_DPF((PVR_DBG_VERBOSE, "BM_GetMMUContext"));

	return pBMHeap->pBMContext->psMMUContext;
}

MMU_CONTEXT* BM_GetMMUContextFromMemContext(IMG_HANDLE hDevMemContext)
{
	BM_CONTEXT *pBMContext = (BM_CONTEXT*)hDevMemContext;

	PVR_DPF ((PVR_DBG_VERBOSE, "BM_GetMMUContextFromMemContext"));

	return pBMContext->psMMUContext;
}

IMG_HANDLE BM_GetMMUHeap(IMG_HANDLE hDevMemHeap)
{
	PVR_DPF((PVR_DBG_VERBOSE, "BM_GetMMUHeap"));

	return (IMG_HANDLE)((BM_HEAP*)hDevMemHeap)->pMMUHeap;
}


PVRSRV_DEVICE_NODE* BM_GetDeviceNode(IMG_HANDLE hDevMemContext)
{
	PVR_DPF((PVR_DBG_VERBOSE, "BM_GetDeviceNode"));

	return ((BM_CONTEXT*)hDevMemContext)->psDeviceNode;
}


IMG_HANDLE BM_GetMappingHandle(PVRSRV_KERNEL_MEM_INFO *psMemInfo)
{
	PVR_DPF((PVR_DBG_VERBOSE, "BM_GetMappingHandle"));

	return ((BM_BUF*)psMemInfo->sMemBlk.hBuffer)->pMapping->hOSMemHandle;
}

