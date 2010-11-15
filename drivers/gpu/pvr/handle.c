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

#ifdef	PVR_SECURE_HANDLES
#include <stddef.h>

#include "services_headers.h"
#include "handle.h"

#ifdef	DEBUG
#define	HANDLE_BLOCK_SHIFT	2
#else
#define	HANDLE_BLOCK_SHIFT	8
#endif

#define	DIVIDE_BY_BLOCK_SIZE(i)		(((IMG_UINT32)(i)) >> HANDLE_BLOCK_SHIFT)
#define	MULTIPLY_BY_BLOCK_SIZE(i)	(((IMG_UINT32)(i)) << HANDLE_BLOCK_SHIFT)

#define HANDLE_BLOCK_SIZE       MULTIPLY_BY_BLOCK_SIZE(1)
#define	HANDLE_SUB_BLOCK_MASK	(HANDLE_BLOCK_SIZE - 1)
#define	HANDLE_BLOCK_MASK	(~(HANDLE_SUB_BLOCK_MASK))

#define	HANDLE_HASH_TAB_INIT_SIZE	32

#define	INDEX_IS_VALID(psBase, i) ((i) < (psBase)->ui32TotalHandCount)

#define	INDEX_TO_HANDLE(i) ((IMG_HANDLE)((i) + 1))
#define	HANDLE_TO_INDEX(h) ((IMG_UINT32)(h) - 1)

#define	INDEX_TO_BLOCK_INDEX(i)		DIVIDE_BY_BLOCK_SIZE(i)
#define BLOCK_INDEX_TO_INDEX(i)		MULTIPLY_BY_BLOCK_SIZE(i)
#define INDEX_TO_SUB_BLOCK_INDEX(i)	((i) & HANDLE_SUB_BLOCK_MASK)

#define INDEX_TO_INDEX_STRUCT_PTR(psArray, i) (&((psArray)[INDEX_TO_BLOCK_INDEX(i)]))
#define	BASE_AND_INDEX_TO_INDEX_STRUCT_PTR(psBase, i) INDEX_TO_INDEX_STRUCT_PTR((psBase)->psHandleArray, i)

#define	INDEX_TO_FREE_HAND_BLOCK_COUNT(psBase, i) (BASE_AND_INDEX_TO_INDEX_STRUCT_PTR(psBase, i)->ui32FreeHandBlockCount)

#define INDEX_TO_HANDLE_STRUCT_PTR(psBase, i) (BASE_AND_INDEX_TO_INDEX_STRUCT_PTR(psBase, i)->psHandle + INDEX_TO_SUB_BLOCK_INDEX(i))

#define	HANDLE_TO_HANDLE_STRUCT_PTR(psBase, h) (INDEX_TO_HANDLE_STRUCT_PTR(psBase, HANDLE_TO_INDEX(h)))

#define	HANDLE_PTR_TO_INDEX(psHandle) ((psHandle)->ui32Index)
#define	HANDLE_PTR_TO_HANDLE(psHandle) INDEX_TO_HANDLE(HANDLE_PTR_TO_INDEX(psHandle))

#define	ROUND_DOWN_TO_MULTIPLE_OF_BLOCK_SIZE(a) (HANDLE_BLOCK_MASK & (a))
#define	ROUND_UP_TO_MULTIPLE_OF_BLOCK_SIZE(a) ROUND_DOWN_TO_MULTIPLE_OF_BLOCK_SIZE((a) + HANDLE_BLOCK_SIZE - 1)

#define	DEFAULT_MAX_HANDLE		0x7fffffffu
#define	DEFAULT_MAX_INDEX_PLUS_ONE	ROUND_DOWN_TO_MULTIPLE_OF_BLOCK_SIZE(DEFAULT_MAX_HANDLE)

#define	HANDLES_BATCHED(psBase) ((psBase)->ui32HandBatchSize != 0)

#define HANDLE_ARRAY_SIZE(handleCount) DIVIDE_BY_BLOCK_SIZE(ROUND_UP_TO_MULTIPLE_OF_BLOCK_SIZE(handleCount))

#define	SET_FLAG(v, f) ((IMG_VOID)((v) |= (f)))
#define	CLEAR_FLAG(v, f) ((IMG_VOID)((v) &= ~(f)))
#define	TEST_FLAG(v, f) ((IMG_BOOL)(((v) & (f)) != 0))

#define	TEST_ALLOC_FLAG(psHandle, f) TEST_FLAG((psHandle)->eFlag, f)

#define	SET_INTERNAL_FLAG(psHandle, f) SET_FLAG((psHandle)->eInternalFlag, f)
#define	CLEAR_INTERNAL_FLAG(psHandle, f) CLEAR_FLAG((psHandle)->eInternalFlag, f)
#define	TEST_INTERNAL_FLAG(psHandle, f) TEST_FLAG((psHandle)->eInternalFlag, f)

#define	BATCHED_HANDLE(psHandle) TEST_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED)

#define	SET_BATCHED_HANDLE(psHandle) SET_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED)

#define	SET_UNBATCHED_HANDLE(psHandle) CLEAR_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED)

#define	BATCHED_HANDLE_PARTIALLY_FREE(psHandle) TEST_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED_PARTIALLY_FREE)

#define SET_BATCHED_HANDLE_PARTIALLY_FREE(psHandle) SET_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED_PARTIALLY_FREE)

#define	HANDLE_STRUCT_IS_FREE(psHandle) ((psHandle)->eType == PVRSRV_HANDLE_TYPE_NONE && (psHandle)->eInternalFlag == INTERNAL_HANDLE_FLAG_NONE)

#ifdef	MIN
#undef MIN
#endif

#define	MIN(x, y) (((x) < (y)) ? (x) : (y))

struct sHandleList
{
	IMG_UINT32 ui32Prev;
	IMG_UINT32 ui32Next;
	IMG_HANDLE hParent;
};

enum ePVRSRVInternalHandleFlag
{
	INTERNAL_HANDLE_FLAG_NONE = 0x00,
	INTERNAL_HANDLE_FLAG_BATCHED = 0x01,
	INTERNAL_HANDLE_FLAG_BATCHED_PARTIALLY_FREE = 0x02,
};

struct sHandle
{
	
	PVRSRV_HANDLE_TYPE eType;

	
	IMG_VOID *pvData;

	
	IMG_UINT32 ui32NextIndexPlusOne;

	
	enum ePVRSRVInternalHandleFlag eInternalFlag;

	
	PVRSRV_HANDLE_ALLOC_FLAG eFlag;

	
	IMG_UINT32 ui32Index;

	
	struct sHandleList sChildren;

	
	struct sHandleList sSiblings;
};

struct sHandleIndex
{
	
	struct sHandle *psHandle;

	
	IMG_HANDLE hBlockAlloc;

	
	IMG_UINT32 ui32FreeHandBlockCount;
};

struct _PVRSRV_HANDLE_BASE_
{
	
	IMG_HANDLE hBaseBlockAlloc;

	
	IMG_HANDLE hArrayBlockAlloc;

	
	struct sHandleIndex *psHandleArray;

	
	HASH_TABLE *psHashTab;

	
	IMG_UINT32 ui32FreeHandCount;

	
	IMG_UINT32 ui32FirstFreeIndex;

	
	IMG_UINT32 ui32MaxIndexPlusOne;

	
	IMG_UINT32 ui32TotalHandCount;

	
	IMG_UINT32 ui32LastFreeIndexPlusOne;

	
	IMG_UINT32 ui32HandBatchSize;

	
	IMG_UINT32 ui32TotalHandCountPreBatch;

	
	IMG_UINT32 ui32FirstBatchIndexPlusOne;

	
	IMG_UINT32 ui32BatchHandAllocFailures;

	
	IMG_BOOL bPurgingEnabled;
};

enum eHandKey {
	HAND_KEY_DATA = 0,
	HAND_KEY_TYPE,
	HAND_KEY_PARENT,
	HAND_KEY_LEN			
};

PVRSRV_HANDLE_BASE *gpsKernelHandleBase = IMG_NULL;

typedef IMG_UINTPTR_T HAND_KEY[HAND_KEY_LEN];

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListInit)
#endif
static INLINE
IMG_VOID HandleListInit(IMG_UINT32 ui32Index, struct sHandleList *psList, IMG_HANDLE hParent)
{
	psList->ui32Next = ui32Index;
	psList->ui32Prev = ui32Index;
	psList->hParent = hParent;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(InitParentList)
#endif
static INLINE
IMG_VOID InitParentList(struct sHandle *psHandle)
{
	IMG_UINT32 ui32Parent = HANDLE_PTR_TO_INDEX(psHandle);

	HandleListInit(ui32Parent, &psHandle->sChildren, INDEX_TO_HANDLE(ui32Parent));
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(InitChildEntry)
#endif
static INLINE
IMG_VOID InitChildEntry(struct sHandle *psHandle)
{
	HandleListInit(HANDLE_PTR_TO_INDEX(psHandle), &psHandle->sSiblings, IMG_NULL);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListIsEmpty)
#endif
static INLINE
IMG_BOOL HandleListIsEmpty(IMG_UINT32 ui32Index, struct sHandleList *psList)
{
	IMG_BOOL bIsEmpty;

	bIsEmpty = (IMG_BOOL)(psList->ui32Next == ui32Index);

#ifdef	DEBUG
	{
		IMG_BOOL bIsEmpty2;

		bIsEmpty2 = (IMG_BOOL)(psList->ui32Prev == ui32Index);
		PVR_ASSERT(bIsEmpty == bIsEmpty2);
	}
#endif

	return bIsEmpty;
}

#ifdef DEBUG
#ifdef INLINE_IS_PRAGMA
#pragma inline(NoChildren)
#endif
static INLINE
IMG_BOOL NoChildren(struct sHandle *psHandle)
{
	PVR_ASSERT(psHandle->sChildren.hParent == HANDLE_PTR_TO_HANDLE(psHandle));

	return HandleListIsEmpty(HANDLE_PTR_TO_INDEX(psHandle), &psHandle->sChildren);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(NoParent)
#endif
static INLINE
IMG_BOOL NoParent(struct sHandle *psHandle)
{
	if (HandleListIsEmpty(HANDLE_PTR_TO_INDEX(psHandle), &psHandle->sSiblings))
	{
		PVR_ASSERT(psHandle->sSiblings.hParent == IMG_NULL);

		return IMG_TRUE;
	}
	else
	{
		PVR_ASSERT(psHandle->sSiblings.hParent != IMG_NULL);
	}
	return IMG_FALSE;
}
#endif 
#ifdef INLINE_IS_PRAGMA
#pragma inline(ParentHandle)
#endif
static INLINE
IMG_HANDLE ParentHandle(struct sHandle *psHandle)
{
	return psHandle->sSiblings.hParent;
}

#define	LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, i, p, po, eo) \
		((struct sHandleList *)((IMG_CHAR *)(INDEX_TO_HANDLE_STRUCT_PTR(psBase, i)) + (((i) == (p)) ? (po) : (eo))))

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListInsertBefore)
#endif
static INLINE
IMG_VOID HandleListInsertBefore(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32InsIndex, struct sHandleList *psIns, IMG_SIZE_T uiParentOffset, IMG_UINT32 ui32EntryIndex, struct sHandleList *psEntry, IMG_SIZE_T uiEntryOffset, IMG_UINT32 ui32ParentIndex)
{
	 
	struct sHandleList *psPrevIns = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, psIns->ui32Prev, ui32ParentIndex, uiParentOffset, uiEntryOffset);

	PVR_ASSERT(psEntry->hParent == IMG_NULL);
	PVR_ASSERT(ui32InsIndex == psPrevIns->ui32Next);
	PVR_ASSERT(LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, ui32ParentIndex, ui32ParentIndex, uiParentOffset, uiParentOffset)->hParent == INDEX_TO_HANDLE(ui32ParentIndex));

	psEntry->ui32Prev = psIns->ui32Prev;
	psIns->ui32Prev = ui32EntryIndex;
	psEntry->ui32Next = ui32InsIndex;
	psPrevIns->ui32Next = ui32EntryIndex;

	psEntry->hParent = INDEX_TO_HANDLE(ui32ParentIndex);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(AdoptChild)
#endif
static INLINE
IMG_VOID AdoptChild(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psParent, struct sHandle *psChild)
{
	IMG_UINT32 ui32Parent = HANDLE_TO_INDEX(psParent->sChildren.hParent);

	PVR_ASSERT(ui32Parent == HANDLE_PTR_TO_INDEX(psParent));

	HandleListInsertBefore(psBase, ui32Parent, &psParent->sChildren, offsetof(struct sHandle, sChildren), HANDLE_PTR_TO_INDEX(psChild), &psChild->sSiblings, offsetof(struct sHandle, sSiblings), ui32Parent);

}

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListRemove)
#endif
static INLINE
IMG_VOID HandleListRemove(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32EntryIndex, struct sHandleList *psEntry, IMG_SIZE_T uiEntryOffset, IMG_SIZE_T uiParentOffset)
{
	if (!HandleListIsEmpty(ui32EntryIndex, psEntry))
	{
		 
		struct sHandleList *psPrev = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, psEntry->ui32Prev, HANDLE_TO_INDEX(psEntry->hParent), uiParentOffset, uiEntryOffset);
		struct sHandleList *psNext = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, psEntry->ui32Next, HANDLE_TO_INDEX(psEntry->hParent), uiParentOffset, uiEntryOffset);

		
		PVR_ASSERT(psEntry->hParent != IMG_NULL);

		psPrev->ui32Next = psEntry->ui32Next;
		psNext->ui32Prev = psEntry->ui32Prev;

		HandleListInit(ui32EntryIndex, psEntry, IMG_NULL);
	}
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(UnlinkFromParent)
#endif
static INLINE
IMG_VOID UnlinkFromParent(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	HandleListRemove(psBase, HANDLE_PTR_TO_INDEX(psHandle), &psHandle->sSiblings, offsetof(struct sHandle, sSiblings), offsetof(struct sHandle, sChildren));
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListIterate)
#endif
static INLINE
PVRSRV_ERROR HandleListIterate(PVRSRV_HANDLE_BASE *psBase, struct sHandleList *psHead, IMG_SIZE_T uiParentOffset, IMG_SIZE_T uiEntryOffset, PVRSRV_ERROR (*pfnIterFunc)(PVRSRV_HANDLE_BASE *, struct sHandle *))
{
	IMG_UINT32 ui32Index;
	IMG_UINT32 ui32Parent = HANDLE_TO_INDEX(psHead->hParent);

	PVR_ASSERT(psHead->hParent != IMG_NULL);

	
	for(ui32Index = psHead->ui32Next; ui32Index != ui32Parent; )
	{
		struct sHandle *psHandle = INDEX_TO_HANDLE_STRUCT_PTR(psBase, ui32Index);
		 
		struct sHandleList *psEntry = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, ui32Index, ui32Parent, uiParentOffset, uiEntryOffset);
		PVRSRV_ERROR eError;

		PVR_ASSERT(psEntry->hParent == psHead->hParent);
		
		ui32Index = psEntry->ui32Next;

		eError = (*pfnIterFunc)(psBase, psHandle);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(IterateOverChildren)
#endif
static INLINE
PVRSRV_ERROR IterateOverChildren(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psParent, PVRSRV_ERROR (*pfnIterFunc)(PVRSRV_HANDLE_BASE *, struct sHandle *))
{
	 return HandleListIterate(psBase, &psParent->sChildren, offsetof(struct sHandle, sChildren), offsetof(struct sHandle, sSiblings), pfnIterFunc);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(GetHandleStructure)
#endif
static INLINE
PVRSRV_ERROR GetHandleStructure(PVRSRV_HANDLE_BASE *psBase, struct sHandle **ppsHandle, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	IMG_UINT32 ui32Index = HANDLE_TO_INDEX(hHandle);
	struct sHandle *psHandle;

	
	if (!INDEX_IS_VALID(psBase, ui32Index))
	{
		PVR_DPF((PVR_DBG_ERROR, "GetHandleStructure: Handle index out of range (%u >= %u)", ui32Index, psBase->ui32TotalHandCount));
		return PVRSRV_ERROR_HANDLE_INDEX_OUT_OF_RANGE;
	}

	psHandle =  INDEX_TO_HANDLE_STRUCT_PTR(psBase, ui32Index);
	if (psHandle->eType == PVRSRV_HANDLE_TYPE_NONE)
	{
		PVR_DPF((PVR_DBG_ERROR, "GetHandleStructure: Handle not allocated (index: %u)", ui32Index));
		return PVRSRV_ERROR_HANDLE_NOT_ALLOCATED;
	}

	
	if (eType != PVRSRV_HANDLE_TYPE_NONE && eType != psHandle->eType)
	{
		PVR_DPF((PVR_DBG_ERROR, "GetHandleStructure: Handle type mismatch (%d != %d)", eType, psHandle->eType));
		return PVRSRV_ERROR_HANDLE_TYPE_MISMATCH;
	}

	
	*ppsHandle = psHandle;

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(ParentIfPrivate)
#endif
static INLINE
IMG_HANDLE ParentIfPrivate(struct sHandle *psHandle)
{
	return TEST_ALLOC_FLAG(psHandle, PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE) ?
			ParentHandle(psHandle) : IMG_NULL;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(InitKey)
#endif
static INLINE
IMG_VOID InitKey(HAND_KEY aKey, PVRSRV_HANDLE_BASE *psBase, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, IMG_HANDLE hParent)
{
	PVR_UNREFERENCED_PARAMETER(psBase);

	aKey[HAND_KEY_DATA] = (IMG_UINTPTR_T)pvData;
	aKey[HAND_KEY_TYPE] = (IMG_UINTPTR_T)eType;
	aKey[HAND_KEY_PARENT] = (IMG_UINTPTR_T)hParent;
}

static
PVRSRV_ERROR ReallocHandleArray(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32NewCount)
{
	struct sHandleIndex *psOldArray = psBase->psHandleArray;
	IMG_HANDLE hOldArrayBlockAlloc = psBase->hArrayBlockAlloc;
	IMG_UINT32 ui32OldCount = psBase->ui32TotalHandCount;
	struct sHandleIndex *psNewArray = IMG_NULL;
	IMG_HANDLE hNewArrayBlockAlloc = IMG_NULL;
	PVRSRV_ERROR eError;
	PVRSRV_ERROR eReturn = PVRSRV_OK;
	IMG_UINT32 ui32Index;

	if (ui32NewCount == ui32OldCount)
	{
		return PVRSRV_OK;
	}

	if (ui32NewCount != 0 && !psBase->bPurgingEnabled &&
		 ui32NewCount < ui32OldCount)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (((ui32OldCount % HANDLE_BLOCK_SIZE) != 0) ||
		((ui32NewCount % HANDLE_BLOCK_SIZE) != 0))
	{
		PVR_ASSERT((ui32OldCount % HANDLE_BLOCK_SIZE) == 0);
		PVR_ASSERT((ui32NewCount % HANDLE_BLOCK_SIZE) == 0);

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (ui32NewCount != 0)
	{
		
		eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			HANDLE_ARRAY_SIZE(ui32NewCount) * sizeof(struct sHandleIndex),
			(IMG_VOID **)&psNewArray,
			&hNewArrayBlockAlloc,
			"Memory Area");
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "ReallocHandleArray: Couldn't allocate new handle array (%d)", eError));
			eReturn = eError;
			goto error;
		}

		if (ui32OldCount != 0)
		{
			OSMemCopy(psNewArray, psOldArray, HANDLE_ARRAY_SIZE(MIN(ui32NewCount, ui32OldCount)) * sizeof(struct sHandleIndex));
		}
	}

	
	for(ui32Index = ui32NewCount; ui32Index < ui32OldCount; ui32Index += HANDLE_BLOCK_SIZE)
	{
		struct sHandleIndex *psIndex = INDEX_TO_INDEX_STRUCT_PTR(psOldArray, ui32Index);

		eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				sizeof(struct sHandle) * HANDLE_BLOCK_SIZE,
				psIndex->psHandle,
				psIndex->hBlockAlloc);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "ReallocHandleArray: Couldn't free handle structures (%d)", eError));
		}
	}

	
	for(ui32Index = ui32OldCount; ui32Index < ui32NewCount; ui32Index += HANDLE_BLOCK_SIZE)
	{
		 
		struct sHandleIndex *psIndex = INDEX_TO_INDEX_STRUCT_PTR(psNewArray, ui32Index);

		eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				sizeof(struct sHandle) * HANDLE_BLOCK_SIZE,
				(IMG_VOID **)&psIndex->psHandle,
				&psIndex->hBlockAlloc,
				"Memory Area");
		if (eError != PVRSRV_OK)
		{
			psIndex->psHandle = IMG_NULL;
			PVR_DPF((PVR_DBG_ERROR, "ReallocHandleArray: Couldn't allocate handle structures (%d)", eError));
			eReturn = eError;
		}
		else
		{
			IMG_UINT32 ui32SubIndex;

			psIndex->ui32FreeHandBlockCount = HANDLE_BLOCK_SIZE;

			for(ui32SubIndex = 0; ui32SubIndex < HANDLE_BLOCK_SIZE; ui32SubIndex++)
			{
				struct sHandle *psHandle = psIndex->psHandle + ui32SubIndex;


				psHandle->ui32Index = ui32SubIndex + ui32Index;
				psHandle->eType = PVRSRV_HANDLE_TYPE_NONE;
				psHandle->eInternalFlag = INTERNAL_HANDLE_FLAG_NONE;
				psHandle->ui32NextIndexPlusOne  = 0;
			}
		}
	}
	if (eReturn != PVRSRV_OK)
	{
		goto error;
	}

#ifdef	DEBUG_MAX_HANDLE_COUNT
	
	if (ui32NewCount > DEBUG_MAX_HANDLE_COUNT)
	{
		PVR_DPF((PVR_DBG_ERROR, "ReallocHandleArray: Max handle count (%u) reached", DEBUG_MAX_HANDLE_COUNT));
		eReturn = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto error;
	}
#endif

	if (psOldArray != IMG_NULL)
	{
		
		eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			HANDLE_ARRAY_SIZE(ui32OldCount) * sizeof(struct sHandleIndex),
			psOldArray,
			hOldArrayBlockAlloc);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "ReallocHandleArray: Couldn't free old handle array (%d)", eError));
		}
	}

	psBase->psHandleArray = psNewArray;
	psBase->hArrayBlockAlloc = hNewArrayBlockAlloc;
	psBase->ui32TotalHandCount = ui32NewCount;

	if (ui32NewCount > ui32OldCount)
	{
		
		PVR_ASSERT(psBase->ui32FreeHandCount + (ui32NewCount - ui32OldCount) > psBase->ui32FreeHandCount)

		 
		psBase->ui32FreeHandCount += (ui32NewCount - ui32OldCount);

		
		if (psBase->ui32FirstFreeIndex == 0)
		{
			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne == 0)

			psBase->ui32FirstFreeIndex = ui32OldCount;
		}
		else
		{
			if (!psBase->bPurgingEnabled)
			{
				PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne != 0)
				PVR_ASSERT(INDEX_TO_HANDLE_STRUCT_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne == 0)

				INDEX_TO_HANDLE_STRUCT_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne = ui32OldCount + 1;
			}
		}

		if (!psBase->bPurgingEnabled)
		{
			psBase->ui32LastFreeIndexPlusOne = ui32NewCount;
		}
	}
	else
	{
		PVR_ASSERT(ui32NewCount == 0 || psBase->bPurgingEnabled)
		PVR_ASSERT(ui32NewCount == 0 || psBase->ui32FirstFreeIndex <= ui32NewCount)
		PVR_ASSERT(psBase->ui32FreeHandCount - (ui32OldCount - ui32NewCount) < psBase->ui32FreeHandCount)

		 
		psBase->ui32FreeHandCount -= (ui32OldCount - ui32NewCount);

		if (ui32NewCount == 0)
		{
			psBase->ui32FirstFreeIndex = 0;
			psBase->ui32LastFreeIndexPlusOne = 0;
		}
	}

	PVR_ASSERT(psBase->ui32FirstFreeIndex <= psBase->ui32TotalHandCount);

	return PVRSRV_OK;

error:
	PVR_ASSERT(eReturn != PVRSRV_OK);

	if (psNewArray != IMG_NULL)
	{
		
		for(ui32Index = ui32OldCount; ui32Index < ui32NewCount; ui32Index += HANDLE_BLOCK_SIZE)
		{
			struct sHandleIndex *psIndex = INDEX_TO_INDEX_STRUCT_PTR(psNewArray, ui32Index);
			if (psIndex->psHandle != IMG_NULL)
			{
				eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(struct sHandle) * HANDLE_BLOCK_SIZE,
						psIndex->psHandle,
						psIndex->hBlockAlloc);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR, "ReallocHandleArray: Couldn't free handle structures (%d)", eError));
				}
			}
		}

		
		eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			HANDLE_ARRAY_SIZE(ui32NewCount) * sizeof(struct sHandleIndex),
			psNewArray,
			hNewArrayBlockAlloc);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "ReallocHandleArray: Couldn't free new handle array (%d)", eError));
		}
	}

	return eReturn;
}

static PVRSRV_ERROR FreeHandleArray(PVRSRV_HANDLE_BASE *psBase)
{
	return ReallocHandleArray(psBase, 0);
}

static PVRSRV_ERROR FreeHandle(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	HAND_KEY aKey;
	IMG_UINT32 ui32Index = HANDLE_PTR_TO_INDEX(psHandle);
	PVRSRV_ERROR eError;

	
	InitKey(aKey, psBase, psHandle->pvData, psHandle->eType, ParentIfPrivate(psHandle));

	if (!TEST_ALLOC_FLAG(psHandle, PVRSRV_HANDLE_ALLOC_FLAG_MULTI) && !BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
	{
		IMG_HANDLE hHandle;
		hHandle = (IMG_HANDLE) HASH_Remove_Extended(psBase->psHashTab, aKey);

		PVR_ASSERT(hHandle != IMG_NULL);
		PVR_ASSERT(hHandle == INDEX_TO_HANDLE(ui32Index));
		PVR_UNREFERENCED_PARAMETER(hHandle);
	}

	
	UnlinkFromParent(psBase, psHandle);

	
	eError = IterateOverChildren(psBase, psHandle, FreeHandle);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandle: Error whilst freeing subhandles (%d)", eError));
		return eError;
	}

	
	psHandle->eType = PVRSRV_HANDLE_TYPE_NONE;

	if (BATCHED_HANDLE(psHandle) && !BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
	{
		 
        SET_BATCHED_HANDLE_PARTIALLY_FREE(psHandle);
		
		return PVRSRV_OK;
	}

	
	if (!psBase->bPurgingEnabled)
	{
		if (psBase->ui32FreeHandCount == 0)
		{
			PVR_ASSERT(psBase->ui32FirstFreeIndex == 0);
			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne == 0);

			psBase->ui32FirstFreeIndex =  ui32Index;
		}
		else
		{
			
			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne != 0);
			PVR_ASSERT(INDEX_TO_HANDLE_STRUCT_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne == 0);
			INDEX_TO_HANDLE_STRUCT_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne =  ui32Index + 1;
		}

		PVR_ASSERT(psHandle->ui32NextIndexPlusOne == 0);

		
		psBase->ui32LastFreeIndexPlusOne = ui32Index + 1;
	}

	psBase->ui32FreeHandCount++;
	INDEX_TO_FREE_HAND_BLOCK_COUNT(psBase, ui32Index)++;

	PVR_ASSERT(INDEX_TO_FREE_HAND_BLOCK_COUNT(psBase, ui32Index)<= HANDLE_BLOCK_SIZE);

#ifdef DEBUG
	{
		IMG_UINT32 ui32BlockedIndex;
		IMG_UINT32 ui32FreeHandCount = 0;

		for (ui32BlockedIndex = 0; ui32BlockedIndex < psBase->ui32TotalHandCount; ui32BlockedIndex += HANDLE_BLOCK_SIZE)
		{
			ui32FreeHandCount += INDEX_TO_FREE_HAND_BLOCK_COUNT(psBase, ui32BlockedIndex);
		}

		PVR_ASSERT(ui32FreeHandCount == psBase->ui32FreeHandCount);
	}
#endif

	return PVRSRV_OK;
}

static PVRSRV_ERROR FreeAllHandles(PVRSRV_HANDLE_BASE *psBase)
{
	IMG_UINT32 i;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psBase->ui32FreeHandCount == psBase->ui32TotalHandCount)
	{
		return eError;
	}

	for (i = 0; i < psBase->ui32TotalHandCount; i++)
	{
		struct sHandle *psHandle;

		psHandle = INDEX_TO_HANDLE_STRUCT_PTR(psBase, i);

		if (psHandle->eType != PVRSRV_HANDLE_TYPE_NONE)
		{
			eError = FreeHandle(psBase, psHandle);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "FreeAllHandles: FreeHandle failed (%d)", eError));
				break;
			}

			
			if (psBase->ui32FreeHandCount == psBase->ui32TotalHandCount)
			{
				break;
			}
		}
	}

	return eError;
}

static PVRSRV_ERROR FreeHandleBase(PVRSRV_HANDLE_BASE *psBase)
{
	PVRSRV_ERROR eError;

	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_WARNING, "FreeHandleBase: Uncommitted/Unreleased handle batch"));
		PVRSRVReleaseHandleBatch(psBase);
	}

	
	eError = FreeAllHandles(psBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandleBase: Couldn't free handles (%d)", eError));
		return eError;
	}

	
	eError = FreeHandleArray(psBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandleBase: Couldn't free handle array (%d)", eError));
		return eError;
	}

	if (psBase->psHashTab != IMG_NULL)
	{
		
		HASH_Delete(psBase->psHashTab);
	}

	eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		sizeof(*psBase),
		psBase,
		psBase->hBaseBlockAlloc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandleBase: Couldn't free handle base (%d)", eError));
		return eError;
	}

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(FindHandle)
#endif
static INLINE
IMG_HANDLE FindHandle(PVRSRV_HANDLE_BASE *psBase, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, IMG_HANDLE hParent)
{
	HAND_KEY aKey;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	InitKey(aKey, psBase, pvData, eType, hParent);

	return (IMG_HANDLE) HASH_Retrieve_Extended(psBase->psHashTab, aKey);
}

static PVRSRV_ERROR IncreaseHandleArraySize(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32Delta)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32DeltaAdjusted = ROUND_UP_TO_MULTIPLE_OF_BLOCK_SIZE(ui32Delta);
	IMG_UINT32 ui32NewTotalHandCount = psBase->ui32TotalHandCount + ui32DeltaAdjusted;
;

	PVR_ASSERT(ui32Delta != 0);

	
	if (ui32NewTotalHandCount > psBase->ui32MaxIndexPlusOne || ui32NewTotalHandCount <= psBase->ui32TotalHandCount)
	{
		ui32NewTotalHandCount = psBase->ui32MaxIndexPlusOne;

		ui32DeltaAdjusted = ui32NewTotalHandCount - psBase->ui32TotalHandCount;

		if (ui32DeltaAdjusted < ui32Delta)
		{
			PVR_DPF((PVR_DBG_ERROR, "IncreaseHandleArraySize: Maximum handle limit reached (%d)", psBase->ui32MaxIndexPlusOne));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
	}

	PVR_ASSERT(ui32DeltaAdjusted >= ui32Delta);

	
	eError = ReallocHandleArray(psBase, ui32NewTotalHandCount);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "IncreaseHandleArraySize: ReallocHandleArray failed (%d)", eError));
		return eError;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnsureFreeHandles(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32Free)
{
	PVRSRV_ERROR eError;

	if (ui32Free > psBase->ui32FreeHandCount)
	{
		IMG_UINT32 ui32FreeHandDelta = ui32Free - psBase->ui32FreeHandCount;
		eError = IncreaseHandleArraySize(psBase, ui32FreeHandDelta);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnsureFreeHandles: Couldn't allocate %u handles to ensure %u free handles (IncreaseHandleArraySize failed with error %d)", ui32FreeHandDelta, ui32Free, eError));

			return eError;
		}
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR AllocHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, PVRSRV_HANDLE_ALLOC_FLAG eFlag, IMG_HANDLE hParent)
{
	IMG_UINT32 ui32NewIndex = DEFAULT_MAX_INDEX_PLUS_ONE;
	struct sHandle *psNewHandle = IMG_NULL;
	IMG_HANDLE hHandle;
	HAND_KEY aKey;
	PVRSRV_ERROR eError;

	
	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);
	PVR_ASSERT(psBase != IMG_NULL);
	PVR_ASSERT(psBase->psHashTab != IMG_NULL);

	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{
		
		PVR_ASSERT(FindHandle(psBase, pvData, eType, hParent) == IMG_NULL);
	}

	if (psBase->ui32FreeHandCount == 0 && HANDLES_BATCHED(psBase))
	{
		 PVR_DPF((PVR_DBG_WARNING, "AllocHandle: Handle batch size (%u) was too small, allocating additional space", psBase->ui32HandBatchSize));
	}

	
	eError = EnsureFreeHandles(psBase, 1);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "AllocHandle: EnsureFreeHandles failed (%d)", eError));
		return eError;
	}
	PVR_ASSERT(psBase->ui32FreeHandCount != 0)

	if (!psBase->bPurgingEnabled)
	{
		
		ui32NewIndex = psBase->ui32FirstFreeIndex;

		
		psNewHandle = INDEX_TO_HANDLE_STRUCT_PTR(psBase, ui32NewIndex);
	}
	else
	{
		IMG_UINT32 ui32BlockedIndex;

		
		
		PVR_ASSERT((psBase->ui32FirstFreeIndex % HANDLE_BLOCK_SIZE) == 0);

		for (ui32BlockedIndex = ROUND_DOWN_TO_MULTIPLE_OF_BLOCK_SIZE(psBase->ui32FirstFreeIndex); ui32BlockedIndex < psBase->ui32TotalHandCount; ui32BlockedIndex += HANDLE_BLOCK_SIZE)
		{
			struct sHandleIndex *psIndex = BASE_AND_INDEX_TO_INDEX_STRUCT_PTR(psBase, ui32BlockedIndex);

			if (psIndex->ui32FreeHandBlockCount == 0)
			{
				continue;
			}

			for (ui32NewIndex = ui32BlockedIndex; ui32NewIndex < ui32BlockedIndex + HANDLE_BLOCK_SIZE; ui32NewIndex++)
			{
				psNewHandle = INDEX_TO_HANDLE_STRUCT_PTR(psBase, ui32NewIndex);
				if (HANDLE_STRUCT_IS_FREE(psNewHandle))
				{
					break;
				}
			}
		}
		psBase->ui32FirstFreeIndex = 0;
		PVR_ASSERT(ui32NewIndex < psBase->ui32TotalHandCount);
	}
	PVR_ASSERT(psNewHandle != IMG_NULL);

	
	hHandle = INDEX_TO_HANDLE(ui32NewIndex);

	
	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{
		
		InitKey(aKey, psBase, pvData, eType, hParent);

		
		if (!HASH_Insert_Extended(psBase->psHashTab, aKey, (IMG_UINTPTR_T)hHandle))
		{
			PVR_DPF((PVR_DBG_ERROR, "AllocHandle: Couldn't add handle to hash table"));

			return PVRSRV_ERROR_UNABLE_TO_ADD_HANDLE;
		}
	}

	psBase->ui32FreeHandCount--;

	PVR_ASSERT(INDEX_TO_FREE_HAND_BLOCK_COUNT(psBase, ui32NewIndex) <= HANDLE_BLOCK_SIZE);
	PVR_ASSERT(INDEX_TO_FREE_HAND_BLOCK_COUNT(psBase, ui32NewIndex) > 0);

	INDEX_TO_FREE_HAND_BLOCK_COUNT(psBase, ui32NewIndex)--;

	
	if (!psBase->bPurgingEnabled)
	{
		
		if (psBase->ui32FreeHandCount == 0)
		{
			PVR_ASSERT(psBase->ui32FirstFreeIndex == ui32NewIndex);
			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne == (ui32NewIndex + 1));

			psBase->ui32LastFreeIndexPlusOne = 0;
			psBase->ui32FirstFreeIndex = 0;
		}
		else
		{
			
			psBase->ui32FirstFreeIndex = (psNewHandle->ui32NextIndexPlusOne == 0) ?
				ui32NewIndex + 1 :
				psNewHandle->ui32NextIndexPlusOne - 1;
		}
	}

	
	PVR_ASSERT(psNewHandle->ui32Index == ui32NewIndex);

	 
	psNewHandle->eType = eType;
	psNewHandle->pvData = pvData;
	psNewHandle->eInternalFlag = INTERNAL_HANDLE_FLAG_NONE;
	psNewHandle->eFlag = eFlag;

	InitParentList(psNewHandle);
#if defined(DEBUG)
	PVR_ASSERT(NoChildren(psNewHandle));
#endif

	InitChildEntry(psNewHandle);
#if defined(DEBUG)
	PVR_ASSERT(NoParent(psNewHandle));
#endif

	if (HANDLES_BATCHED(psBase))
	{
		
		psNewHandle->ui32NextIndexPlusOne = psBase->ui32FirstBatchIndexPlusOne;

		psBase->ui32FirstBatchIndexPlusOne = ui32NewIndex + 1;

		 
		SET_BATCHED_HANDLE(psNewHandle);
	}
	else
	{
		psNewHandle->ui32NextIndexPlusOne = 0;
	}

	
	*phHandle = hHandle;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVAllocHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, PVRSRV_HANDLE_ALLOC_FLAG eFlag)
{
	IMG_HANDLE hHandle;
	PVRSRV_ERROR eError;

	*phHandle = IMG_NULL;

	if (HANDLES_BATCHED(psBase))
	{
		
		psBase->ui32BatchHandAllocFailures++;
	}

	
	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{
		
		hHandle = FindHandle(psBase, pvData, eType, IMG_NULL);
		if (hHandle != IMG_NULL)
		{
			struct sHandle *psHandle;

			eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocHandle: Lookup of existing handle failed"));
				return eError;
			}

			
			if (TEST_FLAG(psHandle->eFlag & eFlag, PVRSRV_HANDLE_ALLOC_FLAG_SHARED))
			{
				*phHandle = hHandle;
				eError = PVRSRV_OK;
				goto exit_ok;
			}
			return PVRSRV_ERROR_HANDLE_NOT_SHAREABLE;
		}
	}

	eError = AllocHandle(psBase, phHandle, pvData, eType, eFlag, IMG_NULL);

exit_ok:
	if (HANDLES_BATCHED(psBase) && (eError == PVRSRV_OK))
	{
		psBase->ui32BatchHandAllocFailures--;
	}

	return eError;
}

PVRSRV_ERROR PVRSRVAllocSubHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, PVRSRV_HANDLE_ALLOC_FLAG eFlag, IMG_HANDLE hParent)
{
	struct sHandle *psPHand;
	struct sHandle *psCHand;
	PVRSRV_ERROR eError;
	IMG_HANDLE hParentKey;
	IMG_HANDLE hHandle;

	*phHandle = IMG_NULL;

	if (HANDLES_BATCHED(psBase))
	{
		
		psBase->ui32BatchHandAllocFailures++;
	}

	
	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	hParentKey = TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE) ?
			hParent : IMG_NULL;

	
	eError = GetHandleStructure(psBase, &psPHand, hParent, PVRSRV_HANDLE_TYPE_NONE);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{
		
		hHandle = FindHandle(psBase, pvData, eType, hParentKey);
		if (hHandle != IMG_NULL)
		{
			struct sHandle *psCHandle;
			PVRSRV_ERROR eErr;

			eErr = GetHandleStructure(psBase, &psCHandle, hHandle, eType);
			if (eErr != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSubHandle: Lookup of existing handle failed"));
				return eErr;
			}

			PVR_ASSERT(hParentKey != IMG_NULL && ParentHandle(HANDLE_TO_HANDLE_STRUCT_PTR(psBase, hHandle)) == hParent);

			
			if (TEST_FLAG(psCHandle->eFlag & eFlag, PVRSRV_HANDLE_ALLOC_FLAG_SHARED) && ParentHandle(HANDLE_TO_HANDLE_STRUCT_PTR(psBase, hHandle)) == hParent)
			{
				*phHandle = hHandle;
				goto exit_ok;
			}
			return PVRSRV_ERROR_HANDLE_NOT_SHAREABLE;
		}
	}

	eError = AllocHandle(psBase, &hHandle, pvData, eType, eFlag, hParentKey);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	
	psPHand = HANDLE_TO_HANDLE_STRUCT_PTR(psBase, hParent);

	psCHand = HANDLE_TO_HANDLE_STRUCT_PTR(psBase, hHandle);

	AdoptChild(psBase, psPHand, psCHand);

	*phHandle = hHandle;

exit_ok:
	if (HANDLES_BATCHED(psBase))
	{
		psBase->ui32BatchHandAllocFailures--;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVFindHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType)
{
	IMG_HANDLE hHandle;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	
	hHandle = (IMG_HANDLE) FindHandle(psBase, pvData, eType, IMG_NULL);
	if (hHandle == IMG_NULL)
	{
		return PVRSRV_ERROR_HANDLE_NOT_FOUND;
	}

	*phHandle = hHandle;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupHandleAnyType(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, PVRSRV_HANDLE_TYPE *peType, IMG_HANDLE hHandle)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	eError = GetHandleStructure(psBase, &psHandle, hHandle, PVRSRV_HANDLE_TYPE_NONE);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupHandleAnyType: Error looking up handle (%d)", eError));
		return eError;
	}

	*ppvData = psHandle->pvData;
	*peType = psHandle->eType;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupHandle: Error looking up handle (%d)", eError));
		return eError;
	}

	*ppvData = psHandle->pvData;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupSubHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType, IMG_HANDLE hAncestor)
{
	struct sHandle *psPHand;
	struct sHandle *psCHand;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psCHand, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupSubHandle: Error looking up subhandle (%d)", eError));
		return eError;
	}

	
	for (psPHand = psCHand; ParentHandle(psPHand) != hAncestor; )
	{
		eError = GetHandleStructure(psBase, &psPHand, ParentHandle(psPHand), PVRSRV_HANDLE_TYPE_NONE);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupSubHandle: Subhandle doesn't belong to given ancestor"));
			return PVRSRV_ERROR_INVALID_SUBHANDLE;
		}
	}

	*ppvData = psCHand->pvData;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVGetParentHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *phParent, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetParentHandle: Error looking up subhandle (%d)", eError));
		return eError;
	}

	*phParent = ParentHandle(psHandle);

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupAndReleaseHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupAndReleaseHandle: Error looking up handle (%d)", eError));
		return eError;
	}

	*ppvData = psHandle->pvData;

	eError = FreeHandle(psBase, psHandle);

	return eError;
}

PVRSRV_ERROR PVRSRVReleaseHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVReleaseHandle: Error looking up handle (%d)", eError));
		return eError;
	}

	eError = FreeHandle(psBase, psHandle);

	return eError;
}

PVRSRV_ERROR PVRSRVNewHandleBatch(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32BatchSize)
{
	PVRSRV_ERROR eError;

	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVNewHandleBatch: There is a handle batch already in use (size %u)", psBase->ui32HandBatchSize));
		return  PVRSRV_ERROR_HANDLE_BATCH_IN_USE;
	}

	if (ui32BatchSize == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVNewHandleBatch: Invalid batch size (%u)", ui32BatchSize));
		return  PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = EnsureFreeHandles(psBase, ui32BatchSize);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVNewHandleBatch: EnsureFreeHandles failed (error %d)", eError));
		return eError;
	}

	psBase->ui32HandBatchSize = ui32BatchSize;

	
	psBase->ui32TotalHandCountPreBatch = psBase->ui32TotalHandCount;

	PVR_ASSERT(psBase->ui32BatchHandAllocFailures == 0);

	PVR_ASSERT(psBase->ui32FirstBatchIndexPlusOne == 0);

	PVR_ASSERT(HANDLES_BATCHED(psBase));

	return PVRSRV_OK;
}

static PVRSRV_ERROR PVRSRVHandleBatchCommitOrRelease(PVRSRV_HANDLE_BASE *psBase, IMG_BOOL bCommit)
{

	IMG_UINT32 ui32IndexPlusOne;
	IMG_BOOL bCommitBatch = bCommit;

	if (!HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleBatchCommitOrRelease: There is no handle batch"));
		return PVRSRV_ERROR_INVALID_PARAMS;

	}

	if (psBase->ui32BatchHandAllocFailures != 0)
	{
		if (bCommit)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleBatchCommitOrRelease: Attempting to commit batch with handle allocation failures."));
		}
		bCommitBatch = IMG_FALSE;
	}
	
	PVR_ASSERT(psBase->ui32BatchHandAllocFailures == 0 || !bCommit);

	ui32IndexPlusOne = psBase->ui32FirstBatchIndexPlusOne;
	while(ui32IndexPlusOne != 0)
	{
		struct sHandle *psHandle = INDEX_TO_HANDLE_STRUCT_PTR(psBase, ui32IndexPlusOne - 1);
		IMG_UINT32 ui32NextIndexPlusOne = psHandle->ui32NextIndexPlusOne;
		PVR_ASSERT(BATCHED_HANDLE(psHandle));

		psHandle->ui32NextIndexPlusOne = 0;

		if (!bCommitBatch || BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
		{
			PVRSRV_ERROR eError;

			
			if (!BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
			{
				 
				SET_UNBATCHED_HANDLE(psHandle);  
			}

			eError = FreeHandle(psBase, psHandle);
			if (eError != PVRSRV_OK)
			{
				 PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleBatchCommitOrRelease: Error freeing handle (%d)", eError));
			}
			PVR_ASSERT(eError == PVRSRV_OK);
		}
		else
		{
			 
			SET_UNBATCHED_HANDLE(psHandle);
		}

		ui32IndexPlusOne = ui32NextIndexPlusOne;
	}

#ifdef DEBUG
	if (psBase->ui32TotalHandCountPreBatch != psBase->ui32TotalHandCount)
	{
		IMG_UINT32 ui32Delta = psBase->ui32TotalHandCount - psBase->ui32TotalHandCountPreBatch;

		PVR_ASSERT(psBase->ui32TotalHandCount > psBase->ui32TotalHandCountPreBatch);

		PVR_DPF((PVR_DBG_WARNING, "PVRSRVHandleBatchCommitOrRelease: The batch size was too small.  Batch size was %u, but needs to be %u", psBase->ui32HandBatchSize,  psBase->ui32HandBatchSize + ui32Delta));

	}
#endif

	psBase->ui32HandBatchSize = 0;
	psBase->ui32FirstBatchIndexPlusOne = 0;
	psBase->ui32TotalHandCountPreBatch = 0;
	psBase->ui32BatchHandAllocFailures = 0;

	if (psBase->ui32BatchHandAllocFailures != 0 && bCommit)
	{
		PVR_ASSERT(!bCommitBatch);

		return PVRSRV_ERROR_HANDLE_BATCH_COMMIT_FAILURE;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVCommitHandleBatch(PVRSRV_HANDLE_BASE *psBase)
{
	return PVRSRVHandleBatchCommitOrRelease(psBase, IMG_TRUE);
}

IMG_VOID PVRSRVReleaseHandleBatch(PVRSRV_HANDLE_BASE *psBase)
{
	(IMG_VOID) PVRSRVHandleBatchCommitOrRelease(psBase, IMG_FALSE);
}

PVRSRV_ERROR PVRSRVSetMaxHandle(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32MaxHandle)
{
	IMG_UINT32 ui32MaxHandleRounded;

	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetMaxHandle: Limit cannot be set whilst in batch mode"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	if (ui32MaxHandle  == 0 || ui32MaxHandle > DEFAULT_MAX_HANDLE)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetMaxHandle: Limit must be between %u and %u, inclusive", 0, DEFAULT_MAX_HANDLE));

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	if (psBase->ui32TotalHandCount != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetMaxHandle: Limit cannot be set because handles have already been allocated"));

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	ui32MaxHandleRounded = ROUND_DOWN_TO_MULTIPLE_OF_BLOCK_SIZE(ui32MaxHandle);

	
	if (ui32MaxHandleRounded != 0 && ui32MaxHandleRounded < psBase->ui32MaxIndexPlusOne)
	{
		psBase->ui32MaxIndexPlusOne = ui32MaxHandleRounded;
	}

	PVR_ASSERT(psBase->ui32MaxIndexPlusOne != 0);
	PVR_ASSERT(psBase->ui32MaxIndexPlusOne <= DEFAULT_MAX_INDEX_PLUS_ONE);
	PVR_ASSERT((psBase->ui32MaxIndexPlusOne % HANDLE_BLOCK_SIZE) == 0);

	return PVRSRV_OK;
}

IMG_UINT32 PVRSRVGetMaxHandle(PVRSRV_HANDLE_BASE *psBase)
{
	return psBase->ui32MaxIndexPlusOne;
}

PVRSRV_ERROR PVRSRVEnableHandlePurging(PVRSRV_HANDLE_BASE *psBase)
{
	if (psBase->bPurgingEnabled)
	{
		PVR_DPF((PVR_DBG_WARNING, "PVRSRVEnableHandlePurging: Purging already enabled"));
		return PVRSRV_OK;
	}

	
	if (psBase->ui32TotalHandCount != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnableHandlePurging: Handles have already been allocated"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psBase->bPurgingEnabled = IMG_TRUE;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVPurgeHandles(PVRSRV_HANDLE_BASE *psBase)
{
	IMG_UINT32 ui32BlockIndex;
	IMG_UINT32 ui32NewHandCount;

	if (!psBase->bPurgingEnabled)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPurgeHandles: Purging not enabled for this handle base"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPurgeHandles: Purging not allowed whilst in batch mode"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVR_ASSERT((psBase->ui32TotalHandCount % HANDLE_BLOCK_SIZE) == 0);

	for (ui32BlockIndex = INDEX_TO_BLOCK_INDEX(psBase->ui32TotalHandCount); ui32BlockIndex != 0; ui32BlockIndex--)
	{
		if (psBase->psHandleArray[ui32BlockIndex - 1].ui32FreeHandBlockCount != HANDLE_BLOCK_SIZE)
		{
			break;
		}
	}
	ui32NewHandCount = BLOCK_INDEX_TO_INDEX(ui32BlockIndex);

	
	if (ui32NewHandCount <= (psBase->ui32TotalHandCount/2))
	{
		PVRSRV_ERROR eError;

		

		eError = ReallocHandleArray(psBase, ui32NewHandCount);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVAllocHandleBase(PVRSRV_HANDLE_BASE **ppsBase)
{
	PVRSRV_HANDLE_BASE *psBase;
	IMG_HANDLE hBlockAlloc;
	PVRSRV_ERROR eError;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		sizeof(*psBase),
		(IMG_PVOID *)&psBase,
		&hBlockAlloc,
		"Handle Base");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocHandleBase: Couldn't allocate handle base (%d)", eError));
		return eError;
	}
	OSMemSet(psBase, 0, sizeof(*psBase));

	
	psBase->psHashTab = HASH_Create_Extended(HANDLE_HASH_TAB_INIT_SIZE, sizeof(HAND_KEY), HASH_Func_Default, HASH_Key_Comp_Default);
	if (psBase->psHashTab == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocHandleBase: Couldn't create data pointer hash table\n"));
		(IMG_VOID)PVRSRVFreeHandleBase(psBase);
		return PVRSRV_ERROR_UNABLE_TO_CREATE_HASH_TABLE;
	}

	psBase->hBaseBlockAlloc = hBlockAlloc;

	psBase->ui32MaxIndexPlusOne = DEFAULT_MAX_INDEX_PLUS_ONE;

	*ppsBase = psBase;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVFreeHandleBase(PVRSRV_HANDLE_BASE *psBase)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(psBase != gpsKernelHandleBase);

	eError = FreeHandleBase(psBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeHandleBase: FreeHandleBase failed (%d)", eError));
	}

	return eError;
}

PVRSRV_ERROR PVRSRVHandleInit(IMG_VOID)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(gpsKernelHandleBase == IMG_NULL);

	eError = PVRSRVAllocHandleBase(&gpsKernelHandleBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleInit: PVRSRVAllocHandleBase failed (%d)", eError));
		goto error;
	}

	eError = PVRSRVEnableHandlePurging(gpsKernelHandleBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleInit: PVRSRVEnableHandlePurging failed (%d)", eError));
		goto error;
	}

	return PVRSRV_OK;
error:
	(IMG_VOID) PVRSRVHandleDeInit();
	return eError;
}

PVRSRV_ERROR PVRSRVHandleDeInit(IMG_VOID)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (gpsKernelHandleBase != IMG_NULL)
	{
		eError = FreeHandleBase(gpsKernelHandleBase);
		if (eError == PVRSRV_OK)
		{
			gpsKernelHandleBase = IMG_NULL;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleDeInit: FreeHandleBase failed (%d)", eError));
		}
	}

	return eError;
}
#else
#endif	
