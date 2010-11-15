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

#ifndef QUEUE_H
#define QUEUE_H


#if defined(__cplusplus)
extern "C" {
#endif

#define UPDATE_QUEUE_ROFF(psQueue, ui32Size)						\
	(psQueue)->ui32ReadOffset = ((psQueue)->ui32ReadOffset + (ui32Size))	\
	& ((psQueue)->ui32QueueSize - 1);

 typedef struct _COMMAND_COMPLETE_DATA_
 {
	IMG_BOOL			bInUse;
		
	IMG_UINT32			ui32DstSyncCount;	
	IMG_UINT32			ui32SrcSyncCount;	
	PVRSRV_SYNC_OBJECT	*psDstSync;			
	PVRSRV_SYNC_OBJECT	*psSrcSync;			
	IMG_UINT32			ui32AllocSize;		
 }COMMAND_COMPLETE_DATA, *PCOMMAND_COMPLETE_DATA;

#if !defined(USE_CODE)
IMG_VOID QueueDumpDebugInfo(IMG_VOID);

IMG_IMPORT
PVRSRV_ERROR PVRSRVProcessQueues (IMG_UINT32	ui32CallerID,
								  IMG_BOOL		bFlush);

#if defined(__linux__) && defined(__KERNEL__) 
#include <linux/types.h>
#include <linux/seq_file.h>
void* ProcSeqOff2ElementQueue(struct seq_file * sfile, loff_t off);
void ProcSeqShowQueue(struct seq_file *sfile,void* el);
#endif


IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateCommandQueueKM(IMG_SIZE_T ui32QueueSize,
													 PVRSRV_QUEUE_INFO **ppsQueueInfo);
IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyCommandQueueKM(PVRSRV_QUEUE_INFO *psQueueInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVInsertCommandKM(PVRSRV_QUEUE_INFO	*psQueue,
												PVRSRV_COMMAND		**ppsCommand,
												IMG_UINT32			ui32DevIndex,
												IMG_UINT16			CommandType,
												IMG_UINT32			ui32DstSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsDstSync[],
												IMG_UINT32			ui32SrcSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsSrcSync[],
												IMG_SIZE_T			ui32DataByteSize );

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetQueueSpaceKM(PVRSRV_QUEUE_INFO *psQueue,
												IMG_SIZE_T ui32ParamSize,
												IMG_VOID **ppvSpace);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSubmitCommandKM(PVRSRV_QUEUE_INFO *psQueue,
												PVRSRV_COMMAND *psCommand);

IMG_IMPORT
IMG_VOID PVRSRVCommandCompleteKM(IMG_HANDLE hCmdCookie, IMG_BOOL bScheduleMISR);

IMG_IMPORT
PVRSRV_ERROR PVRSRVRegisterCmdProcListKM(IMG_UINT32		ui32DevIndex,
										 PFN_CMD_PROC	*ppfnCmdProcList,
										 IMG_UINT32		ui32MaxSyncsPerCmd[][2],
										 IMG_UINT32		ui32CmdCount);
IMG_IMPORT
PVRSRV_ERROR PVRSRVRemoveCmdProcListKM(IMG_UINT32	ui32DevIndex,
									   IMG_UINT32	ui32CmdCount);

#endif 


#if defined (__cplusplus)
}
#endif

#endif 

