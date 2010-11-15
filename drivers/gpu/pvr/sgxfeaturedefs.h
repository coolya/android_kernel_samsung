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

#if defined(SGX520)
	#define SGX_CORE_FRIENDLY_NAME							"SGX520"
	#define SGX_CORE_ID										SGX_CORE_ID_520
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(28)
	#define SGX_FEATURE_AUTOCLOCKGATING
#else
#if defined(SGX530)
	#define SGX_CORE_FRIENDLY_NAME							"SGX530"
	#define SGX_CORE_ID										SGX_CORE_ID_530
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(28)
	#define SGX_FEATURE_AUTOCLOCKGATING
#else
#if defined(SGX531)
	#define SGX_CORE_FRIENDLY_NAME							"SGX531"
	#define SGX_CORE_ID										SGX_CORE_ID_531
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(28)
	#define SGX_FEATURE_AUTOCLOCKGATING
	#define SGX_FEATURE_MULTI_EVENT_KICK
#else
#if defined(SGX535)
	#define SGX_CORE_FRIENDLY_NAME							"SGX535"
	#define SGX_CORE_ID										SGX_CORE_ID_535
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(32)
	#define SGX_FEATURE_MULTIPLE_MEM_CONTEXTS
	#define SGX_FEATURE_BIF_NUM_DIRLISTS					(16)
	#define SGX_FEATURE_2D_HARDWARE
	#define SGX_FEATURE_AUTOCLOCKGATING
	#define SUPPORT_SGX_GENERAL_MAPPING_HEAP
#else
#if defined(SGX540)
	#define SGX_CORE_FRIENDLY_NAME							"SGX540"
	#define SGX_CORE_ID										SGX_CORE_ID_540
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(28)
	#define SGX_FEATURE_AUTOCLOCKGATING
	#define SGX_FEATURE_MULTI_EVENT_KICK
#else
#if defined(SGX541)
	#define SGX_CORE_FRIENDLY_NAME							"SGX541"
	#define SGX_CORE_ID										SGX_CORE_ID_541
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(32)
	#define SGX_FEATURE_MULTIPLE_MEM_CONTEXTS
	#define SGX_FEATURE_BIF_NUM_DIRLISTS					(8)
	#define SGX_FEATURE_AUTOCLOCKGATING
    #define SGX_FEATURE_SPM_MODE_0
	#define SGX_FEATURE_MULTI_EVENT_KICK
#else
#if defined(SGX543)
	#define SGX_CORE_FRIENDLY_NAME							"SGX543"
	#define SGX_CORE_ID										SGX_CORE_ID_543
	#define SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING
	#define SGX_FEATURE_USE_UNLIMITED_PHASES
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(32)
	#define SGX_FEATURE_MULTIPLE_MEM_CONTEXTS
	#define SGX_FEATURE_BIF_NUM_DIRLISTS					(8)
	#define SGX_FEATURE_AUTOCLOCKGATING
	#define SGX_FEATURE_MONOLITHIC_UKERNEL
    #define SGX_FEATURE_SPM_MODE_0
	#define SGX_FEATURE_MULTI_EVENT_KICK
	#define SGX_FEATURE_DATA_BREAKPOINTS
	#define SGX_FEATURE_2D_HARDWARE
	#define SGX_FEATURE_PTLA
	#define SGX_FEATURE_EXTENDED_PERF_COUNTERS
#else
#if defined(SGX544)
	#define SGX_CORE_FRIENDLY_NAME							"SGX544"
	#define SGX_CORE_ID										SGX_CORE_ID_544
	#define SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING
	#define SGX_FEATURE_USE_UNLIMITED_PHASES
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(32)
	#define SGX_FEATURE_MULTIPLE_MEM_CONTEXTS
	#define SGX_FEATURE_BIF_NUM_DIRLISTS					(8)
	#define SGX_FEATURE_AUTOCLOCKGATING
	#define SGX_FEATURE_MONOLITHIC_UKERNEL
    #define SGX_FEATURE_SPM_MODE_0
	#define SGX_FEATURE_MULTI_EVENT_KICK
	#define SGX_FEATURE_DATA_BREAKPOINTS
	#define SGX_FEATURE_EXTENDED_PERF_COUNTERS
#else
#if defined(SGX545)
	#define SGX_CORE_FRIENDLY_NAME							"SGX545"
	#define SGX_CORE_ID										SGX_CORE_ID_545
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(32)
	#define SGX_FEATURE_AUTOCLOCKGATING
	#define SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING
	#define SGX_FEATURE_USE_UNLIMITED_PHASES
	#define SGX_FEATURE_VOLUME_TEXTURES
	#define SGX_FEATURE_HOST_ALLOC_FROM_DPM
	#define SGX_FEATURE_MULTIPLE_MEM_CONTEXTS
	#define SGX_FEATURE_BIF_NUM_DIRLISTS				(16)
	#define SGX_FEATURE_NUM_USE_PIPES					(4)
	#define	SGX_FEATURE_TEXTURESTRIDE_EXTENSION
	#define SGX_FEATURE_PDS_DATA_INTERLEAVE_2DWORDS
	#define SGX_FEATURE_MONOLITHIC_UKERNEL
	#define SGX_FEATURE_ZLS_EXTERNALZ
	#define SGX_FEATURE_VDM_CONTEXT_SWITCH_REV_2
	#define SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_2
	#define SGX_FEATURE_NUM_PDS_PIPES					(2)
	#define SGX_FEATURE_NATIVE_BACKWARD_BLIT
	#define SGX_FEATURE_MAX_TA_RENDER_TARGETS				(512)
	#define SGX_FEATURE_SPM_MODE_0
	#define SGX_FEATURE_SECONDARY_REQUIRES_USE_KICK
	#define SGX_FEATURE_WRITEBACK_DCU
	
	
	#define SGX_FEATURE_BIF_WIDE_TILING_AND_4K_ADDRESS
	#define SGX_FEATURE_MULTI_EVENT_KICK
#else
#if defined(SGX554)
	#define SGX_CORE_FRIENDLY_NAME							"SGX554"
	#define SGX_CORE_ID										SGX_CORE_ID_554
	#define SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING
	#define SGX_FEATURE_USE_UNLIMITED_PHASES
	#define SGX_FEATURE_ADDRESS_SPACE_SIZE					(32)
	#define SGX_FEATURE_MULTIPLE_MEM_CONTEXTS
	#define SGX_FEATURE_BIF_NUM_DIRLISTS					(8)
	#define SGX_FEATURE_AUTOCLOCKGATING
	#define SGX_FEATURE_MONOLITHIC_UKERNEL
    #define SGX_FEATURE_SPM_MODE_0
	#define SGX_FEATURE_MULTI_EVENT_KICK
	#define SGX_FEATURE_DATA_BREAKPOINTS
	#define SGX_FEATURE_EXTENDED_PERF_COUNTERS
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#if defined(FIX_HW_BRN_22693)
#undef SGX_FEATURE_AUTOCLOCKGATING
#endif

#if defined(FIX_HW_BRN_27266)
#undef SGX_FEATURE_36BIT_MMU
#endif

#if defined(FIX_HW_BRN_27456)
#undef SGX_FEATURE_BIF_WIDE_TILING_AND_4K_ADDRESS
#endif

#if defined(FIX_HW_BRN_22934)	\
	|| defined(FIX_HW_BRN_25499)
#undef SGX_FEATURE_MULTI_EVENT_KICK
#endif

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	#if defined(SGX_FEATURE_36BIT_MMU)
		#error SGX_FEATURE_SYSTEM_CACHE is incompatible with SGX_FEATURE_36BIT_MMU
	#endif
	#if defined(FIX_HW_BRN_26620) && !defined(SGX_FEATURE_MULTI_EVENT_KICK)
		#define SGX_BYPASS_SYSTEM_CACHE
	#endif
#endif

#if defined(SGX_FEATURE_MP)
#if !defined(SGX_FEATURE_MP_CORE_COUNT)
#error SGX_FEATURE_MP_CORE_COUNT must be defined when SGX_FEATURE_MP is defined
#endif
#else
#define SGX_FEATURE_MP_CORE_COUNT	(1)
#endif

#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && !defined(SUPPORT_SGX_PRIORITY_SCHEDULING)
#define SUPPORT_SGX_PRIORITY_SCHEDULING
#endif

#include "img_types.h"

