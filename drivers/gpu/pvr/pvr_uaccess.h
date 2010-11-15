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

#ifndef __PVR_UACCESS_H__
#define __PVR_UACCESS_H__

#ifndef AUTOCONF_INCLUDED
 #include <linux/config.h>
#endif

#include <linux/version.h>
#include <asm/uaccess.h>

static inline unsigned long pvr_copy_to_user(void __user *pvTo, const void *pvFrom, unsigned long ulBytes)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
    if (access_ok(VERIFY_WRITE, pvTo, ulBytes))
    {
	return __copy_to_user(pvTo, pvFrom, ulBytes);
    }
    return ulBytes;
#else
    return copy_to_user(pvTo, pvFrom, ulBytes);
#endif
}

static inline unsigned long pvr_copy_from_user(void *pvTo, const void __user *pvFrom, unsigned long ulBytes)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
    
    if (access_ok(VERIFY_READ, pvFrom, ulBytes))
    {
	return __copy_from_user(pvTo, pvFrom, ulBytes);
    }
    return ulBytes;
#else
    return copy_from_user(pvTo, pvFrom, ulBytes);
#endif
}

#endif 

