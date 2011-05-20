/* linux/drivers/media/video/samsung/tv20/s5pc100/regs/regs-hdmi.h
 *
 * Hdmi register header file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_HDMI_H

#include <mach/map.h>

#define S5P_I2C_HDMI_PHY_BASE(x)	(x)

/*
 * HDMI PHY is configured through the dedicated I2C.
 * The dedicated I2C for HDMI PHY is only used as TX mode
 * I2C-BUS Interface for HDMI PHY is internally connected
 */
#define I2C_HDMI_CON			S5P_I2C_HDMI_PHY_BASE(0x0000)
#define I2C_HDMI_STAT			S5P_I2C_HDMI_PHY_BASE(0x0004)
#define I2C_HDMI_ADD			S5P_I2C_HDMI_PHY_BASE(0x0008)
#define I2C_HDMI_DS			S5P_I2C_HDMI_PHY_BASE(0x000c)
#define I2C_HDMI_LC			S5P_I2C_HDMI_PHY_BASE(0x0010)

#define S5P_HDMI_CTRL_BASE(x)		(x)
#define S5P_HDMI_BASE(x)		(x + 0x00010000)
#define S5P_HDMI_SPDIF_BASE(x)		(x + 0x00030000)
#define S5P_HDMI_I2S_BASE(x)		(x + 0x00040000)
#define S5P_HDMI_TG_BASE(x)		(x + 0x00050000)
#define S5P_HDMI_EFUSE_BASE(x)		(x + 0x00060000)

#define S5P_HDMI_CTRL_INTC_CON		S5P_HDMI_CTRL_BASE(0x0000) /* Interrupt Control Register */
#define S5P_HDMI_CTRL_INTC_FLAG		S5P_HDMI_CTRL_BASE(0x0004) /* Interrupt Flag Register */
#define S5P_HDMI_CTRL_HDCP_KEY_LOAD	S5P_HDMI_CTRL_BASE(0x0008) /* HDCP KEY Status */
#define S5P_HDMI_CTRL_HPD		S5P_HDMI_CTRL_BASE(0x000C) /* HPD signal */
#define S5P_HDMI_CTRL_AUDIO_CLKSEL	S5P_HDMI_CTRL_BASE(0x0010) /* Audio system clock selection */
#define S5P_HDMI_CTRL_PHY_RSTOUT	S5P_HDMI_CTRL_BASE(0x0014) /* HDMI PHY Reset Out */
#define S5P_HDMI_CTRL_PHY_VPLL		S5P_HDMI_CTRL_BASE(0x0018) /* HDMI PHY VPLL Monitor */
#define S5P_HDMI_CTRL_PHY_CMU		S5P_HDMI_CTRL_BASE(0x001C) /* HDMI PHY CMU Monitor */
#define S5P_HDMI_CTRL_CORE_RSTOUT	S5P_HDMI_CTRL_BASE(0x0020) /* HDMI TX Core S/W reset */
			
#define S5P_HDMI_CON_0			S5P_HDMI_BASE(0x0000) /* HDMI System Control Register 0 0x00 */
#define S5P_HDMI_CON_1			S5P_HDMI_BASE(0x0004) /* HDMI System Control Register 1 0x00 */
#define S5P_HDMI_CON_2			S5P_HDMI_BASE(0x0008) /* HDMI System Control Register 2. 0x00 */
#define S5P_STATUS			S5P_HDMI_BASE(0x0010) /* HDMI System Status Register 0x00 */
#define HDMI_PHY_STATUS			S5P_HDMI_BASE(0x0014) /* HDMI Phy Status */
#define S5P_STATUS_EN			S5P_HDMI_BASE(0x0020) /* HDMI System Status Enable Register 0x00 */
#define S5P_HPD				S5P_HDMI_BASE(0x0030) /* Hot Plug Detection Control Register 0x00 */
#define S5P_MODE_SEL			S5P_HDMI_BASE(0x0040) /* HDMI/DVI Mode Selection 0x00 */
#define S5P_ENC_EN			S5P_HDMI_BASE(0x0044) /* HDCP Encryption Enable Register 0x00 */
			
#define S5P_BLUE_SCREEN_0		S5P_HDMI_BASE(0x0050) /* Pixel Values for Blue Screen 0x00 */
#define S5P_BLUE_SCREEN_1		S5P_HDMI_BASE(0x0054) /* Pixel Values for Blue Screen 0x00 */
#define S5P_BLUE_SCREEN_2		S5P_HDMI_BASE(0x0058) /* Pixel Values for Blue Screen 0x00 */
			
#define S5P_HDMI_YMAX			S5P_HDMI_BASE(0x0060) /* Maximum Y (or R,G,B) Pixel Value 0xEB */
#define S5P_HDMI_YMIN			S5P_HDMI_BASE(0x0064) /* Minimum Y (or R,G,B) Pixel Value 0x10 */
#define S5P_HDMI_CMAX			S5P_HDMI_BASE(0x0068) /* Maximum Cb/ Cr Pixel Value 0xF0 */
#define S5P_HDMI_CMIN			S5P_HDMI_BASE(0x006C) /* Minimum Cb/ Cr Pixel Value 0x10 */
			
#define S5P_H_BLANK_0			S5P_HDMI_BASE(0x00A0) /* Horizontal Blanking Setting 0x00 */
#define S5P_H_BLANK_1			S5P_HDMI_BASE(0x00A4) /* Horizontal Blanking Setting 0x00 */
#define S5P_V_BLANK_0			S5P_HDMI_BASE(0x00B0) /* Vertical Blanking Setting 0x00 */
#define S5P_V_BLANK_1			S5P_HDMI_BASE(0x00B4) /* Vertical Blanking Setting 0x00 */
#define S5P_V_BLANK_2			S5P_HDMI_BASE(0x00B8) /* Vertical Blanking Setting 0x00 */
#define S5P_H_V_LINE_0			S5P_HDMI_BASE(0x00C0) /* Hori. Line and Ver. Line 0x00 */
#define S5P_H_V_LINE_1			S5P_HDMI_BASE(0x00C4) /* Hori. Line and Ver. Line 0x00 */
#define S5P_H_V_LINE_2			S5P_HDMI_BASE(0x00C8) /* Hori. Line and Ver. Line 0x00 */
			
#define S5P_SYNC_MODE			S5P_HDMI_BASE(0x00E4) /* Vertical Sync Polarity Control Register 0x00 */
#define S5P_INT_PRO_MODE		S5P_HDMI_BASE(0x00E8) /* Interlace/ Progressive Control Register 0x00 */
			
#define S5P_V_BLANK_F_0			S5P_HDMI_BASE(0x0110) /* Vertical Blanking Setting for Bottom Field 0x00 */
#define S5P_V_BLANK_F_1			S5P_HDMI_BASE(0x0114) /* Vertical Blanking Setting for Bottom Field 0x00 */
#define S5P_V_BLANK_F_2			S5P_HDMI_BASE(0x0118) /* Vertical Blanking Setting for Bottom Field 0x00 */
#define S5P_H_SYNC_GEN_0		S5P_HDMI_BASE(0x0120) /* Horizontal Sync Generation Setting 0x00 */
#define S5P_H_SYNC_GEN_1		S5P_HDMI_BASE(0x0124) /* Horizontal Sync Generation Setting 0x00 */
#define S5P_H_SYNC_GEN_2		S5P_HDMI_BASE(0x0128) /* Horizontal Sync Generation Setting 0x00 */
#define S5P_V_SYNC_GEN_1_0		S5P_HDMI_BASE(0x0130) /* Vertical Sync Generation for Top Field or Frame. 0x01 */
#define S5P_V_SYNC_GEN_1_1		S5P_HDMI_BASE(0x0134) /* Vertical Sync Generation for Top Field or Frame. 0x10 */
#define S5P_V_SYNC_GEN_1_2		S5P_HDMI_BASE(0x0138) /* Vertical Sync Generation for Top Field or Frame. 0x00 */
#define S5P_V_SYNC_GEN_2_0		S5P_HDMI_BASE(0x0140) /* Vertical Sync Generation for Bottom field ? Vertical position. 0x01 */
#define S5P_V_SYNC_GEN_2_1		S5P_HDMI_BASE(0x0144) /* Vertical Sync Generation for Bottom field ? Vertical position. 0x10 */
#define S5P_V_SYNC_GEN_2_2		S5P_HDMI_BASE(0x0148) /* Vertical Sync Generation for Bottom field ? Vertical position. 0x00 */
#define S5P_V_SYNC_GEN_3_0		S5P_HDMI_BASE(0x0150) /* Vertical Sync Generation for Bottom field ? Horizontal position. 0x01 */
#define S5P_V_SYNC_GEN_3_1		S5P_HDMI_BASE(0x0154) /* Vertical Sync Generation for Bottom field ? Horizontal position. 0x10 */
#define S5P_V_SYNC_GEN_3_2		S5P_HDMI_BASE(0x0158) /* Vertical Sync Generation for Bottom field ? Horizontal position. 0x00 */
			
#define S5P_ASP_CON			S5P_HDMI_BASE(0x0160) /* ASP Packet Control Register 0x00 */
#define S5P_ASP_SP_FLAT			S5P_HDMI_BASE(0x0164) /* ASP Packet sp_flat Bit Control 0x00 */
#define S5P_ASP_CHCFG0			S5P_HDMI_BASE(0x0170) /* ASP Audio Channel Configuration 0x04 */
#define S5P_ASP_CHCFG1			S5P_HDMI_BASE(0x0174) /* ASP Audio Channel Configuration 0x1A */
#define S5P_ASP_CHCFG2			S5P_HDMI_BASE(0x0178) /* ASP Audio Channel Configuration 0x2C */
#define S5P_ASP_CHCFG3			S5P_HDMI_BASE(0x017C) /* ASP Audio Channel Configuration 0x3E */
			
#define S5P_ACR_CON			S5P_HDMI_BASE(0x0180) /* ACR Packet Control Register 0x00 */
#define S5P_ACR_MCTS0			S5P_HDMI_BASE(0x0184) /* Measured CTS Value 0x01 */
#define S5P_ACR_MCTS1			S5P_HDMI_BASE(0x0188) /* Measured CTS Value 0x00 */
#define S5P_ACR_MCTS2			S5P_HDMI_BASE(0x018C) /* Measured CTS Value 0x00 */
#define S5P_ACR_CTS0			S5P_HDMI_BASE(0x0190) /* CTS Value for Fixed CTS Transmission Mode. 0xE8 */
#define S5P_ACR_CTS1			S5P_HDMI_BASE(0x0194) /* CTS Value for Fixed CTS Transmission Mode. 0x03 */
#define S5P_ACR_CTS2			S5P_HDMI_BASE(0x0198) /* CTS Value for Fixed CTS Transmission Mode. 0x00 */
#define S5P_ACR_N0			S5P_HDMI_BASE(0x01A0) /* N Value for ACR Packet. 0xE8 */
#define S5P_ACR_N1			S5P_HDMI_BASE(0x01A4) /* N Value for ACR Packet. 0x03 */
#define S5P_ACR_N2			S5P_HDMI_BASE(0x01A8) /* N Value for ACR Packet. 0x00 */
#define S5P_ACR_LSB2			S5P_HDMI_BASE(0x01B0) /* Altenate LSB for Fixed CTS Transmission Mode 0x00 */
#define S5P_ACR_TXCNT			S5P_HDMI_BASE(0x01B4) /* Number of ACR Packet Transmission per frame 0x1F */
#define S5P_ACR_TXINTERVAL		S5P_HDMI_BASE(0x01B8) /* Interval for ACR Packet Transmission 0x63 */
#define S5P_ACR_CTS_OFFSET		S5P_HDMI_BASE(0x01BC) /* CTS Offset for Measured CTS mode. 0x00 */
			
#define S5P_GCP_CON			S5P_HDMI_BASE(0x01C0) /* ACR Packet Control register 0x00 */
#define S5P_GCP_BYTE1			S5P_HDMI_BASE(0x01D0) /* GCP Packet Body 0x00 */
#define S5P_GCP_BYTE2			S5P_HDMI_BASE(0x01D4) /* GCP Packet Body 0x01 */
#define S5P_GCP_BYTE3			S5P_HDMI_BASE(0x01D8) /* GCP Packet Body 0x02 */
			
#define S5P_ACP_CON			S5P_HDMI_BASE(0x01E0) /* ACP Packet Control register 0x00 */
#define S5P_ACP_TYPE			S5P_HDMI_BASE(0x01E4) /* ACP Packet Header 0x00 */
			
#define S5P_ACP_DATA0			S5P_HDMI_BASE(0x0200) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA1			S5P_HDMI_BASE(0x0204) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA2			S5P_HDMI_BASE(0x0208) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA3			S5P_HDMI_BASE(0x020c) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA4			S5P_HDMI_BASE(0x0210) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA5			S5P_HDMI_BASE(0x0214) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA6			S5P_HDMI_BASE(0x0218) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA7			S5P_HDMI_BASE(0x021c) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA8			S5P_HDMI_BASE(0x0220) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA9			S5P_HDMI_BASE(0x0224) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA10			S5P_HDMI_BASE(0x0228) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA11			S5P_HDMI_BASE(0x022c) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA12			S5P_HDMI_BASE(0x0230) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA13			S5P_HDMI_BASE(0x0234) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA14			S5P_HDMI_BASE(0x0238) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA15			S5P_HDMI_BASE(0x023c) /* ACP Packet Body 0x00 */
#define S5P_ACP_DATA16			S5P_HDMI_BASE(0x0240) /* ACP Packet Body 0x00 */
			
#define S5P_ISRC_CON			S5P_HDMI_BASE(0x0250) /* ACR Packet Control Register 0x00 */
#define S5P_ISRC1_HEADER1		S5P_HDMI_BASE(0x0264) /* ISCR1 Packet Header 0x00 */
			
#define S5P_ISRC1_DATA0			S5P_HDMI_BASE(0x0270) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA1			S5P_HDMI_BASE(0x0274) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA2			S5P_HDMI_BASE(0x0278) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA3			S5P_HDMI_BASE(0x027c) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA4			S5P_HDMI_BASE(0x0280) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA5			S5P_HDMI_BASE(0x0284) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA6			S5P_HDMI_BASE(0x0288) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA7			S5P_HDMI_BASE(0x028c) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA8			S5P_HDMI_BASE(0x0290) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA9			S5P_HDMI_BASE(0x0294) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA10		S5P_HDMI_BASE(0x0298) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA11		S5P_HDMI_BASE(0x029c) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA12		S5P_HDMI_BASE(0x02a0) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA13		S5P_HDMI_BASE(0x02a4) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA14		S5P_HDMI_BASE(0x02a8) /* ISRC1 Packet Body 0x00 */
#define S5P_ISRC1_DATA15		S5P_HDMI_BASE(0x02ac) /* ISRC1 Packet Body 0x00 */
			
#define S5P_ISRC2_DATA0			S5P_HDMI_BASE(0x02b0) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA1			S5P_HDMI_BASE(0x02b4) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA2			S5P_HDMI_BASE(0x02b8) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA3			S5P_HDMI_BASE(0x02bc) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA4			S5P_HDMI_BASE(0x02c0) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA5			S5P_HDMI_BASE(0x02c4) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA6			S5P_HDMI_BASE(0x02c8) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA7			S5P_HDMI_BASE(0x02cc) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA8			S5P_HDMI_BASE(0x02d0) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA9			S5P_HDMI_BASE(0x02d4) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA10		S5P_HDMI_BASE(0x02d8) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA11		S5P_HDMI_BASE(0x02dc) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA12		S5P_HDMI_BASE(0x02e0) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA13		S5P_HDMI_BASE(0x02e4) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA14		S5P_HDMI_BASE(0x02e8) /* ISRC2 Packet Body 0x00 */
#define S5P_ISRC2_DATA15		S5P_HDMI_BASE(0x02ec) /* ISRC2 Packet Body 0x00 */
			
#define S5P_AVI_CON			S5P_HDMI_BASE(0x0300) /* AVI Packet Control Register 0x00 */
#define S5P_AVI_CHECK_SUM		S5P_HDMI_BASE(0x0310) /* AVI Packet Checksum 0x00 */
			
#define S5P_AVI_BYTE1			S5P_HDMI_BASE(0x0320) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE2			S5P_HDMI_BASE(0x0324) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE3			S5P_HDMI_BASE(0x0328) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE4			S5P_HDMI_BASE(0x032c) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE5			S5P_HDMI_BASE(0x0330) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE6			S5P_HDMI_BASE(0x0334) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE7			S5P_HDMI_BASE(0x0338) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE8			S5P_HDMI_BASE(0x033c) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE9			S5P_HDMI_BASE(0x0340) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE10			S5P_HDMI_BASE(0x0344) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE11			S5P_HDMI_BASE(0x0348) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE12			S5P_HDMI_BASE(0x034c) /* AVI Packet Body 0x00 */
#define S5P_AVI_BYTE13			S5P_HDMI_BASE(0x0350) /* AVI Packet Body 0x00  */
			
#define S5P_AUI_CON			S5P_HDMI_BASE(0x0360) /* AUI Packet Control Register 0x00 */
#define S5P_AUI_CHECK_SUM		S5P_HDMI_BASE(0x0370) /* AUI Packet Checksum 0x00 */
			
#define S5P_AUI_BYTE1			S5P_HDMI_BASE(0x0380) /* AUI Packet Body 0x00 */
#define S5P_AUI_BYTE2			S5P_HDMI_BASE(0x0384) /* AUI Packet Body 0x00 */
#define S5P_AUI_BYTE3			S5P_HDMI_BASE(0x0388) /* AUI Packet Body 0x00 */
#define S5P_AUI_BYTE4			S5P_HDMI_BASE(0x038c) /* AUI Packet Body 0x00 */
#define S5P_AUI_BYTE5			S5P_HDMI_BASE(0x0390) /* AUI Packet Body 0x00 */
			
#define S5P_MPG_CON			S5P_HDMI_BASE(0x03A0) /* ACR Packet Control Register 0x00 */
#define S5P_MPG_CHECK_SUM		S5P_HDMI_BASE(0x03B0) /* MPG Packet Checksum 0x00 */
			
#define S5P_MPEG_BYTE1			S5P_HDMI_BASE(0x03c0) /* MPEG Packet Body 0x00 */
#define S5P_MPEG_BYTE2			S5P_HDMI_BASE(0x03c4) /* MPEG Packet Body 0x00 */
#define S5P_MPEG_BYTE3			S5P_HDMI_BASE(0x03c8) /* MPEG Packet Body 0x00 */
#define S5P_MPEG_BYTE4			S5P_HDMI_BASE(0x03cc) /* MPEG Packet Body 0x00 */
#define S5P_MPEG_BYTE5			S5P_HDMI_BASE(0x03d0) /* MPEG Packet Body 0x00 */
			
#define S5P_SPD_CON			S5P_HDMI_BASE(0x0400) /* SPD Packet Control Register 0x00 */
#define S5P_SPD_HEADER0			S5P_HDMI_BASE(0x0410) /* SPD Packet Header 0x00 */
#define S5P_SPD_HEADER1			S5P_HDMI_BASE(0x0414) /* SPD Packet Header 0x00 */
#define S5P_SPD_HEADER2			S5P_HDMI_BASE(0x0418) /* SPD Packet Header 0x00 */
			
#define S5P_SPD_DATA0			S5P_HDMI_BASE(0x0420) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA1			S5P_HDMI_BASE(0x0424) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA2			S5P_HDMI_BASE(0x0428) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA3			S5P_HDMI_BASE(0x042c) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA4			S5P_HDMI_BASE(0x0430) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA5			S5P_HDMI_BASE(0x0434) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA6			S5P_HDMI_BASE(0x0438) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA7			S5P_HDMI_BASE(0x043c) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA8			S5P_HDMI_BASE(0x0440) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA9			S5P_HDMI_BASE(0x0444) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA10			S5P_HDMI_BASE(0x0448) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA11			S5P_HDMI_BASE(0x044c) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA12			S5P_HDMI_BASE(0x0450) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA13			S5P_HDMI_BASE(0x0454) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA14			S5P_HDMI_BASE(0x0458) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA15			S5P_HDMI_BASE(0x045c) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA16			S5P_HDMI_BASE(0x0460) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA17			S5P_HDMI_BASE(0x0464) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA18			S5P_HDMI_BASE(0x0468) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA19			S5P_HDMI_BASE(0x046c) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA20			S5P_HDMI_BASE(0x0470) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA21			S5P_HDMI_BASE(0x0474) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA22			S5P_HDMI_BASE(0x0478) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA23			S5P_HDMI_BASE(0x048c) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA24			S5P_HDMI_BASE(0x0480) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA25			S5P_HDMI_BASE(0x0484) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA26			S5P_HDMI_BASE(0x0488) /* SPD Packet Body 0x00 */
#define S5P_SPD_DATA27			S5P_HDMI_BASE(0x048c) /* SPD Packet Body 0x00 */
			
#define S5P_HDCP_RX_SHA1_0_0		S5P_HDMI_BASE(0x0600) /* SHA-1 Value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_0_1		S5P_HDMI_BASE(0x0604) /* SHA-1 Value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_0_2		S5P_HDMI_BASE(0x0608) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_0_3		S5P_HDMI_BASE(0x060C) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_1_0		S5P_HDMI_BASE(0x0610) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_1_1		S5P_HDMI_BASE(0x0614) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_1_2		S5P_HDMI_BASE(0x0618) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_1_3		S5P_HDMI_BASE(0x061C) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_2_0		S5P_HDMI_BASE(0x0620) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_2_1		S5P_HDMI_BASE(0x0624) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_2_2		S5P_HDMI_BASE(0x0628) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_2_3		S5P_HDMI_BASE(0x062C) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_3_0		S5P_HDMI_BASE(0x0630) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_3_1		S5P_HDMI_BASE(0x0634) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_3_2		S5P_HDMI_BASE(0x0638) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_3_3		S5P_HDMI_BASE(0x063C) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_4_0		S5P_HDMI_BASE(0x0640) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_4_1		S5P_HDMI_BASE(0x0644) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_4_2		S5P_HDMI_BASE(0x0648) /* SHA-1 value from Repeater 0x00 */
#define S5P_HDCP_RX_SHA1_4_3		S5P_HDMI_BASE(0x064C) /* SHA-1 value from Repeater 0x00 */
			
#define S5P_HDCP_RX_KSV_0_0		S5P_HDMI_BASE(0x0650) /* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDCP_RX_KSV_0_1		S5P_HDMI_BASE(0x0654) /* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDCP_RX_KSV_0_2		S5P_HDMI_BASE(0x0658) /* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDCP_RX_KSV_0_3		S5P_HDMI_BASE(0x065C) /* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDCP_RX_KSV_0_4		S5P_HDMI_BASE(0x0660) /* Receiver¡¯s KSV 1 0x00 */
			
#define S5P_HDCP_RX_KSV_LIST_CTRL	S5P_HDMI_BASE(0x0664) /* Receiver¡¯s KSV 1 0x00 */
#define S5P_HDCP_AUTH_STATUS		S5P_HDMI_BASE(0x0670) /* 2nd authentication status 0x00 */
#define S5P_HDCP_CTRL1			S5P_HDMI_BASE(0x0680) /* HDCP Control 0x00 */
#define S5P_HDCP_CTRL2			S5P_HDMI_BASE(0x0684) /* HDCP Control 0x00 */
#define S5P_HDCP_CHECK_RESULT		S5P_HDMI_BASE(0x0690) /* HDCP Ri, Pj, V result 0x00 */
			
#define S5P_HDCP_BKSV_0_0		S5P_HDMI_BASE(0x06A0) /* Receiver¡¯s BKSV 0x00 */
#define S5P_HDCP_BKSV_0_1		S5P_HDMI_BASE(0x06A4) /* Receiver¡¯s BKSV 0x00 */
#define S5P_HDCP_BKSV_0_2		S5P_HDMI_BASE(0x06A8) /* Receiver¡¯s BKSV 0x00 */
#define S5P_HDCP_BKSV_0_3		S5P_HDMI_BASE(0x06AC) /* Receiver¡¯s BKSV 0x00 */
#define S5P_HDCP_BKSV_1			S5P_HDMI_BASE(0x06B0) /* Receiver¡¯s BKSV 0x00 */
			
#define S5P_HDCP_AKSV_0_0		S5P_HDMI_BASE(0x06C0) /* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDCP_AKSV_0_1		S5P_HDMI_BASE(0x06C4) /* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDCP_AKSV_0_2		S5P_HDMI_BASE(0x06C8) /* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDCP_AKSV_0_3		S5P_HDMI_BASE(0x06CC) /* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDCP_AKSV_1			S5P_HDMI_BASE(0x06D0) /* Transmitter¡¯s AKSV 0x00 */
			
#define S5P_HDCP_An_0_0			S5P_HDMI_BASE(0x06E0) /* Transmitter¡¯s An 0x00 */
#define S5P_HDCP_An_0_1			S5P_HDMI_BASE(0x06E4) /* Transmitter¡¯s An 0x00 */
#define S5P_HDCP_An_0_2			S5P_HDMI_BASE(0x06E8) /* Transmitter¡¯s An 0x00 */
#define S5P_HDCP_An_0_3			S5P_HDMI_BASE(0x06EC) /* Transmitter¡¯s An 0x00 */
#define S5P_HDCP_An_1_0			S5P_HDMI_BASE(0x06F0) /* Transmitter¡¯s An 0x00 */
#define S5P_HDCP_An_1_1			S5P_HDMI_BASE(0x06F4) /* Transmitter¡¯s An 0x00 */
#define S5P_HDCP_An_1_2			S5P_HDMI_BASE(0x06F8) /* Transmitter¡¯s An 0x00 */
#define S5P_HDCP_An_1_3			S5P_HDMI_BASE(0x06FC) /* Transmitter¡¯s An 0x00 */
			
#define S5P_HDCP_BCAPS			S5P_HDMI_BASE(0x0700) /* Receiver¡¯s BCAPS 0x00 */
#define S5P_HDCP_BSTATUS_0		S5P_HDMI_BASE(0x0710) /* Receiver¡¯s BSTATUS 0x00 */
#define S5P_HDCP_BSTATUS_1		S5P_HDMI_BASE(0x0714) /* Receiver¡¯s BSTATUS 0x00 */
#define S5P_HDCP_Ri_0			S5P_HDMI_BASE(0x0740) /* Transmitter¡¯s Ri 0x00 */
#define S5P_HDCP_Ri_1			S5P_HDMI_BASE(0x0744) /* Transmitter¡¯s Ri 0x00 */
			
#define S5P_HDCP_I2C_INT		S5P_HDMI_BASE(0x0780) /* HDCP I2C interrupt status */
#define S5P_HDCP_AN_INT			S5P_HDMI_BASE(0x0790) /* HDCP An interrupt status */
#define S5P_HDCP_WDT_INT		S5P_HDMI_BASE(0x07a0) /* HDCP Watchdog interrupt status */
#define S5P_HDCP_RI_INT			S5P_HDMI_BASE(0x07b0) /* HDCP RI interrupt status */
			
#define S5P_HDCP_RI_COMPARE_0		S5P_HDMI_BASE(0x07d0) /* HDCP Ri Interrupt Frame number index register 0 */
#define S5P_HDCP_RI_COMPARE_1		S5P_HDMI_BASE(0x07d4) /* HDCP Ri Interrupt Frame number index register 1 */
#define S5P_HDCP_FRAME_COUNT		S5P_HDMI_BASE(0x07e0) /* Current value of the frame count index in the hardware */
			
#define HDMI_GAMUT_CON			S5P_HDMI_BASE(0x0500) /* Gamut Metadata packet transmission control register */
#define HDMI_GAMUT_HEADER0		S5P_HDMI_BASE(0x0504) /* Gamut metadata packet header */
#define HDMI_GAMUT_HEADER1		S5P_HDMI_BASE(0x0508) /* Gamut metadata packet header */
#define HDMI_GAMUT_HEADER2		S5P_HDMI_BASE(0x050c) /* Gamut metadata packet header */
#define HDMI_GAMUT_DATA00		S5P_HDMI_BASE(0x0510) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA01		S5P_HDMI_BASE(0x0514) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA02		S5P_HDMI_BASE(0x0518) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA03		S5P_HDMI_BASE(0x051c) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA04		S5P_HDMI_BASE(0x0520) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA05		S5P_HDMI_BASE(0x0524) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA06		S5P_HDMI_BASE(0x0528) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA07		S5P_HDMI_BASE(0x052c) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA08		S5P_HDMI_BASE(0x0530) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA09		S5P_HDMI_BASE(0x0534) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA10		S5P_HDMI_BASE(0x0538) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA11		S5P_HDMI_BASE(0x053c) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA12		S5P_HDMI_BASE(0x0540) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA13		S5P_HDMI_BASE(0x0544) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA14		S5P_HDMI_BASE(0x0548) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA15		S5P_HDMI_BASE(0x054c) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA16		S5P_HDMI_BASE(0x0550) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA17		S5P_HDMI_BASE(0x0554) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA18		S5P_HDMI_BASE(0x0558) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA19		S5P_HDMI_BASE(0x055c) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA20		S5P_HDMI_BASE(0x0560) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA21		S5P_HDMI_BASE(0x0564) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA22		S5P_HDMI_BASE(0x0568) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA23		S5P_HDMI_BASE(0x056c) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA24		S5P_HDMI_BASE(0x0570) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA25		S5P_HDMI_BASE(0x0574) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA26		S5P_HDMI_BASE(0x0578) /* Gamut Metadata packet body data */
#define HDMI_GAMUT_DATA27		S5P_HDMI_BASE(0x057c) /* Gamut Metadata packet body data */
			
#define	S5P_HDMI_DC_CONTROL		S5P_HDMI_BASE(0x05C0) /* Gamut Metadata packet body data */
#define S5P_HDMI_VIDEO_PATTERN_GEN	S5P_HDMI_BASE(0x05C4) /* Gamut Metadata packet body data */
#define S5P_HDMI_HPD_GEN		S5P_HDMI_BASE(0x05C8) /* Gamut Metadata packet body data */
			
#define S5P_TG_CMD			S5P_HDMI_TG_BASE(0x0000) /* Command Register 0x00 */
#define S5P_TG_H_FSZ_L			S5P_HDMI_TG_BASE(0x0018) /* Horizontal Full Size 0x72 */
#define S5P_TG_H_FSZ_H			S5P_HDMI_TG_BASE(0x001C) /* Horizontal Full Size 0x06 */
#define S5P_TG_HACT_ST_L		S5P_HDMI_TG_BASE(0x0020) /* Horizontal Active Start 0x05 */
#define S5P_TG_HACT_ST_H		S5P_HDMI_TG_BASE(0x0024) /* Horizontal Active Start 0x01 */
#define S5P_TG_HACT_SZ_L		S5P_HDMI_TG_BASE(0x0028) /* Horizontal Active Size 0x00 */
#define S5P_TG_HACT_SZ_H		S5P_HDMI_TG_BASE(0x002C) /* Horizontal Active Size 0x05 */
#define S5P_TG_V_FSZ_L			S5P_HDMI_TG_BASE(0x0030) /* Vertical Full Line Size 0xEE */
#define S5P_TG_V_FSZ_H			S5P_HDMI_TG_BASE(0x0034) /* Vertical Full Line Size 0x02 */
#define S5P_TG_VSYNC_L			S5P_HDMI_TG_BASE(0x0038) /* Vertical Sync Position 0x01 */
#define S5P_TG_VSYNC_H			S5P_HDMI_TG_BASE(0x003C) /* Vertical Sync Position 0x00 */
#define S5P_TG_VSYNC2_L			S5P_HDMI_TG_BASE(0x0040) /* Vertical Sync Position for Bottom Field 0x33 */
#define S5P_TG_VSYNC2_H			S5P_HDMI_TG_BASE(0x0044) /* Vertical Sync Position for Bottom Field 0x02 */
#define S5P_TG_VACT_ST_L		S5P_HDMI_TG_BASE(0x0048) /* Vertical Sync Active Start Position 0x1a */
#define S5P_TG_VACT_ST_H		S5P_HDMI_TG_BASE(0x004C) /* Vertical Sync Active Start Position 0x00 */
#define S5P_TG_VACT_SZ_L		S5P_HDMI_TG_BASE(0x0050) /* Vertical Active Size 0xd0 */
#define S5P_TG_VACT_SZ_H		S5P_HDMI_TG_BASE(0x0054) /* Vertical Active Size 0x02 */
#define S5P_TG_FIELD_CHG_L		S5P_HDMI_TG_BASE(0x0058) /* Field Change Position 0x33 */
#define S5P_TG_FIELD_CHG_H		S5P_HDMI_TG_BASE(0x005C) /* Field Change Position 0x02 */
#define S5P_TG_VACT_ST2_L		S5P_HDMI_TG_BASE(0x0060) /* Vertical Sync Active Start Position for Bottom Field 0x48 */
#define S5P_TG_VACT_ST2_H		S5P_HDMI_TG_BASE(0x0064) /* Vertical Sync Active Start Position for Bottom Field 0x02 */
			
#define S5P_TG_VSYNC_TOP_HDMI_L		S5P_HDMI_TG_BASE(0x0078) /* HDMI Vsync Positon for Top Field 0x01 */
#define S5P_TG_VSYNC_TOP_HDMI_H		S5P_HDMI_TG_BASE(0x007C) /* HDMI Vsync Positon for Top Field 0x00 */
#define S5P_TG_VSYNC_BOT_HDMI_L		S5P_HDMI_TG_BASE(0x0080) /* HDMI Vsync Positon for Bottom Field 0x33 */
#define S5P_TG_VSYNC_BOT_HDMI_H		S5P_HDMI_TG_BASE(0x0084) /* HDMI Vsync Positon for Bottom Field 0x02 */
#define S5P_TG_FIELD_TOP_HDMI_L		S5P_HDMI_TG_BASE(0x0088) /* HDMI Top Field Start Position 0x01 */
#define S5P_TG_FIELD_TOP_HDMI_H		S5P_HDMI_TG_BASE(0x008C) /* HDMI Top Field Start Position 0x00 */
#define S5P_TG_FIELD_BOT_HDMI_L		S5P_HDMI_TG_BASE(0x0090) /* HDMI Bottom Field Start Position 0x33 */
#define S5P_TG_FIELD_BOT_HDMI_H		S5P_HDMI_TG_BASE(0x0094) /* HDMI Bottom Field Start Position 0x02 */
			
#define S5P_EFUSE_CTRL			S5P_HDMI_EFUSE_BASE(0x0000)
#define S5P_EFUSE_STATUS		S5P_HDMI_EFUSE_BASE(0x0004)
#define S5P_EFUSE_ADDR_WIDTH		S5P_HDMI_EFUSE_BASE(0x0008)
#define S5P_EFUSE_SIGDEV_ASSERT		S5P_HDMI_EFUSE_BASE(0x000c)
#define S5P_EFUSE_SIGDEV_DEASSERT	S5P_HDMI_EFUSE_BASE(0x0010)
#define S5P_EFUSE_PRCHG_ASSERT		S5P_HDMI_EFUSE_BASE(0x0014)
#define S5P_EFUSE_PRCHG_DEASSERT	S5P_HDMI_EFUSE_BASE(0x0018)
#define S5P_EFUSE_FSET_ASSERT		S5P_HDMI_EFUSE_BASE(0x001c)
#define S5P_EFUSE_FSET_DEASSERT		S5P_HDMI_EFUSE_BASE(0x0020)
#define S5P_EFUSE_SENSING		S5P_HDMI_EFUSE_BASE(0x0024)
#define S5P_EFUSE_SCK_ASSERT		S5P_HDMI_EFUSE_BASE(0x0028)
#define S5P_EFUSE_SCK_DEASSERT		S5P_HDMI_EFUSE_BASE(0x002c)
#define S5P_EFUSE_SDOUT_OFFSET		S5P_HDMI_EFUSE_BASE(0x0030)
#define S5P_EFUSE_READ_OFFSET		S5P_HDMI_EFUSE_BASE(0x0034)
			
#define S5P_HDMI_I2S_CLK_CON		S5P_HDMI_I2S_BASE(0x0000) /* I2S Clock Enable Register0x00  */
#define S5P_HDMI_I2S_CON_1		S5P_HDMI_I2S_BASE(0x0004) /* I2S Control Register 10x00  */
#define S5P_HDMI_I2S_CON_2		S5P_HDMI_I2S_BASE(0x0008) /* I2S Control Register 20x00  */
#define S5P_HDMI_I2S_PIN_SEL_0		S5P_HDMI_I2S_BASE(0x000C) /* I2S Input Pin Selection Register 0 0x77  */
#define S5P_HDMI_I2S_PIN_SEL_1		S5P_HDMI_I2S_BASE(0x0010) /* I2S Input Pin Selection Register 1 0x77  */
#define S5P_HDMI_I2S_PIN_SEL_2		S5P_HDMI_I2S_BASE(0x0014) /* I2S Input Pin Selection Register 2 0x77  */
#define S5P_HDMI_I2S_PIN_SEL_3		S5P_HDMI_I2S_BASE(0x0018) /* I2S Input Pin Selection Register 30x07  */
#define S5P_HDMI_I2S_DSD_CON		S5P_HDMI_I2S_BASE(0x001C) /* I2S DSD Control Register0x02  */
#define S5P_HDMI_I2S_MUX_CON		S5P_HDMI_I2S_BASE(0x0020) /* I2S In/Mux Control Register 0x60  */
#define S5P_HDMI_I2S_CH_ST_CON		S5P_HDMI_I2S_BASE(0x0024) /* I2S Channel Status Control Register0x00  */
#define S5P_HDMI_I2S_CH_ST_0		S5P_HDMI_I2S_BASE(0x0028) /* I2S Channel Status Block 00x00  */
#define S5P_HDMI_I2S_CH_ST_1		S5P_HDMI_I2S_BASE(0x002C) /* I2S Channel Status Block 10x00  */
#define S5P_HDMI_I2S_CH_ST_2		S5P_HDMI_I2S_BASE(0x0030) /* I2S Channel Status Block 20x00  */
#define S5P_HDMI_I2S_CH_ST_3		S5P_HDMI_I2S_BASE(0x0034) /* I2S Channel Status Block 30x00  */
#define S5P_HDMI_I2S_CH_ST_4		S5P_HDMI_I2S_BASE(0x0038) /* I2S Channel Status Block 40x00  */
#define S5P_HDMI_I2S_CH_ST_SH_0		S5P_HDMI_I2S_BASE(0x003C) /* I2S Channel Status Block Shadow Register 00x00  */
#define S5P_HDMI_I2S_CH_ST_SH_1		S5P_HDMI_I2S_BASE(0x0040) /* I2S Channel Status Block Shadow Register 10x00  */
#define S5P_HDMI_I2S_CH_ST_SH_2		S5P_HDMI_I2S_BASE(0x0044) /* I2S Channel Status Block Shadow Register 20x00  */
#define S5P_HDMI_I2S_CH_ST_SH_3		S5P_HDMI_I2S_BASE(0x0048) /* I2S Channel Status Block Shadow Register 30x00  */
#define S5P_HDMI_I2S_CH_ST_SH_4		S5P_HDMI_I2S_BASE(0x004C) /* I2S Channel Status Block Shadow Register 40x00  */
#define S5P_HDMI_I2S_VD_DATA		S5P_HDMI_I2S_BASE(0x0050) /* I2S Audio Sample Validity Register0x00  */
#define S5P_HDMI_I2S_MUX_CH		S5P_HDMI_I2S_BASE(0x0054) /* I2S Channel Enable Register0x03  */
#define S5P_HDMI_I2S_MUX_CUV		S5P_HDMI_I2S_BASE(0x0058) /* I2S CUV Enable Register0x03  */
#define S5P_HDMI_I2S_IRQ_MASK		S5P_HDMI_I2S_BASE(0x005C) /* I2S Interrupt Request Mask Register0x03  */
#define S5P_HDMI_I2S_IRQ_STATUS		S5P_HDMI_I2S_BASE(0x0060) /* I2S Interrupt Request Status Register0x00  */
#define S5P_HDMI_I2S_CH0_L_0		S5P_HDMI_I2S_BASE(0x0064) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_L_1		S5P_HDMI_I2S_BASE(0x0068) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_L_2		S5P_HDMI_I2S_BASE(0x006C) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_L_3		S5P_HDMI_I2S_BASE(0x0070) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_0		S5P_HDMI_I2S_BASE(0x0074) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_1		S5P_HDMI_I2S_BASE(0x0078) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_2		S5P_HDMI_I2S_BASE(0x007C) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_3		S5P_HDMI_I2S_BASE(0x0080) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_0		S5P_HDMI_I2S_BASE(0x0084) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_1		S5P_HDMI_I2S_BASE(0x0088) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_2		S5P_HDMI_I2S_BASE(0x008C) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_3		S5P_HDMI_I2S_BASE(0x0090) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_0		S5P_HDMI_I2S_BASE(0x0094) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_1		S5P_HDMI_I2S_BASE(0x0098) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_2		S5P_HDMI_I2S_BASE(0x009C) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_3		S5P_HDMI_I2S_BASE(0x00A0) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_0		S5P_HDMI_I2S_BASE(0x00A4) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_1		S5P_HDMI_I2S_BASE(0x00A8) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_2		S5P_HDMI_I2S_BASE(0x00AC) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_3		S5P_HDMI_I2S_BASE(0x00B0) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_R_0		S5P_HDMI_I2S_BASE(0x00B4) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_R_1		S5P_HDMI_I2S_BASE(0x00B8) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_R_2		S5P_HDMI_I2S_BASE(0x00BC) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_Ch2_R_3		S5P_HDMI_I2S_BASE(0x00C0) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_L_0		S5P_HDMI_I2S_BASE(0x00C4) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_L_1		S5P_HDMI_I2S_BASE(0x00C8) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_L_2		S5P_HDMI_I2S_BASE(0x00CC) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_R_0		S5P_HDMI_I2S_BASE(0x00D0) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_R_1		S5P_HDMI_I2S_BASE(0x00D4) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_R_2		S5P_HDMI_I2S_BASE(0x00D8) /* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CUV_L_R		S5P_HDMI_I2S_BASE(0x00DC) /* I2S CUV Output Data Register0x00  */
			
#define S5P_SPDIFIN_CLK_CTRL		S5P_HDMI_SPDIF_BASE(0x0000) /* SPDIFIN_CLK_CTRL [1:0] 0x02 */
#define S5P_SPDIFIN_OP_CTRL		S5P_HDMI_SPDIF_BASE(0x0004) /* SPDIFIN_OP_CTRL [1:0] 0x00 */
#define S5P_SPDIFIN_IRQ_MASK		S5P_HDMI_SPDIF_BASE(0x0008) /* SPDIFIN_IRQ_MASK[7:0] 0x00 */
#define S5P_SPDIFIN_IRQ_STATUS		S5P_HDMI_SPDIF_BASE(0x000C) /* SPDIFIN_IRQ_STATUS [7:0] 0x00 */
#define S5P_SPDIFIN_CONFIG_1		S5P_HDMI_SPDIF_BASE(0x0010) /* SPDIFIN_CONFIG [7:0] 0x00 */
#define S5P_SPDIFIN_CONFIG_2		S5P_HDMI_SPDIF_BASE(0x0014) /* SPDIFIN_CONFIG [11:8] 0x00 */
#define S5P_SPDIFIN_USER_VALUE_1	S5P_HDMI_SPDIF_BASE(0x0020) /* SPDIFIN_USER_VALUE [7:0] 0x00 */
#define S5P_SPDIFIN_USER_VALUE_2	S5P_HDMI_SPDIF_BASE(0x0024) /* SPDIFIN_USER_VALUE [15:8] 0x00 */
#define S5P_SPDIFIN_USER_VALUE_3	S5P_HDMI_SPDIF_BASE(0x0028) /* SPDIFIN_USER_VALUE [23:16] 0x00 */
#define S5P_SPDIFIN_USER_VALUE_4	S5P_HDMI_SPDIF_BASE(0x002C) /* SPDIFIN_USER_VALUE [31:24] 0x00 */
#define S5P_SPDIFIN_CH_STATUS_0_1	S5P_HDMI_SPDIF_BASE(0x0030) /* SPDIFIN_CH_STATUS_0 [7:0] 0x00 */
#define S5P_SPDIFIN_CH_STATUS_0_2	S5P_HDMI_SPDIF_BASE(0x0034) /* SPDIFIN_CH_STATUS_0 [15:8] 0x00 */
#define S5P_SPDIFIN_CH_STATUS_0_3	S5P_HDMI_SPDIF_BASE(0x0038) /* SPDIFIN_CH_STATUS_0 [23:16] 0x00 */
#define S5P_SPDIFIN_CH_STATUS_0_4	S5P_HDMI_SPDIF_BASE(0x003C) /* SPDIFIN_CH_STATUS_0 [31:24] 0x00 */
#define S5P_SPDIFIN_CH_STATUS_1		S5P_HDMI_SPDIF_BASE(0x0040) /* SPDIFIN_CH_STATUS_1 0x00 */
#define S5P_SPDIFIN_FRAME_PERIOD_1	S5P_HDMI_SPDIF_BASE(0x0048) /* SPDIF_FRAME_PERIOD [7:0] 0x00 */
#define S5P_SPDIFIN_FRAME_PERIOD_2	S5P_HDMI_SPDIF_BASE(0x004C) /* SPDIF_FRAME_PERIOD [15:8] 0x00 */
#define S5P_SPDIFIN_Pc_INFO_1		S5P_HDMI_SPDIF_BASE(0x0050) /* SPDIFIN_Pc_INFO [7:0] 0x00 */
#define S5P_SPDIFIN_Pc_INFO_2		S5P_HDMI_SPDIF_BASE(0x0054) /* SPDIFIN_Pc_INFO [15:8] 0x00 */
#define S5P_SPDIFIN_Pd_INFO_1		S5P_HDMI_SPDIF_BASE(0x0058) /* SPDIFIN_Pd_INFO [7:0] 0x00 */
#define S5P_SPDIFIN_Pd_INFO_2		S5P_HDMI_SPDIF_BASE(0x005C) /* SPDIFIN_Pd_INFO [15:8] 0x00 */
#define S5P_SPDIFIN_DATA_BUF_0_1	S5P_HDMI_SPDIF_BASE(0x0060) /* SPDIFIN_DATA_BUF_0 [7:0] 0x00 */
#define S5P_SPDIFIN_DATA_BUF_0_2	S5P_HDMI_SPDIF_BASE(0x0064) /* SPDIFIN_DATA_BUF_0 [15:8] 0x00 */
#define S5P_SPDIFIN_DATA_BUF_0_3	S5P_HDMI_SPDIF_BASE(0x0068) /* SPDIFIN_DATA_BUF_0 [23:16] 0x00 */
#define S5P_SPDIFIN_USER_BUF_0		S5P_HDMI_SPDIF_BASE(0x006C) /* SPDIFIN_DATA_BUF_0 [31:28] 0x00 */
#define S5P_SPDIFIN_DATA_BUF_1_1	S5P_HDMI_SPDIF_BASE(0x0070) /* SPDIFIN_DATA_BUF_1 [7:0] 0x00 */
#define S5P_SPDIFIN_DATA_BUF_1_2	S5P_HDMI_SPDIF_BASE(0x0074) /* SPDIFIN_DATA_BUF_1 [15:8] 0x00 */
#define S5P_SPDIFIN_DATA_BUF_1_3	S5P_HDMI_SPDIF_BASE(0x0078) /* SPDIFIN_DATA_BUF_1 [23:16] 0x00 */
#define S5P_SPDIFIN_USER_BUF_1		S5P_HDMI_SPDIF_BASE(0x007C) /* SPDIFIN_DATA_BUF_1 [31:28] 0x00 */
			
/* HDMI_CON0 */
#define BLUE_SCR_EN   (1<<5)
#define BLUE_SCR_DIS  (0<<5)
#define ASP_EN    (1<<2)
#define ASP_DIS   (0<<2)
#define PWDN_ENB_NORMAL (1<<1)
#define PWDN_ENB_PD   (0<<1)
#define HDMI_EN   (1<<0)
#define HDMI_DIS    (~HDMI_EN)

/* HDMI_CON1 */
#define PX_LMT_CTRL_BYPASS  (0<<5)
#define PX_LMT_CTRL_RGB   (1<<5)
#define PX_LMT_CTRL_YPBPR   (2<<5)
#define PX_LMT_CTRL_RESERVED  (3<<5)

/* HDMI_CON2 */
#define VID_PREAMBLE_EN (0<<5)
#define VID_PREAMBLE_DIS  (1<<5)
#define GUARD_BAND_EN   (0<<1)
#define GUARD_BAND_DIS  (1<<1)

/* HDMI_STATUS */
#define AUTHEN_ACK_AUTH   (1<<7)
#define AUTHEN_ACK_NOT    (0<<7)
#define AUD_FIFO_OVF_FULL   (1<<6)
#define AUD_FIFO_OVF_NOT    (0<<6)
#define UPDATE_RI_INT_OCC   (1<<4)
#define UPDATE_RI_INT_NOT   (0<<4)
#define UPDATE_RI_INT_CLEAR   (1<<4)
#define UPDATE_PJ_INT_OCC   (1<<3)
#define UPDATE_PJ_INT_NOT   (0<<3)
#define UPDATE_PJ_INT_CLEAR   (1<<3)
#define EXCHANGEKSV_INT_OCC   (1<<2)
#define EXCHANGEKSV_INT_NOT   (0<<2)
#define EXCHANGEKSV_INT_CLEAR (1<<2)
#define WATCHDOG_INT_OCC    (1<<1)
#define WATCHDOG_INT_NOT    (0<<1)
#define WATCHDOG_INT_CLEAR  (1<<1)
#define WTFORACTIVERX_INT_OCC (1)
#define WTFORACTIVERX_INT_NOT (0)
#define WTFORACTIVERX_INT_CLEAR (1)

/* HDMI_STATUS_EN */
#define AUD_FIFO_OVF_EN   (1<<6)
#define AUD_FIFO_OVF_DIS    (0<<6)
#define UPDATE_RI_INT_EN    (1<<4)
#define UPDATE_RI_INT_DIS   (0<<4)
#define UPDATE_PJ_INT_EN    (1<<3)
#define UPDATE_PJ_INT_DIS   (0<<3)
#define EXCHANGEKSV_INT_EN  (1<<2)
#define EXCHANGEKSV_INT_DIS   (0<<2)
#define WATCHDOG_INT_EN   (1<<1)
#define WATCHDOG_INT_DIS    (0<<1)
#define WTFORACTIVERX_INT_EN  (1)
#define WTFORACTIVERX_INT_DIS (0)
#define HDCP_STATUS_EN_ALL 	UPDATE_RI_INT_EN|\
				UPDATE_PJ_INT_DIS|\
				EXCHANGEKSV_INT_EN|\
				WATCHDOG_INT_EN|\
				WTFORACTIVERX_INT_EN
				
#define HDCP_STATUS_DIS_ALL	(~0x1f)	

/* HDMI_HPD */
#define SW_HPD_PLUGGED  (1<<1)
#define SW_HPD_UNPLUGGED  (0<<1)

/* HDMI_MODE_SEL */
#define HDMI_MODE_EN  (1<<1)
#define HDMI_MODE_DIS (0<<1)
#define DVI_MODE_EN   (1)
#define DVI_MODE_DIS  (0)

/* HDCP_ENC_EN */
#define HDCP_ENC_ENABLE   (1)
#define HDCP_ENC_DISABLE  (0)

/* HDMI_BLUE_SCREEN0 */
#define SET_BLUESCREEN_0(a) (0xff&(a))

/* HDMI_BLUE_SCREEN1 */
#define SET_BLUESCREEN_1(a) (0xff&(a))

/* HDMI_BLUE_SCREEN2 */
#define SET_BLUESCREEN_2(a) (0xff&(a))

/* HDMI_YMAX */
#define SET_HDMI_YMAX(a)  (0xff&(a))

/* HDMI_YMIN */
#define SET_HDMI_YMIN(a)  (0xff&(a))

/* HDMI_CMAX */
#define SET_HDMI_CMAX(a)  (0xff&(a))

/* HDMI_CMIN */
#define SET_HDMI_CMIN(a)  (0xff&(a))

/* HDMI_DI_PREFIX */

/* HDMI_VBI_ST_MG */
#define SET_VBI_ST_MG(a)  (0xff&(a))

/* HDMI_VBI_END_MG */
#define SET_VBI_END_MG(a) (0xff&(a))

/* HDMI_VACT_ST_MG */
#define SET_VACT_ST_MG(a) (0xff&(a))

/* HDMI_VACT_END_MG
 HDMI_AUTH_ST_MG0
 HDMI_AUTH_ST_MG1
 HDMI_AUTH_END_MG0
 HDMI_AUTH_END_MG1 */

/* HDMI_H_BLANK0 */
#define SET_H_BLANK_L(a)  (0xff&(a))

/* HDMI_H_BLANK1 */
#define SET_H_BLANK_H(a)  (0x7&((a)>>8))

/* HDMI_V_BLANK0 */
#define SET_V2_BLANK_L(a) (0xff&(a))

/* HDMI_V_BLANK1 */
#define SET_V1_BLANK_L(a) ((0x1f&(a))<<3)
#define SET_V2_BLANK_H(a) (0x7&((a)>>8))

/* HDMI_V_BLANK2 */
#define SET_V1_BLANK_H(a) (0x3f&((a)>>5))

/* HDMI_H_V_LINE0 */
#define SET_V_LINE_L(a) (0xff&(a))

/* HDMI_H_V_LINE1 */
#define SET_H_LINE_L(a) ((0xf&(a))<<4)
#define SET_V_LINE_H(a) (0xf&((a)>>8))

/* HDMI_H_V_LINE2 */
#define SET_H_LINE_H(a) (0xff&((a)>>4))

/* HDMI_SYNC_MODE */
#define V_SYNC_POL_ACT_LOW  (1)
#define V_SYNC_POL_ACT_HIGH (0)

/* HDMI_INT_PRO_MODE */
#define INT_PRO_MODE_INTERLACE  (1)
#define INT_PRO_MODE_PROGRESSIVE  (0)

/* HDMI_SEND_PER_START0
 HDMI_SEND_PER_START1
 HDMI_SEND_PER_END0 */
#define SET_V_BOT_ST_L(a) (0xff&(a))

/* HDMI_SEND_PER_END1 */
#define SET_V_BOT_END_L(a)  ((0x1f&(a))<<3)
#define SET_V_BOT_ST_H(a) (0x7&((a)>>8))

/* HDMI_SEND_PER_END2 */
#define SET_V_BOT_END_H(a)  (0x3f&((a)>>5))

/* HDMI_V_BLANK_INTERLACE
 HDMI_V_BLANK_INTERLACE
 HDMI_V_BLANK_INTERLACE */

/* HDMI_H_SYNC_GEN0 */
#define SET_HSYNC_START_L(a)  (0xff&(a))

/* HDMI_H_SYNC_GEN1 */
#define SET_HSYNC_END_L(a)  ((0x3f&(a))<<2)
#define SET_HSYNC_START_H(a)  (0x3&((a)>>8))

/* HDMI_H_SYNC_GEN2 */
#define SET_HSYNC_POL_ACT_LOW (1<<4)
#define SET_HSYNC_POL_ACT_HIGH  (0<<4)
#define SET_HSYNC_END_H(a)  (0xf&((a)>>6))

/* HDMI_V_SYNC_GEN1_0 */
#define SET_VSYNC_T_END_L(a)  (0xff&(a))

/* HDMI_V_SYNC_GEN1_1 */
#define SET_VSYNC_T_ST_L(a)   ((0xf&(a))<<4)
#define SET_VSYNC_T_END_H(a)  (0xf&((a)>>8))

/* HDMI_V_SYNC_GEN1_2 */
#define SET_VSYNC_T_ST_H(a)   (0xff&((a)>>4))

/* HDMI_V_SYNC_GEN2_0 */
#define SET_VSYNC_B_END_L(a)  (0xff&(a))

/* HDMI_V_SYNC_GEN2_1 */
#define SET_VSYNC_B_ST_L(a)   ((0xf&(a))<<4)
#define SET_VSYNC_B_END_H(a)  (0xf&((a)>>8))

/* HDMI_V_SYNC_GEN2_2 */
#define SET_VSYNC_B_ST_H(a)   (0xff&((a)>>4))


/* HDMI_V_SYNC_GEN3_0 */
#define SET_VSYNC_H_POST_END_L(a) (0xff&(a))

/* HDMI_V_SYNC_GEN3_1 */
#define SET_VSYNC_H_POST_ST_L(a)  ((0xf&(a))<<4)
#define SET_VSYNC_H_POST_END_H(a) (0xf&((a)>>8))

/* HDMI_V_SYNC_GEN3_2 */
#define SET_VSYNC_H_POST_ST_H(a)  (0xff&((a)>>4))


/* Audio releated packet register
  HDMI_ASP_CON */
#define SACD_EN   (1<<5)
#define SACD_DIS  (0<<5)
#define AUD_MODE_MULTI_CH (1<<4)
#define AUD_MODE_2_CH   (0<<4)
#define SET_SP_PRE(a)   (0xf&(a))

/* HDMI_ASP_SP_FLAT */
#define SET_SP_FLAT(a)  (0xf&(a))

/* HDMI_ASP_CHCFG0
 HDMI_ASP_CHCFG1
 HDMI_ASP_CHCFG2
 HDMI_ASP_CHCFG3 */
#define SPK3R_SEL_I_PCM0L  (0<<27)
#define SPK3R_SEL_I_PCM0R  (1<<27)
#define SPK3R_SEL_I_PCM1L  (2<<27)
#define SPK3R_SEL_I_PCM1R  (3<<27)
#define SPK3R_SEL_I_PCM2L  (4<<27)
#define SPK3R_SEL_I_PCM2R  (5<<27)
#define SPK3R_SEL_I_PCM3L  (6<<27)
#define SPK3R_SEL_I_PCM3R  (7<<27)
#define SPK3L_SEL_I_PCM0L  (0<<24)
#define SPK3L_SEL_I_PCM0R  (1<<24)
#define SPK3L_SEL_I_PCM1L  (2<<24)
#define SPK3L_SEL_I_PCM1R  (3<<24)
#define SPK3L_SEL_I_PCM2L  (4<<24)
#define SPK3L_SEL_I_PCM2R  (5<<24)
#define SPK3L_SEL_I_PCM3L  (6<<24)
#define SPK3L_SEL_I_PCM3R  (7<<24)
#define SPK2R_SEL_I_PCM0L  (0<<19)
#define SPK2R_SEL_I_PCM0R  (1<<19)
#define SPK2R_SEL_I_PCM1L  (2<<19)
#define SPK2R_SEL_I_PCM1R  (3<<19)
#define SPK2R_SEL_I_PCM2L  (4<<19)
#define SPK2R_SEL_I_PCM2R  (5<<19)
#define SPK2R_SEL_I_PCM3L  (6<<19)
#define SPK2R_SEL_I_PCM3R  (7<<19)
#define SPK2L_SEL_I_PCM0L  (0<<16)
#define SPK2L_SEL_I_PCM0R  (1<<16)
#define SPK2L_SEL_I_PCM1L  (2<<16)
#define SPK2L_SEL_I_PCM1R  (3<<16)
#define SPK2L_SEL_I_PCM2L  (4<<16)
#define SPK2L_SEL_I_PCM2R  (5<<16)
#define SPK2L_SEL_I_PCM3L  (6<<16)
#define SPK2L_SEL_I_PCM3R  (7<<16)
#define SPK1R_SEL_I_PCM0L  (0<<11)
#define SPK1R_SEL_I_PCM0R  (1<<11)
#define SPK1R_SEL_I_PCM1L  (2<<11)
#define SPK1R_SEL_I_PCM1R  (3<<11)
#define SPK1R_SEL_I_PCM2L  (4<<11)
#define SPK1R_SEL_I_PCM2R  (5<<11)
#define SPK1R_SEL_I_PCM3L  (6<<11)
#define SPK1R_SEL_I_PCM3R  (7<<11)
#define SPK1L_SEL_I_PCM0L  (0<<8)
#define SPK1L_SEL_I_PCM0R  (1<<8)
#define SPK1L_SEL_I_PCM1L  (2<<8)
#define SPK1L_SEL_I_PCM1R  (3<<8)
#define SPK1L_SEL_I_PCM2L  (4<<8)
#define SPK1L_SEL_I_PCM2R  (5<<8)
#define SPK1L_SEL_I_PCM3L  (6<<8)
#define SPK1L_SEL_I_PCM3R  (7<<8)
#define SPK0R_SEL_I_PCM0L  (0<<3)
#define SPK0R_SEL_I_PCM0R  (1<<3)
#define SPK0R_SEL_I_PCM1L  (2<<3)
#define SPK0R_SEL_I_PCM1R  (3<<3)
#define SPK0R_SEL_I_PCM2L  (4<<3)
#define SPK0R_SEL_I_PCM2R  (5<<3)
#define SPK0R_SEL_I_PCM3L  (6<<3)
#define SPK0R_SEL_I_PCM3R  (7<<3)
#define SPK0L_SEL_I_PCM0L  (0)
#define SPK0L_SEL_I_PCM0R  (1)
#define SPK0L_SEL_I_PCM1L  (2)
#define SPK0L_SEL_I_PCM1R  (3)
#define SPK0L_SEL_I_PCM2L  (4)
#define SPK0L_SEL_I_PCM2R  (5)
#define SPK0L_SEL_I_PCM3L  (6)
#define SPK0L_SEL_I_PCM3R  (7)

/* HDMI_ACR_CON */
#define ALT_CTS_RATE_CTS_1  (0<<3)
#define ALT_CTS_RATE_CTS_11   (1<<3)
#define ALT_CTS_RATE_CTS_21   (2<<3)
#define ALT_CTS_RATE_CTS_31   (3<<3)
#define ACR_TX_MODE_NO_TX   (0)
#define ACR_TX_MODE_TX_ONCE   (1)
#define ACR_TX_MODE_TXCNT_VBI (2)
#define ACR_TX_MODE_TX_VPC  (3)
#define ACR_TX_MODE_MESURE_CTS  (4)

/* HDMI_ACR_MCTS0
 HDMI_ACR_MCTS1
 HDMI_ACR_MCTS2 */
#define SET_ACR_MCTS(a) (0xfffff&(a))

/* HDMI_ACR_CTS0
 HDMI_ACR_CTS1
 HDMI_ACR_CTS2 */
#define SET_ACR_CTS(a)  (0xfffff&(a))

/* HDMI_ACR_N0
 HDMI_ACR_N1
 HDMI_ACR_N2 */
#define SET_ACR_N(a)  (0xfffff&(a))

/* HDMI_ACR_LSB2 */
#define SET_ACR_LSB2(a) (0xff&(a))

/* HDMI_ACR_TXCNT */
#define SET_ACR_TXCNT(a)  (0x1f&(a))

/* HDMI_ACR_TXINTERNAL */
#define SET_ACR_TX_INTERNAL(a)  (0xff&(a))

/* HDMI_ACR_CTS_OFFSET */
#define SET_ACR_CTS_OFFSET(a) (0xff&(a))

/* HDMI_GCP_CON */
#define GCP_CON_NO_TRAN     (0)
#define GCP_CON_TRANS_ONCE    (1)
#define GCP_CON_TRANS_EVERY_VSYNC (2)

/* HDMI_GCP_BYTE1 */
#define SET_GCP_BYTE1(a)  (0xff&(a))


/* ACP and ISRC1/2 packet registers
 HDMI_ACP_CON */
#define SET_ACP_FR_RATE(a)    ((0x1f&(a))<<3)
#define ACP_CON_NO_TRAN     (0)
#define ACP_CON_TRANS_ONCE    (1)
#define ACP_CON_TRANS_EVERY_VSYNC (2)

/* HDMI_ACP_TYPE */
#define SET_ACP_TYPE(a)   (0xff&(a))


/* HDMI_ACP_DATA0
 HDMI_ACP_DATA1
 HDMI_ACP_DATA2
 HDMI_ACP_DATA3
 HDMI_ACP_DATA4
 HDMI_ACP_DATA5
 HDMI_ACP_DATA6
 HDMI_ACP_DATA7
 HDMI_ACP_DATA8
 HDMI_ACP_DATA9
 HDMI_ACP_DATA10
 HDMI_ACP_DATA11
 HDMI_ACP_DATA12
 HDMI_ACP_DATA13
 HDMI_ACP_DATA14
 HDMI_ACP_DATA15
 HDMI_ACP_DATA16 */
#define SET_ACP_DATA(a)   (0xff&(a))


/* HDMI_ISRC_CON */
#define SET_ISRC_FR_RATE(a)     ((0x1f&(a))<<3)
#define ISRC_EN         (1<<2)
#define ISRC_DIS        (0<<2)
#define ISRC_TX_CON_NO_TRANS    (0)
#define ISRC_TX_CON_TRANS_ONCE    (1)
#define ISRC_TX_CON_TRANS_EVERY_VSYNC (2)

/* HDMI_ISRC1_HEADER1 */
#define SET_ISRC1_HEADER(a) (0xff&(a))

/* HDMI_ISRC1_DATA0
 HDMI_ISRC1_DATA1
 HDMI_ISRC1_DATA2
 HDMI_ISRC1_DATA3
 HDMI_ISRC1_DATA4
 HDMI_ISRC1_DATA5
 HDMI_ISRC1_DATA6
 HDMI_ISRC1_DATA7
 HDMI_ISRC1_DATA8
 HDMI_ISRC1_DATA9
 HDMI_ISRC1_DATA10
 HDMI_ISRC1_DATA11
 HDMI_ISRC1_DATA12
 HDMI_ISRC1_DATA13
 HDMI_ISRC1_DATA14
 HDMI_ISRC1_DATA15 */
#define SET_ISRC1_DATA(a) (0xff&(a))

/* HDMI_ISRC2_DATA0
 HDMI_ISRC2_DATA1
 HDMI_ISRC2_DATA2
 HDMI_ISRC2_DATA3
 HDMI_ISRC2_DATA4
 HDMI_ISRC2_DATA5
 HDMI_ISRC2_DATA6
 HDMI_ISRC2_DATA7
 HDMI_ISRC2_DATA8
 HDMI_ISRC2_DATA9
 HDMI_ISRC2_DATA10
 HDMI_ISRC2_DATA11
 HDMI_ISRC2_DATA12
 HDMI_ISRC2_DATA13
 HDMI_ISRC2_DATA14
 HDMI_ISRC2_DATA15 */
#define SET_ISRC2_DATA(a) (0xff&(a))


/* AVI info-frame registers
 HDMI_AVI_CON */
#define AVI_TX_CON_NO_TRANS     (0)
#define AVI_TX_CON_TRANS_ONCE     (1)
#define AVI_TX_CON_TRANS_EVERY_VSYNC  (2)


/* HDMI_AVI_CHECK_SUM */
#define SET_AVI_CHECK_SUM(a)  (0xff&(a))

#define HDMI_CON_PXL_REP_RATIO_MASK		(1<<1 | 1<<0)
#define HDMI_DOUBLE_PIXEL_REPETITION		(0x01)
#define AVI_PIXEL_REPETITION_DOUBLE		(1<<0)
#define AVI_PICTURE_ASPECT_4_3			(1<<4)                            
#define AVI_PICTURE_ASPECT_16_9			(1<<5)

/* HDMI_AVI_BYTE1
 HDMI_AVI_BYTE2
 HDMI_AVI_BYTE3
 HDMI_AVI_BYTE4
 HDMI_AVI_BYTE5
 HDMI_AVI_BYTE6
 HDMI_AVI_BYTE7
 HDMI_AVI_BYTE8
 HDMI_AVI_BYTE9
 HDMI_AVI_BYTE10
 HDMI_AVI_BYTE11
 HDMI_AVI_BYTE12
 HDMI_AVI_BYTE13 */
#define SET_AVI_BYTE(a)   (0xff&(a))

/* Audio info-frame registers
 HDMI_AUI_CON */
#define AUI_TX_CON_NO_TRANS     (0)
#define AUI_TX_CON_TRANS_ONCE     (1)
#define AUI_TX_CON_TRANS_EVERY_VSYNC  (2)

/* HDMI_AUI_CHECK_SUM
 HDMI_AVI_CHECK_SUM */
#define SET_AUI_CHECK_SUM(a)  (0xff&(a))

/* HDMI_AUI_BYTE1
 HDMI_AUI_BYTE2
 HDMI_AUI_BYTE3
 HDMI_AUI_BYTE4
 HDMI_AUI_BYTE5 */
#define SET_AUI_BYTE(a)   (0xff&(a))

/* MPEG source info-frame registers
 HDMI_MPG_CON */
#define MPG_TX_CON_NO_TRANS     (0)
#define MPG_TX_CON_TRANS_ONCE     (1)
#define MPG_TX_CON_TRANS_EVERY_VSYNC  (2)

/* HDMI_MPG_CHECK_SUM */
#define SET_MPG_CHECK_SUM(a)  (0xff&(a))

/* HDMI_MPG_BYTE1
 HDMI_MPG_BYTE2
 HDMI_MPG_BYTE3
 HDMI_MPG_BYTE4
 HDMI_MPG_BYTE5 */
#define SET_MPG_BYTE(a)   (0xff&(a))

/* Souerce product desciptor info-f
 HDMI_SPD_CON */
#define SPD_TX_CON_NO_TRANS     (0)
#define SPD_TX_CON_TRANS_ONCE     (1)
#define SPD_TX_CON_TRANS_EVERY_VSYNC  (2)

/* HDMI_SPD_HEADER0
 HDMI_SPD_HEADER1
 HDMI_SPD_HEADER2 */
#define SET_SPD_HEADER(a) (0xff&(a))

/* HDMI_SPD_DATA0
 HDMI_SPD_DATA1
 HDMI_SPD_DATA2
 HDMI_SPD_DATA3
 HDMI_SPD_DATA4
 HDMI_SPD_DATA5
 HDMI_SPD_DATA6
 HDMI_SPD_DATA7
 HDMI_SPD_DATA8
 HDMI_SPD_DATA9
 HDMI_SPD_DATA10
 HDMI_SPD_DATA11
 HDMI_SPD_DATA12
 HDMI_SPD_DATA13
 HDMI_SPD_DATA14
 HDMI_SPD_DATA15
 HDMI_SPD_DATA16
 HDMI_SPD_DATA17
 HDMI_SPD_DATA18
 HDMI_SPD_DATA19
 HDMI_SPD_DATA20
 HDMI_SPD_DATA21
 HDMI_SPD_DATA22
 HDMI_SPD_DATA23
 HDMI_SPD_DATA24
 HDMI_SPD_DATA25
 HDMI_SPD_DATA26
 HDMI_SPD_DATA27 */
#define SET_SPD_DATA(a)   (0xff&(a))


/* HDMI_CSC_CON */
#define OUT_OFFSET_SEL_RGB_FR (0<<4)
#define OUT_OFFSET_SEL_RGB_LR (2<<4)
#define OUT_OFFSET_SEL_YCBCR  (3<<4)
#define IN_CLIP_EN      (1<<2)
#define IN_CLIP_DIS     (0<<2)
#define IN_OFFSET_SEL_RGB_FR  (0)
#define IN_OFFSET_SEL_RGB_LR  (2)
#define IN_OFFSET_SEL_YCBCR   (3)

/* HDMI_Y_G_COEF_L
 HDMI_Y_G_COEF_H
 HDMI_Y_B_COEF_L
 HDMI_Y_B_COEF_H
 HDMI_Y_R_COEF_L
 HDMI_Y_R_COEF_H
 HDMI_CB_G_COEF_L
 HDMI_CB_G_COEF_H
 HDMI_CB_B_COEF_L
 HDMI_CB_B_COEF_H
 HDMI_CB_R_COEF_L
 HDMI_CB_R_COEF_H
 HDMI_CR_G_COEF_L
 HDMI_CR_G_COEF_H
 HDMI_CR_B_COEF_L
 HDMI_CR_B_COEF_H
 HDMI_CR_R_COEF_L
 HDMI_CR_R_COEF_H */
#define SET_HDMI_CSC_COEF_L(a)  (0xff&(a))
#define SET_HDMI_CSC_COEF_H(a)  (0x3&((a)>>8))

/* Test pattern generation register
 HDMI_TPGEN_0
 HDMI_TPGEN_1
 HDMI_TPGEN_2
 HDMI_TPGEN_3
 HDMI_TPGEN_4
 HDMI_TPGEN_5
 HDMI_TPGEN_6

 HDCP_RX_SHA_1_0_0
 HDCP_RX_SHA_1_0_1
 HDCP_RX_SHA_1_0_2
 HDCP_RX_SHA_1_0_3
 HDCP_RX_SHA_1_1_0
 HDCP_RX_SHA_1_1_1
 HDCP_RX_SHA_1_1_2
 HDCP_RX_SHA_1_1_3
 HDCP_RX_SHA_1_2_0
 HDCP_RX_SHA_1_2_1
 HDCP_RX_SHA_1_2_2
 HDCP_RX_SHA_1_2_3
 HDCP_RX_SHA_1_3_0
 HDCP_RX_SHA_1_3_1
 HDCP_RX_SHA_1_3_2
 HDCP_RX_SHA_1_3_3
 HDCP_RX_SHA_1_4_0
 HDCP_RX_SHA_1_4_1
 HDCP_RX_SHA_1_4_2
 HDCP_RX_SHA_1_4_3 */
#define SET_HDMI_SHA1(a)  (0xff&(a))

/* HDCP_RX_KSV_0_0
 HDCP_RX_KSV_0_1
 HDCP_RX_KSV_0_2
 HDCP_RX_KSV_0_3
 HDCP_RX_KSV_1_0
 HDCP_RX_KSV_1_1

 HDCP_AUTH_STAT

 HDCP_CTRL

 HDCP_CHECK_RESULT

 HDCP_BKSV0_0
 HDCP_BKSV0_1
 HDCP_BKSV0_2
 HDCP_BKSV0_3
 HDCP_BKSV1
 HDCP_AKSV0_0
 HDCP_AKSV0_1
 HDCP_AKSV0_2
 HDCP_AKSV0_3
 HDCP_AKSV1

 HDCP_AN0_0
 HDCP_AN0_1
 HDCP_AN0_2
 HDCP_AN0_3
 HDCP_AN1_0
 HDCP_AN1_1
 HDCP_AN1_2
 HDCP_AN1_3

 HDCP_BCAPS
 HDCP_BSTATUS0
 HDCP_BSTATUS1

 HDCP_RI_0
 HDCP_RI_1
 HDCP_PJ

 HDCP_OFFSET_TX0
 HDCP_OFFSET_TX1
 HDCP_OFFSET_TX2
 HDCP_OFFSET_TX3
 HDCP_CYCLE_AA
 HDCP_I2C_INT
 HDCP_AN_INT
 HDCP_WATCHDOG_INT
 HDCP_RI_INT
 HDCP_PJ_INT

 TG SFR
 TG_CMD */
#define GETSYNC_TYPE_EN   	(1<<4)
#define GETSYNC_TYPE_DIS	(~GETSYNC_TYPE_EN)
#define GETSYNC_EN    		(1<<3)
#define GETSYNC_DIS   		(~GETSYNC_EN)
#define FIELD_EN    		(1<<1)
#define FIELD_DIS    		(~FIELD_EN)
#define TG_EN     		(1)
#define TG_DIS      		(~TG_EN)

/* TG_CFG
 TG_CB_SZ
 TG_INDELAY_L
 TG_INDELAY_H
 TG_POL_CTRL

 TG_H_FSZ_L */
#define SET_TG_H_FSZ_L(a) (0xff&(a))

/* TG_H_FSZ_H */
#define SET_TG_H_FSZ_H(a) (0x1f&((a)>>8))

/* TG_HACT_ST_L */
#define SET_TG_HACT_ST_L(a) (0xff&(a))

/* TG_HACT_ST_H */
#define SET_TG_HACT_ST_H(a) (0xf&((a)>>8))

/* TG_HACT_SZ_L */
#define SET_TG_HACT_SZ_L(a) (0xff&(a))

/* TG_HACT_SZ_H */
#define SET_TG_HACT_SZ_H(a) (0xf&((a)>>8))

/* TG_V_FSZ_L */
#define SET_TG_V_FSZ_L(a) (0xff&(a))

/* TG_V_FSZ_H */
#define SET_TG_V_FSZ_H(a) (0x7&((a)>>8))

/* TG_VSYNC_L */
#define SET_TG_VSYNC_L(a) (0xff&(a))

/* TG_VSYNC_H */
#define SET_TG_VSYNC_H(a) (0x7&((a)>>8))

/* TG_VSYNC2_L */
#define SET_TG_VSYNC2_L(a)  (0xff&(a))

/* TG_VSYNC2_H */
#define SET_TG_VSYNC2_H(a)  (0x7&((a)>>8))

/* TG_VACT_ST_L */
#define SET_TG_VACT_ST_L(a) (0xff&(a))

/* TG_VACT_ST_H */
#define SET_TG_VACT_ST_H(a) (0x7&((a)>>8))

/* TG_VACT_SZ_L */
#define SET_TG_VACT_SZ_L(a) (0xff&(a))
 
/* TG_VACT_SZ_H */
#define SET_TG_VACT_SZ_H(a) (0x7&((a)>>8))

/* TG_FIELD_CHG_L */
#define SET_TG_FIELD_CHG_L(a) (0xff&(a))

/* TG_FIELD_CHG_H */
#define SET_TG_FIELD_CHG_H(a) (0x7&((a)>>8))

/* TG_VACT_ST2_L */
#define SET_TG_VACT_ST2_L(a)  (0xff&(a))

/* TG_VACT_ST2_H */
#define SET_TG_VACT_ST2_H(a)  (0x7&((a)>>8))

/* TG_VACT_SC_ST_L
 TG_VACT_SC_ST_H
 TG_VACT_SC_SZ_L
 TG_VACT_SC_SZ_H

 TG_VSYNC_TOP_HDMI_L */
#define SET_TG_VSYNC_TOP_HDMI_L(a)  (0xff&(a))

/* TG_VSYNC_TOP_HDMI_H */
#define SET_TG_VSYNC_TOP_HDMI_H(a)  (0x7&((a)>>8))

/* TG_VSYNC_BOT_HDMI_L */
#define SET_TG_VSYNC_BOT_HDMI_L(a)  (0xff&(a))

/* TG_VSYNC_BOT_HDMI_H */
#define SET_TG_VSYNC_BOT_HDMI_H(a)  (0x7&((a)>>8))

/* TG_FIELD_TOP_HDMI_L */
#define SET_TG_FIELD_TOP_HDMI_L(a)  (0xff&(a))

/* TG_FIELD_TOP_HDMI_H */
#define SET_TG_FIELD_TOP_HDMI_H(a)  (0x7&((a)>>8))

/* TG_FIELD_BOT_HDMI_L */
#define SET_TG_FIELD_BOT_HDMI_L(a)  (0xff&(a))

/* TG_FIELD_BOT_HDMI_H */
#define SET_TG_FIELD_BOT_HDMI_H(a)  (0x7&((a)>>8))

/* TG_HSYNC_HDOUT_ST_L
 TG_HSYNC_HDOUT_ST_H
 TG_HSYNC_HDOUT_END_L
 TG_HSYNC_HDOUT_END_H
 TG_VSYNC_HDOUT_ST_L
 TG_VSYNC_HDOUT_ST_H
 TG_VSYNC_HDOUT_END_L
 TG_VSYNC_HDOUT_END_H
 TG_VSYNC_HDOUT_DLY_L
 TG_VSYNC_HDOUT_DLY_H
 TG_BT_ERR_RANGE
 TG_BT_ERR_RESULT
 TG_COR_THR
 TG_COR_NUM
 TG_BT_CON
 TG_BT_H_FSZ_L
 TG_BT_H_FSZ_H
 TG_BT_HSYNC_ST
 TG_BT_HSYNC_SZ
 TG_BT_FSZ_L
 TG_BT_FSZ_H
 TG_BT_VACT_T_ST_L
 TG_BT_VACT_T_ST_H
 TG_BT_VACT_B_ST_L
 TG_BT_VACT_B_ST_H
 TG_BT_VACT_SZ_L
 TG_BT_VACT_SZ_H
 TG_BT_VSYNC_SZ
 SPDIFIN_CLK_CTRL
 SPDIFIN_OP_CTRL

 SPDIFIN_IRQ_MASK */
#define IRQ_WRONG_SIGNAL_ENABLE          (1<<0)
#define IRQ_CH_STATUS_RECOVERED_ENABLE         (1<<1)
#define IRQ_WRONG_PREAMBLE_ENABLE          (1<<2)
#define IRQ_STREAM_HEADER_NOT_DETECTED_ENABLE      (1<<3)
#define IRQ_STREAM_HEADER_DETECTED_ENABLE        (1<<4)
#define IRQ_STREAM_HEADER_NOT_DETECTED_AT_RIGHTTIME_ENABLE (1<<5)
#define IRQ_ABNORMAL_PD_ENABLE           (1<<6)
#define IRQ_BUFFER_OVERFLOW_ENABLE         (1<<7)

/* SPDIFIN_IRQ_STATUS
SPDIFIN_CONFIG_1 */

#define CONFIG_FILTER_3_SAMPLE      (0<<6)
#define CONFIG_FILTER_2_SAMPLE      (1<<6)
#define CONFIG_LINEAR_PCM_TYPE      (0<<5)
#define CONFIG_NON_LINEAR_PCM_TYPE    (1<<5)
#define CONFIG_PCPD_AUTO_SET      (0<<4)
#define CONFIG_PCPD_MANUAL_SET      (1<<4)
#define CONFIG_WORD_LENGTH_AUTO_SET   (0<<3)
#define CONFIG_WORD_LENGTH_MANUAL_SET   (1<<3)
#define CONFIG_U_V_C_P_NEGLECT      (0<<2)
#define CONFIG_U_V_C_P_REPORT     (1<<2)
#define CONFIG_BURST_SIZE_1       (0<<1)
#define CONFIG_BURST_SIZE_2       (1<<1)
#define CONFIG_DATA_ALIGN_16BIT     (0<<0)
#define CONFIG_DATA_ALIGN_32BIT     (1<<0)

/* SPDIFIN_CONFIG_2
 SPDIFIN_USER_VALUE_1
 SPDIFIN_USER_VALUE_2
 SPDIFIN_USER_VALUE_3
 SPDIFIN_USER_VALUE_4
 SPDIFIN_CH_STATUS_0_1
 SPDIFIN_CH_STATUS_0_2
 SPDIFIN_CH_STATUS_0_3
 SPDIFIN_CH_STATUS_0_4
 SPDIFIN_CH_STATUS_1
 SPDIFIN_FRAME_PERIOD_1
 SPDIFIN_FRAME_PERIOD_2
 SPDIFIN_PC_INFO_1
 SPDIFIN_PC_INFO_2
 SPDIFIN_PD_INFO_1
 SPDIFIN_PD_INFO_2
 SPDIFIN_DATA_BUF_0_1
 SPDIFIN_DATA_BUF_0_2
 SPDIFIN_DATA_BUF_0_3
 SPDIFIN_USER_BUF_0
 SPDIFIN_USER_BUF_1_1
 SPDIFIN_USER_BUF_1_2
 SPDIFIN_USER_BUF_1_3
 SPDIFIN_USER_BUF_1

 HAES_START
 HAES_DATA_SIZE_L
 HAES_DATA_SIZE_H
 HAES_DATA */

/* Macros - for HDCP */

/* HDMI SYSTEM STATUS FLAG REGISTER (STATUS, R/W, ADDRESS = 0XF030_0010) */
#define AUTHEN_ACK_POS     7
#define AUD_FIFO_OVF_POS     6
/* RESERVED         5 */
#define UPDATE_RI_INT_POS    4
#define UPDATE_PJ_INT_POS    3
#define EXCHANGEKSV_INT_POS    2
#define WATCHDOG_INT_POS     1
#define WTFORACTIVERX_INT_POS  0

#define AUTHENTICATED        (0x1 << 7)
#define NOT_YET_AUTHENTICATED    (0x0 << 7)
#define AUD_FIFO_OVF_INT_OCCURRED    (0x1 << 6)
#define AUD_FIFO_OVF_INT_NOT_OCCURRED  (0x0 << 6)
/* RESERVED           5 */
#define UPDATE_RI_INT_OCCURRED     (0x1 << 4)
#define UPDATE_RI_INT_NOT_OCCURRED   (0x0 << 4)
#define UPDATE_PJ_INT_OCCURRED     (0x1 << 3)
#define UPDATE_PJ_INT_NOT_OCCURRED   (0x0 << 3)
#define EXCHANGEKSV_INT_OCCURRED   (0x1 << 2)
#define EXCHANGEKSV_INT_NOT_OCCURRED   (0x0 << 2)
#define WATCHDOG_INT_OCCURRED    (0x1 << 1)
#define WATCHDOG_INT_NOT_OCCURRED    (0x0 << 1)
#define WTFORACTIVERX_INT_OCCURRED   (0x1 << 0)
#define WTFORACTIVERX_INT_NOT_OCCURRED (0x0 << 0)

/* HDMI SYSTEM STATUS ENABLE REGISTER (STATUS_EN, R/W, ADDRESS = 0XF030_0020) */
/* RESERVED         7 */
#define AUD_FIFO_OVF_INT_EN  (0x1 << 6)
#define AUD_FIFO_OVF_INT_DIS (0x0 << 6)
/* RESERVED         5 */

/* EFUSE CONTROL REGISTER */
#define EFUSE_CTRL_ACTIVATE		(1)
#define EFUSE_ADDR_WIDTH		(240)
#define EFUSE_SIGDEV_ASSERT		(0)
#define EFUSE_SIGDEV_DEASSERT		(96)
#define EFUSE_PRCHG_ASSERT		(0)
#define EFUSE_PRCHG_DEASSERT		(144)
#define EFUSE_FSET_ASSERT		(48)
#define EFUSE_FSET_DEASSERT		(192)
#define EFUSE_SENSING			(240)
#define EFUSE_SCK_ASSERT		(48)
#define EFUSE_SCK_DEASSERT		(144)
#define EFUSE_SDOUT_OFFSET		(192)
#define EFUSE_READ_OFFSET		(192)

#define EFUSE_ECC_DONE			(1<<0)
#define EFUSE_ECC_BUSY			(1<<1)
#define EFUSE_ECC_FAIL			(1<<2)

/* HDCP CONTROL REGISTER (HDCP_CTRL1, R/W, ADDRESS = 0XF030_ 0680) */
#define EN_PJ_EN	(0x1 << 4)
#define EN_PJ_DIS	(~EN_PJ_EN)
/* RESERVED         3 */
#define SET_REPEATER_TIMEOUT	(0x1 << 2)
#define CLEAR_REPEATER_TIMEOUT	(~SET_REPEATER_TIMEOUT)
#define CP_DESIRED_EN		(0x1 << 1)
#define CP_DESIRED_DIS		(~CP_DESIRED_EN)
#define ENABLE_1_DOT_1_FEATURE_EN	(0x1 << 0)
#define ENABLE_1_DOT_1_FEATURE_DIS	(~ENABLE_1_DOT_1_FEATURE_EN)

/* HDCP_CHECK_RESULT, R/W, ADDRESS = 0XF030_ 0690 */
#define Pi_MATCH_RESULT__YES ((0x1<<3) | (0x1<<2))
#define Pi_MATCH_RESULT__NO  ((0x1<<3) | (0x0<<2))
#define Ri_MATCH_RESULT__YES ((0x1<<1) | (0x1<<0))
#define Ri_MATCH_RESULT__NO  ((0x1<<1) | (0x0<<0))
#define CLEAR_ALL_RESULTS  0x0

/* HDCP ENCRYPTION ENABLE REGISTER (ENC_EN, R/W, ADDRESS = 0XF030_0044) */
#define HDCP_ENC_DIS  (0x0 << 0)

/* BCAPS INFORMATION FROM RX. THIS VALUE IS THE DATA READ FROM RX
 * (HDCP_BCAPS, R/W,ADDRESS = 0XF030_0700)
 RESERVED         7 */
#define REPEATER_SET   (0x1 << 6)
#define REPEATERP_CLEAR  (0x1 << 6)
#define READY_SET    (0x1 << 5)
#define READY_CLEAR    (0x1 << 5)
#define FAST_SET     (0x1 << 4)
#define FAST_CLEAR   (0x1 << 4)
/* RESERVED         3
 RESERVED         2 */
#define ONE_DOT_ONE_FEATURES_SET    (0x1 << 1)
#define ONE_DOT_ONE_FEATURES_CLEAR  (0x1 << 1)
#define FAST_REAUTHENTICATION_SET (0x1 << 0)
#define FAST_REAUTHENTICATION_CLEAR (0x1 << 0)

/* HAES REGISTERS
 HAES CONTROL */
#define SCRAMBLER_KEY_START_EN  (0x1 << 7)
#define SCRAMBLER_KEY_START_DIS   (~SCRAMBLER_KEY_START_EN)
#define SCRAMBLER_KEY_DONE    (0x1 << 6)
#define SCRAMBLER_KEY_GENERATING  (0x0 << 6)
/* RESERVED          1<-->5 */
#define HAES_START_EN     (0x1 << 0)
#define HAES_DECRYPTION_DONE    (0x0 << 0)

#define AN_SIZE 8
#define AKSV_SIZE 5
#define BKSV_SIZE 5
#define HDCPLink_Addr 0x74

#define CABLE_PLUGGED  1<<1
#define CABLE_UNPLUGGED  0<<1

#define DDC_Addr   0xA0
#define eDDC_Addr    0x60
#define HDCPLink_Addr  0x74

#define HDCP_Bksv    0x00
#define HDCP_Aksv    0x10
#define HDCP_Ainfo   0x15
#define HDCP_An    0x18
#define HDCP_Ri    0x08
#define HDCP_Bcaps   0x40
#define HDCP_BStatus   0x41
#define HDCP_Pj    0x0a

#define HDCP_KSVFIFO   0x43
#define HDCP_SHA1    0x20

#define HDMI_MODE_HDMI 0
#define HDMI_MODE_DVI  1

#define EDID_SEGMENT_ID  0x60
#define EDID_SEGMENT0  0x00
#define EDID_SEGMENT1  0x01

#define EDID_DEVICE_ID 0xA0
#define EDID_ADDR_START  0x00
#define EDID_ADDR_EXT  0x80
#define EDID_RCOUNT  127

#define EDID_POS_EXTENSION   0x7E
#define EDID_POS_CHECKSUM    0x7F
/* #define EDID_POS_ERROR   516 // 512+1  // move to hdmi.h by shin...1229 */
#define VALID_EDID     0xA5
#define NO_VALID_EDID    0

#define EDID_POS_RBUFFER0    0x00  //segment0, 128-byte
#define EDID_POS_RBUFFER1    0x80  //segment0, 256-byte
#define EDID_POS_RBUFFER2    0x100  //segment1, 128-byte
#define EDID_POS_RBUFFER3    0x180  //segment1, 256-byte

#define EDID_TIMING_EXT_TAG_ADDR_POS   0x80
#define EDID_TIMING_EXT_REV_NUMBER     0x81
#define EDID_DETAILED_TIMING_OFFSET_POS  0x82
#define EDID_COLOR_SPACE_ADDR      0x83
#define EDID_DATA_BLOCK_ADDRESS      0x84
#define EDID_TIMING_EXT_TAG_VAL      0x02
#define EDID_YCBCR444_CS_MASK      0x20
#define EDID_YCBCR422_CS_MASK      0x10
#define EDID_TAG_CODE_MASK       0xE0
#define EDID_DATA_BLOCK_SIZE_MASK    0x1F
#define EDID_NATIVE_RESOLUTION_MASK    0x80

#define EDID_SHORT_AUD_DEC_TAG    0x20
#define EDID_SHORT_VID_DEC_TAG    0x40
#define EDID_HDMI_VSDB_TAG      0x60
#define EDID_SPEAKER_ALLOCATION_TAG   0x80

#define COLOR_SPACE_RGB   0
#define COLOR_SPACE_YCBCR444  1
#define COLOR_SPACE_YCBCR422  2

#define SHORT_VID_720_480P_4_3_NT   0x01
#define SHORT_VID_720_480P_16_9_NT  0x02
#define SHORT_VID_1280_720P_16_9_NT   0x04
#define SHORT_VID_1920_1080i_16_9_NT  0x08
#define SHORT_VID_720_576P_4_3_PAL  0x10
#define SHORT_VID_720_576P_16_9_PAL   0x20
#define SHORT_VID_1280_720P_16_9_PAL  0x40
#define SHORT_VID_1920_1080i_16_9_PAL 0x80

#define SET_HDMI_RESOLUTION_480P    0x00
#define SET_HDMI_RESOLUTION_720P    0x01
#define SET_HDMI_RESOLUTION_1080i     0x02

#define HDMI_WAIT_TIMEOUT    20
#define AUTHENTICATION_SUCCESS  0
#define AUTHENTICATION_FAILURE  1
#define AUTHENTICATION_FAIL_CNT 2

#define HDCP_MAX_DEVS 128
#define HDCP_KSV_SIZE 5

#define CMD_IIC_ADDRMODE_CHANGE    0xFF

/* IIC Addressing Mode Definition */
#define IIC_ADDRMODE_1      0
#define IIC_ADDRMODE_2      1
#define IIC_ADDRMODE_3      2
#define HDMI_IIC_ADDRMODE     IIC_ADDRMODE_1

#define IIC_ACK  0
#define IIC_NOACK  1

#define EDID_POS_ERROR    512
#define R_VAL_RETRY_CNT   5

#define CABLE_INSERT 1
#define CABLE_REMOVE (~CABLE_INSERT)

#define HDMI_PHY_READY          (1<<0)

/* Color Depth
 CD0, CD1, CD2, CD3 */

#define GCP_CD_NOT_INDICATED		0
#define GCP_CD_24BPP			(1<<2)
#define GCP_CD_30BPP			(1<<0 | 1<<2)
#define GCP_CD_36BPP			(1<<1 | 1<<2)
#define GCP_CD_48BPP			(1<<0 | 1<<1 | 1<<2)

#define GCP_DEFAULT_PHASE		1

/* for DC_CONTRAL */
#define HDMI_DC_CTL_8 			0
#define HDMI_DC_CTL_10			(1<<0)
#define HDMI_DC_CTL_12			(1<<1)

#define DO_NOT_TRANSMIT			(0)

#define HPD_SW_ENABLE               	(1<<0)
#define HPD_SW_DISABLE              	(0)
#define HPD_ON                      	(1<<1)
#define HPD_OFF                     	(0)


#endif /* __ASM_ARCH_REGS_HDMI_H */


