#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#include "smb_acc.h"
#include "smb380calib.h"
/* add by inter.park */
//extern void enable_acc_pins(void);

struct class *acc_class;

/* no use */
//static int smb380_irq_num = NO_IRQ;

/* create smb380 object */
smb380_t smb380;

/* create smb380 registers object */
smb380regs_t smb380regs;


#if 0
static irqreturn_t smb_acc_isr( int irq, void *unused, struct pt_regs *regs )
{
	printk( "smb_acc_isr event occur!!!\n" );
	
	return IRQ_HANDLED;
}
#endif


int smb_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t smb_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t smb_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

int smb_release (struct inode *inode, struct file *filp)
{
	return 0;
}

int smb_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_num,  unsigned long arg)
{
	smb380acc_t accels;
	unsigned char arg_data; 
	int temp;
	unsigned char data[6];
	int err = 0;
	
	switch( ioctl_num )
	{
		case IOCTL_SMB_GET_ACC_VALUE :
			{
				smb380_read_accel_xyz( &accels );

				//printk( "acc data x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );
				
				if( copy_to_user( (smb380acc_t*)arg, &accels, sizeof(smb380acc_t) ) )
				{
					err = -EFAULT;
				}   

			}
			break;
		
		case IOC_SET_ACCELEROMETER :  
			{
				if( copy_from_user( (unsigned int*)&arg_data, (unsigned int*)arg, sizeof(unsigned int) ) )
				{
				
				}
				if( arg_data == SMB_POWER_ON )
				{
					printk( "ioctl : smb power on\n" );
					smb380_set_mode( SMB380_MODE_NORMAL );
				}
				else
				{
					printk( "ioctl : smb power off\n" );
					smb380_set_mode( SMB380_MODE_SLEEP );
				}
			}
		break;

	/* offset calibration routine */
	case BMA150_CALIBRATION:
		if(copy_from_user((smb380acc_t*)data,(smb380acc_t*)arg,6)!=0)
		{
			printk("copy_from_user error\n");
			return -EFAULT;
		}
		/* iteration time 20 */
		temp = 20;
		err = smb380_calibrate(*(smb380acc_t*)data, &temp);
		printk( "BMA150_CALIBRATION status: %d\n", err);
		default : 
			break;
	}

	return err;

}


struct file_operations acc_fops =
{
	.owner   = THIS_MODULE,
	.read    = smb_read,
	.write   = smb_write,
	.open    = smb_open,
	.ioctl   = smb_ioctl,
	.release = smb_release,
};

static inline void bma150_i2c_delay(unsigned int msec)
{
	mdelay(msec);
}
void smb_chip_init(void)
{
	/*assign register memory to smb380 object */
	smb380.image = &smb380regs;

	smb380.smb_bus_write = i2c_acc_smb_write;
	smb380.smb_bus_read  = i2c_acc_smb_read;
	smb380.delay_msec  = bma150_i2c_delay;

	/*call init function to set read write functions, read registers */
	smb380_init( &smb380 );

	/* from this point everything is prepared for sensor communication */


	/* set range to 2G mode, other constants: 
	 * 	   			4G: SMB380_RANGE_4G, 
	 * 	    		8G: SMB380_RANGE_8G */

	smb380_set_range(SMB380_RANGE_2G); 

	/* set bandwidth to 25 HZ */
	smb380_set_bandwidth(SMB380_BW_25HZ);

	/* for interrupt setting */
//	smb380_set_low_g_threshold( SMB380_HG_THRES_IN_G(0.35, 2) );

//	smb380_set_interrupt_mask( SMB380_INT_LG );

}
//TEST
static ssize_t smb380_fs_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	smb380acc_t acc;
	smb380_read_accel_xyz(&acc);

       printk("x: %d,y: %d,z: %d\n", acc.x, acc.y, acc.z);
	count = sprintf(buf,"%d,%d,%d\n", acc.x, acc.y, acc.z );
	

	return count;
}

static ssize_t smb380_fs_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	//buf[size]=0;
	printk("input data --> %s\n", buf);

	return size;
}
static DEVICE_ATTR(acc_file, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, smb380_fs_read, smb380_fs_write);

static ssize_t smb380_calibration(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int err;
	smb380acc_t data;
	/* iteration time 20 */
	int temp = 20;
	int i = 0;

	//buf[size]=0;
	printk("input data --> %s\n", buf);
	if(*(buf+i) == '-')
	{
		i++;
		data.x = -(*(buf+i) - '0');
		i++;
	}
	else
	{
		data.x =(*(buf+i) - '0');
		i++;
	}
	if(*(buf+i) == '-')
	{
		i++;
		data.y = -(*(buf+i) - '0');
		i++;
	}
	else
	{
		data.y = (*(buf+i) - '0');
		i++;
	}
	if(*(buf+i) == '-')
	{
		i++;
		data.z = -(*(buf+i) - '0');
		i++;
	}
	else
	{
		data.z = (*(buf+i) - '0');
		i++;
	}
	if((data.x >= -1 && data.x <=1) && (data.y >= -1 && data.y <=1) && (data.z >= -1 && data.z <=1) )
	{
		/* iteration time 20 */
		err = smb380_calibrate(data, &temp);
		printk( "BMA150_CALIBRATION status: %d\n", err);
	}
	else
	{
		printk( "BMA150_CALIBRATION data err \n");
	}
	return size;
}

static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, NULL, smb380_calibration);

//TEST
int smb_acc_start(void)
{
	int result;

	struct device *dev_t;
	
	//smb380acc_t accels; /* only for test */
	
	result = register_chrdev( ACC_DEV_MAJOR, ACC_DEV_NAME, &acc_fops);

	if (result < 0) 
	{
		return result;
	}
	
	acc_class = class_create (THIS_MODULE, "accelerometer");
	
	if (IS_ERR(acc_class)) 
	{
		unregister_chrdev( ACC_DEV_MAJOR, ACC_DEV_NAME );
		return PTR_ERR( acc_class );
	}

	dev_t = device_create( acc_class, NULL, MKDEV(ACC_DEV_MAJOR, 0), "%s", "accelerometer");

//TEST
	if (device_create_file(dev_t, &dev_attr_acc_file) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_acc_file.attr.name);
//TEST

	if (device_create_file(dev_t, &dev_attr_calibration) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_calibration.attr.name);

	if (IS_ERR(dev_t)) 
	{
		return PTR_ERR(dev_t);
	}
	
	result = i2c_acc_smb_init();

	if(result)
	{
		return result;
	}

	smb_chip_init();
	
	printk("%s ....\n",__FUNCTION__);
	/* only for test */
	#if 0
	printk( "before get xyz\n" );
	mdelay(3000);

	while(1)
	{
		smb380_read_accel_xyz( &accels );

		printk( "acc data x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );
	
		mdelay(100);
	}
	#endif
	
	return 0;
}

void smb_acc_end(void)
{
	unregister_chrdev( ACC_DEV_MAJOR, ACC_DEV_NAME );
	
	i2c_acc_smb_exit();
	
	class_destroy( acc_class );

}


static int smb_accelerometer_probe( struct platform_device* pdev )
{
/* not use interrupt */
#if 0	
	int ret;

	//enable_acc_pins();
	/*
	mhn_gpio_set_direction(MFP_ACC_INT, GPIO_DIR_IN);
	mhn_mfp_set_pull(MFP_ACC_INT, MFP_PULL_HIGH);
	*/

	smb380_irq_num = platform_get_irq(pdev, 0);
	ret = request_irq(smb380_irq_num, (void *)smb_acc_isr, IRQF_DISABLED, pdev->name, NULL);
	if(ret) {
		printk("[SMB ACC] isr register error\n");
		return ret;
	}

	//set_irq_type (smb380_irq_num, IRQT_BOTHEDGE);
	
	/* if( request_irq( IRQ_GPIO( MFP2GPIO(MFP_ACC_INT) ), (void *) smb_acc_isr, 0, "SMB_ACC_ISR", (void *)0 ) )
	if(
	{
		printk ("[SMB ACC] isr register error\n" );
	}
	else
	{
		printk( "[SMB ACC] isr register success!!!\n" );
	}*/
	
	// set_irq_type ( IRQ_GPIO( MFP2GPIO(MFP_ACC_INT) ), IRQT_BOTHEDGE );

	/* if interrupt don't register Process don't stop for polling mode */ 

#endif 

	return smb_acc_start();
}


static int smb_accelerometer_suspend( struct platform_device* pdev, pm_message_t state )
{
	//printk( "smb_accelerometer_suspend\n" );
	
	smb380_set_mode( SMB380_MODE_SLEEP );

	return 0;
}


static int smb_accelerometer_resume( struct platform_device* pdev )
{
	//int result;

	//printk( "smb_accelerometer_resume\n" );

	smb380_set_mode( SMB380_MODE_NORMAL );

#if 0
	result = i2c_acc_smb_init();

	if( result )
	{
		return result;
	}
#endif

	return 0;
}


static struct platform_device *smb_accelerometer_device;

static struct platform_driver smb_accelerometer_driver = {
	.probe 	 = smb_accelerometer_probe,
	.suspend = smb_accelerometer_suspend,
	.resume  = smb_accelerometer_resume,
	.driver  = {
		.name = "smb-accelerometer", 
	}
};


static int __init smb_init(void)
{
	int result;

	printk("%s \n",__FUNCTION__);
	
	result = platform_driver_register( &smb_accelerometer_driver );

	if( result ) 
	{
		return result;
	}

	smb_accelerometer_device  = platform_device_register_simple( "smb-accelerometer", -1, NULL, 0 );
	
	if( IS_ERR( smb_accelerometer_device ) )
	{
		return PTR_ERR( smb_accelerometer_device );
	}

	return 0;
}


static void __exit smb_exit(void)
{
	smb_acc_end();

/* add by inter.park */
//	free_irq(smb380_irq_num, NULL);
//	free_irq( IRQ_GPIO( MFP2GPIO( MFP_ACC_INT ) ), (void*)0 );

	platform_device_unregister( smb_accelerometer_device );
	platform_driver_unregister( &smb_accelerometer_driver );
}


module_init( smb_init );
module_exit( smb_exit );

MODULE_AUTHOR("Throughc");
MODULE_DESCRIPTION("GRANDPRIX accelerometer driver for SMB380");
MODULE_LICENSE("GPL");
