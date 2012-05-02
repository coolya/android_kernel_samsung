/* linux/drivers/media/video/samsung/tv20/s5pc100/regs/regs-sdout.h
 *
 * TV Encoder register header file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_SDAOUT_H

#include <mach/map.h>

#define S5P_SDAOUT_BASE(x) (x)
/*
 Registers
*/
#define	S5P_SDO_CLKCON	S5P_SDAOUT_BASE(0x0000)	//  Clock Control Register 0x0000_0000
#define	S5P_SDO_CONFIG	S5P_SDAOUT_BASE(0x0008)	//  Video Standard Configuration Register 0x0024_2430
#define	S5P_SDO_SCALE	S5P_SDAOUT_BASE(0x000C)	//  Video Scale Configuration Register 0x0000_0006
#define	S5P_SDO_SYNC	S5P_SDAOUT_BASE(0x0010)	//  Video Sync Configuration Register 0x0000_0001
#define	S5P_SDO_VBI	S5P_SDAOUT_BASE(0x0014)	//  VBI Configuration Register 0x0007_77FF
#define	S5P_SDO_SCALE_CH0	S5P_SDAOUT_BASE(0x001C)	//  Scale Control Register for DAC Channel0 0x0000_0800
#define	S5P_SDO_SCALE_CH1	S5P_SDAOUT_BASE(0x0020)	//  Scale Control Register for DAC Channel1 0x0000_0800
#define	S5P_SDO_SCALE_CH2	S5P_SDAOUT_BASE(0x0024)	//  Scale Control Register for DAC Channel2 0x0000_0800
#define	S5P_SDO_YCDELAY	S5P_SDAOUT_BASE(0x0034)	//  Video Delay Control Register 0x0000_FA00
#define	S5P_SDO_SCHLOCK	S5P_SDAOUT_BASE(0x0038)	//  SCH Phase Control Register 0x0000_0000
#define	S5P_SDO_DAC	S5P_SDAOUT_BASE(0x003C)	//  DAC Configuration Register 0x0000_0000
#define	S5P_SDO_FINFO	S5P_SDAOUT_BASE(0x0040)	// Status Register 0x0000_0002
#define	S5P_SDO_Y0	S5P_SDAOUT_BASE(0x0044)	//  Y- AAF 1¡¯st and 23¡¯th Coefficient (AAF : Anti-Aliasing Filter) 0x0000_0000
#define	S5P_SDO_Y1	S5P_SDAOUT_BASE(0x0048)	//  Y- AAF 2¡¯nd and 22¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y2	S5P_SDAOUT_BASE(0x004C)	//  Y- AAF 3¡¯rd and 21¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y3	S5P_SDAOUT_BASE(0x0050)	//  Y- AAF 4¡¯th and 20¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y4	S5P_SDAOUT_BASE(0x0054)	//  Y- AAF 5¡¯th and 19¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y5	S5P_SDAOUT_BASE(0x0058)	//  Y- AAF 6¡¯th and 18¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y6	S5P_SDAOUT_BASE(0x005C)	//  Y- AAF 7¡¯th and 17¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y7	S5P_SDAOUT_BASE(0x0060)	//  Y- AAF 8¡¯th and 16¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y8	S5P_SDAOUT_BASE(0x0064)	//  Y - AAF 9¡¯th and 15¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y9	S5P_SDAOUT_BASE(0x0068)	//  Y- AAF 10¡¯th and 14¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y10	S5P_SDAOUT_BASE(0x006C)	//  Y- AAF 11¡¯th and 13¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_Y11	S5P_SDAOUT_BASE(0x0070)	//  Y- AAF 12¡¯th Coefficient 0x0000_025D
#define	S5P_SDO_CB0	S5P_SDAOUT_BASE(0x0080)	//  CB- AAF 1¡¯st and 23¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CB1	S5P_SDAOUT_BASE(0x0084)	//  CB- AAF 2¡¯nd and 22¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CB2	S5P_SDAOUT_BASE(0x0088)	//  CB- AAF 3¡¯rd and 21¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CB3	S5P_SDAOUT_BASE(0x008C)	//  CB-AAF 4¡¯th and 20¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CB4	S5P_SDAOUT_BASE(0x0090)	//  CB- AAF 5¡¯th and 19¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CB5	S5P_SDAOUT_BASE(0x0094)	//  CB- AAF 6¡¯th and 18¡¯th Coefficient 0x0000_0001
#define	S5P_SDO_CB6	S5P_SDAOUT_BASE(0x0098)	//  CB- AAF 7¡¯th and 17¡¯th Coefficient 0x0000_0007
#define	S5P_SDO_CB7	S5P_SDAOUT_BASE(0x009C)	//  CB- AAF 8¡¯th and 16¡¯th Coefficient 0x0000_0014
#define	S5P_SDO_CB8	S5P_SDAOUT_BASE(0x00A0)	//  CB- AAF 9¡¯th and 15¡¯th Coefficient 0x0000_0028
#define	S5P_SDO_CB9	S5P_SDAOUT_BASE(0x00A4)	//  CB- AAF 10¡¯th and 14¡¯th Coefficient 0x0000_003F
#define	S5P_SDO_CB10	S5P_SDAOUT_BASE(0x00A8)	//  CB- AAF 11¡¯th and 13¡¯th Coefficient 0x0000_0052
#define	S5P_SDO_CB11	S5P_SDAOUT_BASE(0x00AC)	//  CB- AAF 12¡¯th Coefficient 0x0000_005A
#define	S5P_SDO_CR0	S5P_SDAOUT_BASE(0x00C0)	//  CR- AAF 1¡¯st and 23¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CR1	S5P_SDAOUT_BASE(0x00C4)	//  CR- AAF 2¡¯nd and 22¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CR2	S5P_SDAOUT_BASE(0x00C8)	//  CR- AAF 3¡¯rd and 21¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CR3	S5P_SDAOUT_BASE(0x00CC)	//  CR-AAF 4¡¯th and 20¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CR4	S5P_SDAOUT_BASE(0x00D0)	//  CR- AAF 5¡¯th and 19¡¯th Coefficient 0x0000_0000
#define	S5P_SDO_CR5	S5P_SDAOUT_BASE(0x00D4)	//  CR- AAF 6¡¯th and 18¡¯th Coefficient 0x0000_0001
#define	S5P_SDO_CR6	S5P_SDAOUT_BASE(0x00D8)	//  CR- AAF 7¡¯th and 17¡¯th Coefficient 0x0000_0009
#define	S5P_SDO_CR7	S5P_SDAOUT_BASE(0x00DC)	//  CR- AAF 8¡¯th and 16¡¯th Coefficient 0x0000_001C
#define	S5P_SDO_CR8	S5P_SDAOUT_BASE(0x00E0)	//  CR- AAF 9¡¯th and 15¡¯th Coefficient 0x0000_0039
#define	S5P_SDO_CR9	S5P_SDAOUT_BASE(0x00E4)	//  CR- AAF 10¡¯th and 14¡¯th Coefficient 0x0000_005A
#define	S5P_SDO_CR10	S5P_SDAOUT_BASE(0x00E8)	//  CR- AAF 11¡¯th and 13¡¯th Coefficient 0x0000_0074
#define	S5P_SDO_CR11	S5P_SDAOUT_BASE(0x00EC)	//  CR- AAF 12¡¯th Coefficient 0x0000_007E
#define	S5P_SDO_MV_ON				S5P_SDAOUT_BASE(0x0100)
#define	S5P_SDO_MV_SLINE_FIRST_EVEN		S5P_SDAOUT_BASE(0x0104)
#define	S5P_SDO_MV_SLINE_FIRST_SPACE_EVEN	S5P_SDAOUT_BASE(0x0108)
#define	S5P_SDO_MV_SLINE_FIRST_ODD		S5P_SDAOUT_BASE(0x010C)
#define	S5P_SDO_MV_SLINE_FIRST_SPACE_ODD	S5P_SDAOUT_BASE(0x0110)
#define	S5P_SDO_MV_SLINE_SPACING		S5P_SDAOUT_BASE(0x0114)
#define	S5P_SDO_MV_STRIPES_NUMBER		S5P_SDAOUT_BASE(0x0118)
#define	S5P_SDO_MV_STRIPES_THICKNESS		S5P_SDAOUT_BASE(0x011C)
#define	S5P_SDO_MV_PSP_DURATION			S5P_SDAOUT_BASE(0x0120)
#define	S5P_SDO_MV_PSP_FIRST			S5P_SDAOUT_BASE(0x0124)
#define	S5P_SDO_MV_PSP_SPACING			S5P_SDAOUT_BASE(0x0128)
#define	S5P_SDO_MV_SEL_LINE_PSP_AGC		S5P_SDAOUT_BASE(0x012C)
#define	S5P_SDO_MV_SEL_FORMAT_PSP_AGC		S5P_SDAOUT_BASE(0x0130)
#define	S5P_SDO_MV_PSP_AGC_A_ON			S5P_SDAOUT_BASE(0x0134)
#define	S5P_SDO_MV_PSP_AGC_B_ON			S5P_SDAOUT_BASE(0x0138)
#define	S5P_SDO_MV_BACK_PORCH			S5P_SDAOUT_BASE(0x013C)
#define	S5P_SDO_MV_BURST_ADVANCED_ON		S5P_SDAOUT_BASE(0x0140)
#define	S5P_SDO_MV_BURST_DURATION_ZONE1		S5P_SDAOUT_BASE(0x0144)
#define	S5P_SDO_MV_BURST_DURATION_ZONE2		S5P_SDAOUT_BASE(0x0148)
#define	S5P_SDO_MV_BURST_DURATION_ZONE3		S5P_SDAOUT_BASE(0x014C)
#define	S5P_SDO_MV_BURST_PHASE_ZONE		S5P_SDAOUT_BASE(0x0150)
#define	S5P_SDO_MV_SLICE_PHASE_LINE		S5P_SDAOUT_BASE(0x0154)
#define	S5P_SDO_MV_RGB_PROTECTION_ON		S5P_SDAOUT_BASE(0x0158)
#define	S5P_SDO_MV_480P_PROTECTION_ON		S5P_SDAOUT_BASE(0x015C)
#define	S5P_SDO_CCCON		S5P_SDAOUT_BASE(0x0180)	//  Color Compensation On/ Off Control 0x0000_0000
#define	S5P_SDO_YSCALE		S5P_SDAOUT_BASE(0x0184)	//  Brightness Control for Y 0x0080_0000
#define	S5P_SDO_CBSCALE		S5P_SDAOUT_BASE(0x0188)	//  Hue/ Saturation Control for CB 0x0080_0000
#define	S5P_SDO_CRSCALE		S5P_SDAOUT_BASE(0x018C)	//  Hue/ Saturation Control for CR 0x0000_0080
#define	S5P_SDO_CB_CR_OFFSET	S5P_SDAOUT_BASE(0x0190)	//  Hue/ Sat Offset Control for CB/CR 0x0000_0000
#define	S5P_SDO_RGB_CC		S5P_SDAOUT_BASE(0x0194)	//  Color Compensation of RGB Output 0x0000_EB10
#define	S5P_SDO_CVBS_CC_Y1	S5P_SDAOUT_BASE(0x0198)	//  Color Compensation of CVBS Output 0x0200_0000
#define	S5P_SDO_CVBS_CC_Y2	S5P_SDAOUT_BASE(0x019C)	//  Color Compensation of CVBS Output 0x03FF_0200
#define	S5P_SDO_CVBS_CC_C	S5P_SDAOUT_BASE(0x01A0)	//  Color Compensation of CVBS Output 0x0000_01FF
#define	S5P_SDO_YC_CC_Y		S5P_SDAOUT_BASE(0x01A4)	//  Color Compensation of S-video Output 0x03FF_0000
#define	S5P_SDO_YC_CC_C		S5P_SDAOUT_BASE(0x01A8)	//  Color Compensation of S-video Output 0x0000_01FF
#define	S5P_SDO_CSC_525_PORCH	S5P_SDAOUT_BASE(0x01B0)	//  Porch Position Control of CSC in 525 Line 0x008A_0359
#define	S5P_SDO_CSC_625_PORCH	S5P_SDAOUT_BASE(0x01B4)	//  Porch Position Control of CSC in 625 Line 0x0096_035C
#define	S5P_SDO_RGBSYNC		S5P_SDAOUT_BASE(0x01C0)	//  VESA RGB Sync Control Register 0x0000_0000
#define	S5P_SDO_OSFC00_0	S5P_SDAOUT_BASE(0x0200)	//  OverSampling Filter (OSF) Coefficient 1 & 0. of channel #0 0x00FD_00FE
#define	S5P_SDO_OSFC01_0	S5P_SDAOUT_BASE(0x0204)	//  OSF Coefficient 3 & 2 of Channel #0 0x0000_0000
#define	S5P_SDO_OSFC02_0	S5P_SDAOUT_BASE(0x0208)	//  OSF Coefficient 5 & 4 of Channel #0 0x0005_0004
#define	S5P_SDO_OSFC03_0	S5P_SDAOUT_BASE(0x020C)	//  OSF Coefficient 7 & 6 of Channel #0 0x0000_00FF
#define	S5P_SDO_OSFC04_0	S5P_SDAOUT_BASE(0x0210)	//  OSF Coefficient 9 & 8 of Channel #0 0x00F7_00FA
#define	S5P_SDO_OSFC05_0	S5P_SDAOUT_BASE(0x0214)	//  OSF Coefficient 11 & 10 of Channel #0 0x0000_0001
#define	S5P_SDO_OSFC06_0	S5P_SDAOUT_BASE(0x0218)	//  OSF Coefficient 13 & 12 of Channel #0 0x000E_000A
#define	S5P_SDO_OSFC07_0	S5P_SDAOUT_BASE(0x021C)	//  OSF Coefficient 15 & 14 of Channel #0 0x0000_01FF
#define	S5P_SDO_OSFC08_0	S5P_SDAOUT_BASE(0x0220)	//  OSF Coefficient 17 & 16 of Channel #0 0x01EC_01F2
#define	S5P_SDO_OSFC09_0	S5P_SDAOUT_BASE(0x0224)	//  OSF Coefficient 19 & 18 of Channel #0 0x0000_0001
#define	S5P_SDO_OSFC10_0	S5P_SDAOUT_BASE(0x0228)	//  OSF Coefficient 21 & 20 of Channel #0 0x001D_0014
#define	S5P_SDO_OSFC11_0	S5P_SDAOUT_BASE(0x022C)	//  OSF Coefficient 23 & 22 of Channel #0 0x0000_01FE
#define	S5P_SDO_OSFC12_0	S5P_SDAOUT_BASE(0x0230)	//  OSF Coefficient 25 & 24 of Channel #0 0x03D8_03E4
#define	S5P_SDO_OSFC13_0	S5P_SDAOUT_BASE(0x0234)	//  OSF Coefficient 27 & 26 of Channel #0 0x0000_0002
#define	S5P_SDO_OSFC14_0	S5P_SDAOUT_BASE(0x0238)	//  OSF Coefficient 29 & 28 of Channel #0 0x0038_0028
#define	S5P_SDO_OSFC15_0	S5P_SDAOUT_BASE(0x023C)	//  OSF Coefficient 31 & 30 of Channel #0 0x0000_03FD
#define	S5P_SDO_OSFC16_0	S5P_SDAOUT_BASE(0x0240)	//  OSF Coefficient 33 & 32 of Channel #0 0x03B0_03C7
#define	S5P_SDO_OSFC17_0	S5P_SDAOUT_BASE(0x0244)	//  OSF Coefficient 35 & 34 of Channel #0 0x0000_0005
#define	S5P_SDO_OSFC18_0	S5P_SDAOUT_BASE(0x0248)	//  OSF Coefficient 37 & 36 of Channel #0 0x0079_0056
#define	S5P_SDO_OSFC19_0	S5P_SDAOUT_BASE(0x024C)	//  OSF Coefficient 39 & 38 of Channel #0 0x0000_03F6
#define	S5P_SDO_OSFC20_0	S5P_SDAOUT_BASE(0x0250)	//  OSF Coefficient 41 & 40 of Channel #0 0x072C_0766
#define	S5P_SDO_OSFC21_0	S5P_SDAOUT_BASE(0x0254)	//  OSF Coefficient 43 & 42 of Channel #0 0x0000_001B
#define	S5P_SDO_OSFC22_0	S5P_SDAOUT_BASE(0x0258)	//  OSF Coefficient 45 & 44 of Channel #0 0x028B_0265
#define	S5P_SDO_OSFC23_0	S5P_SDAOUT_BASE(0x025C)	//  OSF Coefficient 47 & 46 of Channel #0 0x0400_0ECC
#define	S5P_SDO_XTALK0		S5P_SDAOUT_BASE(0x0260)	//  Crosstalk Cancel Coefficient for Ch.0 0x0000_0000
#define	S5P_SDO_XTALK1		S5P_SDAOUT_BASE(0x0264)	//  Crosstalk Cancel Coefficient for Ch.1 0x0000_0000
#define	S5P_SDO_XTALK2		S5P_SDAOUT_BASE(0x0268)	//  Crosstalk Cancel Coefficient for Ch.2 0x0000_0000
#define	S5P_SDO_BB_CTRL		S5P_SDAOUT_BASE(0x026C)	//  Blackburst Test Control 0x0001_1A00
#define	S5P_SDO_IRQ		S5P_SDAOUT_BASE(0x0280)	//  Interrupt Request Register 0x0000_0000
#define	S5P_SDO_IRQMASK		S5P_SDAOUT_BASE(0x0284)	//  Interrupt Request Enable Register 0x0000_0000
#define	S5P_SDO_OSFC00_1	S5P_SDAOUT_BASE(0x02C0)	//  OverSampling Filter (OSF) Coefficient 1 & 0. of Channel #1 0x00FD_00FE
#define	S5P_SDO_OSFC01_1	S5P_SDAOUT_BASE(0x02C4)	//  OSF Coefficient 3 & 2 of Channel #1 0x0000_0000
#define	S5P_SDO_OSFC02_1	S5P_SDAOUT_BASE(0x02C8)	//  OSF Coefficient 5 & 4 of Channel #1 0x0005_0004
#define	S5P_SDO_OSFC03_1	S5P_SDAOUT_BASE(0x02CC)	//  OSF Coefficient 7 & 6 of Channel #1 0x0000_00FF
#define	S5P_SDO_OSFC04_1	S5P_SDAOUT_BASE(0x02D0)	//  OSF Coefficient 9 & 8 of Channel #1 0x00F7_00FA
#define	S5P_SDO_OSFC05_1	S5P_SDAOUT_BASE(0x02D4)	//  OSF Coefficient 11 & 10 of Channel #1 0x0000_0001
#define	S5P_SDO_OSFC06_1	S5P_SDAOUT_BASE(0x02D8)	//  OSF Coefficient 13 & 12 of Channel #1 0x000E_000A
#define	S5P_SDO_OSFC07_1	S5P_SDAOUT_BASE(0x02DC)	//  OSF Coefficient 15 & 14 of Channel #1 0x0000_01FF
#define	S5P_SDO_OSFC08_1	S5P_SDAOUT_BASE(0x02E0)	//  OSF Coefficient 17 & 16 of Channel #1 0x01EC_01F2
#define	S5P_SDO_OSFC09_1	S5P_SDAOUT_BASE(0x02E4)	//  OSF Coefficient 19 & 18 of Channel #1 0x0000_0001
#define	S5P_SDO_OSFC10_1	S5P_SDAOUT_BASE(0x02E8)	//  OSF Coefficient 21 & 20 of Channel #1 0x001D_0014
#define	S5P_SDO_OSFC11_1	S5P_SDAOUT_BASE(0x02EC)	//  OSF Coefficient 23 & 22 of Channel #1 0x0000_01FE
#define	S5P_SDO_OSFC12_1	S5P_SDAOUT_BASE(0x02E0)	//  OSF Coefficient 25 & 24 of Channel #1 0x03D8_03E4
#define	S5P_SDO_OSFC13_1	S5P_SDAOUT_BASE(0x02F4)	//  OSF Coefficient 27 & 26 of Channel #1 0x0000_0002
#define	S5P_SDO_OSFC14_1	S5P_SDAOUT_BASE(0x02F8)	//  OSF Coefficient 29 & 28 of Channel #1 0x0038_0028
#define	S5P_SDO_OSFC15_1	S5P_SDAOUT_BASE(0x02FC)	//  OSF Coefficient 31 & 30 of Channel #1 0x0000_03FD
#define	S5P_SDO_OSFC16_1	S5P_SDAOUT_BASE(0x0300)	//  OSF Coefficient 33 & 32 of Channel #1 0x03B0_03C7
#define	S5P_SDO_OSFC17_1	S5P_SDAOUT_BASE(0x0304)	//  OSF Coefficient 35 & 34 of Channel #1 0x0000_0005
#define	S5P_SDO_OSFC18_1	S5P_SDAOUT_BASE(0x0308)	//  OSF Coefficient 37 & 36 of Channel #1 0x0079_0056
#define	S5P_SDO_OSFC19_1	S5P_SDAOUT_BASE(0x030C)	//  OSF Coefficient 39 & 38 of Channel #1 0x0000_03F6
#define	S5P_SDO_OSFC20_1	S5P_SDAOUT_BASE(0x0310)	//  OSF Coefficient 41 & 40 of Channel #1 0x072C_0766
#define	S5P_SDO_OSFC21_1	S5P_SDAOUT_BASE(0x0314)	//  OSF Coefficient 43 & 42 of Channel #1 0x0000_001B
#define	S5P_SDO_OSFC22_1	S5P_SDAOUT_BASE(0x0318)	//  OSF Coefficient 45 & 44 of Channel #1 0x028B_0265
#define	S5P_SDO_OSFC23_1	S5P_SDAOUT_BASE(0x031C)	//  OSF Coefficient 47 & 46 of Channel #1 0x0400_0ECC
#define	S5P_SDO_OSFC00_2	S5P_SDAOUT_BASE(0x0320)	//  OverSampling Filter (OSF) Coefficient 1 & 0. of Channel #2 0x00FD_00FE
#define	S5P_SDO_OSFC01_2	S5P_SDAOUT_BASE(0x0324)	//  OSF Coefficient 3 & 2 of Channel #2 0x0000_0000
#define	S5P_SDO_OSFC02_2	S5P_SDAOUT_BASE(0x0328)	//  OSF Coefficient 5 & 4 of Channel #2 0x0005_0004
#define	S5P_SDO_OSFC03_2	S5P_SDAOUT_BASE(0x032C)	//  OSF Coefficient 7 & 6 of Channel #2 0x0000_00FF
#define	S5P_SDO_OSFC04_2	S5P_SDAOUT_BASE(0x0330)	//  OSF Coefficient 9 & 8 of Channel #2 0x00F7_00FA
#define	S5P_SDO_OSFC05_2	S5P_SDAOUT_BASE(0x0334)	//  OSF Coefficient 11 & 10 of Channel #2 0x0000_0001
#define	S5P_SDO_OSFC06_2	S5P_SDAOUT_BASE(0x0338)	//  OSF Coefficient 13 & 12 of Channel #2 0x000E_000A
#define	S5P_SDO_OSFC07_2	S5P_SDAOUT_BASE(0x033C)	//  OSF Coefficient 15 & 14 of Channel #2 0x0000_01FF
#define	S5P_SDO_OSFC08_2	S5P_SDAOUT_BASE(0x0340)	//  OSF Coefficient 17 & 16 of Channel #2 0x01EC_01F2
#define	S5P_SDO_OSFC09_2	S5P_SDAOUT_BASE(0x0344)	//  OSF Coefficient 19 & 18 of Channel #2 0x0000_0001
#define	S5P_SDO_OSFC10_2	S5P_SDAOUT_BASE(0x0348)	//  OSF Coefficient 21 & 20 of Channel #2 0x001D_0014
#define	S5P_SDO_OSFC11_2	S5P_SDAOUT_BASE(0x034C)	//  OSF Coefficient 23 & 22 of Channel #2 0x0000_01FE
#define	S5P_SDO_OSFC12_2	S5P_SDAOUT_BASE(0x0350)	//  OSF Coefficient 25 & 24 of Channel #2 0x03D8_03E4
#define	S5P_SDO_OSFC13_2	S5P_SDAOUT_BASE(0x0354)	//  OSF Coefficient 27 & 26 of Channel #2 0x0000_0002
#define	S5P_SDO_OSFC14_2	S5P_SDAOUT_BASE(0x0358)	//  OSF Coefficient 29 & 28 of Channel #2 0x0038_0028
#define	S5P_SDO_OSFC15_2	S5P_SDAOUT_BASE(0x035C)	//  OSF Coefficient 31 & 30 of Channel #2 0x0000_03FD
#define	S5P_SDO_OSFC16_2	S5P_SDAOUT_BASE(0x0360)	//  OSF Coefficient 33 & 32 of Channel #2 0x03B0_03C7
#define	S5P_SDO_OSFC17_2	S5P_SDAOUT_BASE(0x0364)	//  OSF Coefficient 35 & 34 of Channel #2 0x0000_0005
#define	S5P_SDO_OSFC18_2	S5P_SDAOUT_BASE(0x0368)	//  OSF Coefficient 37 & 36 of Channel #2 0x0079_0056
#define	S5P_SDO_OSFC19_2	S5P_SDAOUT_BASE(0x036C)	//  OSF Coefficient 39 & 38 of Channel #2 0x0000_03F6
#define	S5P_SDO_OSFC20_2	S5P_SDAOUT_BASE(0x0370)	//  OSF Coefficient 41 & 40 of Channel #2 0x072C_0766
#define	S5P_SDO_OSFC21_2	S5P_SDAOUT_BASE(0x0374)	//  OSF Coefficient 43 & 42 of Channel #2 0x0000_001B
#define	S5P_SDO_OSFC22_2	S5P_SDAOUT_BASE(0x0378)	//  OSF Coefficient 45 & 44 of Channel #2 0x028B_0265
#define	S5P_SDO_OSFC23_2	S5P_SDAOUT_BASE(0x037C)	//  OSF Coefficient 47 & 46 of Channel #2 0x0400_0ECC
#define	S5P_SDO_ARMCC		S5P_SDAOUT_BASE(0x03C0)	//  Closed Caption Data Register 0x0000_0000
#define	S5P_SDO_ARMWSS525	S5P_SDAOUT_BASE(0x03C4)	//  WSS 525 Data Register 0x0000_0000
#define	S5P_SDO_ARMWSS625	S5P_SDAOUT_BASE(0x03C8)	//  WSS 625 Data Register 0x0000_0000
#define	S5P_SDO_ARMCGMS525	S5P_SDAOUT_BASE(0x03CC)	//  CGMS-A 525 Data Register 0x0000_0000
#define	S5P_SDO_ARMCGMS625	S5P_SDAOUT_BASE(0x03D4)	//  CGMS-A 625 Data Register 0x0000_0000
#define	S5P_SDO_VERSION		S5P_SDAOUT_BASE(0x03D8)	// TVOUT Version Number Read Register 0x0000_000C
#define	S5P_SDO_CC		S5P_SDAOUT_BASE(0x0380)	//  Closed Caption Data Shadow register 0x0000_0000
#define	S5P_SDO_WSS525		S5P_SDAOUT_BASE(0x0384)	//  WSS 525 Data Shadow Register 0x0000_0000
#define	S5P_SDO_WSS625		S5P_SDAOUT_BASE(0x0388)	//  WSS 625 Data Shadow Register 0x0000_0000
#define	S5P_SDO_CGMS525		S5P_SDAOUT_BASE(0x038C)	//  CGMS-A 525 Data Shadow Register 0x0000_0000
#define	S5P_SDO_CGMS625		S5P_SDAOUT_BASE(0x0394)	//  CGMS-A 625 Data Shadow Register 0x0000_0000

/*
 Shadow Registers
*/

/*
 Registers Bit Description
*/
/*
 Macros
*/
/* SDO_CLKCON */
#define SDO_TVOUT_SW_RESET      (1<<4)
#define SDO_TVOUT_CLK_DOWN_RDY	(1<<1)
#define SDO_TVOUT_CLOCK_ON      (1)
#define SDO_TVOUT_CLOCK_OFF     (0)

/* SDO_CONFIG */
#define SDO_DAC2_Y_G  (0<<20)
#define SDO_DAC2_PB_B (1<<20)
#define SDO_DAC2_PR_R (2<<20)
#define SDO_DAC1_Y_G  (0<<18)
#define SDO_DAC1_PB_B (1<<18)
#define SDO_DAC1_PR_R (2<<18)
#define SDO_DAC0_Y_G  (0<<16)
#define SDO_DAC0_PB_B (1<<16)
#define SDO_DAC0_PR_R (2<<16)
#define SDO_DAC2_CVBS   (0<<12)
#define SDO_DAC2_Y      (1<<12)
#define SDO_DAC2_C      (2<<12)
#define SDO_DAC1_CVBS   (0<<10)
#define SDO_DAC1_Y      (1<<10)
#define SDO_DAC1_C      (2<<10)
#define SDO_DAC0_CVBS   (0<<8)
#define SDO_DAC0_Y      (1<<8)
#define SDO_DAC0_C      (2<<8)
#define SDO_COMPOSITE   (0<<6)
#define SDO_COMPONENT   (1<<6)
#define SDO_RGB     (0<<5)
#define SDO_YPBPR   (1<<5)
#define SDO_INTERLACED  (0<<4)
#define SDO_PROGRESSIVE (1<<4)
#define SDO_NTSC_M      (0)
#define SDO_PAL_M       (1)
#define SDO_PAL_BGHID   (2)
#define SDO_PAL_N       (3)
#define SDO_PAL_NC      (4)
#define SDO_NTSC_443    (8)
#define SDO_PAL_60      (9)

/* SDO_SCALE */
#define SDO_COMPONENT_LEVEL_SEL_0IRE    (0<<3)
#define SDO_COMPONENT_LEVEL_SEL_75IRE   (1<<3)
#define SDO_COMPONENT_VTOS_RATIO_10_4   (0<<2)
#define SDO_COMPONENT_VTOS_RATIO_7_3    (1<<2)
#define SDO_COMPOSITE_LEVEL_SEL_0IRE    (0<<1)
#define SDO_COMPOSITE_LEVEL_SEL_75IRE   (1<<1)
#define SDO_COMPOSITE_VTOS_RATIO_10_4   (0<<0)
#define SDO_COMPOSITE_VTOS_RATIO_7_3    (1<<0)

/* SDO_SYNC */
#define SDO_COMPONENT_SYNC_ABSENT   (0)
#define SDO_COMPONENT_SYNC_YG       (1)
#define SDO_COMPONENT_SYNC_ALL      (3)

/* SDO_VBI */
#define SDO_CVBS_NO_WSS     (0<<14)
#define SDO_CVBS_WSS_INS    (1<<14)
#define SDO_CVBS_NO_CLOSED_CAPTION          (0<<12)
#define SDO_CVBS_21H_CLOSED_CAPTION         (1<<12)
#define SDO_CVBS_21H_284H_CLOSED_CAPTION    (2<<12)
#define SDO_CVBS_USE_OTHERS                 (3<<12)
#define SDO_SVIDEO_NO_WSS   (0<<10)
#define SDO_SVIDEO_WSS_INS  (1<<10)
#define SDO_SVIDEO_NO_CLOSED_CAPTION        (0<<8)
#define SDO_SVIDEO_21H_CLOSED_CAPTION       (1<<8)
#define SDO_SVIDEO_21H_284H_CLOSED_CAPTION  (2<<8)
#define SDO_SVIDEO_USE_OTHERS               (3<<8)
#define SDO_RGB_NO_CGMSA    (0<<7)
#define SDO_RGB_CGMSA_INS   (1<<7)
#define SDO_RGB_NO_WSS      (0<<6)
#define SDO_RGB_WSS_INS     (1<<6)
#define SDO_RGB_NO_CLOSED_CAPTION           (0<<4)
#define SDO_RGB_21H_CLOSED_CAPTION          (1<<4)
#define SDO_RGB_21H_284H_CLOSED_CAPTION     (2<<4)
#define SDO_RGB_USE_OTHERS                 (3<<4)
#define SDO_YPBPR_NO_CGMSA  (0<<3)
#define SDO_YPBPR_CGMSA_INS (1<<3)
#define SDO_YPBPR_NO_WSS    (0<<2)
#define SDO_YPBPR_WSS_INS   (1<<2)
#define SDO_YPBPR_NO_CLOSED_CAPTION         (0)
#define SDO_YPBPR_21H_CLOSED_CAPTION        (1)
#define SDO_YPBPR_21H_284H_CLOSED_CAPTION   (2)
#define SDO_YPBPR_USE_OTHERS               (3)

/* SDO_SCALE_CHx */
#define SDO_SCALE_CONV_OFFSET(a)    ((0x3ff&a)<<16)
#define SDO_SCALE_CONV_GAIN(a)      (0xfff&a)

/* SDO_YCDELAY */
#define SDO_DELAY_YTOC(a)           ((0xf&a)<<16)
#define SDO_ACTIVE_START_OFFSET(a)  ((0xff&a)<<8)
#define SDO_ACTIVE_END_OFFSET(a)    (0xff&a)

/* SDO_SCHLOCK */
#define SDO_COLOR_SC_PHASE_ADJ      (1)
#define SDO_COLOR_SC_PHASE_NOADJ    (0)

/* SDO_DAC */
#define SDO_POWER_ON_DAC2   (1<<2)
#define SDO_POWER_DOWN_DAC2 (0<<2)
#define SDO_POWER_ON_DAC1   (1<<1)
#define SDO_POWER_DOWN_DAC1 (0<<1)
#define SDO_POWER_ON_DAC0   (1<<0)
#define SDO_POWER_DOWN_DAC0 (0<<0)

/* SDO_FINFO */
#define SDO_FIELD_MOD_1001(a)               (((0x3ff<<16)&a)>>16)
#define SDO_FIELD_ID_BOTTOM(a)              ((1<<1)&a)
#define SDO_FIELD_ID_BOTTOM_PI_INCATION(a)  (1)
/* SDO_Y0 */
/*
#define SDO_AA_75_73_CB     (0x251)
#define SDO_AA_75_104_CB    (0x25d)
#define SDO_AA_75_73_CB     (0x281)
#define SDO_AA_0_73_CB      (0x28f)
#define SDO_AA_75_73_CR     (0x1f3)
#define SDO_AA_75_104_CR    (0x200)
#define SDO_AA_75_73_CR     (0x21e)
#define SDO_AA_0_73_CR      (0x228)
#define SDO_AA_75_73        (0x2c0)
#define SDO_AA_75_104       (0x2d1)
#define SDO_AA_75_73        (0x2c0)
#define SDO_AA_0_73         (0x30d)
*/
/* SDO_MV_480P_PROTECTION_ON */
#define SDO_MV_AGC_103_ON   (1)

/* SDO_CCCON */
#define SDO_COMPONENT_BHS_ADJ_ON        (0<<4)
#define SDO_COMPONENT_BHS_ADJ_OFF       (1<<4)
#define SDO_COMPONENT_YPBPR_COMP_ON     (0<<3)
#define SDO_COMPONENT_YPBPR_COMP_OFF    (1<<3)
#define SDO_COMPONENT_RGB_COMP_ON       (0<<2)
#define SDO_COMPONENT_RGB_COMP_OFF      (1<<2)
#define SDO_COMPONENT_YC_COMP_ON        (0<<1)
#define SDO_COMPONENT_YC_COMP_OFF       (1<<1)
#define SDO_COMPONENT_CVBS_COMP_ON      (0)
#define SDO_COMPONENT_CVBS_COMP_OFF     (1)

/* SDO_YSCALE */
#define SDO_BRIGHTNESS_GAIN(a)      ((0xff&a)<<16)
#define SDO_BRIGHTNESS_OFFSET(a)    (0xff&a)

/* SDO_CBSCALE */
#define SDO_HS_CB_GAIN0(a)  ((0x1ff&a)<<16)
#define SDO_HS_CB_GAIN1(a)  (0x1ff&a)

/* SDO_CRSCALE */
#define SDO_HS_CR_GAIN0(a)  ((0x1ff&a)<<16)
#define SDO_HS_CR_GAIN1(a)  (0x1ff&a)

/* SDO_CB_CR_OFFSET */
#define SDO_HS_CR_OFFSET(a) ((0x3ff&a)<<16)
#define SDO_HS_CB_OFFSET(a) (0x3ff&a)

/* SDO_RGB_CC */
#define SDO_MAX_RGB_CUBE(a) ((0xff&a)<<8)
#define SDO_MIN_RGB_CUBE(a) (0xff&a)

/* SDO_CVBS_CC_Y1 */
#define SDO_Y_LOWER_MID_CVBS_CORN(a)    ((0x3ff&a)<<16)
#define SDO_Y_BOTTOM_CVBS_CORN(a)       (0x3ff&a)

/* SDO_CVBS_CC_Y2 */
#define SDO_Y_TOP_CVBS_CORN(a)          ((0x3ff&a)<<16)
#define SDO_Y_UPPER_MID_CVBS_CORN(a)    (0x3ff&a)

/* SDO_CVBS_CC_C */
#define SDO_RADIUS_CVBS_CORN(a) (0x1ff&a)

/* SDO_YC_CC_Y */
#define SDO_Y_TOP_YC_CYLINDER(a)    ((0x3ff&a)<<16)
#define SDO_Y_BOTOM_YC_CYLINDER(a)  (0x3ff&a)

/* SDO_CVBS_CC_C */
#define SDO_RADIUS_YC_CYLINDER(a)   (0x1ff&a)

/* SDO_CSC_525_PORCH */
#define SDO_COMPONENT_525_BP(a) ((0x3ff&a)<<16)
#define SDO_COMPONENT_525_FP(a) (0x3ff&a)

/* SDO_CSC_525_PORCH */
#define SDO_COMPONENT_625_BP(a) ((0x3ff&a)<<16)
#define SDO_COMPONENT_625_FP(a) (0x3ff&a)

/* SDO_RGBSYNC */
#define SDO_RGB_SYNC_COMPOSITE  (0<<8)
#define SDO_RGB_SYNC_SEPERATE   (1<<8)
#define SDO_RGB_VSYNC_LOW_ACT   (0<<4)
#define SDO_RGB_VSYNC_HIGH_ACT  (1<<4)
#define SDO_RGB_HSYNC_LOW_ACT   0
#define SDO_RGB_HSYNC_HIGH_ACT  1

/* SDO_OSFCxx_x */
#define SDO_OSF_COEF_ODD(a)     ((0xfff&a)<<16)
#define SDO_OSF_COEF_EVEN(a)    (0xfff&a)

/* SDO_XTALKx */
#define SDO_XTALK_COEF02(a) ((0xff&a)<<16)
#define SDO_XTALK_COEF01(a) (0xff&a)

/* SDO_BB_CTRL */
#define SDO_REF_BB_LEVEL_NTSC   (0x11a<<8)
#define SDO_REF_BB_LEVEL_PAL    (0xfb<<8)
#define SDO_SEL_BB_CJAN_CVBS0_BB1_BB2   (0<<4)
#define SDO_SEL_BB_CJAN_BB0_CVBS1_BB2   (1<<4)
#define SDO_SEL_BB_CJAN_BB0_BB1_CVBS2   (2<<4)
#define SDO_BB_MODE_ENABLE  (1)
#define SDO_BB_MODE_DISABLE (0)

/* SDO_IRQ */
#define SDO_VSYNC_IRQ_PEND  (1)
#define SDO_VSYNC_NO_IRQ    (0)

/* SDO_IRQMASK */
#define SDO_VSYNC_IRQ_ENABLE    (0)
#define SDO_VSYNC_IRQ_DISABLE   (1)

/* SDO_ARMCC */
#define SDO_DISPLAY_CC_CAPTION(a)       ((0xff&a)<<16)
#define SDO_NON_DISPLAY_CC_CAPTION(a)   (0xff&a)

/* SDO_WSS525 */
#define SDO_CRC_WSS525(a)   ((0x3f&a)<<14)
#define SDO_WORD2_WSS525_COPY_PERMIT    (0<<6)
#define SDO_WORD2_WSS525_ONECOPY_PERMIT (1<<6)
#define SDO_WORD2_WSS525_NOCOPY_PERMIT  (3<<6)
#define SDO_WORD2_WSS525_MV_PSP_OFF             (0<<8)
#define SDO_WORD2_WSS525_MV_PSP_ON_2LINE_BURST  (1<<8)
#define SDO_WORD2_WSS525_MV_PSP_ON_BURST_OFF    (2<<8)
#define SDO_WORD2_WSS525_MV_PSP_ON_4LINE_BURST  (3<<8)
#define SDO_WORD2_WSS525_ANALOG_OFF             (0<<10)
#define SDO_WORD2_WSS525_ANALOG_ON              (1<<10)
#define SDO_WORD1_WSS525_COPY_INFO      (0<<2)
#define SDO_WORD1_WSS525_DEFAULT        (0xf<<2)
#define SDO_WORD0_WSS525_4_3_NORMAL     (0)
#define SDO_WORD0_WSS525_16_9_ANAMORPIC (1)
#define SDO_WORD0_WSS525_4_3_LETTERBOX  (2)

/* SDO_WSS625 */
#define SDO_WSS625_SURROUND_SOUND_DISABLE   (0<<11)
#define SDO_WSS625_SURROUND_SOUND_ENABLE    (1<<11)
#define SDO_WSS625_NO_COPYRIGHT (0<<12)
#define SDO_WSS625_COPYRIGHT    (1<<12)
#define SDO_WSS625_COPY_NOT_RESTRICTED  (0<<13)
#define SDO_WSS625_COPY_RESTRICTED      (1<<13)
#define SDO_WSS625_TELETEXT_NO_SUBTITLES    (0<<8)
#define SDO_WSS625_TELETEXT_SUBTITLES       (1<<8)
#define SDO_WSS625_NO_OPEN_SUBTITLES        (0<<9)
#define SDO_WSS625_INACT_OPEN_SUBTITLES     (1<<9)
#define SDO_WSS625_OUTACT_OPEN_SUBTITLES    (2<<9)
#define SDO_WSS625_CAMERA   (0<<4)
#define SDO_WSS625_FILM     (1<<4)
#define SDO_WSS625_NORMAL_PAL                   (0<<5)
#define SDO_WSS625_MOTION_ADAPTIVE_COLORPLUS    (1<<5)
#define SDO_WSS625_HELPER_NO_SIG    (0<<6)
#define SDO_WSS625_HELPER_SIG       (1<<6)
#define SDO_WSS625_4_3_FULL_576                 (0x8)
#define SDO_WSS625_14_9_LETTERBOX_CENTER_504    (0x1)
#define SDO_WSS625_14_9_LETTERBOX_TOP_504       (0x2)
#define SDO_WSS625_16_9_LETTERBOX_CENTER_430    (0xb)
#define SDO_WSS625_16_9_LETTERBOX_TOP_430       (0x4)
#define SDO_WSS625_16_9_LETTERBOX_CENTER        (0xd)
#define SDO_WSS625_14_9_FULL_CENTER_576         (0xe)
#define SDO_WSS625_16_9_ANAMORPIC_576           (0x7)

/* SDO_CGMS525 */
#define SDO_CRC_CGMS525(a)  ((0x3f&a)<<14)
#define SDO_WORD2_CGMS525_COPY_PERMIT       (0<<6)
#define SDO_WORD2_CGMS525_ONECOPY_PERMIT    (1<<6)
#define SDO_WORD2_CGMS525_NOCOPY_PERMIT     (3<<6)
#define SDO_WORD2_CGMS525_MV_PSP_OFF            (0<<8)
#define SDO_WORD2_CGMS525_MV_PSP_ON_2LINE_BURST (1<<8)
#define SDO_WORD2_CGMS525_MV_PSP_ON_BURST_OFF   (2<<8)
#define SDO_WORD2_CGMS525_MV_PSP_ON_4LINE_BURST (3<<8)
#define SDO_WORD2_CGMS525_ANALOG_OFF            (0<<10)
#define SDO_WORD2_CGMS525_ANALOG_ON             (1<<10)
#define SDO_WORD1_CGMS525_COPY_INFO         (0<<2)
#define SDO_WORD1_CGMS525_DEFAULT           (0xf<<2)
#define SDO_WORD0_CGMS525_4_3_NORMAL        (0)
#define SDO_WORD0_CGMS525_16_9_ANAMORPIC    (1)
#define SDO_WORD0_CGMS525_4_3_LETTERBOX     (2)

/* SDO_CGMS625 */
#define SDO_CGMS625_SURROUND_SOUND_DISABLE  (0<<11)
#define SDO_CGMS625_SURROUND_SOUND_ENABLE   (1<<11)
#define SDO_CGMS625_NO_COPYRIGHT    (0<<12)
#define SDO_CGMS625_COPYRIGHT       (1<<12)
#define SDO_CGMS625_COPY_NOT_RESTRICTED (0<<13)
#define SDO_CGMS625_COPY_RESTRICTED     (1<<13)
#define SDO_CGMS625_TELETEXT_NO_SUBTITLES   (0<<8)
#define SDO_CGMS625_TELETEXT_SUBTITLES      (1<<8)
#define SDO_CGMS625_NO_OPEN_SUBTITLES       (0<<9)
#define SDO_CGMS625_INACT_OPEN_SUBTITLES    (1<<9)
#define SDO_CGMS625_OUTACT_OPEN_SUBTITLES   (2<<9)
#define SDO_CGMS625_CAMERA  (0<<4)
#define SDO_CGMS625_FILM    (1<<4)
#define SDO_CGMS625_NORMAL_PAL                  (0<<5)
#define SDO_CGMS625_MOTION_ADAPTIVE_COLORPLUS   (1<<5)
#define SDO_CGMS625_HELPER_NO_SIG   (0<<6)
#define SDO_CGMS625_HELPER_SIG      (1<<6)
#define SDO_CGMS625_4_3_FULL_576                (0x8)
#define SDO_CGMS625_14_9_LETTERBOX_CENTER_504   (0x1)
#define SDO_CGMS625_14_9_LETTERBOX_TOP_504      (0x2)
#define SDO_CGMS625_16_9_LETTERBOX_CENTER_430   (0xb)
#define SDO_CGMS625_16_9_LETTERBOX_TOP_430      (0x4)
#define SDO_CGMS625_16_9_LETTERBOX_CENTER       (0xd)
#define SDO_CGMS625_14_9_FULL_CENTER_576        (0xe)
#define SDO_CGMS625_16_9_ANAMORPIC_576          (0x7)

#endif /*__ASM_ARCH_REGS_SDAOUT_H */
