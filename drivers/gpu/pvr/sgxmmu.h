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

#if !defined(__SGXMMU_KM_H__)
#define __SGXMMU_KM_H__

#define SGX_MMU_PAGE_SHIFT				(12)
#define SGX_MMU_PAGE_SIZE				(1U<<SGX_MMU_PAGE_SHIFT)
#define SGX_MMU_PAGE_MASK				(SGX_MMU_PAGE_SIZE - 1U)

#define SGX_MMU_PD_SHIFT				(10)
#define SGX_MMU_PD_SIZE					(1U<<SGX_MMU_PD_SHIFT)
#define SGX_MMU_PD_MASK					(0xFFC00000U)

#if defined(SGX_FEATURE_36BIT_MMU)
	#define SGX_MMU_PDE_ADDR_MASK			(0xFFFFFF00U)
	#define SGX_MMU_PDE_ADDR_ALIGNSHIFT		(4)
#else
	#define SGX_MMU_PDE_ADDR_MASK			(0xFFFFF000U)
	#define SGX_MMU_PDE_ADDR_ALIGNSHIFT		(0)
#endif
#define SGX_MMU_PDE_VALID				(0x00000001U)
#define SGX_MMU_PDE_PAGE_SIZE_4K		(0x00000000U)
#if defined(SGX_FEATURE_VARIABLE_MMU_PAGE_SIZE)
	#define SGX_MMU_PDE_PAGE_SIZE_16K		(0x00000002U)
	#define SGX_MMU_PDE_PAGE_SIZE_64K		(0x00000004U)
	#define SGX_MMU_PDE_PAGE_SIZE_256K		(0x00000006U)
	#define SGX_MMU_PDE_PAGE_SIZE_1M		(0x00000008U)
	#define SGX_MMU_PDE_PAGE_SIZE_4M		(0x0000000AU)
	#define SGX_MMU_PDE_PAGE_SIZE_MASK		(0x0000000EU)
#else
	#define SGX_MMU_PDE_WRITEONLY			(0x00000002U)
	#define SGX_MMU_PDE_READONLY			(0x00000004U)
	#define SGX_MMU_PDE_CACHECONSISTENT		(0x00000008U)
	#define SGX_MMU_PDE_EDMPROTECT			(0x00000010U)
#endif

#define SGX_MMU_PT_SHIFT				(10)
#define SGX_MMU_PT_SIZE					(1U<<SGX_MMU_PT_SHIFT)
#define SGX_MMU_PT_MASK					(0x003FF000U)

#if defined(SGX_FEATURE_36BIT_MMU)
	#define SGX_MMU_PTE_ADDR_MASK			(0xFFFFFF00U)
	#define SGX_MMU_PTE_ADDR_ALIGNSHIFT		(4)
#else
	#define SGX_MMU_PTE_ADDR_MASK			(0xFFFFF000U)
	#define SGX_MMU_PTE_ADDR_ALIGNSHIFT		(0)
#endif
#define SGX_MMU_PTE_VALID				(0x00000001U)
#define SGX_MMU_PTE_WRITEONLY			(0x00000002U)
#define SGX_MMU_PTE_READONLY			(0x00000004U)
#define SGX_MMU_PTE_CACHECONSISTENT		(0x00000008U)
#define SGX_MMU_PTE_EDMPROTECT			(0x00000010U)

#endif	

