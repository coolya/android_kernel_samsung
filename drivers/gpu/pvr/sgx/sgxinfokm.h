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

#ifndef __SGXINFOKM_H__
#define __SGXINFOKM_H__

#include "sgxdefs.h"
#include "device.h"
#include "power.h"
#include "sysconfig.h"
#include "sgxscript.h"
#include "sgxinfo.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define		SGX_HOSTPORT_PRESENT			0x00000001UL


#define SGX_PDUMPREG_NAME		"SGXREG"

typedef struct _PVRSRV_STUB_PBDESC_ PVRSRV_STUB_PBDESC;


typedef struct _PVRSRV_SGX_CCB_INFO_ *PPVRSRV_SGX_CCB_INFO;

typedef struct _PVRSRV_SGXDEV_INFO_
{
	PVRSRV_DEVICE_TYPE		eDeviceType;
	PVRSRV_DEVICE_CLASS		eDeviceClass;

	IMG_UINT8				ui8VersionMajor;
	IMG_UINT8				ui8VersionMinor;
	IMG_UINT32				ui32CoreConfig;
	IMG_UINT32				ui32CoreFlags;

	
	IMG_PVOID				pvRegsBaseKM;

#if defined(SGX_FEATURE_HOST_PORT)
	
	IMG_PVOID				pvHostPortBaseKM;
	
	IMG_UINT32				ui32HPSize;
	
	IMG_SYS_PHYADDR			sHPSysPAddr;
#endif

	
	IMG_HANDLE				hRegMapping;

	
	IMG_SYS_PHYADDR			sRegsPhysBase;
	
	IMG_UINT32				ui32RegSize;

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	
	IMG_UINT32				ui32ExtSysCacheRegsSize;
	
	IMG_DEV_PHYADDR			sExtSysCacheRegsDevPBase;
	
	IMG_UINT32				*pui32ExtSystemCacheRegsPT;
	
	IMG_HANDLE				hExtSystemCacheRegsPTPageOSMemHandle;
	
	IMG_SYS_PHYADDR			sExtSystemCacheRegsPTSysPAddr;
#endif

	
	IMG_UINT32				ui32CoreClockSpeed;
	IMG_UINT32				ui32uKernelTimerClock;

	PVRSRV_STUB_PBDESC		*psStubPBDescListKM;


	
	IMG_DEV_PHYADDR			sKernelPDDevPAddr;

	IMG_VOID				*pvDeviceMemoryHeap;
	PPVRSRV_KERNEL_MEM_INFO	psKernelCCBMemInfo;			
	PVRSRV_SGX_KERNEL_CCB	*psKernelCCB;			
	PPVRSRV_SGX_CCB_INFO	psKernelCCBInfo;		
	PPVRSRV_KERNEL_MEM_INFO	psKernelCCBCtlMemInfo;	
	PVRSRV_SGX_CCB_CTL		*psKernelCCBCtl;		
	PPVRSRV_KERNEL_MEM_INFO psKernelCCBEventKickerMemInfo; 
	IMG_UINT32				*pui32KernelCCBEventKicker; 
#if defined(PDUMP)
	IMG_UINT32				ui32KernelCCBEventKickerDumpVal; 
#endif 
 	PVRSRV_KERNEL_MEM_INFO	*psKernelSGXMiscMemInfo;	
	IMG_UINT32				aui32HostKickAddr[SGXMKIF_CMD_MAX];		
#if defined(SGX_SUPPORT_HWPROFILING)
	PPVRSRV_KERNEL_MEM_INFO psKernelHWProfilingMemInfo;
#endif
	PPVRSRV_KERNEL_MEM_INFO		psKernelHWPerfCBMemInfo;		
	PPVRSRV_KERNEL_MEM_INFO		psKernelTASigBufferMemInfo;		
	PPVRSRV_KERNEL_MEM_INFO		psKernel3DSigBufferMemInfo;		
#if defined(FIX_HW_BRN_29702)
	PPVRSRV_KERNEL_MEM_INFO psKernelCFIMemInfo;	
#endif
#if defined(FIX_HW_BRN_29823)
	PPVRSRV_KERNEL_MEM_INFO	psKernelDummyTermStreamMemInfo; 
#endif
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	PPVRSRV_KERNEL_MEM_INFO	psKernelEDMStatusBufferMemInfo; 
#endif
#if defined(SGX_FEATURE_OVERLAPPED_SPM)
	PPVRSRV_KERNEL_MEM_INFO	psKernelTmpRgnHeaderMemInfo; 
#endif
#if defined(SGX_FEATURE_SPM_MODE_0)
	PPVRSRV_KERNEL_MEM_INFO	psKernelTmpDPMStateMemInfo; 
#endif

	
	IMG_UINT32				ui32ClientRefCount;

	
	IMG_UINT32				ui32CacheControl;

	
	IMG_UINT32				ui32ClientBuildOptions;

	
	SGX_MISCINFO_STRUCT_SIZES	sSGXStructSizes;

	


	IMG_VOID				*pvMMUContextList;

	
	IMG_BOOL				bForcePTOff;

	IMG_UINT32				ui32EDMTaskReg0;
	IMG_UINT32				ui32EDMTaskReg1;

	IMG_UINT32				ui32ClkGateStatusReg;
	IMG_UINT32				ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
	IMG_UINT32				ui32MasterClkGateStatusReg;
	IMG_UINT32				ui32MasterClkGateStatusMask;
	IMG_UINT32				ui32MasterClkGateStatus2Reg;
	IMG_UINT32				ui32MasterClkGateStatus2Mask;
#endif 
	SGX_INIT_SCRIPTS		sScripts;

	
	IMG_HANDLE 				hBIFResetPDOSMemHandle;
	IMG_DEV_PHYADDR 		sBIFResetPDDevPAddr;
	IMG_DEV_PHYADDR 		sBIFResetPTDevPAddr;
	IMG_DEV_PHYADDR 		sBIFResetPageDevPAddr;
	IMG_UINT32				*pui32BIFResetPD;
	IMG_UINT32				*pui32BIFResetPT;

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
	
	IMG_HANDLE				hBRN22997PTPageOSMemHandle;
	IMG_HANDLE				hBRN22997PDPageOSMemHandle;
	IMG_DEV_PHYADDR 		sBRN22997PTDevPAddr;
	IMG_DEV_PHYADDR 		sBRN22997PDDevPAddr;
	IMG_UINT32				*pui32BRN22997PT;
	IMG_UINT32				*pui32BRN22997PD;
	IMG_SYS_PHYADDR 		sBRN22997SysPAddr;
#endif 

#if defined(SUPPORT_HW_RECOVERY)
	
	IMG_HANDLE				hTimer;
	
	IMG_UINT32				ui32TimeStamp;
#endif

	
	IMG_UINT32				ui32NumResets;

	
	PVRSRV_KERNEL_MEM_INFO			*psKernelSGXHostCtlMemInfo;
	SGXMKIF_HOST_CTL				*psSGXHostCtl;

	
	PVRSRV_KERNEL_MEM_INFO			*psKernelSGXTA3DCtlMemInfo;

	IMG_UINT32				ui32Flags;

	
	IMG_UINT32				ui32MemTilingUsage;

	#if defined(PDUMP)
	PVRSRV_SGX_PDUMP_CONTEXT	sPDContext;
	#endif

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
	
	IMG_VOID				*pvDummyPTPageCpuVAddr;
	IMG_DEV_PHYADDR			sDummyPTDevPAddr;
	IMG_HANDLE				hDummyPTPageOSMemHandle;
	IMG_VOID				*pvDummyDataPageCpuVAddr;
	IMG_DEV_PHYADDR 		sDummyDataDevPAddr;
	IMG_HANDLE				hDummyDataPageOSMemHandle;
#endif
#if defined(PDUMP)
	PDUMP_MMU_ATTRIB sMMUAttrib;
#endif
	IMG_UINT32				asSGXDevData[SGX_MAX_DEV_DATA];

} PVRSRV_SGXDEV_INFO;


typedef struct _SGX_TIMING_INFORMATION_
{
	IMG_UINT32			ui32CoreClockSpeed;
	IMG_UINT32			ui32HWRecoveryFreq;
	IMG_BOOL			bEnableActivePM;
	IMG_UINT32			ui32ActivePowManLatencyms;
	IMG_UINT32			ui32uKernelFreq;
} SGX_TIMING_INFORMATION;

typedef struct _SGX_DEVICE_MAP_
{
	IMG_UINT32				ui32Flags;

	
	IMG_SYS_PHYADDR			sRegsSysPBase;
	IMG_CPU_PHYADDR			sRegsCpuPBase;
	IMG_CPU_VIRTADDR		pvRegsCpuVBase;
	IMG_UINT32				ui32RegsSize;

#if defined(SGX_FEATURE_HOST_PORT)
	IMG_SYS_PHYADDR			sHPSysPBase;
	IMG_CPU_PHYADDR			sHPCpuPBase;
	IMG_UINT32				ui32HPSize;
#endif

	
	IMG_SYS_PHYADDR			sLocalMemSysPBase;
	IMG_DEV_PHYADDR			sLocalMemDevPBase;
	IMG_CPU_PHYADDR			sLocalMemCpuPBase;
	IMG_UINT32				ui32LocalMemSize;

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	IMG_UINT32				ui32ExtSysCacheRegsSize;
	IMG_DEV_PHYADDR			sExtSysCacheRegsDevPBase;
#endif

	
	IMG_UINT32				ui32IRQ;

#if !defined(SGX_DYNAMIC_TIMING_INFO)
	
	SGX_TIMING_INFORMATION	sTimingInfo;
#endif
#if defined(PDUMP)
	
	IMG_CHAR				*pszPDumpDevName;
#endif
} SGX_DEVICE_MAP;


struct _PVRSRV_STUB_PBDESC_
{
	IMG_UINT32		ui32RefCount;
	IMG_UINT32		ui32TotalPBSize;
	PVRSRV_KERNEL_MEM_INFO  *psSharedPBDescKernelMemInfo;
	PVRSRV_KERNEL_MEM_INFO  *psHWPBDescKernelMemInfo;
	PVRSRV_KERNEL_MEM_INFO	**ppsSubKernelMemInfos;
	IMG_UINT32		ui32SubKernelMemInfosCount;
	IMG_HANDLE		hDevCookie;
	PVRSRV_KERNEL_MEM_INFO  *psBlockKernelMemInfo;
	PVRSRV_KERNEL_MEM_INFO  *psHWBlockKernelMemInfo;
	PVRSRV_STUB_PBDESC	*psNext;
	PVRSRV_STUB_PBDESC	**ppsThis;
};

typedef struct _PVRSRV_SGX_CCB_INFO_
{
	PVRSRV_KERNEL_MEM_INFO	*psCCBMemInfo;			
	PVRSRV_KERNEL_MEM_INFO	*psCCBCtlMemInfo;		
	SGXMKIF_COMMAND		*psCommands;			
	IMG_UINT32				*pui32WriteOffset;		
	volatile IMG_UINT32		*pui32ReadOffset;		
#if defined(PDUMP)
	IMG_UINT32				ui32CCBDumpWOff;		
#endif
} PVRSRV_SGX_CCB_INFO;

PVRSRV_ERROR SGXRegisterDevice (PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_VOID SGXOSTimer(IMG_VOID *pvData);

IMG_VOID SGXReset(PVRSRV_SGXDEV_INFO	*psDevInfo,
				  IMG_BOOL				bHardwareRecovery,
				  IMG_UINT32			ui32PDUMPFlags);

PVRSRV_ERROR SGXInitialise(PVRSRV_SGXDEV_INFO	*psDevInfo,
						   IMG_BOOL				bHardwareRecovery);
PVRSRV_ERROR SGXDeinitialise(IMG_HANDLE hDevCookie);

PVRSRV_ERROR SGXPrePowerState(IMG_HANDLE				hDevHandle, 
							  PVRSRV_DEV_POWER_STATE	eNewPowerState, 
							  PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

PVRSRV_ERROR SGXPostPowerState(IMG_HANDLE				hDevHandle, 
							   PVRSRV_DEV_POWER_STATE	eNewPowerState, 
							   PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

PVRSRV_ERROR SGXPreClockSpeedChange(IMG_HANDLE				hDevHandle,
									IMG_BOOL				bIdleDevice,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

PVRSRV_ERROR SGXPostClockSpeedChange(IMG_HANDLE				hDevHandle,
									 IMG_BOOL				bIdleDevice,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

IMG_VOID SGXPanic(PVRSRV_SGXDEV_INFO	*psDevInfo);

PVRSRV_ERROR SGXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode);

#if defined(SGX_DYNAMIC_TIMING_INFO)
IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psSGXTimingInfo);
#endif

#if defined(NO_HARDWARE)
static INLINE IMG_VOID NoHardwareGenerateEvent(PVRSRV_SGXDEV_INFO		*psDevInfo,
												IMG_UINT32 ui32StatusRegister,
												IMG_UINT32 ui32StatusValue,
												IMG_UINT32 ui32StatusMask)
{
	IMG_UINT32 ui32RegVal;

	ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32StatusRegister);

	ui32RegVal &= ~ui32StatusMask;
	ui32RegVal |= (ui32StatusValue & ui32StatusMask);

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32StatusRegister, ui32RegVal);
}
#endif

#if defined(__cplusplus)
}
#endif

#endif 

