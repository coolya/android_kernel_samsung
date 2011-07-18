 
 /*****************************************************************************
 *
 * COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2015 ALL RIGHTS RESERVED
 *
 *****************************************************************************/
 


#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>

#define YAMAHA_GSENSOR_TRANSFORMATION_EMUL    \
    { { 1,  0,  0}, \
      { 0,  -1,  0}, \
      { 0,  0,  1} }

#define YAMAHA_MSENSOR_TRANSFORMATION_EMUL    \
    { { 0,  -1,  0}, \
      { -1,  0,   0}, \
      { 0 , 0 ,  -1} }

#define YAMAHA_GSENSOR_TRANSFORMATION    \
    {{ -1,    0,    0}, \
      {   0,  -1,    0}, \
      {   0,    0,  -1} }

#define YAMAHA_MSENSOR_TRANSFORMATION    \
    {{ 0,  1,   0}, \
      { 1,  0,   0}, \
      { 0 , 0, -1} }
      
#define YAMAHA_IOCTL_GET_MARRAY            _IOR('Y', 0x01, char[9])
#define YAMAHA_IOCTL_GET_GARRAY            _IOR('Y', 0x02, char[9])

extern unsigned int HWREV;

static int yamaha_open(struct inode *inode, struct file *file)
{
	//printk(" yamaha_open() entered\n");
	return 0;
}

static int 
yamaha_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{

	signed char marray[3][3] = YAMAHA_MSENSOR_TRANSFORMATION;
	signed char garray[3][3] = YAMAHA_GSENSOR_TRANSFORMATION;
	signed char marray_emul[3][3] = YAMAHA_MSENSOR_TRANSFORMATION_EMUL;
	signed char garray_emul[3][3] = YAMAHA_GSENSOR_TRANSFORMATION_EMUL;

	switch (cmd) {
		case YAMAHA_IOCTL_GET_MARRAY:
          if (copy_to_user((void *)arg, marray, sizeof(marray))) 
          {
            printk("YAMAHA_MSENSOR_TRANSFORMATION copy failed\n");
            return -EFAULT;
          }
		  break;

        case YAMAHA_IOCTL_GET_GARRAY:
          if (copy_to_user((void *)arg, garray, sizeof(garray))) 
          {
            printk("YAMAHA_GSENSOR_TRANSFORMATION copy failed\n");
            return -EFAULT;
          }
          break;

        default:
          return -ENOTTY;
	}

	return 0;
}

static int yamaha_release(struct inode *inode, struct file *file)
{
	//printk(" yamaha_release() entered\n");
	return 0;
}

static struct file_operations yamaha_fops = {
	.owner = THIS_MODULE,
	.open = yamaha_open,
	.release = yamaha_release,
	.ioctl = yamaha_ioctl,
};


static struct miscdevice yamaha_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "yamaha_compass",
	.fops = &yamaha_fops,
};

static int __init yamaha_init(void)
{
	int err;
	printk("yamaha_init\n");
	err = misc_register(&yamaha_device);
	if (err) {
		printk("yamaha_init Cannot register miscdev \n",err);
	}
	return err;
}

static void __exit yamaha_exit(void)
{
	misc_deregister(&yamaha_device);
	printk("__exit\n");
}

module_init(yamaha_init);
module_exit(yamaha_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("yamaha compass driver");
MODULE_LICENSE("GPL");
