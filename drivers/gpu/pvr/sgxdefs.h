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

#ifndef _SGXDEFS_H_
#define	_SGXDEFS_H_

#include "sgxerrata.h"
#include "sgxfeaturedefs.h"

#if defined(SGX520)
#include "sgx520defs.h"
#else
#if defined(SGX530)
#include "sgx530defs.h"
#else
#if defined(SGX535)
#include "sgx535defs.h"
#else
#if defined(SGX535_V1_1)
#include "sgx535defs.h"
#else
#if defined(SGX540)
#include "sgx540defs.h"
#else
#if defined(SGX541)
#include "sgx541defs.h"
#else
#if defined(SGX543)
#include "sgx543defs.h"
#else
#if defined(SGX544)
#include "sgx544defs.h"
#else
#if defined(SGX545)
#include "sgx545defs.h"
#else
#if defined(SGX531)
#include "sgx531defs.h"
#else
#if defined(SGX554)
#include "sgx554defs.h"
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
#endif

#if defined(SGX_FEATURE_MP)
#if defined(SGX541)
#if SGX_CORE_REV == 100
#include "sgx541_100mpdefs.h"
#else
#include "sgx541mpdefs.h"
#endif 
#else
#include "sgxmpdefs.h"
#endif 
#else 
#if defined(SGX_FEATURE_SYSTEM_CACHE)
#include "mnemedefs.h"
#endif
#endif 

#endif 

