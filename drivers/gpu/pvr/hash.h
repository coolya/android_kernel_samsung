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

#ifndef _HASH_H_
#define _HASH_H_

#include "img_types.h"
#include "osfunc.h"

#if defined (__cplusplus)
extern "C" {
#endif

typedef IMG_UINT32 HASH_FUNC(IMG_SIZE_T uKeySize, IMG_VOID *pKey, IMG_UINT32 uHashTabLen);
typedef IMG_BOOL HASH_KEY_COMP(IMG_SIZE_T uKeySize, IMG_VOID *pKey1, IMG_VOID *pKey2);

typedef struct _HASH_TABLE_ HASH_TABLE;

IMG_UINT32 HASH_Func_Default (IMG_SIZE_T uKeySize, IMG_VOID *pKey, IMG_UINT32 uHashTabLen);

IMG_BOOL HASH_Key_Comp_Default (IMG_SIZE_T uKeySize, IMG_VOID *pKey1, IMG_VOID *pKey2);

HASH_TABLE * HASH_Create_Extended (IMG_UINT32 uInitialLen, IMG_SIZE_T uKeySize, HASH_FUNC *pfnHashFunc, HASH_KEY_COMP *pfnKeyComp);

HASH_TABLE * HASH_Create (IMG_UINT32 uInitialLen);

IMG_VOID HASH_Delete (HASH_TABLE *pHash);

IMG_BOOL HASH_Insert_Extended (HASH_TABLE *pHash, IMG_VOID *pKey, IMG_UINTPTR_T v);

IMG_BOOL HASH_Insert (HASH_TABLE *pHash, IMG_UINTPTR_T k, IMG_UINTPTR_T v);

IMG_UINTPTR_T HASH_Remove_Extended(HASH_TABLE *pHash, IMG_VOID *pKey);

IMG_UINTPTR_T HASH_Remove (HASH_TABLE *pHash, IMG_UINTPTR_T k);

IMG_UINTPTR_T HASH_Retrieve_Extended (HASH_TABLE *pHash, IMG_VOID *pKey);

IMG_UINTPTR_T HASH_Retrieve (HASH_TABLE *pHash, IMG_UINTPTR_T k);

#ifdef HASH_TRACE
IMG_VOID HASH_Dump (HASH_TABLE *pHash);
#endif

#if defined (__cplusplus)
}
#endif

#endif 

