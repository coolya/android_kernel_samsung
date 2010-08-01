/* linux/drivers/video/samsung/s3cfb_lvds.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 **/
#include "s3cfb.h"

#if 0
static struct s3cfb_lcd lvds = {
        .width = 1024,
        .height = 600,
        .bpp = 32,
        .freq = 60,

        .timing = {
                 .h_fp = 79,
                .h_bp = 200,
                .h_sw = 40,
                .v_fp = 10,
                .v_fpe = 1,
                .v_bp = 11,
                .v_bpe = 1,
                .v_sw = 10,
	
        },

        .polarity = {
                .rise_vclk = 0,
                .inv_hsync = 1,
                .inv_vsync = 1,
                .inv_vden = 0,
        },
}
/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info_lvds(struct s3cfb_global *ctrl)
{
	lvds.init_ldi = NULL;
	ctrl->lcd = &lvds;
}
#endif

//#define MDNIE_TUNING

#ifdef MDNIE_TUNING

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define MDNIE_TUNING_TUNEFILE_PATH	"/sdcard/sd/p1/mdnie_tune"
#define TUNE_TEXT_MAX_LINES			100
#define TUNE_MAX_VALUES				200

unsigned short *test[1];
EXPORT_SYMBOL(test);

extern void mDNIe_txtbuf_to_parsing(void);

static int parse_from_text(char * src, int len, unsigned short * output)
{
	int i,count, ret;
	int out_index=0;
	char * str_line[TUNE_TEXT_MAX_LINES];
	char * sstart;
	char * c;
	unsigned int data1, data2;

	c = src;
	count = 0;
	sstart = c;
	for(i=0; i<len; i++,c++)
		{
		char a = *c;
		if(a=='\r' || a=='\n')
			{
			if(c > sstart)
				{
				str_line[count] = sstart;
				count++;
				}
			*c='\0';
			sstart = c+1;
			}
		}
	if(c > sstart)
		{
		str_line[count] = sstart;
		count++;
		}

	printk("<MDNIE> %s lines:%d\n", __func__, count);

	for(i=0; i<count; i++)
		{
		printk("line:%d, string:%s[end]\n", i, str_line[i]);
		ret = sscanf(str_line[i], "0x%x,0x%x\n", &data1, &data2);
		printk("line:%d, num:%d, d1:0x%04x, d2:0x%04x\n", i, ret, data1, data2);
		if(ret == 2)
			{
			output[out_index++] = data1;
			output[out_index++] = data2;
			}
		}

	output[out_index] = 0xffff;		// END_SEQ

	return out_index;

}

int mDNIe_tuning_load_from_file(void)
{
	struct file	*filp;
	char		*dp;
	long		l;
	loff_t		pos;
	int		i;
	int		ret;

	printk("<MDNIE> %s start.\n", __func__);

	mm_segment_t fs	= get_fs();

	set_fs(get_ds());

	filp = filp_open(MDNIE_TUNING_TUNEFILE_PATH, O_RDONLY, 0);

	if(IS_ERR(filp))
	{
		printk("<MDNIE> file open error:%d\n", filp);
		return -1;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	printk("<MDNIE> l = %ld\n", l);

	//dp = kmalloc(l, GFP_KERNEL);
	dp = kmalloc(l+10, GFP_KERNEL);		// add cushion
	if(dp == NULL)
	{
		printk("<MDNIE> Out of Memory\n");
		filp_close(filp, current->files);
		return -1;
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);

	if(ret != l)
	{
		printk("<MDNIE> Failed to read file ret = %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	for(i=0; i<l; i++)
		{
		printk("%x ", dp[i]);
		}
	printk("\n");

	test[0] = kmalloc(TUNE_MAX_VALUES*sizeof(short), GFP_KERNEL);
	if(test[0] == NULL)
		{
		printk("<MDNIE> Out of Memory\n");
		kfree(dp);
		return -1;
		}

	ret = parse_from_text(dp, l, test[0]);
	printk("<MDNIE> parsing data:%d\n", ret);

	if(ret > 0)
		{
		printk("<MDNIE> Call mDNIe_txtbuf_to_parsing\n");
		mDNIe_txtbuf_to_parsing();
		}

	kfree(test[0]);
	kfree(dp);

	return ret;
//	printk("<=PCAM=> sr200pc10_regs_table 0x%x, %ld\n", (unsigned int)dp, l);
}
EXPORT_SYMBOL(mDNIe_tuning_load_from_file);


#endif

MODULE_LICENSE("GPL");

