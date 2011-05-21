#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include <asm/uaccess.h>

#define LEVEL_DEV_NAME	"level"

#define LEVEL_DEV_IOCTL_CMD   0xee

#define LEVEL_DEV_UNSET_UPLOAD    _IO(LEVEL_DEV_IOCTL_CMD, 0x1)
#define LEVEL_DEV_SET_AUTOTEST    _IO(LEVEL_DEV_IOCTL_CMD, 0x2)
#define LEVEL_DEV_SET_DEBUGLEVEL    _IO(LEVEL_DEV_IOCTL_CMD, 0x3)
#define LEVEL_DEV_GET_DEBUGLEVEL    _IO(LEVEL_DEV_IOCTL_CMD, 0x4)

#define KERNEL_SEC_DEBUG_LEVEL_LOW 0xA0A0
#define KERNEL_SEC_DEBUG_LEVEL_MID 0xB0B0
#define KERNEL_SEC_DEBUG_LEVEL_HIGH 0xC0C0
#define FAKED_DEBUG_LEVEL KERNEL_SEC_DEBUG_LEVEL_HIGH


static unsigned int get_debug_level(void);

static ssize_t show_control(struct device *d,
		struct device_attribute *attr, char *buf);
static ssize_t store_control(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count);
		
static DEVICE_ATTR(control, S_IRUGO | S_IWUGO, show_control, store_control);

static struct attribute *levelctl_attributes[] = {
	&dev_attr_control.attr,
	NULL
};

static const struct attribute_group levelctl_group = {
	.attrs = levelctl_attributes,
};

static ssize_t show_control(struct device *d,
		struct device_attribute *attr, char *buf)
{
	char *p = buf;
	unsigned int val;

	val = get_debug_level();
	
	p += sprintf(p, "0x%4x\n",val);

	return p - buf;
}

static ssize_t store_control(struct device *d,
		struct device_attribute *attr, const char *buf, size_t count)
{

	if(!strncmp(buf, "clear", 5)) {
		// clear upload magic number
		printk("fake_level: %s clear", __func__);
		//kernel_sec_clear_upload_magic_number();
		return count;
	}

	if(!strncmp(buf, "autotest", 8)) {
		// set auto test
		printk("fake_level: %s autotest", __func__);
		//kernel_sec_set_autotest();
		return count;
	}

	if(!strncmp(buf, "set", 3)) {
		// set debug level
		printk("fake_level: %s set", __func__);
		//set_debug_level();
		return count;
	}

return count;
}

static int level_open(struct inode *inode, struct file *filp)
{
	printk("level Device open\n");

	return 0;
}

static int level_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int val;

	switch (cmd) {
		case LEVEL_DEV_UNSET_UPLOAD:
			printk("fake_level: %s LEVEL_DEV_UNSET_UPLOAD", __func__);
			//kernel_sec_clear_upload_magic_number();
			return 0;

		case LEVEL_DEV_SET_AUTOTEST:
			printk("fake_level: %s LEVEL_DEV_SET_AUTOTEST", __func__);
			//kernel_sec_set_autotest();
			return 0;

		case LEVEL_DEV_SET_DEBUGLEVEL:
			printk("fake_level: %s LEVEL_DEV_SET_DEBUGLEVEL", __func__);
			//set_debug_level();
			return 0;
			
		case LEVEL_DEV_GET_DEBUGLEVEL:
		{
			val = get_debug_level();
			return copy_to_user((unsigned int *)arg, &val, sizeof(val));
		}
		default:
			printk("Unknown Cmd: %x\n", cmd);
			break;
		}
	return -ENOTSUPP;
}

static unsigned int get_debug_level()
{
	return FAKED_DEBUG_LEVEL;
}

static struct file_operations level_fops = 
{
	.owner = THIS_MODULE,
	.open = level_open,
	.ioctl = level_ioctl,
};

static struct miscdevice level_device = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name   = LEVEL_DEV_NAME,
	.fops   = &level_fops,
};

/* init & cleanup. */
static int __init level_init(void)
{
	int result;

	printk("level device init\n");

	result = misc_register(&level_device);
	if (result <0) 
		return result;
	
  result = sysfs_create_group(&level_device.this_device->kobj, &levelctl_group);
	if (result < 0) {
		printk("failed to create sysfs files\n");
	}

	return 0;
}

static void __exit level_exit(void)
{
	printk("level device exit\n");
	misc_deregister(&level_device);
}

module_init(level_init);
module_exit(level_exit);

