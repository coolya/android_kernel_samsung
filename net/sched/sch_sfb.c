/*
 * sch_sfb.c    Stochastic Fair Blue
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Juliusz Chroboczek <jch@pps.jussieu.fr>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/random.h>
#include <linux/jhash.h>
#include <net/ip.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>
#include <net/sfb.h>

struct bucket {
        u16 qlen;
        u16 pm;
};

struct sfb_sched_data
{
        u16 numhashes, numbuckets;
        u16 rehash_interval, db_interval;
        u16 max, target;
        u16 increment, decrement;
        u32 limit;
        u32 penalty_rate, penalty_burst;
        u32 tokens_avail;
        psched_time_t rehash_time, token_time;

        u8 hash_type;
        u8 filter;
        u8 double_buffering;
        u32 perturbation[2][MAXHASHES];
        struct bucket buckets[2][MAXHASHES][MAXBUCKETS];
	struct Qdisc *qdisc;

        __u32 earlydrop, penaltydrop, bucketdrop, queuedrop, marked;
};

static unsigned sfb_hash(struct sk_buff *skb, int hash, int filter,
                         struct sfb_sched_data *q)
{
	u32 h, h2;
        u8 hash_type = q->hash_type;

	switch (skb->protocol) {
	case htons(ETH_P_IP):
	{
		const struct iphdr *iph = ip_hdr(skb);
		h = hash_type == SFB_HASH_SOURCE ? 0 : iph->daddr;
		h2 = hash_type == SFB_HASH_DEST ? 0 : iph->saddr;
		if (hash_type == SFB_HASH_FLOW) {
                        h2 ^= iph->protocol;
                        if(!(iph->frag_off&htons(IP_MF|IP_OFFSET)) &&
                           (iph->protocol == IPPROTO_TCP ||
                            iph->protocol == IPPROTO_UDP ||
                            iph->protocol == IPPROTO_UDPLITE ||
                            iph->protocol == IPPROTO_SCTP ||
                            iph->protocol == IPPROTO_DCCP ||
                            iph->protocol == IPPROTO_ESP)) {
                                h2 ^= *(((u32*)iph) + iph->ihl);
                        }
                }
		break;
	}
	case htons(ETH_P_IPV6):
	{
		struct ipv6hdr *iph = ipv6_hdr(skb);
		h = hash_type == SFB_HASH_SOURCE ? 0 :
                        iph->daddr.s6_addr32[1] ^ iph->daddr.s6_addr32[3];
		h2 = hash_type == SFB_HASH_DEST ? 0 :
                        iph->saddr.s6_addr32[1] ^ iph->saddr.s6_addr32[3];
                if (hash_type == SFB_HASH_FLOW) {
                        h2 ^= iph->nexthdr;
                        if(iph->nexthdr == IPPROTO_TCP ||
                           iph->nexthdr == IPPROTO_UDP ||
                           iph->nexthdr == IPPROTO_UDPLITE ||
                           iph->nexthdr == IPPROTO_SCTP ||
                           iph->nexthdr == IPPROTO_DCCP ||
                           iph->nexthdr == IPPROTO_ESP)
                                h2 ^= *(u32*)&iph[1];
                }
		break;
	}
	default:
		h = skb->protocol;
                if(hash_type != SFB_HASH_SOURCE)
			h ^= (u32)(unsigned long)skb_dst(skb);
                h2 = hash_type == SFB_HASH_FLOW ?
                        (u32)(unsigned long)skb->sk : 0;
	}

        return jhash_2words(h, h2, q->perturbation[filter][hash]) %
                q->numbuckets;

}

static inline u16 prob_plus(u16 p1, u16 p2)
{
        return p1 < SFB_MAX_PROB - p2 ? p1 + p2 : SFB_MAX_PROB;
}

static inline u16 prob_minus(u16 p1, u16 p2)
{
        return p1 > p2 ? p1 - p2 : 0;
}

static void
increment_one_qlen(struct sk_buff *skb, int filter, struct sfb_sched_data *q)
{
        int i;
        for(i = 0; i < q->numhashes; i++) {
                unsigned hash = sfb_hash(skb, i, filter, q);
                if(q->buckets[filter][i][hash].qlen < 0xFFFF)
                        q->buckets[filter][i][hash].qlen++;
        }
}

static void
increment_qlen(struct sk_buff *skb, struct sfb_sched_data *q)
{
        increment_one_qlen(skb, q->filter, q);
        if(q->double_buffering)
                increment_one_qlen(skb, !q->filter, q);
}

static void
decrement_one_qlen(struct sk_buff *skb, int filter, struct sfb_sched_data *q)
{
        int i;
        for(i = 0; i < q->numhashes; i++) {
                unsigned hash = sfb_hash(skb, i, filter, q);
                if(q->buckets[filter][i][hash].qlen > 0)
                    q->buckets[filter][i][hash].qlen--;
        }
}

static void
decrement_qlen(struct sk_buff *skb, struct sfb_sched_data *q)
{
        decrement_one_qlen(skb, q->filter, q);
        if(q->double_buffering)
                decrement_one_qlen(skb, !q->filter, q);
}

static inline void
decrement_prob(int filter, int bucket, unsigned hash, struct sfb_sched_data *q)
{
        q->buckets[filter][bucket][hash].pm =
                prob_minus(q->buckets[filter][bucket][hash].pm,
                           q->decrement);
}

static inline void
increment_prob(int filter, int bucket, unsigned hash, struct sfb_sched_data *q)
{
        q->buckets[filter][bucket][hash].pm =
                prob_plus(q->buckets[filter][bucket][hash].pm,
                           q->increment);
}

static void
zero_all_buckets(int filter, struct sfb_sched_data *q)
{
        int i, j;
        for(i = 0; i < MAXHASHES; i++) {
                for(j = 0; j < MAXBUCKETS; j++) {
                        q->buckets[filter][i][j].pm = 0;
                        q->buckets[filter][i][j].qlen = 0;
                }
        }
}

static void
compute_qlen(u16 *qlen_r, u16 *prob_r, struct sfb_sched_data *q)
{
        int i, j, filter = q->filter;
        u16 qlen = 0, prob = 0;
        for(i = 0; i < q->numhashes; i++) {
                for(j = 0; j < q->numbuckets; j++) {
                        if(qlen < q->buckets[filter][i][j].qlen)
                                qlen = q->buckets[filter][i][j].qlen;
                        if(qlen < q->buckets[filter][i][j].pm)
                                prob = q->buckets[filter][i][j].pm;
                }
        }
        *qlen_r = qlen;
        *prob_r = prob;
}


static void
init_perturbation(int filter, struct sfb_sched_data *q)
{
        get_random_bytes(q->perturbation[filter],
                         sizeof(q->perturbation[filter]));
}

static void
swap_buffers(struct sfb_sched_data *q)
{
        q->filter = !q->filter;
        zero_all_buckets(!q->filter, q);
        init_perturbation(!q->filter, q);
        q->double_buffering = 0;
}

static int rate_limit(struct sk_buff *skb, psched_time_t now,
                      struct sfb_sched_data* q)
{
        if(q->penalty_rate == 0 || q->penalty_burst == 0)
                return 1;

        if(q->tokens_avail < 1) {
                psched_tdiff_t age;

                age = psched_tdiff_bounded(now, q->token_time,
                                           256 * PSCHED_TICKS_PER_SEC);
                q->tokens_avail =
                        (age * q->penalty_rate / PSCHED_TICKS_PER_SEC);
                if(q->tokens_avail > q->penalty_burst)
                        q->tokens_avail = q->penalty_burst;
                q->token_time = now;
                if(q->tokens_avail < 1)
                        return 1;
        }

        q->tokens_avail--;
        return 0;
}

static int sfb_enqueue(struct sk_buff *skb, struct Qdisc* sch)
{

	struct sfb_sched_data *q = qdisc_priv(sch);
        struct Qdisc *child = q->qdisc;
        psched_time_t now;
        int filter;
        u16 minprob = SFB_MAX_PROB;
        u16 minqlen = (u16)(~0);
        u32 r;
        int ret, i;

        now = psched_get_time();

        if(q->rehash_interval > 0) {
                psched_tdiff_t age;
                age = psched_tdiff_bounded(now, q->rehash_time,
                                           q->rehash_interval *
                                           PSCHED_TICKS_PER_SEC);
                if(unlikely(age >= q->rehash_interval * PSCHED_TICKS_PER_SEC)) {
                        swap_buffers(q);
                        q->rehash_time = now;
                }
                if(unlikely(!q->double_buffering && q->db_interval > 0 &&
                            age >= (q->rehash_interval - q->db_interval) *
                            PSCHED_TICKS_PER_SEC))
                        q->double_buffering = 1;
        }

        filter = q->filter;

        for(i = 0; i < q->numhashes; i++) {
                unsigned hash = sfb_hash(skb, i, filter, q);
                if(q->buckets[filter][i][hash].qlen == 0)
                        decrement_prob(filter, i, hash, q);
                else if(unlikely(q->buckets[filter][i][hash].qlen >= q->target))
                        increment_prob(filter, i, hash, q);
                if(minqlen > q->buckets[filter][i][hash].qlen)
                        minqlen = q->buckets[filter][i][hash].qlen;
                if(minprob > q->buckets[filter][i][hash].pm)
                        minprob = q->buckets[filter][i][hash].pm;
        }

        if(q->double_buffering) {
                for(i = 0; i < q->numhashes; i++) {
                        unsigned hash = sfb_hash(skb, i, !filter, q);
                        if(q->buckets[!filter][i][hash].qlen == 0)
                                decrement_prob(!filter, i, hash, q);
                        else if(unlikely(q->buckets[!filter][i][hash].qlen >=
                                         q->target))
                                increment_prob(!filter, i, hash, q);
                }
        }
        
        if(unlikely(minqlen >= q->max || sch->q.qlen >= q->limit)) {
                sch->qstats.overlimits++;
                if(likely(minqlen >= q->max))
                        q->bucketdrop++;
                else
                        q->queuedrop++;
                goto drop;
        }

        if(unlikely(minprob >= SFB_MAX_PROB)) {
                /* Inelastic flow */
                if(rate_limit(skb, now, q)) {
                        sch->qstats.overlimits++;
                        q->penaltydrop++;
                        goto drop;
                }
                goto enqueue;
        }

        r = net_random() & SFB_MAX_PROB;

        if(unlikely(r < minprob)) {
                if(unlikely(minprob > SFB_MAX_PROB / 2)) {
                        /* If we're marking that many packets, then either
                           this flow is unresponsive, or we're badly congested.
                           In either case, we want to start dropping packets. */
                        if(r < (minprob - SFB_MAX_PROB / 2) * 2) {
                                q->earlydrop++;
                                goto drop;
                        }
                }
                if(INET_ECN_set_ce(skb)) {
                        q->marked++;
                } else {
                        q->earlydrop++;
                        goto drop;
                }
        }

 enqueue:
        ret = qdisc_enqueue(skb, child);
        if(likely(ret == NET_XMIT_SUCCESS)) {
                sch->q.qlen++;
                increment_qlen(skb, q);
		sch->bstats.packets++;
		sch->bstats.bytes += skb->len;
                sch->qstats.backlog += skb->len;
        } else if(net_xmit_drop_count(ret)) {
                q->queuedrop++;
                sch->qstats.drops++;
        }
        return ret;

 drop:
        qdisc_drop(skb, sch);
        return NET_XMIT_CN;
}

static struct sk_buff *sfb_dequeue(struct Qdisc* sch)
{
	struct sfb_sched_data *q = qdisc_priv(sch);
        struct Qdisc *child = q->qdisc;
	struct sk_buff *skb;

	skb = child->dequeue(q->qdisc);

        if(skb) {
                sch->q.qlen--;
		sch->qstats.backlog -= skb->len;
                decrement_qlen(skb, q);
        }

        return skb;
}

static struct sk_buff *sfb_peek(struct Qdisc* sch)
{
	struct sfb_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;

	return child->ops->peek(child);
}

/* No sfb_drop -- impossible since the child doesn't return the dropped skb. */

static void sfb_reset(struct Qdisc* sch)
{
	struct sfb_sched_data *q = qdisc_priv(sch);

	qdisc_reset(q->qdisc);
        sch->q.qlen = 0;
        sch->qstats.backlog = 0;
        q->filter = 0;
        q->double_buffering = 0;
        zero_all_buckets(0, q);
        zero_all_buckets(1, q);
        init_perturbation(q->filter, q);
}

static void sfb_destroy(struct Qdisc *sch)
{
	struct sfb_sched_data *q = qdisc_priv(sch);
	qdisc_destroy(q->qdisc);
        q->qdisc = NULL;
}

static const struct nla_policy sfb_policy[TCA_SFB_MAX + 1] = {
	[TCA_SFB_PARMS]	= { .len = sizeof(struct tc_sfb_qopt) },
};

static int sfb_change(struct Qdisc *sch, struct nlattr *opt)
{
        struct sfb_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = NULL;
	struct nlattr *tb[TCA_SFB_MAX + 1];
	struct tc_sfb_qopt *ctl;
        u16 numhashes, numbuckets, rehash_interval, db_interval;
        u8 hash_type;
        u32 limit;
        u16 max, target;
        u16 increment, decrement;
        u32 penalty_rate, penalty_burst;
	int err;

	if (opt == NULL) {
                numhashes = 6;
                numbuckets = 32;
                hash_type = SFB_HASH_FLOW;
                rehash_interval = 600;
                db_interval = 60;
                limit = 0;
                max = 25;
                target = 20;
                increment = (SFB_MAX_PROB + 1000) / 2000;
                decrement = (SFB_MAX_PROB + 10000) / 20000;
                penalty_rate = 10;
                penalty_burst = 20;
        } else {
                err = nla_parse_nested(tb, TCA_SFB_MAX, opt, sfb_policy);
		if(err < 0)
                        return -EINVAL;

                if (tb[TCA_SFB_PARMS] == NULL)
		    return -EINVAL;

                ctl = nla_data(tb[TCA_SFB_PARMS]);

                numhashes = ctl->numhashes;
                numbuckets = ctl->numbuckets;
                rehash_interval = ctl->rehash_interval;
                db_interval = ctl->db_interval;
                hash_type = ctl->hash_type;
                limit = ctl->limit;
                max = ctl->max;
                target = ctl->target;
                increment = ctl->increment;
                decrement = ctl->decrement;
                penalty_rate = ctl->penalty_rate;
                penalty_burst = ctl->penalty_burst;
        }

        if(numbuckets <= 0 || numbuckets > MAXBUCKETS)
                numbuckets = MAXBUCKETS;
        if(numhashes <= 0 || numhashes > MAXHASHES)
                numhashes = MAXHASHES;
        if(hash_type >= __SFB_HASH_MAX)
                hash_type = SFB_HASH_FLOW;
        if(limit == 0)
                limit = qdisc_dev(sch)->tx_queue_len;
	if(limit == 0)
		limit = 1;

        child = fifo_create_dflt(sch, &pfifo_qdisc_ops, limit);
        if(IS_ERR(child))
		return PTR_ERR(child);
                
        sch_tree_lock(sch);
	if(child) {
		qdisc_tree_decrease_qlen(q->qdisc, q->qdisc->q.qlen);
		qdisc_destroy(q->qdisc);
		q->qdisc = child;
	}

        q->numhashes = numhashes;
        q->numbuckets = numbuckets;
        q->rehash_interval = rehash_interval;
        q->db_interval = db_interval;
        q->hash_type = hash_type;
        q->limit = limit;
        q->increment = increment;
        q->decrement = decrement;
        q->max = max;
        q->target = target;
        q->penalty_rate = penalty_rate;
        q->penalty_burst = penalty_burst;

        q->filter = 0;
        q->double_buffering = 0;
        zero_all_buckets(0, q);
        zero_all_buckets(1, q);
        init_perturbation(q->filter, q);

	sch_tree_unlock(sch);

	return 0;
}

static int sfb_init(struct Qdisc *sch, struct nlattr *opt)
{
      	struct sfb_sched_data *q = qdisc_priv(sch);

        q->qdisc = &noop_qdisc;
        return sfb_change(sch, opt);
}

static int sfb_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct sfb_sched_data *q = qdisc_priv(sch);
	struct nlattr *opts = NULL;
	struct tc_sfb_qopt opt = { .numhashes = q->numhashes,
                                   .numbuckets = q->numbuckets,
                                   .rehash_interval = q->rehash_interval,
                                   .db_interval = q->db_interval,
                                   .hash_type = q->hash_type,
                                   .limit = q->limit,
                                   .max = q->max,
                                   .target = q->target,
                                   .increment = q->increment,
                                   .decrement = q->decrement,
                                   .penalty_rate = q->penalty_rate,
                                   .penalty_burst = q->penalty_burst,
        };

	opts = nla_nest_start(skb, TCA_OPTIONS);
	NLA_PUT(skb, TCA_SFB_PARMS, sizeof(opt), &opt);
	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -EMSGSIZE;
}

static int sfb_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct sfb_sched_data *q = qdisc_priv(sch);
	struct tc_sfb_xstats st = {
		.earlydrop = q->earlydrop,
		.penaltydrop = q->penaltydrop,
		.bucketdrop = q->bucketdrop,
		.marked = q->marked
	};

        compute_qlen(&st.maxqlen, &st.maxprob, q);

	return gnet_stats_copy_app(d, &st, sizeof(st));
}
        
static int sfb_dump_class(struct Qdisc *sch, unsigned long cl,
			  struct sk_buff *skb, struct tcmsg *tcm)
{
        return -ENOSYS;
}

static int sfb_graft(struct Qdisc *sch, unsigned long arg, struct Qdisc *new,
		     struct Qdisc **old)
{
	struct sfb_sched_data *q = qdisc_priv(sch);

	if (new == NULL)
		new = &noop_qdisc;

	sch_tree_lock(sch);
	*old = xchg(&q->qdisc, new);
	qdisc_tree_decrease_qlen(*old, (*old)->q.qlen);
	qdisc_reset(*old);
	sch_tree_unlock(sch);
	return 0;
}

static struct Qdisc *sfb_leaf(struct Qdisc *sch, unsigned long arg)
{
	struct sfb_sched_data *q = qdisc_priv(sch);
	return q->qdisc;
}

static unsigned long sfb_get(struct Qdisc *sch, u32 classid)
{
	return 1;
}

static void sfb_put(struct Qdisc *sch, unsigned long arg)
{
	return;
}

static int sfb_change_class(struct Qdisc *sch, u32 classid, u32 parentid,
			    struct nlattr **tca, unsigned long *arg)
{
	return -ENOSYS;
}

static int sfb_delete(struct Qdisc *sch, unsigned long cl)
{
	return -ENOSYS;
}

static void sfb_walk(struct Qdisc *sch, struct qdisc_walker *walker)
{
	if (!walker->stop) {
		if (walker->count >= walker->skip)
			if (walker->fn(sch, 1, walker) < 0) {
				walker->stop = 1;
				return;
			}
		walker->count++;
	}
}

static struct Qdisc_class_ops sfb_class_ops =
{
	.graft		=	sfb_graft,
	.leaf		=	sfb_leaf,
	.get		=	sfb_get,
	.put		=	sfb_put,
	.change		=	sfb_change_class,
	.delete		=	sfb_delete,
	.walk		=	sfb_walk,
	.dump		=	sfb_dump_class,
};

struct Qdisc_ops sfb_qdisc_ops = {
	.id		=	"sfb",
	.priv_size	=	sizeof(struct sfb_sched_data),
	.cl_ops		=	&sfb_class_ops,
	.enqueue	=	sfb_enqueue,
	.dequeue	=	sfb_dequeue,
	.peek		=	sfb_peek,
	.init		=	sfb_init,
	.reset		=	sfb_reset,
	.destroy	=	sfb_destroy,
	.change		=	sfb_change,
	.dump		=	sfb_dump,
	.dump_stats	=	sfb_dump_stats,
	.owner		=	THIS_MODULE,
};

static int __init sfb_module_init(void)
{
	return register_qdisc(&sfb_qdisc_ops);
}

static void __exit sfb_module_exit(void)
{
	unregister_qdisc(&sfb_qdisc_ops);
}

module_init(sfb_module_init)
module_exit(sfb_module_exit)

MODULE_DESCRIPTION("Stochastic Fair Blue queue discipline");
MODULE_AUTHOR("Juliusz Chroboczek");
MODULE_LICENSE("GPL");
