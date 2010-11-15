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

#include "img_types.h"

#ifndef __TTRACE_COMMON_H__
#define __TTRACE_COMMON_H__

#define PVRSRV_TRACE_HEADER		0
#define PVRSRV_TRACE_TIMESTAMP		1
#define PVRSRV_TRACE_HOSTUID		2
#define PVRSRV_TRACE_DATA_HEADER	3
#define PVRSRV_TRACE_DATA_PAYLOAD	4

#define PVRSRV_TRACE_ITEM_SIZE		16

#define PVRSRV_TRACE_GROUP_MASK		0xff
#define PVRSRV_TRACE_CLASS_MASK		0xff
#define PVRSRV_TRACE_TOKEN_MASK		0xffff

#define PVRSRV_TRACE_GROUP_SHIFT	24
#define PVRSRV_TRACE_CLASS_SHIFT	16
#define PVRSRV_TRACE_TOKEN_SHIFT	0

#define PVRSRV_TRACE_SIZE_MASK		0xffff
#define PVRSRV_TRACE_TYPE_MASK		0xf
#define PVRSRV_TRACE_COUNT_MASK		0xfff

#define PVRSRV_TRACE_SIZE_SHIFT		16
#define PVRSRV_TRACE_TYPE_SHIFT		12
#define PVRSRV_TRACE_COUNT_SHIFT	0


#define WRITE_HEADER(n,m) \
	((m & PVRSRV_TRACE_##n##_MASK) << PVRSRV_TRACE_##n##_SHIFT)

#define READ_HEADER(n,m) \
	((m & (PVRSRV_TRACE_##n##_MASK << PVRSRV_TRACE_##n##_SHIFT)) >> PVRSRV_TRACE_##n##_SHIFT)

#define TIME_TRACE_BUFFER_SIZE		4096

#define PVRSRV_TRACE_TYPE_UI8		0
#define PVRSRV_TRACE_TYPE_UI16		1
#define PVRSRV_TRACE_TYPE_UI32		2
#define PVRSRV_TRACE_TYPE_UI64		3

#define PVRSRV_TRACE_TYPE_SYNC		15
 #define PVRSRV_TRACE_SYNC_UID		0
 #define PVRSRV_TRACE_SYNC_WOP		1
 #define PVRSRV_TRACE_SYNC_WOC		2
 #define PVRSRV_TRACE_SYNC_ROP		3
 #define PVRSRV_TRACE_SYNC_ROC		4
 #define PVRSRV_TRACE_SYNC_WO_DEV_VADDR	5
 #define PVRSRV_TRACE_SYNC_RO_DEV_VADDR	6
 #define PVRSRV_TRACE_SYNC_OP		7
#define PVRSRV_TRACE_TYPE_SYNC_SIZE	((PVRSRV_TRACE_SYNC_OP + 1) * sizeof(IMG_UINT32))

#endif 
