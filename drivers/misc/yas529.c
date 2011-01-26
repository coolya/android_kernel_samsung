/*
 * Copyright (c) 2011 Kolja Dummann <k.dummann@gmail.com>
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/yas529.h>

#include "yas529_const.h"

struct yas529_data {
	struct input_dev *input_data;
	struct yas529_platform_data *pdata;
	struct i2c_client *this_client;
	struct mutex lock;
	struct hrtimer timer;
	struct workqueue_struct *wq;
	struct work_struct work;
	int transform;
	int powerstate;
	int16_t matrix[9]; 
    uint8_t rough_offset[3];
	int8_t temp_coeff[3];
	int16_t temperature;
	ktime_t polling_delay;
};

/*----- YAMAHA YAS529 Registers ------*/
enum YAS_REG {
       YAS_REG_CMDR            = 0x00, /* 000 < 5 */
       YAS_REG_XOFFSETR        = 0x20, /* 001 < 5 */
       YAS_REG_Y1OFFSETR       = 0x40, /* 010 < 5 */
       YAS_REG_Y2OFFSETR       = 0x60, /* 011 < 5 */
       YAS_REG_ICOILR          = 0x80, /* 100 < 5 */
       YAS_REG_CAL             = 0xA0, /* 101 < 5 */
       YAS_REG_CONFR           = 0xC0, /* 110 < 5 */
       YAS_REG_DOUTR           = 0xE0  /* 111 < 5 */
};

static const int8_t
		YAS529_TRANSFORMATION[][9] = { { 0, 1, 0, -1, 0, 0, 0, 0, 1 }, { -1, 0,
				0, 0, -1, 0, 0, 0, 1 }, { 0, -1, 0, 1, 0, 0, 0, 0, 1 }, { 1, 0,
				0, 0, 1, 0, 0, 0, 1 }, { 0, -1, 0, -1, 0, 0, 0, 0, -1 }, { 1,
				0, 0, 0, -1, 0, 0, 0, -1 }, { 0, 1, 0, 1, 0, 0, 0, 0, -1 }, {
				-1, 0, 0, 0, 1, 0, 0, 0, -1 }, };

/* I2C helper */
static int yas529_i2c_write(struct yas529_data *data, int len,
		const uint8_t *buf) {
	return i2c_master_send(data->this_client, buf, len);
}

static int yas529_i2c_read(struct yas529_data *data, int len, uint8_t *buf) {
	return i2c_master_recv(data->this_client, buf, len);
}

static int32_t calc_intensity(const int32_t *p) {
	int32_t intensity = 0;
	int i;

	for (i = 0; i < 3; i++) {
		intensity += ((p[i] / 1000) + 1) * ((p[i] / 1000) + 1);
	}

	return intensity;
}





static int yas529_cdrv_transform(const int16_t *matrix, const int32_t *raw,
		int32_t *data) {
	int i, j;
	int32_t temp;

	for (i = 0; i < 3; ++i) {
		temp = 0;
		for (j = 0; j < 3; ++j) {
			temp += (int32_t) matrix[i * 3 + j] * raw[j];
		}
		data[i] = temp;
	}

	return 0;
}

static int yas529_temperature_correction(struct yas529_data *data,
		const int32_t *raw, int32_t *msens) {

	static const int16_t center16[3] = { 16 * YAS529_CDRV_CENTER_X, 16
			* YAS529_CDRV_CENTER_Y1, 16 * YAS529_CDRV_CENTER_Y2 };

	int32_t temp_rawdata[3];
	int32_t raw_xyz[3];
	int32_t temps32;
	int i;

	for (i = 0; i < 3; ++i) {

		LOGV("%s: raw[%d] = %d", __func__,i, raw[i]);

		temp_rawdata[i] = raw[i];
		temp_rawdata[i] <<= 4;
		temp_rawdata[i] -= center16[i];

		/*
		 Memo:
		 The number '3 / 100' is approximated to '0x7ae1 / 2^20'.
		 */

		temps32 = ((int32_t) data->temperature - YAS529_CDRV_CENTER_T)
				* data->temp_coeff[i] * 0x7ae1;

		LOGV("%s: temps32 = %d", __func__,temps32);

		if (temps32 >= 0) {
			temp_rawdata[i] -= (int16_t) (temps32 >> 16);
		} else {
			temp_rawdata[i] += (int16_t) ((-temps32) >> 16);
		}

LOGV	("%s: temp_rawdata[%d] = %d", __func__, i, temp_rawdata[i]);
}

/*calculate real Y and Z axis values from the two Y axis */
raw_xyz[0] = -temp_rawdata[0];
raw_xyz[1] = temp_rawdata[2] - temp_rawdata[1];
raw_xyz[2] = temp_rawdata[2] + temp_rawdata[1];

//msens = raw_xyz;

if(data->transform)
yas529_cdrv_transform(data->matrix, raw_xyz, msens);

for (i = 0; i < 3; ++i) {
	LOGV("%s: raw_xyz[%d] = %d", __func__, i, raw_xyz[i]);
	msens[i] = raw_xyz[i]; // 1600;
	LOGV ("%s: msens[%d] = %d", __func__, i, msens[i]);
}

return 0;
}

static int yas529_measure_sensor(struct yas529_data *data, int32_t *msens) {
	int rv;
	uint8_t dat;

	dat = MS3CDRV_RDSEL_MEASURE;
	if (yas529_i2c_write(data, 1, &dat) < 0) {
		return YAS529_CDRV_ERR_I2CCTRL;
	}

	uint8_t buf[8];
	rv = YAS529_CDRV_NO_ERROR;
	int32_t temp_msens[3];
	int i;

	dat = MS3CDRV_CMD_MEASURE_XY1Y2T;
	if (yas529_i2c_write(data, 1, &dat) < 0) {
		return YAS529_CDRV_ERR_I2CCTRL;
	}

	for (i = 0; i < MS3CDRV_WAIT_MEASURE_XY1Y2T; i++) {
		msleep(1);

		if (yas529_i2c_read(data, 8, buf) < 0) {
			return YAS529_CDRV_ERR_I2CCTRL;
		}
		if (!(buf[0] & 0x80)) {
			break;
		}
	}

	if (buf[0] & 0x80) {
		return YAS529_CDRV_ERR_BUSY;
	}

	for (i = 0; i < 3; ++i) {
		temp_msens[2 - i] = ((buf[i << 1] & 0x7) << 8) + buf[(i << 1) | 1];
	}

	data->temperature = ((buf[6] & 0x7) << 8) + buf[7];

	LOGV("%s: temperatur = %d", __func__, data->temperature);

	if (temp_msens[0] <= YAS529_CDRV_NORMAL_MEASURE_UF_VALUE || temp_msens[0]
			>= YAS529_CDRV_NORMAL_MEASURE_OF_VALUE) {
		rv |= YAS529_CDRV_MEASURE_X_OFUF;
	}
	if (temp_msens[1] <= YAS529_CDRV_NORMAL_MEASURE_UF_VALUE || temp_msens[1]
			>= YAS529_CDRV_NORMAL_MEASURE_OF_VALUE) {
		rv |= YAS529_CDRV_MEASURE_Y1_OFUF;
	}
	if (temp_msens[2] <= YAS529_CDRV_NORMAL_MEASURE_UF_VALUE || temp_msens[2]
			>= YAS529_CDRV_NORMAL_MEASURE_OF_VALUE) {
		rv |= YAS529_CDRV_MEASURE_Y2_OFUF;
	}

	yas529_temperature_correction(data, temp_msens, msens);

	return rv;
}

static int yas529_measure(struct yas529_data *data, int32_t *magnetic) {
	int32_t intensity = 0;
	int result;

	int err = 0, i;
	int32_t raw[3];

	if ((err = yas529_measure_sensor(data, raw)) < 0) {

		if ((err = yas529_measure_sensor(data, raw)) < 0) {
			return err;
		}
	}

	if (err & MEASURE_RESULT_OVER_UNDER_FLOW) {
LOGI	("%s x[%d] y[%d] z[%d]\n", __func__,
			!!(err & 0x01), !!(err & 0x02), !!(err & 0x04));
}

for (i = 0; i < 3; i++) {
	LOGV("%s :raw[%d] = %d", __func__, i, raw[i]);
	magnetic[i] = raw[i];
	//magnetic[i] *= 400; /* typically raw * 400 is nT in unit */
	LOGV("%s :magnetic[%d] = %d", __func__, i, magnetic[i]);
}

intensity = calc_intensity(magnetic);

LOGV("%s : intensity = %d", __func__, intensity);

return err;
}




STATIC int
yas529_cdrv_measure_rough_offset(struct yas529_data *data, uint8_t *rough_offset)
{
    int i;
    uint8_t dat;
    uint8_t buf[6];
    int rv = YAS529_CDRV_NO_ERROR;

    if (rough_offset == NULL) {
        return YAS529_CDRV_ERR_ARG;
    }

    dat = MS3CDRV_RDSEL_MEASURE;
    if (yas529_i2c_write(data, 1, &dat) < 0) {
        return YAS529_CDRV_ERR_I2CCTRL;
    }

    dat = MS3CDRV_CMD_MEASURE_ROUGHOFFSET;
    if (yas529_i2c_write(data, 1, &dat) < 0) {
        return YAS529_CDRV_ERR_I2CCTRL;
    }

    msleep(MS3CDRV_WAIT_MEASURE_ROUGHOFFSET);

    if (yas529_i2c_read(data, 6, buf) < 0) {
        return YAS529_CDRV_ERR_I2CCTRL;
    }

    if (buf[0] & 0x80) {
        return YAS529_CDRV_ERR_BUSY;
    }

    for (i = 0; i < 3; ++i) {
        rough_offset[2 - i] = ((buf[i << 1] & 0x7) << 8) | buf[(i << 1) | 1];
    }

    if (rough_offset[0] <= YAS529_CDRV_ROUGHOFFSET_MEASURE_UF_VALUE
        || rough_offset[0] >= YAS529_CDRV_ROUGHOFFSET_MEASURE_OF_VALUE) {
        rv |= YAS529_CDRV_MEASURE_X_OFUF;
    }
    if (rough_offset[1] <= YAS529_CDRV_ROUGHOFFSET_MEASURE_UF_VALUE
        || rough_offset[1] >= YAS529_CDRV_ROUGHOFFSET_MEASURE_OF_VALUE) {
        rv |= YAS529_CDRV_MEASURE_Y1_OFUF;
    }
    if (rough_offset[2] <= YAS529_CDRV_ROUGHOFFSET_MEASURE_UF_VALUE
        || rough_offset[2] >= YAS529_CDRV_ROUGHOFFSET_MEASURE_OF_VALUE) {
        rv |= YAS529_CDRV_MEASURE_Y2_OFUF;
    }

    return rv;
}

STATIC int
yas529_cdrv_set_rough_offset(struct yas529_data *data, const uint8_t *rough_offset)
{
    int i;
    uint8_t dat;
    static const uint8_t addr[3] = { 0x20, 0x40, 0x60};
    uint8_t tmp[3];
    int rv = YAS529_CDRV_NO_ERROR;

    if (rough_offset == NULL) {
        return YAS529_CDRV_ERR_ARG;
    }
    if (rough_offset[0] > 32 || rough_offset[1] > 32 || rough_offset[2] > 32) {
        return YAS529_CDRV_ERR_ARG;
    }

    for (i = 0; i < 3; i++) {
        tmp[i] = rough_offset[i];
    }

    for (i = 0; i < 3; ++i) {
        if (tmp[i] <= 5) {
            tmp[i] = 0;
        }
        else {
            tmp[i] -= 5;
        }
    }
    for (i = 0; i < 3; ++i) {
        dat = addr[i] | tmp[i];
        if (yas529_i2c_write(data, 1, &dat) < 0) {
            return YAS529_CDRV_ERR_I2CCTRL;
        }
    }

    //c_driver.roughoffset_is_set = 1;

    return rv;
}





static int yas529_init_correction_matrix(const int8_t *transfrom,
		const int16_t *matrix, int16_t *target) {
	int i, j, k;
	int16_t temp16;

	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			temp16 = 0;
			for (k = 0; k < 3; ++k) {
				temp16 += transfrom[i * 3 + k] * matrix[k * 3 + j];
			}
			target[i * 3 + j] = temp16;
		}
	}

	return 0;
}

static int yas529_init_sensor(struct yas529_data *data) {
	int i;
	uint8_t buf[9];
	uint8_t dat;
	unsigned char rawData[6];
	uint8_t tempu8;
	//uint8_t rough_offset[3];
	
	int result = 0;
	short xoffset, y1offset, y2offset;

	static const uint8_t addr[3] = { 0x20, 0x40, 0x60};
    	uint8_t tmp[3];
	
	unsigned char dummyData[1] = { 0 };
    	unsigned char dummyRegister = 0;

	LOGV("%s: IN", __func__);
	
		   
	if(yas529_cdrv_measure_rough_offset(data, data->rough_offset) != YAS529_CDRV_NO_ERROR){
		LOGE("%s: Error setting up Offset", __func__);
	}

	if(yas529_cdrv_set_rough_offset(data, data->rough_offset) != YAS529_CDRV_NO_ERROR){
		LOGE("%s: Error setting up Offset", __func__);
	}
	
	
	/* preparation to read CAL register */
	dat = MS3CDRV_CMD_MEASURE_ROUGHOFFSET;
	if (yas529_i2c_write(data, 1, &dat) < 0) {
		LOGE("%s: yas529_mach_i2c_write(%d) failed", __FUNCTION__,
				MS3CDRV_CMD_MEASURE_ROUGHOFFSET);
		return YAS529_CDRV_ERR_I2CCTRL;
	}
	msleep(MS3CDRV_WAIT_MEASURE_ROUGHOFFSET);

	
	dat = MS3CDRV_RDSEL_CALREGISTER;
	if (yas529_i2c_write(data, 1, &dat) < 0) {
		LOGE("%s: yas529_mach_i2c_write(%d) failed", __FUNCTION__,
				MS3CDRV_RDSEL_CALREGISTER);
		return YAS529_CDRV_ERR_I2CCTRL;
	}
	

	/* dummy read */
	if (yas529_i2c_read(data, 9, buf) < 0) {
		LOGE("%s: yas529_mach_i2c_read() failed", __FUNCTION__);
		return YAS529_CDRV_ERR_I2CCTRL;
	}

	if (yas529_i2c_read(data, 9, buf) < 0) {
		LOGE("%s: yas529_mach_i2c_read() - 2 failed", __FUNCTION__);
		return YAS529_CDRV_ERR_I2CCTRL;
	}
	

	int16_t tempMatrix[9];

	/* correction matrix black magic */
	tempMatrix[0] = 100;
	LOGV("%s: tempMatrix[0] = %d", __func__, tempMatrix[0]);

	tempu8 = (buf[0] & 0xfc) >> 2;
	tempMatrix[1] = tempu8 - 0x20;
	LOGV("%s: tempMatrix[1] = %d", __func__, tempMatrix[1]);

	tempu8 = ((buf[0] & 0x3) << 2) | ((buf[1] & 0xc0) >> 6);
	tempMatrix[2] = tempu8 - 8;
	LOGV("%s: tempMatrix[2] = %d", __func__, tempMatrix[2]);

	tempu8 = buf[1] & 0x3f;
	tempMatrix[3] = tempu8 - 0x20;
	LOGV("%s: tempMatrix[3] = %d", __func__, tempMatrix[3]);

	tempu8 = (buf[2] & 0xfc) >> 2;
	tempMatrix[4] = tempu8 + 38;
	LOGV("%s: tempMatrix[4] = %d", __func__, tempMatrix[4]);

	tempu8 = ((buf[2] & 0x3) << 4) | (buf[3] & 0xf0) >> 4;
	tempMatrix[5] = tempu8 - 0x20;
	LOGV("%s: tempMatrix[5] = %d", __func__, tempMatrix[5]);

	tempu8 = ((buf[3] & 0xf) << 2) | ((buf[4] & 0xc0) >> 6);
	tempMatrix[6] = tempu8 - 0x20;
	LOGV("%s: tempMatrix[6] = %d", __func__, tempMatrix[6]);

	tempu8 = buf[4] & 0x3f;
	tempMatrix[7] = tempu8 - 0x20;
	LOGV("%s: tempMatrix[7] = %d", __func__, tempMatrix[7]);

	tempu8 = (buf[5] & 0xfe) >> 1;
	tempMatrix[8] = tempu8 + 66;
	LOGV("%s: tempMatrix[8] = %d", __func__, tempMatrix[8]);

	yas529_init_correction_matrix(
			YAS529_TRANSFORMATION[CONFIG_INPUT_YAS529_POSITION], tempMatrix,
			data->matrix);

	for (i = 0; i < 3; ++i) {
		data->temp_coeff[i] = (int8_t) ((int16_t) buf[i + 6] - 0x80);
	}

	return 0;
}

static void yas529_reset(struct yas529_data *data) {
	int counter = 0;
	uint8_t cmd[2] = { 0 };
	struct i2c_msg i2cmsg[2];

	i2cmsg[0].addr = data->this_client->addr;
	i2cmsg[0].flags = 0;
	i2cmsg[0].len = 1;
	i2cmsg[0].buf = cmd;
	i2cmsg[1].addr = data->this_client->addr;
	i2cmsg[1].flags = 1;
	i2cmsg[1].len = 2;
	i2cmsg[1].buf = cmd;

	cmd[0] = 0xd0;

	for (counter = 0; counter < 3; counter++) {

		gpio_set_value(data->pdata->reset_line, data->pdata->reset_asserted);
		msleep(2);
		gpio_set_value(data->pdata->reset_line, !data->pdata->reset_asserted);

		if (i2c_transfer(data->this_client->adapter, i2cmsg, 2) < 0) {
dev_err		(&data->this_client->dev, "[init] %d Read I2C ERROR!\n",
				counter);
	}
	dev_err(&data->this_client->dev, "[init] %d DeviceID is %x %x\n",
			counter, cmd[0], cmd[1]);
	if (cmd[1] == 0x40)
	break;
	msleep(10);
	cmd[0] = 0xd0;
	cmd[1] = 0;
}

yas529_init_sensor(data);
}

static ssize_t yas529_show_data(struct device *dev,
		struct device_attribute *attr, char *buf) {
	struct yas529_data *data = dev_get_drvdata(dev);
	int32_t magdata[3];
	mutex_lock(&data->lock);
	yas529_measure(data, magdata);
	mutex_unlock(&data->lock);

	return sprintf(buf, "%d, %d, %d\n", magdata[0], magdata[1], magdata[2]);
}

static DEVICE_ATTR(data, S_IRUGO, yas529_show_data, NULL);

static ssize_t yas529_show_transform(struct device *dev,
		struct device_attribute *attr, char *buf) {
	struct yas529_data *data = dev_get_drvdata(dev);
	int transform;

	mutex_lock(&data->lock);
	transform = data->transform;
	mutex_unlock(&data->lock);

	return sprintf(buf, "%d\n", transform);
}

static ssize_t yas529_store_transform(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) {
	struct yas529_data *data = dev_get_drvdata(dev);
	int transform;

	if (sysfs_streq(buf, "1"))
		transform = 1;
	else if (sysfs_streq(buf, "0"))
		transform = 0;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&data->lock);
	data->transform = transform;
	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(transform, S_IRUGO | S_IWUSR | S_IWGRP, yas529_show_transform, yas529_store_transform);

static void yas529_enable(struct yas529_data *data)
{
	LOGV("%s: sensor ON", __func__);
	yas529_reset(data);
	hrtimer_start(&data->timer, data->polling_delay, HRTIMER_MODE_REL);
}

static void yas529_disable(struct yas529_data *data)
{
	LOGV("%s: sensor OFF", __func__);
	hrtimer_cancel(&data->timer);
	cancel_work_sync(&data->work);

}


static ssize_t yas529_show_enable(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct yas529_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->powerstate);
}

static ssize_t yas529_store_enable(struct device *dev,
				  struct device_attribute *attr, const char *buf, size_t count)
{
	struct yas529_data *data = dev_get_drvdata(dev);

	bool new_value;
	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if(data->powerstate != new_value && new_value == true)
	{
		data->powerstate = new_value;
		yas529_enable(data);
	}else if(data->powerstate != new_value && new_value == false){
		data->powerstate = new_value;
		yas529_disable(data);
	}

	return count;

}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP, yas529_show_enable, yas529_store_enable);

static struct attribute *yas529_attributes[] = { &dev_attr_data.attr,
		&dev_attr_transform.attr, &dev_attr_enable.attr, NULL };

static const struct attribute_group
		yas529_group = { .attrs = yas529_attributes, };


/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart yas529_timer_func(struct hrtimer *timer)
{
	struct yas529_data *data = container_of(timer, struct yas529_data, timer);

	queue_work(data->wq, &data->work);
	hrtimer_forward_now(&data->timer, data->polling_delay);
	return HRTIMER_RESTART;
}


static void yas529_input_work_func(struct work_struct *work) {

	struct yas529_data *data = container_of(work, struct yas529_data, work);
	int32_t magdata[3];
	int rt;
	//FIXME mutex !
	LOGV("%s working", __func__);
	mutex_lock(&data->lock);
	
	//Read Mag Data	
	rt = yas529_measure(data, magdata);

	if (rt < 0) {
		pr_err("work on compass failed[%d]\n", rt);
	}
	mutex_unlock(&data->lock);
	
	if (rt >= 0) {

		/* report magnetic data */
		input_report_abs(data->input_data, ABS_X, magdata[0]);
		input_report_abs(data->input_data, ABS_Y, magdata[1]);
		input_report_abs(data->input_data, ABS_Z, magdata[2]);
		input_sync(data->input_data);

	}
}



static int yas529_probe(struct i2c_client *client,
	const struct i2c_device_id *id) {
	struct yas529_data *data = NULL;
	struct input_dev *input_dev = NULL;
	int err = 0;

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit_platform_data_null;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C check failed, exiting.\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	data = kzalloc(sizeof(struct yas529_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	data->pdata = client->dev.platform_data;
	mutex_init(&data->lock);

	data->this_client = client;
	i2c_set_clientdata(client, data);

	err = gpio_request(data->pdata->reset_line, "YAS529 Reset Line");
	if (err < 0)
		goto exit_reset_gpio_request_failed;
	gpio_direction_output(data->pdata->reset_line, !data->pdata->reset_asserted);

	yas529_reset(data);

	/* allocate accel input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate_device;
	}
	data->transform = 1;
	data->input_data = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "yas529";

	input_set_capability(input_dev, EV_ABS, ABS_X);
	input_set_capability(input_dev, EV_ABS, ABS_Y);
	input_set_capability(input_dev, EV_ABS, ABS_Z);

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev, "registering input device is failed\n");
		goto failed_reg;
	}

	err = sysfs_create_group(&input_dev->dev.kobj, &yas529_group);
	if (err) {
		dev_err(&input_dev->dev, "creating attribute group is failed\n");
		goto failed_sysfs;
	}

	/* hrtimer settings.  we poll for magnetic values using a timer. */
	hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->polling_delay = ns_to_ktime(100 * NSEC_PER_MSEC);
	data->timer.function = yas529_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	 to read the i2c (can be slow and blocking). */
	data->wq = create_singlethread_workqueue("yas529_wq");
	if (!data->wq) {
		err = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
	}

	INIT_WORK(&data->work,yas529_input_work_func );

	dev_info(&client->dev, "registered\n");
	return 0;

	failed_sysfs: failed_reg: if (input_dev)
		input_free_device(input_dev);
	err_input_allocate_device:
	mutex_destroy(&data->lock);
	exit_reset_gpio_request_failed: kfree(data);
	exit_alloc_data_failed: exit_check_functionality_failed: exit_platform_data_null: return err;

}

static void yas529_unregister_input_device(struct yas529_data *data) {

	input_unregister_device(data->input_data);
	data->input_data = NULL;
}

static int __devexit yas529_remove(struct i2c_client *client) {
	struct yas529_data *data = i2c_get_clientdata(client);

	yas529_unregister_input_device(data);
	sysfs_remove_group(&client->dev.kobj, &yas529_group);

	hrtimer_cancel(&data->timer);
	cancel_work_sync(&data->work);
	destroy_workqueue(data->wq);

	kfree(data);
	return 0;
}

/* I2C Device Driver */
static struct i2c_device_id yas529_idtable[] = { { YAS_I2C_DEVICE_NAME, 0 },
		{ } };
MODULE_DEVICE_TABLE( i2c, yas529_idtable);

static struct i2c_driver yas529_i2c_driver = { .driver = {
	.name = YAS_I2C_DEVICE_NAME,
	.owner = THIS_MODULE,
},

.id_table = yas529_idtable, .probe = yas529_probe, .remove = yas529_remove,

};

static int __init
yas529_init(void) {
	return i2c_add_driver(&yas529_i2c_driver);
}

static void __exit
yas529_term(void) {
	i2c_del_driver(&yas529_i2c_driver);
}
module_init(yas529_init)
;
module_exit(yas529_term)
;

MODULE_AUTHOR("Kolja Dummann");
MODULE_DESCRIPTION("YAS529 Geomagnetic Sensor Driver");
MODULE_LICENSE( "GPL" );
MODULE_VERSION("0.1");
