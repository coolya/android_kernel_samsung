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

#ifndef _HOSTFUNC_
#define _HOSTFUNC_

#define HOST_PAGESIZE			(4096)
#define DBG_MEMORY_INITIALIZER	(0xe2)

IMG_UINT32 HostReadRegistryDWORDFromString(IMG_CHAR *pcKey, IMG_CHAR *pcValueName, IMG_UINT32 *pui32Data);

IMG_VOID * HostPageablePageAlloc(IMG_UINT32 ui32Pages);
IMG_VOID HostPageablePageFree(IMG_VOID * pvBase);
IMG_VOID * HostNonPageablePageAlloc(IMG_UINT32 ui32Pages);
IMG_VOID HostNonPageablePageFree(IMG_VOID * pvBase);

IMG_VOID * HostMapKrnBufIntoUser(IMG_VOID * pvKrnAddr, IMG_UINT32 ui32Size, IMG_VOID * *ppvMdl);
IMG_VOID HostUnMapKrnBufFromUser(IMG_VOID * pvUserAddr, IMG_VOID * pvMdl, IMG_VOID * pvProcess);

IMG_VOID HostCreateRegDeclStreams(IMG_VOID);

IMG_VOID * HostCreateMutex(IMG_VOID);
IMG_VOID HostAquireMutex(IMG_VOID * pvMutex);
IMG_VOID HostReleaseMutex(IMG_VOID * pvMutex);
IMG_VOID HostDestroyMutex(IMG_VOID * pvMutex);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
IMG_INT32 HostCreateEventObjects(IMG_VOID);
IMG_VOID HostWaitForEvent(DBG_EVENT eEvent);
IMG_VOID HostSignalEvent(DBG_EVENT eEvent);
IMG_VOID HostDestroyEventObjects(IMG_VOID);
#endif	

#endif

