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

#ifndef __INCLUDED_PRIVATE_DATA_H_
#define __INCLUDED_PRIVATE_DATA_H_

#if defined(SUPPORT_DRI_DRM) && defined(PVR_SECURE_DRM_AUTH_EXPORT)
#include <linux/list.h>
#include <drm/drmP.h>
#endif

#if defined(SUPPORT_DRI_DRM) && defined(PVR_LINUX_USING_WORKQUEUES)
#include <linux/workqueue.h>
#endif

typedef struct
{
	
	IMG_UINT32 ui32OpenPID;

#if defined(PVR_SECURE_FD_EXPORT)
	
	IMG_HANDLE hKernelMemInfo;
#endif 

#if defined(SUPPORT_DRI_DRM)
#if defined(PVR_SECURE_DRM_AUTH_EXPORT)
	struct drm_file *psDRMFile;

	
	struct list_head sDRMAuthListItem;
#endif

#if defined(PVR_LINUX_USING_WORKQUEUES)
	struct work_struct sReleaseWork;
#endif

#if defined(SUPPORT_DRI_DRM_EXT)
	IMG_PVOID pPriv;	
#endif
#endif	

#if defined(SUPPORT_MEMINFO_IDS)
	
	IMG_UINT64 ui64Stamp;
#endif 

	
	IMG_HANDLE hBlockAlloc;
}
PVRSRV_FILE_PRIVATE_DATA;

#endif 

