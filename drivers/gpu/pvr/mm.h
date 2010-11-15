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

#ifndef __IMG_LINUX_MM_H__
#define __IMG_LINUX_MM_H__

#ifndef AUTOCONF_INCLUDED
 #include <linux/config.h>
#endif

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/list.h>

#include <asm/io.h>

#define	PHYS_TO_PFN(phys) ((phys) >> PAGE_SHIFT)
#define PFN_TO_PHYS(pfn) ((pfn) << PAGE_SHIFT)

#define RANGE_TO_PAGES(range) (((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)

#define	ADDR_TO_PAGE_OFFSET(addr) (((unsigned long)(addr)) & (PAGE_SIZE - 1))

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10))
#define	REMAP_PFN_RANGE(vma, addr, pfn, size, prot) remap_pfn_range(vma, addr, pfn, size, prot)
#else
#define	REMAP_PFN_RANGE(vma, addr, pfn, size, prot) remap_page_range(vma, addr, PFN_TO_PHYS(pfn), size, prot)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
#define	IO_REMAP_PFN_RANGE(vma, addr, pfn, size, prot) io_remap_pfn_range(vma, addr, pfn, size, prot)
#else
#define	IO_REMAP_PFN_RANGE(vma, addr, pfn, size, prot) io_remap_page_range(vma, addr, PFN_TO_PHYS(pfn), size, prot)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))
#define	VM_INSERT_PAGE(vma, addr, page) vm_insert_page(vma, addr, page)
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10))
#define VM_INSERT_PAGE(vma, addr, page) remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE, vma->vm_page_prot);
#else
#define VM_INSERT_PAGE(vma, addr, page) remap_page_range(vma, addr, page_to_phys(page), PAGE_SIZE, vma->vm_page_prot);
#endif
#endif

static inline IMG_UINT32 VMallocToPhys(IMG_VOID *pCpuVAddr)
{
	return (page_to_phys(vmalloc_to_page(pCpuVAddr)) + ADDR_TO_PAGE_OFFSET(pCpuVAddr));
		
}

typedef enum {
    LINUX_MEM_AREA_IOREMAP,
	LINUX_MEM_AREA_EXTERNAL_KV,
    LINUX_MEM_AREA_IO,
    LINUX_MEM_AREA_VMALLOC,
    LINUX_MEM_AREA_ALLOC_PAGES,
    LINUX_MEM_AREA_SUB_ALLOC,
    LINUX_MEM_AREA_TYPE_COUNT
}LINUX_MEM_AREA_TYPE;

typedef struct _LinuxMemArea LinuxMemArea;


struct _LinuxMemArea {
    LINUX_MEM_AREA_TYPE eAreaType;
    union _uData
    {
        struct _sIORemap
        {
            
            IMG_CPU_PHYADDR CPUPhysAddr;
            IMG_VOID *pvIORemapCookie;
        }sIORemap;
        struct _sExternalKV
        {
            
	    IMG_BOOL bPhysContig;
	    union {
		    
		    IMG_SYS_PHYADDR SysPhysAddr;
		    IMG_SYS_PHYADDR *pSysPhysAddr;
	    } uPhysAddr;
            IMG_VOID *pvExternalKV;
        }sExternalKV;
        struct _sIO
        {
            
            IMG_CPU_PHYADDR CPUPhysAddr;
        }sIO;
        struct _sVmalloc
        {
            
            IMG_VOID *pvVmallocAddress;
        }sVmalloc;
        struct _sPageList
        {
            
            struct page **pvPageList;
	    IMG_HANDLE hBlockPageList;
        }sPageList;
        struct _sSubAlloc
        {
            
            LinuxMemArea *psParentLinuxMemArea;
            IMG_UINT32 ui32ByteOffset;
        }sSubAlloc;
    }uData;

    IMG_UINT32 ui32ByteSize;		

    IMG_UINT32 ui32AreaFlags;		

    IMG_BOOL bMMapRegistered;		

    IMG_BOOL bNeedsCacheInvalidate;	

    
    struct list_head	sMMapItem;

    
    struct list_head	sMMapOffsetStructList;
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17))
typedef kmem_cache_t LinuxKMemCache;
#else
typedef struct kmem_cache LinuxKMemCache;
#endif


PVRSRV_ERROR LinuxMMInit(IMG_VOID);


IMG_VOID LinuxMMCleanup(IMG_VOID);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMallocWrapper(ui32ByteSize) _KMallocWrapper(ui32ByteSize, __FILE__, __LINE__)
#else
#define KMallocWrapper(ui32ByteSize) _KMallocWrapper(ui32ByteSize, NULL, 0)
#endif
IMG_VOID *_KMallocWrapper(IMG_UINT32 ui32ByteSize, IMG_CHAR *szFileName, IMG_UINT32 ui32Line);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KFreeWrapper(pvCpuVAddr) _KFreeWrapper(pvCpuVAddr, __FILE__, __LINE__)
#else
#define KFreeWrapper(pvCpuVAddr) _KFreeWrapper(pvCpuVAddr, NULL, 0)
#endif
IMG_VOID _KFreeWrapper(IMG_VOID *pvCpuVAddr, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define VMallocWrapper(ui32Bytes, ui32AllocFlags) _VMallocWrapper(ui32Bytes, ui32AllocFlags, __FILE__, __LINE__)
#else
#define VMallocWrapper(ui32Bytes, ui32AllocFlags) _VMallocWrapper(ui32Bytes, ui32AllocFlags, NULL, 0)
#endif
IMG_VOID *_VMallocWrapper(IMG_UINT32 ui32Bytes, IMG_UINT32 ui32AllocFlags, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define VFreeWrapper(pvCpuVAddr) _VFreeWrapper(pvCpuVAddr, __FILE__, __LINE__)
#else
#define VFreeWrapper(pvCpuVAddr) _VFreeWrapper(pvCpuVAddr, NULL, 0)
#endif
IMG_VOID _VFreeWrapper(IMG_VOID *pvCpuVAddr, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


LinuxMemArea *NewVMallocLinuxMemArea(IMG_UINT32 ui32Bytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeVMallocLinuxMemArea(LinuxMemArea *psLinuxMemArea);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags) \
    _IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags, __FILE__, __LINE__)
#else
#define IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags) \
    _IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags, NULL, 0)
#endif
IMG_VOID *_IORemapWrapper(IMG_CPU_PHYADDR BasePAddr,
                          IMG_UINT32 ui32Bytes,
                          IMG_UINT32 ui32MappingFlags,
                          IMG_CHAR *pszFileName,
                          IMG_UINT32 ui32Line);


LinuxMemArea *NewIORemapLinuxMemArea(IMG_CPU_PHYADDR BasePAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeIORemapLinuxMemArea(LinuxMemArea *psLinuxMemArea);

LinuxMemArea *NewExternalKVLinuxMemArea(IMG_SYS_PHYADDR *pBasePAddr, IMG_VOID *pvCPUVAddr, IMG_UINT32 ui32Bytes, IMG_BOOL bPhysContig, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeExternalKVLinuxMemArea(LinuxMemArea *psLinuxMemArea);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define IOUnmapWrapper(pvIORemapCookie) \
    _IOUnmapWrapper(pvIORemapCookie, __FILE__, __LINE__)
#else
#define IOUnmapWrapper(pvIORemapCookie) \
    _IOUnmapWrapper(pvIORemapCookie, NULL, 0)
#endif
IMG_VOID _IOUnmapWrapper(IMG_VOID *pvIORemapCookie, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


struct page *LinuxMemAreaOffsetToPage(LinuxMemArea *psLinuxMemArea, IMG_UINT32 ui32ByteOffset);


LinuxKMemCache *KMemCacheCreateWrapper(IMG_CHAR *pszName, size_t Size, size_t Align, IMG_UINT32 ui32Flags);


IMG_VOID KMemCacheDestroyWrapper(LinuxKMemCache *psCache);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMemCacheAllocWrapper(psCache, Flags) _KMemCacheAllocWrapper(psCache, Flags, __FILE__, __LINE__)
#else
#define KMemCacheAllocWrapper(psCache, Flags) _KMemCacheAllocWrapper(psCache, Flags, NULL, 0)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
IMG_VOID *_KMemCacheAllocWrapper(LinuxKMemCache *psCache, gfp_t Flags, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);
#else
IMG_VOID *_KMemCacheAllocWrapper(LinuxKMemCache *psCache, int Flags, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMemCacheFreeWrapper(psCache, pvObject) _KMemCacheFreeWrapper(psCache, pvObject, __FILE__, __LINE__)
#else
#define KMemCacheFreeWrapper(psCache, pvObject) _KMemCacheFreeWrapper(psCache, pvObject, NULL, 0)
#endif
IMG_VOID _KMemCacheFreeWrapper(LinuxKMemCache *psCache, IMG_VOID *pvObject, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


const IMG_CHAR *KMemCacheNameWrapper(LinuxKMemCache *psCache);


LinuxMemArea *NewIOLinuxMemArea(IMG_CPU_PHYADDR BasePAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeIOLinuxMemArea(LinuxMemArea *psLinuxMemArea);


LinuxMemArea *NewAllocPagesLinuxMemArea(IMG_UINT32 ui32Bytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeAllocPagesLinuxMemArea(LinuxMemArea *psLinuxMemArea);


LinuxMemArea *NewSubLinuxMemArea(LinuxMemArea *psParentLinuxMemArea,
                                 IMG_UINT32 ui32ByteOffset,
                                 IMG_UINT32 ui32Bytes);


IMG_VOID LinuxMemAreaDeepFree(LinuxMemArea *psLinuxMemArea);


#if defined(LINUX_MEM_AREAS_DEBUG)
IMG_VOID LinuxMemAreaRegister(LinuxMemArea *psLinuxMemArea);
#else
#define LinuxMemAreaRegister(X)
#endif


IMG_VOID *LinuxMemAreaToCpuVAddr(LinuxMemArea *psLinuxMemArea);


IMG_CPU_PHYADDR LinuxMemAreaToCpuPAddr(LinuxMemArea *psLinuxMemArea, IMG_UINT32 ui32ByteOffset);


#define	 LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32ByteOffset) PHYS_TO_PFN(LinuxMemAreaToCpuPAddr(psLinuxMemArea, ui32ByteOffset).uiAddr)

IMG_BOOL LinuxMemAreaPhysIsContig(LinuxMemArea *psLinuxMemArea);

static inline LinuxMemArea *
LinuxMemAreaRoot(LinuxMemArea *psLinuxMemArea)
{
    if(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC)
    {
        return psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea;
    }
    else
    {
        return psLinuxMemArea;
    }
}


static inline LINUX_MEM_AREA_TYPE
LinuxMemAreaRootType(LinuxMemArea *psLinuxMemArea)
{
    return LinuxMemAreaRoot(psLinuxMemArea)->eAreaType;
}


const IMG_CHAR *LinuxMemAreaTypeToString(LINUX_MEM_AREA_TYPE eMemAreaType);


#if defined(DEBUG) || defined(DEBUG_LINUX_MEM_AREAS)
const IMG_CHAR *HAPFlagsToString(IMG_UINT32 ui32Flags);
#endif

#endif 

