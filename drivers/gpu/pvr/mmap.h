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

#if !defined(__MMAP_H__)
#define __MMAP_H__

#include <linux/mm.h>
#include <linux/list.h>

#if defined(VM_MIXEDMAP)
#define	PVR_MAKE_ALL_PFNS_SPECIAL
#endif

#include "perproc.h"
#include "mm.h"

typedef struct KV_OFFSET_STRUCT_TAG
{
    
    IMG_UINT32			ui32Mapped;

    
    IMG_UINT32                  ui32MMapOffset;
    
    IMG_UINT32			ui32RealByteSize;

    
    LinuxMemArea                *psLinuxMemArea;
    
#if !defined(PVR_MAKE_ALL_PFNS_SPECIAL)
    
    IMG_UINT32			ui32TID;
#endif

    
    IMG_UINT32			ui32PID;

    
    IMG_BOOL			bOnMMapList;

    
    IMG_UINT32			ui32RefCount;

    
    IMG_UINT32			ui32UserVAddr;

    
#if defined(DEBUG_LINUX_MMAP_AREAS)
    const IMG_CHAR		*pszName;
#endif
    
   
   struct list_head		sMMapItem;

   
   struct list_head		sAreaItem;
}KV_OFFSET_STRUCT, *PKV_OFFSET_STRUCT;



IMG_VOID PVRMMapInit(IMG_VOID);


IMG_VOID PVRMMapCleanup(IMG_VOID);


PVRSRV_ERROR PVRMMapRegisterArea(LinuxMemArea *psLinuxMemArea);


PVRSRV_ERROR PVRMMapRemoveRegisteredArea(LinuxMemArea *psLinuxMemArea);


PVRSRV_ERROR PVRMMapOSMemHandleToMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
					     IMG_HANDLE hMHandle,
                                             IMG_UINT32 *pui32MMapOffset,
                                             IMG_UINT32 *pui32ByteOffset,
                                             IMG_UINT32 *pui32RealByteSize,						     IMG_UINT32 *pui32UserVAddr);

PVRSRV_ERROR
PVRMMapReleaseMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
				IMG_HANDLE hMHandle,
				IMG_BOOL *pbMUnmap,
				IMG_UINT32 *pui32RealByteSize,
                                IMG_UINT32 *pui32UserVAddr);

int PVRMMap(struct file* pFile, struct vm_area_struct* ps_vma);


#endif	

