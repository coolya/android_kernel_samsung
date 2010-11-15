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

#if !defined (__KERNELDISPLAY_H__)
#define __KERNELDISPLAY_H__

#if defined (__cplusplus)
extern "C" {
#endif

typedef PVRSRV_ERROR (*PFN_OPEN_DC_DEVICE)(IMG_UINT32, IMG_HANDLE*, PVRSRV_SYNC_DATA*);
typedef PVRSRV_ERROR (*PFN_CLOSE_DC_DEVICE)(IMG_HANDLE);
typedef PVRSRV_ERROR (*PFN_ENUM_DC_FORMATS)(IMG_HANDLE, IMG_UINT32*, DISPLAY_FORMAT*);
typedef PVRSRV_ERROR (*PFN_ENUM_DC_DIMS)(IMG_HANDLE,
										 DISPLAY_FORMAT*,
										 IMG_UINT32*,
										 DISPLAY_DIMS*);
typedef PVRSRV_ERROR (*PFN_GET_DC_SYSTEMBUFFER)(IMG_HANDLE, IMG_HANDLE*);
typedef PVRSRV_ERROR (*PFN_GET_DC_INFO)(IMG_HANDLE, DISPLAY_INFO*);
typedef PVRSRV_ERROR (*PFN_CREATE_DC_SWAPCHAIN)(IMG_HANDLE,
												IMG_UINT32, 
												DISPLAY_SURF_ATTRIBUTES*, 
												DISPLAY_SURF_ATTRIBUTES*,
												IMG_UINT32, 
												PVRSRV_SYNC_DATA**,
												IMG_UINT32,
												IMG_HANDLE*, 
												IMG_UINT32*);
typedef PVRSRV_ERROR (*PFN_DESTROY_DC_SWAPCHAIN)(IMG_HANDLE, 
												 IMG_HANDLE);
typedef PVRSRV_ERROR (*PFN_SET_DC_DSTRECT)(IMG_HANDLE, IMG_HANDLE, IMG_RECT*);
typedef PVRSRV_ERROR (*PFN_SET_DC_SRCRECT)(IMG_HANDLE, IMG_HANDLE, IMG_RECT*);
typedef PVRSRV_ERROR (*PFN_SET_DC_DSTCK)(IMG_HANDLE, IMG_HANDLE, IMG_UINT32);
typedef PVRSRV_ERROR (*PFN_SET_DC_SRCCK)(IMG_HANDLE, IMG_HANDLE, IMG_UINT32);
typedef PVRSRV_ERROR (*PFN_GET_DC_BUFFERS)(IMG_HANDLE,
										   IMG_HANDLE,
										   IMG_UINT32*,
										   IMG_HANDLE*);
typedef PVRSRV_ERROR (*PFN_SWAP_TO_DC_BUFFER)(IMG_HANDLE,
											  IMG_HANDLE,
											  IMG_UINT32,
											  IMG_HANDLE,
											  IMG_UINT32,
											  IMG_RECT*);
typedef PVRSRV_ERROR (*PFN_SWAP_TO_DC_SYSTEM)(IMG_HANDLE, IMG_HANDLE);
typedef IMG_VOID (*PFN_QUERY_SWAP_COMMAND_ID)(IMG_HANDLE, IMG_HANDLE, IMG_HANDLE, IMG_HANDLE, IMG_UINT16*, IMG_BOOL*);
typedef IMG_VOID (*PFN_SET_DC_STATE)(IMG_HANDLE, IMG_UINT32);

typedef struct PVRSRV_DC_SRV2DISP_KMJTABLE_TAG
{
	IMG_UINT32						ui32TableSize;
	PFN_OPEN_DC_DEVICE				pfnOpenDCDevice;
	PFN_CLOSE_DC_DEVICE				pfnCloseDCDevice;
	PFN_ENUM_DC_FORMATS				pfnEnumDCFormats;
	PFN_ENUM_DC_DIMS				pfnEnumDCDims;
	PFN_GET_DC_SYSTEMBUFFER			pfnGetDCSystemBuffer;
	PFN_GET_DC_INFO					pfnGetDCInfo;
	PFN_GET_BUFFER_ADDR				pfnGetBufferAddr;
	PFN_CREATE_DC_SWAPCHAIN			pfnCreateDCSwapChain;
	PFN_DESTROY_DC_SWAPCHAIN		pfnDestroyDCSwapChain;
	PFN_SET_DC_DSTRECT				pfnSetDCDstRect;
	PFN_SET_DC_SRCRECT				pfnSetDCSrcRect;
	PFN_SET_DC_DSTCK				pfnSetDCDstColourKey;
	PFN_SET_DC_SRCCK				pfnSetDCSrcColourKey;
	PFN_GET_DC_BUFFERS				pfnGetDCBuffers;
	PFN_SWAP_TO_DC_BUFFER			pfnSwapToDCBuffer;
	PFN_SWAP_TO_DC_SYSTEM			pfnSwapToDCSystem;
	PFN_SET_DC_STATE				pfnSetDCState;
	PFN_QUERY_SWAP_COMMAND_ID		pfnQuerySwapCommandID;

} PVRSRV_DC_SRV2DISP_KMJTABLE;

typedef IMG_BOOL (*PFN_ISR_HANDLER)(IMG_VOID*);

typedef PVRSRV_ERROR (*PFN_DC_REGISTER_DISPLAY_DEV)(PVRSRV_DC_SRV2DISP_KMJTABLE*, IMG_UINT32*);
typedef PVRSRV_ERROR (*PFN_DC_REMOVE_DISPLAY_DEV)(IMG_UINT32);
typedef PVRSRV_ERROR (*PFN_DC_OEM_FUNCTION)(IMG_UINT32, IMG_VOID*, IMG_UINT32, IMG_VOID*, IMG_UINT32);
typedef PVRSRV_ERROR (*PFN_DC_REGISTER_COMMANDPROCLIST)(IMG_UINT32, PPFN_CMD_PROC,IMG_UINT32[][2], IMG_UINT32);
typedef PVRSRV_ERROR (*PFN_DC_REMOVE_COMMANDPROCLIST)(IMG_UINT32, IMG_UINT32);
typedef IMG_VOID (*PFN_DC_CMD_COMPLETE)(IMG_HANDLE, IMG_BOOL);
typedef PVRSRV_ERROR (*PFN_DC_REGISTER_SYS_ISR)(PFN_ISR_HANDLER, IMG_VOID*, IMG_UINT32, IMG_UINT32);
typedef PVRSRV_ERROR (*PFN_DC_REGISTER_POWER)(IMG_UINT32, PFN_PRE_POWER, PFN_POST_POWER,
											  PFN_PRE_CLOCKSPEED_CHANGE, PFN_POST_CLOCKSPEED_CHANGE,
											  IMG_HANDLE, PVRSRV_DEV_POWER_STATE, PVRSRV_DEV_POWER_STATE);

typedef struct PVRSRV_DC_DISP2SRV_KMJTABLE_TAG
{
	IMG_UINT32						ui32TableSize;
	PFN_DC_REGISTER_DISPLAY_DEV		pfnPVRSRVRegisterDCDevice;
	PFN_DC_REMOVE_DISPLAY_DEV		pfnPVRSRVRemoveDCDevice;
	PFN_DC_OEM_FUNCTION				pfnPVRSRVOEMFunction;
	PFN_DC_REGISTER_COMMANDPROCLIST	pfnPVRSRVRegisterCmdProcList;
	PFN_DC_REMOVE_COMMANDPROCLIST	pfnPVRSRVRemoveCmdProcList;
	PFN_DC_CMD_COMPLETE				pfnPVRSRVCmdComplete;
	PFN_DC_REGISTER_SYS_ISR			pfnPVRSRVRegisterSystemISRHandler;
	PFN_DC_REGISTER_POWER			pfnPVRSRVRegisterPowerDevice;
	PFN_DC_CMD_COMPLETE				pfnPVRSRVFreeCmdCompletePacket;
} PVRSRV_DC_DISP2SRV_KMJTABLE, *PPVRSRV_DC_DISP2SRV_KMJTABLE;


typedef struct DISPLAYCLASS_FLIP_COMMAND_TAG
{
	
	IMG_HANDLE hExtDevice;

	
	IMG_HANDLE hExtSwapChain;

	
	IMG_HANDLE hExtBuffer;

	
	IMG_HANDLE hPrivateTag;

	
	IMG_UINT32 ui32ClipRectCount;

	
	IMG_RECT *psClipRect;

	
	IMG_UINT32	ui32SwapInterval;

} DISPLAYCLASS_FLIP_COMMAND;

#define DC_FLIP_COMMAND		0

#define DC_STATE_NO_FLUSH_COMMANDS		0
#define DC_STATE_FLUSH_COMMANDS			1


typedef IMG_BOOL (*PFN_DC_GET_PVRJTABLE)(PPVRSRV_DC_DISP2SRV_KMJTABLE);

IMG_IMPORT IMG_BOOL PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *psJTable);


#if defined (__cplusplus)
}
#endif

#endif

