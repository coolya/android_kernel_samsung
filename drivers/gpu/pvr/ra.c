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
#include "hash.h"
#include "ra.h"
#include "buffer_manager.h"
#include "osfunc.h"

#ifdef __linux__
#include <linux/kernel.h>
#include "proc.h"
#endif

#ifdef USE_BM_FREESPACE_CHECK
#include <stdio.h>
#endif

#define MINIMUM_HASH_SIZE (64)

#if defined(VALIDATE_ARENA_TEST)

typedef enum RESOURCE_DESCRIPTOR_TAG {

	RESOURCE_SPAN_LIVE				= 10,
	RESOURCE_SPAN_FREE,
	IMPORTED_RESOURCE_SPAN_START,
	IMPORTED_RESOURCE_SPAN_LIVE,
	IMPORTED_RESOURCE_SPAN_FREE,
	IMPORTED_RESOURCE_SPAN_END,

} RESOURCE_DESCRIPTOR;

typedef enum RESOURCE_TYPE_TAG {

	IMPORTED_RESOURCE_TYPE		= 20,
	NON_IMPORTED_RESOURCE_TYPE

} RESOURCE_TYPE;


static IMG_UINT32 ui32BoundaryTagID = 0;

IMG_UINT32 ValidateArena(RA_ARENA *pArena);
#endif

struct _BT_
{
	enum bt_type
	{
		btt_span,				
		btt_free,				
		btt_live				
	} type;

	
	IMG_UINTPTR_T base;
	IMG_SIZE_T uSize;

	
	struct _BT_ *pNextSegment;
	struct _BT_ *pPrevSegment;
	
	struct _BT_ *pNextFree;
	struct _BT_ *pPrevFree;
	
	BM_MAPPING *psMapping;

#if defined(VALIDATE_ARENA_TEST)
	RESOURCE_DESCRIPTOR eResourceSpan;
	RESOURCE_TYPE		eResourceType;

	
	IMG_UINT32			ui32BoundaryTagID;
#endif

};
typedef struct _BT_ BT;


struct _RA_ARENA_
{
	
	IMG_CHAR *name;

	
	IMG_SIZE_T uQuantum;

	
	IMG_BOOL (*pImportAlloc)(IMG_VOID *,
							 IMG_SIZE_T uSize,
							 IMG_SIZE_T *pActualSize,
							 BM_MAPPING **ppsMapping,
							 IMG_UINT32 uFlags,
							 IMG_UINTPTR_T *pBase);
	IMG_VOID (*pImportFree) (IMG_VOID *,
						 IMG_UINTPTR_T,
						 BM_MAPPING *psMapping);
	IMG_VOID (*pBackingStoreFree) (IMG_VOID *, IMG_SIZE_T, IMG_SIZE_T, IMG_HANDLE);

	
	IMG_VOID *pImportHandle;

	
#define FREE_TABLE_LIMIT 32

	
	BT *aHeadFree [FREE_TABLE_LIMIT];

	
	BT *pHeadSegment;
	BT *pTailSegment;

	
	HASH_TABLE *pSegmentHash;

#ifdef RA_STATS
	RA_STATISTICS sStatistics;
#endif

#if defined(CONFIG_PROC_FS) && defined(DEBUG)
#define PROC_NAME_SIZE		32

	struct proc_dir_entry* pProcInfo;
	struct proc_dir_entry* pProcSegs;

	IMG_BOOL bInitProcEntry;
#endif
};
#if defined(ENABLE_RA_DUMP)
IMG_VOID RA_Dump (RA_ARENA *pArena);
#endif

#if defined(CONFIG_PROC_FS) && defined(DEBUG)

static void RA_ProcSeqShowInfo(struct seq_file *sfile, void* el);
static void* RA_ProcSeqOff2ElementInfo(struct seq_file * sfile, loff_t off);

static void RA_ProcSeqShowRegs(struct seq_file *sfile, void* el);
static void* RA_ProcSeqOff2ElementRegs(struct seq_file * sfile, loff_t off);

#endif 

#ifdef USE_BM_FREESPACE_CHECK
IMG_VOID CheckBMFreespace(IMG_VOID);
#endif

#if defined(CONFIG_PROC_FS) && defined(DEBUG)
static IMG_CHAR *ReplaceSpaces(IMG_CHAR * const pS)
{
	IMG_CHAR *pT;

	for(pT = pS; *pT != 0; pT++)
	{
		if (*pT == ' ' || *pT == '\t')
		{
			*pT = '_';
		}
	}

	return pS;
}
#endif

static IMG_BOOL
_RequestAllocFail (IMG_VOID *_h,
				  IMG_SIZE_T _uSize,
				  IMG_SIZE_T *_pActualSize,
				  BM_MAPPING **_ppsMapping,
				  IMG_UINT32 _uFlags,
				  IMG_UINTPTR_T *_pBase)
{
	PVR_UNREFERENCED_PARAMETER (_h);
	PVR_UNREFERENCED_PARAMETER (_uSize);
	PVR_UNREFERENCED_PARAMETER (_pActualSize);
	PVR_UNREFERENCED_PARAMETER (_ppsMapping);
	PVR_UNREFERENCED_PARAMETER (_uFlags);
	PVR_UNREFERENCED_PARAMETER (_pBase);

	return IMG_FALSE;
}

static IMG_UINT32
pvr_log2 (IMG_SIZE_T n)
{
	IMG_UINT32 l = 0;
	n>>=1;
	while (n>0)
	{
		n>>=1;
		l++;
	}
	return l;
}

static PVRSRV_ERROR
_SegmentListInsertAfter (RA_ARENA *pArena,
						 BT *pInsertionPoint,
						 BT *pBT)
{
	PVR_ASSERT (pArena != IMG_NULL);
	PVR_ASSERT (pInsertionPoint != IMG_NULL);

	if ((pInsertionPoint == IMG_NULL) || (pArena == IMG_NULL))
	{
		PVR_DPF ((PVR_DBG_ERROR,"_SegmentListInsertAfter: invalid parameters"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	pBT->pNextSegment = pInsertionPoint->pNextSegment;
	pBT->pPrevSegment = pInsertionPoint;
	if (pInsertionPoint->pNextSegment == IMG_NULL)
		pArena->pTailSegment = pBT;
	else
		pInsertionPoint->pNextSegment->pPrevSegment = pBT;
	pInsertionPoint->pNextSegment = pBT;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_SegmentListInsert (RA_ARENA *pArena, BT *pBT)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	
	if (pArena->pHeadSegment == IMG_NULL)
	{
		pArena->pHeadSegment = pArena->pTailSegment = pBT;
		pBT->pNextSegment = pBT->pPrevSegment = IMG_NULL;
	}
	else
	{
		BT *pBTScan;

		if (pBT->base < pArena->pHeadSegment->base)
		{
			
			pBT->pNextSegment = pArena->pHeadSegment;
			pArena->pHeadSegment->pPrevSegment = pBT;
			pArena->pHeadSegment = pBT;
			pBT->pPrevSegment = IMG_NULL;
		}
		else
		{

			


			pBTScan = pArena->pHeadSegment;

			while ((pBTScan->pNextSegment != IMG_NULL)  && (pBT->base >= pBTScan->pNextSegment->base))
			{
				pBTScan = pBTScan->pNextSegment;
			}

			eError = _SegmentListInsertAfter (pArena, pBTScan, pBT);
			if (eError != PVRSRV_OK)
			{
				return eError;
			}
		}
	}
	return eError;
}

static IMG_VOID
_SegmentListRemove (RA_ARENA *pArena, BT *pBT)
{
	if (pBT->pPrevSegment == IMG_NULL)
		pArena->pHeadSegment = pBT->pNextSegment;
	else
		pBT->pPrevSegment->pNextSegment = pBT->pNextSegment;

	if (pBT->pNextSegment == IMG_NULL)
		pArena->pTailSegment = pBT->pPrevSegment;
	else
		pBT->pNextSegment->pPrevSegment = pBT->pPrevSegment;
}

static BT *
_SegmentSplit (RA_ARENA *pArena, BT *pBT, IMG_SIZE_T uSize)
{
	BT *pNeighbour;

	PVR_ASSERT (pArena != IMG_NULL);

	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"_SegmentSplit: invalid parameter - pArena"));
		return IMG_NULL;
	}

	if(OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
					sizeof(BT),
					(IMG_VOID **)&pNeighbour, IMG_NULL,
					"Boundary Tag") != PVRSRV_OK)
	{
		return IMG_NULL;
	}

	OSMemSet(pNeighbour, 0, sizeof(BT));

#if defined(VALIDATE_ARENA_TEST)
	pNeighbour->ui32BoundaryTagID = ++ui32BoundaryTagID;
#endif

	pNeighbour->pPrevSegment = pBT;
	pNeighbour->pNextSegment = pBT->pNextSegment;
	if (pBT->pNextSegment == IMG_NULL)
		pArena->pTailSegment = pNeighbour;
	else
		pBT->pNextSegment->pPrevSegment = pNeighbour;
	pBT->pNextSegment = pNeighbour;

	pNeighbour->type = btt_free;
	pNeighbour->uSize = pBT->uSize - uSize;
	pNeighbour->base = pBT->base + uSize;
	pNeighbour->psMapping = pBT->psMapping;
	pBT->uSize = uSize;

#if defined(VALIDATE_ARENA_TEST)
	if (pNeighbour->pPrevSegment->eResourceType == IMPORTED_RESOURCE_TYPE)
	{
		pNeighbour->eResourceType = IMPORTED_RESOURCE_TYPE;
		pNeighbour->eResourceSpan = IMPORTED_RESOURCE_SPAN_FREE;
	}
	else if (pNeighbour->pPrevSegment->eResourceType == NON_IMPORTED_RESOURCE_TYPE)
	{
		pNeighbour->eResourceType = NON_IMPORTED_RESOURCE_TYPE;
		pNeighbour->eResourceSpan = RESOURCE_SPAN_FREE;
	}
	else
	{
		PVR_DPF ((PVR_DBG_ERROR,"_SegmentSplit: pNeighbour->pPrevSegment->eResourceType unrecognized"));
		PVR_DBG_BREAK;
	}
#endif

	return pNeighbour;
}

static IMG_VOID
_FreeListInsert (RA_ARENA *pArena, BT *pBT)
{
	IMG_UINT32 uIndex;
	uIndex = pvr_log2 (pBT->uSize);
	pBT->type = btt_free;
	pBT->pNextFree = pArena->aHeadFree [uIndex];
	pBT->pPrevFree = IMG_NULL;
	if (pArena->aHeadFree[uIndex] != IMG_NULL)
		pArena->aHeadFree[uIndex]->pPrevFree = pBT;
	pArena->aHeadFree [uIndex] = pBT;
}

static IMG_VOID
_FreeListRemove (RA_ARENA *pArena, BT *pBT)
{
	IMG_UINT32 uIndex;
	uIndex = pvr_log2 (pBT->uSize);
	if (pBT->pNextFree != IMG_NULL)
		pBT->pNextFree->pPrevFree = pBT->pPrevFree;
	if (pBT->pPrevFree == IMG_NULL)
		pArena->aHeadFree[uIndex] = pBT->pNextFree;
	else
		pBT->pPrevFree->pNextFree = pBT->pNextFree;
}

static BT *
_BuildSpanMarker (IMG_UINTPTR_T base, IMG_SIZE_T uSize)
{
	BT *pBT;

	if(OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
					sizeof(BT),
					(IMG_VOID **)&pBT, IMG_NULL,
					"Boundary Tag") != PVRSRV_OK)
	{
		return IMG_NULL;
	}

	OSMemSet(pBT, 0, sizeof(BT));

#if defined(VALIDATE_ARENA_TEST)
	pBT->ui32BoundaryTagID = ++ui32BoundaryTagID;
#endif

	pBT->type = btt_span;
	pBT->base = base;
	pBT->uSize = uSize;
	pBT->psMapping = IMG_NULL;

	return pBT;
}

static BT *
_BuildBT (IMG_UINTPTR_T base, IMG_SIZE_T uSize)
{
	BT *pBT;

	if(OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
					sizeof(BT),
					(IMG_VOID **)&pBT, IMG_NULL,
					"Boundary Tag") != PVRSRV_OK)
	{
		return IMG_NULL;
	}

	OSMemSet(pBT, 0, sizeof(BT));

#if defined(VALIDATE_ARENA_TEST)
	pBT->ui32BoundaryTagID = ++ui32BoundaryTagID;
#endif

	pBT->type = btt_free;
	pBT->base = base;
	pBT->uSize = uSize;

	return pBT;
}

static BT *
_InsertResource (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_SIZE_T uSize)
{
	BT *pBT;
	PVR_ASSERT (pArena!=IMG_NULL);
	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"_InsertResource: invalid parameter - pArena"));
		return IMG_NULL;
	}

	pBT = _BuildBT (base, uSize);
	if (pBT != IMG_NULL)
	{

#if defined(VALIDATE_ARENA_TEST)
		pBT->eResourceSpan = RESOURCE_SPAN_FREE;
		pBT->eResourceType = NON_IMPORTED_RESOURCE_TYPE;
#endif

		if (_SegmentListInsert (pArena, pBT) != PVRSRV_OK)
		{
			PVR_DPF ((PVR_DBG_ERROR,"_InsertResource: call to _SegmentListInsert failed"));
			return IMG_NULL;
		}
		_FreeListInsert (pArena, pBT);
#ifdef RA_STATS
		pArena->sStatistics.uTotalResourceCount+=uSize;
		pArena->sStatistics.uFreeResourceCount+=uSize;
		pArena->sStatistics.uSpanCount++;
#endif
	}
	return pBT;
}

static BT *
_InsertResourceSpan (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_SIZE_T uSize)
{
	PVRSRV_ERROR eError;
	BT *pSpanStart;
	BT *pSpanEnd;
	BT *pBT;

	PVR_ASSERT (pArena != IMG_NULL);
	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"_InsertResourceSpan: invalid parameter - pArena"));
		return IMG_NULL;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "RA_InsertResourceSpan: arena='%s', base=0x%x, size=0x%x",
			  pArena->name, base, uSize));

	pSpanStart = _BuildSpanMarker (base, uSize);
	if (pSpanStart == IMG_NULL)
	{
		goto fail_start;
	}

#if defined(VALIDATE_ARENA_TEST)
	pSpanStart->eResourceSpan = IMPORTED_RESOURCE_SPAN_START;
	pSpanStart->eResourceType = IMPORTED_RESOURCE_TYPE;
#endif

	pSpanEnd = _BuildSpanMarker (base + uSize, 0);
	if (pSpanEnd == IMG_NULL)
	{
		goto fail_end;
	}

#if defined(VALIDATE_ARENA_TEST)
	pSpanEnd->eResourceSpan = IMPORTED_RESOURCE_SPAN_END;
	pSpanEnd->eResourceType = IMPORTED_RESOURCE_TYPE;
#endif

	pBT = _BuildBT (base, uSize);
	if (pBT == IMG_NULL)
	{
		goto fail_bt;
	}

#if defined(VALIDATE_ARENA_TEST)
	pBT->eResourceSpan = IMPORTED_RESOURCE_SPAN_FREE;
	pBT->eResourceType = IMPORTED_RESOURCE_TYPE;
#endif

	eError = _SegmentListInsert (pArena, pSpanStart);
	if (eError != PVRSRV_OK)
	{
		goto fail_SegListInsert;
	}

	eError = _SegmentListInsertAfter (pArena, pSpanStart, pBT);
	if (eError != PVRSRV_OK)
	{
		goto fail_SegListInsert;
	}

	_FreeListInsert (pArena, pBT);

	eError = _SegmentListInsertAfter (pArena, pBT, pSpanEnd);
	if (eError != PVRSRV_OK)
	{
		goto fail_SegListInsert;
	}

#ifdef RA_STATS
	pArena->sStatistics.uTotalResourceCount+=uSize;
#endif
	return pBT;

  fail_SegListInsert:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), pBT, IMG_NULL);
	
  fail_bt:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), pSpanEnd, IMG_NULL);
	
  fail_end:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), pSpanStart, IMG_NULL);
	
  fail_start:
	return IMG_NULL;
}

static IMG_VOID
_FreeBT (RA_ARENA *pArena, BT *pBT, IMG_BOOL bFreeBackingStore)
{
	BT *pNeighbour;
	IMG_UINTPTR_T uOrigBase;
	IMG_SIZE_T uOrigSize;

	PVR_ASSERT (pArena!=IMG_NULL);
	PVR_ASSERT (pBT!=IMG_NULL);

	if ((pArena == IMG_NULL) || (pBT == IMG_NULL))
	{
		PVR_DPF ((PVR_DBG_ERROR,"_FreeBT: invalid parameter"));
		return;
	}

#ifdef RA_STATS
	pArena->sStatistics.uLiveSegmentCount--;
	pArena->sStatistics.uFreeSegmentCount++;
	pArena->sStatistics.uFreeResourceCount+=pBT->uSize;
#endif

	uOrigBase = pBT->base;
	uOrigSize = pBT->uSize;

	
	pNeighbour = pBT->pPrevSegment;
	if (pNeighbour!=IMG_NULL
		&& pNeighbour->type == btt_free
		&& pNeighbour->base + pNeighbour->uSize == pBT->base)
	{
		_FreeListRemove (pArena, pNeighbour);
		_SegmentListRemove (pArena, pNeighbour);
		pBT->base = pNeighbour->base;
		pBT->uSize += pNeighbour->uSize;
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), pNeighbour, IMG_NULL);
		
#ifdef RA_STATS
		pArena->sStatistics.uFreeSegmentCount--;
#endif
	}

	
	pNeighbour = pBT->pNextSegment;
	if (pNeighbour!=IMG_NULL
		&& pNeighbour->type == btt_free
		&& pBT->base + pBT->uSize == pNeighbour->base)
	{
		_FreeListRemove (pArena, pNeighbour);
		_SegmentListRemove (pArena, pNeighbour);
		pBT->uSize += pNeighbour->uSize;
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), pNeighbour, IMG_NULL);
		
#ifdef RA_STATS
		pArena->sStatistics.uFreeSegmentCount--;
#endif
	}

	
	if (pArena->pBackingStoreFree != IMG_NULL && bFreeBackingStore)
	{
		IMG_UINTPTR_T	uRoundedStart, uRoundedEnd;

		
		uRoundedStart = (uOrigBase / pArena->uQuantum) * pArena->uQuantum;
		
		if (uRoundedStart < pBT->base)
		{
			uRoundedStart += pArena->uQuantum;
		}

		
		uRoundedEnd = ((uOrigBase + uOrigSize + pArena->uQuantum - 1) / pArena->uQuantum) * pArena->uQuantum;
		
		if (uRoundedEnd > (pBT->base + pBT->uSize))
		{
			uRoundedEnd -= pArena->uQuantum;
		}

		if (uRoundedStart < uRoundedEnd)
		{
			pArena->pBackingStoreFree(pArena->pImportHandle, (IMG_SIZE_T)uRoundedStart, (IMG_SIZE_T)uRoundedEnd, (IMG_HANDLE)0);
		}
	}

	if (pBT->pNextSegment!=IMG_NULL && pBT->pNextSegment->type == btt_span
		&& pBT->pPrevSegment!=IMG_NULL && pBT->pPrevSegment->type == btt_span)
	{
		BT *next = pBT->pNextSegment;
		BT *prev = pBT->pPrevSegment;
		_SegmentListRemove (pArena, next);
		_SegmentListRemove (pArena, prev);
		_SegmentListRemove (pArena, pBT);
		pArena->pImportFree (pArena->pImportHandle, pBT->base, pBT->psMapping);
#ifdef RA_STATS
		pArena->sStatistics.uSpanCount--;
		pArena->sStatistics.uExportCount++;
		pArena->sStatistics.uFreeSegmentCount--;
		pArena->sStatistics.uFreeResourceCount-=pBT->uSize;
		pArena->sStatistics.uTotalResourceCount-=pBT->uSize;
#endif
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), next, IMG_NULL);
		
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), prev, IMG_NULL);
		
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), pBT, IMG_NULL);
		
	}
	else
		_FreeListInsert (pArena, pBT);
}


static IMG_BOOL
_AttemptAllocAligned (RA_ARENA *pArena,
					  IMG_SIZE_T uSize,
					  BM_MAPPING **ppsMapping,
					  IMG_UINT32 uFlags,
					  IMG_UINT32 uAlignment,
					  IMG_UINT32 uAlignmentOffset,
					  IMG_UINTPTR_T *base)
{
	IMG_UINT32 uIndex;
	PVR_ASSERT (pArena!=IMG_NULL);
	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"_AttemptAllocAligned: invalid parameter - pArena"));
		return IMG_FALSE;
	}

	if (uAlignment>1)
		uAlignmentOffset %= uAlignment;

	

	uIndex = pvr_log2 (uSize);

#if 0
	
	if (1u<<uIndex < uSize)
		uIndex++;
#endif

	while (uIndex < FREE_TABLE_LIMIT && pArena->aHeadFree[uIndex]==IMG_NULL)
		uIndex++;

	while (uIndex < FREE_TABLE_LIMIT)
	{
		if (pArena->aHeadFree[uIndex]!=IMG_NULL)
		{
			
			BT *pBT;

			pBT = pArena->aHeadFree [uIndex];
			while (pBT!=IMG_NULL)
			{
				IMG_UINTPTR_T aligned_base;

				if (uAlignment>1)
					aligned_base = (pBT->base + uAlignmentOffset + uAlignment - 1) / uAlignment * uAlignment - uAlignmentOffset;
				else
					aligned_base = pBT->base;
				PVR_DPF ((PVR_DBG_MESSAGE,
						  "RA_AttemptAllocAligned: pBT-base=0x%x "
						  "pBT-size=0x%x alignedbase=0x%x size=0x%x",
						pBT->base, pBT->uSize, aligned_base, uSize));

				if (pBT->base + pBT->uSize >= aligned_base + uSize)
				{
					if(!pBT->psMapping || pBT->psMapping->ui32Flags == uFlags)
					{
						_FreeListRemove (pArena, pBT);

						PVR_ASSERT (pBT->type == btt_free);

#ifdef RA_STATS
						pArena->sStatistics.uLiveSegmentCount++;
						pArena->sStatistics.uFreeSegmentCount--;
						pArena->sStatistics.uFreeResourceCount-=pBT->uSize;
#endif

						
						if (aligned_base > pBT->base)
						{
							BT *pNeighbour;
							pNeighbour = _SegmentSplit (pArena, pBT, (IMG_SIZE_T)(aligned_base - pBT->base));
							
							if (pNeighbour==IMG_NULL)
							{
								PVR_DPF ((PVR_DBG_ERROR,"_AttemptAllocAligned: Front split failed"));
								
								_FreeListInsert (pArena, pBT);
								return IMG_FALSE;
							}

							_FreeListInsert (pArena, pBT);
	#ifdef RA_STATS
							pArena->sStatistics.uFreeSegmentCount++;
							pArena->sStatistics.uFreeResourceCount+=pBT->uSize;
	#endif
							pBT = pNeighbour;
						}

						
						if (pBT->uSize > uSize)
						{
							BT *pNeighbour;
							pNeighbour = _SegmentSplit (pArena, pBT, uSize);
							
							if (pNeighbour==IMG_NULL)
							{
								PVR_DPF ((PVR_DBG_ERROR,"_AttemptAllocAligned: Back split failed"));
								
								_FreeListInsert (pArena, pBT);
								return IMG_FALSE;
							}

							_FreeListInsert (pArena, pNeighbour);
	#ifdef RA_STATS
							pArena->sStatistics.uFreeSegmentCount++;
							pArena->sStatistics.uFreeResourceCount+=pNeighbour->uSize;
	#endif
						}

						pBT->type = btt_live;

#if defined(VALIDATE_ARENA_TEST)
						if (pBT->eResourceType == IMPORTED_RESOURCE_TYPE)
						{
							pBT->eResourceSpan = IMPORTED_RESOURCE_SPAN_LIVE;
						}
						else if (pBT->eResourceType == NON_IMPORTED_RESOURCE_TYPE)
						{
							pBT->eResourceSpan = RESOURCE_SPAN_LIVE;
						}
						else
						{
							PVR_DPF ((PVR_DBG_ERROR,"_AttemptAllocAligned ERROR: pBT->eResourceType unrecognized"));
							PVR_DBG_BREAK;
						}
#endif
						if (!HASH_Insert (pArena->pSegmentHash, pBT->base, (IMG_UINTPTR_T) pBT))
						{
							_FreeBT (pArena, pBT, IMG_FALSE);
							return IMG_FALSE;
						}

						if (ppsMapping!=IMG_NULL)
							*ppsMapping = pBT->psMapping;

						*base = pBT->base;

						return IMG_TRUE;
					}
					else
					{
						PVR_DPF ((PVR_DBG_MESSAGE,
								"AttemptAllocAligned: mismatch in flags. Import has %x, request was %x", pBT->psMapping->ui32Flags, uFlags));

					}
				}
				pBT = pBT->pNextFree;
			}

		}
		uIndex++;
	}

	return IMG_FALSE;
}



RA_ARENA *
RA_Create (IMG_CHAR *name,
		   IMG_UINTPTR_T base,
		   IMG_SIZE_T uSize,
		   BM_MAPPING *psMapping,
		   IMG_SIZE_T uQuantum,
		   IMG_BOOL (*imp_alloc)(IMG_VOID *, IMG_SIZE_T uSize, IMG_SIZE_T *pActualSize,
		                     BM_MAPPING **ppsMapping, IMG_UINT32 _flags, IMG_UINTPTR_T *pBase),
		   IMG_VOID (*imp_free) (IMG_VOID *, IMG_UINTPTR_T, BM_MAPPING *),
		   IMG_VOID (*backingstore_free) (IMG_VOID*, IMG_SIZE_T, IMG_SIZE_T, IMG_HANDLE),
		   IMG_VOID *pImportHandle)
{
	RA_ARENA *pArena;
	BT *pBT;
	IMG_INT i;

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "RA_Create: name='%s', base=0x%x, uSize=0x%x, alloc=0x%x, free=0x%x",
			  name, base, uSize, (IMG_UINTPTR_T)imp_alloc, (IMG_UINTPTR_T)imp_free));


	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
					 sizeof (*pArena),
					 (IMG_VOID **)&pArena, IMG_NULL,
					 "Resource Arena") != PVRSRV_OK)
	{
		goto arena_fail;
	}

	pArena->name = name;
	pArena->pImportAlloc = (imp_alloc!=IMG_NULL) ? imp_alloc : &_RequestAllocFail;
	pArena->pImportFree = imp_free;
	pArena->pBackingStoreFree = backingstore_free;
	pArena->pImportHandle = pImportHandle;
	for (i=0; i<FREE_TABLE_LIMIT; i++)
		pArena->aHeadFree[i] = IMG_NULL;
	pArena->pHeadSegment = IMG_NULL;
	pArena->pTailSegment = IMG_NULL;
	pArena->uQuantum = uQuantum;

#ifdef RA_STATS
	pArena->sStatistics.uSpanCount = 0;
	pArena->sStatistics.uLiveSegmentCount = 0;
	pArena->sStatistics.uFreeSegmentCount = 0;
	pArena->sStatistics.uFreeResourceCount = 0;
	pArena->sStatistics.uTotalResourceCount = 0;
	pArena->sStatistics.uCumulativeAllocs = 0;
	pArena->sStatistics.uCumulativeFrees = 0;
	pArena->sStatistics.uImportCount = 0;
	pArena->sStatistics.uExportCount = 0;
#endif

#if defined(CONFIG_PROC_FS) && defined(DEBUG)
	if(strcmp(pArena->name,"") != 0)
	{
		IMG_INT ret;
		IMG_CHAR szProcInfoName[PROC_NAME_SIZE];
		IMG_CHAR szProcSegsName[PROC_NAME_SIZE];
		struct proc_dir_entry* (*pfnCreateProcEntrySeq)(const IMG_CHAR *,
										 IMG_VOID*,
										 pvr_next_proc_seq_t,
										 pvr_show_proc_seq_t,
										 pvr_off2element_proc_seq_t,
										 pvr_startstop_proc_seq_t,
										 write_proc_t);

		pArena->bInitProcEntry = !PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL);

		
		pfnCreateProcEntrySeq = pArena->bInitProcEntry ? CreateProcEntrySeq : CreatePerProcessProcEntrySeq;

		ret = snprintf(szProcInfoName, sizeof(szProcInfoName), "ra_info_%s", pArena->name);
		if (ret > 0 && ret < sizeof(szProcInfoName))
		{
			pArena->pProcInfo =  pfnCreateProcEntrySeq(ReplaceSpaces(szProcInfoName), pArena, NULL,
											 RA_ProcSeqShowInfo, RA_ProcSeqOff2ElementInfo, NULL, NULL);
		}
		else
		{
			pArena->pProcInfo = 0;
			PVR_DPF((PVR_DBG_ERROR, "RA_Create: couldn't create ra_info proc entry for arena %s", pArena->name));
		}

		ret = snprintf(szProcSegsName, sizeof(szProcSegsName), "ra_segs_%s", pArena->name);
		if (ret > 0 && ret < sizeof(szProcInfoName))
		{
			pArena->pProcSegs = pfnCreateProcEntrySeq(ReplaceSpaces(szProcSegsName), pArena, NULL,
											 RA_ProcSeqShowRegs, RA_ProcSeqOff2ElementRegs, NULL, NULL);
		}
		else
		{
			pArena->pProcSegs = 0;
			PVR_DPF((PVR_DBG_ERROR, "RA_Create: couldn't create ra_segs proc entry for arena %s", pArena->name));
		}
	}
#endif 

	pArena->pSegmentHash = HASH_Create (MINIMUM_HASH_SIZE);
	if (pArena->pSegmentHash==IMG_NULL)
	{
		goto hash_fail;
	}
	if (uSize>0)
	{
		uSize = (uSize + uQuantum - 1) / uQuantum * uQuantum;
		pBT = _InsertResource (pArena, base, uSize);
		if (pBT == IMG_NULL)
		{
			goto insert_fail;
		}
		pBT->psMapping = psMapping;

	}
	return pArena;

insert_fail:
	HASH_Delete (pArena->pSegmentHash);
hash_fail:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(RA_ARENA), pArena, IMG_NULL);
	
arena_fail:
	return IMG_NULL;
}

IMG_VOID
RA_Delete (RA_ARENA *pArena)
{
	IMG_UINT32 uIndex;

	PVR_ASSERT(pArena != IMG_NULL);

	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"RA_Delete: invalid parameter - pArena"));
		return;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "RA_Delete: name='%s'", pArena->name));

	for (uIndex=0; uIndex<FREE_TABLE_LIMIT; uIndex++)
		pArena->aHeadFree[uIndex] = IMG_NULL;

	while (pArena->pHeadSegment != IMG_NULL)
	{
		BT *pBT = pArena->pHeadSegment;

		if (pBT->type != btt_free)
		{
			PVR_DPF ((PVR_DBG_ERROR,"RA_Delete: allocations still exist in the arena that is being destroyed"));
			PVR_DPF ((PVR_DBG_ERROR,"Likely Cause: client drivers not freeing alocations before destroying devmemcontext"));
			PVR_DPF ((PVR_DBG_ERROR,"RA_Delete: base = 0x%x size=0x%x", pBT->base, pBT->uSize));
		}

		_SegmentListRemove (pArena, pBT);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(BT), pBT, IMG_NULL);
		
#ifdef RA_STATS
		pArena->sStatistics.uSpanCount--;
#endif
	}
#if defined(CONFIG_PROC_FS) && defined(DEBUG)
	{
		IMG_VOID (*pfnRemoveProcEntrySeq)(struct proc_dir_entry*);

		pfnRemoveProcEntrySeq = pArena->bInitProcEntry ? RemoveProcEntrySeq : RemovePerProcessProcEntrySeq;

		if (pArena->pProcInfo != 0)
		{
			pfnRemoveProcEntrySeq( pArena->pProcInfo );
		}

		if (pArena->pProcSegs != 0)
		{
			pfnRemoveProcEntrySeq( pArena->pProcSegs );
		}
	}
#endif
	HASH_Delete (pArena->pSegmentHash);
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(RA_ARENA), pArena, IMG_NULL);
	
}

IMG_BOOL
RA_TestDelete (RA_ARENA *pArena)
{
	PVR_ASSERT(pArena != IMG_NULL);

	if (pArena != IMG_NULL)
	{
		while (pArena->pHeadSegment != IMG_NULL)
		{
			BT *pBT = pArena->pHeadSegment;
			if (pBT->type != btt_free)
			{
				PVR_DPF ((PVR_DBG_ERROR,"RA_TestDelete: detected resource leak!"));
				PVR_DPF ((PVR_DBG_ERROR,"RA_TestDelete: base = 0x%x size=0x%x", pBT->base, pBT->uSize));
				return IMG_FALSE;
			}
		}
	}

	return IMG_TRUE;
}

IMG_BOOL
RA_Add (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_SIZE_T uSize)
{
	PVR_ASSERT (pArena != IMG_NULL);

	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"RA_Add: invalid parameter - pArena"));
		return IMG_FALSE;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "RA_Add: name='%s', base=0x%x, size=0x%x", pArena->name, base, uSize));

	uSize = (uSize + pArena->uQuantum - 1) / pArena->uQuantum * pArena->uQuantum;
	return ((IMG_BOOL)(_InsertResource (pArena, base, uSize) != IMG_NULL));
}

IMG_BOOL
RA_Alloc (RA_ARENA *pArena,
		  IMG_SIZE_T uRequestSize,
		  IMG_SIZE_T *pActualSize,
		  BM_MAPPING **ppsMapping,
		  IMG_UINT32 uFlags,
		  IMG_UINT32 uAlignment,
		  IMG_UINT32 uAlignmentOffset,
		  IMG_UINTPTR_T *base)
{
	IMG_BOOL bResult;
	IMG_SIZE_T uSize = uRequestSize;

	PVR_ASSERT (pArena!=IMG_NULL);

	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"RA_Alloc: invalid parameter - pArena"));
		return IMG_FALSE;
	}

#if defined(VALIDATE_ARENA_TEST)
	ValidateArena(pArena);
#endif

#ifdef USE_BM_FREESPACE_CHECK
	CheckBMFreespace();
#endif

	if (pActualSize != IMG_NULL)
	{
		*pActualSize = uSize;
	}

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "RA_Alloc: arena='%s', size=0x%x(0x%x), alignment=0x%x, offset=0x%x",
		   pArena->name, uSize, uRequestSize, uAlignment, uAlignmentOffset));

	

	bResult = _AttemptAllocAligned (pArena, uSize, ppsMapping, uFlags,
									uAlignment, uAlignmentOffset, base);
	if (!bResult)
	{
		BM_MAPPING *psImportMapping;
		IMG_UINTPTR_T import_base;
		IMG_SIZE_T uImportSize = uSize;

		


		if (uAlignment > pArena->uQuantum)
		{
			uImportSize += (uAlignment - 1);
		}

		
		uImportSize = ((uImportSize + pArena->uQuantum - 1)/pArena->uQuantum)*pArena->uQuantum;

		bResult =
			pArena->pImportAlloc (pArena->pImportHandle, uImportSize, &uImportSize,
								 &psImportMapping, uFlags, &import_base);
		if (bResult)
		{
			BT *pBT;
			pBT = _InsertResourceSpan (pArena, import_base, uImportSize);
			
			if (pBT == IMG_NULL)
			{
				
				pArena->pImportFree(pArena->pImportHandle, import_base,
									psImportMapping);
				PVR_DPF ((PVR_DBG_MESSAGE,
						  "RA_Alloc: name='%s', size=0x%x failed!",
						  pArena->name, uSize));
				
				return IMG_FALSE;
			}
			pBT->psMapping = psImportMapping;
#ifdef RA_STATS
			pArena->sStatistics.uFreeSegmentCount++;
			pArena->sStatistics.uFreeResourceCount += uImportSize;
			pArena->sStatistics.uImportCount++;
			pArena->sStatistics.uSpanCount++;
#endif
			bResult = _AttemptAllocAligned(pArena, uSize, ppsMapping, uFlags,
										   uAlignment, uAlignmentOffset,
										   base);
			if (!bResult)
			{
				PVR_DPF ((PVR_DBG_MESSAGE,
						  "RA_Alloc: name='%s' uAlignment failed!",
						  pArena->name));
			}
		}
	}
#ifdef RA_STATS
	if (bResult)
		pArena->sStatistics.uCumulativeAllocs++;
#endif

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "RA_Alloc: name='%s', size=0x%x, *base=0x%x = %d",
			  pArena->name, uSize, *base, bResult));

	

#if defined(VALIDATE_ARENA_TEST)
	ValidateArena(pArena);
#endif

	return bResult;
}


#if defined(VALIDATE_ARENA_TEST)

IMG_UINT32 ValidateArena(RA_ARENA *pArena)
{
	BT* pSegment;
	RESOURCE_DESCRIPTOR eNextSpan;

	pSegment = pArena->pHeadSegment;

	if (pSegment == IMG_NULL)
	{
		return 0;
	}

	if (pSegment->eResourceType == IMPORTED_RESOURCE_TYPE)
	{
		PVR_ASSERT(pSegment->eResourceSpan == IMPORTED_RESOURCE_SPAN_START);

		while (pSegment->pNextSegment)
		{
			eNextSpan = pSegment->pNextSegment->eResourceSpan;

			switch (pSegment->eResourceSpan)
			{
				case IMPORTED_RESOURCE_SPAN_LIVE:

					if (!((eNextSpan == IMPORTED_RESOURCE_SPAN_LIVE) ||
						  (eNextSpan == IMPORTED_RESOURCE_SPAN_FREE) ||
						  (eNextSpan == IMPORTED_RESOURCE_SPAN_END)))
					{
						
						PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

						PVR_DBG_BREAK;
					}
				break;

				case IMPORTED_RESOURCE_SPAN_FREE:

					if (!((eNextSpan == IMPORTED_RESOURCE_SPAN_LIVE) ||
						  (eNextSpan == IMPORTED_RESOURCE_SPAN_END)))
					{
						
						PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

						PVR_DBG_BREAK;
					}
				break;

				case IMPORTED_RESOURCE_SPAN_END:

					if ((eNextSpan == IMPORTED_RESOURCE_SPAN_LIVE) ||
						(eNextSpan == IMPORTED_RESOURCE_SPAN_FREE) ||
						(eNextSpan == IMPORTED_RESOURCE_SPAN_END))
					{
						
						PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

						PVR_DBG_BREAK;
					}
				break;


				case IMPORTED_RESOURCE_SPAN_START:

					if (!((eNextSpan == IMPORTED_RESOURCE_SPAN_LIVE) ||
						  (eNextSpan == IMPORTED_RESOURCE_SPAN_FREE)))
					{
						
						PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

						PVR_DBG_BREAK;
					}
				break;

				default:
					PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

					PVR_DBG_BREAK;
				break;
			}
			pSegment = pSegment->pNextSegment;
		}
	}
	else if (pSegment->eResourceType == NON_IMPORTED_RESOURCE_TYPE)
	{
		PVR_ASSERT((pSegment->eResourceSpan == RESOURCE_SPAN_FREE) || (pSegment->eResourceSpan == RESOURCE_SPAN_LIVE));

		while (pSegment->pNextSegment)
		{
			eNextSpan = pSegment->pNextSegment->eResourceSpan;

			switch (pSegment->eResourceSpan)
			{
				case RESOURCE_SPAN_LIVE:

					if (!((eNextSpan == RESOURCE_SPAN_FREE) ||
						  (eNextSpan == RESOURCE_SPAN_LIVE)))
					{
						
						PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

						PVR_DBG_BREAK;
					}
				break;

				case RESOURCE_SPAN_FREE:

					if (!((eNextSpan == RESOURCE_SPAN_FREE) ||
						  (eNextSpan == RESOURCE_SPAN_LIVE)))
					{
						
						PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

						PVR_DBG_BREAK;
					}
				break;

				default:
					PVR_DPF((PVR_DBG_ERROR, "ValidateArena ERROR: adjacent boundary tags %d (base=0x%x) and %d (base=0x%x) are incompatible (arena: %s)",
								pSegment->ui32BoundaryTagID, pSegment->base, pSegment->pNextSegment->ui32BoundaryTagID, pSegment->pNextSegment->base, pArena->name));

					PVR_DBG_BREAK;
				break;
			}
			pSegment = pSegment->pNextSegment;
		}

	}
	else
	{
		PVR_DPF ((PVR_DBG_ERROR,"ValidateArena ERROR: pSegment->eResourceType unrecognized"));

		PVR_DBG_BREAK;
	}

	return 0;
}

#endif


IMG_VOID
RA_Free (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_BOOL bFreeBackingStore)
{
	BT *pBT;

	PVR_ASSERT (pArena != IMG_NULL);

	if (pArena == IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_ERROR,"RA_Free: invalid parameter - pArena"));
		return;
	}

#ifdef USE_BM_FREESPACE_CHECK
	CheckBMFreespace();
#endif

	PVR_DPF ((PVR_DBG_MESSAGE,
			  "RA_Free: name='%s', base=0x%x", pArena->name, base));

	pBT = (BT *) HASH_Remove (pArena->pSegmentHash, base);
	PVR_ASSERT (pBT != IMG_NULL);

	if (pBT)
	{
		PVR_ASSERT (pBT->base == base);

#ifdef RA_STATS
		pArena->sStatistics.uCumulativeFrees++;
#endif

#ifdef USE_BM_FREESPACE_CHECK
{
	IMG_BYTE* p;
	IMG_BYTE* endp;

	p = (IMG_BYTE*)pBT->base + SysGetDevicePhysOffset();
	endp = (IMG_BYTE*)((IMG_UINT32)(p + pBT->uSize));
	while ((IMG_UINT32)p & 3)
	{
		*p++ = 0xAA;
	}
	while (p < (IMG_BYTE*)((IMG_UINT32)endp & 0xfffffffc))
	{
		*(IMG_UINT32*)p = 0xAAAAAAAA;
		p += sizeof(IMG_UINT32);
	}
	while (p < endp)
	{
		*p++ = 0xAA;
	}
	PVR_DPF((PVR_DBG_MESSAGE,"BM_FREESPACE_CHECK: RA_Free Cleared %08X to %08X (size=0x%x)",(IMG_BYTE*)pBT->base + SysGetDevicePhysOffset(),endp-1,pBT->uSize));
}
#endif
		_FreeBT (pArena, pBT, bFreeBackingStore);
	}
}


IMG_BOOL RA_GetNextLiveSegment(IMG_HANDLE hArena, RA_SEGMENT_DETAILS *psSegDetails)
{
	BT        *pBT;

	if (psSegDetails->hSegment)
	{
		pBT = (BT *)psSegDetails->hSegment;
	}
	else
	{
		RA_ARENA *pArena = (RA_ARENA *)hArena;

		pBT = pArena->pHeadSegment;
	}
	
	while (pBT != IMG_NULL)
	{
		if (pBT->type == btt_live)
		{
			psSegDetails->uiSize = pBT->uSize;
			psSegDetails->sCpuPhyAddr.uiAddr = pBT->base;
			psSegDetails->hSegment = (IMG_HANDLE)pBT->pNextSegment;

			return IMG_TRUE;
		}

		pBT = pBT->pNextSegment;
	}

	psSegDetails->uiSize = 0;
	psSegDetails->sCpuPhyAddr.uiAddr = 0;
	psSegDetails->hSegment = (IMG_HANDLE)IMG_UNDEF;

	return IMG_FALSE;
}


#ifdef USE_BM_FREESPACE_CHECK
RA_ARENA* pJFSavedArena = IMG_NULL;

IMG_VOID CheckBMFreespace(IMG_VOID)
{
	BT *pBT;
	IMG_BYTE* p;
	IMG_BYTE* endp;

	if (pJFSavedArena != IMG_NULL)
	{
		for (pBT=pJFSavedArena->pHeadSegment; pBT!=IMG_NULL; pBT=pBT->pNextSegment)
		{
			if (pBT->type == btt_free)
			{
				p = (IMG_BYTE*)pBT->base + SysGetDevicePhysOffset();
				endp = (IMG_BYTE*)((IMG_UINT32)(p + pBT->uSize) & 0xfffffffc);

				while ((IMG_UINT32)p & 3)
				{
					if (*p++ != 0xAA)
					{
						fprintf(stderr,"BM_FREESPACE_CHECK: Blank space at %08X has changed to 0x%x\n",p,*(IMG_UINT32*)p);
						for (;;);
						break;
					}
				}
				while (p < endp)
				{
					if (*(IMG_UINT32*)p != 0xAAAAAAAA)
					{
						fprintf(stderr,"BM_FREESPACE_CHECK: Blank space at %08X has changed to 0x%x\n",p,*(IMG_UINT32*)p);
						for (;;);
						break;
					}
					p += 4;
				}
			}
		}
	}
}
#endif


#if (defined(CONFIG_PROC_FS) && defined(DEBUG)) || defined (RA_STATS)
static IMG_CHAR *
_BTType (IMG_INT eType)
{
	switch (eType)
	{
	case btt_span: return "span";
	case btt_free: return "free";
	case btt_live: return "live";
	}
	return "junk";
}
#endif 

#if defined(ENABLE_RA_DUMP)
IMG_VOID
RA_Dump (RA_ARENA *pArena)
{
	BT *pBT;
	PVR_ASSERT (pArena != IMG_NULL);
	PVR_DPF ((PVR_DBG_MESSAGE,"Arena '%s':", pArena->name));
	PVR_DPF ((PVR_DBG_MESSAGE,"  alloc=%08X free=%08X handle=%08X quantum=%d",
			 pArena->pImportAlloc, pArena->pImportFree, pArena->pImportHandle,
			 pArena->uQuantum));
	PVR_DPF ((PVR_DBG_MESSAGE,"  segment Chain:"));
	if (pArena->pHeadSegment != IMG_NULL &&
	    pArena->pHeadSegment->pPrevSegment != IMG_NULL)
		PVR_DPF ((PVR_DBG_MESSAGE,"  error: head boundary tag has invalid pPrevSegment"));
	if (pArena->pTailSegment != IMG_NULL &&
	    pArena->pTailSegment->pNextSegment != IMG_NULL)
		PVR_DPF ((PVR_DBG_MESSAGE,"  error: tail boundary tag has invalid pNextSegment"));

	for (pBT=pArena->pHeadSegment; pBT!=IMG_NULL; pBT=pBT->pNextSegment)
	{
		PVR_DPF ((PVR_DBG_MESSAGE,"\tbase=0x%x size=0x%x type=%s ref=%08X",
				 (IMG_UINT32) pBT->base, pBT->uSize, _BTType (pBT->type),
				 pBT->pRef));
	}

#ifdef HASH_TRACE
	HASH_Dump (pArena->pSegmentHash);
#endif
}
#endif 


#if defined(CONFIG_PROC_FS) && defined(DEBUG)


static void RA_ProcSeqShowInfo(struct seq_file *sfile, void* el)
{
	PVR_PROC_SEQ_HANDLERS *handlers = (PVR_PROC_SEQ_HANDLERS*)sfile->private;
	RA_ARENA *pArena = (RA_ARENA *)handlers->data;
	IMG_INT off = (IMG_INT)el;

	switch (off)
	{
	case 1:
		seq_printf(sfile, "quantum\t\t\t%u\n", pArena->uQuantum);
		break;
	case 2:
		seq_printf(sfile, "import_handle\t\t%08X\n", (IMG_UINT)pArena->pImportHandle);
		break;
#ifdef RA_STATS
	case 3:
		seq_printf(sfile,"span count\t\t%u\n", pArena->sStatistics.uSpanCount);
		break;
	case 4:
		seq_printf(sfile, "live segment count\t%u\n", pArena->sStatistics.uLiveSegmentCount);
		break;
	case 5:
		seq_printf(sfile, "free segment count\t%u\n", pArena->sStatistics.uFreeSegmentCount);
		break;
	case 6:
		seq_printf(sfile, "free resource count\t%u (0x%x)\n",
							pArena->sStatistics.uFreeResourceCount,
							(IMG_UINT)pArena->sStatistics.uFreeResourceCount);
		break;
	case 7:
		seq_printf(sfile, "total allocs\t\t%u\n", pArena->sStatistics.uCumulativeAllocs);
		break;
	case 8:
		seq_printf(sfile, "total frees\t\t%u\n", pArena->sStatistics.uCumulativeFrees);
		break;
	case 9:
		seq_printf(sfile, "import count\t\t%u\n", pArena->sStatistics.uImportCount);
		break;
	case 10:
		seq_printf(sfile, "export count\t\t%u\n", pArena->sStatistics.uExportCount);
		break;
#endif
	}

}

static void* RA_ProcSeqOff2ElementInfo(struct seq_file * sfile, loff_t off)
{
#ifdef RA_STATS
	if(off <= 9)
#else
	if(off <= 1)
#endif
		return (void*)(IMG_INT)(off+1);
	return 0;
}

static void RA_ProcSeqShowRegs(struct seq_file *sfile, void* el)
{
	PVR_PROC_SEQ_HANDLERS *handlers = (PVR_PROC_SEQ_HANDLERS*)sfile->private;
	RA_ARENA *pArena = (RA_ARENA *)handlers->data;
	BT *pBT = (BT*)el;

	if (el == PVR_PROC_SEQ_START_TOKEN)
	{
		seq_printf(sfile, "Arena \"%s\"\nBase         Size Type Ref\n", pArena->name);
		return;
	}

	if (pBT)
	{
		seq_printf(sfile, "%08x %8x %4s %08x\n",
				   (IMG_UINT)pBT->base, (IMG_UINT)pBT->uSize, _BTType (pBT->type),
			       (IMG_UINT)pBT->psMapping);
	}
}

static void* RA_ProcSeqOff2ElementRegs(struct seq_file * sfile, loff_t off)
{
	PVR_PROC_SEQ_HANDLERS *handlers = (PVR_PROC_SEQ_HANDLERS*)sfile->private;
	RA_ARENA *pArena = (RA_ARENA *)handlers->data;
	BT *pBT = 0;

	if(off == 0)
		return PVR_PROC_SEQ_START_TOKEN;

	for (pBT=pArena->pHeadSegment; --off && pBT; pBT=pBT->pNextSegment);

	return (void*)pBT;
}

#endif 


#ifdef RA_STATS
PVRSRV_ERROR RA_GetStats(RA_ARENA *pArena,
							IMG_CHAR **ppszStr,
							IMG_UINT32 *pui32StrLen)
{
	IMG_CHAR 	*pszStr = *ppszStr;
	IMG_UINT32 	ui32StrLen = *pui32StrLen;
	IMG_INT32	i32Count;
	BT 			*pBT;

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "\nArena '%s':\n", pArena->name);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);


	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "  allocCB=%p freeCB=%p handle=%p quantum=%d\n",
							 pArena->pImportAlloc,
							 pArena->pImportFree,
							 pArena->pImportHandle,
							 pArena->uQuantum);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "span count\t\t%u\n", pArena->sStatistics.uSpanCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "live segment count\t%u\n", pArena->sStatistics.uLiveSegmentCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "free segment count\t%u\n", pArena->sStatistics.uFreeSegmentCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "free resource count\t%u (0x%x)\n",
							pArena->sStatistics.uFreeResourceCount,
							(IMG_UINT)pArena->sStatistics.uFreeResourceCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "total allocs\t\t%u\n", pArena->sStatistics.uCumulativeAllocs);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "total frees\t\t%u\n", pArena->sStatistics.uCumulativeFrees);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "import count\t\t%u\n", pArena->sStatistics.uImportCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "export count\t\t%u\n", pArena->sStatistics.uExportCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "  segment Chain:\n");
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	if (pArena->pHeadSegment != IMG_NULL &&
	    pArena->pHeadSegment->pPrevSegment != IMG_NULL)
	{
		CHECK_SPACE(ui32StrLen);
		i32Count = OSSNPrintf(pszStr, 100, "  error: head boundary tag has invalid pPrevSegment\n");
		UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	}

	if (pArena->pTailSegment != IMG_NULL &&
	    pArena->pTailSegment->pNextSegment != IMG_NULL)
	{
		CHECK_SPACE(ui32StrLen);
		i32Count = OSSNPrintf(pszStr, 100, "  error: tail boundary tag has invalid pNextSegment\n");
		UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	}

	for (pBT=pArena->pHeadSegment; pBT!=IMG_NULL; pBT=pBT->pNextSegment)
	{
		CHECK_SPACE(ui32StrLen);
		i32Count = OSSNPrintf(pszStr, 100, "\tbase=0x%x size=0x%x type=%s ref=%p\n",
											 (IMG_UINT32) pBT->base,
											 pBT->uSize,
											 _BTType(pBT->type),
											 pBT->psMapping);
		UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	}

	*ppszStr = pszStr;
	*pui32StrLen = ui32StrLen;

	return PVRSRV_OK;
}

PVRSRV_ERROR RA_GetStatsFreeMem(RA_ARENA *pArena,
								IMG_CHAR **ppszStr, 
								IMG_UINT32 *pui32StrLen)
{
	IMG_CHAR 	*pszStr = *ppszStr;
	IMG_UINT32 	ui32StrLen = *pui32StrLen;
	IMG_INT32	i32Count;
	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "Bytes free: Arena %-30s: %u (0x%x)\n", pArena->name,
		pArena->sStatistics.uFreeResourceCount,
		pArena->sStatistics.uFreeResourceCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	*ppszStr = pszStr;
	*pui32StrLen = ui32StrLen;
	
	return PVRSRV_OK;
}
#endif

