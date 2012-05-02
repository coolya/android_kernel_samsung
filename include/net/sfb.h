/*
 * sfb.h        Stochastic Fair Blue
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Juliusz Chroboczek <jch@pps.jussieu.fr>
 */

#include <linux/types.h>

#define MAXHASHES 12
#define MAXBUCKETS 32

enum
{
	TCA_SFB_UNSPEC,
	TCA_SFB_PARMS,
	__TCA_SFB_MAX,
};

#define TCA_SFB_MAX (__TCA_SFB_MAX - 1)

enum {
        SFB_HASH_FLOW,
        SFB_HASH_SOURCE,
        SFB_HASH_DEST,
        SFB_HASH_SOURCE_DEST,
        __SFB_HASH_MAX,
};

struct tc_sfb_qopt
{
        __u8 hash_type, pad;
        __u16 numhashes, numbuckets;
        __u16 rehash_interval, db_interval;
        __u16 max, target;
        __u16 increment, decrement;
        __u16 pad2;
        __u32 limit;
        __u32 penalty_rate, penalty_burst;
};

struct tc_sfb_xstats
{
        __u32 earlydrop, penaltydrop, bucketdrop, queuedrop, marked;
        __u16 maxqlen, maxprob;
};

#define SFB_MAX_PROB 0xFFFF
