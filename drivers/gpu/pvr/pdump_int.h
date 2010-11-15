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

#ifndef __PDUMP_INT_H__
#define __PDUMP_INT_H__

#if defined (__cplusplus)
extern "C" {
#endif

#if !defined(_UITRON)
#include "dbgdrvif.h"

IMG_EXPORT IMG_VOID PDumpConnectionNotify(IMG_VOID);

#endif 

typedef enum
{
	
	PDUMP_WRITE_MODE_CONTINUOUS = 0,
	
	PDUMP_WRITE_MODE_LASTFRAME,
	
	PDUMP_WRITE_MODE_BINCM,
	
	PDUMP_WRITE_MODE_PERSISTENT
} PDUMP_DDWMODE;


IMG_UINT32 DbgWrite(PDBG_STREAM psStream, IMG_UINT8 *pui8Data, IMG_UINT32 ui32BCount, IMG_UINT32 ui32Flags);

IMG_UINT32 PDumpOSDebugDriverWrite(	PDBG_STREAM psStream,
									PDUMP_DDWMODE eDbgDrvWriteMode,
									IMG_UINT8 *pui8Data,
									IMG_UINT32 ui32BCount,
									IMG_UINT32 ui32Level,
									IMG_UINT32 ui32DbgDrvFlags);

#if defined (__cplusplus)
}
#endif
#endif 

