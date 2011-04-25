/*
 * BMA023 accelerometer driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#define BMA023_NAME "bma023"

/* Default parameters */
#define BMA023_DEFAULT_DELAY            100
#define BMA023_MAX_DELAY                2000

/* Registers */
#define BMA023_CHIP_ID_REG              0x00
#define BMA023_CHIP_ID                  0x02

#define BMA023_AL_VERSION_REG           0x01
#define BMA023_AL_VERSION_MASK          0x0f
#define BMA023_AL_VERSION_SHIFT         0

#define BMA023_ML_VERSION_REG           0x01
#define BMA023_ML_VERSION_MASK          0xf0
#define BMA023_ML_VERSION_SHIFT         4

#define BMA023_ACC_REG                  0x02

#define BMA023_SOFT_RESET_REG           0x0a
#define BMA023_SOFT_RESET_MASK          0x02
#define BMA023_SOFT_RESET_SHIFT         1

#define BMA023_SLEEP_REG                0x0a
#define BMA023_SLEEP_MASK               0x01
#define BMA023_SLEEP_SHIFT              0

#define BMA023_RANGE_REG                0x14
#define BMA023_RANGE_MASK               0x18
#define BMA023_RANGE_SHIFT              3
#define BMA023_RANGE_2G                 0
#define BMA023_RANGE_4G                 1
#define BMA023_RANGE_8G                 2

#define BMA023_BANDWIDTH_REG            0x14
#define BMA023_BANDWIDTH_MASK           0x07
#define BMA023_BANDWIDTH_SHIFT          0
#define BMA023_BANDWIDTH_25HZ           0
#define BMA023_BANDWIDTH_50HZ           1
#define BMA023_BANDWIDTH_100HZ          2
#define BMA023_BANDWIDTH_190HZ          3
#define BMA023_BANDWIDTH_375HZ          4
#define BMA023_BANDWIDTH_750HZ          5
#define BMA023_BANDWIDTH_1500HZ         6

#define BMA023_SHADOW_DIS_REG           0x15
#define BMA023_SHADOW_DIS_MASK          0x08
#define BMA023_SHADOW_DIS_SHIFT         3

#define BMA023_WAKE_UP_PAUSE_REG        0x15
#define BMA023_WAKE_UP_PAUSE_MASK       0x06
#define BMA023_WAKE_UP_PAUSE_SHIFT      1

#define BMA023_WAKE_UP_REG              0x15
#define BMA023_WAKE_UP_MASK             0x01
#define BMA023_WAKE_UP_SHIFT            0

/* filter setting */
#define BMA023_FILTER_LEN		8
#define BMA023_STABLE_TH		5

/* ioctl commnad */
#define DCM_IOC_MAGIC			's'
#define BMA023_CALIBRATION		_IOWR(DCM_IOC_MAGIC, 48, short)


/* Acceleration measurement */
struct acceleration {
	short x;
	short y;
	short z;
};

/* Output data rate  */
struct bma023_odr {
	unsigned long delay;	/* min delay (msec) in the range of ODR */
	u8 odr;			/* bandwidth register value */
};

static const struct bma023_odr bma023_odr_table[] = {
/*	{  1, BMA023_BANDWIDTH_1500HZ }, */
	{  2, BMA023_BANDWIDTH_750HZ },
	{  3, BMA023_BANDWIDTH_375HZ },
	{  6, BMA023_BANDWIDTH_190HZ },
	{ 10, BMA023_BANDWIDTH_100HZ },
	{ 20, BMA023_BANDWIDTH_50HZ },
	{ 40, BMA023_BANDWIDTH_25HZ },
};

struct bma023_fir_filter {
	int num;
	int filter_len;
	int index;
	int32_t sequence[BMA023_FILTER_LEN];
};

/* driver private data */
struct bma023_data {
	atomic_t enable;                /* attribute value */
	atomic_t delay;                 /* attribute value */
	struct mutex enable_mutex;
	struct mutex data_mutex;
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	struct miscdevice bma023_device;
	struct bma023_fir_filter filter[3];
};

#define delay_to_jiffies(d) ((d) ? msecs_to_jiffies(d) : 1)
#define actual_delay(d)     (jiffies_to_msecs(delay_to_jiffies(d)))

/* register access functions */
#define bma023_read_bits(p, r) \
	((i2c_smbus_read_byte_data((p)->client, r##_REG) \
	& r##_MASK) >> r##_SHIFT)

#define bma023_update_bits(p, r, v) \
	i2c_smbus_write_byte_data((p)->client, r##_REG, \
	((i2c_smbus_read_byte_data((p)->client, r##_REG) \
	& ~r##_MASK) | ((v) << r##_SHIFT)))

static void fir_filter_init(struct bma023_fir_filter *filter, int len)
{
	int i;

	filter->num = 0;
	filter->index = 0;
	filter->filter_len = len;

	for (i = 0; i < filter->filter_len; ++i)
		filter->sequence[i] = 0;
}

static s16 fir_filter_filter(struct bma023_fir_filter *filter, s16 in)
{
	int out = 0;
	int i;

	if (filter->filter_len == 0)
		return in;
	if (filter->num < filter->filter_len) {
		filter->sequence[filter->index++] = in;
		filter->num++;
		return in;
	} else {
		if (filter->filter_len <= filter->index)
			filter->index = 0;
		filter->sequence[filter->index++] = in;

		for (i = 0; i < filter->filter_len; i++)
			out += filter->sequence[i];
		return out / filter->filter_len;
	}
}

static void filter_init(struct bma023_data *bma023)
{
	int i;

	for (i = 0; i < 3; i++)
		fir_filter_init(&bma023->filter[i], BMA023_FILTER_LEN);
}

static void filter_filter(struct bma023_data *bma023, s16 *orig, s16 *filtered)
{
	int i;

	for (i = 0; i < 3; i++)
		filtered[i] = fir_filter_filter(&bma023->filter[i], orig[i]);
}

static void filter_stabilizer(struct bma023_data *bma023,
					s16 *orig, s16 *stabled)
{
	int i;
	static s16 buffer[3] = { 0, };

	for (i = 0; i < 3; i++) {
		if ((buffer[i] - orig[i] >= BMA023_STABLE_TH)
			|| (buffer[i] - orig[i] <= -BMA023_STABLE_TH)) {
			stabled[i] = orig[i];
			buffer[i] = stabled[i];
		} else
			stabled[i] = buffer[i];
	}
}

/* Device dependant operations */
static int bma023_power_up(struct bma023_data *bma023)
{
	bma023_update_bits(bma023, BMA023_SLEEP, 0);
	/* wait 1ms for wake-up time from sleep to operational mode */
	msleep(1);
	return 0;
}

static int bma023_power_down(struct bma023_data *bma023)
{
	bma023_update_bits(bma023, BMA023_SLEEP, 1);
	return 0;
}

static int bma023_hw_init(struct bma023_data *bma023)
{
	/* reset hardware */
	bma023_power_up(bma023);
	i2c_smbus_write_byte_data(bma023->client,
			  BMA023_SOFT_RESET_REG, BMA023_SOFT_RESET_MASK);

	/* wait 10us after soft_reset to start I2C transaction */
	msleep(1);

	bma023_update_bits(bma023, BMA023_RANGE, BMA023_RANGE_2G);
	bma023_power_down(bma023);

	return 0;
}

static int bma023_get_enable(struct device *dev)
{
	struct bma023_data *bma023 = dev_get_drvdata(dev);
	return atomic_read(&bma023->enable);
}

static void bma023_set_enable(struct device *dev, int enable)
{
	struct bma023_data *bma023 = dev_get_drvdata(dev);
	int delay = atomic_read(&bma023->delay);

	mutex_lock(&bma023->enable_mutex);
	if (enable) { /* enable if state will be changed */
		if (!atomic_cmpxchg(&bma023->enable, 0, 1)) {
			bma023_power_up(bma023);
			schedule_delayed_work(&bma023->work,
					      delay_to_jiffies(delay) + 1);
		}
	} else { /* disable if state will be changed */
		if (atomic_cmpxchg(&bma023->enable, 1, 0)) {
			cancel_delayed_work_sync(&bma023->work);
			bma023_power_down(bma023);
		}
	}
	atomic_set(&bma023->enable, enable);
	mutex_unlock(&bma023->enable_mutex);
}

static int bma023_get_delay(struct device *dev)
{
	struct bma023_data *bma023 = dev_get_drvdata(dev);
	return atomic_read(&bma023->delay);
}

static void bma023_set_delay(struct device *dev, int delay)
{
	struct bma023_data *bma023 = dev_get_drvdata(dev);
	int i;
	u8 odr;

	/* determine optimum ODR */
	for (i = 1; (i < ARRAY_SIZE(bma023_odr_table)) &&
		     (actual_delay(delay) >= bma023_odr_table[i].delay); i++)
		;
	odr = bma023_odr_table[i-1].odr;
	atomic_set(&bma023->delay, delay);

	mutex_lock(&bma023->enable_mutex);
	if (bma023_get_enable(dev)) {
		cancel_delayed_work_sync(&bma023->work);
		bma023_update_bits(bma023, BMA023_BANDWIDTH, odr);
		schedule_delayed_work(&bma023->work,
				      delay_to_jiffies(delay) + 1);
	} else {
		bma023_power_up(bma023);
		bma023_update_bits(bma023, BMA023_BANDWIDTH, odr);
		bma023_power_down(bma023);
	}
	mutex_unlock(&bma023->enable_mutex);
}

static int bma023_measure(struct bma023_data *bma023,
				struct acceleration *accel)
{
	struct i2c_client *client = bma023->client;
	int err;
	int i;
	s16 raw_data[3];
	s16 filtered_data[3];
	s16 stabled_data[3];
	u8 buf[6];

	/* read acceleration raw data */
	err = i2c_smbus_read_i2c_block_data(client,
			BMA023_ACC_REG, sizeof(buf), buf) != sizeof(buf);
	if (err < 0) {
		pr_err("%s: i2c read fail addr=0x%02x, len=%d\n",
			__func__, BMA023_ACC_REG, sizeof(buf));
		for (i = 0; i < 3; i++)
			raw_data[i] = 0;
	} else
		for (i = 0; i < 3; i++)
			raw_data[i] = (*(s16 *)&buf[i*2]) >> 6;

	/* filter out sizzling values */
	/* filter_filter(bma023, raw_data, filtered_data); */
	/* filter_stabilizer(bma023, filtered_data, stabled_data); */
	filter_stabilizer(bma023, raw_data, stabled_data);

	accel->x = stabled_data[0];
	accel->y = stabled_data[1];
	accel->z = stabled_data[2];

	return err;
}

static void bma023_work_func(struct work_struct *work)
{
	struct bma023_data *bma023 = container_of((struct delayed_work *)work,
						  struct bma023_data, work);
	struct acceleration accel;
	unsigned long delay = delay_to_jiffies(atomic_read(&bma023->delay));

	bma023_measure(bma023, &accel);

	input_report_rel(bma023->input, REL_X, accel.x);
	input_report_rel(bma023->input, REL_Y, accel.y);
	input_report_rel(bma023->input, REL_Z, accel.z);
	input_sync(bma023->input);

	schedule_delayed_work(&bma023->work, delay);
}

/* Input device interface */
static int bma023_input_init(struct bma023_data *bma023)
{
	struct input_dev *dev;
	int err;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;
	dev->name = "accelerometer_sensor";
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_RY);
	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_capability(dev, EV_REL, REL_Z);
	input_set_drvdata(dev, bma023);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	bma023->input = dev;

	return 0;
}

static void bma023_input_fini(struct bma023_data *bma023)
{
	struct input_dev *dev = bma023->input;

	input_unregister_device(dev);
	input_free_device(dev);
}

/* sysfs device attributes */
static ssize_t bma023_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bma023_get_enable(dev));
}

static ssize_t bma023_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int err;
	unsigned long enable;
	err = strict_strtoul(buf, 10, &enable);

	if ((enable == 0) || (enable == 1))
		bma023_set_enable(dev, enable);

	return count;
}

static ssize_t bma023_delay_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bma023_get_delay(dev));
}

static ssize_t bma023_delay_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int err;
	long delay;

	err = strict_strtoul(buf, 10, &delay);
	if (err < 0)
		return count;

	if (delay > BMA023_MAX_DELAY)
		delay = BMA023_MAX_DELAY;

	bma023_set_delay(dev, delay);

	return count;
}

static ssize_t bma023_wake_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	static atomic_t serial = ATOMIC_INIT(0);

	input_report_rel(input, REL_RY, atomic_inc_return(&serial));

	return count;
}

static ssize_t bma023_data_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bma023_data *bma023 = input_get_drvdata(input);
	struct acceleration accel;
	int on;

	mutex_lock(&bma023->data_mutex);
	on = bma023_get_enable(dev);
	if (!on)
		bma023_set_enable(dev, 1);
	bma023_measure(bma023, &accel);
	if (!on)
		bma023_set_enable(dev, 0);
	mutex_unlock(&bma023->data_mutex);

	return sprintf(buf, "%d,%d,%d\n", accel.x, accel.y, accel.z);
}

static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
		   bma023_enable_show, bma023_enable_store);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
		   bma023_delay_show, bma023_delay_store);
static DEVICE_ATTR(wake, S_IWUSR|S_IWGRP,
		   NULL, bma023_wake_store);
static DEVICE_ATTR(data, S_IRUGO,
		   bma023_data_show, NULL);

static struct attribute *bma023_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	&dev_attr_wake.attr,
	&dev_attr_data.attr,
	NULL
};

static struct attribute_group bma023_attribute_group = {
	.attrs = bma023_attributes
};

static int bma023_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	int id;

	id = i2c_smbus_read_byte_data(client, BMA023_CHIP_ID_REG);
	if (id != BMA023_CHIP_ID)
		return -ENODEV;

	return 0;
}

int bma023_get_offset(struct bma023_data *bma023,
			unsigned char xyz, unsigned short *offset)
{
	unsigned char data;

	data = i2c_smbus_read_byte_data(bma023->client, 0x16 + xyz);
	data = (data & 0xC0) >> 6;

	*offset = data;
	data = i2c_smbus_read_byte_data(bma023->client, 0x1A + xyz);
	*offset |= (data << 2);

	return 0;
}

int bma023_set_offset(struct bma023_data *bma023,
			unsigned char xyz, unsigned short offset)
{
	int err;
	unsigned char data;

	data = i2c_smbus_read_byte_data(bma023->client, 0x16 + xyz);
	data = (data & ~0xC0) | ((offset << 6) & 0xC0);

	err = i2c_smbus_write_byte_data(bma023->client, 0x16 + xyz, data);
	data = (offset & 0x3ff) >> 2;
	err = i2c_smbus_write_byte_data(bma023->client, 0x1A + xyz, data);

	return err;
}

int bma023_set_offset_eeprom(struct bma023_data *bma023,
		unsigned char xyz, unsigned short offset)
{
	int err;
	unsigned char data;

	data = i2c_smbus_read_byte_data(bma023->client, 0x16 + xyz);
	data = (data & ~0xC0) | ((offset << 6) & 0xC0);

	data = i2c_smbus_write_byte_data(bma023->client,
					0x20 + 0x16 + xyz, data);
	msleep(34);

	data = (offset&0x3ff)>>2;
	err = i2c_smbus_write_byte_data(bma023->client,
					0x20 + 0x1A + xyz, data);
	msleep(34);

	return err;
}

int bma023_set_ee_w(struct bma023_data *bma023, unsigned char eew)
{
	unsigned char data;
	int err;

	data = i2c_smbus_read_byte_data(bma023->client, 0x0A);
	data = (data & ~0x10) | ((eew<<4)&0x10);
	err = i2c_smbus_write_byte_data(bma023->client, 0x0A, data);
	return err;
}

int bma023_calc_new_offset(struct acceleration orientation,
				struct acceleration accel,
				unsigned short *offset_x,
				unsigned short *offset_y,
				unsigned short *offset_z)
{
	short old_offset_x, old_offset_y, old_offset_z;
	short new_offset_x, new_offset_y, new_offset_z;
	unsigned char  calibrated = 0;

	old_offset_x = *offset_x;
	old_offset_y = *offset_y;
	old_offset_z = *offset_z;

	accel.x = accel.x - (orientation.x * 256);
	accel.y = accel.y - (orientation.y * 256);
	accel.z = accel.z - (orientation.z * 256);

	/* does x axis need calibration? */
	if ((accel.x > 4) | (accel.x < -4)) {
		/* check for values less than quantisation of offset register */
		if ((accel.x < 8) && accel.x > 0)
			new_offset_x = old_offset_x - 1;
		else if ((accel.x > -8) && (accel.x < 0))
			new_offset_x = old_offset_x + 1;
		else
			/* calculate new offset due to formula */
			new_offset_x = old_offset_x - (accel.x / 8);
		/* check for register boundary */
		if (new_offset_x < 0)
			new_offset_x = 0;
		else if (new_offset_x > 1023)
			new_offset_x = 1023;
		*offset_x = new_offset_x;
		calibrated = 1;
	}

	/* does y axis need calibration? */
	if ((accel.y > 4) | (accel.y < -4)) {
		/* check for values less than quantisation of offset register */
		if ((accel.y < 8) && accel.y > 0)
			new_offset_y = old_offset_y - 1;
		else if ((accel.y > -8) && accel.y < 0)
			new_offset_y = old_offset_y + 1;
		else
			/* calculate new offset due to formula */
			new_offset_y = old_offset_y - accel.y / 8;
		/* check for register boundary */
		if (new_offset_y < 0)
			new_offset_y = 0;
		else if (new_offset_y > 1023)
			new_offset_y = 1023;
		*offset_y = new_offset_y;
		calibrated = 1;
	}

	/* does z axis need calibration? */
	if ((accel.z > 4) | (accel.z < -4)) {
		/* check for values less than quantisation of offset register */
		if ((accel.z < 8) && accel.z > 0)
			new_offset_z = old_offset_z - 1;
		else if ((accel.z > -8) && accel.z < 0)
			new_offset_z = old_offset_z + 1;
		else
			/* calculate new offset due to formula */
			new_offset_z = old_offset_z - (accel.z / 8);
		/* check for register boundary */
		if (new_offset_z < 0)
			new_offset_z = 0;
		else if (new_offset_z > 1023)
			new_offset_z = 1023;
		*offset_z = new_offset_z;
		calibrated = 1;
	}
	return calibrated;
}

/* reads out acceleration data and averages them, measures min and max
 * \param orientation pass orientation one axis needs to be absolute 1
 * the others need to be 0
 * \param num_avg numer of samples for averaging
 * \param *min returns the minimum measured value
 * \param *max returns the maximum measured value
 * \param *average returns the average value
 */

int bma023_read_accel_avg(struct bma023_data *bma023, int num_avg,
					struct acceleration *min,
					struct acceleration *max,
					struct acceleration *avg)
{
	long x_avg = 0, y_avg = 0, z_avg = 0;
	int comres = 0;
	int i;
	struct acceleration accel;

	x_avg = 0; y_avg = 0; z_avg = 0;
	max->x = -512; max->y = -512; max->z = -512;
	min->x = 512;  min->y = 512; min->z = 512;

	for (i = 0; i < num_avg; i++) {
		/* read 10 acceleration data triples */
		comres += bma023_measure(bma023, &accel);
		if (accel.x > max->x)
			max->x = accel.x;
		if (accel.x < min->x)
			min->x = accel.x;

		if (accel.y > max->y)
			max->y = accel.y;
		if (accel.y < min->y)
			min->y = accel.y;

		if (accel.z > max->z)
			max->z = accel.z;
		if (accel.z < min->z)
			min->z = accel.z;

		x_avg += accel.x;
		y_avg += accel.y;
		z_avg += accel.z;
		msleep(10);
	}
	/* calculate averages, min and max values */
	avg->x = x_avg /= num_avg;
	avg->y = y_avg /= num_avg;
	avg->z = z_avg /= num_avg;
	return comres;
}

/* verifies the accerleration values to be good enough
 * for calibration calculations
 * \param min takes the minimum measured value
 * \param max takes the maximum measured value
 * \param takes returns the average value
 * \return 1: min,max values are in range, 0: not in range
 */

int bma023_verify_min_max(struct acceleration min, struct acceleration max,
						struct acceleration avg)
{
	short dx, dy, dz;
	int ver_ok = 1;

	/* calc delta max-min */
	dx =  max.x - min.x;
	dy =  max.y - min.y;
	dz =  max.z - min.z;

	if (dx > 10 || dx < -10)
		ver_ok = 0;
	if (dy > 10 || dy < -10)
		ver_ok = 0;
	if (dz > 10 || dz < -10)
		ver_ok = 0;

	return ver_ok;
}

/* overall calibration process. This function
 * takes care about all other functions
 * \param orientation input for orientation [0;0;1] for measuring the device
 * in horizontal surface up
 * \param *tries takes the number of wanted iteration steps, this pointer
 * returns the number of calculated steps after this routine has finished
 * \return 1: calibration passed 2: did not pass within N steps
 */

int bma023_calibrate(struct bma023_data *bma023,
			struct acceleration orientation, int *tries)
{
	struct acceleration min, max, avg;
	unsigned short offset_x, offset_y, offset_z;
	unsigned short old_offset_x, old_offset_y, old_offset_z;
	int need_calibration = 0, min_max_ok = 0;
	int ltries;
	int retry = 30;
	int on;

	on = bma023_get_enable(&bma023->client->dev);
	if (!on)
		bma023_set_enable(&bma023->client->dev, 1);

	bma023_update_bits(bma023, BMA023_RANGE, BMA023_RANGE_2G);
	bma023_update_bits(bma023, BMA023_BANDWIDTH, BMA023_BANDWIDTH_25HZ);
	bma023_set_ee_w(bma023, 1);

	bma023_get_offset(bma023, 0, &offset_x);
	bma023_get_offset(bma023, 1, &offset_y);
	bma023_get_offset(bma023, 2, &offset_z);

	old_offset_x = offset_x;
	old_offset_y = offset_y;
	old_offset_z = offset_z;
	ltries = *tries;

	do {
		/* read acceleration data min, max, avg */
		bma023_read_accel_avg(bma023, 10, &min, &max, &avg);
		min_max_ok = bma023_verify_min_max(min, max, avg);

		if (!min_max_ok) {
			retry--;
			if (retry <= 0)
				return -1;
		}
		/* check if calibration is needed */
		if (min_max_ok)
			need_calibration
				= bma023_calc_new_offset(orientation, avg,
					&offset_x, &offset_y, &offset_z);

		if (*tries == 0) /*number of maximum tries reached? */
			break;

		if (need_calibration) {
			/* when needed calibration is updated in image */
			(*tries)--;
			bma023_set_offset(bma023, 0, offset_x);
			bma023_set_offset(bma023, 1, offset_y);
			bma023_set_offset(bma023, 2, offset_z);
			msleep(20);
		}
	} while (need_calibration || !min_max_ok);

	if (*tries > 0 && *tries < ltries) {
		if (old_offset_x != offset_x)
			bma023_set_offset_eeprom(bma023, 0, offset_x);
		if (old_offset_y != offset_y)
			bma023_set_offset_eeprom(bma023, 1, offset_y);
		if (old_offset_z != offset_z)
			bma023_set_offset_eeprom(bma023, 2, offset_z);
	}

	bma023_set_ee_w(bma023, 0);
	msleep(20);
	*tries = ltries - *tries;

	if (!on)
		bma023_set_enable(&bma023->client->dev, 0);

	return !need_calibration;
}

static int bma023_open(struct inode *inode, struct file *file)
{
	struct bma023_data *bma023 = container_of(file->private_data,
						struct bma023_data,
						bma023_device);
	file->private_data = bma023;
	return 0;
}

static int bma023_close(struct inode *inode, struct file *file)
{
	return 0;
}

static int bma023_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	struct bma023_data *bma023 = file->private_data;
	int try;
	int err = 0;
	unsigned char data[6];

	switch (cmd) {
	case BMA023_CALIBRATION:
		if (copy_from_user((struct acceleration *)data,
			(struct acceleration *)arg, 6) != 0) {
			pr_err("copy_from_user error\n");
			return -EFAULT;
		}
		/* iteration time = 20 */
		try = 20;
		err = bma023_calibrate(bma023,
				*(struct acceleration *)data, &try);
		break;
	default:
		break;
	}
	return 0;
}

static const struct file_operations bma023_fops = {
	.owner = THIS_MODULE,
	.open = bma023_open,
	.release = bma023_close,
	.ioctl = bma023_ioctl,
};

static int bma023_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct bma023_data *bma023;
	int err;

	/* setup private data */
	bma023 = kzalloc(sizeof(struct bma023_data), GFP_KERNEL);
	if (!bma023)
		return -ENOMEM;
	mutex_init(&bma023->enable_mutex);
	mutex_init(&bma023->data_mutex);

	/* setup i2c client */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto err_i2c_fail;
	}
	i2c_set_clientdata(client, bma023);
	bma023->client = client;

	/* detect and init hardware */
	err = bma023_detect(client, NULL);
	if (err)
		goto err_id_read;
	dev_info(&client->dev, "%s found\n", id->name);
	dev_info(&client->dev, "al_version=%d, ml_version=%d\n",
		 bma023_read_bits(bma023, BMA023_AL_VERSION),
		 bma023_read_bits(bma023, BMA023_ML_VERSION));

	bma023_hw_init(bma023);
	bma023_set_delay(&client->dev, BMA023_DEFAULT_DELAY);

	/* setup driver interfaces */
	INIT_DELAYED_WORK(&bma023->work, bma023_work_func);

	err = bma023_input_init(bma023);
	if (err < 0)
		goto err_input_allocate;

	err = sysfs_create_group(&bma023->input->dev.kobj,
					&bma023_attribute_group);
	if (err < 0)
		goto err_sys_create;

	bma023->bma023_device.minor = MISC_DYNAMIC_MINOR;
	bma023->bma023_device.name = "accelerometer";
	bma023->bma023_device.fops = &bma023_fops;

	err = misc_register(&bma023->bma023_device);
	if (err) {
		pr_err("%s: misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	/* filter init */
	filter_init(bma023);

	return 0;

err_misc_register:
	sysfs_remove_group(&bma023->input->dev.kobj,
				&bma023_attribute_group);
err_sys_create:
	bma023_input_fini(bma023);
err_input_allocate:
err_id_read:
err_i2c_fail:
	kfree(bma023);
	return err;
}

static int bma023_remove(struct i2c_client *client)
{
	struct bma023_data *bma023 = i2c_get_clientdata(client);

	bma023_set_enable(&client->dev, 0);

	sysfs_remove_group(&bma023->input->dev.kobj, &bma023_attribute_group);
	bma023_input_fini(bma023);
	kfree(bma023);

	return 0;
}

static int bma023_suspend(struct device *dev)
{
	struct bma023_data *bma023 = dev_get_drvdata(dev);

	mutex_lock(&bma023->enable_mutex);
	if (bma023_get_enable(dev)) {
		cancel_delayed_work_sync(&bma023->work);
		bma023_power_down(bma023);
	}
	mutex_unlock(&bma023->enable_mutex);

	return 0;
}

static int bma023_resume(struct device *dev)
{
	struct bma023_data *bma023 = dev_get_drvdata(dev);
	int delay = atomic_read(&bma023->delay);

	bma023_hw_init(bma023);
	bma023_set_delay(dev, delay);

	mutex_lock(&bma023->enable_mutex);
	if (bma023_get_enable(dev)) {
		bma023_power_up(bma023);
		schedule_delayed_work(&bma023->work,
				      delay_to_jiffies(delay) + 1);
	}
	mutex_unlock(&bma023->enable_mutex);

	return 0;
}

static const struct dev_pm_ops bma023_pm_ops = {
	.suspend = bma023_suspend,
	.resume = bma023_resume,
};

static const struct i2c_device_id bma023_id[] = {
	{BMA023_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, bma023_id);

struct i2c_driver bma023_driver = {
	.driver = {
		.name = "bma023",
		.owner = THIS_MODULE,
		.pm = &bma023_pm_ops,
	},
	.probe = bma023_probe,
	.remove = bma023_remove,
	.id_table = bma023_id,
};

static int __init bma023_init(void)
{
	return i2c_add_driver(&bma023_driver);
}
module_init(bma023_init);

static void __exit bma023_exit(void)
{
	i2c_del_driver(&bma023_driver);
}
module_exit(bma023_exit);

MODULE_DESCRIPTION("BMA023 accelerometer driver");
MODULE_AUTHOR("tim.sk.lee@samsung.com");
MODULE_LICENSE("GPL");
MODULE_VERSION(1.0.0);
