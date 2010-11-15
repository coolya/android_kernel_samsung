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

#ifndef _HOTKEY_
#define _HOTKEY_


typedef struct _hotkeyinfo
{
	IMG_UINT8 ui8ScanCode;
	IMG_UINT8 ui8Type;
	IMG_UINT8 ui8Flag;
	IMG_UINT8 ui8Filler1;
	IMG_UINT32 ui32ShiftState;
	IMG_UINT32 ui32HotKeyProc;
	IMG_VOID *pvStream;
	IMG_UINT32 hHotKey;			
} HOTKEYINFO, *PHOTKEYINFO;

typedef struct _privatehotkeydata
{
	IMG_UINT32		ui32ScanCode;
	IMG_UINT32		ui32ShiftState;
	HOTKEYINFO	sHotKeyInfo;
} PRIVATEHOTKEYDATA, *PPRIVATEHOTKEYDATA;


IMG_VOID ReadInHotKeys (IMG_VOID);
IMG_VOID ActivateHotKeys(PDBG_STREAM psStream);
IMG_VOID DeactivateHotKeys(IMG_VOID);

IMG_VOID RemoveHotKey (IMG_UINT32 hHotKey);
IMG_VOID DefineHotKey (IMG_UINT32 ui32ScanCode, IMG_UINT32 ui32ShiftState, PHOTKEYINFO psInfo);
IMG_VOID RegisterKeyPressed (IMG_UINT32 ui32ScanCode, PHOTKEYINFO psInfo);

#endif

