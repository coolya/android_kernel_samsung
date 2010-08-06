/*
 * s3c-idma.h  --  I2S0's Internal Dma driver
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 * 	Jaswinder Singh <jassi.brar@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __S3C_IDMA_H_
#define __S3C_IDMA_H_

#ifdef CONFIG_ARCH_S5PC1XX /* S5PC100 */
#define MAX_LP_BUFF	(128 * 1024)
#define LP_DMA_PERIOD (105 * 1024)
#else
#define MAX_LP_BUFF	(160 * 1024)
#define LP_DMA_PERIOD (128 * 1024)
#endif

#define LP_TXBUFF_ADDR    (0xC0000000)

/* dma_state */
#define LPAM_DMA_STOP    0
#define LPAM_DMA_START   1

#endif /* __S3C_IDMA_H_ */
