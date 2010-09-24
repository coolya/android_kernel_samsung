/* linux/drivers/media/video/samsung/jpeg/regs-jpeg.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Register definition file for Samsung JPEG Encoder/Decoder
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARM_REGS_S3C_JPEG_H
#define __ASM_ARM_REGS_S3C_JPEG_H

/* JPEG Registers part */
#define S3C_JPEG_REG(x)		((x))

/* Sub-sampling Mode Register */
#define S3C_JPEG_MOD_REG	S3C_JPEG_REG(0x00)
/* Operation Status Register */
#define S3C_JPEG_OPR_REG	S3C_JPEG_REG(0x04)
/* Quantization Table Number Register and Huffman Table Number Register	*/
#define S3C_JPEG_QTBL_REG	S3C_JPEG_REG(0x08)
/* Huffman Table Number Register */
#define S3C_JPEG_HTBL_REG	S3C_JPEG_REG(0x0c)
#define S3C_JPEG_DRI_U_REG	S3C_JPEG_REG(0x10)	/* MCU, which inserts RST marker(upper 8bit)	*/
#define S3C_JPEG_DRI_L_REG	S3C_JPEG_REG(0x14)	/* MCU, which inserts RST marker(lower 8bit) 	*/
#define S3C_JPEG_Y_U_REG	S3C_JPEG_REG(0x18)	/* Vertical Resolution (upper 8bit)		*/
#define S3C_JPEG_Y_L_REG	S3C_JPEG_REG(0x1c)	/* Vertical Resolution (lower 8bit)		*/
#define S3C_JPEG_X_U_REG	S3C_JPEG_REG(0x20)	/* Horizontal Resolution (upper 8bit)		*/
#define S3C_JPEG_X_L_REG	S3C_JPEG_REG(0x24)	/* Horizontal Resolution (lower 8bit)		*/
#define S3C_JPEG_CNT_U_REG	S3C_JPEG_REG(0x28)	/* The amount of the compressed data in bytes (upper 8bit)	*/
#define S3C_JPEG_CNT_M_REG	S3C_JPEG_REG(0x2c)	/* The amount of the compressed data in bytes (middle 8bit)	*/
#define S3C_JPEG_CNT_L_REG	S3C_JPEG_REG(0x30)	/* The amount of the compressed data in bytes (lowerz 8bit)	*/
#define S3C_JPEG_INTSE_REG	S3C_JPEG_REG(0x34)	/* Interrupt setting register			*/
#define S3C_JPEG_INTST_REG	S3C_JPEG_REG(0x38)	/* Interrupt status				*/

#define S3C_JPEG_COM_REG	S3C_JPEG_REG(0x4c)	/* Command register 			*/

#define S3C_JPEG_IMGADR_REG	S3C_JPEG_REG(0x50)	/* Source or destination image addresss	*/

#define S3C_JPEG_JPGADR_REG	S3C_JPEG_REG(0x58)	/* Source or destination JPEG file address		*/
#define S3C_JPEG_COEF1_REG	S3C_JPEG_REG(0x5c)	/* Coefficient values for RGB <-> YCbCr converter	*/
#define S3C_JPEG_COEF2_REG	S3C_JPEG_REG(0x60)	/* Coefficient values for RGB <-> YCbCr converter	*/
#define S3C_JPEG_COEF3_REG	S3C_JPEG_REG(0x64)	/* Coefficient values for RGB <-> YCbCr converter	*/

#define S3C_JPEG_CMOD_REG	S3C_JPEG_REG(0x68)	/* Mode selection and core clock setting		*/
#define S3C_JPEG_CLKCON_REG	S3C_JPEG_REG(0x6c)	/* Power on/off and clock down control			*/

#define S3C_JPEG_JSTART_REG	S3C_JPEG_REG(0x70)	/* Start compression or decompression			*/
#define S3C_JPEG_JRSTART_REG	S3C_JPEG_REG(0x74)	/* Restart decompression after header analysis		*/
#define S3C_JPEG_SW_RESET_REG	S3C_JPEG_REG(0x78)	/* S/W reset						*/

#define S3C_JPEG_TIMER_SE_REG	S3C_JPEG_REG(0x7c)	/* Internal timer setting register		*/
#define S3C_JPEG_TIMER_ST_REG	S3C_JPEG_REG(0x80)	/* Internal timer status register		*/
#define S3C_JPEG_COMSTAT_REG	S3C_JPEG_REG(0x84)	/* Command status register			*/
#define S3C_JPEG_OUTFORM_REG	S3C_JPEG_REG(0x88)	/* Output color format of decompression		*/
#define S3C_JPEG_VERSION_REG	S3C_JPEG_REG(0x8c)	/* Version register				*/

#define S3C_JPEG_ENC_STREAM_INTSE_REG	S3C_JPEG_REG(0x98)	/* Compressed stream size interrupt setting register	*/
#define S3C_JPEG_ENC_STREAM_INTST_REG	S3C_JPEG_REG(0x9c)	/* Compressed stream size interrupt status register	*/

#define S3C_JPEG_QTBL0_REG		S3C_JPEG_REG(0x400)	/* Quantization table 0		*/
#define S3C_JPEG_QTBL1_REG		S3C_JPEG_REG(0x500)	/* Quantization table 1		*/
#define S3C_JPEG_QTBL2_REG		S3C_JPEG_REG(0x600)	/* Quantization table 2		*/
#define S3C_JPEG_QTBL3_REG		S3C_JPEG_REG(0x700)	/* Quantization table 3		*/

#define S3C_JPEG_HDCTBL0_REG		S3C_JPEG_REG(0x800)	/* DC huffman table 0		*/
#define S3C_JPEG_HDCTBLG0_REG		S3C_JPEG_REG(0x840)	/* DC huffman table group 0	*/
#define S3C_JPEG_HACTBL0_REG		S3C_JPEG_REG(0x880)	/* AC huffman table 0		*/
#define S3C_JPEG_HACTBLG0_REG		S3C_JPEG_REG(0x8c0)	/* AC huffman table group 0	*/
#define S3C_JPEG_HDCTBL1_REG		S3C_JPEG_REG(0xc00)	/* DC huffman table 1		*/
#define S3C_JPEG_HDCTBLG1_REG		S3C_JPEG_REG(0xc40)	/* DC huffman table group 1	*/
#define S3C_JPEG_HACTBL1_REG		S3C_JPEG_REG(0xc80)	/* AC huffman table 1		*/
#define S3C_JPEG_HACTBLG1_REG		S3C_JPEG_REG(0xcc0)	/* AC huffman table group 1	*/

/* JPEG Mode Register bit */
#define S3C_JPEG_MOD_REG_PROC_ENC			(0<<3)
#define S3C_JPEG_MOD_REG_PROC_DEC			(1<<3)

#define S3C_JPEG_MOD_REG_SUBSAMPLE_444			(0<<0)
#define S3C_JPEG_MOD_REG_SUBSAMPLE_422			(1<<0)
#define S3C_JPEG_MOD_REG_SUBSAMPLE_420			(2<<0)
#define S3C_JPEG_MOD_REG_SUBSAMPLE_GRAY			(3<<0)

/* JPEG Operation Status Register bit */
#define S3C_JPEG_OPR_REG_OPERATE			(1<<0)
#define S3C_JPEG_OPR_REG_NO_OPERATE			(0<<0)

/* Quantization Table And Huffman Table Number Register bit */
#define S3C_JPEG_QHTBL_REG_QT_NUM4			(1<<6)
#define S3C_JPEG_QHTBL_REG_QT_NUM3			(1<<4)
#define S3C_JPEG_QHTBL_REG_QT_NUM2			(1<<2)
#define S3C_JPEG_QHTBL_REG_QT_NUM1			(1<<0)

#define S3C_JPEG_QHTBL_REG_HT_NUM4_AC			(1<<7)
#define S3C_JPEG_QHTBL_REG_HT_NUM4_DC			(1<<6)
#define S3C_JPEG_QHTBL_REG_HT_NUM3_AC			(1<<5)
#define S3C_JPEG_QHTBL_REG_HT_NUM3_DC			(1<<4)
#define S3C_JPEG_QHTBL_REG_HT_NUM2_AC			(1<<3)
#define S3C_JPEG_QHTBL_REG_HT_NUM2_DC			(1<<2)
#define S3C_JPEG_QHTBL_REG_HT_NUM1_AC			(1<<1)
#define S3C_JPEG_QHTBL_REG_HT_NUM1_DC			(1<<0)


/* JPEG Color Mode Register bit */
#define S3C_JPEG_CMOD_REG_MOD_SEL_RGB			(2<<5)
#define S3C_JPEG_CMOD_REG_MOD_SEL_YCBCR422		(1<<5)
#define S3C_JPEG_CMOD_REG_MOD_MODE_Y16			(1<<1)
#define S3C_JPEG_CMOD_REG_MOD_MODE_0			(0<<1)

/* JPEG Clock Control Register bit */
#define S3C_JPEG_CLKCON_REG_CLK_DOWN_READY_ENABLE	(0<<1)
#define S3C_JPEG_CLKCON_REG_CLK_DOWN_READY_DISABLE	(1<<1)
#define S3C_JPEG_CLKCON_REG_POWER_ON_ACTIVATE		(1<<0)
#define S3C_JPEG_CLKCON_REG_POWER_ON_DISABLE		(0<<0)

/* JPEG Start Register bit */
#define S3C_JPEG_JSTART_REG_ENABLE			(1<<0)

/* JPEG Rdstart Register bit */
#define S3C_JPEG_JRSTART_REG_ENABLE			(1<<0)

/* JPEG SW Reset Register bit */
#define S3C_JPEG_SW_RESET_REG_ENABLE			(1<<0)

/* JPEG Interrupt Setting Register bit */
#define S3C_JPEG_INTSE_REG_RSTM_INT_EN			(1<<7)
#define S3C_JPEG_INTSE_REG_DATA_NUM_INT_EN		(1<<6)
#define S3C_JPEG_INTSE_REG_FINAL_MCU_NUM_INT_EN		(1<<5)

/* JPEG Decompression Output Format Register bit */
#define S3C_JPEG_OUTFORM_REG_YCBCY422			(0<<0)
#define S3C_JPEG_OUTFORM_REG_YCBCY420			(1<<0)

/* JPEG Decompression Input Stream Size Register bit */
#define S3C_JPEG_DEC_STREAM_SIZE_REG_PROHIBIT		(0x1FFFFFFF<<0)

/* JPEG Command Register bit */
#define S3C_JPEG_COM_INT_RELEASE			(1<<2)

#endif /*__ASM_ARM_REGS_S3C_JPEG_H */
