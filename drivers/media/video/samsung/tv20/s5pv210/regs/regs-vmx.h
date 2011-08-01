/* linux/drivers/media/video/samsung/tv20/s5pc100/regs/regs-vmx.h
 *
 * Mixer register header file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <mach/map.h>

#define S5P_MIXER_BASE(x) (x)
/*
 Registers
*/
#define S5P_MXR_STATUS		S5P_MIXER_BASE(0x0000)	//Status of MIXER Operation 
#define S5P_MXR_CFG		S5P_MIXER_BASE(0x0004)	//MIXER Mode Setting  
#define S5P_MXR_INT_EN		S5P_MIXER_BASE(0x0008)	//Interrupt Enable 
#define S5P_MXR_INT_STATUS	S5P_MIXER_BASE(0x000C)	//Interrupt Status 
#define S5P_MXR_LAYER_CFG	S5P_MIXER_BASE(0x0010)	//Video & Graphic Layer Priority and On// Off 
#define S5P_MXR_VIDEO_CFG	S5P_MIXER_BASE(0x0014)	//Video Layer Configuration 
#define S5P_MXR_GRAPHIC0_CFG	S5P_MIXER_BASE(0x0020)	//Graphic Layer0 Configuration 
#define S5P_MXR_GRAPHIC0_BASE	S5P_MIXER_BASE(0x0024)	//Base Address for Graphic Layer0 
#define S5P_MXR_GRAPHIC0_SPAN	S5P_MIXER_BASE(0x0028)	//Span for Graphic Layer0 
#define S5P_MXR_GRAPHIC0_SXY	S5P_MIXER_BASE(0x002C)	//Source X//Y Positions for Graphic Layer0 
#define S5P_MXR_GRAPHIC0_WH	S5P_MIXER_BASE(0x0030)	//Width// Height for Graphic Layer0 
#define S5P_MXR_GRAPHIC0_DXY	S5P_MIXER_BASE(0x0034)	//Destination X//Y Positions for Graphic Layer0 
#define S5P_MXR_GRAPHIC0_BLANK	S5P_MIXER_BASE(0x0038)	//Blank Pixel Value for Graphic Layer0 
#define S5P_MXR_GRAPHIC1_CFG	S5P_MIXER_BASE(0x0040)	//Graphic Layer1 Configuration 
#define S5P_MXR_GRAPHIC1_BASE	S5P_MIXER_BASE(0x0044)	//Base Address for Graphic Layer1 
#define S5P_MXR_GRAPHIC1_SPAN	S5P_MIXER_BASE(0x0048)	//Span for Graphic Layer1 
#define S5P_MXR_GRAPHIC1_SXY	S5P_MIXER_BASE(0x004C)	//Source X//Y Positions for Graphic Layer1 
#define S5P_MXR_GRAPHIC1_WH	S5P_MIXER_BASE(0x0050)	//Width// Height for Graphic Layer1 
#define S5P_MXR_GRAPHIC1_DXY	S5P_MIXER_BASE(0x0054)	//Destination X//Y Positions for Graphic Layer1 
#define S5P_MXR_GRAPHIC1_BLANK	S5P_MIXER_BASE(0x0058)	//Blank Pixel Value for Graphic Layer1 
#define S5P_MXR_BG_CFG		S5P_MIXER_BASE(0x0060)
#define S5P_MXR_BG_COLOR0	S5P_MIXER_BASE(0x0064)	//Background Color of First Point 
#define S5P_MXR_BG_COLOR1	S5P_MIXER_BASE(0x0068)	//Background Color of Second Point 
#define S5P_MXR_BG_COLOR2	S5P_MIXER_BASE(0x006C)	//Background Color of Last Point 
#define S5P_MXR_CM_COEFF_Y	S5P_MIXER_BASE(0x0080)	//Scaled Color Space Conversion (RGB to Y) Coefficient for Graphic Layer 
#define S5P_MXR_CM_COEFF_CB	S5P_MIXER_BASE(0x0084)	//Scaled Color Space Conversion (RGB to CB) Coefficient for Graphic Layer 
#define S5P_MXR_CM_COEFF_CR	S5P_MIXER_BASE(0x0088)	//Scaled Color Space Conversion (RGB to Cr) Coefficient for Graphic Layer 
#define S5P_MXR_VER		S5P_MIXER_BASE(0x0100)	//Mixer Version 

/*
 Shadow Registers
*/
#define S5P_MXR_STATUS_S		S5P_MIXER_BASE(0x2000)	//Status of MIXER Operation (Shadow) 
#define S5P_MXR_CFG_S			S5P_MIXER_BASE(0x2004)	//MIXER Mode Setting (Shadow) 
#define S5P_MXR_LAYER_CFG_S		S5P_MIXER_BASE(0x2010)	//Video & Graphic Layer Priority and On// Off (Shadow) 
#define S5P_MXR_VIDEO_CFG_S		S5P_MIXER_BASE(0x2014)	//Video Layer Configuration (Shadow) 
#define S5P_MXR_GRAPHIC0_CFG_S		S5P_MIXER_BASE(0x2020)	//Graphic Layer0 Configuration (Shadow) 
#define S5P_MXR_GRAPHIC0_BASE_S		S5P_MIXER_BASE(0x2024)	//Graphic0 Base Address (Shadow) 
#define S5P_MXR_GRAPHIC0_SPAN_S		S5P_MIXER_BASE(0x2028)	//Graphic0 Span (Shadow) 
#define S5P_MXR_GRAPHIC0_SXY_S		S5P_MIXER_BASE(0x202C)	//Graphic0 Source X//Y Coordinates (Shadow) 
#define S5P_MXR_GRAPHIC0_WH_S		S5P_MIXER_BASE(0x2030)	//Graphic0 Width// Height (Shadow) 
#define S5P_MXR_GRAPHIC0_DXY_S		S5P_MIXER_BASE(0x2034)	//Graphic0 Destination X//Y Coordinates (Shadow) 
#define S5P_MXR_GRAPHIC0_BLANK_PIXEL_S	S5P_MIXER_BASE(0x2038)	//Graphic0 Blank Pixel (Shadow) 
#define S5P_MXR_GRAPHIC1_CFG_S		S5P_MIXER_BASE(0x2040)	//Graphic Layer1 Configuration (Shadow) 
#define S5P_MXR_GRAPHIC1_BASE_S		S5P_MIXER_BASE(0x2044)	//Graphic1 Base Address (Shadow) 
#define S5P_MXR_GRAPHIC1_SPAN_S		S5P_MIXER_BASE(0x2048)	//Graphic1 Span (Shadow) 
#define S5P_MXR_GRAPHIC1_SXY_S		S5P_MIXER_BASE(0x204C)	//Graphic1 Source X//Y Coordinates (Shadow) 
#define S5P_MXR_GRAPHIC1_WH_S		S5P_MIXER_BASE(0x2050)	//Graphic1 Width// Height (Shadow) 
#define S5P_MXR_GRAPHIC1_DXY_S		S5P_MIXER_BASE(0x2054)	//Graphic1 Destination X//YCoordinates (Shadow) 
#define S5P_MXR_GRAPHIC1_BLANK_PIXEL_S	S5P_MIXER_BASE(0x2058)	//Graphic1 Blank Pixel (Shadow) 
#define S5P_MXR_BG_COLOR0_S		S5P_MIXER_BASE(0x2064)	//Background First Color (Shadow) 
#define S5P_MXR_BG_COLOR1_S		S5P_MIXER_BASE(0x2068)	//Background Second Color (Shadow) 
#define S5P_MXR_BG_COLOR2_S		S5P_MIXER_BASE(0x206C)	//Background Last Color (Shadow) 

/*
 Registers Bit Description
*/
/* S5P_MXR_STATUS */
#define S5P_MXR_STATUS_RUN		(1<<0)
#define S5P_MXR_STATUS_STOP		(0<<0)
#define S5P_MXR_STATUS_SYNC_DISABLE	(0<<2)
#define S5P_MXR_STATUS_SYNC_ENABLE	(1<<2)
#define S5P_MXR_STATUS_LITTLE		(0<<3)
#define S5P_MXR_STATUS_BIT		(1<<3)
#define S5P_MXR_STATUS_8_BURST		(0<<7)
#define S5P_MXR_STATUS_16_BURST		(1<<7)

/* S5P_MXR_CFG */
#define S5P_MXR_CFG_SD			(0<<0)
#define S5P_MXR_CFG_HD			(1<<0)
#define S5P_MXR_CFG_NTSC		(0<<1)
#define S5P_MXR_CFG_PAL			(1<<1)
#define S5P_MXR_CFG_INTERLACE		(0<<2)
#define S5P_MXR_CFG_PROGRASSIVE		(1<<2)
#define S5P_MXR_CFG_VIDEO_DISABLE	(0<<3)
#define S5P_MXR_CFG_VIDEO_ENABLE	(1<<4)
#define S5P_MXR_CFG_GRAPHIC0_DISABLE	(0<<4)
#define S5P_MXR_CFG_GRAPHIC0_ENABLE	(1<<4)
#define S5P_MXR_CFG_GRAPHIC1_DISABLE	(0<<5)
#define S5P_MXR_CFG_GRAPHIC1_ENABLE	(1<<5)
#define S5P_MXR_CFG_HD_720P		(0<<6)
#define S5P_MXR_CFG_HD_1080I		(1<<6)
#define S5P_MXR_CFG_TV_OUT		(0<<7)
#define S5P_MXR_CFG_HDMI_OUT		(1<<7)

/* S5P_MXR_INT_EN */
#define S5P_MXR_INT_EN_GRP0_DISABLE 	(0<<8)
#define S5P_MXR_INT_EN_GRP0_ENABLE	(1<<8)
#define S5P_MXR_INT_EN_GRP1_DISABLE 	(0<<9)
#define S5P_MXR_INT_EN_GRP1_ENABLE	(1<<9)
#define S5P_MXR_INT_EN_VP_DISABLE 	(0<<10)
#define S5P_MXR_INT_EN_VP_ENABLE	(1<<10)

/* S5P_MXR_INT_STATUS */
#define S5P_MXR_STATUS_EN_GRP0_N_FIRED 	(0<<8)
#define S5P_MXR_STATUS_EN_GRP0_FIRED 	(1<<8)
#define S5P_MXR_STATUS_EN_GRP1_N_FIRED 	(0<<9)
#define S5P_MXR_STATUS_EN_GRP1_FIRED	(1<<9)
#define S5P_MXR_STATUS_EN_VP_N_FIRED	(0<<10)
#define S5P_MXR_STATUS_EN_VP_FIRED	(1<<10)

/* S5P_MXR_LAYER_CFG */
#define S5P_MXR_LAYER_CFG_VP_HIDE	(0<<0)
#define S5P_MXR_LAYER_CFG_GRP0_HIDE	(0<<4)
#define S5P_MXR_LAYER_CFG_GRP1_HIDE	(0<<8)

/* S5P_MXR_VIDEO_CFG */
#define S5P_MXR_VIDEO_CFG_BLEND_EN	(1<<16)

/* Macros */
/* MIXER_STATUS */
#define S5P_MXR_BURST16_MODE			(1<<7)
#define S5P_MXR_BURST8_MODE			(0<<7)
#define S5P_MXR_BIG_ENDIAN_SOURCE_FORMAT	(1<<3)
#define S5P_MXR_LITTLE_ENDIAN_SOURCE_FORMAT	(0<<3)
#define S5P_MXR_MIXER_RESERVED			(1<<2)
#define S5P_MXR_CMU_STOP_CLOCK			(1<<1)
#define S5P_MXR_CMU_CANNOT_STOP_CLOCK		(0<<1)
#define S5P_MXR_MIXER_START			(1<<0)
#define S5P_MXR_MIXER_STOP			(0<<0)

/* MIXER_CFG */
#define S5P_MXR_DST_SEL_HDMI  		(1<<7)
#define S5P_MXR_DST_SEL_ANALOG	 	~(1<<7)
#define S5P_MXR_HD_1080I_MODE		(1<<6)
/* C110 */
#define S5P_MXR_HD_1080P_MODE   S5P_MXR_HD_1080I_MODE

#define S5P_MXR_HD_720P_MODE		(0<<6)
#define S5P_MXR_GRAPHIC1_LAYER_SHOW	(1<<5)
#define S5P_MXR_GRAPHIC1_LAYER_HIDE	(0<<5)
#define S5P_MXR_GRAPHIC0_LAYER_SHOW	(1<<4)
#define S5P_MXR_GRAPHIC0_LAYER_HIDE	(0<<4)
#define S5P_MXR_VIDEO_LAYER_SHOW	(1<<3)
#define S5P_MXR_VIDEO_LAYER_HIDE	(0<<3)
#define S5P_MXR_PROGRESSVE_MODE		(1<<2)
#define S5P_MXR_INTERLACE_MODE		~(1<<2)
#define S5P_MXR_PAL			(1<<1)
#define S5P_MXR_NTSC			(0<<1)
#define S5P_MXR_HD			(1<<0)
#define S5P_MXR_SD			(0<<0)

/* MIXER_INT_EN */
#define S5P_MXR_VP_INT_ENABLE		(1<<10)
#define S5P_MXR_VP_INT_DISABLE		(0<<10)
#define S5P_MXR_GRP1_INT_ENABLE		(1<<9)
#define S5P_MXR_GRP1_INT_DISABLE	(0<<9)
#define S5P_MXR_GRP0_INT_ENABLE		(1<<8)
#define S5P_MXR_GRP0_INT_DISABLE	(0<<8)

/* MIXER_INT_STATUS */
#define S5P_MXR_VP_INT_FIRED		(1<<10)
#define S5P_MXR_GRP1_INT_FIRED		(1<<9)
#define S5P_MXR_GRP0_INT_FIRED		(1<<8)
#define S5P_MXR_INT_FIRED		(1<<0)

#define S5P_MXR_ALPHA			(0xff)

/* MIXER_LAYER_CFG */
#define S5P_MXR_GRP1_LAYER_PRIORITY(a)		((0xf&a)<<8)
#define S5P_MXR_GRP0_LAYER_PRIORITY(a)		((0xf&a)<<4)
#define S5P_MXR_VP_LAYER_PRIORITY(a)		((0xf&a)<<0)
#define S5P_MXR_GRP1_LAYER_PRIORITY_CLEAR(a)	((~(0xf<<8))&a)
#define S5P_MXR_GRP0_LAYER_PRIORITY_CLEAR(a)	((~(0xf<<4))&a)
#define S5P_MXR_VP_LAYER_PRIORITY_CLEAR(a)	((~(0xf<<0))&a)
#define S5P_MXR_GRP1_LAYER_PRIORITY_INFO(a)	((0xf<<8)&a)
#define S5P_MXR_GRP0_LAYER_PRIORITY_INFO(a)	((0xf<<4)&a)
#define S5P_MXR_VP_LAYER_PRIORITY_INFO(a)	((0xf<<0)&a)

/* MIXER_VIDEO_CFG */
#define S5P_MXR_VP_BLEND_ENABLE			(1<<16)
#define S5P_MXR_VP_BLEND_DISABLE		(0<<16)
#define S5P_MXR_VP_ALPHA_VALUE(a)		((0xff&a)<<0)
#define S5P_MXR_VP_ALPHA_VALUE_CLEAR(a)		((~(0xff<<0))&a)

/* MIXER_GRAPHx_CFG */
#define S5P_MXR_BLANK_CHANGE_NEW_PIXEL		(1<<21)
#define S5P_MXR_BLANK_NOT_CHANGE_NEW_PIXEL	(0<<21)
#define S5P_MXR_PRE_MUL_MODE			(1<<20)
#define S5P_MXR_NORMAL_MODE			(0<<20)
#define S5P_MXR_WIN_BLEND_ENABLE		(1<<17)
#define S5P_MXR_WIN_BLEND_DISABLE		(0<<17)
#define S5P_MXR_PIXEL_BLEND_ENABLE		(1<<16)
#define S5P_MXR_PIXEL_BLEND_DISABLE		(0<<16)
#define S5P_MXR_EG_COLOR_FORMAT(a)		((0xf&a)<<8)
#define S5P_MXR_EG_COLOR_FORMAT_CLEAR(a)	((~(0xf<<8))&a)
#define S5P_MXR_GRP_ALPHA_VALUE(a)		((0xff&a)<<0)
#define S5P_MXR_GRP_ALPHA_VALUE_CLEAR(a)	((~(0xff<<0))&a)
/*
enum s5p_tv_vmx_color_fmt
{
    S5P_MXR_DIRECT_RGB565  = 4,
    S5P_MXR_DIRECT_RGB1555 = 5,
    S5P_MXR_DIRECT_RGB4444 = 6,
    S5P_MXR_DIRECT_RGB8888 = 7
}
*/

/* MIXER_GRAPHx_BASE */
#define S5P_MXR_GPR_BASE(a)		(0xffffffff&a)
#define S5P_MXR_GRP_ADDR_ILLEGAL(a)	(0x3&a)

/* MIXER_GRAPH1_SPAN */
#define S5P_MXR_GRP_SPAN(a)		(0x7fff&a)

/* MIXER_GRAPH1_WH */
#define S5P_MXR_GRP_WIDTH(a)		((0x7ff&a)<<16)
#define S5P_MXR_GRP_HEIGHT(a)		((0x7ff&a)<<0)

/* MIXER_GRAPH1_SXY */
#define S5P_MXR_GRP_STARTX(a)		((0x7ff&a)<<16)
#define S5P_MXR_GRP_STARTY(a)		((0x7ff&a)<<0)

/* MIXER_GRAPH1_DXY */
#define S5P_MXR_GRP_DESTX(a)		((0x7ff&a)<<16)
#define S5P_MXR_GRP_DESTY(a)		((0x7ff&a)<<0)

/* MIXER_GRAPH1_BLANK */
#define S5P_MXR_GPR_BLANK_COLOR(a) 	(0xffffffff&a)

/* MIXER_BG_CFG */
#define S5P_MXR_BG_CR_DIHER_EN   (1<<19)
#define S5P_MXR_BG_CB_DIHER_EN   (1<<18)
#define S5P_MXR_BG_Y_DIHER_EN    (1<<17)

/* MIXER_BG_COLORx */
#define S5P_MXR_BG_COLOR_Y(a)    ((0xff&a)<<16)
#define S5P_MXR_BG_COLOR_CB(a)   ((0xff&a)<<8)
#define S5P_MXR_BG_COLOR_CR(a)   ((0xff&a)<<0)

/* MIXER_CM_COEFF_x */
#define S5P_MXR_BG_COLOR_WIDE    (1<<30)
#define S5P_MXR_BG_COLOR_NARROW  (0<<30)
#define S5P_MXR_BG_COEFF_0(a)    ((0x3f&a)<<20)
#define S5P_MXR_BG_COEFF_1(a)    ((0x3f&a)<<10)
#define S5P_MXR_BG_COEFF_2(a)    ((0x3f&a)<<0)

