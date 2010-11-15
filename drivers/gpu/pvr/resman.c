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
#include "resman.h"

#ifdef __linux__
#ifndef AUTOCONF_INCLUDED
 #include <linux/config.h>
#endif

#include <linux/version.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
#include <linux/hardirq.h>
#else
#include <asm/hardirq.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

static DECLARE_MUTEX(lock);

#define ACQUIRE_SYNC_OBJ  do {							\
		if (in_interrupt()) { 							\
			printk ("ISR cannot take RESMAN mutex\n"); 	\
			BUG(); 										\
		} 												\
		else down (&lock); 								\
} while (0)
#define RELEASE_SYNC_OBJ up (&lock)

#else

#define ACQUIRE_SYNC_OBJ
#define RELEASE_SYNC_OBJ

#endif

#define RESMAN_SIGNATURE 0x12345678

typedef struct _RESMAN_ITEM_
{
#ifdef DEBUG
	IMG_UINT32				ui32Signature;
#endif
	struct _RESMAN_ITEM_	**ppsThis;	
	struct _RESMAN_ITEM_	*psNext;	

	IMG_UINT32				ui32Flags;	
	IMG_UINT32				ui32ResType;

	IMG_PVOID				pvParam;	
	IMG_UINT32				ui32Param;	

	RESMAN_FREE_FN			pfnFreeResource;
} RESMAN_ITEM;


typedef struct _RESMAN_CONTEXT_
{
#ifdef DEBUG
	IMG_UINT32					ui32Signature;
#endif
	struct	_RESMAN_CONTEXT_	**ppsThis;
	struct	_RESMAN_CONTEXT_	*psNext;

	PVRSRV_PER_PROCESS_DATA		*psPerProc; 

	RESMAN_ITEM					*psResItemList;

} RESMAN_CONTEXT;


typedef struct
{
	RESMAN_CONTEXT	*psContextList; 

} RESMAN_LIST, *PRESMAN_LIST;	


PRESMAN_LIST	gpsResList = IMG_NULL;

#include "lists.h"	 

static IMPLEMENT_LIST_ANY_VA(RESMAN_ITEM)
static IMPLEMENT_LIST_ANY_VA_2(RESMAN_ITEM, IMG_BOOL, IMG_FALSE)
static IMPLEMENT_LIST_INSERT(RESMAN_ITEM)
static IMPLEMENT_LIST_REMOVE(RESMAN_ITEM)
static IMPLEMENT_LIST_REVERSE(RESMAN_ITEM)

static IMPLEMENT_LIST_REMOVE(RESMAN_CONTEXT)
static IMPLEMENT_LIST_INSERT(RESMAN_CONTEXT)


#define PRINT_RESLIST(x, y, z)

static PVRSRV_ERROR FreeResourceByPtr(RESMAN_ITEM *psItem, IMG_BOOL bExecuteCallback);

static PVRSRV_ERROR FreeResourceByCriteria(PRESMAN_CONTEXT	psContext,
										   IMG_UINT32		ui32SearchCriteria,
										   IMG_UINT32		ui32ResType,
										   IMG_PVOID		pvParam,
										   IMG_UINT32		ui32Param,
										   IMG_BOOL			bExecuteCallback);


#ifdef DEBUG
	static IMG_VOID ValidateResList(PRESMAN_LIST psResList);
	#define VALIDATERESLIST() ValidateResList(gpsResList)
#else
	#define VALIDATERESLIST()
#endif






PVRSRV_ERROR ResManInit(IMG_VOID)
{
	if (gpsResList == IMG_NULL)
	{
		
		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(*gpsResList),
						(IMG_VOID **)&gpsResList, IMG_NULL,
						"Resource Manager List") != PVRSRV_OK)
		{
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		
		gpsResList->psContextList = IMG_NULL;

		
		VALIDATERESLIST();
	}

	return PVRSRV_OK;
}


IMG_VOID ResManDeInit(IMG_VOID)
{
	if (gpsResList != IMG_NULL)
	{
		
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*gpsResList), gpsResList, IMG_NULL);
		gpsResList = IMG_NULL;
	}
}


PVRSRV_ERROR PVRSRVResManConnect(IMG_HANDLE			hPerProc,
								 PRESMAN_CONTEXT	*phResManContext)
{
	PVRSRV_ERROR	eError;
	PRESMAN_CONTEXT	psResManContext;

	
	ACQUIRE_SYNC_OBJ;

	
	VALIDATERESLIST();

	
	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psResManContext),
						(IMG_VOID **)&psResManContext, IMG_NULL,
						"Resource Manager Context");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVResManConnect: ERROR allocating new RESMAN context struct"));

		
		VALIDATERESLIST();

		
		RELEASE_SYNC_OBJ;

		return eError;
	}

#ifdef DEBUG
	psResManContext->ui32Signature = RESMAN_SIGNATURE;
#endif 
	psResManContext->psResItemList	= IMG_NULL;
	psResManContext->psPerProc = hPerProc;

	
	List_RESMAN_CONTEXT_Insert(&gpsResList->psContextList, psResManContext);

	
	VALIDATERESLIST();

	
	RELEASE_SYNC_OBJ;

	*phResManContext = psResManContext;

	return PVRSRV_OK;
}


IMG_VOID PVRSRVResManDisconnect(PRESMAN_CONTEXT psResManContext,
								IMG_BOOL		bKernelContext)
{
	
	ACQUIRE_SYNC_OBJ;

	
	VALIDATERESLIST();

	
	PRINT_RESLIST(gpsResList, psResManContext, IMG_TRUE);

	

	if (!bKernelContext)
	{
		
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_OS_USERMODE_MAPPING, 0, 0, IMG_TRUE);

		
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_EVENT_OBJECT, 0, 0, IMG_TRUE);

		
		
		List_RESMAN_ITEM_Reverse(&psResManContext->psResItemList);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_MODIFY_SYNC_OPS, 0, 0, IMG_TRUE);
		List_RESMAN_ITEM_Reverse(&psResManContext->psResItemList);  

		
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_HW_RENDER_CONTEXT, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_HW_TRANSFER_CONTEXT, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_HW_2D_CONTEXT, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_TRANSFER_CONTEXT, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_SHARED_PB_DESC_CREATE_LOCK, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_SHARED_PB_DESC, 0, 0, IMG_TRUE);

		

		
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_DISPLAYCLASS_SWAPCHAIN_REF, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_DISPLAYCLASS_DEVICE, 0, 0, IMG_TRUE);

		
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_BUFFERCLASS_DEVICE, 0, 0, IMG_TRUE);

		
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_SYNC_INFO, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_DEVICECLASSMEM_MAPPING, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_DEVICEMEM_WRAP, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_DEVICEMEM_MAPPING, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_KERNEL_DEVICEMEM_ALLOCATION, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_DEVICEMEM_ALLOCATION, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_DEVICEMEM_CONTEXT, 0, 0, IMG_TRUE);
		FreeResourceByCriteria(psResManContext, RESMAN_CRITERIA_RESTYPE, RESMAN_TYPE_SHARED_MEM_INFO, 0, 0, IMG_TRUE);
	}

	
	PVR_ASSERT(psResManContext->psResItemList == IMG_NULL);

	
	List_RESMAN_CONTEXT_Remove(psResManContext);

	
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(RESMAN_CONTEXT), psResManContext, IMG_NULL);
	


	
	VALIDATERESLIST();

	
	PRINT_RESLIST(gpsResList, psResManContext, IMG_FALSE);

	
	RELEASE_SYNC_OBJ;
}


PRESMAN_ITEM ResManRegisterRes(PRESMAN_CONTEXT	psResManContext,
							   IMG_UINT32		ui32ResType,
							   IMG_PVOID		pvParam,
							   IMG_UINT32		ui32Param,
							   RESMAN_FREE_FN	pfnFreeResource)
{
	PRESMAN_ITEM	psNewResItem;

	PVR_ASSERT(psResManContext != IMG_NULL);
	PVR_ASSERT(ui32ResType != 0);

	if (psResManContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "ResManRegisterRes: invalid parameter - psResManContext"));
		return (PRESMAN_ITEM) IMG_NULL;
	}

	
	ACQUIRE_SYNC_OBJ;

	
	VALIDATERESLIST();

	PVR_DPF((PVR_DBG_MESSAGE, "ResManRegisterRes: register resource "
			"Context 0x%x, ResType 0x%x, pvParam 0x%x, ui32Param 0x%x, "
			"FreeFunc %08X",
			(IMG_UINTPTR_T)psResManContext,
			ui32ResType,
			(IMG_UINTPTR_T)pvParam,
			ui32Param,
			(IMG_UINTPTR_T)pfnFreeResource));

	
	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				   sizeof(RESMAN_ITEM), (IMG_VOID **)&psNewResItem,
				   IMG_NULL,
				   "Resource Manager Item") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "ResManRegisterRes: "
				"ERROR allocating new resource item"));

		
		RELEASE_SYNC_OBJ;

		return((PRESMAN_ITEM)IMG_NULL);
	}

	
#ifdef DEBUG
	psNewResItem->ui32Signature		= RESMAN_SIGNATURE;
#endif 
	psNewResItem->ui32ResType		= ui32ResType;
	psNewResItem->pvParam			= pvParam;
	psNewResItem->ui32Param			= ui32Param;
	psNewResItem->pfnFreeResource	= pfnFreeResource;
	psNewResItem->ui32Flags		    = 0;

	
	List_RESMAN_ITEM_Insert(&psResManContext->psResItemList, psNewResItem);

	
	VALIDATERESLIST();

	
	RELEASE_SYNC_OBJ;

	return(psNewResItem);
}

PVRSRV_ERROR ResManFreeResByPtr(RESMAN_ITEM	*psResItem)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(psResItem != IMG_NULL);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "ResManFreeResByPtr: NULL ptr - nothing to do"));
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "ResManFreeResByPtr: freeing resource at %08X",
			(IMG_UINTPTR_T)psResItem));

	
	ACQUIRE_SYNC_OBJ;

	
	VALIDATERESLIST();

	
	eError = FreeResourceByPtr(psResItem, IMG_TRUE);

	
	VALIDATERESLIST();

	
	RELEASE_SYNC_OBJ;

	return(eError);
}


PVRSRV_ERROR ResManFreeResByCriteria(PRESMAN_CONTEXT	psResManContext,
									 IMG_UINT32			ui32SearchCriteria,
									 IMG_UINT32			ui32ResType,
									 IMG_PVOID			pvParam,
									 IMG_UINT32			ui32Param)
{
	PVRSRV_ERROR	eError;

	PVR_ASSERT(psResManContext != IMG_NULL);

	
	ACQUIRE_SYNC_OBJ;

	
	VALIDATERESLIST();

	PVR_DPF((PVR_DBG_MESSAGE, "ResManFreeResByCriteria: "
			"Context 0x%x, Criteria 0x%x, Type 0x%x, Addr 0x%x, Param 0x%x",
			(IMG_UINTPTR_T)psResManContext, ui32SearchCriteria, ui32ResType,
			(IMG_UINTPTR_T)pvParam, ui32Param));

	
	eError = FreeResourceByCriteria(psResManContext, ui32SearchCriteria,
									ui32ResType, pvParam, ui32Param,
									IMG_TRUE);

	
	VALIDATERESLIST();

	
	RELEASE_SYNC_OBJ;

	return eError;
}


PVRSRV_ERROR ResManDissociateRes(RESMAN_ITEM		*psResItem,
							 PRESMAN_CONTEXT	psNewResManContext)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_ASSERT(psResItem != IMG_NULL);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "ResManDissociateRes: invalid parameter - psResItem"));
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#ifdef DEBUG 
	PVR_ASSERT(psResItem->ui32Signature == RESMAN_SIGNATURE);
#endif

	if (psNewResManContext != IMG_NULL)
	{
		
		List_RESMAN_ITEM_Remove(psResItem);

		
		List_RESMAN_ITEM_Insert(&psNewResManContext->psResItemList, psResItem);

	}
	else
	{
		eError = FreeResourceByPtr(psResItem, IMG_FALSE);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "ResManDissociateRes: failed to free resource by pointer"));
			return eError;
		}
	}

	return eError;
}

static IMG_BOOL ResManFindResourceByPtr_AnyVaCb(RESMAN_ITEM *psCurItem, va_list va)
{
	RESMAN_ITEM		*psItem;

	psItem = va_arg(va, RESMAN_ITEM*);

	return (IMG_BOOL)(psCurItem == psItem);
}


IMG_INTERNAL PVRSRV_ERROR ResManFindResourceByPtr(PRESMAN_CONTEXT	psResManContext,
												  RESMAN_ITEM		*psItem)
{
	PVRSRV_ERROR	eResult;

	PVR_ASSERT(psResManContext != IMG_NULL);
	PVR_ASSERT(psItem != IMG_NULL);

	if ((psItem == IMG_NULL) || (psResManContext == IMG_NULL))
	{
		PVR_DPF((PVR_DBG_ERROR, "ResManFindResourceByPtr: invalid parameter"));
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#ifdef DEBUG	
	PVR_ASSERT(psItem->ui32Signature == RESMAN_SIGNATURE);
#endif

	
	ACQUIRE_SYNC_OBJ;

	PVR_DPF((PVR_DBG_MESSAGE,
			"FindResourceByPtr: psItem=%08X, psItem->psNext=%08X",
			(IMG_UINTPTR_T)psItem, (IMG_UINTPTR_T)psItem->psNext));

	PVR_DPF((PVR_DBG_MESSAGE,
			"FindResourceByPtr: Resource Ctx 0x%x, Type 0x%x, Addr 0x%x, "
			"Param 0x%x, FnCall %08X, Flags 0x%x",
			(IMG_UINTPTR_T)psResManContext,
			psItem->ui32ResType,
			(IMG_UINTPTR_T)psItem->pvParam,
			psItem->ui32Param,
			(IMG_UINTPTR_T)psItem->pfnFreeResource,
			psItem->ui32Flags));

	
	if(List_RESMAN_ITEM_IMG_BOOL_Any_va(psResManContext->psResItemList,
										&ResManFindResourceByPtr_AnyVaCb,
										psItem))
	{
		eResult = PVRSRV_OK;
	}
	else
	{
		eResult = PVRSRV_ERROR_NOT_OWNER;
	}

	
	RELEASE_SYNC_OBJ;

	return eResult;
}

static PVRSRV_ERROR FreeResourceByPtr(RESMAN_ITEM	*psItem,
									  IMG_BOOL		bExecuteCallback)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(psItem != IMG_NULL);

	if (psItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeResourceByPtr: invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#ifdef DEBUG	
	PVR_ASSERT(psItem->ui32Signature == RESMAN_SIGNATURE);
#endif

	PVR_DPF((PVR_DBG_MESSAGE,
			"FreeResourceByPtr: psItem=%08X, psItem->psNext=%08X",
			(IMG_UINTPTR_T)psItem, (IMG_UINTPTR_T)psItem->psNext));

	PVR_DPF((PVR_DBG_MESSAGE,
			"FreeResourceByPtr: Type 0x%x, Addr 0x%x, "
			"Param 0x%x, FnCall %08X, Flags 0x%x",
			psItem->ui32ResType,
			(IMG_UINTPTR_T)psItem->pvParam, psItem->ui32Param,
			(IMG_UINTPTR_T)psItem->pfnFreeResource, psItem->ui32Flags));

	
	List_RESMAN_ITEM_Remove(psItem);


	
	RELEASE_SYNC_OBJ;

	
	if (bExecuteCallback)
	{
		eError = psItem->pfnFreeResource(psItem->pvParam, psItem->ui32Param);
	 	if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "FreeResourceByPtr: ERROR calling FreeResource function"));
		}
	}

	
	ACQUIRE_SYNC_OBJ;

	
	eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(RESMAN_ITEM), psItem, IMG_NULL);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeResourceByPtr: ERROR freeing resource list item memory"));
	}

	return(eError);
}

static IMG_VOID* FreeResourceByCriteria_AnyVaCb(RESMAN_ITEM *psCurItem, va_list va)
{
	IMG_UINT32 ui32SearchCriteria;
	IMG_UINT32 ui32ResType;
	IMG_PVOID pvParam;
	IMG_UINT32 ui32Param;

	ui32SearchCriteria = va_arg(va, IMG_UINT32);
	ui32ResType = va_arg(va, IMG_UINT32);
	pvParam = va_arg(va, IMG_PVOID);
	ui32Param = va_arg(va, IMG_UINT32);

	
	if(
	
		(((ui32SearchCriteria & RESMAN_CRITERIA_RESTYPE) == 0UL) ||
		(psCurItem->ui32ResType == ui32ResType))
	&&
	
		(((ui32SearchCriteria & RESMAN_CRITERIA_PVOID_PARAM) == 0UL) ||
			 (psCurItem->pvParam == pvParam))
	&&
	
		(((ui32SearchCriteria & RESMAN_CRITERIA_UI32_PARAM) == 0UL) ||
			 (psCurItem->ui32Param == ui32Param))
		)
	{
		return psCurItem;
	}
	else
	{
		return IMG_NULL;
	}
}

static PVRSRV_ERROR FreeResourceByCriteria(PRESMAN_CONTEXT	psResManContext,
										   IMG_UINT32		ui32SearchCriteria,
										   IMG_UINT32		ui32ResType,
										   IMG_PVOID		pvParam,
										   IMG_UINT32		ui32Param,
										   IMG_BOOL			bExecuteCallback)
{
	PRESMAN_ITEM	psCurItem;
	PVRSRV_ERROR	eError = PVRSRV_OK;

	
	
	while((psCurItem = (PRESMAN_ITEM)
				List_RESMAN_ITEM_Any_va(psResManContext->psResItemList,
										&FreeResourceByCriteria_AnyVaCb,
										ui32SearchCriteria,
										ui32ResType,
						 				pvParam,
						 				ui32Param)) != IMG_NULL
		  	&& eError == PVRSRV_OK)
	{
		eError = FreeResourceByPtr(psCurItem, bExecuteCallback);
	}

	return eError;
}


#ifdef DEBUG
static IMG_VOID ValidateResList(PRESMAN_LIST psResList)
{
	PRESMAN_ITEM	psCurItem, *ppsThisItem;
	PRESMAN_CONTEXT	psCurContext, *ppsThisContext;

	
	if (psResList == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "ValidateResList: resman not initialised yet"));
		return;
	}

	psCurContext = psResList->psContextList;
	ppsThisContext = &psResList->psContextList;

	
	while(psCurContext != IMG_NULL)
	{
		
		PVR_ASSERT(psCurContext->ui32Signature == RESMAN_SIGNATURE);
		if (psCurContext->ppsThis != ppsThisContext)
		{
			PVR_DPF((PVR_DBG_WARNING,
					"psCC=%08X psCC->ppsThis=%08X psCC->psNext=%08X ppsTC=%08X",
					(IMG_UINTPTR_T)psCurContext,
					(IMG_UINTPTR_T)psCurContext->ppsThis,
					(IMG_UINTPTR_T)psCurContext->psNext,
					(IMG_UINTPTR_T)ppsThisContext));
			PVR_ASSERT(psCurContext->ppsThis == ppsThisContext);
		}

		
		psCurItem = psCurContext->psResItemList;
		ppsThisItem = &psCurContext->psResItemList;
		while(psCurItem != IMG_NULL)
		{
			
			PVR_ASSERT(psCurItem->ui32Signature == RESMAN_SIGNATURE);
			if (psCurItem->ppsThis != ppsThisItem)
			{
				PVR_DPF((PVR_DBG_WARNING,
						"psCurItem=%08X psCurItem->ppsThis=%08X psCurItem->psNext=%08X ppsThisItem=%08X",
						(IMG_UINTPTR_T)psCurItem,
						(IMG_UINTPTR_T)psCurItem->ppsThis,
						(IMG_UINTPTR_T)psCurItem->psNext,
						(IMG_UINTPTR_T)ppsThisItem));
				PVR_ASSERT(psCurItem->ppsThis == ppsThisItem);
			}

			
			ppsThisItem = &psCurItem->psNext;
			psCurItem = psCurItem->psNext;
		}

		
		ppsThisContext = &psCurContext->psNext;
		psCurContext = psCurContext->psNext;
	}
}
#endif 


