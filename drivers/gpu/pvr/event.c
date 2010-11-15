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

#ifndef AUTOCONF_INCLUDED
 #include <linux/config.h>
#endif

#include <linux/version.h>
#include <asm/io.h>
#include <asm/page.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
#include <asm/system.h>
#endif
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <linux/string.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/hardirq.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/capability.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "img_types.h"
#include "services_headers.h"
#include "mm.h"
#include "pvrmmap.h"
#include "mmap.h"
#include "env_data.h"
#include "proc.h"
#include "mutex.h"
#include "lock.h"
#include "event.h"

typedef struct PVRSRV_LINUX_EVENT_OBJECT_LIST_TAG
{
   rwlock_t		sLock;
   struct list_head	sList;
   
} PVRSRV_LINUX_EVENT_OBJECT_LIST;


typedef struct PVRSRV_LINUX_EVENT_OBJECT_TAG
{
   	atomic_t	sTimeStamp;
   	IMG_UINT32  ui32TimeStampPrevious;
#if defined(DEBUG)
	IMG_UINT	ui32Stats;
#endif
    wait_queue_head_t sWait;	
	struct list_head        sList;
	IMG_HANDLE		hResItem;				
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList;
} PVRSRV_LINUX_EVENT_OBJECT;

PVRSRV_ERROR LinuxEventObjectListCreate(IMG_HANDLE *phEventObjectList)
{
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psEventObjectList;

	if(OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT_LIST), 
		(IMG_VOID **)&psEventObjectList, IMG_NULL,
		"Linux Event Object List") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectCreate: failed to allocate memory for event list"));		
		return PVRSRV_ERROR_OUT_OF_MEMORY;	
	}

	INIT_LIST_HEAD(&psEventObjectList->sList);

	rwlock_init(&psEventObjectList->sLock);
	
	*phEventObjectList = (IMG_HANDLE *) psEventObjectList;

	return PVRSRV_OK;
}

PVRSRV_ERROR LinuxEventObjectListDestroy(IMG_HANDLE hEventObjectList)
{

	PVRSRV_LINUX_EVENT_OBJECT_LIST *psEventObjectList = (PVRSRV_LINUX_EVENT_OBJECT_LIST *) hEventObjectList ;

	if(psEventObjectList)	
	{
		IMG_BOOL bListEmpty;

		read_lock(&psEventObjectList->sLock);
		bListEmpty = list_empty(&psEventObjectList->sList);
		read_unlock(&psEventObjectList->sLock);

		if (!bListEmpty) 
		{
			 PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectListDestroy: Event List is not empty"));
			 return PVRSRV_ERROR_UNABLE_TO_DESTROY_EVENT;
		}

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT_LIST), psEventObjectList, IMG_NULL);
		
	}

	return PVRSRV_OK;
}


PVRSRV_ERROR LinuxEventObjectDelete(IMG_HANDLE hOSEventObjectList, IMG_HANDLE hOSEventObject)
{
	if(hOSEventObjectList)
	{
		if(hOSEventObject)
		{
			PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject = (PVRSRV_LINUX_EVENT_OBJECT *)hOSEventObject; 
#if defined(DEBUG)
			PVR_DPF((PVR_DBG_MESSAGE, "LinuxEventObjectListDelete: Event object waits: %u", psLinuxEventObject->ui32Stats));
#endif
			if(ResManFreeResByPtr(psLinuxEventObject->hResItem) != PVRSRV_OK)
			{
				return PVRSRV_ERROR_UNABLE_TO_DESTROY_EVENT;
			}
			
			return PVRSRV_OK;
		}
	}
	return PVRSRV_ERROR_UNABLE_TO_DESTROY_EVENT;

}

static PVRSRV_ERROR LinuxEventObjectDeleteCallback(IMG_PVOID pvParam, IMG_UINT32 ui32Param)
{
	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject = pvParam;
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList = psLinuxEventObject->psLinuxEventObjectList;
	unsigned long ulLockFlags;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	write_lock_irqsave(&psLinuxEventObjectList->sLock, ulLockFlags);
	list_del(&psLinuxEventObject->sList);
	write_unlock_irqrestore(&psLinuxEventObjectList->sLock, ulLockFlags);

#if defined(DEBUG)
	PVR_DPF((PVR_DBG_MESSAGE, "LinuxEventObjectDeleteCallback: Event object waits: %u", psLinuxEventObject->ui32Stats));
#endif	

	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT), psLinuxEventObject, IMG_NULL);
	

	return PVRSRV_OK;
}
PVRSRV_ERROR LinuxEventObjectAdd(IMG_HANDLE hOSEventObjectList, IMG_HANDLE *phOSEventObject)
 {
	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject; 
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList = (PVRSRV_LINUX_EVENT_OBJECT_LIST*)hOSEventObjectList; 
	IMG_UINT32 ui32PID = OSGetCurrentProcessIDKM();
	PVRSRV_PER_PROCESS_DATA *psPerProc;
	unsigned long ulLockFlags;

	psPerProc = PVRSRVPerProcessData(ui32PID);
	if (psPerProc == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectAdd: Couldn't find per-process data"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	
	if(OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT), 
		(IMG_VOID **)&psLinuxEventObject, IMG_NULL,
		"Linux Event Object") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectAdd: failed to allocate memory "));		
		return PVRSRV_ERROR_OUT_OF_MEMORY;	
	}
	
	INIT_LIST_HEAD(&psLinuxEventObject->sList);

	atomic_set(&psLinuxEventObject->sTimeStamp, 0);
	psLinuxEventObject->ui32TimeStampPrevious = 0;

#if defined(DEBUG)
	psLinuxEventObject->ui32Stats = 0;
#endif
    init_waitqueue_head(&psLinuxEventObject->sWait);

	psLinuxEventObject->psLinuxEventObjectList = psLinuxEventObjectList;

	psLinuxEventObject->hResItem = ResManRegisterRes(psPerProc->hResManContext,
													 RESMAN_TYPE_EVENT_OBJECT,
													 psLinuxEventObject,
													 0,
													 &LinuxEventObjectDeleteCallback);	

	write_lock_irqsave(&psLinuxEventObjectList->sLock, ulLockFlags);
	list_add(&psLinuxEventObject->sList, &psLinuxEventObjectList->sList);
	write_unlock_irqrestore(&psLinuxEventObjectList->sLock, ulLockFlags);
	
	*phOSEventObject = psLinuxEventObject;

	return PVRSRV_OK;	 
}

PVRSRV_ERROR LinuxEventObjectSignal(IMG_HANDLE hOSEventObjectList)
{
	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject;
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList = (PVRSRV_LINUX_EVENT_OBJECT_LIST*)hOSEventObjectList; 
	struct list_head *psListEntry, *psList;

	psList = &psLinuxEventObjectList->sList;

	
	read_lock(&psLinuxEventObjectList->sLock);
	list_for_each(psListEntry, psList) 
	{       	

		psLinuxEventObject = (PVRSRV_LINUX_EVENT_OBJECT *)list_entry(psListEntry, PVRSRV_LINUX_EVENT_OBJECT, sList);	
		
		atomic_inc(&psLinuxEventObject->sTimeStamp);	
	 	wake_up_interruptible(&psLinuxEventObject->sWait);
	}
	read_unlock(&psLinuxEventObjectList->sLock);

	return 	PVRSRV_OK;
  	
}

PVRSRV_ERROR LinuxEventObjectWait(IMG_HANDLE hOSEventObject, IMG_UINT32 ui32MSTimeout)
{
	IMG_UINT32 ui32TimeStamp;
	DEFINE_WAIT(sWait);

	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject = (PVRSRV_LINUX_EVENT_OBJECT *) hOSEventObject;

	IMG_UINT32 ui32TimeOutJiffies = msecs_to_jiffies(ui32MSTimeout);
	
	do	
	{
		prepare_to_wait(&psLinuxEventObject->sWait, &sWait, TASK_INTERRUPTIBLE);
		ui32TimeStamp = (IMG_UINT32)atomic_read(&psLinuxEventObject->sTimeStamp);
   	
		if(psLinuxEventObject->ui32TimeStampPrevious != ui32TimeStamp)
		{
			break;
		}

		LinuxUnLockMutex(&gPVRSRVLock);		

		ui32TimeOutJiffies = (IMG_UINT32)schedule_timeout((IMG_INT32)ui32TimeOutJiffies);
		
		LinuxLockMutex(&gPVRSRVLock);
#if defined(DEBUG)
		psLinuxEventObject->ui32Stats++;
#endif			

		
	} while (ui32TimeOutJiffies);

	finish_wait(&psLinuxEventObject->sWait, &sWait);	

	psLinuxEventObject->ui32TimeStampPrevious = ui32TimeStamp;

	return ui32TimeOutJiffies ? PVRSRV_OK : PVRSRV_ERROR_TIMEOUT;

}

