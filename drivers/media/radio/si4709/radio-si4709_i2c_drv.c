
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#include "radio-si4709_dev.h"
#include "radio-si4709_common.h"

/*extern functions*/
int Si4709_i2c_drv_init(void);
void Si4709_i2c_drv_exit(void);

/*static functions*/
static int Si4709_probe (struct i2c_client *);
static int Si4709_remove(struct i2c_client *);
static int Si4709_suspend(struct i2c_client *, pm_message_t mesg);
static int Si4709_resume(struct i2c_client *);

static struct i2c_client *Si4709_i2c_client;


struct si4709_data {
	struct i2c_client		*client;
};

/*I2C Setting*/
#define SI4709_I2C_ADDRESS      0x20

static unsigned short Si4709_normal_i2c[] = { I2C_CLIENT_END };
 
static struct i2c_driver Si4709_i2c_driver;

static const struct i2c_device_id si4709_id[] = {
	{"Si4709", 0},
	{}
};

static int Si4709_probe (struct i2c_client *client)
{
    int ret = 0;

    debug("Si4709 i2c driver Si4709_probe called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_probe: device not supported");
    }
    else if( (ret = Si4709_dev_init(client)) < 0 )
    {
        debug("Si4709_dev_init failed");
    }

    return ret;
}

static int Si4709_remove(struct i2c_client *client)
{
    int ret = 0;

    debug("Si4709 i2c driver Si4709_remove called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_remove: device not supported");
    }
    else if( (ret = Si4709_dev_exit()) < 0 )
    {
        debug("Si4709_dev_exit failed");
    }

    return ret;
}

static int __devinit si4709_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;         
	struct si4709_data *si4709_dev;

	debug("----- %s %d\n", __func__, __LINE__);

	si4709_dev = kzalloc(sizeof(struct si4709_data), GFP_KERNEL);
	
	if(!si4709_dev)
	{
		err = -ENOMEM;
		return err;
	}

	Si4709_i2c_client = client;
	i2c_set_clientdata(client, si4709_dev);
	
	if(Si4709_i2c_client == NULL)
	{
        error("Si4709 i2c_client is NULL");
		return -ENODEV;
	}
	
	Si4709_probe(Si4709_i2c_client);

	return 0;
}

static int __devexit si4709_i2c_remove(struct i2c_client *client)
{
	struct si4709_data *si4709_dev = i2c_get_clientdata(client);

	printk("----- %s %d\n", __func__, __LINE__);

	Si4709_remove(Si4709_i2c_client);
	kfree(si4709_dev);
	kfree(client); 
	si4709_dev = NULL;	
	Si4709_i2c_client = NULL;
	
	return 0;
}


MODULE_DEVICE_TABLE(i2c, si4709_id);

static struct i2c_driver Si4709_i2c_driver =
{
    	.driver = {
		.owner = THIS_MODULE,
        	.name = "Si4709",
    	},
	.id_table = si4709_id,
	.probe = si4709_i2c_probe,
	.remove = __devexit_p(si4709_i2c_remove),
    	.suspend = Si4709_suspend,
    	.resume = Si4709_resume,
	.address_list = Si4709_normal_i2c,
};

static int Si4709_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret = 0;
	   
    debug("Si4709 i2c driver Si4709_suspend called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_suspend: device not supported");
    }
    else if( (ret = Si4709_dev_suspend()) < 0 )
    {
        debug("Si4709_dev_disable failed");
    }

    return 0;
}

static int Si4709_resume(struct i2c_client *client)
{
    int ret = 0;
	   
//    debug("Si4709 i2c driver Si4709_resume called"); 

    if( strcmp(client->name, "Si4709") != 0 )
    {
        ret = -1;
        debug("Si4709_resume: device not supported");
    }
    else if( (ret = Si4709_dev_resume()) < 0 )
    {
        debug("Si4709_dev_enable failed");
    }
 
    return 0;
}

int Si4709_i2c_drv_init(void)
{	
    int ret = 0;

    debug("Si4709 i2c driver Si4709_i2c_driver_init called"); 

    if ( (ret = i2c_add_driver(&Si4709_i2c_driver) < 0) ) 
    {
        error("Si4709 i2c_add_driver failed");
    }

    return ret;
}

void Si4709_i2c_drv_exit(void)
{
    debug("Si4709 i2c driver Si4709_i2c_driver_exit called"); 

    i2c_del_driver(&Si4709_i2c_driver);
}


