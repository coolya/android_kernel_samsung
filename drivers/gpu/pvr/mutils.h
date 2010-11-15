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

#ifndef __IMG_LINUX_MUTILS_H__
#define __IMG_LINUX_MUTILS_H__

#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif

#include <linux/version.h>

#if !(defined(__i386__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)))
#if defined(SUPPORT_LINUX_X86_PAT)
#undef SUPPORT_LINUX_X86_PAT
#endif
#endif

#if defined(SUPPORT_LINUX_X86_PAT)
	pgprot_t pvr_pgprot_writecombine(pgprot_t prot);
	#define	PGPROT_WC(pv)	pvr_pgprot_writecombine(pv)
#else
	#if defined(__arm__) || defined(__sh__)
		#define	PGPROT_WC(pv)	pgprot_writecombine(pv)
	#else
		#if defined(__i386__)
			#define	PGPROT_WC(pv)	pgprot_noncached(pv)
		#else
			#define PGPROT_WC(pv)	pgprot_noncached(pv)
			#error  Unsupported architecture!
		#endif
	#endif
#endif

#define	PGPROT_UC(pv)	pgprot_noncached(pv)

#if defined(__i386__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	#define	IOREMAP(pa, bytes)	ioremap_cache(pa, bytes)
#else	
	#if defined(__arm__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
		#define	IOREMAP(pa, bytes)	ioremap_cached(pa, bytes)
	#else
		#define IOREMAP(pa, bytes)	ioremap(pa, bytes)
	#endif
#endif

#if defined(SUPPORT_LINUX_X86_PAT)
	#if defined(SUPPORT_LINUX_X86_WRITECOMBINE)
		#define IOREMAP_WC(pa, bytes) ioremap_wc(pa, bytes)
	#else
		#define IOREMAP_WC(pa, bytes) ioremap_nocache(pa, bytes)
	#endif
#else
	#if defined(__arm__)
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
			#define IOREMAP_WC(pa, bytes) ioremap_wc(pa, bytes)
		#else
			#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
				#define	IOREMAP_WC(pa, bytes)	ioremap_nocache(pa, bytes)
			#else
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17))
					#define	IOREMAP_WC(pa, bytes)	__ioremap(pa, bytes, L_PTE_BUFFERABLE)
				#else
					#define IOREMAP_WC(pa, bytes)	__ioremap(pa, bytes, , L_PTE_BUFFERABLE, 1)
				#endif
			#endif
		#endif
	#else
		#define IOREMAP_WC(pa, bytes)	ioremap_nocache(pa, bytes)
	#endif
#endif

#define	IOREMAP_UC(pa, bytes)	ioremap_nocache(pa, bytes)

IMG_VOID PVRLinuxMUtilsInit(IMG_VOID);

#endif 

