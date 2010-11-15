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

#include "img_defs.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"

#define MAX_SID_ENTRIES		8

typedef struct _SID_INFO
{
	PDBG_STREAM	psStream;
} SID_INFO, *PSID_INFO;

static SID_INFO gaSID_Xlat_Table[MAX_SID_ENTRIES];

IMG_SID PStream2SID(PDBG_STREAM psStream)
{
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		IMG_INT32 iIdx;

		for (iIdx = 0; iIdx < MAX_SID_ENTRIES; iIdx++)
		{
			if (psStream == gaSID_Xlat_Table[iIdx].psStream)
			{
				
				return (IMG_SID)iIdx+1;
			}
		}
	}

	return (IMG_SID)0;
}


PDBG_STREAM SID2PStream(IMG_SID hStream)
{
	
	IMG_INT32 iIdx = (IMG_INT32)hStream-1;

	if (iIdx >= 0 && iIdx < MAX_SID_ENTRIES)
	{
		return gaSID_Xlat_Table[iIdx].psStream;
	}
	else
	{
    	return (PDBG_STREAM)IMG_NULL;
    }
}


IMG_BOOL AddSIDEntry(PDBG_STREAM psStream)
{
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		IMG_INT32 iIdx;

		for (iIdx = 0; iIdx < MAX_SID_ENTRIES; iIdx++)
		{
			if (psStream == gaSID_Xlat_Table[iIdx].psStream)
			{
				
				return IMG_TRUE;
			}

			if (gaSID_Xlat_Table[iIdx].psStream == (PDBG_STREAM)IMG_NULL)
			{
				
				gaSID_Xlat_Table[iIdx].psStream = psStream;
				return IMG_TRUE;
			}
		}
	}

	return IMG_FALSE;
}

IMG_BOOL RemoveSIDEntry(PDBG_STREAM psStream)
{
	if (psStream != (PDBG_STREAM)IMG_NULL)
	{
		IMG_INT32 iIdx;

		for (iIdx = 0; iIdx < MAX_SID_ENTRIES; iIdx++)
		{
			if (psStream == gaSID_Xlat_Table[iIdx].psStream)
			{
				gaSID_Xlat_Table[iIdx].psStream = (PDBG_STREAM)IMG_NULL;
				return IMG_TRUE;
			}
		}
	}

	return IMG_FALSE;
}


