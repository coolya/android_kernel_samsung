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

#ifndef __SGXSCRIPT_H__
#define __SGXSCRIPT_H__

#if defined (__cplusplus)
extern "C" {
#endif

#define	SGX_MAX_INIT_COMMANDS	64
#define	SGX_MAX_DEINIT_COMMANDS	16

typedef	enum _SGX_INIT_OPERATION
{
	SGX_INIT_OP_ILLEGAL = 0,
	SGX_INIT_OP_WRITE_HW_REG,
#if defined(PDUMP)
	SGX_INIT_OP_PDUMP_HW_REG,
#endif
	SGX_INIT_OP_HALT
} SGX_INIT_OPERATION;

typedef union _SGX_INIT_COMMAND
{
	SGX_INIT_OPERATION eOp;
	struct {
		SGX_INIT_OPERATION eOp;
		IMG_UINT32 ui32Offset;
		IMG_UINT32 ui32Value;
	} sWriteHWReg;
#if defined(PDUMP)
	struct {
		SGX_INIT_OPERATION eOp;
		IMG_UINT32 ui32Offset;
		IMG_UINT32 ui32Value;
	} sPDumpHWReg;
#endif
#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)			
	struct {
		SGX_INIT_OPERATION eOp;
	} sWorkaroundBRN22997;
#endif	
} SGX_INIT_COMMAND;

typedef struct _SGX_INIT_SCRIPTS_
{
	SGX_INIT_COMMAND asInitCommandsPart1[SGX_MAX_INIT_COMMANDS];
	SGX_INIT_COMMAND asInitCommandsPart2[SGX_MAX_INIT_COMMANDS];
	SGX_INIT_COMMAND asDeinitCommands[SGX_MAX_DEINIT_COMMANDS];
} SGX_INIT_SCRIPTS;

#if defined(__cplusplus)
}
#endif

#endif 

