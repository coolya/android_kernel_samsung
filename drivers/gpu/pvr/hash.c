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

#include "pvr_debug.h"
#include "img_defs.h"
#include "services.h"
#include "servicesint.h"
#include "hash.h"
#include "osfunc.h"

#define PRIVATE_MAX(a,b) ((a)>(b)?(a):(b))

#define	KEY_TO_INDEX(pHash, key, uSize) \
	((pHash)->pfnHashFunc((pHash)->uKeySize, (key), (uSize)) % (uSize))

#define	KEY_COMPARE(pHash, pKey1, pKey2) \
	((pHash)->pfnKeyComp((pHash)->uKeySize, (pKey1), (pKey2)))

struct _BUCKET_
{
	
	struct _BUCKET_ *pNext;

	
	IMG_UINTPTR_T v;

	
	IMG_UINTPTR_T k[];		 
};
typedef struct _BUCKET_ BUCKET;

struct _HASH_TABLE_
{
	
	BUCKET **ppBucketTable;

	
	IMG_UINT32 uSize;

	
	IMG_UINT32 uCount;

	
	IMG_UINT32 uMinimumSize;

	
	IMG_UINT32 uKeySize;

	
	HASH_FUNC *pfnHashFunc;

	
	HASH_KEY_COMP *pfnKeyComp;
};

IMG_UINT32
HASH_Func_Default (IMG_SIZE_T uKeySize, IMG_VOID *pKey, IMG_UINT32 uHashTabLen)
{
	IMG_UINTPTR_T *p = (IMG_UINTPTR_T *)pKey;
	IMG_UINT32 uKeyLen = uKeySize / sizeof(IMG_UINTPTR_T);
	IMG_UINT32 ui;
	IMG_UINT32 uHashKey = 0;

	PVR_UNREFERENCED_PARAMETER(uHashTabLen);

	PVR_ASSERT((uKeySize % sizeof(IMG_UINTPTR_T)) == 0);

	for (ui = 0; ui < uKeyLen; ui++)
	{
		IMG_UINT32 uHashPart = (IMG_UINT32)*p++;

		uHashPart += (uHashPart << 12);
		uHashPart ^= (uHashPart >> 22);
		uHashPart += (uHashPart << 4);
		uHashPart ^= (uHashPart >> 9);
		uHashPart += (uHashPart << 10);
		uHashPart ^= (uHashPart >> 2);
		uHashPart += (uHashPart << 7);
		uHashPart ^= (uHashPart >> 12);

		uHashKey += uHashPart;
	}

	return uHashKey;
}

IMG_BOOL
HASH_Key_Comp_Default (IMG_SIZE_T uKeySize, IMG_VOID *pKey1, IMG_VOID *pKey2)
{
	IMG_UINTPTR_T *p1 = (IMG_UINTPTR_T *)pKey1;
	IMG_UINTPTR_T *p2 = (IMG_UINTPTR_T *)pKey2;
	IMG_UINT32 uKeyLen = uKeySize / sizeof(IMG_UINTPTR_T);
	IMG_UINT32 ui;

	PVR_ASSERT((uKeySize % sizeof(IMG_UINTPTR_T)) == 0);

	for (ui = 0; ui < uKeyLen; ui++)
	{
		if (*p1++ != *p2++)
			return IMG_FALSE;
	}

	return IMG_TRUE;
}

static PVRSRV_ERROR
_ChainInsert (HASH_TABLE *pHash, BUCKET *pBucket, BUCKET **ppBucketTable, IMG_UINT32 uSize)
{
	IMG_UINT32 uIndex;

	PVR_ASSERT (pBucket != IMG_NULL);
	PVR_ASSERT (ppBucketTable != IMG_NULL);
	PVR_ASSERT (uSize != 0);

	if ((pBucket == IMG_NULL) || (ppBucketTable == IMG_NULL) || (uSize == 0))
	{
		PVR_DPF((PVR_DBG_ERROR, "_ChainInsert: invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	uIndex = KEY_TO_INDEX(pHash, pBucket->k, uSize);	 
	pBucket->pNext = ppBucketTable[uIndex];
	ppBucketTable[uIndex] = pBucket;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_Rehash (HASH_TABLE *pHash,
	 BUCKET **ppOldTable, IMG_UINT32 uOldSize,
         BUCKET **ppNewTable, IMG_UINT32 uNewSize)
{
	IMG_UINT32 uIndex;
	for (uIndex=0; uIndex< uOldSize; uIndex++)
    {
		BUCKET *pBucket;
		pBucket = ppOldTable[uIndex];
		while (pBucket != IMG_NULL)
		{
			PVRSRV_ERROR eError;
			BUCKET *pNextBucket = pBucket->pNext;
			eError = _ChainInsert (pHash, pBucket, ppNewTable, uNewSize);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "_Rehash: call to _ChainInsert failed"));
				return eError;
			}
			pBucket = pNextBucket;
		}
    }
	return PVRSRV_OK;
}

static IMG_BOOL
_Resize (HASH_TABLE *pHash, IMG_UINT32 uNewSize)
{
	if (uNewSize != pHash->uSize)
    {
		BUCKET **ppNewTable;
        IMG_UINT32 uIndex;

		PVR_DPF ((PVR_DBG_MESSAGE,
                  "HASH_Resize: oldsize=0x%x  newsize=0x%x  count=0x%x",
				pHash->uSize, uNewSize, pHash->uCount));

		OSAllocMem(PVRSRV_PAGEABLE_SELECT,
                      sizeof (BUCKET *) * uNewSize,
                      (IMG_PVOID*)&ppNewTable, IMG_NULL,
					  "Hash Table Buckets");
		if (ppNewTable == IMG_NULL)
            return IMG_FALSE;

        for (uIndex=0; uIndex<uNewSize; uIndex++)
            ppNewTable[uIndex] = IMG_NULL;

        if (_Rehash (pHash, pHash->ppBucketTable, pHash->uSize, ppNewTable, uNewSize) != PVRSRV_OK)
		{
			return IMG_FALSE;
		}

        OSFreeMem (PVRSRV_PAGEABLE_SELECT, sizeof(BUCKET *)*pHash->uSize, pHash->ppBucketTable, IMG_NULL);
        
        pHash->ppBucketTable = ppNewTable;
        pHash->uSize = uNewSize;
    }
    return IMG_TRUE;
}


HASH_TABLE * HASH_Create_Extended (IMG_UINT32 uInitialLen, IMG_SIZE_T uKeySize, HASH_FUNC *pfnHashFunc, HASH_KEY_COMP *pfnKeyComp)
{
	HASH_TABLE *pHash;
	IMG_UINT32 uIndex;

	PVR_DPF ((PVR_DBG_MESSAGE, "HASH_Create_Extended: InitialSize=0x%x", uInitialLen));

	if(OSAllocMem(PVRSRV_PAGEABLE_SELECT,
					sizeof(HASH_TABLE),
					(IMG_VOID **)&pHash, IMG_NULL,
					"Hash Table") != PVRSRV_OK)
	{
		return IMG_NULL;
	}

	pHash->uCount = 0;
	pHash->uSize = uInitialLen;
	pHash->uMinimumSize = uInitialLen;
	pHash->uKeySize = uKeySize;
	pHash->pfnHashFunc = pfnHashFunc;
	pHash->pfnKeyComp = pfnKeyComp;

	OSAllocMem(PVRSRV_PAGEABLE_SELECT,
                  sizeof (BUCKET *) * pHash->uSize,
                  (IMG_PVOID*)&pHash->ppBucketTable, IMG_NULL,
				  "Hash Table Buckets");

	if (pHash->ppBucketTable == IMG_NULL)
    {
		OSFreeMem(PVRSRV_PAGEABLE_SELECT, sizeof(HASH_TABLE), pHash, IMG_NULL);
		
		return IMG_NULL;
    }

	for (uIndex=0; uIndex<pHash->uSize; uIndex++)
		pHash->ppBucketTable[uIndex] = IMG_NULL;
	return pHash;
}

HASH_TABLE * HASH_Create (IMG_UINT32 uInitialLen)
{
	return HASH_Create_Extended(uInitialLen, sizeof(IMG_UINTPTR_T),
		&HASH_Func_Default, &HASH_Key_Comp_Default);
}

IMG_VOID
HASH_Delete (HASH_TABLE *pHash)
{
	if (pHash != IMG_NULL)
    {
		PVR_DPF ((PVR_DBG_MESSAGE, "HASH_Delete"));

		PVR_ASSERT (pHash->uCount==0);
		if(pHash->uCount != 0)
		{
			PVR_DPF ((PVR_DBG_ERROR, "HASH_Delete: leak detected in hash table!"));
			PVR_DPF ((PVR_DBG_ERROR, "Likely Cause: client drivers not freeing alocations before destroying devmemcontext"));
		}
		OSFreeMem(PVRSRV_PAGEABLE_SELECT, sizeof(BUCKET *)*pHash->uSize, pHash->ppBucketTable, IMG_NULL);
		pHash->ppBucketTable = IMG_NULL;
		OSFreeMem(PVRSRV_PAGEABLE_SELECT, sizeof(HASH_TABLE), pHash, IMG_NULL);
		
    }
}

IMG_BOOL
HASH_Insert_Extended (HASH_TABLE *pHash, IMG_VOID *pKey, IMG_UINTPTR_T v)
{
	BUCKET *pBucket;

	PVR_DPF ((PVR_DBG_MESSAGE,
              "HASH_Insert_Extended: Hash=0x%08x, pKey=0x%08x, v=0x%x",
              (IMG_UINTPTR_T)pHash, (IMG_UINTPTR_T)pKey, v));

	PVR_ASSERT (pHash != IMG_NULL);

	if (pHash == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "HASH_Insert_Extended: invalid parameter"));
		return IMG_FALSE;
	}

	if(OSAllocMem(PVRSRV_PAGEABLE_SELECT,
					sizeof(BUCKET) + pHash->uKeySize,
					(IMG_VOID **)&pBucket, IMG_NULL,
					"Hash Table entry") != PVRSRV_OK)
	{
		return IMG_FALSE;
	}

	pBucket->v = v;
	 
	OSMemCopy(pBucket->k, pKey, pHash->uKeySize);
	if (_ChainInsert (pHash, pBucket, pHash->ppBucketTable, pHash->uSize) != PVRSRV_OK)
	{
		return IMG_FALSE;
	}

	pHash->uCount++;

	
	if (pHash->uCount << 1 > pHash->uSize)
    {
        

        _Resize (pHash, pHash->uSize << 1);
    }


	return IMG_TRUE;
}

IMG_BOOL
HASH_Insert (HASH_TABLE *pHash, IMG_UINTPTR_T k, IMG_UINTPTR_T v)
{
	PVR_DPF ((PVR_DBG_MESSAGE,
              "HASH_Insert: Hash=0x%x, k=0x%x, v=0x%x",
              (IMG_UINTPTR_T)pHash, k, v));

	return HASH_Insert_Extended(pHash, &k, v);
}

IMG_UINTPTR_T
HASH_Remove_Extended(HASH_TABLE *pHash, IMG_VOID *pKey)
{
	BUCKET **ppBucket;
	IMG_UINT32 uIndex;

	PVR_DPF ((PVR_DBG_MESSAGE, "HASH_Remove_Extended: Hash=0x%x, pKey=0x%x",
			(IMG_UINTPTR_T)pHash, (IMG_UINTPTR_T)pKey));

	PVR_ASSERT (pHash != IMG_NULL);

	if (pHash == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "HASH_Remove_Extended: Null hash table"));
		return 0;
	}

	uIndex = KEY_TO_INDEX(pHash, pKey, pHash->uSize);

	for (ppBucket = &(pHash->ppBucketTable[uIndex]); *ppBucket != IMG_NULL; ppBucket = &((*ppBucket)->pNext))
	{
		 
		if (KEY_COMPARE(pHash, (*ppBucket)->k, pKey))
		{
			BUCKET *pBucket = *ppBucket;
			IMG_UINTPTR_T v = pBucket->v;
			(*ppBucket) = pBucket->pNext;

			OSFreeMem(PVRSRV_PAGEABLE_SELECT, sizeof(BUCKET) + pHash->uKeySize, pBucket, IMG_NULL);
			

			pHash->uCount--;

			
			if (pHash->uSize > (pHash->uCount << 2) &&
                pHash->uSize > pHash->uMinimumSize)
            {
                

				_Resize (pHash,
                         PRIVATE_MAX (pHash->uSize >> 1,
                                      pHash->uMinimumSize));
            }

			PVR_DPF ((PVR_DBG_MESSAGE,
                      "HASH_Remove_Extended: Hash=0x%x, pKey=0x%x = 0x%x",
                      (IMG_UINTPTR_T)pHash, (IMG_UINTPTR_T)pKey, v));
			return v;
		}
	}
	PVR_DPF ((PVR_DBG_MESSAGE,
              "HASH_Remove_Extended: Hash=0x%x, pKey=0x%x = 0x0 !!!!",
              (IMG_UINTPTR_T)pHash, (IMG_UINTPTR_T)pKey));
	return 0;
}

IMG_UINTPTR_T
HASH_Remove (HASH_TABLE *pHash, IMG_UINTPTR_T k)
{
	PVR_DPF ((PVR_DBG_MESSAGE, "HASH_Remove: Hash=0x%x, k=0x%x",
			(IMG_UINTPTR_T)pHash, k));

	return HASH_Remove_Extended(pHash, &k);
}

IMG_UINTPTR_T
HASH_Retrieve_Extended (HASH_TABLE *pHash, IMG_VOID *pKey)
{
	BUCKET **ppBucket;
	IMG_UINT32 uIndex;

	PVR_DPF ((PVR_DBG_MESSAGE, "HASH_Retrieve_Extended: Hash=0x%x, pKey=0x%x",
			(IMG_UINTPTR_T)pHash, (IMG_UINTPTR_T)pKey));

	PVR_ASSERT (pHash != IMG_NULL);

	if (pHash == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "HASH_Retrieve_Extended: Null hash table"));
		return 0;
	}

	uIndex = KEY_TO_INDEX(pHash, pKey, pHash->uSize);

	for (ppBucket = &(pHash->ppBucketTable[uIndex]); *ppBucket != IMG_NULL; ppBucket = &((*ppBucket)->pNext))
	{
		 
		if (KEY_COMPARE(pHash, (*ppBucket)->k, pKey))
		{
			BUCKET *pBucket = *ppBucket;
			IMG_UINTPTR_T v = pBucket->v;

			PVR_DPF ((PVR_DBG_MESSAGE,
                      "HASH_Retrieve: Hash=0x%x, pKey=0x%x = 0x%x",
                      (IMG_UINTPTR_T)pHash, (IMG_UINTPTR_T)pKey, v));
			return v;
		}
	}
	PVR_DPF ((PVR_DBG_MESSAGE,
              "HASH_Retrieve: Hash=0x%x, pKey=0x%x = 0x0 !!!!",
              (IMG_UINTPTR_T)pHash, (IMG_UINTPTR_T)pKey));
	return 0;
}

IMG_UINTPTR_T
HASH_Retrieve (HASH_TABLE *pHash, IMG_UINTPTR_T k)
{
	PVR_DPF ((PVR_DBG_MESSAGE, "HASH_Retrieve: Hash=0x%x, k=0x%x",
			(IMG_UINTPTR_T)pHash, k));
	return HASH_Retrieve_Extended(pHash, &k);
}

#ifdef HASH_TRACE
IMG_VOID
HASH_Dump (HASH_TABLE *pHash)
{
	IMG_UINT32 uIndex;
	IMG_UINT32 uMaxLength=0;
	IMG_UINT32 uEmptyCount=0;

	PVR_ASSERT (pHash != IMG_NULL);
	for (uIndex=0; uIndex<pHash->uSize; uIndex++)
	{
		BUCKET *pBucket;
		IMG_UINT32 uLength = 0;
		if (pHash->ppBucketTable[uIndex] == IMG_NULL)
		{
			uEmptyCount++;
		}
		for (pBucket=pHash->ppBucketTable[uIndex];
				pBucket != IMG_NULL;
				pBucket = pBucket->pNext)
		{
			uLength++;
		}
		uMaxLength = PRIVATE_MAX (uMaxLength, uLength);
	}

	PVR_TRACE(("hash table: uMinimumSize=%d  size=%d  count=%d",
			pHash->uMinimumSize, pHash->uSize, pHash->uCount));
	PVR_TRACE(("  empty=%d  max=%d", uEmptyCount, uMaxLength));
}
#endif
