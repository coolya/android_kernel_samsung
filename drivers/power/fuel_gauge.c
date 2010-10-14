/* Fuel_guage.c */
#define MAX17042

/* Slave address */
#define MAX17042_SLAVE_ADDR	0x6D

/* Register address */
#define STATUS_REG				0x00
#define VALRT_THRESHOLD_REG	0x01
#define TALRT_THRESHOLD_REG	0x02
#define SALRT_THRESHOLD_REG	0x03
#define SOCREP_REG			0x06
#define VCELL_REG				0x09
#define CURRENT_REG			0x0A
#define AVG_CURRENT_REG		0x0B
#define SOCMIX_REG			0x0D
#define SOCAV_REG				0x0E
#define CONFIG_REG			0x1D
#define MISCCFG_REG			0x2B
#define RCOMP_REG				0x38

#include <linux/jiffies.h>
#include <linux/slab.h>

typedef enum {
	POSITIVE = 0,
	NEGATIVE = 1
} sign_type;

static int pr_cnt = 0;

int fuel_guage_init = 0;
EXPORT_SYMBOL(fuel_guage_init);


static struct i2c_driver fg_i2c_driver;
static struct i2c_client *fg_i2c_client = NULL;


struct fg_state{
	struct i2c_client	*client;	
};
struct fg_state *fg_state;


static int fg_i2c_read(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	u16 value;

	value = swab16(i2c_smbus_read_word_data(client, (u8)reg));

	if (value < 0)
		pr_err("%s: Failed to fg_i2c_read\n", __func__);

	*data = (value &0xff00) >>8;
	*(data+1) = value & 0x00ff;
	
	return 0;
}

static int fg_i2c_write(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	u16 value;
	value=(*(data+1)) | (*(data)<< 8) ;
		
	i2c_smbus_write_word_data(client, (u8)reg, swab16(value));
	
	return 0;
}


int fg_read_vcell(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 vcell = 0;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	if (fg_i2c_read(client, VCELL_REG, data, (u8)2) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	vcell = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vcell += (temp2 << 4);

	if(!(pr_cnt % 30))
		printk("%s : VCELL(%d), data(0x%04x)\n", __func__, vcell, (data[1]<<8) | data[0]);

	return vcell;
}


int fg_read_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 soc = 0;
	u32 temp = 0;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	if (fg_i2c_read(client, SOCREP_REG, data, (u8)2) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}
#if 0  // Not Used
	if (fg_i2c_read(client, SOCMIX_REG, data, (u8)2) < 0) {
		pr_err("%s: Failed to read SOCMIX\n", __func__);
		return -1;
	}
	if (fg_i2c_read(client, SOCAV_REG, data, (u8)2) < 0) {
		pr_err("%s: Failed to read SOCAV\n", __func__);
		return -1;
	}
#endif

	temp = data[0] * 39 / 1000;

	soc = data[1];
	if(soc == 0)
	{
		if(temp > 1)  // over 0.1 %
			soc = 1;
	}

	if(!(pr_cnt % 30))
		printk("%s : SOC(%d), data(0x%04x)\n", __func__, soc, (data[1]<<8) | data[0]);

	return soc;
}


int fg_read_current(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data1[2], data2[2];
	u32 temp, sign;
	s32 i_current = 0;
	s32 avg_current = 0;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	if (fg_i2c_read(client, CURRENT_REG, data1, (u8)2) < 0) {
		pr_err("%s: Failed to read CURRENT\n", __func__);
		return -1;
	}

	if (fg_i2c_read(client, AVG_CURRENT_REG, data2, (u8)2) < 0) {
		pr_err("%s: Failed to read AVERAGE CURRENT\n", __func__);
		return -1;
	}

	temp = ((data1[1]<<8) | data1[0]) & 0xFFFF;
	if(temp & (0x1 << 15))
		{
		sign = NEGATIVE;
		temp = (~(temp) & 0xFFFF) + 1;
		}
	else
		sign = POSITIVE;
//	printk("%s : temp(0x%08x), data1(0x%04x)\n", __func__, temp, (data1[1]<<8) | data1[0]);

	temp = temp * 15625;
	i_current = temp / 100000;
	if(sign)
		i_current *= -1;

	temp = ((data2[1]<<8) | data2[0]) & 0xFFFF;
	if(temp & (0x1 << 15))
		{
		sign = NEGATIVE;
		temp = (~(temp) & 0xFFFF) + 1;
		}
	else
		sign = POSITIVE;
//	printk("%s : temp(0x%08x), data2(0x%04x)\n", __func__, temp, (data2[1]<<8) | data2[0]);

	temp = temp * 15625;
	avg_current = temp / 100000;
	if(sign)
		avg_current *= -1;

	if(!(pr_cnt++ % 30))
	{
		printk("%s : CURRENT(%dmA), AVG_CURRENT(%dmA)\n", __func__, i_current, avg_current);
		pr_cnt = 1;
	}

}


int fg_reset_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	s32 ret = 0;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}
	
	if (fg_i2c_read(client, MISCCFG_REG, data, (u8)2) < 0) {
		pr_err("%s: Failed to read MiscCFG\n", __func__);
		return -1;
	}

	data[1] |= (0x1 << 2);  // Set bit10 makes quick start

	if (fg_i2c_write(client, MISCCFG_REG, data, (u8)2) < 0) {
		pr_err("%s: Failed to write MiscCFG\n", __func__);
		return -1;
	}

	msleep(500);

	return ret;
}


int fg_reset_capacity(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	s32 ret = 0;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	data[0] = 0x6C;
	data[1] = 0x20;

	if (fg_i2c_write(client, 0x18, data, (u8)2) < 0) {
		pr_err("%s: Failed to write DesignCap\n", __func__);
		return -1;
	}

	return ret;
}


int fg_check_chip_state(void)
{
	u32 vcell, soc;

	vcell = fg_read_vcell();
	soc = fg_read_soc();

	printk("%s : vcell(%d), soc(%d)\n", __func__, vcell, soc);
	
	// if read operation fails, then it's not alive status
	if( (vcell < 0) || (soc < 0) )
		return 0;
	else
		return 1;
}


static void fg_test_read(void)
{
	u8 data[2], reg;
	struct i2c_client *client = fg_i2c_client;

	for(reg=0; reg<0x40; reg++)
	{
		if(reg != 0x0C && reg != 0x13 && reg != 0x15 && reg != 0x20 && reg != 0x22
			&& reg != 0x26 && reg != 0x28 && reg != 0x30 && reg != 0x31 && reg != 0x34
			&& reg != 0x35 && reg != 0x3C && reg != 0x3E)
		{
			fg_i2c_read(client, reg, data, (u8)2);
 			printk("%s - addr(0x%02x), data(0x%04x)\n", __func__, reg, (data[1]<<8) | data[0]);
		}
	}
}


#ifdef MAX17042
int fg_alert_init(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 misccgf_data[2];
	u8 salrt_data[2];	
	u8 config_data[2];	

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	// using RepSOC
	if (fg_i2c_read(client, MISCCFG_REG, misccgf_data, (u8)2) < 0) {
		pr_err("%s: Failed to read MISCCFG_REG\n", __func__);
		return -1;
	}
	misccgf_data[0] = misccgf_data[0] & ~(0x03);	
	
	if(fg_i2c_write(client, MISCCFG_REG, misccgf_data, (u8)2))
	{
		pr_info("%s: Failed to write MISCCFG_REG\n", __func__);
		return -1;
	}

	// SALRT Threshold setting
	salrt_data[1]=0xff;
	salrt_data[0]=0x01; //1%
	if(fg_i2c_write(client, SALRT_THRESHOLD_REG, salrt_data, (u8)2))
	{
		pr_info("%s: Failed to write SALRT_THRESHOLD_REG\n", __func__);
		return -1;	
	}
	
	// Enable SOC alerts
	if (fg_i2c_read(client, CONFIG_REG, config_data, (u8)2) < 0) {
		pr_err("%s: Failed to read CONFIG_REG\n", __func__);
		return -1;
	}
	config_data[0] = config_data[0] | (0x1 << 2);	
	
	if(fg_i2c_write(client, CONFIG_REG, config_data, (u8)2))
	{
		pr_info("%s: Failed to write CONFIG_REG\n", __func__);
		return -1;
	}
		
	return 1;
	
}

int fg_check_status_reg(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 status_data[2];
	int ret = 1;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}

	// 1. read and clear STATUS_REG
	if (fg_i2c_read(client, STATUS_REG, status_data, (u8)2) < 0) {
		pr_err("%s: Failed to read STATUS_REG\n", __func__);
		return -1;
	}

	if(status_data[1] & (0x1 << 2))
	{
		ret = 1;
		status_data[1] = status_data[1] & ~(0x1 <<2);
		if(fg_i2c_write(client, STATUS_REG, status_data, (u8)2))
		{
			pr_info("%s: Failed to write STATUS_REG\n", __func__);
			return -1;
		}
	}
	
	return ret;
}
#endif


void fuel_gauge_rcomp(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 rst_cmd[2];
	s32 ret = 0;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return ;
	}
#if 0
	rst_cmd[0] = 0xa0;
	rst_cmd[1] = 0x00;

	ret = fg_i2c_write(client, RCOMP_REG, rst_cmd, (u8)2);
	if (ret)
		pr_info("%s: failed fuel_gauge_rcomp(%d)\n", __func__, ret);
	
	msleep(500);
#endif
}


int fg_read_rcomp(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u16 rcomp = 0;

	if(!fuel_guage_init) {
		printk("%s : fuel guage IC is not initialized!!\n", __func__);
		return -1;
	}
#if 0
	if (fg_i2c_read(client, RCOMP_REG, data, (u8)2) < 0) {
		pr_err("%s: Failed to read RCOMP\n", __func__);
		return -1;
	}

	rcomp = (data[1]<<8) | data[0];
#endif
	return rcomp;
}


static void fg_init_config(void)
{
	// Threshold setting

	// Alert setting

}


static int fg_i2c_remove(struct i2c_client *client)
{
	struct fg_state *fg = i2c_get_clientdata(client);

	kfree(fg);
	return 0;
}

static int fg_i2c_probe(struct i2c_client *client,  const struct i2c_device_id *id)
{
	struct fg_state *fg;

	fg = kzalloc(sizeof(struct fg_state), GFP_KERNEL);
	if (fg == NULL) {		
		printk("failed to allocate memory \n");
		return -ENOMEM;
	}
	
	fg->client = client;
	i2c_set_clientdata(client, fg);
	
	/* rest of the initialisation goes here. */
	
	printk("Fuel guage attach success!!!\n");

	fg_i2c_client = client;

	fuel_guage_init = 1;

	fg_test_read();
	
	return 0;
}


static const struct i2c_device_id fg_device_id[] = {
	{"fuelgauge", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fg_device_id);


static struct i2c_driver fg_i2c_driver = {
	.driver = {
		.name = "fuelgauge",
		.owner = THIS_MODULE,
	},
	.probe	= fg_i2c_probe,
	.remove	= fg_i2c_remove,
	.id_table	= fg_device_id,
};

