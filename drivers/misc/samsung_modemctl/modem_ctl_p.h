/*
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_CONTROL_P_H__
#define __MODEM_CONTROL_P_H__

#define MODEM_OFF		0
#define MODEM_CRASHED		1
#define MODEM_RAMDUMP		2
#define MODEM_POWER_ON		3
#define MODEM_BOOTING_NORMAL	4
#define MODEM_BOOTING_RAMDUMP	5
#define MODEM_DUMPING		6
#define MODEM_RUNNING		7

#define modem_offline(mc) ((mc)->status < MODEM_POWER_ON)
#define modem_running(mc) ((mc)->status == MODEM_RUNNING)

#define M_PIPE_MAX_HDR 16

struct net_device;

struct m_pipe {
	int (*push_header)(struct modem_io *io, void *header);
	int (*pull_header)(struct modem_io *io, void *header);

	unsigned header_size;

	struct m_fifo *tx;
	struct m_fifo *rx;

	struct modemctl *mc;
	unsigned ready;

	struct miscdevice dev;

	struct mutex tx_lock;
	struct mutex rx_lock;

	struct wake_lock wakelock;
};
#define to_m_pipe(misc) container_of(misc, struct m_pipe, dev)

struct m_fifo {
	unsigned *head;
	unsigned *tail;
	unsigned size;
	void *data;

	unsigned avail;
	unsigned bits;
	unsigned unused1;
	unsigned unused2;
};

struct modemstats {
	unsigned request_no_wait;
	unsigned request_wait;

	unsigned release_no_action;
	unsigned release_bp_waiting;
	unsigned release_bp_signaled;

	unsigned bp_req_instant;
	unsigned bp_req_delayed;
	unsigned bp_req_confused;

	unsigned rx_unknown;
	unsigned rx_dropped;
	unsigned rx_purged;
	unsigned rx_received;

	unsigned tx_no_delay;
	unsigned tx_queued;
	unsigned tx_bp_signaled;
	unsigned tx_fifo_full;

	unsigned pipe_tx;
	unsigned pipe_rx;
	unsigned pipe_tx_delayed;
	unsigned pipe_rx_purged;

	unsigned resets;
};

#define MODEM_COUNT(mc,s) (((mc)->stats.s)++)

struct modemctl {
	void __iomem *mmio;
	struct modemstats stats;

	/* lock and waitqueue for shared memory state */
	spinlock_t lock;
	wait_queue_head_t wq;

	/* shared memory semaphore management */
	unsigned mmio_req_count;
	unsigned mmio_bp_request;
	unsigned mmio_owner;
	unsigned mmio_signal_bits;

	struct m_fifo fmt_tx;
	struct m_fifo fmt_rx;
	struct m_fifo raw_tx;
	struct m_fifo raw_rx;
	struct m_fifo rfs_tx;
	struct m_fifo rfs_rx;

	struct wake_lock ip_tx_wakelock;
	struct wake_lock ip_rx_wakelock;

	struct net_device *ndev;

	int open_count;
	int status;

	unsigned mmbase;
	unsigned mmsize;

	int irq_bp;
	int irq_mbox;

	unsigned gpio_phone_active;
	unsigned gpio_pda_active;
	unsigned gpio_cp_reset;

	struct miscdevice dev;

	struct m_pipe cmd_pipe;
	struct m_pipe rfs_pipe;

	struct mutex ctl_lock;
	ktime_t mmio_t0;

	/* used for ramdump mode */
	unsigned ramdump_size;
	loff_t ramdump_pos;

	unsigned logdump;
	unsigned logdump_data;
};
#define to_modemctl(misc) container_of(misc, struct modemctl, dev)


/* called when semaphore is held and there may be io to process */
void modem_handle_io(struct modemctl *mc);
void modem_update_state(struct modemctl *mc);

/* called once at probe() */
int modem_io_init(struct modemctl *mc, void __iomem *mmio);

/* called when modem boots and goes offline */
void modem_io_enable(struct modemctl *mc);
void modem_io_disable(struct modemctl *mc);


/* Block until control of mmio area is obtained (0)
 * or interrupt (-ERESTARTSYS) or failure (-ENODEV)
 * occurs.
 */
int modem_acquire_mmio(struct modemctl *mc);

/* Request control of mmio area.  Returns 1 if
 * control obtained, 0 if not (request pending).
 * Either way, release_mmio() must be called to
 * balance this.
 */
int modem_request_mmio(struct modemctl *mc);

/* Return control of mmio area once requested
 * by modem_request_mmio() or acquired by a
 * successful modem_acquire_mmio().
 *
 * The onedram semaphore is only actually returned
 * to the BP if there is an outstanding request
 * for it from the BP, or if the bits argument
 * to one of the release_mmio() calls was nonzero.
 */
void modem_release_mmio(struct modemctl *mc, unsigned bits);

/* Send a request for the hw mmio sem to the modem.
 * Used ONLY by the internals of modem_request_mmio() and
 * some trickery in vnet_xmit().  Please do not use elsewhere.
 */
void modem_request_sem(struct modemctl *mc);


/* internal glue */
void modem_debugfs_init(struct modemctl *mc);
void modem_force_crash(struct modemctl *mc);

/* protocol definitions */
#define MB_VALID		0x0080
#define MB_COMMAND		0x0040

/* CMD_INIT_END extended bit */
#define CP_BOOT_ONLINE		0x0000
#define CP_BOOT_AIRPLANE	0x1000
#define AP_OS_ANDROID		0x0100
#define AP_OS_WINMOBILE		0x0200
#define AP_OS_LINUX		0x0300
#define AP_OS_SYMBIAN		0x0400

/* CMD_PHONE_START extended bit */
#define CP_QUALCOMM		0x0100
#define CP_INFINEON		0x0200
#define CP_BROADCOM		0x0300

#define MBC_NONE		0x0000
#define MBC_INIT_START		0x0001
#define MBC_INIT_END		0x0002
#define MBC_REQ_ACTIVE		0x0003
#define MBC_RES_ACTIVE		0x0004
#define MBC_TIME_SYNC		0x0005
#define MBC_POWER_OFF		0x0006
#define MBC_RESET		0x0007
#define MBC_PHONE_START		0x0008
#define MBC_ERR_DISPLAY		0x0009
#define MBC_SUSPEND		0x000A
#define MBC_RESUME		0x000B
#define MBC_EMER_DOWN		0x000C
#define MBC_REQ_SEM		0x000D
#define MBC_RES_SEM		0x000E
#define MBC_MAX			0x000F

/* data mailbox flags */
#define MBD_SEND_FMT		0x0002
#define MBD_SEND_RAW		0x0001
#define MBD_SEND_RFS		0x0100

#define MODEM_MSG_SBL_DONE		0x12341234
#define MODEM_CMD_BINARY_LOAD		0x45674567
#define MODEM_MSG_BINARY_DONE		0xabcdabcd

#define MODEM_CMD_RAMDUMP_START		0xDEADDEAD
#define MODEM_MSG_RAMDUMP_LARGE		0x0ADD0ADD // 16MB - 2KB
#define MODEM_CMD_RAMDUMP_MORE		0xEDEDEDED
#define MODEM_MSG_RAMDUMP_SMALL		0xFADEFADE // 5MB + 4KB

#define MODEM_CMD_LOGDUMP_START		0x19732864
//#define MODEM_MSG_LOGDUMP_DONE		0x28641973
#define MODEM_MSG_LOGDUMP_DONE		0x00001973

#define RAMDUMP_LARGE_SIZE	(16*1024*1024 - 2*1024)
#define RAMDUMP_SMALL_SIZE	(5*1024*1024 + 4*1024)


/* onedram shared memory map */
#define OFF_MAGIC		0x00000000
#define OFF_ACCESS		0x00000004

#define OFF_FMT_TX_HEAD		0x00000010
#define OFF_FMT_TX_TAIL		0x00000014
#define OFF_FMT_RX_HEAD		0x00000018
#define OFF_FMT_RX_TAIL		0x0000001C
#define OFF_RAW_TX_HEAD		0x00000020
#define OFF_RAW_TX_TAIL		0x00000024
#define OFF_RAW_RX_HEAD		0x00000028
#define OFF_RAW_RX_TAIL		0x0000002C
#define OFF_RFS_TX_HEAD		0x00000030
#define OFF_RFS_TX_TAIL		0x00000034
#define OFF_RFS_RX_HEAD		0x00000038
#define OFF_RFS_RX_TAIL		0x0000003C

#define OFF_ERROR_MSG		0x00001000
#define SIZ_ERROR_MSG		160

#define OFF_FMT_TX_DATA		0x000FE000
#define OFF_FMT_RX_DATA		0x000FF000
#define SIZ_FMT_DATA		0x00001000
#define OFF_RAW_TX_DATA		0x00100000
#define OFF_RAW_RX_DATA		0x00200000
#define SIZ_RAW_DATA		0x00100000
#define OFF_RFS_TX_DATA		0x00300000
#define OFF_RFS_RX_DATA		0x00400000
#define SIZ_RFS_DATA		0x00100000

#define OFF_LOGDUMP_DATA	0x00A00000
#define SIZ_LOGDUMP_DATA	0x00300000

#define INIT_M_FIFO(name, type, dir, base) \
	name.head = base + OFF_##type##_##dir##_HEAD; \
	name.tail = base + OFF_##type##_##dir##_TAIL; \
	name.data = base + OFF_##type##_##dir##_DATA; \
	name.size = SIZ_##type##_DATA;

/* onedram registers */

/* Mailboxes are named based on who writes to them.
 * MBOX_BP is written to by the (B)aseband (P)rocessor
 * and only readable by the (A)pplication (P)rocessor.
 * MBOX_AP is the opposite.
 */
#define OFF_SEM		0xFFF800
#define OFF_MBOX_BP	0xFFF820
#define OFF_MBOX_AP	0xFFF840
#define OFF_CHECK_BP	0xFFF8A0
#define OFF_CHECK_AP	0xFFF8C0

#endif
