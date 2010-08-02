/* linux/arch/arm/plat-s5p/include/plat/regs-fimc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Register definition file for Samsung Camera Interface (FIMC) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_PLAT_REGS_FIMC_H
#define __ASM_PLAT_REGS_FIMC_H __FILE__

/*
 * Register part
*/ 
#define S3C_CIOYSA(__x) 	(0x18 + (__x) * 4)
#define S3C_CIOCBSA(__x) 	(0x28 + (__x) * 4)
#define S3C_CIOCRSA(__x)  	(0x38 + (__x) * 4)

#define S3C_CISRCFMT		(0x00)	/* Input source format */
#define S3C_CIWDOFST		(0x04)	/* Window offset */
#define S3C_CIGCTRL			(0x08)	/* Global control */
#define S3C_CIWDOFST2		(0x14)	/* Window offset 2 */
#define S3C_CIOYSA1			(0x18)	/* Y 1st frame start address for output DMA */
#define S3C_CIOYSA2			(0x1c)	/* Y 2nd frame start address for output DMA */
#define S3C_CIOYSA3			(0x20)	/* Y 3rd frame start address for output DMA */
#define S3C_CIOYSA4			(0x24)	/* Y 4th frame start address for output DMA */
#define S3C_CIOCBSA1		(0x28)	/* Cb 1st frame start address for output DMA */
#define S3C_CIOCBSA2		(0x2c)	/* Cb 2nd frame start address for output DMA */
#define S3C_CIOCBSA3		(0x30)	/* Cb 3rd frame start address for output DMA */
#define S3C_CIOCBSA4		(0x34)	/* Cb 4th frame start address for output DMA */
#define S3C_CIOCRSA1		(0x38)	/* Cr 1st frame start address for output DMA */
#define S3C_CIOCRSA2		(0x3c)	/* Cr 2nd frame start address for output DMA */
#define S3C_CIOCRSA3		(0x40)	/* Cr 3rd frame start address for output DMA */
#define S3C_CIOCRSA4		(0x44)	/* Cr 4th frame start address for output DMA */
#define S3C_CITRGFMT		(0x48)	/* Target image format */
#define S3C_CIOCTRL			(0x4c)	/* Output DMA control */
#define S3C_CISCPRERATIO	(0x50)	/* Pre-scaler control 1 */
#define S3C_CISCPREDST		(0x54)	/* Pre-scaler control 2 */
#define S3C_CISCCTRL		(0x58)	/* Main scaler control */
#define S3C_CITAREA			(0x5c)	/* Target area */
#define S3C_CISTATUS		(0x64)	/* Status */
#define S3C_CIIMGCPT		(0xc0)	/* Image capture enable command */
#define S3C_CICPTSEQ		(0xc4)	/* Capture sequence */
#define S3C_CIIMGEFF		(0xd0)	/* Image effects */
#define S3C_CIIYSA0			(0xd4)	/* Y frame start address for input DMA */
#define S3C_CIICBSA0		(0xd8)	/* Cb frame start address for input DMA */
#define S3C_CIICRSA0		(0xdc)	/* Cr frame start address for input DMA */
#define S3C_CIILINESKIP_Y	(0xec)	/* Input DMA Y Line Skip */
#define S3C_CIILINESKIP_CB	(0xf0)	/* Input DMA Cb Line Skip */
#define S3C_CIILINESKIP_CR	(0xf4)	/* Input DMA Cr Line Skip */
#define S3C_CIREAL_ISIZE	(0xf8)	/* Real input DMA image size */
#define S3C_MSCTRL			(0xfc)	/* Input DMA control */
#define S3C_CIOYOFF			(0x168)	/* Output DMA Y offset */
#define S3C_CIOCBOFF		(0x16c)	/* Output DMA CB offset */
#define S3C_CIOCROFF		(0x170)	/* Output DMA CR offset */
#define S3C_CIIYOFF			(0x174)	/* Input DMA Y offset */
#define S3C_CIICBOFF		(0x178)	/* Input DMA CB offset */
#define S3C_CIICROFF		(0x17c)	/* Input DMA CR offset */
#define S3C_ORGISIZE		(0x180)	/* Input DMA original image size */
#define S3C_ORGOSIZE		(0x184)	/* Output DMA original image size */
#define S3C_CIEXTEN			(0x188)	/* Real output DMA image size */
#define S3C_CIDMAPARAM		(0x18c)	/* DMA parameter */
#define S3C_CSIIMGFMT		(0x194)	/* MIPI CSI image format */
#define S3C_MISC_FIMC		(0x198)	/* FIMC Clock Source Select */

/*
 * Macro part
*/
#define S3C_CISRCFMT_SOURCEHSIZE(x)			((x) << 16)
#define S3C_CISRCFMT_SOURCEVSIZE(x)			((x) << 0)

#define S3C_CIWDOFST_WINHOROFST(x)	    	((x) << 16)
#define S3C_CIWDOFST_WINVEROFST(x)	    	((x) << 0)

#define S3C_CIWDOFST2_WINHOROFST2(x)		((x) << 16)
#define S3C_CIWDOFST2_WINVEROFST2(x)		((x) << 0)

#define S3C_CITRGFMT_TARGETHSIZE(x)  		(((x) & 0x1fff) << 16)
#define S3C_CITRGFMT_TARGETVSIZE(x)	    	(((x) & 0x1fff) << 0)
 
#define S3C_CISCPRERATIO_SHFACTOR(x)		((x) << 28)
#define S3C_CISCPRERATIO_PREHORRATIO(x)		((x) << 16)
#define S3C_CISCPRERATIO_PREVERRATIO(x)		((x) << 0)

#define S3C_CISCPREDST_PREDSTWIDTH(x)		((x) << 16)
#define S3C_CISCPREDST_PREDSTHEIGHT(x)		((x) << 0)

#define S3C_CISCCTRL_MAINHORRATIO(x)		((x) << 16)
#define S3C_CISCCTRL_MAINVERRATIO(x)		((x) << 0)

#define S3C_CITAREA_TARGET_AREA(x)	    	((x) << 0)

#define S3C_CISTATUS_GET_FRAME_COUNT(x)	    	(((x) >> 26) & 0x3)
#define S3C_CISTATUS_GET_FRAME_END(x)	    	(((x) >> 17) & 0x1)
#define S3C_CISTATUS_GET_LAST_CAPTURE_END(x)	(((x) >> 16) & 0x1)
#define S3C_CISTATUS_GET_LCD_STATUS(x)	    	(((x) >> 9) & 0x1)
#define S3C_CISTATUS_GET_ENVID_STATUS(x)    	((x) & 0x1)

#define S3C_CIIMGEFF_FIN(x)					((x & 0x7) << 26)
#define S3C_CIIMGEFF_PAT_CB(x)		    	((x) << 13)
#define S3C_CIIMGEFF_PAT_CR(x)	    		((x) << 0)

#define S3C_CIILINESKIP(x)		    	    (((x) & 0xf) << 24)

#define S3C_CIREAL_ISIZE_HEIGHT(x)	    	((x) << 16)
#define S3C_CIREAL_ISIZE_WIDTH(x)	    	((x) << 0)

#define S3C_MSCTRL_SUCCESSIVE_COUNT(x)		((x) << 24)
#define S3C_MSCTRL_GET_INDMA_STATUS(x)		((x) & 0x1)

#define S3C_CIOYOFF_VERTICAL(x)	    		((x) << 16)
#define S3C_CIOYOFF_HORIZONTAL(x)	    	((x) << 0)

#define S3C_CIOCBOFF_VERTICAL(x)    		((x) << 16)
#define S3C_CIOCBOFF_HORIZONTAL(x)	    	((x) << 0)

#define S3C_CIOCROFF_VERTICAL(x)	    	((x) << 16)
#define S3C_CIOCROFF_HORIZONTAL(x)	    	((x) << 0)

#define S3C_CIIYOFF_VERTICAL(x)		    	((x) << 16)
#define S3C_CIIYOFF_HORIZONTAL(x)	    	((x) << 0)

#define S3C_CIICBOFF_VERTICAL(x)	    	((x) << 16)
#define S3C_CIICBOFF_HORIZONTAL(x)	    	((x) << 0)

#define S3C_CIICROFF_VERTICAL(x)	    	((x) << 16)
#define S3C_CIICROFF_HORIZONTAL(x)	    	((x) << 0)

#define S3C_ORGISIZE_VERTICAL(x)	    	((x) << 16)
#define S3C_ORGISIZE_HORIZONTAL(x)	    	((x) << 0)

#define S3C_ORGOSIZE_VERTICAL(x)	    	((x) << 16)
#define S3C_ORGOSIZE_HORIZONTAL(x)	    	((x) << 0)

#define S3C_CIEXTEN_TARGETH_EXT(x)	    	(((x) & 0x2000) << 26)
#define S3C_CIEXTEN_TARGETV_EXT(x)	    	(((x) & 0x2000) << 24)
#define S3C_CIEXTEN_MAINHORRATIO_EXT(x)		(((x) & 0x3F) << 10)
#define S3C_CIEXTEN_MAINVERRATIO_EXT(x)		((x) & 0x3F)

/*
 * Bit definition part
*/
/* Source format register */
#define S3C_CISRCFMT_ITU601_8BIT	    	(1 << 31)
#define S3C_CISRCFMT_ITU656_8BIT	    	(0 << 31)
#define S3C_CISRCFMT_ITU601_16BIT	     	(1 << 29)
#define S3C_CISRCFMT_ORDER422_YCBYCR		(0 << 14)
#define S3C_CISRCFMT_ORDER422_YCRYCB		(1 << 14)
#define S3C_CISRCFMT_ORDER422_CBYCRY		(2 << 14)
#define S3C_CISRCFMT_ORDER422_CRYCBY		(3 << 14)
#define S3C_CISRCFMT_ORDER422_Y4CBCRCBCR	(0 << 14)	/* ITU601 16bit only */
#define S3C_CISRCFMT_ORDER422_Y4CRCBCRCB	(1 << 14)	/* ITU601 16bit only */

/* Window offset register */
#define S3C_CIWDOFST_WINOFSEN		    	(1 << 31)
#define S3C_CIWDOFST_CLROVFIY		    	(1 << 30)
#define S3C_CIWDOFST_CLROVRLB		    	(1 << 29)
#define S3C_CIWDOFST_WINHOROFST_MASK		(0x7ff << 16)
#define S3C_CIWDOFST_CLROVFICB	    		(1 << 15)
#define S3C_CIWDOFST_CLROVFICR		    	(1 << 14)
#define S3C_CIWDOFST_WINVEROFST_MASK		(0xfff << 0)

/* Global control register */
#define S3C_CIGCTRL_SWRST	       		(1 << 31)
#define S3C_CIGCTRL_CAMRST_A		    	(1 << 30)
#define S3C_CIGCTRL_SELCAM_ITU_B	    	(0 << 29)
#define S3C_CIGCTRL_SELCAM_ITU_A	    	(1 << 29)
#define S3C_CIGCTRL_SELCAM_ITU_MASK	    	(1 << 29)
#define S3C_CIGCTRL_TESTPATTERN_NORMAL		(0 << 27)
#define S3C_CIGCTRL_TESTPATTERN_COLOR_BAR	(1 << 27)
#define S3C_CIGCTRL_TESTPATTERN_HOR_INC		(2 << 27)
#define S3C_CIGCTRL_TESTPATTERN_VER_INC		(3 << 27)
#define S3C_CIGCTRL_TESTPATTERN_MASK		(3 << 27)
#define S3C_CIGCTRL_TESTPATTERN_SHIFT		(27)
#define S3C_CIGCTRL_INVPOLPCLK		    	(1 << 26)
#define S3C_CIGCTRL_INVPOLVSYNC		    	(1 << 25)
#define S3C_CIGCTRL_INVPOLHREF		    	(1 << 24)
#define S3C_CIGCTRL_IRQ_OVFEN		    	(1 << 22)
#define S3C_CIGCTRL_HREF_MASK		    	(1 << 21)
#define S3C_CIGCTRL_IRQ_EDGE		    	(0 << 20)
#define S3C_CIGCTRL_IRQ_LEVEL		    	(1 << 20)
#define S3C_CIGCTRL_IRQ_CLR		        	(1 << 19)
#define S3C_CIGCTRL_IRQ_DISABLE		    	(0 << 16)
#define S3C_CIGCTRL_IRQ_ENABLE		    	(1 << 16)
#define S3C_CIGCTRL_SHADOW_DISABLE			(1 << 12)
#define	S3C_CIGCTRL_CAM_JPEG				(1 << 8) 
#define S3C_CIGCTRL_SELCAM_MIPI_B	    	(0 << 7)
#define S3C_CIGCTRL_SELCAM_MIPI_A	    	(1 << 7)
#define S3C_CIGCTRL_SELCAM_MIPI_MASK		(1 << 7)
#define S3C_CIGCTRL_SELWB_CAMIF_CAMERA  	(0 << 6)
#define S3C_CIGCTRL_SELWB_CAMIF_WRITEBACK 	(1 << 6)
#define S3C_CIGCTRL_SELWB_CAMIF_MASK  		(1 << 6)
#define S3C_CIGCTRL_CSC_ITU601		    	(0 << 5)
#define S3C_CIGCTRL_CSC_ITU709		    	(1 << 5)
#define S3C_CIGCTRL_CSC_MASK		    	(1 << 5)
#define S3C_CIGCTRL_INVPOLHSYNC		    	(1 << 4)
#define S3C_CIGCTRL_SELCAM_FIMC_ITU	    	(0 << 3)
#define S3C_CIGCTRL_SELCAM_FIMC_MIPI		(1 << 3)
#define S3C_CIGCTRL_SELCAM_FIMC_MASK		(1 << 3)
#define S3C_CIGCTRL_PROGRESSIVE		    	(0 << 0)
#define S3C_CIGCTRL_INTERLACE		    	(1 << 0)

/* Window offset2 register */
#define S3C_CIWDOFST_WINHOROFST2_MASK		(0xfff << 16)
#define S3C_CIWDOFST_WINVEROFST2_MASK		(0xfff << 16)

/* Target format register */
#define S3C_CITRGFMT_INROT90_CLOCKWISE		(1 << 31)
#define S3C_CITRGFMT_OUTFORMAT_YCBCR420		(0 << 29)
#define S3C_CITRGFMT_OUTFORMAT_YCBCR422		(1 << 29)
#define S3C_CITRGFMT_OUTFORMAT_YCBCR422_1PLANE	(2 << 29)
#define S3C_CITRGFMT_OUTFORMAT_RGB	    	(3 << 29)
#define S3C_CITRGFMT_OUTFORMAT_MASK	    	(3 << 29)
#define S3C_CITRGFMT_FLIP_SHIFT		    	(14)
#define S3C_CITRGFMT_FLIP_NORMAL	    	(0 << 14)
#define S3C_CITRGFMT_FLIP_X_MIRROR	    	(1 << 14)
#define S3C_CITRGFMT_FLIP_Y_MIRROR	    	(2 << 14)
#define S3C_CITRGFMT_FLIP_180		    	(3 << 14)
#define S3C_CITRGFMT_FLIP_MASK		    	(3 << 14)
#define S3C_CITRGFMT_OUTROT90_CLOCKWISE		(1 << 13)
#define S3C_CITRGFMT_TARGETV_MASK	    	(0x1fff << 0)
#define S3C_CITRGFMT_TARGETH_MASK	    	(0x1fff << 16)

/* Output DMA control register */
#define S3C_CIOCTRL_WEAVE_OUT	    		(1 << 31)
#define S3C_CIOCTRL_WEAVE_MASK		    	(1 << 31)
#define S3C_CIOCTRL_ORDER2P_LSB_CBCR		(0 << 24)
#define S3C_CIOCTRL_ORDER2P_LSB_CRCB		(1 << 24)
#define S3C_CIOCTRL_ORDER2P_MSB_CRCB		(2 << 24)
#define S3C_CIOCTRL_ORDER2P_MSB_CBCR		(3 << 24)
#define S3C_CIOCTRL_ORDER2P_SHIFT	    	(24)
#define S3C_CIOCTRL_ORDER2P_MASK	    	(3 << 24)
#define S3C_CIOCTRL_YCBCR_3PLANE	    	(0 << 3)
#define S3C_CIOCTRL_YCBCR_2PLANE	    	(1 << 3)
#define S3C_CIOCTRL_YCBCR_PLANE_MASK		(1 << 3)
#define S3C_CIOCTRL_LASTIRQ_ENABLE	    	(1 << 2)
#define S3C_CIOCTRL_ORDER422_YCBYCR	    	(0 << 0)
#define S3C_CIOCTRL_ORDER422_YCRYCB	    	(1 << 0)
#define S3C_CIOCTRL_ORDER422_CBYCRY	    	(2 << 0)
#define S3C_CIOCTRL_ORDER422_CRYCBY	    	(3 << 0)
#define S3C_CIOCTRL_ORDER422_MASK	     	(3 << 0)

/* Main scaler control register */
#define S3C_CISCCTRL_SCALERBYPASS		    (1 << 31)
#define S3C_CISCCTRL_SCALEUP_H		    	(1 << 30)
#define S3C_CISCCTRL_SCALEUP_V		    	(1 << 29)
#define S3C_CISCCTRL_CSCR2Y_NARROW	    	(0 << 28)
#define S3C_CISCCTRL_CSCR2Y_WIDE	    	(1 << 28)
#define S3C_CISCCTRL_CSCY2R_NARROW	    	(0 << 27)
#define S3C_CISCCTRL_CSCY2R_WIDE	    	(1 << 27)
#define S3C_CISCCTRL_LCDPATHEN_FIFO	    	(1 << 26)
#define S3C_CISCCTRL_PROGRESSIVE	    	(0 << 25)
#define S3C_CISCCTRL_INTERLACE		    	(1 << 25)
#define S3C_CISCCTRL_SCAN_MASK		    	(1 << 25)
#define S3C_CISCCTRL_SCALERSTART	    	(1 << 15)
#define S3C_CISCCTRL_INRGB_FMT_RGB565		(0 << 13)
#define S3C_CISCCTRL_INRGB_FMT_RGB666		(1 << 13)
#define S3C_CISCCTRL_INRGB_FMT_RGB888		(2 << 13)
#define S3C_CISCCTRL_INRGB_FMT_RGB_MASK		(3 << 13)
#define S3C_CISCCTRL_OUTRGB_FMT_RGB565		(0 << 11)
#define S3C_CISCCTRL_OUTRGB_FMT_RGB666		(1 << 11)
#define S3C_CISCCTRL_OUTRGB_FMT_RGB888		(2 << 11)
#define S3C_CISCCTRL_OUTRGB_FMT_RGB_MASK	(3 << 11)
#define S3C_CISCCTRL_EXTRGB_NORMAL   		(0 << 10)
#define S3C_CISCCTRL_EXTRGB_EXTENSION		(1 << 10)
#define S3C_CISCCTRL_ONE2ONE		    	(1 << 9)
#define S3C_CISCCTRL_MAIN_V_RATIO_MASK		(0x1ff << 0)
#define S3C_CISCCTRL_MAIN_H_RATIO_MASK		(0x1ff << 16)

/* Status register */
#define S3C_CISTATUS_OVFIY		        	(1 << 31)
#define S3C_CISTATUS_OVFICB		        	(1 << 30)
#define S3C_CISTATUS_OVFICR	        		(1 << 29)
#define S3C_CISTATUS_VSYNC		    	(1 << 28)
#define S3C_CISTATUS_WINOFSTEN			(1 << 25)
#define S3C_CISTATUS_IMGCPTEN			(1 << 22)
#define S3C_CISTATUS_IMGCPTENSC			(1 << 21)
#define S3C_CISTATUS_VSYNC_A			(1 << 20)
#define S3C_CISTATUS_VSYNC_B			(1 << 19)
#define S3C_CISTATUS_OVRLB			    (1 << 18)
#define S3C_CISTATUS_FRAMEEND			(1 << 17)
#define S3C_CISTATUS_LASTCAPTUREEND		(1 << 16)
#define S3C_CISTATUS_VVALID_A			(1 << 15)
#define S3C_CISTATUS_VVALID_B			(1 << 14)

/* Image capture enable register */
#define S3C_CIIMGCPT_IMGCPTEN			(1 << 31)
#define S3C_CIIMGCPT_IMGCPTEN_SC		(1 << 30)
#define S3C_CIIMGCPT_CPT_FREN_ENABLE		(1 << 25)
#define S3C_CIIMGCPT_CPT_FRMOD_EN		(0 << 18)
#define S3C_CIIMGCPT_CPT_FRMOD_CNT		(1 << 18)

/* Image effects register */
#define S3C_CIIMGEFF_IE_DISABLE			(0 << 30)
#define S3C_CIIMGEFF_IE_ENABLE			(1 << 30)
#define S3C_CIIMGEFF_IE_SC_BEFORE		(0 << 29)
#define S3C_CIIMGEFF_IE_SC_AFTER		(1 << 29)
#define S3C_CIIMGEFF_FIN_BYPASS			(0 << 26)
#define S3C_CIIMGEFF_FIN_ARBITRARY		(1 << 26)
#define S3C_CIIMGEFF_FIN_NEGATIVE		(2 << 26)
#define S3C_CIIMGEFF_FIN_ARTFREEZE		(3 << 26)
#define S3C_CIIMGEFF_FIN_EMBOSSING		(4 << 26)
#define S3C_CIIMGEFF_FIN_SILHOUETTE		(5 << 26)
#define S3C_CIIMGEFF_FIN_MASK			(7 << 26)
#define S3C_CIIMGEFF_PAT_CBCR_MASK		((0xff < 13) | (0xff < 0))

/* Real input DMA size register */
#define S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE	(1 << 31)
#define S3C_CIREAL_ISIZE_ADDR_CH_DISABLE	(1 << 30)
#define S3C_CIREAL_ISIZE_HEIGHT_MASK		(0x3FFF << 16)
#define S3C_CIREAL_ISIZE_WIDTH_MASK		(0x3FFF << 0)

/* Input DMA control register */
#define S3C_MSCTRL_FIELD_MASK			(1 << 31)
#define S3C_MSCTRL_FIELD_WEAVE			(1 << 31)
#define S3C_MSCTRL_FIELD_NORMAL			(0 << 31)
#define S3C_MSCTRL_BURST_CNT			(24)
#define S3C_MSCTRL_BURST_CNT_MASK		(0xf << 24)
#define S3C_MSCTRL_ORDER2P_LSB_CBCR		(0 << 16)
#define S3C_MSCTRL_ORDER2P_LSB_CRCB		(1 << 16)
#define S3C_MSCTRL_ORDER2P_MSB_CRCB		(2 << 16)
#define S3C_MSCTRL_ORDER2P_MSB_CBCR		(3 << 16)
#define S3C_MSCTRL_ORDER2P_SHIFT		(16)
#define S3C_MSCTRL_ORDER2P_SHIFT_MASK		(0x3 << 16)
#define S3C_MSCTRL_C_INT_IN_3PLANE		(0 << 15)
#define S3C_MSCTRL_C_INT_IN_2PLANE		(1 << 15)
#define S3C_MSCTRL_FLIP_SHIFT			(13)
#define S3C_MSCTRL_FLIP_NORMAL			(0 << 13)
#define S3C_MSCTRL_FLIP_X_MIRROR		(1 << 13)
#define S3C_MSCTRL_FLIP_Y_MIRROR		(2 << 13)
#define S3C_MSCTRL_FLIP_180		    	(3 << 13)
#define S3C_MSCTRL_FLIP_MASK			(3 << 13)
#define S3C_MSCTRL_ORDER422_CRYCBY		(0 << 4)
#define S3C_MSCTRL_ORDER422_YCRYCB		(1 << 4)
#define S3C_MSCTRL_ORDER422_CBYCRY		(2 << 4)
#define S3C_MSCTRL_ORDER422_YCBYCR		(3 << 4)
#define S3C_MSCTRL_INPUT_EXTCAM			(0 << 3)
#define S3C_MSCTRL_INPUT_MEMORY			(1 << 3)
#define S3C_MSCTRL_INPUT_MASK			(1 << 3)
#define S3C_MSCTRL_INFORMAT_YCBCR420		(0 << 1)
#define S3C_MSCTRL_INFORMAT_YCBCR422		(1 << 1)
#define S3C_MSCTRL_INFORMAT_YCBCR422_1PLANE	(2 << 1)
#define S3C_MSCTRL_INFORMAT_RGB			(3 << 1)
#define S3C_MSCTRL_ENVID			(1 << 0)

/* DMA parameter register */
#define S3C_CIDMAPARAM_R_MODE_LINEAR		(0 << 29)
#define S3C_CIDMAPARAM_R_MODE_CONFTILE		(1 << 29)
#define S3C_CIDMAPARAM_R_MODE_16X16	    	(2 << 29)
#define S3C_CIDMAPARAM_R_MODE_64X32	    	(3 << 29)
#define S3C_CIDMAPARAM_R_MODE_MASK	    	(3 << 29)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_64		(0 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_128		(1 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_256		(2 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_512		(3 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_1024	(4 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_2048	(5 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_4096	(6 << 24)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_1		(0 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_2		(1 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_4		(2 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_8		(3 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_16		(4 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_32		(5 << 20)
#define S3C_CIDMAPARAM_W_MODE_LINEAR		(0 << 13)
#define S3C_CIDMAPARAM_W_MODE_CONFTILE		(1 << 13)
#define S3C_CIDMAPARAM_W_MODE_16X16  		(2 << 13)
#define S3C_CIDMAPARAM_W_MODE_64X32	    	(3 << 13)
#define S3C_CIDMAPARAM_W_MODE_MASK	    	(3 << 13)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_64		(0 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_128		(1 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_256		(2 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_512		(3 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_1024	(4 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_2048	(5 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_4096	(6 << 8)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_1		(0 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_2		(1 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_4		(2 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_8		(3 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_16		(4 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_32		(5 << 4)

/* Gathering Extension register */
#define S3C_CIEXTEN_TARGETH_EXT_MASK		(1 << 26)
#define S3C_CIEXTEN_TARGETV_EXT_MASK		(1 << 24)
#define S3C_CIEXTEN_MAINHORRATIO_EXT_MASK	(0x3F << 10)
#define S3C_CIEXTEN_MAINVERRATIO_EXT_MASK	(0x3F)
#define S3C_CIEXTEN_YUV444_OUT			    (1 << 22)

/* FIMC Clock Source Select register */
#define S3C_CLKSRC_HCLK		        		(0 << 1)
#define S3C_CLKSRC_HCLK_MASK		    	(1 << 1)
#define S3C_CLKSRC_SCLK			        	(1 << 1)

/* FIMC GPIO */
#define S5PV210_GPE0_0_CAM_A_PCLK           (0x2 << 0)
#define S5PV210_GPE0_0_GPIO_INT8_0          (0xf << 0)

#define S5PV210_GPE0_1_CAM_A_VSYNC          (0x2 << 4)
#define S5PV210_GPE0_1_GPIO_INT8_1          (0xf << 4)

#define S5PV210_GPE0_2_CAM_A_HREF           (0x2 << 8)
#define S5PV210_GPE0_2_GPIO_INT8_2          (0xf << 8)

#define S5PV210_GPE0_3_CAM_A_DATA_0         (0x2 << 12)
#define S5PV210_GPE0_3_GPIO_INT8_3          (0xf << 12)

#define S5PV210_GPE0_4_CAM_A_DATA_1         (0x2 << 16)
#define S5PV210_GPE0_4_GPIO_INT8_4          (0xf << 16)

#define S5PV210_GPE0_5_CAM_A_DATA_2         (0x2 << 20)
#define S5PV210_GPE0_5_GPIO_INT8_5          (0xf << 20)

#define S5PV210_GPE0_6_CAM_A_DATA_3         (0x2 << 24)
#define S5PV210_GPE0_6_GPIO_INT8_6          (0xf << 24)

#define S5PV210_GPE0_7_CAM_A_DATA_4         (0x2 << 28)
#define S5PV210_GPE0_7_GPIO_INT8_7          (0xf << 28)

#define S5PV210_GPE1_0_CAM_A_DATA_5         (0x2 << 0)
#define S5PV210_GPE1_1_CAM_A_DATA_6         (0x2 << 4)
#define S5PV210_GPE1_2_CAM_A_DATA_7         (0x2 << 8)
#define S5PV210_GPE1_3_CAM_A_CLKOUT         (0x2 << 12)
#define S5PV210_GPE1_4_CAM_A_FIELD          (0x2 << 16)

#define S5PV210_GPJ0_0_CAM_B_DATA_0         (0x3 << 0)
#define S5PV210_GPJ0_1_CAM_B_DATA_1         (0x3 << 4)
#define S5PV210_GPJ0_2_CAM_B_DATA_2         (0x3 << 8)
#define S5PV210_GPJ0_3_CAM_B_DATA_3         (0x3 << 12)
#define S5PV210_GPJ0_4_CAM_B_DATA_4         (0x3 << 16)
#define S5PV210_GPJ0_5_CAM_B_DATA_5         (0x3 << 20)
#define S5PV210_GPJ0_6_CAM_B_DATA_6         (0x3 << 24)
#define S5PV210_GPJ0_7_CAM_B_DATA_7         (0x3 << 28)

#define S5PV210_GPJ1_0_CAM_B_PCLK           (0x3 << 0)
#define S5PV210_GPJ1_1_CAM_B_VSYNC          (0x3 << 4)
#define S5PV210_GPJ1_2_CAM_B_HREF           (0x3 << 8)
#define S5PV210_GPJ1_3_CAM_B_FIELD          (0x3 << 12)
#define S5PV210_GPJ1_4_CAM_B_CLKOUT         (0x3 << 16)

#endif /* __ASM_PLAT_REGS_FIMC_H */
