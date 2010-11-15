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

#ifndef _METRICS_
#define _METRICS_


#if defined (__cplusplus)
extern "C" {
#endif


#if defined(DEBUG) || defined(TIMING)


typedef struct 
{
	IMG_UINT32 ui32Start;
	IMG_UINT32 ui32Stop;
	IMG_UINT32 ui32Total;
	IMG_UINT32 ui32Count;
} Temporal_Data;

extern Temporal_Data asTimers[]; 

extern IMG_UINT32 PVRSRVTimeNow(IMG_VOID);
extern IMG_VOID   PVRSRVSetupMetricTimers(IMG_VOID *pvDevInfo);
extern IMG_VOID   PVRSRVOutputMetricTotals(IMG_VOID);


#define PVRSRV_TIMER_DUMMY				0

#define PVRSRV_TIMER_EXAMPLE_1			1
#define PVRSRV_TIMER_EXAMPLE_2			2


#define PVRSRV_NUM_TIMERS		(PVRSRV_TIMER_EXAMPLE_2 + 1)

#define PVRSRV_TIME_START(X)	{ \
									asTimers[X].ui32Count += 1; \
									asTimers[X].ui32Count |= 0x80000000L; \
									asTimers[X].ui32Start = PVRSRVTimeNow(); \
									asTimers[X].ui32Stop  = 0; \
								}

#define PVRSRV_TIME_SUSPEND(X)	{ \
									asTimers[X].ui32Stop += PVRSRVTimeNow() - asTimers[X].ui32Start; \
								}

#define PVRSRV_TIME_RESUME(X)	{ \
									asTimers[X].ui32Start = PVRSRVTimeNow(); \
								}

#define PVRSRV_TIME_STOP(X)		{ \
									asTimers[X].ui32Stop  += PVRSRVTimeNow() - asTimers[X].ui32Start; \
									asTimers[X].ui32Total += asTimers[X].ui32Stop; \
									asTimers[X].ui32Count &= 0x7FFFFFFFL; \
								}

#define PVRSRV_TIME_RESET(X)	{ \
									asTimers[X].ui32Start = 0; \
									asTimers[X].ui32Stop  = 0; \
									asTimers[X].ui32Total = 0; \
									asTimers[X].ui32Count = 0; \
								}


#if defined(__sh__)

#define TST_REG   ((volatile IMG_UINT8 *) (psDevInfo->pvSOCRegsBaseKM)) 	

#define TCOR_2    ((volatile IMG_UINT *)  (psDevInfo->pvSOCRegsBaseKM+28))	
#define TCNT_2    ((volatile IMG_UINT *)  (psDevInfo->pvSOCRegsBaseKM+32))	
#define TCR_2     ((volatile IMG_UINT16 *)(psDevInfo->pvSOCRegsBaseKM+36))	

#define TIMER_DIVISOR  4

#endif 





#else 



#define PVRSRV_TIME_START(X)
#define PVRSRV_TIME_SUSPEND(X)
#define PVRSRV_TIME_RESUME(X)
#define PVRSRV_TIME_STOP(X)
#define PVRSRV_TIME_RESET(X)

#define PVRSRVSetupMetricTimers(X)
#define PVRSRVOutputMetricTotals()



#endif 

#if defined(__cplusplus)
}
#endif


#endif 

