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


#ifdef LINUX
#include <linux/string.h>
#endif

#include "img_types.h"
#include "pvr_debug.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hotkey.h"
#include "hostfunc.h"
#include "pvr_debug.h"




#define LAST_FRAME_BUF_SIZE	1024

typedef struct _DBG_LASTFRAME_BUFFER_
{
	PDBG_STREAM	psStream;
	IMG_UINT8 ui8Buffer[LAST_FRAME_BUF_SIZE];
	IMG_UINT32		ui32BufLen;
	struct _DBG_LASTFRAME_BUFFER_	*psNext;
} *PDBG_LASTFRAME_BUFFER;


static PDBG_STREAM	g_psStreamList = 0;
static PDBG_LASTFRAME_BUFFER	g_psLFBufferList;

static IMG_UINT32		g_ui32LOff = 0;
static IMG_UINT32		g_ui32Line = 0;
static IMG_UINT32		g_ui32MonoLines = 25;

static IMG_BOOL			g_bHotkeyMiddump = IMG_FALSE;
static IMG_UINT32		g_ui32HotkeyMiddumpStart = 0xffffffff;
static IMG_UINT32		g_ui32HotkeyMiddumpEnd = 0xffffffff;

IMG_VOID *				g_pvAPIMutex=IMG_NULL;

extern IMG_UINT32		g_ui32HotKeyFrame;
extern IMG_BOOL			g_bHotKeyPressed;
extern IMG_BOOL			g_bHotKeyRegistered;

IMG_BOOL				gbDumpThisFrame = IMG_FALSE;


IMG_UINT32 SpaceInStream(PDBG_STREAM psStream);
IMG_BOOL ExpandStreamBuffer(PDBG_STREAM psStream, IMG_UINT32 ui32NewSize);
PDBG_LASTFRAME_BUFFER FindLFBuf(PDBG_STREAM psStream);

DBGKM_SERVICE_TABLE g_sDBGKMServices =
{
	sizeof (DBGKM_SERVICE_TABLE),
	ExtDBGDrivCreateStream,
	ExtDBGDrivDestroyStream,
	ExtDBGDrivFindStream,
	ExtDBGDrivWriteString,
	ExtDBGDrivReadString,
	ExtDBGDrivWrite,
	ExtDBGDrivRead,
	ExtDBGDrivSetCaptureMode,
	ExtDBGDrivSetOutputMode,
	ExtDBGDrivSetDebugLevel,
	ExtDBGDrivSetFrame,
	ExtDBGDrivGetFrame,
	ExtDBGDrivOverrideMode,
	ExtDBGDrivDefaultMode,
	ExtDBGDrivWrite2,
	ExtDBGDrivWriteStringCM,
	ExtDBGDrivWriteCM,
	ExtDBGDrivSetMarker,
	ExtDBGDrivGetMarker,
	ExtDBGDrivStartInitPhase,
	ExtDBGDrivStopInitPhase,
	ExtDBGDrivIsCaptureFrame,
	ExtDBGDrivWriteLF,
	ExtDBGDrivReadLF,
	ExtDBGDrivGetStreamOffset,
	ExtDBGDrivSetStreamOffset,
	ExtDBGDrivIsLastCaptureFrame,
	ExtDBGDrivWaitForEvent,
	ExtDBGDrivSetConnectNotifier,
	ExtDBGDrivWritePersist
};


static IMG_UINT32 DBGDrivWritePersist(PDBG_STREAM psMainStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
static IMG_VOID InvalidateAllStreams(IMG_VOID);




DBGKM_CONNECT_NOTIFIER g_fnDBGKMNotifier;

IMG_VOID IMG_CALLCONV ExtDBGDrivSetConnectNotifier(DBGKM_CONNECT_NOTIFIER fn_notifier)
{
	
	g_fnDBGKMNotifier = fn_notifier;
}

IMG_VOID * IMG_CALLCONV ExtDBGDrivCreateStream(IMG_CHAR *	pszName, IMG_UINT32 ui32CapMode, IMG_UINT32	ui32OutMode, IMG_UINT32 ui32Flags, IMG_UINT32 ui32Size)
{
	IMG_VOID *	pvRet;

	
	HostAquireMutex(g_pvAPIMutex);

	pvRet=DBGDrivCreateStream(pszName, ui32CapMode, ui32OutMode, ui32Flags, ui32Size);

	
	HostReleaseMutex(g_pvAPIMutex);

	return pvRet;
}

void IMG_CALLCONV ExtDBGDrivDestroyStream(PDBG_STREAM psStream)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivDestroyStream(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

IMG_VOID * IMG_CALLCONV ExtDBGDrivFindStream(IMG_CHAR * pszName, IMG_BOOL bResetStream)
{
	IMG_VOID *	pvRet;

	
	HostAquireMutex(g_pvAPIMutex);

	pvRet=DBGDrivFindStream(pszName, bResetStream);
	if(g_fnDBGKMNotifier.pfnConnectNotifier)
	{
		g_fnDBGKMNotifier.pfnConnectNotifier();
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "pfnConnectNotifier not initialised.\n"));
	}		

	
	HostReleaseMutex(g_pvAPIMutex);

	return pvRet;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level)
{
	IMG_UINT32	ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivWriteString(psStream, pszString, ui32Level);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivReadString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Limit)
{
	IMG_UINT32 ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivReadString(psStream, pszString, ui32Limit);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWrite(PDBG_STREAM psStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	IMG_UINT32	ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivWrite(psStream, pui8InBuf, ui32InBuffSize, ui32Level);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivRead(PDBG_STREAM psStream, IMG_BOOL bReadInitBuffer, IMG_UINT32 ui32OutBuffSize,IMG_UINT8 * pui8OutBuf)
{
	IMG_UINT32 ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivRead(psStream, bReadInitBuffer, ui32OutBuffSize, pui8OutBuf);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

void IMG_CALLCONV ExtDBGDrivSetCaptureMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode,IMG_UINT32 ui32Start,IMG_UINT32 ui32End,IMG_UINT32 ui32SampleRate)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetCaptureMode(psStream, ui32Mode, ui32Start, ui32End, ui32SampleRate);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void IMG_CALLCONV ExtDBGDrivSetOutputMode(PDBG_STREAM psStream,IMG_UINT32 ui32OutMode)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetOutputMode(psStream, ui32OutMode);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void IMG_CALLCONV ExtDBGDrivSetDebugLevel(PDBG_STREAM psStream,IMG_UINT32 ui32DebugLevel)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetDebugLevel(psStream, ui32DebugLevel);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void IMG_CALLCONV ExtDBGDrivSetFrame(PDBG_STREAM psStream,IMG_UINT32 ui32Frame)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetFrame(psStream, ui32Frame);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetFrame(PDBG_STREAM psStream)
{
	IMG_UINT32	ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivGetFrame(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_BOOL IMG_CALLCONV ExtDBGDrivIsLastCaptureFrame(PDBG_STREAM psStream)
{
	IMG_BOOL	bRet;

	
	HostAquireMutex(g_pvAPIMutex);

	bRet = DBGDrivIsLastCaptureFrame(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return bRet;
}

IMG_BOOL IMG_CALLCONV ExtDBGDrivIsCaptureFrame(PDBG_STREAM psStream, IMG_BOOL bCheckPreviousFrame)
{
	IMG_BOOL	bRet;

	
	HostAquireMutex(g_pvAPIMutex);

	bRet = DBGDrivIsCaptureFrame(psStream, bCheckPreviousFrame);

	
	HostReleaseMutex(g_pvAPIMutex);

	return bRet;
}

void IMG_CALLCONV ExtDBGDrivOverrideMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivOverrideMode(psStream, ui32Mode);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void IMG_CALLCONV ExtDBGDrivDefaultMode(PDBG_STREAM psStream)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivDefaultMode(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWrite2(PDBG_STREAM psStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	IMG_UINT32	ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivWrite2(psStream, pui8InBuf, ui32InBuffSize, ui32Level);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWritePersist(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	IMG_UINT32	ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivWritePersist(psStream, pui8InBuf, ui32InBuffSize, ui32Level);
	if(ui32Ret==0xFFFFFFFFU)
	{
		PVR_DPF((PVR_DBG_ERROR, "An error occurred in DBGDrivWritePersist."));
	}

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteStringCM(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level)
{
	IMG_UINT32	ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret=DBGDrivWriteStringCM(psStream, pszString, ui32Level);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteCM(PDBG_STREAM psStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	IMG_UINT32	ui32Ret;
	
	
	HostAquireMutex(g_pvAPIMutex);
	
	ui32Ret=DBGDrivWriteCM(psStream, pui8InBuf, ui32InBuffSize, ui32Level);
	
	
	HostReleaseMutex(g_pvAPIMutex);
	
	return ui32Ret;
}

void IMG_CALLCONV ExtDBGDrivSetMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetMarker(psStream, ui32Marker);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetMarker(PDBG_STREAM psStream)
{
	IMG_UINT32	ui32Marker;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Marker = DBGDrivGetMarker(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Marker;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteLF(PDBG_STREAM psStream, IMG_UINT8 * pui8InBuf, IMG_UINT32 ui32InBuffSize, IMG_UINT32 ui32Level, IMG_UINT32 ui32Flags)
{
	IMG_UINT32	ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivWriteLF(psStream, pui8InBuf, ui32InBuffSize, ui32Level, ui32Flags);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivReadLF(PDBG_STREAM psStream, IMG_UINT32 ui32OutBuffSize, IMG_UINT8 * pui8OutBuf)
{
	IMG_UINT32 ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivReadLF(psStream, ui32OutBuffSize, pui8OutBuf);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}


IMG_VOID IMG_CALLCONV ExtDBGDrivStartInitPhase(PDBG_STREAM psStream)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivStartInitPhase(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

IMG_VOID IMG_CALLCONV ExtDBGDrivStopInitPhase(PDBG_STREAM psStream)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivStopInitPhase(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return;
}

IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetStreamOffset(PDBG_STREAM psStream)
{
	IMG_UINT32 ui32Ret;

	
	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivGetStreamOffset(psStream);

	
	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

IMG_VOID IMG_CALLCONV ExtDBGDrivSetStreamOffset(PDBG_STREAM psStream, IMG_UINT32 ui32StreamOffset)
{
	
	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetStreamOffset(psStream, ui32StreamOffset);

	
	HostReleaseMutex(g_pvAPIMutex);
}

IMG_VOID IMG_CALLCONV ExtDBGDrivWaitForEvent(DBG_EVENT eEvent)
{
#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	DBGDrivWaitForEvent(eEvent);
#else	
	PVR_UNREFERENCED_PARAMETER(eEvent);				
#endif	
}

IMG_UINT32 AtoI(IMG_CHAR *szIn)
{
	IMG_INT		iLen = 0;
	IMG_UINT32	ui32Value = 0;
	IMG_UINT32	ui32Digit=1;
	IMG_UINT32	ui32Base=10;
	IMG_INT		iPos;
	IMG_CHAR	bc;

	
	while (szIn[iLen] > 0)
	{
		iLen ++;
	}

	
	if (iLen == 0)
	{
		return (0);
	}

	
	iPos=0;
	while (szIn[iPos] == '0')
	{
		iPos++;
	}
	if (szIn[iPos] == '\0')
	{
		return 0;
	}
	if (szIn[iPos] == 'x' || szIn[iPos] == 'X')
	{
		ui32Base=16;
		szIn[iPos]='0';
	}

	
	for (iPos = iLen - 1; iPos >= 0; iPos --)
	{
		bc = szIn[iPos];

		if ( (bc >= 'a') && (bc <= 'f') && ui32Base == 16)			
		{
			bc -= 'a' - 0xa;
		}
		else
		if ( (bc >= 'A') && (bc <= 'F') && ui32Base == 16)			
		{
			bc -= 'A' - 0xa;
		}
		else
		if ((bc >= '0') && (bc <= '9'))				
		{
			bc -= '0';
		}
		else
			return (0);

		ui32Value += (IMG_UINT32)bc  * ui32Digit;

		ui32Digit = ui32Digit * ui32Base;
	}
	return (ui32Value);
}


static IMG_BOOL StreamValid(PDBG_STREAM psStream)
{
	PDBG_STREAM	psThis;

	psThis = g_psStreamList;

	while (psThis)
	{
		if (psStream && (psThis == psStream) )
		{
			return(IMG_TRUE);
		}
		else
		{
			psThis = psThis->psNext;
		}
	}

	return(IMG_FALSE);
}


static IMG_BOOL StreamValidForRead(PDBG_STREAM psStream)
{
	if( StreamValid(psStream) &&
		((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_WRITEONLY) == 0) )
	{
		return(IMG_TRUE);
	}

	return(IMG_FALSE);
}

static IMG_BOOL StreamValidForWrite(PDBG_STREAM psStream)
{
	if( StreamValid(psStream) &&
		((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_READONLY) == 0) )
	{
		return(IMG_TRUE);
	}

	return(IMG_FALSE);
}


static void Write(PDBG_STREAM psStream,IMG_PUINT8 pui8Data,IMG_UINT32 ui32InBuffSize)
{
	

	if (!psStream->bCircularAllowed)
	{
		
	}

	if ((psStream->ui32WPtr + ui32InBuffSize) > psStream->ui32Size)
	{
		
		IMG_UINT32 ui32B1 = psStream->ui32Size - psStream->ui32WPtr;
		IMG_UINT32 ui32B2 = ui32InBuffSize - ui32B1;

		
		HostMemCopy((IMG_PVOID)((IMG_UINTPTR_T)psStream->pvBase + psStream->ui32WPtr),
				(IMG_PVOID) pui8Data,
				ui32B1);

		
		HostMemCopy(psStream->pvBase,
				(IMG_PVOID)(pui8Data + ui32B1),
				ui32B2);

		
		psStream->ui32WPtr = ui32B2;
	}
	else
	{	
		HostMemCopy((IMG_PVOID)((IMG_UINTPTR_T)psStream->pvBase + psStream->ui32WPtr),
				(IMG_PVOID) pui8Data,
				ui32InBuffSize);

		psStream->ui32WPtr += ui32InBuffSize;

		if (psStream->ui32WPtr == psStream->ui32Size)
		{
			psStream->ui32WPtr = 0;
		}
	}
	psStream->ui32DataWritten += ui32InBuffSize;
}


void MonoOut(IMG_CHAR * pszString,IMG_BOOL bNewLine)
{
#if defined (_WIN64)
	PVR_UNREFERENCED_PARAMETER(pszString);
	PVR_UNREFERENCED_PARAMETER(bNewLine);

#else
	IMG_UINT32 	i;
	IMG_CHAR *	pScreen;

	pScreen = (IMG_CHAR *) DBGDRIV_MONOBASE;

	pScreen += g_ui32Line * 160;

	

	i=0;
	do
	{
		pScreen[g_ui32LOff + (i*2)] = pszString[i];
		pScreen[g_ui32LOff + (i*2)+1] = 127;
		i++;
	}
	while ((pszString[i] != 0) && (i < 4096));

	g_ui32LOff += i * 2;

	if (bNewLine)
	{
		g_ui32LOff = 0;
		g_ui32Line++;
	}

	

	if (g_ui32Line == g_ui32MonoLines)
	{
		g_ui32Line = g_ui32MonoLines - 1;

		HostMemCopy((IMG_VOID *)DBGDRIV_MONOBASE,(IMG_VOID *)(DBGDRIV_MONOBASE + 160),160 * (g_ui32MonoLines - 1));

		HostMemSet((IMG_VOID *)(DBGDRIV_MONOBASE + (160 * (g_ui32MonoLines - 1))),0,160);
	}
#endif	
}

static IMG_UINT32 WriteExpandingBuffer(PDBG_STREAM psStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize)
{
	IMG_UINT ui32Space;

	

	ui32Space = SpaceInStream(psStream);

	

	if ((psStream->psCtrl->ui32OutMode & DEBUG_OUTMODE_STREAMENABLE) == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "WriteExpandingBuffer: buffer %x is disabled", (IMG_UINTPTR_T) psStream));
		return(0);
	}

	

	if (psStream->psCtrl->ui32Flags & DEBUG_FLAGS_NO_BUF_EXPANDSION)
	{
		


		if (ui32Space < 32)
		{
			PVR_DPF((PVR_DBG_ERROR, "WriteExpandingBuffer: buffer %x is full and isn't expandable", (IMG_UINTPTR_T) psStream));
			return(0);
		}
	}
	else
	{
		if ((ui32Space < 32) || (ui32Space <= (ui32InBuffSize + 4)))
		{
			IMG_UINT32	ui32NewBufSize;

			

			ui32NewBufSize = 2 * psStream->ui32Size;

			PVR_DPF((PVR_DBGDRIV_MESSAGE, "Expanding buffer size = %x, new size = %x",
					psStream->ui32Size, ui32NewBufSize));

			if (ui32InBuffSize > psStream->ui32Size)
			{
				ui32NewBufSize += ui32InBuffSize;
			}

			

			if (!ExpandStreamBuffer(psStream,ui32NewBufSize))
			{
				if (ui32Space < 32)
				{
					if(psStream->bCircularAllowed)
					{
						return(0);
					}
					else
					{
						
						PVR_DPF((PVR_DBG_ERROR, "WriteExpandingBuffer: Unable to expand %x. Out of memory.", (IMG_UINTPTR_T) psStream));
						InvalidateAllStreams();
						return (0xFFFFFFFFUL);
					}
				}
			}

			

			ui32Space = SpaceInStream(psStream);
			PVR_DPF((PVR_DBGDRIV_MESSAGE, "Expanded buffer, free space = %x",
					ui32Space));
		}
	}

	

	if (ui32Space <= (ui32InBuffSize + 4))
	{
		ui32InBuffSize = ui32Space - 4;
	}

	

	Write(psStream,pui8InBuf,ui32InBuffSize);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	if (ui32InBuffSize)
	{
		HostSignalEvent(DBG_EVENT_STREAM_DATA);
	}
#endif
	return(ui32InBuffSize);
}

IMG_VOID * IMG_CALLCONV DBGDrivCreateStream(IMG_CHAR *		pszName,
								   IMG_UINT32 	ui32CapMode,
								   IMG_UINT32 	ui32OutMode,
								   IMG_UINT32	ui32Flags,
								   IMG_UINT32 	ui32Size)
{
	PDBG_STREAM psStream;
	PDBG_STREAM	psInitStream;
	PDBG_LASTFRAME_BUFFER	psLFBuffer;
	PDBG_STREAM_CONTROL psCtrl;
	IMG_UINT32		ui32Off;
	IMG_VOID *		pvBase;	
	static IMG_CHAR pszNameInitSuffix[] = "_Init";
	IMG_UINT32		ui32OffSuffix;

	


	psStream = (PDBG_STREAM) DBGDrivFindStream(pszName, IMG_FALSE);

	if (psStream)
	{
		return ((IMG_VOID *) psStream);
	}

	

	psStream = HostNonPageablePageAlloc(1);
	psInitStream = HostNonPageablePageAlloc(1);
	psLFBuffer = HostNonPageablePageAlloc(1);
	psCtrl = HostNonPageablePageAlloc(1);
	if	(
			(!psStream) ||
			(!psInitStream) ||
			(!psLFBuffer) ||
			(!psCtrl)
		)
	{
		PVR_DPF((PVR_DBG_ERROR,"DBGDriv: Couldn't alloc control structs\n\r"));
		return((IMG_VOID *) 0);
	}

	
	if ((ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
	{
		pvBase = HostNonPageablePageAlloc(ui32Size);
	}
	else
	{
		pvBase = HostPageablePageAlloc(ui32Size);
	}

	if (!pvBase)
	{
		PVR_DPF((PVR_DBG_ERROR,"DBGDriv: Couldn't alloc Stream buffer\n\r"));
		HostNonPageablePageFree(psStream);
		return((IMG_VOID *) 0);
	}

	
	psCtrl->ui32Flags = ui32Flags;
	psCtrl->ui32CapMode = ui32CapMode;
	psCtrl->ui32OutMode = ui32OutMode;
	psCtrl->ui32DebugLevel = DEBUG_LEVEL_0;
	psCtrl->ui32DefaultMode = ui32CapMode;
	psCtrl->ui32Start = 0;
	psCtrl->ui32End = 0;
	psCtrl->ui32Current = 0;
	psCtrl->ui32SampleRate = 1;
	psCtrl->bInitPhaseComplete = IMG_FALSE;

	

	psStream->psNext = 0;
	psStream->pvBase = pvBase;
	psStream->psCtrl = psCtrl;
	psStream->ui32Size = ui32Size * 4096UL;
	psStream->ui32RPtr = 0;
	psStream->ui32WPtr = 0;
	psStream->ui32DataWritten = 0;
	psStream->ui32Marker = 0;
	psStream->bCircularAllowed = IMG_TRUE;
	psStream->ui32InitPhaseWOff = 0;
	


	
	if ((ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
	{
		pvBase = HostNonPageablePageAlloc(ui32Size);
	}
	else
	{
		pvBase = HostPageablePageAlloc(ui32Size);
	}

	if (!pvBase)
	{
		PVR_DPF((PVR_DBG_ERROR,"DBGDriv: Couldn't alloc InitStream buffer\n\r"));
		
		if ((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
		{
			HostNonPageablePageFree(psStream->pvBase);
		}
		else
		{
			HostPageablePageFree(psStream->pvBase);
		}
		HostNonPageablePageFree(psStream);
		return((IMG_VOID *) 0);
	}

	
	psInitStream->psNext = 0;
	psInitStream->pvBase = pvBase;
	psInitStream->psCtrl = psCtrl;
	psInitStream->ui32Size = ui32Size * 4096UL;
	psInitStream->ui32RPtr = 0;
	psInitStream->ui32WPtr = 0;
	psInitStream->ui32DataWritten = 0;
	psInitStream->ui32Marker = 0;
	psInitStream->bCircularAllowed = IMG_FALSE;
	psInitStream->ui32InitPhaseWOff = 0;
	


	psStream->psInitStream = psInitStream;

	
	psLFBuffer->psStream = psStream;
	psLFBuffer->ui32BufLen = 0UL;

	g_bHotkeyMiddump = IMG_FALSE;
	g_ui32HotkeyMiddumpStart = 0xffffffffUL;
	g_ui32HotkeyMiddumpEnd = 0xffffffffUL;

	

	ui32Off = 0;

	do
	{
		psStream->szName[ui32Off] = pszName[ui32Off];
		psInitStream->szName[ui32Off] = pszName[ui32Off];
		ui32Off++;
	}
	while ((pszName[ui32Off] != 0) && (ui32Off < (4096UL - sizeof(DBG_STREAM))));
	psStream->szName[ui32Off] = pszName[ui32Off];	

	

	ui32OffSuffix = 0;
	do
	{
		psInitStream->szName[ui32Off] = pszNameInitSuffix[ui32OffSuffix];
		ui32Off++;
		ui32OffSuffix++;
	}
	while (	(pszNameInitSuffix[ui32OffSuffix] != 0) &&
			(ui32Off < (4096UL - sizeof(DBG_STREAM))));
	psInitStream->szName[ui32Off] = pszNameInitSuffix[ui32OffSuffix];	

	

	psStream->psNext = g_psStreamList;
	g_psStreamList = psStream;

	psLFBuffer->psNext = g_psLFBufferList;
	g_psLFBufferList = psLFBuffer;

	AddSIDEntry(psStream);
	
	return((IMG_VOID *) psStream);
}

void IMG_CALLCONV DBGDrivDestroyStream(PDBG_STREAM psStream)
{
	PDBG_STREAM	psStreamThis;
	PDBG_STREAM	psStreamPrev;
	PDBG_LASTFRAME_BUFFER	psLFBuffer;
	PDBG_LASTFRAME_BUFFER	psLFThis;
	PDBG_LASTFRAME_BUFFER	psLFPrev;

	PVR_DPF((PVR_DBG_MESSAGE, "DBGDriv: Destroying stream %s\r\n", psStream->szName ));

	

	if (!StreamValid(psStream))
	{
		return;
	}

	RemoveSIDEntry(psStream);
	
	psLFBuffer = FindLFBuf(psStream);

	

	psStreamThis = g_psStreamList;
	psStreamPrev = 0;

	while (psStreamThis)
	{
		if (psStreamThis == psStream)
		{
			if (psStreamPrev)
			{
				psStreamPrev->psNext = psStreamThis->psNext;
			}
			else
			{
				g_psStreamList = psStreamThis->psNext;
			}

			psStreamThis = 0;
		}
		else
		{
			psStreamPrev = psStreamThis;
			psStreamThis = psStreamThis->psNext;
		}
	}

	psLFThis = g_psLFBufferList;
	psLFPrev = 0;

	while (psLFThis)
	{
		if (psLFThis == psLFBuffer)
		{
			if (psLFPrev)
			{
				psLFPrev->psNext = psLFThis->psNext;
			}
			else
			{
				g_psLFBufferList = psLFThis->psNext;
			}

			psLFThis = 0;
		}
		else
		{
			psLFPrev = psLFThis;
			psLFThis = psLFThis->psNext;
		}
	}
	

	if (psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_HOTKEY)
	{
		DeactivateHotKeys();
	}

	

	if ((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
	{
		HostNonPageablePageFree(psStream->psCtrl);
		HostNonPageablePageFree(psStream->pvBase);
		HostNonPageablePageFree(psStream->psInitStream->pvBase);
	}
	else
	{
		HostNonPageablePageFree(psStream->psCtrl);
		HostPageablePageFree(psStream->pvBase);
		HostPageablePageFree(psStream->psInitStream->pvBase);
	}
	
	HostNonPageablePageFree(psStream->psInitStream);
	HostNonPageablePageFree(psStream);
	HostNonPageablePageFree(psLFBuffer);

	if (g_psStreamList == 0)
	{
		PVR_DPF((PVR_DBG_MESSAGE,"DBGDriv: Stream list now empty" ));
	}

	return;
}

IMG_VOID * IMG_CALLCONV DBGDrivFindStream(IMG_CHAR * pszName, IMG_BOOL bResetStream)
{
	PDBG_STREAM	psStream;
	PDBG_STREAM	psThis;
	IMG_UINT32	ui32Off;
	IMG_BOOL	bAreSame;

	psStream = 0;

	PVR_DPF((PVR_DBGDRIV_MESSAGE, "PDump client connecting to %s %s",
			pszName,
			(bResetStream == IMG_TRUE) ? "with reset" : "no reset"));

	

	for (psThis = g_psStreamList; psThis != IMG_NULL; psThis = psThis->psNext)
	{
		bAreSame = IMG_TRUE;
		ui32Off = 0;

		if (strlen(psThis->szName) == strlen(pszName))
		{
			while ((psThis->szName[ui32Off] != 0) && (pszName[ui32Off] != 0) && (ui32Off < 128) && bAreSame)
			{
				if (psThis->szName[ui32Off] != pszName[ui32Off])
				{
					bAreSame = IMG_FALSE;
				}

				ui32Off++;
			}
		}
		else
		{
			bAreSame = IMG_FALSE;
		}

		if (bAreSame)
		{
			psStream = psThis;
			break;
		}
	}

	if(bResetStream && psStream)
	{
		static IMG_CHAR szComment[] = "-- Init phase terminated\r\n";
		psStream->psInitStream->ui32RPtr = 0;
		psStream->ui32RPtr = 0;
		psStream->ui32WPtr = 0;
		psStream->ui32DataWritten = psStream->psInitStream->ui32DataWritten;
		if (psStream->psCtrl->bInitPhaseComplete == IMG_FALSE)
		{
			if (psStream->psCtrl->ui32Flags & DEBUG_FLAGS_TEXTSTREAM)
			{
				DBGDrivWrite2(psStream, (IMG_UINT8 *)szComment, sizeof(szComment) - 1, 0x01);
			}
			psStream->psCtrl->bInitPhaseComplete = IMG_TRUE;
		}

		{
			
			
			psStream->psInitStream->ui32InitPhaseWOff = psStream->psInitStream->ui32WPtr;
			PVR_DPF((PVR_DBGDRIV_MESSAGE, "Set %s client marker bo %x, total bw %x",
					psStream->szName,
					psStream->psInitStream->ui32InitPhaseWOff,
					psStream->psInitStream->ui32DataWritten ));
		}
	}

	return((IMG_VOID *) psStream);
}

static void IMG_CALLCONV DBGDrivInvalidateStream(PDBG_STREAM psStream)
{
	IMG_CHAR pszErrorMsg[] = "**OUTOFMEM\n";
	IMG_UINT32 ui32Space;
	IMG_UINT32 ui32Off = 0;
	IMG_UINT32 ui32WPtr = psStream->ui32WPtr;
	IMG_PUINT8 pui8Buffer = (IMG_UINT8 *) psStream->pvBase;
	
	PVR_DPF((PVR_DBG_ERROR, "DBGDrivInvalidateStream: An error occurred for stream %s\r\n", psStream->szName ));

	

	




	
	ui32Space = SpaceInStream(psStream);

	
	if(ui32Space > 0)
	{
		ui32Space--;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "DBGDrivInvalidateStream: Buffer full."));
	}

	while((pszErrorMsg[ui32Off] != 0) && (ui32Off < ui32Space))
	{
		pui8Buffer[ui32WPtr] = (IMG_UINT8)pszErrorMsg[ui32Off];
		ui32Off++;
		ui32WPtr++;
	}
	pui8Buffer[ui32WPtr++] = '\0';
	psStream->ui32WPtr = ui32WPtr;

	
	psStream->psCtrl->ui32Flags |= DEBUG_FLAGS_READONLY;
}

static IMG_VOID InvalidateAllStreams(IMG_VOID)
{
	PDBG_STREAM psStream = g_psStreamList;
	while (psStream != IMG_NULL)
	{
		DBGDrivInvalidateStream(psStream);
		psStream = psStream->psNext;
	}
	return;
}



IMG_UINT32 IMG_CALLCONV DBGDrivWriteStringCM(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level)
{
	

	if (!StreamValidForWrite(psStream))
	{
		return(0xFFFFFFFFUL);
	}

	

	if (psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_FRAMED)
	{
		if	((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE) == 0)
		{
			return(0);
		}
	}
	else
	{
		if (psStream->psCtrl->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
		{
			if ((psStream->psCtrl->ui32Current != g_ui32HotKeyFrame) || (g_bHotKeyPressed == IMG_FALSE))
			{
				return(0);
			}
		}
	}

	return(DBGDrivWriteString(psStream,pszString,ui32Level));

}

IMG_UINT32 IMG_CALLCONV DBGDrivWriteString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level)
{
	IMG_UINT32	ui32Len;
	IMG_UINT32	ui32Space;
	IMG_UINT32	ui32WPtr;
	IMG_UINT8 *	pui8Buffer;

	

	if (!StreamValidForWrite(psStream))
	{
		return(0xFFFFFFFFUL);
	}

	

	if ((psStream->psCtrl->ui32DebugLevel & ui32Level) == 0)
	{
		return(0xFFFFFFFFUL);
	}

	


	if ((psStream->psCtrl->ui32OutMode & DEBUG_OUTMODE_ASYNC) == 0)
	{
		if (psStream->psCtrl->ui32OutMode & DEBUG_OUTMODE_STANDARDDBG)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"%s: %s\r\n",psStream->szName, pszString));
		}

		

		if (psStream->psCtrl->ui32OutMode & DEBUG_OUTMODE_MONO)
		{
			MonoOut(psStream->szName,IMG_FALSE);
			MonoOut(": ",IMG_FALSE);
			MonoOut(pszString,IMG_TRUE);
		}
	}

	

	if	(
			!(
				((psStream->psCtrl->ui32OutMode & DEBUG_OUTMODE_STREAMENABLE) != 0) ||
				((psStream->psCtrl->ui32OutMode & DEBUG_OUTMODE_ASYNC) != 0)
			)
		)
	{
		return(0xFFFFFFFFUL);
	}

	

	ui32Space=SpaceInStream(psStream);

	
	if(ui32Space > 0)
	{
		ui32Space--;
	}

	ui32Len		= 0;
	ui32WPtr	= psStream->ui32WPtr;
	pui8Buffer	= (IMG_UINT8 *) psStream->pvBase;

	while((pszString[ui32Len] != 0) && (ui32Len < ui32Space))
	{
		pui8Buffer[ui32WPtr] = (IMG_UINT8)pszString[ui32Len];
		ui32Len++;
		ui32WPtr++;
		if (ui32WPtr == psStream->ui32Size)
		{
			ui32WPtr = 0;
		}
	}

	if (ui32Len < ui32Space)
	{
		
		pui8Buffer[ui32WPtr] = (IMG_UINT8)pszString[ui32Len];
		ui32Len++;
		ui32WPtr++;
		if (ui32WPtr == psStream->ui32Size)
		{
			ui32WPtr = 0;
		}

		
		psStream->ui32WPtr = ui32WPtr;
		psStream->ui32DataWritten+= ui32Len;
	} else
	{
		ui32Len = 0;
	}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	if (ui32Len)
	{
		HostSignalEvent(DBG_EVENT_STREAM_DATA);
	}
#endif

	return(ui32Len);
}

IMG_UINT32 IMG_CALLCONV DBGDrivReadString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Limit)
{
	IMG_UINT32				ui32OutLen;
	IMG_UINT32				ui32Len;
	IMG_UINT32				ui32Offset;
	IMG_UINT8				*pui8Buff;

	

	if (!StreamValidForRead(psStream))
	{
		return(0);
	}

	

	pui8Buff = (IMG_UINT8 *)psStream->pvBase;
	ui32Offset = psStream->ui32RPtr;

	if (psStream->ui32RPtr == psStream->ui32WPtr)
	{
		return(0);
	}

	

	ui32Len = 0;
	while((pui8Buff[ui32Offset] != 0) && (ui32Offset != psStream->ui32WPtr))
	{
		ui32Offset++;
		ui32Len++;

		

		if (ui32Offset == psStream->ui32Size)
		{
			ui32Offset = 0;
		}
	}

	ui32OutLen = ui32Len + 1;

	

	if (ui32Len > ui32Limit)
	{
		return(0);
	}

	

	ui32Offset = psStream->ui32RPtr;
	ui32Len = 0;

	while ((pui8Buff[ui32Offset] != 0) && (ui32Len < ui32Limit))
	{
		pszString[ui32Len] = (IMG_CHAR)pui8Buff[ui32Offset];
		ui32Offset++;
		ui32Len++;

		

		if (ui32Offset == psStream->ui32Size)
		{
			ui32Offset = 0;
		}
	}

	pszString[ui32Len] = (IMG_CHAR)pui8Buff[ui32Offset];

	psStream->ui32RPtr = ui32Offset + 1;

	if (psStream->ui32RPtr == psStream->ui32Size)
	{
		psStream->ui32RPtr = 0;
	}

	return(ui32OutLen);
}

IMG_UINT32 IMG_CALLCONV DBGDrivWrite(PDBG_STREAM psMainStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	IMG_UINT32				ui32Space;
	DBG_STREAM *psStream;

	

	if (!StreamValidForWrite(psMainStream))
	{
		return(0xFFFFFFFFUL);
	}

	

	if ((psMainStream->psCtrl->ui32DebugLevel & ui32Level) == 0)
	{
		return(0xFFFFFFFFUL);
	}

	

	if (psMainStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_FRAMED)
	{
		if	((psMainStream->psCtrl->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE) == 0)
		{
			
			return(ui32InBuffSize);
		}
	}
	else if (psMainStream->psCtrl->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
	{
		if ((psMainStream->psCtrl->ui32Current != g_ui32HotKeyFrame) || (g_bHotKeyPressed == IMG_FALSE))
		{
			
			return(ui32InBuffSize);
		}
	}

	if(psMainStream->psCtrl->bInitPhaseComplete)
	{
		psStream = psMainStream;
	}
	else
	{
		psStream = psMainStream->psInitStream;
	}

	

	ui32Space=SpaceInStream(psStream);

	PVR_DPF((PVR_DBGDRIV_MESSAGE, "Recv %d b for %s: Roff = %x, WOff = %x",
			ui32InBuffSize,
			psStream->szName,
			psStream->ui32RPtr,
			psStream->ui32WPtr));

	

	if ((psStream->psCtrl->ui32OutMode & DEBUG_OUTMODE_STREAMENABLE) == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "DBGDrivWrite: buffer %x is disabled", (IMG_UINTPTR_T) psStream));
		return(0);
	}

	if (ui32Space < 8)
	{
		PVR_DPF((PVR_DBG_ERROR, "DBGDrivWrite: buffer %x is full", (IMG_UINTPTR_T) psStream));
		return(0);
	}

	

	if (ui32Space <= (ui32InBuffSize + 4))
	{
		ui32InBuffSize = ui32Space - 8;
	}

	

	Write(psStream,(IMG_UINT8 *) &ui32InBuffSize,4);
	Write(psStream,pui8InBuf,ui32InBuffSize);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	if (ui32InBuffSize)
	{
		HostSignalEvent(DBG_EVENT_STREAM_DATA);
	}
#endif
	return(ui32InBuffSize);
}

IMG_UINT32 IMG_CALLCONV DBGDrivWriteCM(PDBG_STREAM psStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	

	if (!StreamValidForWrite(psStream))
	{
		return(0xFFFFFFFFUL);
	}

	

	if (psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_FRAMED)
	{
		if	((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE) == 0)
		{
			
			return(ui32InBuffSize);
		}
	}
	else
	{
		if (psStream->psCtrl->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
		{
			if ((psStream->psCtrl->ui32Current != g_ui32HotKeyFrame) || (g_bHotKeyPressed == IMG_FALSE))
			{
				
				return(ui32InBuffSize);
			}
		}
	}

	return(DBGDrivWrite2(psStream,pui8InBuf,ui32InBuffSize,ui32Level));
}


static IMG_UINT32 DBGDrivWritePersist(PDBG_STREAM psMainStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	DBG_STREAM	*psStream;
	PVR_UNREFERENCED_PARAMETER(ui32Level);

	

	if (!StreamValidForWrite(psMainStream))
	{
		return(0xFFFFFFFFUL);
	}

	
	psStream = psMainStream->psInitStream;
	if(psStream->bCircularAllowed == IMG_TRUE)
	{
		PVR_DPF((PVR_DBG_WARNING, "DBGDrivWritePersist: Init phase is a circular buffer, some data may be lost"));
	}

	PVR_DPF((PVR_DBGDRIV_MESSAGE, "Append %x b to %s: Roff = %x, WOff = %x [bw = %x]",
			ui32InBuffSize,
			psStream->szName,
			psStream->ui32RPtr,
			psStream->ui32WPtr,
			psStream->ui32DataWritten));

	return( WriteExpandingBuffer(psStream, pui8InBuf, ui32InBuffSize) );
}

IMG_UINT32 IMG_CALLCONV DBGDrivWrite2(PDBG_STREAM psMainStream,IMG_UINT8 * pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level)
{
	DBG_STREAM	*psStream;

	

	if (!StreamValidForWrite(psMainStream))
	{
		PVR_DPF((PVR_DBG_ERROR, "DBGDrivWrite2: stream not valid"));
		return(0xFFFFFFFFUL);
	}

	

	if ((psMainStream->psCtrl->ui32DebugLevel & ui32Level) == 0)
	{
		return(0);
	}

	if(psMainStream->psCtrl->bInitPhaseComplete)
	{
		psStream = psMainStream;
	}
	else
	{
		psStream = psMainStream->psInitStream;
	}

	PVR_DPF((PVR_DBGDRIV_MESSAGE, "Recv(exp) %d b for %s: Roff = %x, WOff = %x",
			ui32InBuffSize,
			psStream->szName,
			psStream->ui32RPtr,
			psStream->ui32WPtr));

	return( WriteExpandingBuffer(psStream, pui8InBuf, ui32InBuffSize) );
}

IMG_UINT32 IMG_CALLCONV DBGDrivRead(PDBG_STREAM psMainStream, IMG_BOOL bReadInitBuffer, IMG_UINT32 ui32OutBuffSize,IMG_UINT8 * pui8OutBuf)
{
	IMG_UINT32 ui32Data;
	DBG_STREAM *psStream;

	

	if (!StreamValidForRead(psMainStream))
	{
		PVR_DPF((PVR_DBG_ERROR, "DBGDrivRead: buffer %x is invalid", (IMG_UINTPTR_T) psMainStream));
		return(0);
	}

	if(bReadInitBuffer)
	{
		psStream = psMainStream->psInitStream;
	}
	else
	{
		psStream = psMainStream;
	}

	
	if (psStream->ui32RPtr == psStream->ui32WPtr ||
		((psStream->ui32InitPhaseWOff > 0) &&
		 (psStream->ui32RPtr >= psStream->ui32InitPhaseWOff)) )
	{
		return(0);
	}

	

	if (psStream->ui32RPtr <= psStream->ui32WPtr)
	{
		ui32Data = psStream->ui32WPtr - psStream->ui32RPtr;
	}
	else
	{
		ui32Data = psStream->ui32WPtr + (psStream->ui32Size - psStream->ui32RPtr);
	}

	

	if ((psStream->ui32InitPhaseWOff > 0) &&
		(psStream->ui32InitPhaseWOff < psStream->ui32WPtr))
	{
		ui32Data = psStream->ui32InitPhaseWOff - psStream->ui32RPtr;
	}

	

	if (ui32Data > ui32OutBuffSize)
	{
		ui32Data = ui32OutBuffSize;
	}

	PVR_DPF((PVR_DBGDRIV_MESSAGE, "Send %x b from %s: Roff = %x, WOff = %x",
			ui32Data,
			psStream->szName,
			psStream->ui32RPtr,
			psStream->ui32WPtr));

	

	if ((psStream->ui32RPtr + ui32Data) > psStream->ui32Size)
	{	
		IMG_UINT32 ui32B1 = psStream->ui32Size - psStream->ui32RPtr;
		IMG_UINT32 ui32B2 = ui32Data - ui32B1;

		
		HostMemCopy((IMG_VOID *) pui8OutBuf,
				(IMG_VOID *)((IMG_UINTPTR_T)psStream->pvBase + psStream->ui32RPtr),
				ui32B1);

		
		HostMemCopy((IMG_VOID *)(pui8OutBuf + ui32B1),
				psStream->pvBase,
				ui32B2);

		
		psStream->ui32RPtr = ui32B2;
	}
	else
	{	
		HostMemCopy((IMG_VOID *) pui8OutBuf,
				(IMG_VOID *)((IMG_UINTPTR_T)psStream->pvBase + psStream->ui32RPtr),
				ui32Data);

		
		psStream->ui32RPtr += ui32Data;

		
		if (psStream->ui32RPtr == psStream->ui32Size)
		{
			psStream->ui32RPtr = 0;
		}
	}

	return(ui32Data);
}

void IMG_CALLCONV DBGDrivSetCaptureMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode,IMG_UINT32 ui32Start,IMG_UINT32 ui32End,IMG_UINT32 ui32SampleRate)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->psCtrl->ui32CapMode = ui32Mode;
	psStream->psCtrl->ui32DefaultMode = ui32Mode;
	psStream->psCtrl->ui32Start = ui32Start;
	psStream->psCtrl->ui32End = ui32End;
	psStream->psCtrl->ui32SampleRate = ui32SampleRate;

	

	if (psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_HOTKEY)
	{
		ActivateHotKeys(psStream);
	}
}

void IMG_CALLCONV DBGDrivSetOutputMode(PDBG_STREAM psStream,IMG_UINT32 ui32OutMode)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->psCtrl->ui32OutMode = ui32OutMode;
}

void IMG_CALLCONV DBGDrivSetDebugLevel(PDBG_STREAM psStream,IMG_UINT32 ui32DebugLevel)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->psCtrl->ui32DebugLevel = ui32DebugLevel;
}

void IMG_CALLCONV DBGDrivSetFrame(PDBG_STREAM psStream,IMG_UINT32 ui32Frame)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->psCtrl->ui32Current = ui32Frame;

	if ((ui32Frame >= psStream->psCtrl->ui32Start) &&
		(ui32Frame <= psStream->psCtrl->ui32End) &&
		(((ui32Frame - psStream->psCtrl->ui32Start) % psStream->psCtrl->ui32SampleRate) == 0))
	{
		psStream->psCtrl->ui32Flags |= DEBUG_FLAGS_ENABLESAMPLE;
	}
	else
	{
		psStream->psCtrl->ui32Flags &= ~DEBUG_FLAGS_ENABLESAMPLE;
	}

	if (g_bHotkeyMiddump)
	{
		if ((ui32Frame >= g_ui32HotkeyMiddumpStart) &&
			(ui32Frame <= g_ui32HotkeyMiddumpEnd) &&
			(((ui32Frame - g_ui32HotkeyMiddumpStart) % psStream->psCtrl->ui32SampleRate) == 0))
		{
			psStream->psCtrl->ui32Flags |= DEBUG_FLAGS_ENABLESAMPLE;
		}
		else
		{
			psStream->psCtrl->ui32Flags &= ~DEBUG_FLAGS_ENABLESAMPLE;
			if (psStream->psCtrl->ui32Current > g_ui32HotkeyMiddumpEnd)
			{
				g_bHotkeyMiddump = IMG_FALSE;
			}
		}
	}

	
	if (g_bHotKeyRegistered)
	{
		g_bHotKeyRegistered = IMG_FALSE;

		PVR_DPF((PVR_DBG_MESSAGE,"Hotkey pressed (%p)!\n",psStream));

		if (!g_bHotKeyPressed)
		{
			

			g_ui32HotKeyFrame = psStream->psCtrl->ui32Current + 2;

			

			g_bHotKeyPressed = IMG_TRUE;
		}

		

		if (((psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_FRAMED) != 0) && 
			((psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_HOTKEY) != 0))
		{
			if (!g_bHotkeyMiddump)
			{
				
				g_ui32HotkeyMiddumpStart = g_ui32HotKeyFrame + 1;
				g_ui32HotkeyMiddumpEnd = 0xffffffff;
				g_bHotkeyMiddump = IMG_TRUE;
				PVR_DPF((PVR_DBG_MESSAGE,"Sampling every %d frame(s)\n", psStream->psCtrl->ui32SampleRate));
			}
			else
			{
				
				g_ui32HotkeyMiddumpEnd = g_ui32HotKeyFrame;
				PVR_DPF((PVR_DBG_MESSAGE,"Turning off sampling\n"));
			}
		}

	}

	

	if (psStream->psCtrl->ui32Current > g_ui32HotKeyFrame)
	{
		g_bHotKeyPressed = IMG_FALSE;
	}
}

IMG_UINT32 IMG_CALLCONV DBGDrivGetFrame(PDBG_STREAM psStream)
{
	

	if (!StreamValid(psStream))
	{
		return(0);
	}

	return(psStream->psCtrl->ui32Current);
}

IMG_BOOL IMG_CALLCONV DBGDrivIsLastCaptureFrame(PDBG_STREAM psStream)
{
	IMG_UINT32	ui32NextFrame;

	

	if (!StreamValid(psStream))
	{
		return IMG_FALSE;
	}

	if (psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_FRAMED)
	{
		ui32NextFrame = psStream->psCtrl->ui32Current + psStream->psCtrl->ui32SampleRate;
		if (ui32NextFrame > psStream->psCtrl->ui32End)
		{
			return IMG_TRUE;
		}
	}
	return IMG_FALSE;
}

IMG_BOOL IMG_CALLCONV DBGDrivIsCaptureFrame(PDBG_STREAM psStream, IMG_BOOL bCheckPreviousFrame)
{
	IMG_UINT32 ui32FrameShift = bCheckPreviousFrame ? 1UL : 0UL;

	

	if (!StreamValid(psStream))
	{
		return IMG_FALSE;
	}

	if (psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_FRAMED)
	{
		
		if (g_bHotkeyMiddump)
		{
			if ((psStream->psCtrl->ui32Current >= (g_ui32HotkeyMiddumpStart - ui32FrameShift)) &&
				(psStream->psCtrl->ui32Current <= (g_ui32HotkeyMiddumpEnd - ui32FrameShift)) &&
				((((psStream->psCtrl->ui32Current + ui32FrameShift) - g_ui32HotkeyMiddumpStart) % psStream->psCtrl->ui32SampleRate) == 0))
			{
				return IMG_TRUE;
			}
		}
		else
		{
			if ((psStream->psCtrl->ui32Current >= (psStream->psCtrl->ui32Start - ui32FrameShift)) &&
				(psStream->psCtrl->ui32Current <= (psStream->psCtrl->ui32End - ui32FrameShift)) &&
				((((psStream->psCtrl->ui32Current + ui32FrameShift) - psStream->psCtrl->ui32Start) % psStream->psCtrl->ui32SampleRate) == 0))
			{
				return IMG_TRUE;
			}
		}
	}
	else if (psStream->psCtrl->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
	{
		if ((psStream->psCtrl->ui32Current == (g_ui32HotKeyFrame-ui32FrameShift)) && (g_bHotKeyPressed))
		{
			return IMG_TRUE;
		}
	}
	return IMG_FALSE;
}

void IMG_CALLCONV DBGDrivOverrideMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->psCtrl->ui32CapMode = ui32Mode;
}

void IMG_CALLCONV DBGDrivDefaultMode(PDBG_STREAM psStream)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->psCtrl->ui32CapMode = psStream->psCtrl->ui32DefaultMode;
}

IMG_VOID IMG_CALLCONV DBGDrivSetClientMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->ui32InitPhaseWOff = ui32Marker;
}

void IMG_CALLCONV DBGDrivSetMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker)
{
	

	if (!StreamValid(psStream))
	{
		return;
	}

	psStream->ui32Marker = ui32Marker;
}

IMG_UINT32 IMG_CALLCONV DBGDrivGetMarker(PDBG_STREAM psStream)
{
	

	if (!StreamValid(psStream))
	{
		return 0;
	}

	return psStream->ui32Marker;
}


IMG_UINT32 IMG_CALLCONV DBGDrivGetStreamOffset(PDBG_STREAM psMainStream)
{
	PDBG_STREAM psStream;

	

	if (!StreamValid(psMainStream))
	{
		return 0;
	}

	if(psMainStream->psCtrl->bInitPhaseComplete)
	{
		psStream = psMainStream;
	}
	else
	{
		psStream = psMainStream->psInitStream;
	}

	return psStream->ui32DataWritten;
}

IMG_VOID IMG_CALLCONV DBGDrivSetStreamOffset(PDBG_STREAM psMainStream, IMG_UINT32 ui32StreamOffset)
{
	PDBG_STREAM psStream;

	

	if (!StreamValid(psMainStream))
	{
		return;
	}

	if(psMainStream->psCtrl->bInitPhaseComplete)
	{
		psStream = psMainStream;
	}
	else
	{
		psStream = psMainStream->psInitStream;
	}

	PVR_DPF((PVR_DBGDRIV_MESSAGE, "DBGDrivSetStreamOffset: %s set to %x b",
			psStream->szName,
			ui32StreamOffset));
	psStream->ui32DataWritten = ui32StreamOffset;
}

IMG_PVOID IMG_CALLCONV DBGDrivGetServiceTable(IMG_VOID)
{
	return((IMG_PVOID)&g_sDBGKMServices);
}

IMG_UINT32 IMG_CALLCONV DBGDrivWriteLF(PDBG_STREAM psStream, IMG_UINT8 * pui8InBuf, IMG_UINT32 ui32InBuffSize, IMG_UINT32 ui32Level, IMG_UINT32 ui32Flags)
{
	PDBG_LASTFRAME_BUFFER	psLFBuffer;

	

	if (!StreamValidForWrite(psStream))
	{
		return(0xFFFFFFFFUL);
	}

	

	if ((psStream->psCtrl->ui32DebugLevel & ui32Level) == 0)
	{
		return(0xFFFFFFFFUL);
	}

	

	if ((psStream->psCtrl->ui32CapMode & DEBUG_CAPMODE_FRAMED) != 0)
	{
		if	((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE) == 0)
		{
			
			return(ui32InBuffSize);
		}
	}
	else if (psStream->psCtrl->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
	{
		if ((psStream->psCtrl->ui32Current != g_ui32HotKeyFrame) || (g_bHotKeyPressed == IMG_FALSE))
		{
			
			return(ui32InBuffSize);
		}
	}

	psLFBuffer = FindLFBuf(psStream);

	if (ui32Flags & WRITELF_FLAGS_RESETBUF)
	{
		

		ui32InBuffSize = (ui32InBuffSize > LAST_FRAME_BUF_SIZE) ? LAST_FRAME_BUF_SIZE : ui32InBuffSize;
		HostMemCopy((IMG_VOID *)psLFBuffer->ui8Buffer, (IMG_VOID *)pui8InBuf, ui32InBuffSize);
		psLFBuffer->ui32BufLen = ui32InBuffSize;
	}
	else
	{
		

		ui32InBuffSize = ((psLFBuffer->ui32BufLen + ui32InBuffSize) > LAST_FRAME_BUF_SIZE) ? (LAST_FRAME_BUF_SIZE - psLFBuffer->ui32BufLen) : ui32InBuffSize;
		HostMemCopy((IMG_VOID *)(&psLFBuffer->ui8Buffer[psLFBuffer->ui32BufLen]), (IMG_VOID *)pui8InBuf, ui32InBuffSize);
		psLFBuffer->ui32BufLen += ui32InBuffSize;
	}

	return(ui32InBuffSize);
}

IMG_UINT32 IMG_CALLCONV DBGDrivReadLF(PDBG_STREAM psStream, IMG_UINT32 ui32OutBuffSize, IMG_UINT8 * pui8OutBuf)
{
	PDBG_LASTFRAME_BUFFER	psLFBuffer;
	IMG_UINT32	ui32Data;

	

	if (!StreamValidForRead(psStream))
	{
		return(0);
	}

	psLFBuffer = FindLFBuf(psStream);

	

	ui32Data = (ui32OutBuffSize < psLFBuffer->ui32BufLen) ? ui32OutBuffSize : psLFBuffer->ui32BufLen;

	

	HostMemCopy((IMG_VOID *)pui8OutBuf, (IMG_VOID *)psLFBuffer->ui8Buffer, ui32Data);

	return ui32Data;
}

IMG_VOID IMG_CALLCONV DBGDrivStartInitPhase(PDBG_STREAM psStream)
{
	psStream->psCtrl->bInitPhaseComplete = IMG_FALSE;
}

IMG_VOID IMG_CALLCONV DBGDrivStopInitPhase(PDBG_STREAM psStream)
{
	psStream->psCtrl->bInitPhaseComplete = IMG_TRUE;
}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
IMG_VOID IMG_CALLCONV DBGDrivWaitForEvent(DBG_EVENT eEvent)
{
	HostWaitForEvent(eEvent);
}
#endif

IMG_BOOL ExpandStreamBuffer(PDBG_STREAM psStream, IMG_UINT32 ui32NewSize)
{
	IMG_VOID *	pvNewBuf;
	IMG_UINT32	ui32NewSizeInPages;
	IMG_UINT32	ui32NewWOffset;
	IMG_UINT32	ui32NewROffset;
	IMG_UINT32	ui32SpaceInOldBuf;

	

	if (psStream->ui32Size >= ui32NewSize)
	{
		return IMG_FALSE;
	}

	

	ui32SpaceInOldBuf = SpaceInStream(psStream);

	

	ui32NewSizeInPages = ((ui32NewSize + 0xfffUL) & ~0xfffUL) / 4096UL;

	if ((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
	{
		pvNewBuf = HostNonPageablePageAlloc(ui32NewSizeInPages);
	}
	else
	{
		pvNewBuf = HostPageablePageAlloc(ui32NewSizeInPages);
	}

	if (pvNewBuf == IMG_NULL)
	{
		return IMG_FALSE;
	}

	if(psStream->bCircularAllowed)
	{
		


		if (psStream->ui32RPtr <= psStream->ui32WPtr)
		{
			

		HostMemCopy(pvNewBuf,
					(IMG_VOID *)((IMG_UINTPTR_T)psStream->pvBase + psStream->ui32RPtr),
					psStream->ui32WPtr - psStream->ui32RPtr);
		}
		else
		{
			IMG_UINT32	ui32FirstCopySize;
	
			

			ui32FirstCopySize = psStream->ui32Size - psStream->ui32RPtr;
	
			HostMemCopy(pvNewBuf,
					(IMG_VOID *)((IMG_UINTPTR_T)psStream->pvBase + psStream->ui32RPtr),
					ui32FirstCopySize);
	
			

			HostMemCopy((IMG_VOID *)((IMG_UINTPTR_T)pvNewBuf + ui32FirstCopySize),
					(IMG_VOID *)(IMG_PBYTE)psStream->pvBase,
					psStream->ui32WPtr);
		}
		ui32NewROffset = 0;
	}
	else
	{
		
		HostMemCopy(pvNewBuf, psStream->pvBase,	psStream->ui32WPtr);
		ui32NewROffset = psStream->ui32RPtr;
	}

	

                                                        
	ui32NewWOffset = psStream->ui32Size - ui32SpaceInOldBuf;

	

	if ((psStream->psCtrl->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
	{
		HostNonPageablePageFree(psStream->pvBase);
	}
	else
	{
		HostPageablePageFree(psStream->pvBase);
	}

	

	psStream->pvBase = pvNewBuf;
	psStream->ui32RPtr = ui32NewROffset;
	psStream->ui32WPtr = ui32NewWOffset;
	psStream->ui32Size = ui32NewSizeInPages * 4096;

	return IMG_TRUE;
}

IMG_UINT32 SpaceInStream(PDBG_STREAM psStream)
{
	IMG_UINT32	ui32Space;

	if (psStream->bCircularAllowed)
	{
		
	if (psStream->ui32RPtr > psStream->ui32WPtr)
	{
		ui32Space = psStream->ui32RPtr - psStream->ui32WPtr;
	}
	else
	{
		ui32Space = psStream->ui32RPtr + (psStream->ui32Size - psStream->ui32WPtr);
	}
	}
	else
	{
		
		ui32Space = psStream->ui32Size - psStream->ui32WPtr;
	}

	return ui32Space;
}


void DestroyAllStreams(void)
{
	while (g_psStreamList != IMG_NULL)
	{
		DBGDrivDestroyStream(g_psStreamList);
	}
	return;
}

PDBG_LASTFRAME_BUFFER FindLFBuf(PDBG_STREAM psStream)
{
	PDBG_LASTFRAME_BUFFER	psLFBuffer;

	psLFBuffer = g_psLFBufferList;

	while (psLFBuffer)
	{
		if (psLFBuffer->psStream == psStream)
		{
			break;
		}

		psLFBuffer = psLFBuffer->psNext;
	}

	return psLFBuffer;
}

