/*
 * Copyright (c) 2010 Yamaha Corporation
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

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <asm/atomic.h>
#include "yas529.h"
#include <linux/yas529.h>

#define YAS529_DEFAULT_CALIB_INTERVAL	50	/* 50 msecs */
#define YAS529_DEFAULT_DATA_INTERVAL	200	/* 200 msecs */
#define YAS529_INITCOIL_INTERVAL	3000	/* 3 seconds */
#define YAS529_INITCOIL_GIVEUP_INTERVAL	180000 /* 180 seconds */
#define YAS529_INTENSITY_THRESH0	120	/* 120 uT */
#define YAS529_INTENSITY_THRESH1	90	/* 90 uT */
#define YAS529_DETECT_OVERFLOW_INTERVAL	0	/* 0 second */
#define YAS529_MAX_FILTER_LEN		30
#define YAS529_DEFAULT_FILTER_LEN	10

#define MEASURE_ERROR_DELAY		100
#define MEASURE_STATE_NORMAL		0
#define MEASURE_STATE_INIT_COIL		1
#define MEASURE_STATE_ROUGH_MEASURE	2
#define MEASURE_RESULT_OVER_UNDER_FLOW	0x07

#define REL_STATUS			(REL_RX)
#define REL_WAKE			(REL_RY)

#define REL_RAW_DISTORTION		(REL_HWHEEL)
#define REL_RAW_THRESHOLD		(REL_DIAL)
#define REL_RAW_SHAPE			(REL_WHEEL)
#define REL_RAW_REPORT			(REL_MISC)

#define INTENSITY_THRESHOLD_1 \
	((YAS529_INTENSITY_THRESH1)*(YAS529_INTENSITY_THRESH1))
#define INTENSITY_THRESHOLD_0 \
	((YAS529_INTENSITY_THRESH0)*(YAS529_INTENSITY_THRESH0))
#define MIN(a, b)	((a) > (b) ? (b) : (a))


struct yas529_compact_driver {
	uint8_t raw_calreg[9];
	int8_t roughoffset_is_set;
	int16_t matrix[9];
	int16_t correction_m[9];
	int8_t temp_coeff[3];
	int8_t initialized;
	int16_t temperature;
};

static struct yas529_compact_driver c_driver = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	0,
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0},
	0,
	-1
};

static const int8_t YAS529_TRANSFORMATION[][9] = {
	{ 0, 1, 0, -1, 0, 0, 0, 0, 1 },
	{ -1, 0, 0, 0, -1, 0, 0, 0, 1 },
	{ 0, -1, 0, 1, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 1, 0, 0, 0, 1 },
	{ 0, -1, 0, -1, 0, 0, 0, 0, -1 },
	{ 1, 0, 0, 0, -1, 0, 0, 0, -1 },
	{ 0, 1, 0, 1, 0, 0, 0, 0, -1 },
	{ -1, 0, 0, 0, 1, 0, 0, 0, -1 },
};

static const int supported_data_interval[] = {1, 20, 60, 200, 1000};
static const int supported_calib_interval[] = {10, 20, 60, 50, 50};
static const int32_t INVALID_FINE_OFFSET[]
				= {0x7fffffff, 0x7fffffff, 0x7fffffff};
static const uint8_t INVALID_ROUGH_OFFSET[] = {0xff, 0xff, 0xff};

struct yas529_fir_filter {
	int num;
	int filter_len;
	int index;
	int32_t sequence[YAS529_MAX_FILTER_LEN];
};

struct yas529_driver {
	struct geomagnetic_hwdep_callback callback;
	struct utimer data_timer;
	struct utimer initcoil_timer;
	struct utimer initcoil_giveup_timer;
	struct utimer detect_overflow_timer;
	int32_t fine_offset[3];
	int accuracy;
	uint8_t rough_offset[3];
	int32_t prev_magnetic[3];
	int32_t prev_magnetic_raw[3];
	int measure_state;
	int active;
	int overflow;
	int initcoil_gaveup;
	int position;
	int delay_timer_use_data;
	int delay_timer_interval;
	int delay_timer_counter;
	int filter_enable;
	int filter_len;
	struct yas529_fir_filter filter[3];
};

static struct yas529_driver this_driver = {
	.measure_state = MEASURE_STATE_NORMAL,
	.active = 0,
	.overflow = 0,
	.initcoil_gaveup = 0,
	.position = 0,
	.delay_timer_use_data = 0,
	.delay_timer_interval = (YAS529_DEFAULT_DATA_INTERVAL
				/ YAS529_DEFAULT_CALIB_INTERVAL),
	.delay_timer_counter = (YAS529_DEFAULT_DATA_INTERVAL
				/ YAS529_DEFAULT_CALIB_INTERVAL),
	.filter_len = YAS529_DEFAULT_FILTER_LEN,
};

static int geomagnetic_lock(void);
static int geomagnetic_unlock(void);
static void geomagnetic_current_time(int32_t *sec, int32_t *msec);

struct geomagnetic_data {
	struct input_dev *input_data;
	struct input_dev *input_raw;
    struct yas529_platform_data *pdata;
	struct delayed_work work;
	struct semaphore driver_lock;
	struct semaphore multi_lock;
	atomic_t last_data[3];
	atomic_t last_status;
	atomic_t enable;
	atomic_t filter_enable;
	atomic_t filter_len;
	atomic_t delay;
	int32_t threshold;
	int32_t distortion[3];
	int32_t shape;
	struct yas529_driver_state driver_state;
};

static struct geomagnetic_hwdep_driver hwdep_driver = {
	.callback = {
		.lock = geomagnetic_lock,
		.unlock = geomagnetic_unlock,
		.current_time = geomagnetic_current_time,
	},
};

static struct i2c_client *this_client;

static void (*current_time)(int *sec, int *msec);


STATIC int
utimeval_init(struct utimeval *val)
{
	if (val == NULL)
		return -1;
	val->tv_sec = val->tv_msec = 0;
	return 0;
}

STATIC int
utimeval_is_initial(struct utimeval *val)
{
	if (val == NULL)
		return 0;
	return val->tv_sec == 0 && val->tv_msec == 0;
}

STATIC int
utimeval_is_overflow(struct utimeval *val)
{
	int32_t max;

	if (val == NULL)
		return 0;

	max = (int32_t) ((uint32_t) 0xffffffff / (uint32_t) 1000);
	if (val->tv_sec > max)
		return 1; /* overflow */
	else if (val->tv_sec == max) {
		if (val->tv_msec >
			(int32_t)((uint32_t)0xffffffff % (uint32_t)1000))
			return 1; /* overflow */
	}

	return 0;
}

STATIC struct utimeval
utimeval_plus(struct utimeval *first, struct utimeval *second)
{
	struct utimeval result = {0, 0};
	int32_t tmp;

	if (first == NULL || second == NULL)
		return result;

	tmp = first->tv_sec + second->tv_sec;
	if (first->tv_sec >= 0 && second->tv_sec >= 0 && tmp < 0)
		goto overflow;
	if (first->tv_sec < 0 && second->tv_sec < 0 && tmp >= 0)
		goto underflow;

	result.tv_sec = tmp;
	result.tv_msec = first->tv_msec + second->tv_msec;
	if (1000 <= result.tv_msec) {
		tmp = result.tv_sec + result.tv_msec / 1000;
		if (result.tv_sec >= 0 && result.tv_msec >= 0 && tmp < 0)
			goto overflow;
		result.tv_sec = tmp;
		result.tv_msec = result.tv_msec % 1000;
	}
	if (result.tv_msec < 0) {
		tmp = result.tv_sec + result.tv_msec / 1000 - 1;
		if (result.tv_sec < 0 && result.tv_msec < 0 && tmp >= 0)
			goto underflow;
		result.tv_sec = tmp;
		result.tv_msec = result.tv_msec % 1000 + 1000;
	}

	return result;

overflow:
	result.tv_sec = 0x7fffffff;
	result.tv_msec = 999;
	return result;

underflow:
	result.tv_sec = 0x80000000;
	result.tv_msec = 0;
	return result;
}

STATIC struct utimeval
utimeval_minus(struct utimeval *first, struct utimeval *second)
{
	struct utimeval result = {0, 0}, tmp;

	if (first == NULL ||
		second == NULL ||
		second->tv_sec == (int)0x80000000)
		return result;

	tmp.tv_sec = -second->tv_sec;
	tmp.tv_msec = -second->tv_msec;
	return utimeval_plus(first, &tmp);
}

STATIC int
utimeval_less_than(struct utimeval *first, struct utimeval *second)
{
	if (first == NULL || second == NULL)
		return 0;

	if (first->tv_sec > second->tv_sec)
		return 1;
	else if (first->tv_sec < second->tv_sec)
		return 0;
	else {
		if (first->tv_msec > second->tv_msec)
			return 1;
		else
			return 0;
	}
}

STATIC int
utimeval_greater_than(struct utimeval *first, struct utimeval *second)
{
	if (first == NULL || second == NULL)
		return 0;

	if (first->tv_sec < second->tv_sec)
		return 1;
	else if (first->tv_sec > second->tv_sec)
		return 0;
	else {
		if (first->tv_msec < second->tv_msec)
			return 1;
		else
			return 0;
	}
}

STATIC int
utimeval_greater_or_equal(struct utimeval *first,
			struct utimeval *second)
{
	return !utimeval_less_than(first, second);
}

STATIC int
utimeval_greater_than_zero(struct utimeval *val)
{
	struct utimeval zero = {0, 0};
	return utimeval_greater_than(&zero, val);
}

STATIC int
utimeval_less_than_zero(struct utimeval *val)
{
	struct utimeval zero = {0, 0};
	return utimeval_less_than(&zero, val);
}

STATIC struct utimeval *
msec_to_utimeval(struct utimeval *result, uint32_t msec)
{
	if (result == NULL)
		return result;
	result->tv_sec = msec / 1000;
	result->tv_msec = msec % 1000;

	return result;
}

STATIC uint32_t
utimeval_to_msec(struct utimeval *val)
{
	if (val == NULL)
		return 0;
	if (utimeval_less_than_zero(val))
		return 0;
	if (utimeval_is_overflow(val))
		return 0xffffffff;

	return val->tv_sec * 1000 + val->tv_msec;
}

STATIC struct utimeval
utimer_calc_next_time(struct utimer *ut, struct utimeval *cur)
{
	struct utimeval result = {0, 0}, delay;

	if (ut == NULL || cur == NULL)
		return result;

	utimer_update_with_curtime(ut, cur);
	if (utimer_is_timeout(ut))
		result = *cur;
	else {
		delay = utimeval_minus(&ut->delay_ms, &ut->total_time);
		result = utimeval_plus(cur, &delay);
	}

	return result;
}

STATIC struct utimeval
utimer_current_time(void)
{
	struct utimeval tv;
	int sec, msec;

	if (current_time != NULL)
		current_time(&sec, &msec);
	else
		sec = 0, msec = 0;

	tv.tv_sec = sec;
	tv.tv_msec = msec;

	return tv;
}

STATIC int
utimer_clear(struct utimer *ut)
{
	if (ut == NULL)
		return -1;
	utimeval_init(&ut->prev_time);
	utimeval_init(&ut->total_time);

	return 0;
}

STATIC int
utimer_update_with_curtime(struct utimer *ut, struct utimeval *cur)
{
	struct utimeval tmp;

	if (ut == NULL || cur == NULL)
		return -1;
	if (utimeval_is_initial(&ut->prev_time))
		ut->prev_time = *cur;
	if (utimeval_greater_than_zero(&ut->delay_ms)) {
		tmp = utimeval_minus(cur, &ut->prev_time);
		ut->total_time = utimeval_plus(&tmp, &ut->total_time);
		ut->prev_time = *cur;
	}

	return 0;
}

STATIC int
utimer_update(struct utimer *ut)
{
	struct utimeval cur;

	if (ut == NULL)
		return -1;
	cur = utimer_current_time();
	utimer_update_with_curtime(ut, &cur);
	return 0;
}

STATIC int
utimer_is_timeout(struct utimer *ut)
{
	if (ut == NULL)
		return 0;
	if (utimeval_greater_than_zero(&ut->delay_ms))
		return utimeval_greater_or_equal(&ut->delay_ms,
							&ut->total_time);
	else
		return 1;
}

STATIC int
utimer_clear_timeout(struct utimer *ut)
{
	uint32_t delay, total;

	if (ut == NULL)
		return -1;

	delay = utimeval_to_msec(&ut->delay_ms);
	if (delay == 0 || utimeval_is_overflow(&ut->total_time))
		total = 0;
	else {
		total = utimeval_to_msec(&ut->total_time);
		if (delay * 2 <= total)
			total = delay;
		else
			total = total % delay;
	}
	msec_to_utimeval(&ut->total_time, total);

	return 0;
}

STATIC uint32_t
utimer_sleep_time_with_curtime(struct utimer *ut, struct utimeval *cur)
{
	struct utimeval tv;

	if (ut == NULL || cur == NULL)
		return 0;
	tv = utimer_calc_next_time(ut, cur);
	tv = utimeval_minus(&tv, cur);
	if (utimeval_less_than_zero(&tv))
		return 0;

	return utimeval_to_msec(&tv);
}

STATIC uint32_t
utimer_sleep_time(struct utimer *ut)
{
	struct utimeval cur;

	if (ut == NULL)
		return 0;

	cur = utimer_current_time();
	return utimer_sleep_time_with_curtime(ut, &cur);
}

STATIC int
utimer_init(struct utimer *ut, uint32_t delay_ms)
{
	if (ut == NULL)
		return -1;
	utimer_clear(ut);
	msec_to_utimeval(&ut->delay_ms, delay_ms);

	return 0;
}

STATIC uint32_t
utimer_get_total_time(struct utimer *ut)
{
	return utimeval_to_msec(&ut->total_time);
}

STATIC uint32_t
utimer_get_delay(struct utimer *ut)
{
	if (ut == NULL)
		return -1;
	return utimeval_to_msec(&ut->delay_ms);
}

STATIC int
utimer_set_delay(struct utimer *ut, uint32_t delay_ms)
{
	return utimer_init(ut, delay_ms);
}

STATIC void
utimer_lib_init(void (*func)(int *sec, int *msec))
{
	current_time = func;
}

static int
yas529_cdrv_check_transformation_matrix(const int8_t *p)
{
	int i, j;
	int n;

	for (i = 0; i < 3; ++i) {
		n = 0;
		for (j = 0; j < 3; ++j) {
			if (p[i*3 + j] == 1 || p[i*3 + j] == -1)
				n++;
		}
		if (n != 1)
			return -1;
	}

	for (i = 0; i < 3; ++i) {
		n = 0;
		for (j = 0; j < 3; ++j) {
			if (p[j*3 + i] == 1 || p[j*3 + i] == -1)
				n++;
		}
		if (n != 1)
			return -1;
	}

	return 0;
}

static int
yas529_cdrv_make_correction_matrix(const int8_t *p, const int16_t *matrix,
					int16_t *ans)
{
	int i, j, k;
	int16_t temp16;

	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			temp16 = 0;
			for (k = 0; k < 3; ++k)
				temp16 += p[i*3 + k] * matrix[k*3 + j];
			ans[i*3 + j] = temp16;
		}
	}

	return 0;
}

static int
yas529_cdrv_transform(const int16_t *matrix, const int32_t *raw, int32_t *data)
{
	int i, j;
	int32_t temp;

	for (i = 0; i < 3; ++i) {
		temp = 0;
		for (j = 0; j < 3; ++j)
			temp += (int32_t)matrix[i*3 + j] * raw[j];
		data[i] = temp;
	}

	return 0;
}

static int
yas529_cdrv_msens_correction(const int32_t *raw, uint16_t temperature,
				int32_t *data)
{
	static const int16_t center16[3] = {
		16 * YAS529_CDRV_CENTER_X,
		16 * YAS529_CDRV_CENTER_Y1,
		16 * YAS529_CDRV_CENTER_Y2
	};
	int32_t temp_rawdata[3];
	int32_t raw_xyz[3];
	int32_t temps32;
	int i;

	for (i = 0; i < 3; i++)
		temp_rawdata[i] = raw[i];

	for (i = 0; i < 3; ++i) {
		temp_rawdata[i] <<= 4;
		temp_rawdata[i] -= center16[i];

		/*
		  Memo:
		  The number '3 / 100' is approximated to '0x7ae1 / 2^20'.
		*/

		temps32 = ((int32_t)temperature - YAS529_CDRV_CENTER_T)
				* c_driver.temp_coeff[i] * 0x7ae1;
		if (temps32 >= 0)
			temp_rawdata[i] -= (int16_t)(temps32 >> 16);
		else
			temp_rawdata[i] += (int16_t)((-temps32) >> 16);
	}

	raw_xyz[0] = -temp_rawdata[0];
	raw_xyz[1] = temp_rawdata[2] - temp_rawdata[1];
	raw_xyz[2] = temp_rawdata[2] + temp_rawdata[1];

	yas529_cdrv_transform(c_driver.correction_m, raw_xyz, data);

	for (i = 0; i < 3; ++i)
		data[i] /= 1600;

	return 0;
}

STATIC int
yas529_cdrv_actuate_initcoil(void)
{
	int i;
	static const uint8_t InitCoilTable[16] = {
		0x90, 0x81, 0x91, 0x82, 0x92, 0x83, 0x93, 0x84,
		0x94, 0x85, 0x95, 0x86, 0x96, 0x87, 0x97, 0x80
	};

	if (!c_driver.initialized)
		return YAS529_CDRV_ERR_NOT_INITIALIZED;

	for (i = 0; i < 16; ++i) {
		if (i2c_master_send(this_client, &InitCoilTable[i], 1) < 0)
			return YAS529_CDRV_ERR_I2CCTRL;
	}

	return YAS529_CDRV_NO_ERROR;
}

STATIC int
yas529_cdrv_measure_rough_offset(uint8_t *rough_offset)
{
	int i;
	uint8_t dat;
	uint8_t buf[6];
	int rv = YAS529_CDRV_NO_ERROR;

	if (rough_offset == NULL)
		return YAS529_CDRV_ERR_ARG;

	if (!c_driver.initialized)
		return YAS529_CDRV_ERR_NOT_INITIALIZED;

	dat = MS3CDRV_RDSEL_MEASURE;
	if (i2c_master_send(this_client, &dat, 1) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	dat = MS3CDRV_CMD_MEASURE_ROUGHOFFSET;
	if (i2c_master_send(this_client, &dat, 1) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	msleep(MS3CDRV_WAIT_MEASURE_ROUGHOFFSET);

	if (i2c_master_recv(this_client, buf, 6) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	if (buf[0] & 0x80)
		return YAS529_CDRV_ERR_BUSY;

	for (i = 0; i < 3; ++i)
		rough_offset[2 - i]
			= ((buf[i << 1] & 0x7) << 8) | buf[(i << 1) | 1];

	if (rough_offset[0] <= YAS529_CDRV_ROUGHOFFSET_MEASURE_UF_VALUE ||
		rough_offset[0] >= YAS529_CDRV_ROUGHOFFSET_MEASURE_OF_VALUE) {
		rv |= YAS529_CDRV_MEASURE_X_OFUF;
	}
	if (rough_offset[1] <= YAS529_CDRV_ROUGHOFFSET_MEASURE_UF_VALUE ||
		rough_offset[1] >= YAS529_CDRV_ROUGHOFFSET_MEASURE_OF_VALUE) {
		rv |= YAS529_CDRV_MEASURE_Y1_OFUF;
	}
	if (rough_offset[2] <= YAS529_CDRV_ROUGHOFFSET_MEASURE_UF_VALUE ||
		rough_offset[2] >= YAS529_CDRV_ROUGHOFFSET_MEASURE_OF_VALUE) {
		rv |= YAS529_CDRV_MEASURE_Y2_OFUF;
	}

	return rv;
}

STATIC int
yas529_cdrv_set_rough_offset(const uint8_t *rough_offset)
{
	int i;
	uint8_t dat;
	static const uint8_t addr[3] = { 0x20, 0x40, 0x60};
	uint8_t tmp[3];
	int rv = YAS529_CDRV_NO_ERROR;

	if (rough_offset == NULL)
		return YAS529_CDRV_ERR_ARG;
	if (rough_offset[0] > 32 || rough_offset[1] > 32
				|| rough_offset[2] > 32)
		return YAS529_CDRV_ERR_ARG;

	if (!c_driver.initialized)
		return YAS529_CDRV_ERR_NOT_INITIALIZED;

	for (i = 0; i < 3; i++)
		tmp[i] = rough_offset[i];

	for (i = 0; i < 3; ++i) {
		if (tmp[i] <= 5)
			tmp[i] = 0;
		else
			tmp[i] -= 5;
	}
	for (i = 0; i < 3; ++i) {
		dat = addr[i] | tmp[i];
		if (i2c_master_send(this_client, &dat, 1) < 0)
			return YAS529_CDRV_ERR_I2CCTRL;
	}

	c_driver.roughoffset_is_set = 1;

	return rv;
}

static int
yas529_cdrv_measure_preparation(void)
{
	uint8_t dat;

	dat = MS3CDRV_RDSEL_MEASURE;
	if (i2c_master_send(this_client, &dat, 1) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	return YAS529_CDRV_NO_ERROR;
}

static int
yas529_cdrv_measure_sub(int32_t *msens, int32_t *raw, int16_t *t)
{
	uint8_t dat;
	uint8_t buf[8];
	uint8_t rv = YAS529_CDRV_NO_ERROR;
	int32_t temp_msens[3];
	int i;

	if (c_driver.roughoffset_is_set == 0)
		return YAS529_CDRV_ERR_ROUGHOFFSET_NOT_WRITTEN;

	dat = MS3CDRV_CMD_MEASURE_XY1Y2T;
	if (i2c_master_send(this_client, &dat, 1) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	for (i = 0; i < MS3CDRV_WAIT_MEASURE_XY1Y2T; i++) {
		msleep(1);

		if (i2c_master_recv(this_client, buf, 8) < 0)
			return YAS529_CDRV_ERR_I2CCTRL;
		if (!(buf[0] & 0x80))
			break;
	}

	if (buf[0] & 0x80)
		return YAS529_CDRV_ERR_BUSY;

	for (i = 0; i < 3; ++i)
		temp_msens[2 - i]
			= ((buf[i << 1] & 0x7) << 8) + buf[(i << 1) | 1];
	c_driver.temperature = ((buf[6] & 0x7) << 8) + buf[7];

	if (temp_msens[0] <= YAS529_CDRV_NORMAL_MEASURE_UF_VALUE
		|| temp_msens[0] >= YAS529_CDRV_NORMAL_MEASURE_OF_VALUE)
			rv |= YAS529_CDRV_MEASURE_X_OFUF;
	if (temp_msens[1] <= YAS529_CDRV_NORMAL_MEASURE_UF_VALUE
		|| temp_msens[1] >= YAS529_CDRV_NORMAL_MEASURE_OF_VALUE)
			rv |= YAS529_CDRV_MEASURE_Y1_OFUF;
	if (temp_msens[2] <= YAS529_CDRV_NORMAL_MEASURE_UF_VALUE
		|| temp_msens[2] >= YAS529_CDRV_NORMAL_MEASURE_OF_VALUE)
			rv |= YAS529_CDRV_MEASURE_Y2_OFUF;

	yas529_cdrv_msens_correction(temp_msens, c_driver.temperature, msens);

	if (raw != NULL) {
		for (i = 0; i < 3; i++)
			raw[i] = temp_msens[i];
	}
	if (t != NULL)
		*t = c_driver.temperature;

	return (int)rv;
}

STATIC int
yas529_cdrv_measure(int32_t *msens, int32_t *raw, int16_t *t)
{
	int rv;

	if (!c_driver.initialized)
		return YAS529_CDRV_ERR_NOT_INITIALIZED;

	if (msens == NULL)
		return YAS529_CDRV_ERR_ARG;

	rv = yas529_cdrv_measure_preparation();
	if (rv < 0)
		return rv;

	rv = yas529_cdrv_measure_sub(msens, raw, t);

	return rv;
}

STATIC int
yas529_cdrv_recalc_fine_offset(int32_t *prev_fine_offset,
				int32_t *new_fine_offset,
				uint8_t *prev_rough_offset,
				uint8_t *new_rough_offset)
{
	int32_t tmp[3], resolution[9], base[3];
	int32_t raw[3];
	int32_t diff, i;

	if (!c_driver.initialized)
		return YAS529_CDRV_ERR_NOT_INITIALIZED;

	if (prev_fine_offset == NULL || new_fine_offset == NULL
		|| prev_rough_offset == NULL || new_rough_offset == NULL)
			return YAS529_CDRV_ERR_ARG;

	raw[0] = raw[1] = raw[2] = 0;
	yas529_cdrv_msens_correction(raw, c_driver.temperature, base);
	for (i = 0; i < 3; i++) {
		raw[0] = raw[1] = raw[2] = 0;
		raw[i] = 226;
		yas529_cdrv_msens_correction(raw, c_driver.temperature,
							&resolution[i*3]);
		resolution[i*3 + 0] -= base[0];
		resolution[i*3 + 1] -= base[1];
		resolution[i*3 + 2] -= base[2];
	}

	for (i = 0; i < 3; i++)
		tmp[i] = prev_fine_offset[i];
	for (i = 0; i < 3; i++) {
		diff = (int32_t)new_rough_offset[i]
				- (int32_t)prev_rough_offset[i];
		while (diff > 0) {
			tmp[0] -= resolution[i*3 + 0];
			tmp[1] -= resolution[i*3 + 1];
			tmp[2] -= resolution[i*3 + 2];
			diff--;
		}
		while (diff < 0) {
			tmp[0] += resolution[i*3 + 0];
			tmp[1] += resolution[i*3 + 1];
			tmp[2] += resolution[i*3 + 2];
			diff++;
		}
	}
	for (i = 0; i < 3; i++)
		new_fine_offset[i] = tmp[i];

	return 0;
}

STATIC int
yas529_cdrv_set_transformatiom_matrix(const int8_t *transform)
{
	if (transform == NULL)
		return YAS529_CDRV_ERR_ARG;

	if (!c_driver.initialized)
		return YAS529_CDRV_ERR_NOT_INITIALIZED;

	if (yas529_cdrv_check_transformation_matrix(transform) < 0)
		return YAS529_CDRV_ERR_ARG;

	yas529_cdrv_make_correction_matrix(transform, c_driver.matrix,
						c_driver.correction_m);

	return 0;
}

STATIC int
yas529_cdrv_init(const int8_t *transform)
{
	int i;
	uint8_t dat;
	uint8_t *buf = c_driver.raw_calreg;
	uint8_t tempu8;

	if (transform == NULL)
		return YAS529_CDRV_ERR_ARG;

	if (yas529_cdrv_check_transformation_matrix(transform) < 0)
		return YAS529_CDRV_ERR_ARG;

	c_driver.roughoffset_is_set = 0;


	/* preparation to read CAL register */
	dat = MS3CDRV_CMD_MEASURE_ROUGHOFFSET;
	if (i2c_master_send(this_client, &dat, 1) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;
	msleep(MS3CDRV_WAIT_MEASURE_ROUGHOFFSET);

	dat = MS3CDRV_RDSEL_CALREGISTER;
	if (i2c_master_send(this_client, &dat, 1) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	/* dummy read */
	if (i2c_master_recv(this_client, buf, 9) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	if (i2c_master_recv(this_client, buf, 9) < 0)
		return YAS529_CDRV_ERR_I2CCTRL;

	c_driver.matrix[0] = 100;

	tempu8 = (buf[0] & 0xfc) >> 2;
	c_driver.matrix[1] = tempu8 - 0x20;

	tempu8 = ((buf[0] & 0x3) << 2) | ((buf[1] & 0xc0) >> 6);
	c_driver.matrix[2] = tempu8 - 8;

	tempu8 = buf[1] & 0x3f;
	c_driver.matrix[3] = tempu8 - 0x20;

	tempu8 = (buf[2] & 0xfc) >> 2;
	c_driver.matrix[4] = tempu8 + 38;

	tempu8 = ((buf[2] & 0x3) << 4) | (buf[3] & 0xf0) >> 4;
	c_driver.matrix[5] = tempu8 - 0x20;

	tempu8 = ((buf[3] & 0xf) << 2) | ((buf[4] & 0xc0) >> 6);
	c_driver.matrix[6] = tempu8 - 0x20;

	tempu8 = buf[4] & 0x3f;
	c_driver.matrix[7] = tempu8 - 0x20;

	tempu8 = (buf[5] & 0xfe) >> 1;
	c_driver.matrix[8] = tempu8 + 66;

	yas529_cdrv_make_correction_matrix(transform, c_driver.matrix,
							c_driver.correction_m);

	for (i = 0; i < 3; ++i)
		c_driver.temp_coeff[i] = (int8_t)((int16_t)buf[i + 6] - 0x80);

	c_driver.initialized = 1;

	return YAS529_CDRV_NO_ERROR;
}

STATIC int
yas529_cdrv_term(void)
{
	if (!c_driver.initialized)
		return YAS529_CDRV_ERR_NOT_INITIALIZED;
	c_driver.initialized = 0;

	return YAS529_CDRV_NO_ERROR;
}

static int
lock(void)
{
	if (this_driver.callback.lock != NULL) {
		if (this_driver.callback.lock() < 0)
			return YAS529_ERROR_RESTARTSYS;
	}
	return 0;
}

static int
unlock(void)
{
	if (this_driver.callback.unlock != NULL) {
		if (this_driver.callback.unlock() < 0)
			return YAS529_ERROR_RESTARTSYS;
	}
	return 0;
}

static void
fir_filter_init(struct yas529_fir_filter *filter, int len)
{
	int i;

	filter->num = 0;
	filter->index = 0;
	filter->filter_len = len;

	for (i = 0; i < filter->filter_len; ++i)
		filter->sequence[i] = 0;
}

static int32_t
fir_filter_filter(struct yas529_fir_filter *filter, int32_t in)
{
	int32_t out = 0;
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

static void
filter_init(struct yas529_driver *d)
{
	int i;

	for (i = 0; i < 3; i++)
		fir_filter_init(&d->filter[i], d->filter_len);
}

static void
filter_filter(struct yas529_driver *d, int32_t *orig, int32_t *filtered)
{
	int i;

	for (i = 0; i < 3; i++)
		filtered[i] = fir_filter_filter(&d->filter[i], orig[i]);
}

static int32_t
calc_intensity(const int32_t *p)
{
	int32_t intensity = 0;
	int i;

	for (i = 0; i < 3; i++)
		intensity += ((p[i] / 1000) + 1) * ((p[i] / 1000) + 1);

	return intensity;
}

static int
is_valid_rough_offset(const uint8_t *p)
{
	return (p != NULL && (p[0] < 33) && (p[1] < 33) && (p[2] < 33));
}

static int
is_valid_fine_offset(const int32_t *p)
{
	return (p != NULL
		&& (-0x4000 <= p[0] && p[0] <= 0x4000)
		&& (-0x4000 <= p[1] && p[1] <= 0x4000)
		&& (-0x4000 <= p[2] && p[2] <= 0x4000));
}

static int
is_rough_offset_differ(const uint8_t *p0, const uint8_t *p1)
{
	return p0[0] != p1[0] || p0[1] != p1[1] || p0[2] != p1[2];
}

static int
is_fine_offset_differ(const int32_t *p0, const int32_t *p1)
{
	return p0[0] != p1[0] || p0[1] != p1[1] || p0[2] != p1[2];
}

static int
get_overflow(struct yas529_driver *d)
{
	return d->overflow;
}

static void
set_overflow(struct yas529_driver *d, const int overflow)
{
	if (d->overflow != overflow)
		d->overflow = overflow;
}

static int
get_initcoil_gaveup(struct yas529_driver *d)
{
	return d->initcoil_gaveup;
}

static void
set_initcoil_gaveup(struct yas529_driver *d, const int initcoil_gaveup)
{
	d->initcoil_gaveup = initcoil_gaveup;
}

static int32_t *
get_fine_offset(struct yas529_driver *d)
{
	return d->fine_offset;
}

static void
set_fine_offset(struct yas529_driver *d, const int32_t *offset)
{
	int i;

	if (is_fine_offset_differ(d->fine_offset, offset)) {
		for (i = 0; i < 3; i++)
			d->fine_offset[i] = offset[i];
	}
}

static uint8_t *
get_rough_offset(struct yas529_driver *d)
{
	return d->rough_offset;
}

static void
set_rough_offset(struct yas529_driver *d, const uint8_t *offset)
{
	int i;

	if (is_rough_offset_differ(d->rough_offset, offset)) {
		for (i = 0; i < 3; i++)
			d->rough_offset[i] = offset[i];
	}
}

static int
get_accuracy(struct yas529_driver *d)
{
	return d->accuracy;
}

static void
set_accuracy(struct yas529_driver *d, const int accuracy)
{
	d->accuracy = accuracy;
}

static int
get_active(struct yas529_driver *d)
{
	return d->active;
}

static void
set_active(struct yas529_driver *d, const int active)
{
	d->active = active;
}

static int
get_position(struct yas529_driver *d)
{
	return d->position;
}

static void
set_position(struct yas529_driver *d, const int position)
{
	d->position = position;
}

static int
get_measure_state(struct yas529_driver *d)
{
	return d->measure_state;
}

static void
set_measure_state(struct yas529_driver *d, const int state)
{
	d->measure_state = state;
}

static struct utimer *
get_data_timer(struct yas529_driver *d)
{
	return &d->data_timer;
}

static struct utimer *
get_initcoil_timer(struct yas529_driver *d)
{
	return &d->initcoil_timer;
}

static struct utimer *
get_initcoil_giveup_timer(struct yas529_driver *d)
{
	return &d->initcoil_giveup_timer;
}

static struct utimer *
get_detect_overflow_timer(struct yas529_driver *d)
{
	return &d->detect_overflow_timer;
}

static int
get_delay_timer_use_data(struct yas529_driver *d)
{
	return d->delay_timer_use_data;
}

static void
set_delay_timer_use_data(struct yas529_driver *d, int flag)
{
	d->delay_timer_use_data = !!flag;
}

static int
get_delay_timer_interval(struct yas529_driver *d)
{
	return d->delay_timer_interval;
}

static void
set_delay_timer_interval(struct yas529_driver *d, int interval)
{
	d->delay_timer_interval = interval;
}

static int
get_delay_timer_counter(struct yas529_driver *d)
{
	return d->delay_timer_counter;
}

static void
set_delay_timer_counter(struct yas529_driver *d, int counter)
{
	d->delay_timer_counter = counter;
}

static int
get_filter_enable(struct yas529_driver *d)
{
	return d->filter_enable;
}

static void
set_filter_enable(struct yas529_driver *d, int enable)
{
	if (!d->filter_enable && enable)
		filter_init(d);
	d->filter_enable = enable;
}

static int
get_filter_len(struct yas529_driver *d)
{
	return d->filter_len;
}

static void
set_filter_len(struct yas529_driver *d, int len)
{
	if (d->filter_len < 0)
		d->filter_len = 0;
	if (d->filter_len > YAS529_MAX_FILTER_LEN)
		d->filter_len = YAS529_MAX_FILTER_LEN;
	d->filter_len = len;
	filter_init(d);
}

static int32_t*
get_previous_magnetic(struct yas529_driver *d)
{
	return d->prev_magnetic;
}

static void
set_previous_magnetic(struct yas529_driver *d, int32_t *data)
{
	int i;
	for (i = 0; i < 3; i++)
		d->prev_magnetic[i] = data[i];
}

static int32_t*
get_previous_magnetic_raw(struct yas529_driver *d)
{
	return d->prev_magnetic_raw;
}

static void
set_previous_magnetic_raw(struct yas529_driver *d, int32_t *data)
{
	int i;
	for (i = 0; i < 3; i++)
		d->prev_magnetic_raw[i] = data[i];
}

static int
init_coil(struct yas529_driver *d)
{
	int rt;

	YLOGD("init_coil IN\n");

	utimer_update(get_initcoil_timer(d));
	if (!get_initcoil_gaveup(d)) {
		utimer_update(get_initcoil_giveup_timer(d));
		if (utimer_is_timeout(get_initcoil_giveup_timer(d))) {
			utimer_clear_timeout(get_initcoil_giveup_timer(d));
			set_initcoil_gaveup(d, TRUE);
		}
	}
	if (utimer_is_timeout(get_initcoil_timer(d)) &&
		!get_initcoil_gaveup(d)) {
		utimer_clear_timeout(get_initcoil_timer(d));
		YLOGI("init_coil!\n");
		rt = yas529_cdrv_actuate_initcoil();
		if (rt < 0) {
			pr_err("yas529_cdrv_actuate_initcoil failed[%d]\n", rt);
			return rt;
		}
		if (get_overflow(d) ||
			!is_valid_rough_offset(get_rough_offset(d)))
			set_measure_state(d, MEASURE_STATE_ROUGH_MEASURE);
		else
			set_measure_state(d, MEASURE_STATE_NORMAL);
	}

	YLOGD("init_coil OUT\n");

	return 0;
}

static int
measure_rough_offset(struct yas529_driver *d)
{
	uint8_t roffset[3];
	int32_t moffset[3];
	int rt, result = 0, i;

	YLOGI("measure_rough_offset IN\n");

	rt = yas529_cdrv_measure_rough_offset(roffset);
	if (rt < 0) {
		pr_err("yas529_cdrv_measure_rough_offset failed[%d]\n", rt);
		return rt;
	}

	YLOGI("rough offset[%d][%d][%d]\n", roffset[0], roffset[1], roffset[2]);

	if (rt & MEASURE_RESULT_OVER_UNDER_FLOW) {
		YLOGI("yas529_cdrv_measure_rough_offset overflow"
			"x[%d] y[%d] z[%d]\n",
			!!(rt&0x01), !!(rt&0x02), !!(rt&0x04));

		set_overflow(d, TRUE);
		set_measure_state(d, MEASURE_STATE_INIT_COIL);
		set_accuracy(d, 0);
		return YAS529_REPORT_OVERFLOW_OCCURED;
	}

	for (i = 0; i < 3; i++)
		moffset[i] = get_fine_offset(d)[i];
	if (is_rough_offset_differ(get_rough_offset(d), roffset)) {
		if (is_valid_rough_offset(get_rough_offset(d))
			&& is_valid_fine_offset(get_fine_offset(d))) {
				yas529_cdrv_recalc_fine_offset(
					get_fine_offset(d),
					moffset, get_rough_offset(d), roffset);
			result |= YAS529_REPORT_FINE_OFFSET_CHANGED;
		}
	}

	rt = yas529_cdrv_set_rough_offset(roffset);
	if (rt < 0) {
		pr_err("yas529_cdrv_set_rough_offset failed[%d]\n", rt);
		return rt;
	}

	result |= YAS529_REPORT_ROUGH_OFFSET_CHANGED;
	set_rough_offset(d, roffset);
	if (is_valid_fine_offset(moffset))
		set_fine_offset(d, moffset);
	set_measure_state(d, MEASURE_STATE_NORMAL);

	YLOGI("measure_rough_offset OUT\n");

	return result;
}

static int
measure_msensor_normal(struct yas529_driver *d, int32_t *raw, int32_t *magnetic)
{
	int rt = 0, result, i;
	int32_t filtered[3];

	YLOGD("measure_msensor_normal IN\n");

	result = 0;
	rt = yas529_cdrv_measure(raw, NULL, NULL);
	if (rt < 0) {
		pr_err("yas529_cdrv_measure failed[%d]\n", rt);
		return rt;
	}
	if (rt & MEASURE_RESULT_OVER_UNDER_FLOW) {
		YLOGI("yas529_cdrv_measure overflow x[%d] y[%d] z[%d]\n",
			!!(rt&0x01), !!(rt&0x02), !!(rt&0x04));
		utimer_update(get_detect_overflow_timer(d));
		set_overflow(d, TRUE);
		if (utimer_is_timeout(get_detect_overflow_timer(d))) {
			utimer_clear_timeout(get_detect_overflow_timer(d));
			result |= YAS529_REPORT_OVERFLOW_OCCURED;
			set_accuracy(d, 0);
		}
		if (get_measure_state(d) == MEASURE_STATE_NORMAL)
			set_measure_state(d, MEASURE_STATE_INIT_COIL);
	} else {
		utimer_clear(get_detect_overflow_timer(d));
		set_overflow(d, FALSE);
		if (get_measure_state(d) == MEASURE_STATE_NORMAL) {
			utimer_clear(get_initcoil_timer(d));
			utimer_clear(get_initcoil_giveup_timer(d));
		}
	}
	if (get_filter_enable(d))
		filter_filter(d, raw, filtered);

	if (is_valid_fine_offset(get_fine_offset(d))) {
		for (i = 0; i < 3; i++) {
			magnetic[i] = get_filter_enable(d)
				? filtered[i] - get_fine_offset(d)[i]
				: raw[i] - get_fine_offset(d)[i];
		}
	} else {
		for (i = 0; i < 3; i++)
			magnetic[i]
				= get_filter_enable(d) ? filtered[i] : raw[i];
	}
	for (i = 0; i < 3; i++)
		magnetic[i] *= 400; /* typically raw * 400 is nT in unit */

	YLOGD("measure_msensor_normal OUT\n");

	return result;
}

static int
measure_msensor(struct yas529_driver *d, int32_t *raw, int32_t *magnetic)
{
	int result, i;

	YLOGD("measure_msensor IN\n");

	for (i = 0; i < 3; i++) {
		raw[i] = get_previous_magnetic_raw(d)[i];
		magnetic[i] = get_previous_magnetic(d)[i];
	}

	result = 0;
	switch (get_measure_state(d)) {
	case MEASURE_STATE_INIT_COIL:
		result = init_coil(d);
		break;
	case MEASURE_STATE_ROUGH_MEASURE:
		result = measure_rough_offset(d);
		break;
	case MEASURE_STATE_NORMAL:
		result = 0;
		break;
	default:
		result = -1;
		break;
	}

	if (result < 0)
		return result;

	if (!(result & YAS529_REPORT_OVERFLOW_OCCURED))
		result |= measure_msensor_normal(d, raw, magnetic);
	set_previous_magnetic(d, magnetic);
	set_previous_magnetic_raw(d, raw);

	YLOGD("measure_msensor OUT\n");

	return result;
}

static int
measure(struct yas529_driver *d, int32_t *magnetic, int32_t *raw,
	int *accuracy, uint32_t *time_delay)
{
	int32_t intensity = 0;
	int result;
	int counter;
	uint32_t total = 0;

	YLOGD("measure IN\n");

	utimer_update(get_data_timer(d));

	result = measure_msensor(d, raw, magnetic);
	if (result < 0)
		return result;

	counter = get_delay_timer_counter(d);
	total = utimer_get_total_time(get_data_timer(d));
	if (utimer_get_delay(get_data_timer(d)) > 0)
		counter -= total / utimer_get_delay(get_data_timer(d));
	else
		counter = 0;

	if (utimer_is_timeout(get_data_timer(d))) {
		utimer_clear_timeout(get_data_timer(d));

		if (get_delay_timer_use_data(d)) {
			result |= YAS529_REPORT_DATA;
			if (counter <= 0)
				result |= YAS529_REPORT_CALIB;
		} else {
			result |= YAS529_REPORT_CALIB;
			if (counter <= 0)
				result |= YAS529_REPORT_DATA;
		}
	}

	if (counter <= 0)
		set_delay_timer_counter(d, get_delay_timer_interval(d));
	else
		set_delay_timer_counter(d, counter);

	intensity = calc_intensity(magnetic);
	*accuracy = get_accuracy(d);
	if (get_accuracy(d) > 1 && intensity >= INTENSITY_THRESHOLD_1)
		*accuracy = 1;
	else if (get_accuracy(d) > 2 && intensity >= INTENSITY_THRESHOLD_0)
		*accuracy = 2;

	*time_delay = utimer_sleep_time(get_data_timer(d));

	YLOGD("measure OUT [%d]\n", result);

	return result;
}

static int
deactivate(struct yas529_driver *d)
{
	YLOGI("deactivate IN\n");

	/* restart initialization coil give up timer */
	utimer_clear(get_initcoil_giveup_timer(d));
	set_initcoil_gaveup(d, FALSE);

	YLOGI("deactivate OUT\n");

	return 0;
}

static int
ioctl_get_driver_state(struct yas529_driver *d, int32_t *fine_offset,
			uint8_t *rough_offset, int *accuracy)
{
	int i;

	YLOGI("ioctl_get_driver_state IN\n");

	for (i = 0; i < 3; i++)
		rough_offset[i] = (int32_t)get_rough_offset(d)[i];
	for (i = 0; i < 3; i++)
		fine_offset[i] = get_fine_offset(d)[i];
	*accuracy = get_accuracy(d);
	YLOGI("\t r[%u][%u][%u] f[%d][%d][%d] a[%d]\n",
		rough_offset[0],
		rough_offset[1],
		rough_offset[2],
		fine_offset[0],
		fine_offset[1],
		fine_offset[2],
		*accuracy);

	YLOGI("ioctl_get_driver_state OUT\n");

	return 0;
}

static int
ioctl_set_driver_state(struct yas529_driver *d, int32_t *fine_offset,
			uint8_t *rough_offset, int accuracy)
{
	YLOGI("ioctl_set_driver_state IN\n");

	set_rough_offset(d, rough_offset);
	set_fine_offset(d, fine_offset);
	set_accuracy(d, accuracy);
	YLOGI("\t r[%d][%d][%d] f[%d][%d][%d] a[%d]\n",
		get_rough_offset(d)[0],
		get_rough_offset(d)[1],
		get_rough_offset(d)[2],
		get_fine_offset(d)[0],
		get_fine_offset(d)[1],
		get_fine_offset(d)[2],
		get_accuracy(d));

	if (is_valid_rough_offset(get_rough_offset(d)))
		yas529_cdrv_set_rough_offset(get_rough_offset(d));
	else
		set_measure_state(d, MEASURE_STATE_ROUGH_MEASURE);

	YLOGI("ioctl_set_driver_state OUT\n");

	return 0;
}

static int
yas529_init_nolock(struct yas529_driver *d)
{
	int rt;

	YLOGI("yas529_init_nolock IN\n");

	rt = yas529_cdrv_init(YAS529_TRANSFORMATION[get_position(d)]);
	if (rt < 0) {
		pr_err("yas529_cdrv_init failed[%d]\n", rt);
		return rt;
	}

	utimer_lib_init(this_driver.callback.current_time);
	utimer_init(get_data_timer(d), 50);
	utimer_init(get_initcoil_timer(d), YAS529_INITCOIL_INTERVAL);
	utimer_init(get_initcoil_giveup_timer(d),
			YAS529_INITCOIL_GIVEUP_INTERVAL);
	utimer_init(get_detect_overflow_timer(d),
			YAS529_DETECT_OVERFLOW_INTERVAL);
	set_initcoil_gaveup(d, FALSE);

	set_fine_offset(d, INVALID_FINE_OFFSET);
	set_rough_offset(d, INVALID_ROUGH_OFFSET);
	set_accuracy(d, 0);

	rt = yas529_cdrv_actuate_initcoil();
	if (rt < 0) {
		pr_err("yas529_cdrv_actuate_initcoil failed[%d]\n", rt);
		set_measure_state(d, MEASURE_STATE_INIT_COIL);
	} else
		set_measure_state(d, MEASURE_STATE_ROUGH_MEASURE);

	YLOGI("yas529_init_nolock OUT\n");

	return 0;
}

static void
yas529_term_nolock(struct yas529_driver *d)
{
	YLOGI("yas529_term_nolock\n");

	(void) d;
	yas529_cdrv_term();

	YLOGI("yas529_term_nolock out\n");
}

static int
yas529_set_enable_nolock(struct yas529_driver *d, int active)
{
	if (active) {
		if (!get_active(d)) {
			set_active(d, TRUE);
			return 0;
		}
	} else {
		if (get_active(d)) {
			if (deactivate(d) < 0)
				return -1;
			set_active(d, FALSE);
			return 0;
		}
	}
	return 0;
}

static int
yas529_get_enable_nolock(struct yas529_driver *d)
{
	return get_active(d);
}

static int
yas529_set_filter_enable_nolock(struct yas529_driver *d, int enable)
{
	set_filter_enable(d, enable);
	return 0;
}

static int
yas529_get_filter_enable_nolock(struct yas529_driver *d)
{
	return get_filter_enable(d);
}

static int
yas529_set_filter_len_nolock(struct yas529_driver *d, int len)
{
	set_filter_len(d, len);
	return 0;
}

static int
yas529_get_filter_len_nolock(struct yas529_driver *d)
{
	return get_filter_len(d);
}

static int
check_interval(int ms)
{
	int index;

	if (ms == 0)
		ms = supported_data_interval[0];
	for (index = 0; index < NELEMS(supported_data_interval); index++) {
		if (ms == supported_data_interval[index])
			return index;
	}
	return -1;
}

static int
yas529_set_delay_nolock(struct yas529_driver *d, int ms)
{
	int index;
	uint32_t delay_data, delay_calib;

	index = check_interval(ms);
	if (index < 0)
		return -1;
	delay_data = supported_data_interval[index];
	delay_calib = supported_calib_interval[index];
	set_delay_timer_use_data(d, delay_data < delay_calib);
	if (delay_data < delay_calib) {
		set_delay_timer_interval(d, delay_calib / delay_data);
		set_delay_timer_counter(d, delay_calib / delay_data);
		utimer_set_delay(get_data_timer(d),
					supported_data_interval[index]);
	} else {
		set_delay_timer_interval(d, delay_data / delay_calib);
		set_delay_timer_counter(d, delay_data / delay_calib);
		utimer_set_delay(get_data_timer(d),
					supported_calib_interval[index]);
	}

	return 0;
}

static int
yas529_get_delay_nolock(struct yas529_driver *d, int *ms)
{
	if (get_delay_timer_use_data(d))
		*ms = utimer_get_delay(get_data_timer(d));
	else
		*ms = utimer_get_delay(get_data_timer(d))
				* get_delay_timer_interval(d);
	return 0;
}

static int
yas529_set_position_nolock(struct yas529_driver *d, int position)
{
	set_position(d, position);
	yas529_cdrv_set_transformatiom_matrix(YAS529_TRANSFORMATION[position]);
	return 0;
}

static int
yas529_get_position_nolock(struct yas529_driver *d, int *position)
{
	*position = get_position(d);
	return 0;
}

static int
yas529_measure(int32_t *magnetic, int32_t *raw,
		int32_t *accuracy, uint32_t *time_delay_ms)
{
	uint32_t time_delay = MEASURE_ERROR_DELAY;
	int rt;

	YLOGD("yas529_measure IN\n");

	if (magnetic == NULL || raw == NULL || accuracy == NULL
		|| time_delay_ms == NULL)
		return YAS529_ERROR_ARG;

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	*time_delay_ms = MEASURE_ERROR_DELAY;
	rt = 0;

	if (get_active(&this_driver)) {
		rt = measure(&this_driver, magnetic, raw,
					accuracy, &time_delay);
		if (rt >= 0) {
			*time_delay_ms = time_delay;
			if (*time_delay_ms > 0)
				/* for the system that the time
				 *is in usec unit */
				*time_delay_ms += 1;
		}
	} else
		rt = YAS529_ERROR_NOT_ACTIVE;

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGD("yas529_measure OUT[%d]\n", rt);

	return rt;
}

static int
yas529_get_enable(void)
{
	int rt;

	YLOGI("yas529_get_enable\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_get_enable_nolock(&this_driver);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_get_enable OUT[%d]\n", rt);

	return rt;
}

static int
yas529_set_enable(int enable)
{
	int rt;

	YLOGI("yas529_set_enable IN\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_set_enable_nolock(&this_driver, enable);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_set_enable OUT\n");

	return rt;
}

static int
yas529_get_filter_enable(void)
{
	int rt;

	YLOGI("yas529_get_filter_enable\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_get_filter_enable_nolock(&this_driver);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_get_filter_enable OUT[%d]\n", rt);

	return rt;
}

static int
yas529_set_filter_enable(int enable)
{
	int rt;

	YLOGI("yas529_set_filter_enable IN\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_set_filter_enable_nolock(&this_driver, enable);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_set_filter_enable OUT\n");

	return rt;
}

static int
yas529_get_filter_len(void)
{
	int rt;

	YLOGI("yas529_get_filter_len\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_get_filter_len_nolock(&this_driver);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_get_filter_len[%d] OUT\n", rt);

	return rt;
}

static int
yas529_set_filter_len(int len)
{
	int rt;

	YLOGI("yas529_set_filter_len IN\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_set_filter_len_nolock(&this_driver, len);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_set_filter_len OUT\n");

	return rt;
}

static int
yas529_get_delay(void)
{
	int ms;

	YLOGI("yas529_get_delay\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	yas529_get_delay_nolock(&this_driver, &ms);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_get_delay[%d] OUT\n", ms);

	return ms;
}

static int
yas529_set_delay(int delay)
{
	int rt;

	YLOGI("yas529_set_delay\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_set_delay_nolock(&this_driver, delay);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_set_delay OUT\n");

	return rt;
}

static int
yas529_get_position(void)
{
	int position = 0;

	YLOGI("yas529_get_position\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	yas529_get_position_nolock(&this_driver, &position);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_get_position[%d] OUT\n", position);

	return position;
}

static int
yas529_set_position(int position)
{
	int rt;

	if (position < 0 || 7 < position)
		return YAS529_ERROR_ARG;

	YLOGI("yas529_set_position\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_set_position_nolock(&this_driver, position);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGI("yas529_set_position[%d] OUT\n", position);

	return rt;
}

static int
yas529_dev_ioctl_get_driver_state(unsigned long args)
{
	struct yas529_driver_state *p;
	int32_t fine_offset[3];
	uint8_t rough_offset[3];
	int accuracy;
	int i;

	YLOGI("yas529_dev_ioctl_get_driver_state [%p]\n", (void *)args);

	p = (struct yas529_driver_state *) args;
	ioctl_get_driver_state(&this_driver, fine_offset,
				rough_offset, &accuracy);

	for (i = 0; i < 3; i++) {
		p->fine_offset[i] = fine_offset[i];
		p->rough_offset[i] = rough_offset[i];
	}
	p->accuracy = accuracy;

	YLOGI("yas529_dev_ioctl_get_driver_state OUT\n");

	return 0;
}

static int
yas529_dev_ioctl_set_driver_state(unsigned long args)
{
	struct yas529_driver_state *p;
	int32_t fine_offset[3];
	uint8_t rough_offset[3];
	int accuracy;
	int i;

	YLOGI("yas529_dev_ioctl_set_driver_state [%p]\n", (void *)args);

	p = (struct yas529_driver_state *) args;
	for (i = 0; i < 3; i++) {
		fine_offset[i] = p->fine_offset[i];
		rough_offset[i] = p->rough_offset[i];
	}
	accuracy = p->accuracy;

	ioctl_set_driver_state(&this_driver, fine_offset,
				rough_offset, accuracy);

	YLOGI("yas529_dev_ioctl_set_driver_state OUT\n");

	return 0;
}

static int
yas529_dev_ioctl(unsigned int cmd, unsigned long args)
{
	int rt;

	YLOGD("yas529_dev_ioctl IN [%d][%p]\n", cmd, (void *)args);

	if ((void *)args == NULL)
		return YAS529_ERROR_ARG;

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	switch (cmd) {
	case YAS529_IOC_GET_DRIVER_STATE:
		rt = yas529_dev_ioctl_get_driver_state(args);
		break;
	case YAS529_IOC_SET_DRIVER_STATE:
		rt = yas529_dev_ioctl_set_driver_state(args);
		break;
	default:
		rt = -1;
		break;
	}

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	YLOGD("yas529_dev_ioctl OUT[%d]\n", rt);

	return rt;
}

static int
yas529_init(void)
{
	int rt;

	YLOGI("yas529_init\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	rt = yas529_init_nolock(&this_driver);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	return rt;
}

static int
yas529_term(void)
{
	YLOGI("yas529_term\n");

	if (lock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	yas529_term_nolock(&this_driver);

	if (unlock() < 0)
		return YAS529_ERROR_RESTARTSYS;

	return 0;
}

STATIC int
geomagnetic_driver_init(struct geomagnetic_hwdep_driver *hwdep_driver)
{
	if (hwdep_driver == NULL)
		return -1;

	YLOGI("geomagnetic_driver_init\n");

	hwdep_driver->init = yas529_init;
	hwdep_driver->term = yas529_term;
	hwdep_driver->get_enable = yas529_get_enable;
	hwdep_driver->set_enable = yas529_set_enable;
	hwdep_driver->get_filter_enable = yas529_get_filter_enable;
	hwdep_driver->set_filter_enable = yas529_set_filter_enable;
	hwdep_driver->get_filter_len = yas529_get_filter_len;
	hwdep_driver->set_filter_len = yas529_set_filter_len;
	hwdep_driver->get_delay = yas529_get_delay;
	hwdep_driver->set_delay = yas529_set_delay;
	hwdep_driver->get_position = yas529_get_position;
	hwdep_driver->set_position = yas529_set_position;
	hwdep_driver->measure = yas529_measure;
	hwdep_driver->ioctl = yas529_dev_ioctl;
	this_driver.callback.lock = hwdep_driver->callback.lock;
	this_driver.callback.unlock = hwdep_driver->callback.unlock;
	this_driver.callback.current_time
				= hwdep_driver->callback.current_time;

	return 0;
}


static int
geomagnetic_multi_lock(void)
{
	struct geomagnetic_data *data = NULL;
	int rt;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);

	rt = down_interruptible(&data->multi_lock);
	if (rt < 0)
		up(&data->multi_lock);
	return rt;
}

static int
geomagnetic_multi_unlock(void)
{
	struct geomagnetic_data *data = NULL;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);

	up(&data->multi_lock);
	return 0;
}

static int
geomagnetic_enable(struct geomagnetic_data *data)
{
	if (!atomic_cmpxchg(&data->enable, 0, 1))
		schedule_delayed_work(&data->work, 0);

	return 0;
}

static int
geomagnetic_disable(struct geomagnetic_data *data)
{
	if (atomic_cmpxchg(&data->enable, 1, 0))
		cancel_delayed_work_sync(&data->work);

	return 0;
}

/* Sysfs interface */
static ssize_t
geomagnetic_delay_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->delay));
}

static ssize_t
geomagnetic_delay_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	long value;
	int err;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;

	if (hwdep_driver.set_delay == NULL)
		return -ENOTTY;

	if (hwdep_driver.set_delay(value) == 0)
		atomic_set(&data->delay, value);

	return count;
}

static ssize_t
geomagnetic_enable_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t
geomagnetic_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	long value;
	int err;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;
	if (value != 0 && value != 1)
		return count;
	if (hwdep_driver.set_enable == NULL)
		return -ENOTTY;

	if (geomagnetic_multi_lock() < 0)
		return count;

	if (value) {
		hwdep_driver.set_enable(value);
		geomagnetic_enable(data);
	} else {
		geomagnetic_disable(data);
		hwdep_driver.set_enable(value);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_enable_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->filter_enable));
}

static ssize_t
geomagnetic_filter_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	long value;
	int err;

	if (hwdep_driver.set_filter_enable == NULL)
		return -ENOTTY;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;

	if (value != 0 && value != 1)
		return count;
	if (geomagnetic_multi_lock() < 0)
		return count;

	hwdep_driver.set_filter_enable(value);
	atomic_set(&data->filter_enable, value);

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_len_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->filter_len));
}

static ssize_t
geomagnetic_filter_len_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	long value;
	int err;

	if (hwdep_driver.set_filter_len == NULL)
		return -ENOTTY;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;

	if (geomagnetic_multi_lock() < 0)
		return count;

	hwdep_driver.set_filter_len(value);
	atomic_set(&data->filter_len, value);

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_position_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	if (hwdep_driver.get_position == NULL)
		 return -ENOTTY;
	return sprintf(buf, "%d\n", hwdep_driver.get_position());
}

static ssize_t
geomagnetic_position_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	long value;
	int err;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;

	if (hwdep_driver.set_position == NULL)
		return -ENOTTY;
	hwdep_driver.set_position(value);

	return count;
}

static ssize_t
geomagnetic_data_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int rt;

	rt = sprintf(buf, "%d %d %d\n",
		atomic_read(&data->last_data[0]),
		atomic_read(&data->last_data[1]),
		atomic_read(&data->last_data[2]));

	return rt;
}

static ssize_t
geomagnetic_status_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int rt;

	rt = sprintf(buf, "%d\n", atomic_read(&data->last_status));

	return rt;
}

static ssize_t
geomagnetic_wake_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	static int16_t cnt = 1;

	input_report_rel(data->input_data, REL_WAKE, cnt++);

	return count;
}

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_delay_show, geomagnetic_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_enable_show, geomagnetic_enable_store);
static DEVICE_ATTR(filter_enable, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_filter_enable_show, geomagnetic_filter_enable_store);
static DEVICE_ATTR(filter_len, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_filter_len_show, geomagnetic_filter_len_store);
static DEVICE_ATTR(data, S_IRUGO, geomagnetic_data_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, geomagnetic_status_show, NULL);
static DEVICE_ATTR(wake, S_IWUSR|S_IWGRP, NULL, geomagnetic_wake_store);
static DEVICE_ATTR(position, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_position_show, geomagnetic_position_store);

static struct attribute *geomagnetic_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_filter_enable.attr,
	&dev_attr_filter_len.attr,
	&dev_attr_data.attr,
	&dev_attr_status.attr,
	&dev_attr_wake.attr,
	&dev_attr_position.attr,
	NULL
};

static struct attribute_group geomagnetic_attribute_group = {
	.attrs = geomagnetic_attributes
};

static ssize_t
geomagnetic_threshold_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int threshold;

	geomagnetic_multi_lock();

	threshold = data->threshold;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", threshold);
}

static ssize_t
geomagnetic_threshold_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	long value;
	int err;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;

	geomagnetic_multi_lock();

	if (0 <= value && value <= 2) {
		data->threshold = value;
		input_report_rel(data->input_raw, REL_RAW_THRESHOLD, value);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_distortion_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int rt;

	geomagnetic_multi_lock();

	rt = sprintf(buf, "%d %d %d\n",
		data->distortion[0],
		data->distortion[1],
		data->distortion[2]);

	geomagnetic_multi_unlock();

	return rt;
}

static ssize_t
geomagnetic_distortion_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int32_t distortion[3];
	static int32_t val = 1;
	int i;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d",
		&distortion[0],
		&distortion[1],
		&distortion[2]);
	if (distortion[0] > 0 && distortion[1] > 0 && distortion[2] > 0) {
		for (i = 0; i < 3; i++)
			data->distortion[i] = distortion[i];
		input_report_rel(data->input_raw, REL_RAW_DISTORTION, val++);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_shape_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int shape;

	geomagnetic_multi_lock();

	shape = data->shape;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", shape);
}

static ssize_t
geomagnetic_shape_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	long value;
	int err;

	err = strict_strtoul(buf, 10, &value);
	if (err < 0)
		return count;

	geomagnetic_multi_lock();

	if (0 <= value && value <= 1) {
		data->shape = value;
		input_report_rel(data->input_raw, REL_RAW_SHAPE, value);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_offsets_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas529_driver_state state;

	geomagnetic_multi_lock();

	state = data->driver_state;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%u %u %u %d %d %d %d\n",
		state.rough_offset[0],
		state.rough_offset[1],
		state.rough_offset[2],
		state.fine_offset[0],
		state.fine_offset[1],
		state.fine_offset[2],
		state.accuracy);
}

static ssize_t
geomagnetic_offsets_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas529_driver_state state;
	uint32_t rough_offset[3];
	int i;

	geomagnetic_multi_lock();

	sscanf(buf, "%u %u %u %d %d %d %d",
		&rough_offset[0],
		&rough_offset[1],
		&rough_offset[2],
		&state.fine_offset[0],
		&state.fine_offset[1],
		&state.fine_offset[2],
		&state.accuracy);
	for (i = 0; i < 3; i++)
		state.rough_offset[i] = (uint8_t)rough_offset[i];
	hwdep_driver.ioctl(YAS529_IOC_SET_DRIVER_STATE,
		(unsigned long) &state);
	data->driver_state = state;

	geomagnetic_multi_unlock();

	return count;
}

static DEVICE_ATTR(threshold, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_threshold_show, geomagnetic_threshold_store);
static DEVICE_ATTR(distortion, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_distortion_show, geomagnetic_distortion_store);
static DEVICE_ATTR(shape, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_shape_show, geomagnetic_shape_store);
static DEVICE_ATTR(offsets, S_IRUGO|S_IWUSR|S_IWGRP,
	geomagnetic_offsets_show, geomagnetic_offsets_store);

static struct attribute *geomagnetic_raw_attributes[] = {
	&dev_attr_threshold.attr,
	&dev_attr_distortion.attr,
	&dev_attr_shape.attr,
	&dev_attr_offsets.attr,
	NULL
};

static struct attribute_group geomagnetic_raw_attribute_group = {
	.attrs = geomagnetic_raw_attributes
};

/* Interface Functions for Lower Layer */

static void
geomagnetic_input_work_func(struct work_struct *work)
{
	struct geomagnetic_data *data
		= container_of((struct delayed_work *)work,
				struct geomagnetic_data, work);
	int32_t magdata[3], raw[3];
	uint32_t time_delay_ms = 100;
	struct yas529_driver_state state;
	int accuracy, rt, i;

	if (hwdep_driver.measure == NULL || hwdep_driver.ioctl == NULL)
		return;

	rt = hwdep_driver.measure(magdata, raw, &accuracy, &time_delay_ms);
	if (rt < 0)
		pr_err("measure failed[%d]\n", rt);

	if (rt >= 0) {
		if (rt & YAS529_REPORT_DATA) {
			for (i = 0; i < 3; i++)
				atomic_set(&data->last_data[i], magdata[i]);
			atomic_set(&data->last_status, accuracy);

			/* report magnetic data in [nT] */
			input_report_rel(data->input_data, REL_X, magdata[0]);
			input_report_rel(data->input_data, REL_Y, magdata[1]);
			input_report_rel(data->input_data, REL_Z, magdata[2]);
			if (accuracy == 0)
				accuracy = -1;
			input_report_rel(data->input_data, REL_STATUS,
								accuracy);
			input_sync(data->input_data);
		}

		if ((rt & YAS529_REPORT_OVERFLOW_OCCURED)
			|| (rt & YAS529_REPORT_ROUGH_OFFSET_CHANGED)
			|| (rt & YAS529_REPORT_FINE_OFFSET_CHANGED)) {
			static uint16_t count = 1;
			int code = 0;
			int value = 0;

			hwdep_driver.ioctl(YAS529_IOC_GET_DRIVER_STATE,
				(unsigned long) &state);
			geomagnetic_multi_lock();
			data->driver_state = state;
			geomagnetic_multi_unlock();

			/* report event */
			code |= (rt & YAS529_REPORT_OVERFLOW_OCCURED);
			code |= (rt & YAS529_REPORT_ROUGH_OFFSET_CHANGED);
			code |= (rt & YAS529_REPORT_FINE_OFFSET_CHANGED);
			value = (count++ << 16) | (code);
			input_report_rel(data->input_raw,
						REL_RAW_REPORT, value);
		}

		if (rt & YAS529_REPORT_CALIB) {
			/* report raw magnetic data */
			input_report_rel(data->input_raw, REL_X, raw[0]);
			input_report_rel(data->input_raw, REL_Y, raw[1]);
			input_report_rel(data->input_raw, REL_Z, raw[2]);
			input_sync(data->input_raw);
		}
	} else
		time_delay_ms = 100;

	if (time_delay_ms > 0) {
		schedule_delayed_work(&data->work,
			msecs_to_jiffies(time_delay_ms) + 1);
	} else
		schedule_delayed_work(&data->work, 0);
}

static int
geomagnetic_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);
	if (atomic_read(&data->enable))
		cancel_delayed_work_sync(&data->work);
	return 0;
}

static int
geomagnetic_resume(struct i2c_client *client)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (atomic_read(&data->enable))
		schedule_delayed_work(&data->work, 0);

	return 0;
}

static int
geomagnetic_lock(void)
{
	struct geomagnetic_data *data = NULL;
	int rt;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);

	rt = down_interruptible(&data->driver_lock);
	if (rt < 0)
		up(&data->driver_lock);
	return rt;
}

static int
geomagnetic_unlock(void)
{
	struct geomagnetic_data *data = NULL;

	if (this_client == NULL)
		return -1;

	data = i2c_get_clientdata(this_client);

	up(&data->driver_lock);
	return 0;
}

static void
geomagnetic_current_time(int32_t *sec, int32_t *msec)
{
	struct timeval tv;

	do_gettimeofday(&tv);

	*sec = tv.tv_sec;
	*msec = tv.tv_usec / 1000;
}

static void yas529_reset(struct geomagnetic_data *data) {
	int counter = 0;
	uint8_t cmd[2] = { 0 };
	struct i2c_msg i2cmsg[2];

	i2cmsg[0].addr = this_client->addr;
	i2cmsg[0].flags = 0;
	i2cmsg[0].len = 1;
	i2cmsg[0].buf = cmd;
	i2cmsg[1].addr = this_client->addr;
	i2cmsg[1].flags = 1;
	i2cmsg[1].len = 2;
	i2cmsg[1].buf = cmd;

	cmd[0] = 0xd0;

	for (counter = 0; counter < 3; counter++) {

		gpio_set_value(data->pdata->reset_line, data->pdata->reset_asserted);
		msleep(2);
		gpio_set_value(data->pdata->reset_line, !data->pdata->reset_asserted);

		if (i2c_transfer(this_client->adapter, i2cmsg, 2) < 0) {
			dev_err(&this_client->dev, "[init] %d Read I2C ERROR!\n",
				counter);
	}
	dev_err(&this_client->dev, "[init] %d DeviceID is %x %x\n",
			counter, cmd[0], cmd[1]);
	if (cmd[1] == 0x40)
	break;
	msleep(10);
	cmd[0] = 0xd0;
	cmd[1] = 0;
	//yas529_init_sensor(data);
	}
}
static int
geomagnetic_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct geomagnetic_data *data = NULL;
	struct input_dev *input_data = NULL, *input_raw = NULL;
	int rt, sysfs_created = 0, sysfs_raw_created = 0;
	int data_registered = 0, raw_registered = 0, i;

	i2c_set_clientdata(client, NULL);
	data = kzalloc(sizeof(struct geomagnetic_data), GFP_KERNEL);
	if (data == NULL) {
		rt = -ENOMEM;
		goto err;
	}
	data->threshold = YAS529_DEFAULT_THRESHOLD;
	for (i = 0; i < 3; i++)
		data->distortion[i] = YAS529_DEFAULT_DISTORTION;
	data->shape = YAS529_DEFAULT_SHAPE;
	atomic_set(&data->enable, 0);
	for (i = 0; i < 3; i++)
		atomic_set(&data->last_data[i], 0);
	atomic_set(&data->last_status, 0);
	INIT_DELAYED_WORK(&data->work, geomagnetic_input_work_func);
	init_MUTEX(&data->driver_lock);
	init_MUTEX(&data->multi_lock);

	input_data = input_allocate_device();
	if (input_data == NULL) {
		rt = -ENOMEM;
		pr_err("%s: Failed to allocate input_data device\n", __func__);
		goto err;
	}

	input_data->name = GEOMAGNETIC_INPUT_NAME;
	//input_data->id.bustype = BUS_I2C;
	set_bit(EV_REL, input_data->evbit);
	input_set_capability(input_data, EV_REL, REL_X);
	input_set_capability(input_data, EV_REL, REL_Y);
	input_set_capability(input_data, EV_REL, REL_Z);
	input_set_capability(input_data, EV_REL, REL_STATUS);
	input_set_capability(input_data, EV_REL, REL_WAKE);
	//input_data->dev.parent = &client->dev;

	rt = input_register_device(input_data);
	if (rt) {
		pr_err("%s: Unable to register input_data device: %s\n",
			__func__, input_data->name);
		goto err;
	}
	data_registered = 1;

	rt = sysfs_create_group(&input_data->dev.kobj,
		&geomagnetic_attribute_group);
	if (rt) {
		pr_err("%s: sysfs_create_group failed[%s]\n",
			__func__, input_data->name);
		goto err;
	}
	sysfs_created = 1;

	input_raw = input_allocate_device();
	if (input_raw == NULL) {
		rt = -ENOMEM;
		pr_err("%s: Failed to allocate input_raw device\n", __func__);
		goto err;
	}



	input_raw->name = GEOMAGNETIC_INPUT_RAW_NAME;
	//input_raw->id.bustype = BUS_I2C;
	set_bit(EV_REL, input_raw->evbit);
	input_set_capability(input_raw, EV_REL, REL_X);
	input_set_capability(input_raw, EV_REL, REL_Y);
	input_set_capability(input_raw, EV_REL, REL_Z);
	input_set_capability(input_raw, EV_REL, REL_RAW_DISTORTION);
	input_set_capability(input_raw, EV_REL, REL_RAW_THRESHOLD);
	input_set_capability(input_raw, EV_REL, REL_RAW_SHAPE);
	input_set_capability(input_raw, EV_REL, REL_RAW_REPORT);
	//input_raw->dev.parent = &client->dev;

	rt = input_register_device(input_raw);
	if (rt) {
		pr_err("%s: Unable to register input_raw device: %s\n",
			__func__, input_raw->name);
		goto err;
	}
	raw_registered = 1;

	rt = sysfs_create_group(&input_raw->dev.kobj,
		&geomagnetic_raw_attribute_group);
	if (rt) {
		pr_err("geomagnetic_probe: sysfs_create_group failed[%s]\n",
			input_data->name);
		goto err;
	}
	sysfs_raw_created = 1;

	data->pdata = client->dev.platform_data;
	//mutex_init(&data->lock);

	this_client = client;
	data->input_raw = input_raw;
	data->input_data = input_data;
	input_set_drvdata(input_data, data);
	input_set_drvdata(input_raw, data);
	i2c_set_clientdata(client, data);

	gpio_request(data->pdata->reset_line, "YAS529 Reset Line");
	//if (err < 0)
	//	goto exit_reset_gpio_request_failed;
	gpio_direction_output(data->pdata->reset_line, !data->pdata->reset_asserted);

	yas529_reset(data);

	rt = geomagnetic_driver_init(&hwdep_driver);
	if (rt < 0) {
		pr_err("geomagnetic_driver_init failed[%d]\n", rt);
		goto err;
	}
	if (hwdep_driver.init != NULL) {
		rt = hwdep_driver.init();
		if (rt < 0) {
			pr_err("hwdep_driver.init() failed[%d]\n", rt);
			goto err;
		}
	}
	if (hwdep_driver.set_position != NULL) {
		if (hwdep_driver.set_position(CONFIG_INPUT_YAS529_POSITION)
			< 0) {
			pr_err("hwdep_driver.set_position() failed[%d]\n", rt);
			goto err;
		}
	}
	YLOGD("ioctl(YAS529_IOC_GET_DRIVER_STATE)\n");
	if (hwdep_driver.ioctl != NULL) {
		if (hwdep_driver.ioctl(YAS529_IOC_GET_DRIVER_STATE,
			(unsigned long)&data->driver_state) < 0) {
			pr_err("hwdep_driver get_driver_state failed\n");
			goto err;
		}
	}
	if (hwdep_driver.get_delay != NULL)
		atomic_set(&data->delay, hwdep_driver.get_delay());
	if (hwdep_driver.get_filter_enable != NULL)
		atomic_set(&data->filter_enable,
				hwdep_driver.get_filter_enable());
	if (hwdep_driver.get_filter_len != NULL)
		atomic_set(&data->filter_len, hwdep_driver.get_filter_len());

	/* enabling filter */
	hwdep_driver.set_filter_enable(1);
	atomic_set(&data->filter_enable, 1);

	YLOGD("return 0\n");
	return 0;

err:
	if (data != NULL) {
		if (input_raw != NULL) {
			if (sysfs_raw_created) {
				sysfs_remove_group(&input_raw->dev.kobj,
					&geomagnetic_raw_attribute_group);
			}
			if (raw_registered)
				input_unregister_device(input_raw);
			else
				input_free_device(input_raw);
		}
		if (input_data != NULL) {
			if (sysfs_created) {
				sysfs_remove_group(&input_data->dev.kobj,
					&geomagnetic_attribute_group);
			}
			if (data_registered)
				input_unregister_device(input_data);
			else
				input_free_device(input_data);
		}
		kfree(data);
	}

	return rt;
}

static int
geomagnetic_remove(struct i2c_client *client)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (data != NULL) {
		geomagnetic_disable(data);
		if (hwdep_driver.term != NULL)
			hwdep_driver.term();

		input_unregister_device(data->input_raw);
		sysfs_remove_group(&data->input_data->dev.kobj,
			&geomagnetic_attribute_group);
		sysfs_remove_group(&data->input_raw->dev.kobj,
			&geomagnetic_raw_attribute_group);
		input_unregister_device(data->input_data);
		kfree(data);
	}

	return 0;
}

/* I2C Device Driver */
static struct i2c_device_id geomagnetic_idtable[] = {
    {GEOMAGNETIC_I2C_DEVICE_NAME, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, geomagnetic_idtable);

static struct i2c_driver geomagnetic_i2c_driver = {
	.driver = {
		.name = GEOMAGNETIC_I2C_DEVICE_NAME,
		.owner = THIS_MODULE,
	},

	.id_table = geomagnetic_idtable,
	.probe = geomagnetic_probe,
	.remove = geomagnetic_remove,
	.suspend = geomagnetic_suspend,
	.resume = geomagnetic_resume,
};

static int __init
geomagnetic_init(void)
{
    return i2c_add_driver(&geomagnetic_i2c_driver);
}

static void __exit
geomagnetic_term(void)
{
    i2c_del_driver(&geomagnetic_i2c_driver);
}

module_init(geomagnetic_init);
module_exit(geomagnetic_term);

MODULE_AUTHOR("Yamaha Corporation");
MODULE_DESCRIPTION("YAS529 Geomagnetic Sensor Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.2.0");
