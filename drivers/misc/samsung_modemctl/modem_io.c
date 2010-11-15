/* modem_io.c
 *
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

/* TODO
 * - on modem crash return -ENODEV from recv/send, poll==readable
 * - ensure all modem off/reset cases fault out io properly
 * - request thread irq?
 * - stats/debugfs
 * - purge txq on restart
 * - test, test, test
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include <linux/circ_buf.h>
#include <linux/wakelock.h>

#include "modem_ctl.h"
#include "modem_ctl_p.h"

#define RAW_CH_VNET0 10


/* general purpose fifo access routines */

typedef void * (*copyfunc)(void *, const void *, __kernel_size_t);

static void *x_copy_to_user(void *dst, const void *src, __kernel_size_t sz)
{
	if (copy_to_user((void __user *) dst, src, sz) != 0)
		pr_err("modemctl: cannot copy userdata\n");
	return dst;
}

static void *x_copy_from_user(void *dst, const void *src, __kernel_size_t sz)
{
	if (copy_from_user(dst, (const void __user *) src, sz) != 0)
		pr_err("modemctl: cannot copy userdata\n");
	return dst;
}

static unsigned _fifo_read(struct m_fifo *q, void *dst,
			   unsigned count, copyfunc copy)
{
	unsigned n;
	unsigned head = *q->head;
	unsigned tail = *q->tail;
	unsigned size = q->size;

	if (CIRC_CNT(head, tail, size) < count)
		return 0;

	n = CIRC_CNT_TO_END(head, tail, size);

	if (likely(n >= count)) {
		copy(dst, q->data + tail, count);
	} else {
		copy(dst, q->data + tail, n);
		copy(dst + n, q->data, count - n);
	}
	*q->tail = (tail + count) & (size - 1);

	return count;
}

static unsigned _fifo_write(struct m_fifo *q, void *src,
			    unsigned count, copyfunc copy)
{
	unsigned n;
	unsigned head = *q->head;
	unsigned tail = *q->tail;
	unsigned size = q->size;

	if (CIRC_SPACE(head, tail, size) < count)
		return 0;

	n = CIRC_SPACE_TO_END(head, tail, size);

	if (likely(n >= count)) {
		copy(q->data + head, src, count);
	} else {
		copy(q->data + head, src, n);
		copy(q->data, src + n, count - n);
	}
	*q->head = (head + count) & (size - 1);

	return count;
}

static void fifo_purge(struct m_fifo *q)
{
	*q->head = 0;
	*q->tail = 0;
}

static unsigned fifo_skip(struct m_fifo *q, unsigned count)
{
	if (CIRC_CNT(*q->head, *q->tail, q->size) < count)
		return 0;
	*q->tail = (*q->tail + count) & (q->size - 1);
	return count;
}

#define fifo_read(q, dst, count) \
	_fifo_read(q, dst, count, memcpy)
#define fifo_read_user(q, dst, count) \
	_fifo_read(q, dst, count, x_copy_to_user)

#define fifo_write(q, src, count) \
	_fifo_write(q, src, count, memcpy)
#define fifo_write_user(q, src, count) \
	_fifo_write(q, src, count, x_copy_from_user)

#define fifo_count(mf) CIRC_CNT(*(mf)->head, *(mf)->tail, (mf)->size)
#define fifo_space(mf) CIRC_SPACE(*(mf)->head, *(mf)->tail, (mf)->size)

static void fifo_dump(const char *tag, struct m_fifo *q,
		      unsigned start, unsigned count)
{
	if (count > 64)
		count = 64;

	if ((start + count) <= q->size) {
		print_hex_dump_bytes(tag, DUMP_PREFIX_ADDRESS,
				     q->data + start, count);
	} else {
		print_hex_dump_bytes(tag, DUMP_PREFIX_ADDRESS,
				     q->data + start, q->size - start);
		print_hex_dump_bytes(tag, DUMP_PREFIX_ADDRESS,
				     q->data, count - (q->size - start));
	}
}



/* Called with mc->lock held whenever we gain access
 * to the mmio region.
 */
void modem_update_state(struct modemctl *mc)
{
	/* update our idea of space available in fifos */
	mc->fmt_tx.avail = fifo_space(&mc->fmt_tx);
	mc->fmt_rx.avail = fifo_count(&mc->fmt_rx);
	if (mc->fmt_rx.avail)
		wake_lock(&mc->cmd_pipe.wakelock);
	else
		wake_unlock(&mc->cmd_pipe.wakelock);

	mc->rfs_tx.avail = fifo_space(&mc->rfs_tx);
	mc->rfs_rx.avail = fifo_count(&mc->rfs_rx);
	if (mc->rfs_rx.avail)
		wake_lock(&mc->rfs_pipe.wakelock);
	else
		wake_unlock(&mc->rfs_pipe.wakelock);

	mc->raw_tx.avail = fifo_space(&mc->raw_tx);
	mc->raw_rx.avail = fifo_count(&mc->raw_rx);

	/* wake up blocked or polling read/write operations */
	wake_up(&mc->wq);
}

void modem_update_pipe(struct m_pipe *pipe)
{
	unsigned long flags;
	spin_lock_irqsave(&pipe->mc->lock, flags);
	pipe->tx->avail = fifo_space(pipe->tx);
	pipe->rx->avail = fifo_count(pipe->rx);
	if (pipe->rx->avail)
		wake_lock(&pipe->wakelock);
	else
		wake_unlock(&pipe->wakelock);
	spin_unlock_irqrestore(&pipe->mc->lock, flags);
}


/* must be called with pipe->tx_lock held */
static int modem_pipe_send(struct m_pipe *pipe, struct modem_io *io)
{
	char hdr[M_PIPE_MAX_HDR];
	static char ftr = 0x7e;
	unsigned size;
	int ret;

	ret = pipe->push_header(io, hdr);
	if (ret)
		return ret;

	size = io->size + pipe->header_size + 1;

	if (io->size > 0x10000000)
		return -EINVAL;
	if (size >= (pipe->tx->size - 1))
		return -EINVAL;

	for (;;) {
		ret = modem_acquire_mmio(pipe->mc);
		if (ret)
			return ret;

		modem_update_pipe(pipe);

		if (pipe->tx->avail >= size) {
			fifo_write(pipe->tx, hdr, pipe->header_size);
			fifo_write_user(pipe->tx, io->data, io->size);
			fifo_write(pipe->tx, &ftr, 1);
			modem_update_pipe(pipe);
			modem_release_mmio(pipe->mc, pipe->tx->bits);
			MODEM_COUNT(pipe->mc, pipe_tx);
			return 0;
		}

		pr_info("modem_pipe_send: wait for space\n");
		MODEM_COUNT(pipe->mc, pipe_tx_delayed);
		modem_release_mmio(pipe->mc, 0);

		ret = wait_event_interruptible(pipe->mc->wq,
					       (pipe->tx->avail >= size)
					       || modem_offline(pipe->mc));
		if (ret)
			return ret;
	}
}

static int modem_pipe_read(struct m_pipe *pipe, struct modem_io *io)
{
	unsigned data_size = io->size;
	char hdr[M_PIPE_MAX_HDR];
	int ret;

	if (fifo_read(pipe->rx, hdr, pipe->header_size) == 0)
		return -EAGAIN;

	ret = pipe->pull_header(io, hdr);
	if (ret)
		return ret;

	if (data_size < io->size) {
		pr_info("modem_pipe_read: discarding packet (%d)\n", io->size);
		if (fifo_skip(pipe->rx, io->size + 1) != (io->size + 1))
			return -EIO;
		return -EAGAIN;
	} else {
		if (fifo_read_user(pipe->rx, io->data, io->size) != io->size)
			return -EIO;
		if (fifo_skip(pipe->rx, 1) != 1)
			return -EIO;
	}
	return 0;
}

/* must be called with pipe->rx_lock held */
static int modem_pipe_recv(struct m_pipe *pipe, struct modem_io *io)
{
	int ret;

	ret = modem_acquire_mmio(pipe->mc);
	if (ret)
		return ret;

	ret = modem_pipe_read(pipe, io);

	modem_update_pipe(pipe);

	if ((ret != 0) && (ret != -EAGAIN)) {
		pr_err("[MODEM] purging %s fifo\n", pipe->dev.name);
		fifo_purge(pipe->rx);
		MODEM_COUNT(pipe->mc, pipe_rx_purged);
	} else if (ret == 0) {
		MODEM_COUNT(pipe->mc, pipe_rx);
	}

	modem_release_mmio(pipe->mc, 0);

	return ret;
}

struct raw_hdr {
	u8 start;
	u32 len;
	u8 channel;
	u8 control;
} __attribute__ ((packed));

struct vnet {
	struct modemctl *mc;
	struct sk_buff_head txq;
};

static void handle_raw_rx(struct modemctl *mc)
{
	struct raw_hdr raw;
	struct sk_buff *skb = NULL;
	int recvdata = 0;

	/* process inbound packets */
	while (fifo_read(&mc->raw_rx, &raw, sizeof(raw)) == sizeof(raw)) {
		struct net_device *dev = mc->ndev;
		unsigned sz = raw.len - (sizeof(raw) - 1);

		if (unlikely(raw.channel != RAW_CH_VNET0)) {
			MODEM_COUNT(mc, rx_unknown);
			pr_err("[VNET] unknown channel %d\n", raw.channel);
			if (fifo_skip(&mc->raw_rx, sz + 1) != (sz + 1))
				goto purge_raw_fifo;
			continue;
		}

		skb = dev_alloc_skb(sz + NET_IP_ALIGN);
		if (skb == NULL) {
			MODEM_COUNT(mc, rx_dropped);
			/* TODO: consider timer + retry instead of drop? */
			pr_err("[VNET] cannot alloc %d byte packet\n", sz);
			if (fifo_skip(&mc->raw_rx, sz + 1) != (sz + 1))
				goto purge_raw_fifo;
			continue;
		}
		skb->dev = dev;
		skb_reserve(skb, NET_IP_ALIGN);

		if (fifo_read(&mc->raw_rx, skb_put(skb, sz), sz) != sz)
			goto purge_raw_fifo;
		if (fifo_skip(&mc->raw_rx, 1) != 1)
			goto purge_raw_fifo;

		skb->protocol = __constant_htons(ETH_P_IP);
		dev->stats.rx_packets++;
		dev->stats.rx_bytes += skb->len;

		netif_rx(skb);
		recvdata = 1;
		MODEM_COUNT(mc, rx_received);
	}

	if (recvdata)
		wake_lock_timeout(&mc->ip_rx_wakelock, HZ * 2);
	return;

purge_raw_fifo:
	if (skb)
		dev_kfree_skb_irq(skb);
	pr_err("[VNET] purging raw rx fifo!\n");
	fifo_purge(&mc->raw_tx);
	MODEM_COUNT(mc, rx_purged);
}

int handle_raw_tx(struct modemctl *mc, struct sk_buff *skb)
{
	struct raw_hdr raw;
	unsigned char ftr = 0x7e;
	unsigned sz;

	sz = skb->len + sizeof(raw) + 1;

	if (fifo_space(&mc->raw_tx) < sz) {
		MODEM_COUNT(mc, tx_fifo_full);
		return -1;
	}

	raw.start = 0x7f;
	raw.len = 6 + skb->len;
	raw.channel = RAW_CH_VNET0;
	raw.control = 0;

	fifo_write(&mc->raw_tx, &raw, sizeof(raw));
	fifo_write(&mc->raw_tx, skb->data, skb->len);
	fifo_write(&mc->raw_tx, &ftr, 1);

	mc->ndev->stats.tx_packets++;
	mc->ndev->stats.tx_bytes += skb->len;

	mc->mmio_signal_bits |= MBD_SEND_RAW;

	dev_kfree_skb_irq(skb);
	return 0;
}

void modem_handle_io(struct modemctl *mc)
{
	struct sk_buff *skb;
	struct vnet *vn = netdev_priv(mc->ndev);

	handle_raw_rx(mc);

	while ((skb = skb_dequeue(&vn->txq)))
		if (handle_raw_tx(mc, skb)) {
			skb_queue_head(&vn->txq, skb);
			break;
		}
	if (skb == NULL)
		wake_unlock(&vn->mc->ip_tx_wakelock);
}

static int vnet_open(struct net_device *ndev)
{
	netif_start_queue(ndev);
	return 0;
}

static int vnet_stop(struct net_device *ndev)
{
	netif_stop_queue(ndev);
	return 0;
}

static int vnet_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct vnet *vn = netdev_priv(ndev);
	struct modemctl *mc = vn->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);
	if (readl(mc->mmio + OFF_SEM) & 1) {
		/* if we happen to hold the hw mmio sem, transmit NOW */
		if (handle_raw_tx(mc, skb)) {
			wake_lock(&mc->ip_tx_wakelock);
			skb_queue_tail(&vn->txq, skb);
		} else {
			MODEM_COUNT(mc, tx_no_delay);
		}
		if (!mc->mmio_owner) {
			/* if we don't own the semaphore, immediately
			 * give it back to the modem and signal the modem
			 * to process the packet
			 */
			writel(0, mc->mmio + OFF_SEM);
			writel(MB_VALID | MBD_SEND_RAW,
			       mc->mmio + OFF_MBOX_AP);
			MODEM_COUNT(mc, tx_bp_signaled);
		}
	} else {
		/* otherwise request the hw mmio sem and queue */
		modem_request_sem(mc);
		skb_queue_tail(&vn->txq, skb);
		MODEM_COUNT(mc, tx_queued);
	}
	spin_unlock_irqrestore(&mc->lock, flags);

	return NETDEV_TX_OK;
}

static struct net_device_ops vnet_ops = {
	.ndo_open =		vnet_open,
	.ndo_stop =		vnet_stop,
	.ndo_start_xmit =	vnet_xmit,
};

static void vnet_setup(struct net_device *ndev)
{
	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_PPP;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	ndev->hard_header_len = 0;
	ndev->addr_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
}

struct fmt_hdr {
	u8 start;
	u16 len;
	u8 control;
} __attribute__ ((packed));

static int push_fmt_header(struct modem_io *io, void *header)
{
	struct fmt_hdr *fh = header;

	if (io->id)
		return -EINVAL;
	if (io->cmd)
		return -EINVAL;
	fh->start = 0x7f;
	fh->len = io->size + 3;
	fh->control = 0;
	return 0;
}

static int pull_fmt_header(struct modem_io *io, void *header)
{
	struct fmt_hdr *fh = header;

	if (fh->start != 0x7f)
		return -EINVAL;
	if (fh->control != 0x00)
		return -EINVAL;
	if (fh->len < 3)
		return -EINVAL;
	io->size = fh->len - 3;
	io->id = 0;
	io->cmd = 0;
	return 0;
}

struct rfs_hdr {
	u8 start;
	u32 len;
	u8 cmd;
	u8 id;
} __attribute__ ((packed));

static int push_rfs_header(struct modem_io *io, void *header)
{
	struct rfs_hdr *rh = header;

	if (io->id > 0xFF)
		return -EINVAL;
	if (io->cmd > 0xFF)
		return -EINVAL;
	rh->start = 0x7f;
	rh->len = io->size + 6;
	rh->id = io->id;
	rh->cmd = io->cmd;
	return 0;
}

static int pull_rfs_header(struct modem_io *io, void *header)
{
	struct rfs_hdr *rh = header;

	if (rh->start != 0x7f)
		return -EINVAL;
	if (rh->len < 6)
		return -EINVAL;
	io->size = rh->len - 6;
	io->id = rh->id;
	io->cmd = rh->cmd;
	return 0;
}

static int pipe_open(struct inode *inode, struct file *filp)
{
	struct m_pipe *pipe = to_m_pipe(filp->private_data);
	filp->private_data = pipe;
	return 0;
}

static long pipe_ioctl(struct file *filp, unsigned int cmd, unsigned long _arg)
{
	void __user *arg = (void *) _arg;
	struct m_pipe *pipe = filp->private_data;
	struct modem_io mio;
	int ret;

	switch (cmd) {
	case IOCTL_MODEM_SEND:
		if (copy_from_user(&mio, arg, sizeof(mio)) != 0)
			return -EFAULT;
		if (mutex_lock_interruptible(&pipe->tx_lock))
			return -EINTR;
		ret = modem_pipe_send(pipe, &mio);
		mutex_unlock(&pipe->tx_lock);
		return ret;

	case IOCTL_MODEM_RECV:
		if (copy_from_user(&mio, arg, sizeof(mio)) != 0)
			return -EFAULT;
		if (mutex_lock_interruptible(&pipe->rx_lock))
			return -EINTR;
		ret = modem_pipe_recv(pipe, &mio);
		mutex_unlock(&pipe->rx_lock);
		if (copy_to_user(arg, &mio, sizeof(mio)) != 0)
			return -EFAULT;
		return ret;

	default:
		return -EINVAL;
	}
}

static unsigned int pipe_poll(struct file *filp, poll_table *wait)
{
	unsigned long flags;
	struct m_pipe *pipe = filp->private_data;
	int ret;

	poll_wait(filp, &pipe->mc->wq, wait);

	spin_lock_irqsave(&pipe->mc->lock, flags);
	if (pipe->rx->avail || modem_offline(pipe->mc))
		ret = POLLIN | POLLRDNORM;
	else
		ret = 0;
	spin_unlock_irqrestore(&pipe->mc->lock, flags);

	return ret;
}

static const struct file_operations modem_io_fops = {
	.owner =		THIS_MODULE,
	.open =			pipe_open,
	.poll =			pipe_poll,
	.unlocked_ioctl =	pipe_ioctl,
};

static int modem_pipe_register(struct m_pipe *pipe, const char *devname)
{
	pipe->dev.minor = MISC_DYNAMIC_MINOR;
	pipe->dev.name = devname;
	pipe->dev.fops = &modem_io_fops;

	wake_lock_init(&pipe->wakelock, WAKE_LOCK_SUSPEND, devname);

	mutex_init(&pipe->tx_lock);
	mutex_init(&pipe->rx_lock);
	return misc_register(&pipe->dev);
}

int modem_io_init(struct modemctl *mc, void __iomem *mmio)
{
	struct net_device *ndev;
	struct vnet *vn;
	int r;

	INIT_M_FIFO(mc->fmt_tx, FMT, TX, mmio);
	INIT_M_FIFO(mc->fmt_rx, FMT, RX, mmio);
	INIT_M_FIFO(mc->raw_tx, RAW, TX, mmio);
	INIT_M_FIFO(mc->raw_rx, RAW, RX, mmio);
	INIT_M_FIFO(mc->rfs_tx, RFS, TX, mmio);
	INIT_M_FIFO(mc->rfs_rx, RFS, RX, mmio);

	ndev = alloc_netdev(0, "rmnet%d", vnet_setup);
	if (ndev) {
		vn = netdev_priv(ndev);
		vn->mc = mc;
		skb_queue_head_init(&vn->txq);
		r = register_netdev(ndev);
		if (r)
			free_netdev(ndev);
		else
			mc->ndev = ndev;
	}

	mc->cmd_pipe.tx = &mc->fmt_tx;
	mc->cmd_pipe.rx = &mc->fmt_rx;
	mc->cmd_pipe.tx->bits = MBD_SEND_FMT;
	mc->cmd_pipe.push_header = push_fmt_header;
	mc->cmd_pipe.pull_header = pull_fmt_header;
	mc->cmd_pipe.header_size = sizeof(struct fmt_hdr);
	mc->cmd_pipe.mc = mc;
	if (modem_pipe_register(&mc->cmd_pipe, "modem_fmt"))
		pr_err("failed to register modem_fmt pipe\n");

	mc->rfs_pipe.tx = &mc->rfs_tx;
	mc->rfs_pipe.rx = &mc->rfs_rx;
	mc->rfs_pipe.tx->bits = MBD_SEND_RFS;
	mc->rfs_pipe.push_header = push_rfs_header;
	mc->rfs_pipe.pull_header = pull_rfs_header;
	mc->rfs_pipe.header_size = sizeof(struct rfs_hdr);
	mc->rfs_pipe.mc = mc;
	if (modem_pipe_register(&mc->rfs_pipe, "modem_rfs"))
		pr_err("failed to register modem_rfs pipe\n");

	return 0;
}
