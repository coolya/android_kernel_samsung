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

#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif


#define MAX_PDUMP_STRING_LENGTH (256)


	
#define PDUMP_GET_SCRIPT_STRING()				\
	IMG_HANDLE hScript;							\
	IMG_UINT32	ui32MaxLen;						\
	PVRSRV_ERROR eError;						\
	eError = PDumpOSGetScriptString(&hScript, &ui32MaxLen);\
	if(eError != PVRSRV_OK) return eError;

#define PDUMP_GET_MSG_STRING()					\
	IMG_CHAR *pszMsg;							\
	IMG_UINT32	ui32MaxLen;						\
	PVRSRV_ERROR eError;						\
	eError = PDumpOSGetMessageString(&pszMsg, &ui32MaxLen);\
	if(eError != PVRSRV_OK) return eError;

#define PDUMP_GET_FILE_STRING()				\
	IMG_CHAR *pszFileName;					\
	IMG_UINT32	ui32MaxLen;					\
	PVRSRV_ERROR eError;					\
	eError = PDumpOSGetFilenameString(&pszFileName, &ui32MaxLen);\
	if(eError != PVRSRV_OK) return eError;

#define PDUMP_GET_SCRIPT_AND_FILE_STRING()		\
	IMG_HANDLE hScript;							\
	IMG_CHAR *pszFileName;						\
	IMG_UINT32	ui32MaxLenScript;				\
	IMG_UINT32	ui32MaxLenFileName;				\
	PVRSRV_ERROR eError;						\
	eError = PDumpOSGetScriptString(&hScript, &ui32MaxLenScript);\
	if(eError != PVRSRV_OK) return eError;		\
	eError = PDumpOSGetFilenameString(&pszFileName, &ui32MaxLenFileName);\
	if(eError != PVRSRV_OK) return eError;

	
	PVRSRV_ERROR PDumpOSGetScriptString(IMG_HANDLE *phScript, IMG_UINT32 *pui32MaxLen);

	
	PVRSRV_ERROR PDumpOSGetMessageString(IMG_CHAR **ppszMsg, IMG_UINT32 *pui32MaxLen);

	
	PVRSRV_ERROR PDumpOSGetFilenameString(IMG_CHAR **ppszFile, IMG_UINT32 *pui32MaxLen);




#define PDUMP_va_list	va_list
#define PDUMP_va_start	va_start
#define PDUMP_va_end	va_end



IMG_HANDLE PDumpOSGetStream(IMG_UINT32 ePDumpStream);

IMG_UINT32 PDumpOSGetStreamOffset(IMG_UINT32 ePDumpStream);

IMG_UINT32 PDumpOSGetParamFileNum(IMG_VOID);

IMG_VOID PDumpOSCheckForSplitting(IMG_HANDLE hStream, IMG_UINT32 ui32Size, IMG_UINT32 ui32Flags);

IMG_BOOL PDumpOSIsSuspended(IMG_VOID);

IMG_BOOL PDumpOSJTInitialised(IMG_VOID);

IMG_BOOL PDumpOSWriteString(IMG_HANDLE hDbgStream,
		IMG_UINT8 *psui8Data,
		IMG_UINT32 ui32Size,
		IMG_UINT32 ui32Flags);

IMG_BOOL PDumpOSWriteString2(IMG_HANDLE	hScript, IMG_UINT32 ui32Flags);

PVRSRV_ERROR PDumpOSBufprintf(IMG_HANDLE hBuf, IMG_UINT32 ui32ScriptSizeMax, IMG_CHAR* pszFormat, ...) IMG_FORMAT_PRINTF(3, 4);

IMG_VOID PDumpOSDebugPrintf(IMG_CHAR* pszFormat, ...) IMG_FORMAT_PRINTF(1, 2);

PVRSRV_ERROR PDumpOSSprintf(IMG_CHAR *pszComment, IMG_UINT32 ui32ScriptSizeMax, IMG_CHAR *pszFormat, ...) IMG_FORMAT_PRINTF(3, 4);

PVRSRV_ERROR PDumpOSVSprintf(IMG_CHAR *pszMsg, IMG_UINT32 ui32ScriptSizeMax, IMG_CHAR* pszFormat, PDUMP_va_list vaArgs) IMG_FORMAT_PRINTF(3, 0);

IMG_UINT32 PDumpOSBuflen(IMG_HANDLE hBuffer, IMG_UINT32 ui32BufferSizeMax);

IMG_VOID PDumpOSVerifyLineEnding(IMG_HANDLE hBuffer, IMG_UINT32 ui32BufferSizeMax);

IMG_VOID PDumpOSCPUVAddrToDevPAddr(PVRSRV_DEVICE_TYPE eDeviceType,
        IMG_HANDLE hOSMemHandle,
		IMG_UINT32 ui32Offset,
		IMG_UINT8 *pui8LinAddr,
		IMG_UINT32 ui32PageSize,
		IMG_DEV_PHYADDR *psDevPAddr);

IMG_VOID PDumpOSCPUVAddrToPhysPages(IMG_HANDLE hOSMemHandle,
		IMG_UINT32 ui32Offset,
		IMG_PUINT8 pui8LinAddr,
		IMG_UINT32 ui32DataPageMask,
		IMG_UINT32 *pui32PageOffset);

IMG_VOID PDumpOSReleaseExecution(IMG_VOID);

IMG_BOOL PDumpOSIsCaptureFrameKM(IMG_VOID);

PVRSRV_ERROR PDumpOSSetFrameKM(IMG_UINT32 ui32Frame);

#if defined (__cplusplus)
}
#endif
