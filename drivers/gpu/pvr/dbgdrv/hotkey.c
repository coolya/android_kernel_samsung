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


#if !defined(LINUX)
#include <ntddk.h>
#include <windef.h>
#endif

#include "img_types.h"
#include "pvr_debug.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hotkey.h"
#include "hostfunc.h"





IMG_UINT32	g_ui32HotKeyFrame = 0xFFFFFFFF;
IMG_BOOL	g_bHotKeyPressed = IMG_FALSE;
IMG_BOOL	g_bHotKeyRegistered = IMG_FALSE;

PRIVATEHOTKEYDATA    g_PrivateHotKeyData;


IMG_VOID ReadInHotKeys(IMG_VOID)
{
	g_PrivateHotKeyData.ui32ScanCode = 0x58;	
	g_PrivateHotKeyData.ui32ShiftState = 0x0;

	

#if 0
	if (_RegOpenKey(HKEY_LOCAL_MACHINE,pszRegPath,&hKey) == ERROR_SUCCESS)
	{
		

		QueryReg(hKey,"ui32ScanCode",&g_PrivateHotKeyData.ui32ScanCode);
		QueryReg(hKey,"ui32ShiftState",&g_PrivateHotKeyData.ui32ShiftState);
	}
#else
	HostReadRegistryDWORDFromString("DEBUG\\Streams", "ui32ScanCode"  , &g_PrivateHotKeyData.ui32ScanCode);
	HostReadRegistryDWORDFromString("DEBUG\\Streams", "ui32ShiftState", &g_PrivateHotKeyData.ui32ShiftState);
#endif
}

IMG_VOID RegisterKeyPressed(IMG_UINT32 dwui32ScanCode, PHOTKEYINFO pInfo)
{
	PDBG_STREAM	psStream;

	PVR_UNREFERENCED_PARAMETER(pInfo);

	if (dwui32ScanCode == g_PrivateHotKeyData.ui32ScanCode)
	{
		PVR_DPF((PVR_DBG_MESSAGE,"PDUMP Hotkey pressed !\n"));

		psStream = (PDBG_STREAM) g_PrivateHotKeyData.sHotKeyInfo.pvStream;

		if (!g_bHotKeyPressed)
		{
			

			g_ui32HotKeyFrame = psStream->psCtrl->ui32Current + 2;

			

			g_bHotKeyPressed = IMG_TRUE;
		}
	}
}

IMG_VOID ActivateHotKeys(PDBG_STREAM psStream)
{
	

	ReadInHotKeys();

	

	if (!g_PrivateHotKeyData.sHotKeyInfo.hHotKey)
	{
		if (g_PrivateHotKeyData.ui32ScanCode != 0)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"Activate HotKey for PDUMP.\n"));

			

			g_PrivateHotKeyData.sHotKeyInfo.pvStream = psStream;

			DefineHotKey(g_PrivateHotKeyData.ui32ScanCode, g_PrivateHotKeyData.ui32ShiftState, &g_PrivateHotKeyData.sHotKeyInfo);
		}
		else
		{
			g_PrivateHotKeyData.sHotKeyInfo.hHotKey = 0;
		}
	}
}

IMG_VOID DeactivateHotKeys(IMG_VOID)
{
	if (g_PrivateHotKeyData.sHotKeyInfo.hHotKey != 0)
	{
		PVR_DPF((PVR_DBG_MESSAGE,"Deactivate HotKey.\n"));

		RemoveHotKey(g_PrivateHotKeyData.sHotKeyInfo.hHotKey);
		g_PrivateHotKeyData.sHotKeyInfo.hHotKey = 0;
	}
}


