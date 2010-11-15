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

#ifndef __TTRACE_TOKENS_H__
#define __TTRACE_TOKENS_H__

#define PVRSRV_TRACE_GROUP_KICK		0
#define PVRSRV_TRACE_GROUP_TRANSFER	1
#define PVRSRV_TRACE_GROUP_QUEUE	2
#define PVRSRV_TRACE_GROUP_POWER	3
#define PVRSRV_TRACE_GROUP_MKSYNC	4

#define PVRSRV_TRACE_GROUP_PADDING	255

#define PVRSRV_TRACE_CLASS_FUNCTION_ENTER	0
#define PVRSRV_TRACE_CLASS_FUNCTION_EXIT	1
#define PVRSRV_TRACE_CLASS_SYNC			2
#define PVRSRV_TRACE_CLASS_CCB			3
#define PVRSRV_TRACE_CLASS_CMD_START		4
#define PVRSRV_TRACE_CLASS_CMD_END		5
#define PVRSRV_TRACE_CLASS_CMD_COMP_START	6
#define PVRSRV_TRACE_CLASS_CMD_COMP_END		7

#define PVRSRV_TRACE_CLASS_NONE			255

#define PVRSRV_SYNCOP_SAMPLE		0
#define PVRSRV_SYNCOP_COMPLETE		1
#define PVRSRV_SYNCOP_DUMP		2

#define KICK_TOKEN_DOKICK		0
#define KICK_TOKEN_CCB_OFFSET		1
#define KICK_TOKEN_TA3D_SYNC		2
#define KICK_TOKEN_TA_SYNC		3
#define KICK_TOKEN_3D_SYNC		4
#define KICK_TOKEN_SRC_SYNC		5
#define KICK_TOKEN_DST_SYNC		6

#define TRANSFER_TOKEN_SUBMIT		0
#define TRANSFER_TOKEN_TA_SYNC		1
#define TRANSFER_TOKEN_3D_SYNC		2
#define TRANSFER_TOKEN_SRC_SYNC		3
#define TRANSFER_TOKEN_DST_SYNC		4
#define TRANSFER_TOKEN_CCB_OFFSET	5

#define QUEUE_TOKEN_GET_SPACE		0
#define QUEUE_TOKEN_INSERTKM		1
#define QUEUE_TOKEN_SUBMITKM		2
#define QUEUE_TOKEN_PROCESS_COMMAND	3
#define QUEUE_TOKEN_PROCESS_QUEUES	4
#define QUEUE_TOKEN_COMMAND_COMPLETE	5
#define QUEUE_TOKEN_UPDATE_DST		6
#define QUEUE_TOKEN_UPDATE_SRC		7
#define QUEUE_TOKEN_SRC_SYNC		8
#define QUEUE_TOKEN_DST_SYNC		9
#define QUEUE_TOKEN_COMMAND_TYPE	10

#define MKSYNC_TOKEN_KERNEL_CCB_OFFSET	0
#define MKSYNC_TOKEN_CORE_CLK		1
#define MKSYNC_TOKEN_UKERNEL_CLK	2

#endif 
