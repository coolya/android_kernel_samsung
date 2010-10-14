/*
 * drivers/usb/gadget/s3c_udc.h
 * Samsung S3C on-chip full/high speed USB device controllers
 * Copyright (C) 2005 for Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __S3C_USB_GADGET
#define __S3C_USB_GADGET

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/dma.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
/* #include <asm/hardware.h> */

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

/* Max packet size */
#if defined(CONFIG_USB_GADGET_S3C_FS)
#define EP0_FIFO_SIZE		8
#define EP_FIFO_SIZE		64
#define S3C_MAX_ENDPOINTS	5
#elif defined(CONFIG_USB_GADGET_S3C_HS) || defined(CONFIG_ARCH_S5PV210)
#define EP0_FIFO_SIZE		64
#define EP_FIFO_SIZE		512
#define EP_FIFO_SIZE2		1024
#define S3C_MAX_ENDPOINTS	16
#define DED_TX_FIFO		1	/* Dedicated NPTx fifo for s5p6440 */
#else
#define EP0_FIFO_SIZE		64
#define EP_FIFO_SIZE		512
#define EP_FIFO_SIZE2		1024
#define S3C_MAX_ENDPOINTS	16
#endif

#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4
#define RegReadErr		5
#define FAIL_TO_SETUP		6

#define TEST_J_SEL		0x1
#define TEST_K_SEL		0x2
#define TEST_SE0_NAK_SEL	0x3
#define TEST_PACKET_SEL		0x4
#define TEST_FORCE_ENABLE_SEL	0x5


typedef enum ep_type {
	ep_control, ep_bulk_in, ep_bulk_out, ep_interrupt
} ep_type_t;

struct s3c_ep {
	struct usb_ep ep;
	struct s3c_udc *dev;

	const struct usb_endpoint_descriptor *desc;
	struct list_head queue;
	unsigned long pio_irqs;

	u8 stopped;
	u8 bEndpointAddress;
	u8 bmAttributes;

	ep_type_t ep_type;
	u32 fifo;
#ifdef CONFIG_USB_GADGET_S3C_FS
	u32 csr1;
	u32 csr2;
#endif
};

struct s3c_request {
	struct usb_request req;
	struct list_head queue;
	unsigned char mapped;
};

struct s3c_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	/* struct device *dev; */
	struct platform_device *dev;
	spinlock_t lock;
	u16 status;
	int ep0state;
	struct s3c_ep ep[S3C_MAX_ENDPOINTS];

	unsigned char usb_address;

	unsigned req_pending:1, req_std:1, req_config:1;

	struct regulator *udc_vcc_d, *udc_vcc_a;
	int udc_enabled;
};

extern struct s3c_udc *the_controller;
extern void otg_phy_init(void);
extern void otg_phy_off(void);
extern struct usb_ctrlrequest usb_ctrl;
extern struct i2c_driver fsa9480_i2c_driver;

#define ep_is_in(EP)		(((EP)->bEndpointAddress&USB_DIR_IN) == USB_DIR_IN)
#define ep_index(EP)		((EP)->bEndpointAddress&0xF)
#define ep_maxpacket(EP)	((EP)->ep.maxpacket)

#endif
