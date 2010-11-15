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
#include <asm/uaccess.h>
#include "pvr_uaccess.h"
#endif 

#include "img_types.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hotkey.h"
#include "dbgdriv_ioctl.h"


static IMG_UINT32 DBGDIOCDrivCreateStream(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_CREATESTREAM psIn;
	IMG_VOID * *ppvOut;
	#ifdef LINUX
	static IMG_CHAR name[32];
	#endif

	psIn = (PDBG_IN_CREATESTREAM) pvInBuffer;
	ppvOut = (IMG_VOID * *) pvOutBuffer;

	#ifdef LINUX

	if(pvr_copy_from_user(name, psIn->u.pszName, 32) != 0)
	{
		return IMG_FALSE;
	}

	*ppvOut = ExtDBGDrivCreateStream(name, psIn->ui32CapMode, psIn->ui32OutMode, 0, psIn->ui32Pages);

	#else
	*ppvOut = ExtDBGDrivCreateStream(psIn->u.pszName, psIn->ui32CapMode, psIn->ui32OutMode, DEBUG_FLAGS_NO_BUF_EXPANDSION, psIn->ui32Pages);
	#endif


	return(IMG_TRUE);
}

static IMG_UINT32 DBGDIOCDrivDestroyStream(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_STREAM *ppsStream;
	PDBG_STREAM  psStream;

	ppsStream = (PDBG_STREAM *) pvInBuffer;
	psStream  = (PDBG_STREAM) *ppsStream;

	PVR_UNREFERENCED_PARAMETER(	pvOutBuffer);

	ExtDBGDrivDestroyStream(psStream);

	return(IMG_TRUE);
}

static IMG_UINT32 DBGDIOCDrivGetStream(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_FINDSTREAM psParams;
	IMG_SID *	phStream;

	psParams	= (PDBG_IN_FINDSTREAM)pvInBuffer;
	phStream	= (IMG_SID *)pvOutBuffer;

	*phStream = PStream2SID(ExtDBGDrivFindStream(psParams->u.pszName, psParams->bResetStream));

	return(IMG_TRUE);
}

static IMG_UINT32 DBGDIOCDrivWriteString(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_WRITESTRING psParams;
	IMG_UINT32 *pui32OutLen;
	PDBG_STREAM psStream;

	psParams = (PDBG_IN_WRITESTRING) pvInBuffer;
	pui32OutLen = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32OutLen = ExtDBGDrivWriteString(psStream,psParams->u.pszString,psParams->ui32Level);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32OutLen = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivWriteStringCM(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_WRITESTRING psParams;
	IMG_UINT32 *pui32OutLen;
	PDBG_STREAM psStream;

	psParams = (PDBG_IN_WRITESTRING) pvInBuffer;
	pui32OutLen = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32OutLen = ExtDBGDrivWriteStringCM(psStream,psParams->u.pszString,psParams->ui32Level);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32OutLen = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivReadString(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 * pui32OutLen;
	PDBG_IN_READSTRING	psParams;
	PDBG_STREAM  psStream;

	psParams = (PDBG_IN_READSTRING) pvInBuffer;
	pui32OutLen = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32OutLen = ExtDBGDrivReadString(psStream,
											psParams->u.pszString,psParams->ui32StringLen);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32OutLen = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivWrite(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *	pui32BytesCopied;
	PDBG_IN_WRITE	psInParams;
	PDBG_STREAM		psStream;

	psInParams = (PDBG_IN_WRITE) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psInParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32BytesCopied = ExtDBGDrivWrite(psStream,
										psInParams->u.pui8InBuffer,
										psInParams->ui32TransferSize,
										psInParams->ui32Level);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32BytesCopied = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivWrite2(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *	pui32BytesCopied;
	PDBG_IN_WRITE	psInParams;
	PDBG_STREAM 	psStream;

	psInParams = (PDBG_IN_WRITE) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psInParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32BytesCopied = ExtDBGDrivWrite2(psStream,
										 psInParams->u.pui8InBuffer,
										 psInParams->ui32TransferSize,
										 psInParams->ui32Level);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32BytesCopied = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivWriteCM(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *	pui32BytesCopied;
	PDBG_IN_WRITE	psInParams;
	PDBG_STREAM		psStream;

	psInParams = (PDBG_IN_WRITE) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psInParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32BytesCopied = ExtDBGDrivWriteCM(psStream,
										  psInParams->u.pui8InBuffer,
										  psInParams->ui32TransferSize,
										  psInParams->ui32Level);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32BytesCopied = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivRead(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *	pui32BytesCopied;
	PDBG_IN_READ	psInParams;
	PDBG_STREAM		psStream;

	psInParams = (PDBG_IN_READ) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psInParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32BytesCopied = ExtDBGDrivRead(psStream,
									   psInParams->bReadInitBuffer,
									   psInParams->ui32OutBufferSize,
									   psInParams->u.pui8OutBuffer);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32BytesCopied = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivSetCaptureMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETDEBUGMODE	psParams;
	PDBG_STREAM				psStream;

	psParams = (PDBG_IN_SETDEBUGMODE) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		ExtDBGDrivSetCaptureMode(psStream,
							 psParams->ui32Mode,
							 psParams->ui32Start,
							 psParams->ui32End,
							 psParams->ui32SampleRate);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivSetOutMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETDEBUGOUTMODE psParams;
	PDBG_STREAM				psStream;

	psParams = (PDBG_IN_SETDEBUGOUTMODE) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		ExtDBGDrivSetOutputMode(psStream,psParams->ui32Mode);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivSetDebugLevel(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETDEBUGLEVEL psParams;
	PDBG_STREAM           psStream;

	psParams = (PDBG_IN_SETDEBUGLEVEL) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		ExtDBGDrivSetDebugLevel(psStream,psParams->ui32Level);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivSetFrame(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETFRAME	psParams;
	PDBG_STREAM			psStream;

	psParams = (PDBG_IN_SETFRAME) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		ExtDBGDrivSetFrame(psStream,psParams->ui32Frame);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivGetFrame(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_STREAM  psStream;
	IMG_UINT32  *pui32Current;

	pui32Current = (IMG_UINT32 *) pvOutBuffer;
	psStream = SID2PStream(*(IMG_SID *)pvInBuffer);
	
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32Current = ExtDBGDrivGetFrame(psStream);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32Current = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivIsCaptureFrame(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_ISCAPTUREFRAME psParams;
	IMG_UINT32 *			pui32Current;
	PDBG_STREAM				psStream;

	psParams = (PDBG_IN_ISCAPTUREFRAME) pvInBuffer;
	pui32Current = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32Current = ExtDBGDrivIsCaptureFrame(psStream,
											 psParams->bCheckPreviousFrame);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32Current = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivOverrideMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_OVERRIDEMODE	psParams;
	PDBG_STREAM				psStream;

	psParams = (PDBG_IN_OVERRIDEMODE) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(	pvOutBuffer);

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		ExtDBGDrivOverrideMode(psStream,psParams->ui32Mode);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivDefaultMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_STREAM  psStream;

	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	psStream = SID2PStream(*(IMG_SID *)pvInBuffer);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		ExtDBGDrivDefaultMode(psStream);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivSetMarker(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETMARKER	psParams;
	PDBG_STREAM			psStream;

	psParams = (PDBG_IN_SETMARKER) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	psStream = SID2PStream(psParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		ExtDBGDrivSetMarker(psStream, psParams->ui32Marker);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivGetMarker(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_STREAM  psStream;
	IMG_UINT32  *pui32Current;

	pui32Current = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(*(IMG_SID *)pvInBuffer);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32Current = ExtDBGDrivGetMarker(psStream);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32Current = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivGetServiceTable(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_PVOID *	ppvOut;

	PVR_UNREFERENCED_PARAMETER(pvInBuffer);
	ppvOut = (IMG_PVOID *) pvOutBuffer;

	*ppvOut = DBGDrivGetServiceTable();

    return(IMG_TRUE);
}

static IMG_UINT32 DBGDIOCDrivWriteLF(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_WRITE_LF psInParams;
	IMG_UINT32      *pui32BytesCopied;
	PDBG_STREAM      psStream;

	psInParams = (PDBG_IN_WRITE_LF) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psInParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32BytesCopied = ExtDBGDrivWriteLF(psStream,
										  psInParams->u.pui8InBuffer,
										  psInParams->ui32BufferSize,
										  psInParams->ui32Level,
										  psInParams->ui32Flags);
		return(IMG_TRUE);
	}
	else
	{
		
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivReadLF(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *	pui32BytesCopied;
	PDBG_IN_READ	psInParams;
	PDBG_STREAM		psStream;

	psInParams = (PDBG_IN_READ) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	psStream = SID2PStream(psInParams->hStream);
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		*pui32BytesCopied = ExtDBGDrivReadLF(psStream,
										 psInParams->ui32OutBufferSize,
										 psInParams->u.pui8OutBuffer);
		return(IMG_TRUE);
	}
	else
	{
		
		*pui32BytesCopied = 0;
		return(IMG_FALSE);
	}
}

static IMG_UINT32 DBGDIOCDrivWaitForEvent(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	DBG_EVENT eEvent = (DBG_EVENT)(*(IMG_UINT32 *)pvInBuffer);

	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivWaitForEvent(eEvent);

	return(IMG_TRUE);
}

IMG_UINT32 (*g_DBGDrivProc[25])(IMG_VOID *, IMG_VOID *) =
{
	DBGDIOCDrivCreateStream,
	DBGDIOCDrivDestroyStream,
	DBGDIOCDrivGetStream,
	DBGDIOCDrivWriteString,
	DBGDIOCDrivReadString,
	DBGDIOCDrivWrite,
	DBGDIOCDrivRead,
	DBGDIOCDrivSetCaptureMode,
	DBGDIOCDrivSetOutMode,
	DBGDIOCDrivSetDebugLevel,
	DBGDIOCDrivSetFrame,
	DBGDIOCDrivGetFrame,
	DBGDIOCDrivOverrideMode,
	DBGDIOCDrivDefaultMode,
	DBGDIOCDrivGetServiceTable,
	DBGDIOCDrivWrite2,
	DBGDIOCDrivWriteStringCM,
	DBGDIOCDrivWriteCM,
	DBGDIOCDrivSetMarker,
	DBGDIOCDrivGetMarker,
	DBGDIOCDrivIsCaptureFrame,
	DBGDIOCDrivWriteLF,
	DBGDIOCDrivReadLF,
	DBGDIOCDrivWaitForEvent
};

