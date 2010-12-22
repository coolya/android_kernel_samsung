 
 /*****************************************************************************
 *
 * COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2015 ALL RIGHTS RESERVED
 *
 *****************************************************************************/


//#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
//#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>

#if defined(CONFIG_SAMSUNG_CAPTIVATE)
#define YAMAHA_GSENSOR_TRANSFORMATION    \
    { { 0,  1,  0}, \
      { -1,  0,  0}, \
      { 0,  0,  -1} }
#else
#define YAMAHA_GSENSOR_TRANSFORMATION    \
    { { -1,  0,  0}, \
      { 0,  -1,  0}, \
      { 0,  0,  -1} }
#endif

#if defined(CONFIG_SAMSUNG_CAPTIVATE)
#define YAMAHA_MSENSOR_TRANSFORMATION    \
    { { 0,  1,  0}, \
      { -1,  0,   0}, \
      { 0,  0,  1} }
#else
#define YAMAHA_MSENSOR_TRANSFORMATION    \
    { { -1,  0,  0}, \
      { 0,  1,   0}, \
      { 0 , 0 ,  -1} }
#endif
      
#define YAMAHA_IOCTL_GET_MARRAY            _IOR('Y', 0x01, char [9])
#define YAMAHA_IOCTL_GET_GARRAY            _IOR('Y', 0x02, char [9])

static struct i2c_client *g_client;
struct yamaha_state *yamaha_state;
struct yamaha_state{
	struct i2c_client	*client;	
};

static int
yamaha_i2c_write(int len, const uint8_t *buf)
{
    return i2c_master_send(g_client, buf, len);
}

static int
yamaha_i2c_read(int len, uint8_t *buf)
{
    return i2c_master_recv(g_client, buf, len);
}

static int yamaha_measureRawdata(int32_t *raw) 
{
    uint8_t dat, buf[6];
 	uint8_t rv = 0;
    int i;

    dat = 0xC0;
    if (yamaha_i2c_write(1, &dat) < 0) {
        return -1;
    }

    dat = 0x00;
    if (yamaha_i2c_write(1, &dat) < 0) {
        return -1;
    }

    for (i = 0; i < 13; i++) {
        msleep(1);

        if (yamaha_i2c_read(6, buf) < 0) {
            return -1;
        }
        if (!(buf[0] & 0x80)) {
            break;
        }
    }

    if (buf[0] & 0x80) {
        return -1;
    }

    for (i = 0; i < 3; ++i) {
        raw[2 - i] = ((buf[i << 1] & 0x7) << 8) + buf[(i << 1) | 1];
    }

    if (raw[0] <= 1024 || raw[0] >= 1) {
        rv |= 0x1;
    }
    if (raw[1] <= 1024 || raw[1] >= 1) {
        rv |= 0x2;
    }
    if (raw[2] <= 1024 || raw[2] >= 1) {
        rv |= 0x4;
    }

    return (int)rv;
}

static int yamaha_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t
yamaha_read(struct file *file, char __user * buffer,
		size_t size, loff_t * pos)
{
	int32_t raw[3];

	raw[0] = raw[1] = raw[2] = 0;

	yamaha_measureRawdata(raw);

	return sprintf(buffer, "%d %d %d\n", raw[0], raw[1], raw[2]);
}

static int yamaha_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int 
yamaha_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{
	signed char marray[3][3] = YAMAHA_MSENSOR_TRANSFORMATION;
	signed char garray[3][3] = YAMAHA_GSENSOR_TRANSFORMATION;

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

static const struct file_operations yamaha_fops = {
	.owner = THIS_MODULE,
	.open = yamaha_open,
	.read = yamaha_read,
	.release = yamaha_release,
	.ioctl = yamaha_ioctl,
};

static struct miscdevice yamaha_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "yamaha_compass",
	.fops = &yamaha_fops,
};

static int __devinit yamaha_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct yamaha_state *yamaha;

	yamaha = kzalloc(sizeof(struct yamaha_state), GFP_KERNEL);
	if (yamaha == NULL) {
		pr_err("%s: failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	yamaha->client = client;
	i2c_set_clientdata(client, yamaha);
    g_client = client;

    return 0;
}

static int __devexit yamaha_remove(struct i2c_client *client)
{
	g_client = NULL;
	return 0;
}

static const struct i2c_device_id yamaha_ids[] = {
	{ "yamaha", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, yamaha_ids);

struct i2c_driver yamaha_i2c_driver =
{
	.driver	= {
		.name	= "yamaha",
	},
	.probe		= yamaha_probe,
	.remove		= __devexit_p(yamaha_remove),
	.id_table	= yamaha_ids,

};

static int __init yamaha_init(void)
{
	int err;

	printk("yamaha_init\n");
	err = misc_register(&yamaha_device);
	if (err) {
		printk("yamaha_init Cannot register miscdev:%d\n",err);
		return err;
	}

	if ( (err = i2c_add_driver(&yamaha_i2c_driver)) ) 
	{
		printk("Driver registration failed, module not inserted.\n");
		return err;
	}

	return 0;
}

static void __exit yamaha_exit(void)
{
	i2c_del_driver(&yamaha_device);
	printk("__exit\n");
}

module_init(yamaha_init);
module_exit(yamaha_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("yamaha compass driver");
MODULE_LICENSE("GPL");
