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

#ifndef _DBGDRIV_
#define _DBGDRIV_

#define BUFFER_SIZE 64*PAGESIZE

#define DBGDRIV_VERSION 	0x100
#define MAX_PROCESSES 		2
#define BLOCK_USED			0x01
#define BLOCK_LOCKED		0x02
#define DBGDRIV_MONOBASE	0x000B0000


extern IMG_VOID *	g_pvAPIMutex;

IMG_VOID * IMG_CALLCONV DBGDrivCreateStream(IMG_CHAR *		pszName,
								   IMG_UINT32 	ui32CapMode,
								   IMG_UINT32 	ui32OutMode,
								   IMG_UINT32	ui32Flags,
								   IMG_UINT32 	ui32Pages);
IMG_VOID   IMG_CALLCONV DBGDrivDestroyStream(PDBG_STREAM psStream);
IMG_VOID * IMG_CALLCONV DBGDrivFindStream(IMG_CHAR * pszName, IMG_BOOL bResetStream);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivReadString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Limit);
IMG_UINT32 IMG_CALLCONV DBGDrivWrite(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivWrite2(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivRead(PDBG_STREAM psStream, IMG_BOOL bReadInitBuffer, IMG_UINT32 ui32OutBufferSize,IMG_UINT8 *pui8OutBuf);
IMG_VOID   IMG_CALLCONV DBGDrivSetCaptureMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode,IMG_UINT32 ui32Start,IMG_UINT32 ui32Stop,IMG_UINT32 ui32SampleRate);
IMG_VOID   IMG_CALLCONV DBGDrivSetOutputMode(PDBG_STREAM psStream,IMG_UINT32 ui32OutMode);
IMG_VOID   IMG_CALLCONV DBGDrivSetDebugLevel(PDBG_STREAM psStream,IMG_UINT32 ui32DebugLevel);
IMG_VOID   IMG_CALLCONV DBGDrivSetFrame(PDBG_STREAM psStream,IMG_UINT32 ui32Frame);
IMG_UINT32 IMG_CALLCONV DBGDrivGetFrame(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV DBGDrivOverrideMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode);
IMG_VOID   IMG_CALLCONV DBGDrivDefaultMode(PDBG_STREAM psStream);
IMG_PVOID  IMG_CALLCONV DBGDrivGetServiceTable(IMG_VOID);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteStringCM(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteCM(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_VOID   IMG_CALLCONV DBGDrivSetClientMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker);
IMG_VOID   IMG_CALLCONV DBGDrivSetMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker);
IMG_UINT32 IMG_CALLCONV DBGDrivGetMarker(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV DBGDrivIsLastCaptureFrame(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV DBGDrivIsCaptureFrame(PDBG_STREAM psStream, IMG_BOOL bCheckPreviousFrame);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteLF(PDBG_STREAM psStream, IMG_UINT8 *pui8InBuf, IMG_UINT32 ui32InBuffSize, IMG_UINT32 ui32Level, IMG_UINT32 ui32Flags);
IMG_UINT32 IMG_CALLCONV DBGDrivReadLF(PDBG_STREAM psStream, IMG_UINT32 ui32OutBuffSize, IMG_UINT8 *pui8OutBuf);
IMG_VOID   IMG_CALLCONV DBGDrivStartInitPhase(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV DBGDrivStopInitPhase(PDBG_STREAM psStream);
IMG_UINT32 IMG_CALLCONV DBGDrivGetStreamOffset(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV DBGDrivSetStreamOffset(PDBG_STREAM psStream, IMG_UINT32 ui32StreamOffset);
IMG_VOID   IMG_CALLCONV DBGDrivWaitForEvent(DBG_EVENT eEvent);

IMG_VOID DestroyAllStreams(IMG_VOID);

IMG_UINT32 AtoI(IMG_CHAR *szIn);

IMG_VOID HostMemSet(IMG_VOID *pvDest,IMG_UINT8 ui8Value,IMG_UINT32 ui32Size);
IMG_VOID HostMemCopy(IMG_VOID *pvDest,IMG_VOID *pvSrc,IMG_UINT32 ui32Size);
IMG_VOID MonoOut(IMG_CHAR * pszString,IMG_BOOL bNewLine);

IMG_SID PStream2SID(PDBG_STREAM psStream);
PDBG_STREAM SID2PStream(IMG_SID hStream); 
IMG_BOOL AddSIDEntry(PDBG_STREAM psStream);
IMG_BOOL RemoveSIDEntry(PDBG_STREAM psStream);

IMG_VOID * IMG_CALLCONV ExtDBGDrivCreateStream(IMG_CHAR *	pszName, IMG_UINT32 ui32CapMode, IMG_UINT32	ui32OutMode, IMG_UINT32 ui32Flags, IMG_UINT32 ui32Size);
IMG_VOID   IMG_CALLCONV ExtDBGDrivDestroyStream(PDBG_STREAM psStream);
IMG_VOID * IMG_CALLCONV ExtDBGDrivFindStream(IMG_CHAR * pszName, IMG_BOOL bResetStream);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivReadString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Limit);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWrite(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivRead(PDBG_STREAM psStream, IMG_BOOL bReadInitBuffer, IMG_UINT32 ui32OutBuffSize,IMG_UINT8 *pui8OutBuf);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetCaptureMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode,IMG_UINT32 ui32Start,IMG_UINT32 ui32End,IMG_UINT32 ui32SampleRate);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetOutputMode(PDBG_STREAM psStream,IMG_UINT32 ui32OutMode);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetDebugLevel(PDBG_STREAM psStream,IMG_UINT32 ui32DebugLevel);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetFrame(PDBG_STREAM psStream,IMG_UINT32 ui32Frame);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetFrame(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivOverrideMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode);
IMG_VOID   IMG_CALLCONV ExtDBGDrivDefaultMode(PDBG_STREAM psStream);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWrite2(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteStringCM(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteCM(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetMarker(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivStartInitPhase(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivStopInitPhase(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV ExtDBGDrivIsLastCaptureFrame(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV ExtDBGDrivIsCaptureFrame(PDBG_STREAM psStream, IMG_BOOL bCheckPreviousFrame);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteLF(PDBG_STREAM psStream, IMG_UINT8 *pui8InBuf, IMG_UINT32 ui32InBuffSize, IMG_UINT32 ui32Level, IMG_UINT32 ui32Flags);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivReadLF(PDBG_STREAM psStream, IMG_UINT32 ui32OutBuffSize, IMG_UINT8 *pui8OutBuf);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetStreamOffset(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetStreamOffset(PDBG_STREAM psStream, IMG_UINT32 ui32StreamOffset);
IMG_VOID   IMG_CALLCONV ExtDBGDrivWaitForEvent(DBG_EVENT eEvent);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetConnectNotifier(DBGKM_CONNECT_NOTIFIER fn_notifier);

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWritePersist(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);

#endif

