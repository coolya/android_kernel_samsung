/* linux/drivers/media/video/samsung/tv20/s5pc100/regs/regs-vprocessor.h
 *
 * Video Processor register header file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_VPROCESSOR_H

#include <mach/map.h>

#define S5P_VPROCESSOR_BASE(x) (x)
/*
 Registers
*/
#define S5P_VP_ENABLE			S5P_VPROCESSOR_BASE(0x0000)	//  Power-Down Ready & Enable 
#define S5P_VP_SRESET			S5P_VPROCESSOR_BASE(0x0004)	//  Software Reset 
#define S5P_VP_SHADOW_UPDATE		S5P_VPROCESSOR_BASE(0x0008)	//  Shadow Register Update Enable 
#define S5P_VP_FIELD_ID			S5P_VPROCESSOR_BASE(0x000C)	//  Field ID of the "Source" Image 
#define S5P_VP_MODE			S5P_VPROCESSOR_BASE(0x0010)	//  VP Operation Mode 
#define S5P_VP_IMG_SIZE_Y		S5P_VPROCESSOR_BASE(0x0014)	//  Luminance Date Size 
#define S5P_VP_IMG_SIZE_C		S5P_VPROCESSOR_BASE(0x0018)	//  Chrominance Date Size 
#define S5P_VP_PER_RATE_CTRL		S5P_VPROCESSOR_BASE(0x001C)
#define S5P_VP_TOP_Y_PTR		S5P_VPROCESSOR_BASE(0x0028)	//  Base Address for Y of Top Field (Frame) 
#define S5P_VP_BOT_Y_PTR		S5P_VPROCESSOR_BASE(0x002C)	//  Base Address for Y of Bottom Field 
#define S5P_VP_TOP_C_PTR		S5P_VPROCESSOR_BASE(0x0030)	//  Base Address for C of Top
#define S5P_VP_BOT_C_PTR		S5P_VPROCESSOR_BASE(0x0034)	//  Base Address for C of Bottom Field 
#define S5P_VP_ENDIAN_MODE		S5P_VPROCESSOR_BASE(0x03CC)	//  Big/Little Endian Mode Selection 
#define S5P_VP_SRC_H_POSITION		S5P_VPROCESSOR_BASE(0x0044)	//  Horizontal Offset in the Source Image 
#define S5P_VP_SRC_V_POSITION		S5P_VPROCESSOR_BASE(0x0048)	//  Vertical Offset in the Source Image 
#define S5P_VP_SRC_WIDTH		S5P_VPROCESSOR_BASE(0x004C)	//  Width of the Source Image 
#define S5P_VP_SRC_HEIGHT		S5P_VPROCESSOR_BASE(0x0050)	//  Height of the Source Image 
#define S5P_VP_DST_H_POSITION		S5P_VPROCESSOR_BASE(0x0054)	//  Horizontal Offset in the Display 
#define S5P_VP_DST_V_POSITION		S5P_VPROCESSOR_BASE(0x0058)	//  Vertical Offset in the Display 
#define S5P_VP_DST_WIDTH		S5P_VPROCESSOR_BASE(0x005C)	//  Width of the Display 
#define S5P_VP_DST_HEIGHT		S5P_VPROCESSOR_BASE(0x0060)	//  Height of the Display 
#define S5P_VP_H_RATIO			S5P_VPROCESSOR_BASE(0x0064)	//  Horizontal Zoom Ratio of SRC:DST 
#define S5P_VP_V_RATIO			S5P_VPROCESSOR_BASE(0x0068)	//  Vertical Zoom Ratio of SRC:DST 
#define S5P_VP_POLY8_Y0_LL		S5P_VPROCESSOR_BASE(0x006C)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling
#define S5P_VP_POLY8_Y0_LH		S5P_VPROCESSOR_BASE(0x0070)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y0_HL		S5P_VPROCESSOR_BASE(0x0074)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y0_HH		S5P_VPROCESSOR_BASE(0x0078)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y1_LL		S5P_VPROCESSOR_BASE(0x007C)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y1_LH		S5P_VPROCESSOR_BASE(0x0080)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y1_HL		S5P_VPROCESSOR_BASE(0x0084)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y1_HH		S5P_VPROCESSOR_BASE(0x0088)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y2_LL		S5P_VPROCESSOR_BASE(0x008C)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y2_LH		S5P_VPROCESSOR_BASE(0x0090)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling
#define S5P_VP_POLY8_Y2_HL		S5P_VPROCESSOR_BASE(0x0094)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y2_HH		S5P_VPROCESSOR_BASE(0x0098)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y3_LL		S5P_VPROCESSOR_BASE(0x009C)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y3_LH		S5P_VPROCESSOR_BASE(0x00A0)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y3_HL		S5P_VPROCESSOR_BASE(0x00A4)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY8_Y3_HH		S5P_VPROCESSOR_BASE(0x00A8)	//  8-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY4_Y0_LL		S5P_VPROCESSOR_BASE(0x00EC)	//  4-Tap Poly-phase Filter Coefficients for Luminance Horizontal Scaling 
#define S5P_VP_POLY4_Y0_LH		S5P_VPROCESSOR_BASE(0x00F0)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y0_HL		S5P_VPROCESSOR_BASE(0x00F4)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y0_HH		S5P_VPROCESSOR_BASE(0x00F8)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y1_LL		S5P_VPROCESSOR_BASE(0x00FC)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y1_LH		S5P_VPROCESSOR_BASE(0x0100)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y1_HL		S5P_VPROCESSOR_BASE(0x0104)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y1_HH		S5P_VPROCESSOR_BASE(0x0108)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y2_LL		S5P_VPROCESSOR_BASE(0x010C)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y2_LH		S5P_VPROCESSOR_BASE(0x0110)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y2_HL		S5P_VPROCESSOR_BASE(0x0114)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y2_HH		S5P_VPROCESSOR_BASE(0x0118)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y3_LL		S5P_VPROCESSOR_BASE(0x011C)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y3_LH		S5P_VPROCESSOR_BASE(0x0120)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y3_HL		S5P_VPROCESSOR_BASE(0x0124)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_Y3_HH		S5P_VPROCESSOR_BASE(0x0128)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling 
#define S5P_VP_POLY4_C0_LL		S5P_VPROCESSOR_BASE(0x012C)	//  4-Tap Poly-phase Filter Coefficients for Luminance Vertical Scaling
#define S5P_VP_POLY4_C0_LH		S5P_VPROCESSOR_BASE(0x0130)	//  4-Tap Poly-phase Filter Coefficients for Chrominance Horizontal Scaling 
#define S5P_VP_POLY4_C0_HL		S5P_VPROCESSOR_BASE(0x0134)	//  4-Tap Poly-phase Filter Coefficients for Chrominance Horizontal Scaling 
#define S5P_VP_POLY4_C0_HH		S5P_VPROCESSOR_BASE(0x0138)	//  4-Tap Poly-phase Filter Coefficients for Chrominance Horizontal Scaling 
#define S5P_VP_POLY4_C1_LL		S5P_VPROCESSOR_BASE(0x013C)	//  4-Tap Poly-phase Filter Coefficients for Chrominance Horizontal Scaling 
#define S5P_VP_POLY4_C1_LH		S5P_VPROCESSOR_BASE(0x0140)	//  4-Tap Poly-phase Filter Coefficients for Chrominance Horizontal Scaling 
#define S5P_VP_POLY4_C1_HL		S5P_VPROCESSOR_BASE(0x0144)	//  4-Tap Poly-phase Filter Coefficients for Chrominance Horizontal Scaling
#define S5P_VP_POLY4_C1_HH		S5P_VPROCESSOR_BASE(0x0148)	//  4-Tap Poly-phase Filter Coefficients for Chrominance Horizontal Scaling
#define S5P_PP_CSC_Y2Y_COEF		S5P_VPROCESSOR_BASE(0x01D4)	// Y to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CB2Y_COEF		S5P_VPROCESSOR_BASE(0x01D8)	// CB to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CR2Y_COEF		S5P_VPROCESSOR_BASE(0x01DC)	// CR to Y CSC Coefficient Setting 
#define S5P_PP_CSC_Y2CB_COEF		S5P_VPROCESSOR_BASE(0x01E0)	// Y to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CB2CB_COEF		S5P_VPROCESSOR_BASE(0x01E4)	// CB to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CR2CB_COEF		S5P_VPROCESSOR_BASE(0x01E8)	// CR to Y CSC Coefficient Setting 
#define S5P_PP_CSC_Y2CR_COEF		S5P_VPROCESSOR_BASE(0x01EC)	// Y to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CB2CR_COEF		S5P_VPROCESSOR_BASE(0x01F0)	// CB to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CR2CR_COEF		S5P_VPROCESSOR_BASE(0x01F4)	// CR to Y CSC Coefficient Setting 
#define S5P_PP_BYPASS			S5P_VPROCESSOR_BASE(0x0200)	// Disable the Post Image Processor 
#define S5P_PP_SATURATION		S5P_VPROCESSOR_BASE(0x020C)	// Color Saturation Factor 
#define S5P_PP_SHARPNESS		S5P_VPROCESSOR_BASE(0x0210)	// Control for the Edge Enhancement 
#define S5P_PP_LINE_EQ0			S5P_VPROCESSOR_BASE(0x0218)	// Line Equation for Contrast Duration "0" 
#define S5P_PP_LINE_EQ1			S5P_VPROCESSOR_BASE(0x021C)	// Line Equation for Contrast Duration "1" 
#define S5P_PP_LINE_EQ2			S5P_VPROCESSOR_BASE(0x0220)	// Line Equation for Contrast Duration "2" 
#define S5P_PP_LINE_EQ3			S5P_VPROCESSOR_BASE(0x0224)	// Line Equation for Contrast Duration "3" 
#define S5P_PP_LINE_EQ4			S5P_VPROCESSOR_BASE(0x0228)	// Line Equation for Contrast Duration "4" 
#define S5P_PP_LINE_EQ5			S5P_VPROCESSOR_BASE(0x022C)	// Line Equation for Contrast Duration "5" 
#define S5P_PP_LINE_EQ6			S5P_VPROCESSOR_BASE(0x0230)	// Line Equation for Contrast Duration "6" 
#define S5P_PP_LINE_EQ7			S5P_VPROCESSOR_BASE(0x0234)	// Line Equation for Contrast Duration "7" 
#define S5P_PP_BRIGHT_OFFSET		S5P_VPROCESSOR_BASE(0x0238)	// Brightness Offset Control for Y 
#define S5P_PP_CSC_EN			S5P_VPROCESSOR_BASE(0x023C)	// Color Space Conversion Control 
#define S5P_VP_VERSION_INFO		S5P_VPROCESSOR_BASE(0x03FC)	// VP Version Information  

/*
 Shadow Registers
*/
#define S5P_VP_FIELD_ID_S		S5P_VPROCESSOR_BASE(0x016C)	// Field ID of the "Source" Image 
#define S5P_VP_MODE_S			S5P_VPROCESSOR_BASE(0x0170)	// VP Operation Mode 
#define S5P_VP_IMG_SIZE_Y_S		S5P_VPROCESSOR_BASE(0x0174)	// Luminance Date Tiled Size 
#define S5P_VP_IMG_SIZE_C_S		S5P_VPROCESSOR_BASE(0x0178)	// Chrominance Date Tiled Size 
#define S5P_VP_TOP_Y_PTR_S		S5P_VPROCESSOR_BASE(0x0190)	// Base Address for Y of Top Field 
#define S5P_VP_BOT_Y_PTR_S		S5P_VPROCESSOR_BASE(0x0194)	// Base Address for Y of Bottom Field 
#define S5P_VP_TOP_C_PTR_S		S5P_VPROCESSOR_BASE(0x0198)	// Base Address for C of Top Frame 
#define S5P_VP_BOT_C_PTR_S		S5P_VPROCESSOR_BASE(0x019C)	// Base Address for C of Bottom field 
#define S5P_VP_ENDIAN_MODE_S		S5P_VPROCESSOR_BASE(0x03EC)	// Big/ Little Endian Mode Selection 
#define S5P_VP_SRC_H_POSITION_S		S5P_VPROCESSOR_BASE(0x01AC)	// Horizontal Offset in the Source Image 
#define S5P_VP_SRC_V_POSITION_S		S5P_VPROCESSOR_BASE(0x01B0)	// Vertical Offset in the Source Image 
#define S5P_VP_SRC_WIDTH_S		S5P_VPROCESSOR_BASE(0x01B4)	// Width of the Source Image 
#define S5P_VP_SRC_HEIGHT_S		S5P_VPROCESSOR_BASE(0x01B8)	// Height of the Source Image 
#define S5P_VP_DST_H_POSITION_S		S5P_VPROCESSOR_BASE(0x01BC)	// Horizontal Offset in the Display 
#define S5P_VP_DST_V_POSITION_S		S5P_VPROCESSOR_BASE(0x01C0)	// Vertical Offset in the Display 
#define S5P_VP_DST_WIDTH_S		S5P_VPROCESSOR_BASE(0x01C4)	// Width of the Display 
#define S5P_VP_DST_HEIGHT_S		S5P_VPROCESSOR_BASE(0x01C8)	// Height of the Display 
#define S5P_VP_H_RATIO_S		S5P_VPROCESSOR_BASE(0x01CC)	// Horizontal Zoom Ratio of SRC:DST 
#define S5P_VP_V_RATIO_S		S5P_VPROCESSOR_BASE(0x01D0)	// Vertical Zoom Ratio of SRC:DST 
#define S5P_PP_BYPASS_S			S5P_VPROCESSOR_BASE(0x0258)	// Disable the Post Image Processor 
#define S5P_PP_SATURATION_S		S5P_VPROCESSOR_BASE(0x025C)	// Color Saturation Factor 
#define S5P_PP_SHARPNESS_S		S5P_VPROCESSOR_BASE(0x0260)	// Control for the Edge Enhancement 
#define S5P_PP_LINE_EQ0_S		S5P_VPROCESSOR_BASE(0x0268)	// Line Equation for Contrast Duration "0" 
#define S5P_PP_LINE_EQ1_S		S5P_VPROCESSOR_BASE(0x026C)	// Line Equation for Contrast Duration "1" 
#define S5P_PP_LINE_EQ2_S		S5P_VPROCESSOR_BASE(0x0270)	// Line Equation for Contrast Duration "2" 
#define S5P_PP_LINE_EQ3_S		S5P_VPROCESSOR_BASE(0x0274)	// Line Equation for Contrast Duration "3" 
#define S5P_PP_LINE_EQ4_S		S5P_VPROCESSOR_BASE(0x0278)	// Line Equation for Contrast Duration "4" 
#define S5P_PP_LINE_EQ5_S		S5P_VPROCESSOR_BASE(0x027C)	// Line Equation for Contrast Duration "5" 
#define S5P_PP_LINE_EQ6_S		S5P_VPROCESSOR_BASE(0x0280)	// Line Equation for Contrast Duration "6" 
#define S5P_PP_LINE_EQ7_S		S5P_VPROCESSOR_BASE(0x0284)	// Line Equation for Contrast Duration "7" 
#define S5P_PP_BRIGHT_OFFSET_S		S5P_VPROCESSOR_BASE(0x0288)	// Brightness Offset Control for Y 
#define S5P_PP_CSC_EN_S			S5P_VPROCESSOR_BASE(0x028C)	// Color Space Conversion Control 
#define S5P_PP_CSC_Y2Y_COEF_S		S5P_VPROCESSOR_BASE(0x0290)	// Y to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CB2Y_COEF_S		S5P_VPROCESSOR_BASE(0x0294)	// CB to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CR2Y_COEF_S		S5P_VPROCESSOR_BASE(0x0298)	// CR to Y CSC Coefficient Setting 
#define S5P_PP_CSC_Y2CB_COEF_S		S5P_VPROCESSOR_BASE(0x029C)	// Y to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CB2CB_COEF_S		S5P_VPROCESSOR_BASE(0x02A0)	// CB to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CR2CB_COEF_S		S5P_VPROCESSOR_BASE(0x02A4)	// CR to Y CSC Coefficient Setting 
#define S5P_PP_CSC_Y2CR_COEF_S		S5P_VPROCESSOR_BASE(0x02A8)	// Y to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CB2CR_COEF_S		S5P_VPROCESSOR_BASE(0x02AC)	// CB to Y CSC Coefficient Setting 
#define S5P_PP_CSC_CR2CR_COEF_S		S5P_VPROCESSOR_BASE(0x02B0)	// CR to Y CSC Coefficient Setting  

/*
 Registers Bit Description
*/
/* S5P_VP_ENABLE */
#define S5P_VP_ENABLE_ON		(1<<0)
#define S5P_VP_ENABLE_ON_S		(1<<2) /* R_ONLY, Shadow bit of the bit [0]*/

/* S5P_VP_SRESET */
#define S5P_VP_SRESET_LAST_COMPLETE	(0<<0)
#define S5P_VP_SRESET_PROCESSING	(1<<0)

/* S5P_VP_SHADOW_UPDATE */
#define S5P_VP_SHADOW_UPDATE_DISABLE	(0<<0) 
#define S5P_VP_SHADOW_UPDATE_ENABLE	(1<<0) 

/* S5P_VP_FIELD_ID */
#define S5P_VP_FIELD_ID_TOP		(0<<0)
#define S5P_VP_FIELD_ID_BOTTOM		(1<<0)

/* S5P_VP_MODE */
#define S5P_VP_MODE_2D_IPC_ENABLE			(1<<1)
#define S5P_VP_MODE_2D_IPC_DISABLE			(0<<1)
#define S5P_VP_MODE_FIELD_ID_MAN_TOGGLING		(0<<2)
#define S5P_VP_MODE_FIELD_ID_AUTO_TOGGLING		(1<<2)
#define S5P_VP_MODE_CROMA_EXPANSION_C_TOP_PTR		(0<<3)
#define S5P_VP_MODE_CROMA_EXPANSION_C_TOPBOTTOM_PTR	(1<<3)
#define S5P_VP_MODE_MEM_MODE_LINEAR			(0<<4)
#define S5P_VP_MODE_MEM_MODE_2D_TILE			(1<<4)	
#define S5P_VP_MODE_LINE_SKIP_OFF			(0<<5)
#define S5P_VP_MODE_LINE_SKIP_ON			(1<<5)

/* S5P_VP_ENDIAN_MODE */		
#define S5P_VP_ENDIAN_MODE_BIG		(0<<0)	
#define S5P_VP_ENDIAN_MODE_LITTLE	(1<<0)

/* Macros */
/* S5P_VP_ENABLE */
#define VP_ON_SW_RESET		(1<<2)
#define VP_POWER_DOWN_RDY	(1<<1)
#define VP_ON_ENABLE		(1<<0)
#define VP_ON_DISABLE		(0<<0)

/* S5P_VP_SRESET */
#define VP_SOFT_RESET		(1<<0)

/* S5P_VP_SHADOW_UPDATA */
#define VP_SHADOW_UPDATE_ENABLE     (1<<0)
#define VP_SHADOW_UPDATE_DISABLE    (0<<0)

/* S5P_VP_FIELD_ID */
#define VP_FIELD_ID_BOTTOM	(1<<0)
#define VP_FIELD_ID_TOP		(0<<0)

/* S5P_VP_MODE */
#define VP_LINE_SKIP_ON			(1<<5)
#define VP_LINE_SKIP_OFF		(0<<5)
#define VP_MEM_2D_MODE			(1<<4)
#define VP_MEM_LINEAR_MODE		(0<<4)
#define VP_CHROMA_USE_TOP_BOTTOM	(1<<3)
#define VP_CHROMA_USE_TOP		(0<<3)
#define VP_FIELD_ID_TOGGLE_VSYNC	(1<<2)
#define VP_FIELD_ID_TOGGLE_USER		(0<<2)
#define VP_2D_IPC_ON			(1<<1)
#define VP_2D_IPC_OFF			(0<<1)

/* S5P_VP_TILE_SIZE_x */
#define VP_IMG_HSIZE(a)		((0x3fff&a)<<16)
#define VP_IMG_VSIZE(a)		((0x3fff&a)<<0)
#define VP_IMG_SIZE_ILLEGAL(a)  (0x7&a)

/* S5P_VP_PER_RATE_CTRL */
#define VP_PEL_RATE_CTRL(a)	((0x3&a)<<0)
/*
enum VP_PIXEL_RATE
{
    VP_PIXEL_PER_RATE_1_1  = 0,
    VP_PIXEL_PER_RATE_1_2  = 1,
    VP_PIXEL_PER_RATE_1_3  = 2,
    VP_PIXEL_PER_RATE_1_4  = 3
}
*/

/* S5P_VP_TOP_x_PTR , VP_BOT_x_PTR */
#define VP_PTR_ILLEGAL(a)	(0x7&a)

/* S5P_ VP_ENDIAN_MODE */
#define VP_LITTLE_ENDIAN_MODE   (1<<0)
#define VP_BIG_ENDIAN_MODE      (0<<0)

/* S5P_VP_SRC_H_POSITION */
#define VP_SRC_H_POSITION(a)    ((0x7ff&a)<<4)
#define VP_SRC_X_FRACT_STEP(a)  (0xf&a)

/* S5P_VP_SRC_V_POSITION */
#define VP_SRC_V_POSITION(a)    (0x7ff&a)

/* S5P_VP_SRC_WIDTH */
#define VP_SRC_WIDTH(a)		(0x7ff&a)

/* S5P_VP_SRC_WIDTH */	
#define VP_SRC_HEIGHT(a)	(0x7ff&a)

/* S5P_VP_DST_H_POSITION */
#define VP_DST_H_POSITION(a)    (0x7ff&a)

/* S5P_VP_DST_V_POSITION */
#define VP_DST_V_POSITION(a)    (0x7ff&a)

/* S5P_VP_DST_WIDTH */
#define VP_DST_WIDTH(a)		(0x7ff&a)

/* S5P_VP_DST_WIDTH
#define VP_DST_HEIGHT(a)	(0x3ff&a)
C110:*/
#define VP_DST_HEIGHT(a)	(0x7ff&a)

/* S5P_VP_H_RATIO */
#define VP_H_RATIO(a)		(0x7ffff&a)

/* S5P_VP_V_RATIO */
#define VP_V_RATIO(a)		(0x7ffff&a)

/* S5P_VP_POLY8_Y0_xx */
#define VP_POLY8_Y0_x0(a)   ((0x7&a)<<24)
#define VP_POLY8_Y0_x1(a)   ((0x7&a)<<16)
#define VP_POLY8_Y0_x2(a)   ((0x7&a)<<8)
#define VP_POLY8_Y0_x3(a)   ((0x7&a)<<0)

/* S5P_VP_POLY8_Y1_xx */
#define VP_POLY8_Y1_x0(a)   ((0x1f&a)<<24)
#define VP_POLY8_Y1_x1(a)   ((0x1f&a)<<16)
#define VP_POLY8_Y1_x2(a)   ((0x1f&a)<<8)
#define VP_POLY8_Y1_x3(a)   ((0x1f&a)<<0)

/* VP_POLY8_Y2_xx */
#define VP_POLY8_Y2_x0(a)   ((0x7f&a)<<24)
#define VP_POLY8_Y2_x1(a)   ((0x7f&a)<<16)
#define VP_POLY8_Y2_x2(a)   ((0x7f&a)<<8)
#define VP_POLY8_Y2_x3(a)   ((0x7f&a)<<0)

/* VP_POLY8_Y3_xx */
#define VP_POLY8_Y3_x0(a)   ((0x7f&a)<<24)
#define VP_POLY8_Y3_x1(a)   ((0x7f&a)<<16)
#define VP_POLY8_Y3_x2(a)   ((0x7f&a)<<8)
#define VP_POLY8_Y3_x3(a)   ((0x7f&a)<<0)

/* VP_POLY4_Y0_xx */
#define VP_POLY4_Y0_x0(a)   ((0x3f&a)<<24)
#define VP_POLY4_Y0_x1(a)   ((0x3f&a)<<16)
#define VP_POLY4_Y0_x2(a)   ((0x3f&a)<<8)
#define VP_POLY4_Y0_x3(a)   ((0x3f&a)<<0)

/* VP_POLY4_Y1_xx */
#define VP_POLY4_Y1_x0(a)   ((0x7f&a)<<24)
#define VP_POLY4_Y1_x1(a)   ((0x7f&a)<<16)
#define VP_POLY4_Y1_x2(a)   ((0x7f&a)<<8)
#define VP_POLY4_Y1_x3(a)   ((0x7f&a)<<0)

/* VP_POLY4_Y2_xx */
#define VP_POLY4_Y2_x0(a)   ((0x7f&a)<<24)
#define VP_POLY4_Y2_x1(a)   ((0x7f&a)<<16)
#define VP_POLY4_Y2_x2(a)   ((0x7f&a)<<8)
#define VP_POLY4_Y2_x3(a)   ((0x7f&a)<<0)

/* VP_POLY4_Y3_xx */
#define VP_POLY4_Y3_x0(a)   ((0x3f&a)<<24)
#define VP_POLY4_Y3_x1(a)   ((0x3f&a)<<16)
#define VP_POLY4_Y3_x2(a)   ((0x3f&a)<<8)
#define VP_POLY4_Y3_x3(a)   ((0x3f&a)<<0)

/* VP_POLY4_C0_LL */
#define VP_POLY4_C0_PH0(a)   ((0x7f&a)<<24)
#define VP_POLY4_C0_PH1(a)   ((0x7f&a)<<16)
#define VP_POLY4_C0_PH2(a)   ((0x7f&a)<<8)
#define VP_POLY4_C0_PH3(a)   ((0x7f&a)<<0)

/* VP_POLY4_C0_LH */
#define VP_POLY4_C0_PH4(a)   ((0x3f&a)<<24)
#define VP_POLY4_C0_PH5(a)   ((0x3f&a)<<16)
#define VP_POLY4_C0_PH6(a)   ((0x3f&a)<<8)
#define VP_POLY4_C0_PH7(a)   ((0x3f&a)<<0)

/* VP_POLY4_C0_HL */
#define VP_POLY4_C0_PH8(a)   ((0x3f&a)<<24)
#define VP_POLY4_C0_PH9(a)   ((0x3f&a)<<16)
#define VP_POLY4_C0_PH10(a)  ((0x3f&a)<<8)
#define VP_POLY4_C0_PH11(a)  ((0x3f&a)<<0)

/* VP_POLY4_C0_HH */
#define VP_POLY4_C0_PH12(a)  ((0x1f&a)<<24)
#define VP_POLY4_C0_PH13(a)  ((0x1f&a)<<16)
#define VP_POLY4_C0_PH14(a)  ((0x1f&a)<<8)
#define VP_POLY4_C0_PH15(a)  ((0x1f&a)<<0)

/* VP_POLY4_C1_LL */
#define VP_POLY4_C1_PH0(a)   ((0xff&a)<<24)
#define VP_POLY4_C1_PH1(a)   ((0xff&a)<<16)
#define VP_POLY4_C1_PH2(a)   ((0xff&a)<<8)
#define VP_POLY4_C1_PH3(a)   ((0xff&a)<<0)

/* VP_POLY4_C1_LH */
#define VP_POLY4_C1_PH4(a)   ((0xff&a)<<24)
#define VP_POLY4_C1_PH5(a)   ((0xff&a)<<16)
#define VP_POLY4_C1_PH6(a)   ((0xff&a)<<8)
#define VP_POLY4_C1_PH7(a)   ((0xff&a)<<0)

/* VP_POLY4_C1_HL */
#define VP_POLY4_C1_PH8(a)   ((0xff&a)<<24)
#define VP_POLY4_C1_PH9(a)   ((0xff&a)<<16)
#define VP_POLY4_C1_PH10(a)  ((0xff&a)<<8)
#define VP_POLY4_C1_PH11(a)  ((0xff&a)<<0)

/* VP_POLY4_C1_HH */
#define VP_POLY4_C1_PH12(a)  ((0x7f&a)<<24)
#define VP_POLY4_C1_PH13(a)  ((0x7f&a)<<16)
#define VP_POLY4_C1_PH14(a)  ((0x7f&a)<<8)
#define VP_POLY4_C1_PH15(a)  ((0x7f&a)<<0)

/* PP_CSC_COEF */
#define VP_CSC_COEF(a)		(0xfff&a)

/* PP_BYPASS */
#define VP_BY_PASS_ENABLE	(0)
#define VP_BY_PASS_DISABLE	(1)

/* PP_SATURATION */
#define VP_SATURATION(a)	(0xff&a)

/* PP_SHARPNESS */
#define VP_TH_HNOISE(a)		((0xf&a)<<8)
#define VP_SHARPNESS(a)		(0x3&a)
/*
enum VP_SHARPNESS_CONTROL
{
    VP_SHARPNESS_NO     = 0,
    VP_SHARPNESS_MIN    = 1,
    VP_SHARPNESS_MOD    = 2,
    VP_SHARPNESS_MAX    = 3
}
*/

/* PP_LINE_EQx */
#define VP_LINE_INTC(a)     ((0xffff&a)<<8)
#define VP_LINE_SLOPE(a)    (0xff&a)
#define VP_LINE_INTC_CLEAR(a)     (~(0xffff<<8)&a)
#define VP_LINE_SLOPE_CLEAR(a)    (~0xff&a)

/* PP_BRIGHT_OFFSET */	
#define VP_BRIGHT_OFFSET(a) (0x1ff&a)

/* PP_CSC_EN */
#define VP_SUB_Y_OFFSET_ENABLE  (1<<1)
#define VP_SUB_Y_OFFSET_DISABLE (0<<1)
#define VP_CSC_ENABLE   (1)
#define VP_CSC_DISABLE  (0)

/* global variables */
static unsigned int g_vp_contrast_brightness = 0;

/*#define VP_UPDATE_RETRY_MAXIMUM 30
#define VP_WAIT_UPDATE_SLEEP 3 */

/* Ref. to VP manual p37-39
 [11] : sign bit, [10] : integer bit, [9:0] : fraction bit
 CSC from BT.601(SD) to BT.709(HD) */
#define Y2Y_COEF_601_TO_709    0x400 // 1.0
#define CB2Y_COEF_601_TO_709   0x879 // about -0.118188  ex) 0.118188*1024 = 121.024512 --> about 121 convert to hex(0x79)
#define CR2Y_COEF_601_TO_709   0x8d9 // about -0.212685

#define Y2CB_COEF_601_TO_709   0x0   // 0
#define CB2CB_COEF_601_TO_709  0x413 // about 1.018640
#define CR2CB_COEF_601_TO_709  0x875 // about -0.114618

#define Y2CR_COEF_601_TO_709   0x0
#define CB2CR_COEF_601_TO_709  0x04d // about 0.075049
#define CR2CR_COEF_601_TO_709  0x41a // about 1.025327

/* CSC from BT.709(HD) to BT.601(SD) */
#define Y2Y_COEF_709_TO_601    0x400
#define CB2Y_COEF_709_TO_601   0x068 // about 0.101579
#define CR2Y_COEF_709_TO_601   0x0c9 // about 0.196076

#define Y2CB_COEF_709_TO_601   0x0
#define CB2CB_COEF_709_TO_601  0x3f6 // about 0.989854
#define CR2CB_COEF_709_TO_601  0x871 // about -0.110653

#define Y2CR_COEF_709_TO_601   0x0
#define CB2CR_COEF_709_TO_601  0x84a // about -0.072453
#define CR2CR_COEF_709_TO_601  0xbef // about -0.983398

#define TILE_WIDTH		0x40
#define MAX_NUM_OF_FRM		34 // according to MFC

#endif /* __ASM_ARCH_REGS_VPROCESSOR_H */

