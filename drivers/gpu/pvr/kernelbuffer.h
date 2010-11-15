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

#if !defined (__KERNELBUFFER_H__)
#define __KERNELBUFFER_H__

#if defined (__cplusplus)
extern "C" {
#endif

typedef PVRSRV_ERROR (*PFN_OPEN_BC_DEVICE)(IMG_UINT32, IMG_HANDLE*);
typedef PVRSRV_ERROR (*PFN_CLOSE_BC_DEVICE)(IMG_UINT32, IMG_HANDLE);
typedef PVRSRV_ERROR (*PFN_GET_BC_INFO)(IMG_HANDLE, BUFFER_INFO*);
typedef PVRSRV_ERROR (*PFN_GET_BC_BUFFER)(IMG_HANDLE, IMG_UINT32, PVRSRV_SYNC_DATA*, IMG_HANDLE*);

typedef struct PVRSRV_BC_SRV2BUFFER_KMJTABLE_TAG
{
	IMG_UINT32							ui32TableSize;
	PFN_OPEN_BC_DEVICE					pfnOpenBCDevice;
	PFN_CLOSE_BC_DEVICE					pfnCloseBCDevice;
	PFN_GET_BC_INFO						pfnGetBCInfo;
	PFN_GET_BC_BUFFER					pfnGetBCBuffer;
	PFN_GET_BUFFER_ADDR					pfnGetBufferAddr;

} PVRSRV_BC_SRV2BUFFER_KMJTABLE;


typedef PVRSRV_ERROR (*PFN_BC_REGISTER_BUFFER_DEV)(PVRSRV_BC_SRV2BUFFER_KMJTABLE*, IMG_UINT32*);
typedef IMG_VOID (*PFN_BC_SCHEDULE_DEVICES)(IMG_VOID);
typedef PVRSRV_ERROR (*PFN_BC_REMOVE_BUFFER_DEV)(IMG_UINT32);	

typedef struct PVRSRV_BC_BUFFER2SRV_KMJTABLE_TAG
{
	IMG_UINT32							ui32TableSize;
	PFN_BC_REGISTER_BUFFER_DEV			pfnPVRSRVRegisterBCDevice;
	PFN_BC_SCHEDULE_DEVICES				pfnPVRSRVScheduleDevices;
	PFN_BC_REMOVE_BUFFER_DEV			pfnPVRSRVRemoveBCDevice;

} PVRSRV_BC_BUFFER2SRV_KMJTABLE, *PPVRSRV_BC_BUFFER2SRV_KMJTABLE;

typedef IMG_BOOL (*PFN_BC_GET_PVRJTABLE) (PPVRSRV_BC_BUFFER2SRV_KMJTABLE); 

IMG_IMPORT IMG_BOOL PVRGetBufferClassJTable(PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable);

#if defined (__cplusplus)
}
#endif

#endif
