/* modem_dbg.c
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

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "modem_ctl.h"
#include "modem_ctl_p.h"

static int crash_open(struct inode *inode, struct file *file)
{
	struct modemctl *mc = inode->i_private;
	modem_force_crash(mc);
	return 0;
}

static const struct file_operations crash_ops = {
	.open = crash_open,
};

#define SHOW(name) seq_printf(sf, "%-20s %d\n", #name, stats->name)

static int stats_show(struct seq_file *sf, void *unused)
{
	struct modemstats *stats = sf->private;

	SHOW(request_no_wait);
	SHOW(request_wait);

	SHOW(release_no_action);
	SHOW(release_bp_waiting);
	SHOW(release_bp_signaled);

	SHOW(bp_req_instant);
	SHOW(bp_req_delayed);
	SHOW(bp_req_confused);

	SHOW(rx_unknown);
	SHOW(rx_dropped);
	SHOW(rx_purged);
	SHOW(rx_received);

	SHOW(tx_no_delay);
	SHOW(tx_queued);
	SHOW(tx_bp_signaled);
	SHOW(tx_fifo_full);

	SHOW(pipe_tx);
	SHOW(pipe_rx);
	SHOW(pipe_tx_delayed);
	SHOW(pipe_rx_purged);

	SHOW(resets);

	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	struct modemctl *mc = inode->i_private;
	struct modemstats *stats;
	unsigned long int flags;
	int ret;

	stats = kzalloc(sizeof(*stats), GFP_KERNEL);
	if (!stats)
		return -ENOMEM;

	spin_lock_irqsave(&mc->lock, flags);
	memcpy(stats, &mc->stats, sizeof(*stats));
	memset(&mc->stats, 0, sizeof(*stats));
	spin_unlock_irqrestore(&mc->lock, flags);

	ret = single_open(file, stats_show, stats);
	if (ret)
		kfree(stats);

	return ret;
}

static int stats_release(struct inode *inode, struct file *file)
{
	struct seq_file *seq = file->private_data;
	struct modemstats *stats = seq->private;
	int ret;
	ret = single_release(inode, file);
	kfree(stats);
	return ret;
}

static const struct file_operations stats_ops = {
	.open = stats_open,
	.release = stats_release,
	.read = seq_read,
	.llseek = seq_lseek,
};

static int log_open(struct inode *inode, struct file *file)
{
	struct modemctl *mc = inode->i_private;
	unsigned long int flags;
	int ret = 0;

	file->private_data = mc;

	mutex_lock(&mc->ctl_lock);
	if (mc->logdump) {
		mutex_unlock(&mc->ctl_lock);
		return -EBUSY;
	}
	mc->logdump = 1;
	mutex_unlock(&mc->ctl_lock);

	spin_lock_irqsave(&mc->lock, flags);
	mc->logdump_data = 0;
	pr_err("modem: send LOGDUMP\n");
	writel(MODEM_CMD_LOGDUMP_START, mc->mmio + OFF_MBOX_AP);
	spin_unlock_irqrestore(&mc->lock, flags);

	ret = wait_event_timeout(mc->wq, mc->logdump_data, 10 * HZ);
	if (ret == 0) {
		mutex_lock(&mc->ctl_lock);
		mc->logdump = 0;
		mutex_unlock(&mc->ctl_lock);
		return -ETIMEDOUT;
	} else {
		return 0;
	}
}

static int log_release(struct inode *inode, struct file *file)
{
	struct modemctl *mc = file->private_data;
	mutex_lock(&mc->ctl_lock);
	mc->logdump = 0;
	mutex_unlock(&mc->ctl_lock);
	return 0;
}

static ssize_t log_read(struct file *filp, char __user *buf,
			size_t count, loff_t *ppos)
{
	struct modemctl *mc = filp->private_data;
	loff_t pos = *ppos;
	int ret;

	if (pos < 0)
		return -EINVAL;
	if (pos >= SIZ_LOGDUMP_DATA)
		return 0;

	ret = modem_acquire_mmio(mc);
	if (ret)
		return ret;

	mutex_lock(&mc->ctl_lock);
	if (count > SIZ_LOGDUMP_DATA - pos)
		count = SIZ_LOGDUMP_DATA - pos;
	ret = copy_to_user(buf, mc->mmio + OFF_LOGDUMP_DATA + pos, count);
	if (ret) {
		ret = -EFAULT;
	} else {
		pos += count;
		ret = count;
	}
	*ppos = pos;
	mutex_unlock(&mc->ctl_lock);

	modem_release_mmio(mc, 0);
	return ret;
}

static const struct file_operations log_ops = {
	.open = log_open,
	.release = log_release,
	.read = log_read,
};

void modem_debugfs_init(struct modemctl *mc)
{
	struct dentry *dent;

	dent = debugfs_create_dir("modemctl", 0);
	if (IS_ERR(dent))
		return;

	debugfs_create_file("crash", 0200, dent, mc, &crash_ops);
	debugfs_create_file("stats", 0444, dent, mc, &stats_ops);
	debugfs_create_file("log", 0440, dent, mc, &log_ops);
}
