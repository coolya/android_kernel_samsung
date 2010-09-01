/* Register address */
#define VCELL_REG		0x02
#define SOCREP_REG		0x04
#define MISCCFG_REG		0x06
#define RCOMP_REG		0x0C
#define CMD_REG			0xFE

#include <linux/jiffies.h>
#include <linux/slab.h>

int fuel_guage_init;
EXPORT_SYMBOL(fuel_guage_init);

static struct i2c_driver fg_i2c_driver;
static struct i2c_client *fg_i2c_client;

struct fg_state{
	struct i2c_client	*client;
};

static int fg_i2c_read(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	int value = i2c_smbus_read_word_data(client, reg);

	if (value < 0) {
		pr_err("%s: Failed to fg_i2c_read\n", __func__);
		return -1;
	}

	*data = value & 0x00ff;
	*(data+1) = (value & 0xff00) >> 8;

	return 0;
}

static int fg_i2c_write(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	u16 value = (*(data+1) << 8) | (*(data)) ;

	return i2c_smbus_write_word_data(client, reg, value);
}

int fg_read_vcell(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 vcell = 0;

	if (!fuel_guage_init) {
		pr_err("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	if (fg_i2c_read(client, VCELL_REG, data, 2) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	vcell = ((((data[0] << 4) & 0xFF0) | ((data[1] >> 4) & 0xF)) * 125)/100;

	return vcell;
}

int fg_read_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 soc = 0;
	u32 temp = 0;
	u32 temp_soc = 0;

	if (!fuel_guage_init) {
		pr_err("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	if (fg_i2c_read(client, SOCREP_REG, data, 2) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	temp = data[0] * 100 + ((data[1] * 100) / 256);

	if (temp >= 100)
		temp_soc = temp;
	else {
		if (temp >= 70)
			temp_soc = 100;
		else
			temp_soc = 0;
	}

	/* rounding off and Changing to percentage */
	soc = temp_soc / 100;

	if (temp_soc % 100 >= 50)
		soc += 1;

	if (soc >= 26)
		soc += 4;
	else
		soc = (30 * temp_soc) / 26 / 100;

	if (soc >= 100)
		soc = 100;

	return soc;
}

int fg_reset_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	s32 ret = 0;

	if (!fuel_guage_init) {
		pr_err("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	/* Quick-start */
	data[0] = 0x40;
	data[1] = 0x00;

	if (fg_i2c_write(client, MISCCFG_REG, data, 2) < 0) {
		pr_err("%s: Failed to write MiscCFG\n", __func__);
		return -1;
	}

	msleep(500);

	return ret;
}

void fuel_gauge_rcomp(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 rst_cmd[2];

	if (!fuel_guage_init) {
		pr_err("%s : fuel guage IC is not initialized!!\n", __func__);
		return ;
	}

	rst_cmd[0] = 0xB0;
	rst_cmd[1] = 0x00;

	if (fg_i2c_write(client, RCOMP_REG, rst_cmd, 2) < 0)
		pr_err("%s: failed fuel_gauge_rcomp\n", __func__);
}

static int fg_i2c_remove(struct i2c_client *client)
{
	struct fg_state *fg = i2c_get_clientdata(client);

	kfree(fg);
	return 0;
}

static int fg_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct fg_state *fg;

	fuel_guage_init = 0;
	fg_i2c_client = NULL;

	fg = kzalloc(sizeof(struct fg_state), GFP_KERNEL);
	if (fg == NULL) {
		pr_err("failed to allocate memory\n");
		return -ENOMEM;
	}

	fg->client = client;
	i2c_set_clientdata(client, fg);

	/* rest of the initialisation goes here. */

	pr_info("Fuel guage attach success!!!\n");

	fg_i2c_client = client;

	fuel_guage_init = 1;

	return 0;
}


static const struct i2c_device_id fg_device_id[] = {
	{"max1704x", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fg_device_id);


static struct i2c_driver fg_i2c_driver = {
	.driver = {
		.name = "max1704x",
		.owner = THIS_MODULE,
	},
	.probe	= fg_i2c_probe,
	.remove	= fg_i2c_remove,
	.id_table	= fg_device_id,
};
