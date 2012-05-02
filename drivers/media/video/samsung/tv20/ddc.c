#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

#define I2C_DRIVERID_S5P_HDCP		510
#define S5P_HDCP_I2C_ADDR		0x74

const static u16 ignore[] = { I2C_CLIENT_END };
const static u16 normal_addr[] = {
	(S5P_HDCP_I2C_ADDR >> 1),
	I2C_CLIENT_END
};

struct i2c_client *ddc_port;

/*
 * DDC read ftn.
 */
int ddc_read(u8 subaddr, u8 *data, u16 len)
{
	u8 addr = subaddr;
	int ret = 0;

	struct i2c_msg msg[] = {
		[0] = {
			.addr = ddc_port->addr,
			.flags = 0,
			.len = 1,
			.buf = &addr
		},
		[1] = {
			.addr = ddc_port->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = data
		}
	};

	if (i2c_transfer(ddc_port->adapter, msg, 2) != 2)
		ret = -EIO;

	return ret;
}
EXPORT_SYMBOL(ddc_read);


/*
 * DDC_write ftn.
 */
int ddc_write(u8 *data, u16 len)
{
	int ret = 0;

	if (i2c_master_send(ddc_port, (const char *) data, len) != len)
		ret = -EIO;

	return ret;
}
EXPORT_SYMBOL(ddc_write);

/*
 * i2c client ftn.
 */
static int __devinit ddc_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	int ret = 0;

	ddc_port = client;

	dev_info(&client->adapter->dev, "attached s5p_ddc "
		"into i2c adapter successfully\n");

	return ret;
}

static int ddc_remove(struct i2c_client *client)
{
	dev_info(&client->adapter->dev, "detached s5p_ddc "
		"from i2c adapter successfully\n");

	return 0;
}

static int ddc_suspend(struct i2c_client *cl, pm_message_t mesg)
{
	return 0;
};

static int ddc_resume(struct i2c_client *cl)
{
	return 0;
};


static struct i2c_device_id ddc_idtable[] = {
	{"s5p_ddc", 0},
};

MODULE_DEVICE_TABLE(i2c, ddc_idtable);

static struct i2c_driver ddc_driver = {
	.driver = {
		.name = "s5p_ddc",
	},
	.id_table 	= ddc_idtable,
	.probe 		= ddc_probe,
	.remove 	= __devexit_p(ddc_remove),
	.address_list 	= &normal_addr,
	.suspend	= ddc_suspend,
	.resume 	= ddc_resume,
};

static int __init ddc_init(void)
{
	return i2c_add_driver(&ddc_driver);
}

static void __exit ddc_exit(void)
{
	i2c_del_driver(&ddc_driver);
}


MODULE_AUTHOR("SangPil Moon <sangpil.moon@samsung.com>");
MODULE_DESCRIPTION("Driver for SMDKV210 I2C DDC devices");

MODULE_LICENSE("GPL");

module_init(ddc_init);
module_exit(ddc_exit);


