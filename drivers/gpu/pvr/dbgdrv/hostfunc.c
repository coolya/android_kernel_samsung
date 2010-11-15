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

#include <linux/version.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))
#include <linux/mutex.h>
#else
#include <asm/semaphore.h>
#endif
#include <linux/hardirq.h>

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#endif	

#include "img_types.h"
#include "pvr_debug.h"

#include "dbgdrvif.h"
#include "hostfunc.h"
#include "dbgdriv.h"

#if defined(DEBUG) && !defined(SUPPORT_DRI_DRM)
IMG_UINT32	gPVRDebugLevel = (DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING);

#define PVR_STRING_TERMINATOR		'\0'
#define PVR_IS_FILE_SEPARATOR(character) ( ((character) == '\\') || ((character) == '/') )

void PVRSRVDebugPrintf	(
						IMG_UINT32	ui32DebugLevel,
						const IMG_CHAR*	pszFileName,
						IMG_UINT32	ui32Line,
						const IMG_CHAR*	pszFormat,
						...
					)
{
	IMG_BOOL bTrace;
#if !defined(__sh__)
	IMG_CHAR *pszLeafName;

	pszLeafName = (char *)strrchr (pszFileName, '\\');

	if (pszLeafName)
	{
		pszFileName = pszLeafName;
	}
#endif 

	bTrace = (IMG_BOOL)(ui32DebugLevel & DBGPRIV_CALLTRACE) ? IMG_TRUE : IMG_FALSE;

	if (gPVRDebugLevel & ui32DebugLevel)
	{
		va_list vaArgs;
		static char szBuffer[256];

		va_start (vaArgs, pszFormat);

		
		if (bTrace == IMG_FALSE)
		{
			switch(ui32DebugLevel)
			{
				case DBGPRIV_FATAL:
				{
					strcpy (szBuffer, "PVR_K:(Fatal): ");
					break;
				}
				case DBGPRIV_ERROR:
				{
					strcpy (szBuffer, "PVR_K:(Error): ");
					break;
				}
				case DBGPRIV_WARNING:
				{
					strcpy (szBuffer, "PVR_K:(Warning): ");
					break;
				}
				case DBGPRIV_MESSAGE:
				{
					strcpy (szBuffer, "PVR_K:(Message): ");
					break;
				}
				case DBGPRIV_VERBOSE:
				{
					strcpy (szBuffer, "PVR_K:(Verbose): ");
					break;
				}
				default:
				{
					strcpy (szBuffer, "PVR_K:(Unknown message level)");
					break;
				}
			}
		}
		else
		{
			strcpy (szBuffer, "PVR_K: ");
		}

		vsprintf (&szBuffer[strlen(szBuffer)], pszFormat, vaArgs);

 		
 		if (bTrace == IMG_FALSE)
		{
			sprintf (&szBuffer[strlen(szBuffer)], " [%d, %s]", (int)ui32Line, pszFileName);
		}

		printk(KERN_INFO "%s\r\n", szBuffer);

		va_end (vaArgs);
	}
}
#endif	

IMG_VOID HostMemSet(IMG_VOID *pvDest, IMG_UINT8 ui8Value, IMG_UINT32 ui32Size)
{
	memset(pvDest, (int) ui8Value, (size_t) ui32Size);
}

IMG_VOID HostMemCopy(IMG_VOID *pvDst, IMG_VOID *pvSrc, IMG_UINT32 ui32Size)
{
#if defined(USE_UNOPTIMISED_MEMCPY)
    unsigned char *src,*dst;
    int i;

    src=(unsigned char *)pvSrc;
    dst=(unsigned char *)pvDst;
    for(i=0;i<ui32Size;i++)
    {
        dst[i]=src[i];
    }
#else
    memcpy(pvDst, pvSrc, ui32Size);
#endif
}

IMG_UINT32 HostReadRegistryDWORDFromString(char *pcKey, char *pcValueName, IMG_UINT32 *pui32Data)
{
    
	return 0;
}

IMG_VOID * HostPageablePageAlloc(IMG_UINT32 ui32Pages)
{
    return (void*)vmalloc(ui32Pages * PAGE_SIZE);
}

IMG_VOID HostPageablePageFree(IMG_VOID * pvBase)
{
    vfree(pvBase);
}

IMG_VOID * HostNonPageablePageAlloc(IMG_UINT32 ui32Pages)
{
    return (void*)vmalloc(ui32Pages * PAGE_SIZE);
}

IMG_VOID HostNonPageablePageFree(IMG_VOID * pvBase)
{
    vfree(pvBase);
}

IMG_VOID * HostMapKrnBufIntoUser(IMG_VOID * pvKrnAddr, IMG_UINT32 ui32Size, IMG_VOID **ppvMdl)
{
    
	return IMG_NULL;
}

IMG_VOID HostUnMapKrnBufFromUser(IMG_VOID * pvUserAddr, IMG_VOID * pvMdl, IMG_VOID * pvProcess)
{
    
}

IMG_VOID HostCreateRegDeclStreams(IMG_VOID)
{
    
}

IMG_VOID * HostCreateMutex(IMG_VOID)
{
	struct semaphore *psSem;

	psSem = kmalloc(sizeof(*psSem), GFP_KERNEL);
	if (psSem)
	{
		init_MUTEX(psSem);
	}

	return psSem;
}

IMG_VOID HostAquireMutex(IMG_VOID * pvMutex)
{
	BUG_ON(in_interrupt());

#if defined(PVR_DEBUG_DBGDRV_DETECT_HOST_MUTEX_COLLISIONS)
	if (down_trylock((struct semaphore *)pvMutex))
	{
		printk(KERN_INFO "HostAquireMutex: Waiting for mutex\n");
		down((struct semaphore *)pvMutex);
	}
#else
	down((struct semaphore *)pvMutex);
#endif
}

IMG_VOID HostReleaseMutex(IMG_VOID * pvMutex)
{
	up((struct semaphore *)pvMutex);
}

IMG_VOID HostDestroyMutex(IMG_VOID * pvMutex)
{
	if (pvMutex)
	{
		kfree(pvMutex);
	}
}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)

#define	EVENT_WAIT_TIMEOUT_MS	500
#define	EVENT_WAIT_TIMEOUT_JIFFIES	(EVENT_WAIT_TIMEOUT_MS * HZ / 1000)

static int iStreamData;
static wait_queue_head_t sStreamDataEvent;

IMG_INT32 HostCreateEventObjects(IMG_VOID)
{
	init_waitqueue_head(&sStreamDataEvent);

	return 0;
}

IMG_VOID HostWaitForEvent(DBG_EVENT eEvent)
{
	switch(eEvent)
	{
		case DBG_EVENT_STREAM_DATA:
			
			wait_event_interruptible_timeout(sStreamDataEvent, iStreamData != 0, EVENT_WAIT_TIMEOUT_JIFFIES);
			iStreamData = 0;
			break;
		default:
			
			msleep_interruptible(EVENT_WAIT_TIMEOUT_MS);
			break;
	}
}

IMG_VOID HostSignalEvent(DBG_EVENT eEvent)
{
	switch(eEvent)
	{
		case DBG_EVENT_STREAM_DATA:
			iStreamData = 1;
			wake_up_interruptible(&sStreamDataEvent);
			break;
		default:
			break;
	}
}

IMG_VOID HostDestroyEventObjects(IMG_VOID)
{
}
#endif	
