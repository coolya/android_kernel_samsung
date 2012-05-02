#include "kr3dh_i2c.h"

kr3dh_t *p_kr3dh;
kr3dh_t kr3dh;
kr3dhregs_t kr3dhregs;
/*
 * maintain kr3dh i2c client state
 */
struct kr3dh_state 
{
    struct i2c_client  *client;
};

static struct i2c_client *g_client;
static struct platform_device *kr3dh_accelerometer_device;
struct class *kr3dh_acc_class;

static struct platform_driver kr3dh_accelerometer_driver = 
{
	.probe 	 = kr3dh_accelerometer_probe,
	.suspend = kr3dh_accelerometer_suspend,
	.resume  = kr3dh_accelerometer_resume,
	.driver  = 
	{
		.name = "kr3dh-accelerometer",
	}
};

/*
 * implement character driver operations
 */
struct file_operations kr3dh_acc_fops =
{
	.owner   = THIS_MODULE,
};


static ssize_t show_xyz(struct device *dev, struct device_attribute *attr,char *buf)
{
	kr3dhacc_t accels;
	kr3dh_read_accel_xyz(&accels);
	#ifdef KR3DH_DEBUG  
	printk("ANSARI---------[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG*/
	return snprintf(buf, PAGE_SIZE, "%d %d %d\n", accels.x, accels.y,accels.z);
}

static ssize_t store_xyz(struct device *dev, const char *buf)
{   
	#ifdef KR3DH_DEBUG  
        printk("ANSARI---------[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG*/

	return strnlen(buf, PAGE_SIZE);
}

static DEVICE_ATTR(xyz, S_IWUSR | S_IRUGO, show_xyz, store_xyz);

char i2c_acc_kr3dh_read(u8 reg, u8 *val, unsigned int len )
{
	int 	 err;
	struct 	 i2c_msg msg[1];
	unsigned char data[1];

	if( (NULL == g_client) || (!g_client->adapter) )
	{
		return -ENODEV;
	}

	msg->addr 	= g_client->addr;
	msg->flags 	= I2C_M_WR;
	msg->len 	= 1;
	msg->buf 	= data;
	*data       = reg;
	
	#ifdef KR3DH_DEBUG
        printk("ANSARI-- i2c kr3dh read[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG*/
	
	err = i2c_transfer(g_client->adapter, msg, 1);


	if (err >= 0)
	{
		msg->flags = I2C_M_RD;
		msg->len   = len;
		msg->buf   = val;
		err = i2c_transfer(g_client->adapter, msg, 1);
	}


	if (err >= 0)
	{
		return 0;
	}
	#ifdef KR3DH_DEBUG
	printk("%s %d i2c transfer error\n", __func__, __LINE__);
	#endif/*#ifdef KR3DH_DEBUG*/
	return err;;

}

char i2c_acc_kr3dh_write( u8 reg, u8 *val )
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if( (NULL == g_client) || (!g_client->adapter) )
	{
		return -ENODEV;
	}

	data[0] = reg;
	data[1] = *val;

	msg->addr = g_client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 2;
	msg->buf = data;
	
	#ifdef KR3DH_DEBUG
        printk("ANSARI-- i2c kr3dh write[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG*/

	err = i2c_transfer(g_client->adapter, msg, 1);

	if (err >= 0)
	{
		 return 0;
	}
	#ifdef KR3DH_DEBUG
	printk(KERN_ERR "%s %d i2c transfer error\n", __func__, __LINE__);
	#endif/*#ifdef KR3DH_DEBUG */
	return err;
}

int kr3dh_set_range(char range)
{
   int comres = 0;
   unsigned char data;
   #ifdef KR3DH_DEBUG 
   printk("Kr3dh set range[%s]\n",__func__);
   #endif/*#ifdef KR3DH_DEBUG */

   if (0 == p_kr3dh)
   {
	return E_KR3DH_NULL_PTR;
   }

   if (range<3)
   {
   	comres = p_kr3dh->KR3DH_BUS_READ_FUNC(p_kr3dh->dev_addr, CTRL_REG4, &data, 1 );
	data = data | (4 << range);
	comres += p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG4, &data, 1);
   }

   return comres;

}

int kr3dh_set_mode(unsigned char mode)
{

	int comres=0;
	unsigned char normal = 0x27;
	unsigned char sleep = 0x00;
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh setmode[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

	if (0 == p_kr3dh)
	{
		return E_KR3DH_NULL_PTR;
	}

	switch(mode)
	{
		case KR3DH_MODE_NORMAL:
		case KR3DH_MODE_WAKE_UP:
			comres += p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG1, &normal, 1);
			#ifdef KR3DH_DEBUG 
                	printk("Kr3dh setmode NORMAL AND WAKE UP[%s]\n",__func__);
        		#endif/*#ifdef KR3DH_DEBUG */

			break;
		case KR3DH_MODE_SLEEP:
			comres += p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG1, &sleep, 1);
			#ifdef KR3DH_DEBUG 
                	printk("Kr3dh setmode SLEEP MODE[%s]\n",__func__);
        		#endif/*#ifdef KR3DH_DEBUG */

			break;
		default:
			return E_OUT_OF_RANGE;
	}
	p_kr3dh->mode = mode;

	return comres;
}

unsigned char kr3dh_get_mode(void)
{
    #ifdef KR3DH_DEBUG 
    printk("Kr3dh get mode[%s]\n",__func__);
    #endif/*#ifdef KR3DH_DEBUG */

    if (0 == p_kr3dh)
    {
    	return E_KR3DH_NULL_PTR;
    }

    return p_kr3dh->mode;

}

int kr3dh_set_bandwidth(char bw)
{
	int comres = 0;
	unsigned char data = 0x27;
	 #ifdef KR3DH_DEBUG 
         printk("Kr3dh set bandwidth [%s]\n",__func__);
         #endif/*#ifdef KR3DH_DEBUG */

	if (0 == p_kr3dh)
        {
	     return E_KR3DH_NULL_PTR;
        }

	if (bw<8)
	{
	  data = data | (3 << bw);
	  comres += p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG1, &data, 1 );
	}

	return comres;
}


int kr3dh_get_bandwidth(unsigned char *bw)
{
	int comres = 1;
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh get bandwidth[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

	if (0 == p_kr3dh)
        {
	    return E_KR3DH_NULL_PTR;
        }

	comres = p_kr3dh->KR3DH_BUS_READ_FUNC(p_kr3dh->dev_addr, CTRL_REG1, bw, 1 );

	*bw = (*bw & 0x18);

	return comres;
}


int kr3dh_read_accel_xyz(kr3dhacc_t * acc)
{
	int comres;
	unsigned char data[3];
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh read accel xyz[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

	if (0 == p_kr3dh)
	{
	    return E_KR3DH_NULL_PTR;
        }

	comres = p_kr3dh->KR3DH_BUS_READ_FUNC(p_kr3dh->dev_addr, OUT_X_L, &data[0], 1);
	comres = p_kr3dh->KR3DH_BUS_READ_FUNC(p_kr3dh->dev_addr, OUT_Y_L, &data[1], 1);
	comres = p_kr3dh->KR3DH_BUS_READ_FUNC(p_kr3dh->dev_addr, OUT_Z_L, &data[2], 1);

	data[0] = (~data[0] + 1);
	data[1] = (~data[1] + 1);
	data[2] = (~data[2] + 1);

	if(1) // system_rev>=CONFIG_INSTINCTQ_REV05)
	{
		if(data[0] & 0x80)
			acc->x = (0x100-data[0])*(-1);
		else
			acc->x = ((data[0]) & 0xFF);
		if(data[1]& 0x80)
			acc->y = (0x100-data[1])*(-1);
		else
			acc->y = ((data[1]) & 0xFF);
		if(data[2]& 0x80)
			acc->z = (0x100-data[2])*(-1);
		else
			acc->z = ((data[2]) & 0xFF);
	}
	else
	{
		if(data[0] & 0x80)
			acc->y = -(0x100-data[0])*(-1);
		else
			acc->y = -((data[0]) & 0xFF);
		if(data[1]& 0x80)
			acc->x = (0x100-data[1])*(-1);
		else
			acc->x = ((data[1]) & 0xFF);
		if(data[2]& 0x80)
			acc->z = (0x100-data[2])*(-1);
		else
			acc->z = ((data[2]) & 0xFF);
	}
	 
	printk("[ANSARI-KR3DH] x = %d  /  y =  %d  /  z = %d    ---[%s]\n", acc->x, acc->y, acc->z,__func__ );

	return comres;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void kr3dh_early_suspend(struct early_suspend *handler)
{

	#ifdef KR3DH_DEBUG 
	kr3dhacc_t accels;
	printk("Kr3dh early suspend[%s]\n",__func__);
	kr3dh_read_accel_xyz(&accels);
 	#endif/*#ifdef KR3DH_DEBUG */
	kr3dh_set_mode( KR3DH_MODE_SLEEP );


}

static void kr3dh_late_resume(struct early_suspend *handler)
{
    
	#ifdef KR3DH_DEBUG 
	kr3dhacc_t accels;
	printk("Kr3dh late resume[%s]\n",__func__);
	kr3dh_read_accel_xyz(&accels);
	#endif/*#ifdef KR3DH_DEBUG */
	kr3dh_set_mode( KR3DH_MODE_NORMAL );

}
#endif /* CONFIG_HAS_EARLYSUSPEND */


void kr3dh_chip_init(void)
{
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh chip init[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

	kr3dh.kr3dh_bus_write = i2c_acc_kr3dh_write;
	kr3dh.kr3dh_bus_read  = i2c_acc_kr3dh_read;

	#ifdef CONFIG_HAS_EARLYSUSPEND
	kr3dh.early_suspend.suspend = kr3dh_early_suspend;
	kr3dh.early_suspend.resume = kr3dh_late_resume;
	register_early_suspend(&kr3dh.early_suspend);
	#endif
	kr3dh_init( &kr3dh );
}


int kr3dh_init(kr3dh_t *kr3dh)
{

	unsigned char val1 = 0x27;
	unsigned char val2 = 0x00;

	 #ifdef KR3DH_DEBUG 
         printk("Kr3dh init[%s]\n",__func__);
         #endif/*#ifdef KR3DH_DEBUG */

	p_kr3dh = kr3dh;
	p_kr3dh->dev_addr = SENS_ADD;										/* preset KR3DH I2C_addr */
	p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG1, &val1, 1 );
	p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG2, &val2, 1 );
	p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG3, &val2, 1 );
	p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG4, &val2, 1 );
	p_kr3dh->KR3DH_BUS_WRITE_FUNC(p_kr3dh->dev_addr, CTRL_REG5, &val2, 1 );

	return 0;
}

int kr3dh_acc_start(void)
{
	int result;
	struct device *dev_t;
       #ifdef KR3DH_DEBUG
       kr3dhacc_t accels; /* only for test */
       #endif /*#ifdef KR3DH_DEBUG*/
	result = register_chrdev( KR3DH_MAJOR, "kr3dh", &kr3dh_acc_fops);

	if (result < 0)
	{
		return result;
	}

	kr3dh_acc_class = class_create (THIS_MODULE, "KR3DH-dev");

	if (IS_ERR(kr3dh_acc_class))
	{
		unregister_chrdev( KR3DH_MAJOR, "kr3dh" );
		return PTR_ERR( kr3dh_acc_class );
	}

	dev_t = device_create( kr3dh_acc_class, NULL, MKDEV(KR3DH_MAJOR, 0), "%s", "kr3dh");

	if (IS_ERR(dev_t))
	{
		return PTR_ERR(dev_t);
	}

	if (device_create_file(dev_t, &dev_attr_xyz) < 0)
        {
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_xyz.attr.name);
        }


	kr3dh_chip_init();
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh start [%s]\n",__func__);
	kr3dh_read_accel_xyz( &accels );
	printk("[KR3DH] x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );
	#endif/*#ifdef KR3DH_DEBUG */

	return 0;
}



static int kr3dh_accelerometer_suspend( struct platform_device* pdev, pm_message_t state )
{
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh suspend[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

	return 0;
}


static int kr3dh_accelerometer_resume( struct platform_device* pdev )
{
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh resume[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

	return 0;
}
static int kr3dh_accelerometer_probe( struct platform_device* pdev )
{
	return kr3dh_acc_start();
}



/*
 * Impplement kr3dh_probe module
 */

static int kr3dh_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        struct device *dev = &client->dev;
	struct kr3dh_state *state;

	#ifdef KR3DH_DEBUG 
        printk("Kr3dh probe[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

        state = kzalloc(sizeof(struct kr3dh_state), GFP_KERNEL);
        if (NULL == state) 
	{
                dev_err(dev, "failed to create kr3dh state\n");
                return -ENOMEM;
        }

        state->client = client;
	g_client = state->client;
        i2c_set_clientdata(client, state);

        /* rest of the initialisation goes here. */

        dev_info(dev, "kr3dh client created\n");

        return 0;
}

/*
 * Implement kr3dh remove module
 */

static int __devexit kr3dh_i2c_remove(struct i2c_client *client)
{

	struct kr3dh_state *state = i2c_get_clientdata(client);
	kfree(state);
	#ifdef KR3DH_DEBUG 
        printk("Kr3dh Remove[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */

        return 0;
}

/* 
 * Kr3dh id table
 */
static struct i2c_device_id kr3dh_idtable[] = {
        { "kr3dh", 0 },
        { }
};

/* 
 * register kr3dh device table
 */
MODULE_DEVICE_TABLE(i2c, kr3dh_idtable);

/* 
 * Create kr3dh i2c driver and registered the routines
 */

static struct i2c_driver kr3dh_driver = {
        .driver         = {
                .owner          = THIS_MODULE,
                .name           = "kr3dh",
        },
        .id_table       = kr3dh_idtable,
        .probe          = kr3dh_i2c_probe,
        .remove         = __devexit_p(kr3dh_i2c_remove),
};

/*
 * Add kr3dh driver
 */
static int __init kr3dh_acc_init(void)
{
        int ret;
	#ifdef KR3DH_DEBUG 
                printk("Kr3dh init[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */
	ret = platform_driver_register( &kr3dh_accelerometer_driver);
	if( ret )
	{
		return ret;
	}
	
	kr3dh_accelerometer_device  = platform_device_register_simple( "kr3dh-accelerometer", -1, NULL, 0 );

	if( IS_ERR( kr3dh_accelerometer_device ) )
	{
		return PTR_ERR( kr3dh_accelerometer_device );
	}


        if ( (ret = i2c_add_driver(&kr3dh_driver)) )
        {
                printk("Driver registration failed, module not inserted.\n");
        }

        return ret;
}

/*
 * Remove kr3dh driver
 */
static void __exit kr3dh_acc_exit(void)
{
        #ifdef KR3DH_DEBUG 
        printk("Kr3dh Exit[%s]\n",__func__);
        #endif/*#ifdef KR3DH_DEBUG */


        i2c_del_driver(&kr3dh_driver);
	
	unregister_chrdev( KR3DH_MAJOR, "kr3dh" );
	device_destroy( kr3dh_acc_class, MKDEV(KR3DH_MAJOR, 0) );
        class_destroy( kr3dh_acc_class );
        unregister_early_suspend(&kr3dh.early_suspend);
	
	platform_device_unregister( kr3dh_accelerometer_device );
	platform_driver_unregister( &kr3dh_accelerometer_driver );
}


/*
 * init/exit kr3dh module
 */
module_init( kr3dh_acc_init );
module_exit( kr3dh_acc_exit );
/*
 * Kr3dh description
 */
MODULE_AUTHOR("L&T_Ansari");
MODULE_DESCRIPTION("accelerometer driver for KR3DH");
MODULE_LICENSE("GPL");
