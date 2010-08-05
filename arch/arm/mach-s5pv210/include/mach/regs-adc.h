/* arch/arm/mach-s3c2410/include/mach/regs-adc.h
 *
 * Copyright (c) 2004 Shannon Holland <holland@loser.net>
 *
 * This program is free software; yosu can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410 ADC registers
*/

#ifndef __ASM_ARCH_REGS_ADC_H
#define __ASM_ARCH_REGS_ADC_H "regs-adc.h"

#define S3C2410_ADCREG(x) (x)

#define S3C2410_ADCCON	   S3C2410_ADCREG(0x00)
#define S3C2410_ADCTSC	   S3C2410_ADCREG(0x04)
#define S3C2410_ADCDLY	   S3C2410_ADCREG(0x08)
#define S3C2410_ADCDAT0	   S3C2410_ADCREG(0x0C)
#define S3C2410_ADCDAT1	   S3C2410_ADCREG(0x10)


/* ADCCON Register Bits */
#define S3C2410_ADCCON_ECFLG		(1<<15)
#define S3C2410_ADCCON_PRSCEN		(1<<14)
#define S3C2410_ADCCON_PRSCVL(x)	(((x)&0xFF)<<6)
#define S3C2410_ADCCON_PRSCVLMASK	(0xFF<<6)
#define S3C2410_ADCCON_SELMUX(x)	(((x)&0x7)<<3)
#define S3C2410_ADCCON_MUXMASK		(0x7<<3)
#define S3C2410_ADCCON_STDBM		(1<<2)
#define S3C2410_ADCCON_READ_START	(1<<1)
#define S3C2410_ADCCON_ENABLE_START	(1<<0)
#define S3C2410_ADCCON_STARTMASK	(0x3<<0)


/* ADCTSC Register Bits */
#define S3C2410_ADCTSC_YM_SEN		(1<<7)
#define S3C2410_ADCTSC_YP_SEN		(1<<6)
#define S3C2410_ADCTSC_XM_SEN		(1<<5)
#define S3C2410_ADCTSC_XP_SEN		(1<<4)
#define S3C2410_ADCTSC_PULL_UP_DISABLE	(1<<3)
#define S3C2410_ADCTSC_AUTO_PST		(1<<2)
#define S3C2410_ADCTSC_XY_PST(x)	(((x)&0x3)<<0)

/* ADCDAT0 Bits */
#define S3C2410_ADCDAT0_UPDOWN		(1<<15)
#define S3C2410_ADCDAT0_AUTO_PST	(1<<14)
#define S3C2410_ADCDAT0_XY_PST		(0x3<<12)
#define S3C2410_ADCDAT0_XPDATA_MASK	(0x03FF)

/* ADCDAT1 Bits */
#define S3C2410_ADCDAT1_UPDOWN		(1<<15)
#define S3C2410_ADCDAT1_AUTO_PST	(1<<14)
#define S3C2410_ADCDAT1_XY_PST		(0x3<<12)
#define S3C2410_ADCDAT1_YPDATA_MASK	(0x03FF)

/*--------------------------- Common definitions for S3C  ---------------------------*/
/* The following definitions will be applied to S3C24XX, S3C64XX, S5PC1XX.	     */
/*-----------------------------------------------------------------------------------*/

#define S3C_ADCREG(x) 			(x)

#define S3C_ADCCON	   		S3C_ADCREG(0x00)
#define S3C_ADCTSC	  		S3C_ADCREG(0x04)
#define S3C_ADCDLY	   		S3C_ADCREG(0x08)
#define S3C_ADCDAT0	   		S3C_ADCREG(0x0C)
#define S3C_ADCDAT1	   		S3C_ADCREG(0x10)
#define S3C_ADCUPDN			S3C_ADCREG(0x14)
#define S3C_ADCCLRINT			S3C_ADCREG(0x18)
#define S3C_ADCMUX			S3C_ADCREG(0x1C)
#define S3C_ADCCLRWK			S3C_ADCREG(0x20)


/* ADCCON Register Bits */
#define S3C_ADCCON_RESSEL_10BIT		(0x0<<16)
#define S3C_ADCCON_RESSEL_12BIT		(0x1<<16)
#define S3C_ADCCON_ECFLG		(1<<15)
#define S3C_ADCCON_PRSCEN		(1<<14)
#define S3C_ADCCON_PRSCVL(x)		(((x)&0xFF)<<6)
#define S3C_ADCCON_PRSCVLMASK		(0xFF<<6)
#define S3C_ADCCON_SELMUX(x)		(((x)&0x7)<<3)
#define S3C_ADCCON_SELMUX_1(x)		(((x)&0xF)<<0)
#define S3C_ADCCON_MUXMASK		(0x7<<3)
#define S3C_ADCCON_RESSEL_10BIT_1	(0x0<<3)
#define S3C_ADCCON_RESSEL_12BIT_1	(0x1<<3)
#define S3C_ADCCON_STDBM		(1<<2)
#define S3C_ADCCON_READ_START		(1<<1)
#define S3C_ADCCON_ENABLE_START		(1<<0)
#define S3C_ADCCON_STARTMASK		(0x3<<0)


/* ADCTSC Register Bits */
#define S3C_ADCTSC_UD_SEN		(1<<8)
#define S3C_ADCTSC_YM_SEN		(1<<7)
#define S3C_ADCTSC_YP_SEN		(1<<6)
#define S3C_ADCTSC_XM_SEN		(1<<5)
#define S3C_ADCTSC_XP_SEN		(1<<4)
#define S3C_ADCTSC_PULL_UP_DISABLE	(1<<3)
#define S3C_ADCTSC_AUTO_PST		(1<<2)
#define S3C_ADCTSC_XY_PST(x)		(((x)&0x3)<<0)

/* ADCDAT0 Bits */
#define S3C_ADCDAT0_UPDOWN		(1<<15)
#define S3C_ADCDAT0_AUTO_PST		(1<<14)
#define S3C_ADCDAT0_XY_PST		(0x3<<12)
#define S3C_ADCDAT0_XPDATA_MASK		(0x03FF)
#define S3C_ADCDAT0_XPDATA_MASK_12BIT	(0x0FFF)

/* ADCDAT1 Bits */
#define S3C_ADCDAT1_UPDOWN		(1<<15)
#define S3C_ADCDAT1_AUTO_PST		(1<<14)
#define S3C_ADCDAT1_XY_PST		(0x3<<12)
#define S3C_ADCDAT1_YPDATA_MASK		(0x03FF)
#define S3C_ADCDAT1_YPDATA_MASK_12BIT	(0x0FFF)

#endif /* __ASM_ARCH_REGS_ADC_H */


