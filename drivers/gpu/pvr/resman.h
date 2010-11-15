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

#ifndef __RESMAN_H__
#define __RESMAN_H__

#if defined (__cplusplus)
extern "C" {
#endif

enum {
	
	RESMAN_TYPE_SHARED_PB_DESC = 1,					
	RESMAN_TYPE_SHARED_PB_DESC_CREATE_LOCK,				
	RESMAN_TYPE_HW_RENDER_CONTEXT,					
	RESMAN_TYPE_HW_TRANSFER_CONTEXT,				
	RESMAN_TYPE_HW_2D_CONTEXT,						
	RESMAN_TYPE_TRANSFER_CONTEXT,					

	
	
	
	
	RESMAN_TYPE_DISPLAYCLASS_SWAPCHAIN_REF,			
	RESMAN_TYPE_DISPLAYCLASS_DEVICE,				

	
	RESMAN_TYPE_BUFFERCLASS_DEVICE,					
	
	
	RESMAN_TYPE_OS_USERMODE_MAPPING,				
	
	
	RESMAN_TYPE_DEVICEMEM_CONTEXT,					
	RESMAN_TYPE_DEVICECLASSMEM_MAPPING,				
	RESMAN_TYPE_DEVICEMEM_MAPPING,					
	RESMAN_TYPE_DEVICEMEM_WRAP,						
	RESMAN_TYPE_DEVICEMEM_ALLOCATION,				
	RESMAN_TYPE_EVENT_OBJECT,						
    RESMAN_TYPE_SHARED_MEM_INFO,                    
    RESMAN_TYPE_MODIFY_SYNC_OPS,					
    RESMAN_TYPE_SYNC_INFO,					        
	
	
	RESMAN_TYPE_KERNEL_DEVICEMEM_ALLOCATION			
};

#define RESMAN_CRITERIA_ALL				0x00000000	
#define RESMAN_CRITERIA_RESTYPE			0x00000001	
#define RESMAN_CRITERIA_PVOID_PARAM		0x00000002	
#define RESMAN_CRITERIA_UI32_PARAM		0x00000004	

typedef PVRSRV_ERROR (*RESMAN_FREE_FN)(IMG_PVOID pvParam, IMG_UINT32 ui32Param); 

typedef struct _RESMAN_ITEM_ *PRESMAN_ITEM;
typedef struct _RESMAN_CONTEXT_ *PRESMAN_CONTEXT;

PVRSRV_ERROR ResManInit(IMG_VOID);
IMG_VOID ResManDeInit(IMG_VOID);

PRESMAN_ITEM ResManRegisterRes(PRESMAN_CONTEXT	hResManContext,
							   IMG_UINT32		ui32ResType, 
							   IMG_PVOID		pvParam, 
							   IMG_UINT32		ui32Param, 
							   RESMAN_FREE_FN	pfnFreeResource);

PVRSRV_ERROR ResManFreeResByPtr(PRESMAN_ITEM	psResItem);

PVRSRV_ERROR ResManFreeResByCriteria(PRESMAN_CONTEXT	hResManContext,
									 IMG_UINT32			ui32SearchCriteria, 
									 IMG_UINT32			ui32ResType, 
									 IMG_PVOID			pvParam, 
									 IMG_UINT32			ui32Param);

PVRSRV_ERROR ResManDissociateRes(PRESMAN_ITEM		psResItem,
							 PRESMAN_CONTEXT	psNewResManContext);

PVRSRV_ERROR ResManFindResourceByPtr(PRESMAN_CONTEXT	hResManContext,
									 PRESMAN_ITEM		psItem);

PVRSRV_ERROR PVRSRVResManConnect(IMG_HANDLE			hPerProc,
								 PRESMAN_CONTEXT	*phResManContext);
IMG_VOID PVRSRVResManDisconnect(PRESMAN_CONTEXT hResManContext,
								IMG_BOOL		bKernelContext);

#if defined (__cplusplus)
}
#endif

#endif 

