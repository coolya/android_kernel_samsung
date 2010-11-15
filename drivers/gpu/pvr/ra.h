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

#ifndef _RA_H_
#define _RA_H_

#include "img_types.h"
#include "hash.h"
#include "osfunc.h"

typedef struct _RA_ARENA_ RA_ARENA;			
typedef struct _BM_MAPPING_ BM_MAPPING;



#define RA_STATS 


struct _RA_STATISTICS_
{
    
    IMG_SIZE_T uSpanCount;

    
    IMG_SIZE_T uLiveSegmentCount;

    
    IMG_SIZE_T uFreeSegmentCount;

    
    IMG_SIZE_T uTotalResourceCount;
    
    
    IMG_SIZE_T uFreeResourceCount;

    
    IMG_SIZE_T uCumulativeAllocs;

    
    IMG_SIZE_T uCumulativeFrees;

    
    IMG_SIZE_T uImportCount;

    
    IMG_SIZE_T uExportCount;
};
typedef struct _RA_STATISTICS_ RA_STATISTICS;

struct _RA_SEGMENT_DETAILS_
{
	IMG_SIZE_T      uiSize;
	IMG_CPU_PHYADDR sCpuPhyAddr;
	IMG_HANDLE      hSegment;
};
typedef struct _RA_SEGMENT_DETAILS_ RA_SEGMENT_DETAILS;

RA_ARENA *
RA_Create (IMG_CHAR *name,
           IMG_UINTPTR_T base,
           IMG_SIZE_T uSize,
           BM_MAPPING *psMapping,
           IMG_SIZE_T uQuantum, 
           IMG_BOOL (*imp_alloc)(IMG_VOID *_h,
                                IMG_SIZE_T uSize,
                                IMG_SIZE_T *pActualSize,
                                BM_MAPPING **ppsMapping,
                                IMG_UINT32 uFlags,
                                IMG_UINTPTR_T *pBase),
           IMG_VOID (*imp_free) (IMG_VOID *,
                                IMG_UINTPTR_T,
                                BM_MAPPING *),
           IMG_VOID (*backingstore_free) (IMG_VOID *,
                                          IMG_SIZE_T,
                                          IMG_SIZE_T,
                                          IMG_HANDLE),
           IMG_VOID *import_handle);

IMG_VOID
RA_Delete (RA_ARENA *pArena);

IMG_BOOL
RA_TestDelete (RA_ARENA *pArena);

IMG_BOOL
RA_Add (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_SIZE_T uSize);

IMG_BOOL
RA_Alloc (RA_ARENA *pArena, 
          IMG_SIZE_T uSize,
          IMG_SIZE_T *pActualSize,
          BM_MAPPING **ppsMapping, 
          IMG_UINT32 uFlags,
          IMG_UINT32 uAlignment,
		  IMG_UINT32 uAlignmentOffset,
          IMG_UINTPTR_T *pBase);

IMG_VOID 
RA_Free (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_BOOL bFreeBackingStore);


#ifdef RA_STATS

#define CHECK_SPACE(total)					\
{											\
	if((total)<100) 							\
		return PVRSRV_ERROR_INVALID_PARAMS;	\
}

#define UPDATE_SPACE(str, count, total)		\
{											\
	if((count) == -1)					 		\
		return PVRSRV_ERROR_INVALID_PARAMS;	\
	else									\
	{										\
		(str) += (count);						\
		(total) -= (count);						\
	}										\
}


IMG_BOOL RA_GetNextLiveSegment(IMG_HANDLE hArena, RA_SEGMENT_DETAILS *psSegDetails);


PVRSRV_ERROR RA_GetStats(RA_ARENA *pArena,
							IMG_CHAR **ppszStr, 
							IMG_UINT32 *pui32StrLen);

PVRSRV_ERROR RA_GetStatsFreeMem(RA_ARENA *pArena,
								IMG_CHAR **ppszStr, 
								IMG_UINT32 *pui32StrLen);

#endif 

#endif

