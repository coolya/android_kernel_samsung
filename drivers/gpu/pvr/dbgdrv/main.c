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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/version.h>

#if defined(LDM_PLATFORM) && !defined(SUPPORT_DRI_DRM)
#include <linux/platform_device.h>
#endif

#if defined(LDM_PCI) && !defined(SUPPORT_DRI_DRM)
#include <linux/pci.h>
#endif

#include <asm/uaccess.h>

#if defined(SUPPORT_DRI_DRM)
#include "drmP.h"
#include "drm.h"
#endif

#include "img_types.h"
#include "linuxsrv.h"
#include "dbgdriv_ioctl.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hostfunc.h"
#include "hotkey.h"
#include "pvr_debug.h"
#include "pvrmodule.h"
#include "pvr_uaccess.h"

#if defined(SUPPORT_DRI_DRM)

#include "pvr_drm_shared.h"
#include "pvr_drm.h"

#else 

#define DRVNAME "dbgdrv"
MODULE_SUPPORTED_DEVICE(DRVNAME);

#if (defined(LDM_PLATFORM) || defined(LDM_PCI)) && !defined(SUPPORT_DRI_DRM)
static struct class *psDbgDrvClass;
#endif

static int AssignedMajorNumber = 0;

long dbgdrv_ioctl(struct file *, unsigned int, unsigned long);

static int dbgdrv_open(struct inode unref__ * pInode, struct file unref__ * pFile)
{
	return 0;
}

static int dbgdrv_release(struct inode unref__ * pInode, struct file unref__ * pFile)
{
	return 0;
}

static int dbgdrv_mmap(struct file* pFile, struct vm_area_struct* ps_vma)
{
	return 0;
}

static struct file_operations dbgdrv_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = dbgdrv_ioctl,
	.open           = dbgdrv_open,
	.release        = dbgdrv_release,
	.mmap           = dbgdrv_mmap,
};

#endif  

IMG_VOID DBGDrvGetServiceTable(IMG_VOID **fn_table);

IMG_VOID DBGDrvGetServiceTable(IMG_VOID **fn_table)
{
	extern DBGKM_SERVICE_TABLE g_sDBGKMServices;

	*fn_table = &g_sDBGKMServices;
}

#if defined(SUPPORT_DRI_DRM)
void dbgdrv_cleanup(void)
#else
static void __exit dbgdrv_cleanup(void)
#endif
{
#if !defined(SUPPORT_DRI_DRM)
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
	device_destroy(psDbgDrvClass, MKDEV(AssignedMajorNumber, 0));
	class_destroy(psDbgDrvClass);
#endif
	unregister_chrdev(AssignedMajorNumber, DRVNAME);
#endif 
#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	HostDestroyEventObjects();
#endif
	HostDestroyMutex(g_pvAPIMutex);
	return;
}

#if defined(SUPPORT_DRI_DRM)
IMG_INT dbgdrv_init(void)
#else
static int __init dbgdrv_init(void)
#endif
{
#if (defined(LDM_PLATFORM) || defined(LDM_PCI)) && !defined(SUPPORT_DRI_DRM)
	struct device *psDev;
#endif

#if !defined(SUPPORT_DRI_DRM)
	int err = -EBUSY;
#endif

	
	if ((g_pvAPIMutex=HostCreateMutex()) == IMG_NULL)
	{
		return -ENOMEM;
	}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	
	(void) HostCreateEventObjects();
#endif

#if !defined(SUPPORT_DRI_DRM)
	AssignedMajorNumber =
	register_chrdev(AssignedMajorNumber, DRVNAME, &dbgdrv_fops);

	if (AssignedMajorNumber <= 0)
	{
		PVR_DPF((PVR_DBG_ERROR," unable to get major\n"));
		goto ErrDestroyEventObjects;
	}

#if defined(LDM_PLATFORM) || defined(LDM_PCI)
	
	psDbgDrvClass = class_create(THIS_MODULE, DRVNAME);
	if (IS_ERR(psDbgDrvClass))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: unable to create class (%ld)",
				 __func__, PTR_ERR(psDbgDrvClass)));
		goto ErrUnregisterCharDev;
	}

	psDev = device_create(psDbgDrvClass, NULL, MKDEV(AssignedMajorNumber, 0),
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
						  NULL,
#endif
						  DRVNAME);
	if (IS_ERR(psDev))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: unable to create device (%ld)",
								__func__, PTR_ERR(psDev)));
		goto ErrDestroyClass;
	}
#endif 
#endif 

	return 0;

#if !defined(SUPPORT_DRI_DRM)
ErrDestroyEventObjects:
#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	HostDestroyEventObjects();
#endif
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
ErrUnregisterCharDev:
	unregister_chrdev(AssignedMajorNumber, DRVNAME);
ErrDestroyClass:
	class_destroy(psDbgDrvClass);
#endif
	return err;
#endif 
}

#if defined(SUPPORT_DRI_DRM)
int dbgdrv_ioctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile)
#else
long dbgdrv_ioctl(struct file *file, unsigned int ioctlCmd, unsigned long arg)
#endif
{
	IOCTL_PACKAGE *pIP = (IOCTL_PACKAGE *) arg;
	char *buffer, *in, *out;
	unsigned int cmd;

	if((pIP->ui32InBufferSize > (PAGE_SIZE >> 1) ) || (pIP->ui32OutBufferSize > (PAGE_SIZE >> 1)))
	{
		PVR_DPF((PVR_DBG_ERROR,"Sizes of the buffers are too large, cannot do ioctl\n"));
		return -1;
	}

	buffer = (char *) HostPageablePageAlloc(1);
	if(!buffer)
	{
		PVR_DPF((PVR_DBG_ERROR,"Failed to allocate buffer, cannot do ioctl\n"));
		return -EFAULT;
	}

	in = buffer;
	out = buffer + (PAGE_SIZE >>1);

	if(pvr_copy_from_user(in, pIP->pInBuffer, pIP->ui32InBufferSize) != 0)
	{
		goto init_failed;
	}

	cmd = ((pIP->ui32Cmd >> 2) & 0xFFF) - 0x801;

	if(pIP->ui32Cmd == DEBUG_SERVICE_READ)
	{
		IMG_UINT32 *pui32BytesCopied = (IMG_UINT32 *)out;
		DBG_IN_READ *psReadInParams = (DBG_IN_READ *)in;
		DBG_STREAM *psStream;
		IMG_CHAR *ui8Tmp;

		ui8Tmp = vmalloc(psReadInParams->ui32OutBufferSize);

		if(!ui8Tmp)
		{
			goto init_failed;
		}

		psStream = SID2PStream(psReadInParams->hStream);
		if(!psStream)
		{
			goto init_failed;
		}

		*pui32BytesCopied = ExtDBGDrivRead(psStream,
										   psReadInParams->bReadInitBuffer,
										   psReadInParams->ui32OutBufferSize,
										   ui8Tmp);

		if(pvr_copy_to_user(psReadInParams->u.pui8OutBuffer,
						ui8Tmp,
						*pui32BytesCopied) != 0)
		{
			vfree(ui8Tmp);
			goto init_failed;
		}

		vfree(ui8Tmp);
	}
	else
	{
		(g_DBGDrivProc[cmd])(in, out);
	}

	if(copy_to_user(pIP->pOutBuffer, out, pIP->ui32OutBufferSize) != 0)
	{
		goto init_failed;
	}

	HostPageablePageFree((IMG_VOID *)buffer);
	return 0;

init_failed:
	HostPageablePageFree((IMG_VOID *)buffer);
	return -EFAULT;
}


IMG_VOID RemoveHotKey (IMG_UINT32 hHotKey)
{
	PVR_UNREFERENCED_PARAMETER(hHotKey);
}

IMG_VOID DefineHotKey (IMG_UINT32 ui32ScanCode, IMG_UINT32 ui32ShiftState, PHOTKEYINFO psInfo)
{
	PVR_UNREFERENCED_PARAMETER(ui32ScanCode);
	PVR_UNREFERENCED_PARAMETER(ui32ShiftState);
	PVR_UNREFERENCED_PARAMETER(psInfo);
}

EXPORT_SYMBOL(DBGDrvGetServiceTable);

#if !defined(SUPPORT_DRI_DRM)
subsys_initcall(dbgdrv_init);
module_exit(dbgdrv_cleanup);
#endif
