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
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))
#include <linux/mutex.h>
#else
#include <asm/semaphore.h>
#endif
#include <linux/module.h>

#include <img_defs.h>
#include <services.h>

#include "mutex.h"


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))

IMG_VOID LinuxInitMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    mutex_init(psPVRSRVMutex);
}

IMG_VOID LinuxLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    mutex_lock(psPVRSRVMutex);
}

PVRSRV_ERROR LinuxLockMutexInterruptible(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    if(mutex_lock_interruptible(psPVRSRVMutex) == -EINTR)
    {
        return PVRSRV_ERROR_MUTEX_INTERRUPTIBLE_ERROR;
    }
    else
    {
        return PVRSRV_OK;
    }
}

IMG_INT32 LinuxTryLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    return mutex_trylock(psPVRSRVMutex);
}

IMG_VOID LinuxUnLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    mutex_unlock(psPVRSRVMutex);
}

IMG_BOOL LinuxIsLockedMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    return (mutex_is_locked(psPVRSRVMutex)) ? IMG_TRUE : IMG_FALSE;
}


#else 


IMG_VOID LinuxInitMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    init_MUTEX(&psPVRSRVMutex->sSemaphore);
    atomic_set(&psPVRSRVMutex->Count, 0);
}

IMG_VOID LinuxLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    down(&psPVRSRVMutex->sSemaphore);
    atomic_dec(&psPVRSRVMutex->Count);
}

PVRSRV_ERROR LinuxLockMutexInterruptible(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    if(down_interruptible(&psPVRSRVMutex->sSemaphore) == -EINTR)
    {
        
        return PVRSRV_ERROR_MUTEX_INTERRUPTIBLE_ERROR;
    }else{
        atomic_dec(&psPVRSRVMutex->Count);
        return PVRSRV_OK;
    }
}

IMG_INT32 LinuxTryLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    IMG_INT32 Status = down_trylock(&psPVRSRVMutex->sSemaphore);
    if(Status == 0)
    {
        atomic_dec(&psPVRSRVMutex->Count);
    }

    return Status;
}

IMG_VOID LinuxUnLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    atomic_inc(&psPVRSRVMutex->Count);
    up(&psPVRSRVMutex->sSemaphore);
}

IMG_BOOL LinuxIsLockedMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex)
{
    IMG_INT32 iCount;
    
    iCount = atomic_read(&psPVRSRVMutex->Count);

    return (IMG_BOOL)iCount;
}

#endif 

