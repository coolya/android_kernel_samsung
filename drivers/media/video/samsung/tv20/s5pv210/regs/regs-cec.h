/* linux/drivers/media/video/samsung/tv20/s5pc110/regs/regs-cec.h
 *
 * CEC register header file for Samsung TVOut driver
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __REGS_CEC_H
#define __REGS_CEC_H

#define HDMIDP_CECREG(x)    (x)

/**
 * @name CEC config/status registers
 */
#define CEC_STATUS_0        HDMIDP_CECREG(0x0000)
#define CEC_STATUS_1        HDMIDP_CECREG(0x0004)
#define CEC_STATUS_2        HDMIDP_CECREG(0x0008)
#define CEC_STATUS_3        HDMIDP_CECREG(0x000C)
#define CEC_IRQ_MASK        HDMIDP_CECREG(0x0010)
#define CEC_IRQ_CLEAR       HDMIDP_CECREG(0x0014)
#define CEC_LOGIC_ADDR      HDMIDP_CECREG(0x0020)
#define CEC_DIVISOR_0       HDMIDP_CECREG(0x0030)
#define CEC_DIVISOR_1       HDMIDP_CECREG(0x0034)
#define CEC_DIVISOR_2       HDMIDP_CECREG(0x0038)
#define CEC_DIVISOR_3       HDMIDP_CECREG(0x003C)

/**
 * @name CEC Tx related registers
 */
#define CEC_TX_CTRL         HDMIDP_CECREG(0x0040)
#define CEC_TX_BYTES        HDMIDP_CECREG(0x0044)
#define CEC_TX_STAT0        HDMIDP_CECREG(0x0060)
#define CEC_TX_STAT1        HDMIDP_CECREG(0x0064)
#define CEC_TX_BUFF0        HDMIDP_CECREG(0x0080)
#define CEC_TX_BUFF1        HDMIDP_CECREG(0x0084)
#define CEC_TX_BUFF2        HDMIDP_CECREG(0x0088)
#define CEC_TX_BUFF3        HDMIDP_CECREG(0x008C)
#define CEC_TX_BUFF4        HDMIDP_CECREG(0x0090)
#define CEC_TX_BUFF5        HDMIDP_CECREG(0x0094)
#define CEC_TX_BUFF6        HDMIDP_CECREG(0x0098)
#define CEC_TX_BUFF7        HDMIDP_CECREG(0x009C)
#define CEC_TX_BUFF8        HDMIDP_CECREG(0x00A0)
#define CEC_TX_BUFF9        HDMIDP_CECREG(0x00A4)
#define CEC_TX_BUFF10       HDMIDP_CECREG(0x00A8)
#define CEC_TX_BUFF11       HDMIDP_CECREG(0x00AC)
#define CEC_TX_BUFF12       HDMIDP_CECREG(0x00B0)
#define CEC_TX_BUFF13       HDMIDP_CECREG(0x00B4)
#define CEC_TX_BUFF14       HDMIDP_CECREG(0x00B8)
#define CEC_TX_BUFF15       HDMIDP_CECREG(0x00BC)

/**
 * @name CEC Rx related registers
 */
#define CEC_RX_CTRL         HDMIDP_CECREG(0x00C0)
#define CEC_RX_STAT0        HDMIDP_CECREG(0x00E0)
#define CEC_RX_STAT1        HDMIDP_CECREG(0x00E4)
#define CEC_RX_BUFF0        HDMIDP_CECREG(0x0100)
#define CEC_RX_BUFF1        HDMIDP_CECREG(0x0104)
#define CEC_RX_BUFF2        HDMIDP_CECREG(0x0108)
#define CEC_RX_BUFF3        HDMIDP_CECREG(0x010C)
#define CEC_RX_BUFF4        HDMIDP_CECREG(0x0110)
#define CEC_RX_BUFF5        HDMIDP_CECREG(0x0114)
#define CEC_RX_BUFF6        HDMIDP_CECREG(0x0118)
#define CEC_RX_BUFF7        HDMIDP_CECREG(0x011C)
#define CEC_RX_BUFF8        HDMIDP_CECREG(0x0120)
#define CEC_RX_BUFF9        HDMIDP_CECREG(0x0124)
#define CEC_RX_BUFF10       HDMIDP_CECREG(0x0128)
#define CEC_RX_BUFF11       HDMIDP_CECREG(0x012C)
#define CEC_RX_BUFF12       HDMIDP_CECREG(0x0130)
#define CEC_RX_BUFF13       HDMIDP_CECREG(0x0134)
#define CEC_RX_BUFF14       HDMIDP_CECREG(0x0138)
#define CEC_RX_BUFF15       HDMIDP_CECREG(0x013C)

#define CEC_RX_FILTER_CTRL  HDMIDP_CECREG(0x0180)
#define CEC_RX_FILTER_TH    HDMIDP_CECREG(0x0184)

/**
 * @name Bit values
 */
#define CEC_IRQ_TX_DONE             (1<<0)
#define CEC_IRQ_TX_ERROR            (1<<1)
#define CEC_IRQ_RX_DONE             (1<<4)
#define CEC_IRQ_RX_ERROR            (1<<5)

#define CEC_TX_CTRL_START           (1<<0)
#define CEC_TX_CTRL_BCAST           (1<<1)
#define CEC_TX_CTRL_RETRY           (0x04<<4)
#define CEC_TX_CTRL_RESET           (1<<7)

#define CEC_RX_CTRL_ENABLE          (1<<0)
#define CEC_RX_CTRL_RESET           (1<<7)

#define CEC_LOGIC_ADDR_MASK         0x0F

#endif /* __REGS_CEC_H */
