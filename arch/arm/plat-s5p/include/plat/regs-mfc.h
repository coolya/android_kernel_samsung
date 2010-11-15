/* linux/arch/arm/plat-s5pc1xx/include/plat/regs-mfc.h
 *
 * Register definition file for Samsung MFC V5.0 Interface (FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 * 
 * history:
 *   2009.09.14 : Delete MFC v4.0 codes & rewrite defintion (Key-Young, Park)
 *   2009.10.08 - Apply 9/30 firmware(Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _REGS_FIMV_H
#define _REGS_FIMV_H

#if defined(CONFIG_CPU_S5PV210)
#define MFC_REG(x)                    (x)
#define MFC_START_ADDR                MFC_REG(0x0000)
#define MFC_END_ADDR                  MFC_REG(0xe008 + 4)
#define MFC_REG_SIZE                  (MFC_END_ADDR - MFC_START_ADDR)
#define MFC_REG_COUNT                 ((MFC_END_ADDR - MFC_START_ADDR) / 4)

/*====================================================================================================*/
/*  MFC Core Control Register                                                                         */
/*====================================================================================================*/
#define MFC_SW_RESET                  MFC_REG(0x0000) /* MFC Software Reset Register                  */
#define MFC_RISC_HOST_INT             MFC_REG(0x0008) /* RISC to HOST Interrupt Reigser               */

#define MFC_HOST2RISC_COMMAND         MFC_REG(0x0030) /* HOST to RISC Command Register                */
#define MFC_HOST2RISC_ARG1            MFC_REG(0x0034) /* first argument register                      */
#define MFC_HOST2RISC_ARG2            MFC_REG(0x0038) /* second argument register                     */
#define MFC_HOST2RISC_ARG3            MFC_REG(0x003c) /* third argument register                      */
#define MFC_HOST2RISC_ARG4            MFC_REG(0x0040) /* fourth argument register                     */

#define MFC_RISC2HOST_COMMAND         MFC_REG(0x0044) /* RISC to HOST Command Register                */
#define MFC_RISC2HOST_ARG1            MFC_REG(0x0048) /* first argument register                      */
#define MFC_RISC2HOST_ARG2            MFC_REG(0x004c) /* second argument register                     */
#define MFC_RISC2HOST_ARG3            MFC_REG(0x0050) /* third argument register                      */
#define MFC_RISC2HOST_ARG4            MFC_REG(0x0054) /* fourth argument register                     */

#define MFC_FW_VERSION                MFC_REG(0x0058) /* firmware version register                    */
#define MFC_FW_STATUS                 MFC_REG(0x0080) /* 0: Not ready, 1: Ready                       */


/*====================================================================================================*/
/*  Memory Controller Register                                                                        */
/*====================================================================================================*/
#define MFC_MC_DRAMBASE_ADDR_A        MFC_REG(0x0508) /* Port A DRAM base address                     */
#define MFC_MC_DRAMBASE_ADDR_B        MFC_REG(0x050c) /* Port B DRAM base address                     */
#define MFC_MC_STATUS                 MFC_REG(0x0510) /* Bus arbiter's status                         */


/*====================================================================================================*/
/*  Common Address Control                                                                            */
/*====================================================================================================*/
/*----------------------------------------------------------------------------------------------------*/
/*  H264 Decoder Buffer Register                                                                      */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_H264DEC_VERT_NB_MV        MFC_REG(0x068c) /* Base:35, vertical neighbor motion vector     */
#define MFC_H264DEC_NB_IP             MFC_REG(0x0690) /* Base:36, neighbor pixels for intra pred      */
#define MFC_H264DEC_LUMA              MFC_REG(0x0700) /* Base:64, Luma0 ~ Luma31                      */
#define MFC_H264DEC_CHROMA            MFC_REG(0x0600) /* Base:0,  Chroma0 ~ Chroma31                  */
#define MFC_H264DEC_MV                MFC_REG(0x0780) /* Base:96, Mv0 ~ Mv31                          */

/*----------------------------------------------------------------------------------------------------*/
/*  Other Decoder Buffer Register                                                                     */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_NB_DCAC                   MFC_REG(0x068c) /* Base:35, neighbor AC/DC coeff. buffer        */
#define MFC_UPNB_MV                   MFC_REG(0x0690) /* Base:36, upper neighbor motion vector buffer */
#define MFC_SUB_ANCHOR_MV             MFC_REG(0x0694) /* Base:37, subseq. anchor motion vector buffer */
#define MFC_OVERLAP_TRANSFORM         MFC_REG(0x0698) /* Base:38, overlap transform line buffer       */
#define MFC_STX_PARSER                MFC_REG(0x06A8) /* Base:42, syntax parser addr                  */
#define MFC_DEC_LUMA                  MFC_REG(0x0700) /* Base:64, Luma0 ~ Luma5                       */
#define MFC_DEC_CHROMA                MFC_REG(0x0600) /* Base:0,  Chroma0 ~ Chroma5                   */
#define MFC_BITPLANE1                 MFC_REG(0x06A4) /* Base:41, bitplane1 addr                      */
#define MFC_BITPLANE2                 MFC_REG(0x06A0) /* Base:40, bitplane2 addr                      */
#define MFC_BITPLANE3                 MFC_REG(0x069C) /* Base:39, bitplane3 addr                      */

/*----------------------------------------------------------------------------------------------------*/
/*  Encoder Buffer Register                                                                           */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_UPPER_MV_ADDR             MFC_REG(0x0600) /* Base:0,  upper motion vector addr            */
#define MFC_DIRECT_COLZERO_FLAG_ADDR  MFC_REG(0x0610) /* Base:4,  direct cozero flag addr             */
#define MFC_UPPER_INTRA_MD_ADDR       MFC_REG(0x0608) /* Base:2,  upper intra MD addr                 */
#define MFC_UPPER_INTRA_PRED_ADDR     MFC_REG(0x0740) /* Base:80,  upper intra PRED addr              */
#define MFC_NBOR_INFO_MPENC_ADDR      MFC_REG(0x0604) /* Base:1,  entropy engine's AC/DC coeff.       */
#define MFC_ACDC_COEF_BASE_ADDR       MFC_REG(0x0604) /* Base:1,  entropy engine's AC/DC coeff.       */
#define MFC_ENC_DPB_Y0_ADDR           MFC_REG(0x061C) /* Base:7,  ref0 Luma addr                      */
#define MFC_ENC_DPB_C0_ADDR           MFC_REG(0x0700) /* Base:64, ref0 Chroma addr                    */
#define MFC_ENC_DPB_Y1_ADDR           MFC_REG(0x0620) /* Base:8,  ref1 Luma addr                      */
#define MFC_ENC_DPB_C1_ADDR           MFC_REG(0x0704) /* Base:65, ref1 Chroma addr                    */
#define MFC_ENC_DPB_Y2_ADDR           MFC_REG(0x0710) /* Base:68, ref2 Luma addr                      */
#define MFC_ENC_DPB_C2_ADDR           MFC_REG(0x0708) /* Base:66, ref2 Chroma addr                    */
#define MFC_ENC_DPB_Y3_ADDR           MFC_REG(0x0714) /* Base:69, ref3 Luma addr                      */
#define MFC_ENC_DPB_C3_ADDR           MFC_REG(0x070c) /* Base:67, ref3 Chroma addr                    */


/*====================================================================================================*/
/*  Channel & Stream Interface Register                                                               */
/*====================================================================================================*/
#define MFC_HSIZE_PX                  MFC_REG(0x0818) /* frame width at encoder                       */
#define MFC_VSIZE_PX                  MFC_REG(0x081c) /* frame height at encoder                      */
#define MFC_PROFILE                   MFC_REG(0x0830) /* profile register                             */
#define MFC_PICTURE_STRUCT            MFC_REG(0x083c) /* picture field/frame flag                     */
#define MFC_LF_CONTROL                MFC_REG(0x0848) /* loop filter control                          */
#define MFC_LF_ALPHA_OFF              MFC_REG(0x084c) /* H.264 loop filter alpha offset               */
#define MFC_LF_BETA_OFF               MFC_REG(0x0850) /* H.264 loop filter beta offset                */
#define MFC_QP_OFFSET                 MFC_REG(0x0c30) /* QP information offset from the DPB address   */
#define MFC_QP_OUT_EN                 MFC_REG(0x0c34) /* QP infomration enable at decoder             */


/*====================================================================================================*/
/*  Channel & Stream Interface Register                                                               */
/*====================================================================================================*/
#define MFC_SI_RTN_CHID               MFC_REG(0x2000) /* Return CH instance ID register               */
#define MFC_SI_CH0_INST_ID            MFC_REG(0x2040) /* CH0 instance ID and control register         */
#define MFC_SI_CH1_INST_ID            MFC_REG(0x2080) /* CH1 instance ID and control register         */

/*----------------------------------------------------------------------------------------------------*/
/*  Decoder Channel and Stream Interface Register                                                     */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_SI_VER_RESOL              MFC_REG(0x2004) /* vertical resolution of decoder               */
#define MFC_SI_HOR_RESOL              MFC_REG(0x2008) /* horizontal resolution of decoder             */
#define MFC_SI_MIN_NUM_DPB            MFC_REG(0x200c) /* number of frames in the decoded pic          */
#define MFC_SI_DISPLAY_Y_ADR          MFC_REG(0x2010) /* luma address of displayed pic                */
#define MFC_SI_DISPLAY_C_ADR          MFC_REG(0x2014) /* chroma address of displayed pic              */
#define MFC_SI_DEC_FRM_SIZE           MFC_REG(0x2018) /* consumed number of bytes to decode a frame   */
#define MFC_SI_DISPLAY_STATUS         MFC_REG(0x201c) /* status of displayed picture                  */
#define MFC_SI_FRAME_TYPE             MFC_REG(0x2020) /* frame type such as skip/I/P/B                */ 
#define MFC_SI_DECODE_Y_ADR           MFC_REG(0x2024) /* luma address of decoded pic                  */
#define MFC_SI_DECODE_C_ADR           MFC_REG(0x2028) /* chroma address of decoded pic                */
#define MFC_SI_DECODE_STATUS          MFC_REG(0x202c) /* status of decoded picture                    */

#define MFC_SI_CH0_ES_ADDR            MFC_REG(0x2044) /* start addr of stream buf                     */
#define MFC_SI_CH0_ES_DEC_UNIT_SIZE   MFC_REG(0x2048) /* size of stream buf                           */
#define MFC_SI_CH0_DESC_ADDR          MFC_REG(0x204c) /* addr of descriptor buf                       */
#define MFC_SI_CH0_FIMV1_VRESOL       MFC_REG(0x2050) /* horizontal resolution                        */
#define MFC_SI_CH0_FIMV1_HRESOL       MFC_REG(0x2054) /* vertical resolution                          */
#define MFC_SI_CH0_CPB_SIZE           MFC_REG(0x2058) /* max size of coded pic. buf                   */
#define MFC_SI_CH0_DESC_SIZE          MFC_REG(0x205c) /* max size of descriptor buf                   */
#define MFC_SI_CH0_RELEASE_BUFFER     MFC_REG(0x2060) /* specifices the availability of each DPB      */
#define MFC_SI_CH0_HOST_WR_ADR        MFC_REG(0x2064) /* start addr of shared memory                  */
#define MFC_SI_CH0_DPB_CONFIG_CTRL    MFC_REG(0x2068) /* DPB configuarion control                     */
#define MFC_SI_CH0_CMD_SEQ_NUM        MFC_REG(0x206c) /* Command sequence number from the host        */

#define MFC_SI_CH1_ES_ADDR            MFC_REG(0x2084) /* start addr of stream buf                     */
#define MFC_SI_CH1_ES_DEC_UNIT_SIZE   MFC_REG(0x2088) /* size of stream buf                           */
#define MFC_SI_CH1_DESC_ADDR          MFC_REG(0x208c) /* addr of descriptor buf                       */
#define MFC_SI_CH1_FIMV1_VRESOL       MFC_REG(0x2090) /* horizontal resolution                        */
#define MFC_SI_CH1_FIMV1_HRESOL       MFC_REG(0x2094) /* vertical resolution                          */
#define MFC_SI_CH1_CPB_SIZE           MFC_REG(0x2098) /* max size of coded pic. buf                   */
#define MFC_SI_CH1_DESC_SIZE          MFC_REG(0x209c) /* max size of descriptor buf                   */
#define MFC_SI_CH1_RELEASE_BUFFER     MFC_REG(0x20a0) /* specifices the availability of each DPB      */
#define MFC_SI_CH1_HOST_WR_ADR        MFC_REG(0x20a4) /* start addr of shared memory                  */
#define MFC_SI_CH1_DPB_CONFIG_CTRL    MFC_REG(0x20a8) /* DPB configuarion control                     */
#define MFC_SI_CH1_CMD_SEQ_NUM        MFC_REG(0x20ac) /* Command sequence number from the host        */

/*----------------------------------------------------------------------------------------------------*/
/*  Encoder Channel and Stream Interface Register                                                     */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_CRC_LUMA0                 MFC_REG(0x2030) /* luma crc data per frame(or top field)        */
#define MFC_CRC_CHROMA0               MFC_REG(0x2034) /* chroma crc data per frame(or top field)      */
#define MFC_CRC_LUMA1                 MFC_REG(0x2038) /* luma crc data per bottom field               */
#define MFC_CRC_CHROMA1               MFC_REG(0x203c) /* chroma crc data per bottom field             */

#define MFC_SI_ENC_STREAM_SIZE        MFC_REG(0x2004) /* stream size                                  */
#define MFC_SI_ENC_PICTURE_CNT        MFC_REG(0x2008) /* picture count                                */
#define MFC_SI_WRITE_POINTER          MFC_REG(0x200c) /* stream buffer write pointer                  */
#define MFC_SI_ENC_SLICE_TYPE         MFC_REG(0x2010) /* slice type(I/P/B/IDR)                        */
#define MFC_SI_ENCODED_Y_ADDR         MFC_REG(0x2014) /* address of the encoded luminance picture     */
#define MFC_SI_ENCODED_C_ADDR         MFC_REG(0x2018) /* address of the encoded chroma address        */

#define MFC_SI_CH0_SB_U_ADDR          MFC_REG(0x2044) /* addr of upper stream buf                     */
#define MFC_SI_CH0_SB_L_ADDR          MFC_REG(0x2048) /* addr of lower stream buf                     */
#define MFC_SI_CH0_BUFFER_SIZE        MFC_REG(0x204c) /* size of stream buf                           */
#define MFC_SI_CH0_CURRENT_Y_ADDR     MFC_REG(0x2050) /* current Luma addr                            */
#define MFC_SI_CH0_CURRENT_C_ADDR     MFC_REG(0x2054) /* current Chroma addr                          */
#define MFC_SI_CH0_ENC_PARA           MFC_REG(0x2058) /* frame insertion control register             */

#define MFC_SI_CH1_SB_U_ADDR          MFC_REG(0x2084) /* addr of upper stream buf                     */
#define MFC_SI_CH1_SB_L_ADDR          MFC_REG(0x2088) /* addr of lower stream buf                     */
#define MFC_SI_CH1_BUFFER_SIZE        MFC_REG(0x208c) /* size of stream buf                           */
#define MFC_SI_CH1_CURRENT_Y_ADDR     MFC_REG(0x2090) /* current Luma addr                            */
#define MFC_SI_CH1_CURRENT_C_ADDR     MFC_REG(0x2094) /* current Chroma addr                          */
#define MFC_SI_CH1_ENC_PARA           MFC_REG(0x2098) /* frame insertion control register             */

/*----------------------------------------------------------------------------------------------------*/
/*  Encoded Data Formatter Unit Register                                                              */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_STR_BF_U_FULL             MFC_REG(0xc004) /* upper stream buf full status                 */	
#define MFC_STR_BF_U_EMPTY            MFC_REG(0xc008) /* upper stream buf empty status                */	
#define MFC_STR_BF_L_FULL             MFC_REG(0xc00c) /* lower stream buf full status                 */	
#define MFC_STR_BF_L_EMPTY            MFC_REG(0xc010) /* lower stream buf empty status                */	
#define MFC_STR_BF_STATUS             MFC_REG(0xc018) /* stream buf interrupt status                  */	
#define MFC_EDFU_SF_EPB_ON_CTRL       MFC_REG(0xc054) /* EDFU stream control                          */	
#define MFC_EDFU_SF_BUF_CTRL          MFC_REG(0xc058) /* EDFU buffer control                          */	
#define MFC_STR_BF_MODE_CTRL          MFC_REG(0xc05c) /* stream buffer mode control                   */	


/*====================================================================================================*/
/*  Encoding Registers                                                                                */
/*====================================================================================================*/
/*----------------------------------------------------------------------------------------------------*/
/*  Common Encoder Register                                                                           */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_RC_CONTROL_CONFIG		  MFC_REG(0x00a0)	/* Rate Control Configuration */
#define MFC_ENC_PIC_TYPE_CTRL         MFC_REG(0xc504) /* pic type level control                       */	
#define MFC_ENC_B_RECON_WRITE_ON      MFC_REG(0xc508) /* B frame recon data write cotrl               */
#define MFC_ENC_MSLICE_CTRL           MFC_REG(0xc50c) /* multi slice control                          */
#define MFC_ENC_MSLICE_MB             MFC_REG(0xc510) /* MB number in the one slice                   */
#define MFC_ENC_MSLICE_BYTE           MFC_REG(0xc514) /* byte count number for one slice              */
#define MFC_ENC_CIR_CTRL              MFC_REG(0xc518) /* number of intra refresh MB                   */
#define MFC_ENC_MAP_FOR_CUR           MFC_REG(0xc51c) /* linear or 64x32 tiled mode                   */
#define MFC_ENC_PADDING_CTRL          MFC_REG(0xc520) /* padding control                              */
#define MFC_ENC_INT_MASK              MFC_REG(0xc528) /* interrupt mask                               */
#define MFC_ENC_COMMON_INTRA_BIAS     MFC_REG(0xc528) /* intra mode bias register                     */
#define MFC_ENC_COMMON_BI_DIRECT_BIAS MFC_REG(0xc528) /* bi-directional mode bias register            */

/*----------------------------------------------------------------------------------------------------*/
/*  Rate Control Register                                                                             */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_RC_CONFIG                 MFC_REG(0xc5a0) /* RC config                                    */
#define MFC_RC_FRAME_RATE             MFC_REG(0xc5a4) /* frame rate                                   */
#define MFC_RC_BIT_RATE               MFC_REG(0xc5a8) /* bit rate                                     */
#define MFC_RC_QBOUND                 MFC_REG(0xc5ac) /* max/min QP                                   */
#define MFC_RC_RPARA                  MFC_REG(0xc5b0) /* rate control reaction coeff.                 */
#define MFC_RC_MB_CTRL                MFC_REG(0xc5b4) /* MB adaptive scaling                          */

/*----------------------------------------------------------------------------------------------------*/
/*  H.264 Encoder Register                                                                            */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_H264_ENC_ENTRP_MODE       MFC_REG(0xd004) /* CAVLC or CABAC                               */
#define MFC_H264_ENC_NUM_OF_REF       MFC_REG(0xd010) /* number of reference for P/B                  */
#define MFC_H264_ENC_MDINTER_WEIGHT   MFC_REG(0xd01c) /* inter weighted parameter                     */
#define MFC_H264_ENC_MDINTRA_WEIGHT   MFC_REG(0xd020) /* intra weighted parameter                     */
#define MFC_H264_ENC_TRANS_8X8_FLAG   MFC_REG(0xd034) /* 8x8 transform flag in PPS & high profile     */

/*----------------------------------------------------------------------------------------------------*/
/*  MPEG4 Encoder Register                                                                            */
/*----------------------------------------------------------------------------------------------------*/
#define MFC_MPEG4_ENC_QUART_PXL       MFC_REG(0xe008) /* quarter pel interpolation control            */


/*====================================================================================================*/
/*  Etc Registers                                                                                     */
/*====================================================================================================*/
#define MFC_NUM_MASTER                MFC_REG(0x0b14) /* Number of master port                        */

//#define MFC_SI_DEC_FRM_SIZE 	      MFC_REG(0x2018)
#endif /* CONFIG_CPU_S5PC1xx */

#endif
