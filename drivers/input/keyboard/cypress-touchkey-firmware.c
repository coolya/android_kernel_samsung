/*
 * Copyright 2010, Cypress Semiconductor Corporation.
 * Copyright (C) 2010-2011, Samsung Electronics Co. Ltd. All Rights Reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA.
 *
 */

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/input/cypress-touchkey.h>
#include <linux/gpio.h>

#include "cypress-touchkey-firmware.h"

#define WRITE_BYTE_START	0x90
#define WRITE_BYTE_END		0xE0
#define NUM_BLOCK_END		0xE0

static void touchkey_init_gpio(struct touchkey_platform_data *pdata)
{
	gpio_direction_input(pdata->sda_pin);
	gpio_direction_input(pdata->scl_pin);
	gpio_set_value(pdata->en_pin, 1);
	mdelay(1);
}

static void touchkey_run_clk(struct touchkey_platform_data *pdata,
								int cycles)
{
	int i;

	for (i = 0; i < cycles; i++) {
		gpio_direction_output(pdata->scl_pin, 0);
		gpio_direction_output(pdata->scl_pin, 1);
	}
}

static u8 touchkey_get_data(struct touchkey_platform_data *pdata)
{
	gpio_direction_input(pdata->sda_pin);
	return !!gpio_get_value(pdata->sda_pin);
}

static u8 touchkey_read_bit(struct touchkey_platform_data *pdata)
{
	touchkey_run_clk(pdata, 1);
	return touchkey_get_data(pdata);
}

static u8 touchkey_read_byte(struct touchkey_platform_data *pdata)
{
	int i;
	u8 byte = 0;

	for (i = 7; i >= 0; i--)
		byte |= touchkey_read_bit(pdata) << i;

	return byte;
}

static void touchkey_send_bits(struct touchkey_platform_data *pdata,
							u8 data, int num_bits)
{
	int i;

	for (i = 0; i < num_bits; i++, data <<= 1) {
		gpio_direction_output(pdata->sda_pin, !!(data & 0x80));
		gpio_direction_output(pdata->scl_pin, 1);
		gpio_direction_output(pdata->scl_pin, 0);
	}
}

static void touchkey_send_vector(struct touchkey_platform_data *pdata,
					const struct issp_vector *vector_data)
{
	int i;
	u16 num_bits;

	for (i = 0, num_bits = vector_data->num_bits; num_bits > 7;
							num_bits -= 8, i++)
		touchkey_send_bits(pdata, vector_data->data[i], 8);

	if (num_bits)
		touchkey_send_bits(pdata, vector_data->data[i], num_bits);
	gpio_direction_output(pdata->sda_pin, 0);
	gpio_direction_input(pdata->sda_pin);
}

static bool touchkey_wait_transaction(struct touchkey_platform_data *pdata)
{
	int i;

	for (i = 0; i <= TRANSITION_TIMEOUT; i++) {
		gpio_direction_output(pdata->scl_pin, 0);
		if (touchkey_get_data(pdata))
			break;
		gpio_direction_output(pdata->scl_pin, 1);
	}

	if (i == TRANSITION_TIMEOUT)
		return true;

	for (i = 0; i <= TRANSITION_TIMEOUT; i++) {
		gpio_direction_output(pdata->scl_pin, 0);
		if (!touchkey_get_data(pdata))
			break;
		gpio_direction_output(pdata->scl_pin, 1);
	}

	return i == TRANSITION_TIMEOUT;
}

static int touchkey_issp_pwr_init(struct touchkey_platform_data *pdata)
{
	touchkey_init_gpio(pdata);
	if (touchkey_wait_transaction(pdata))
		return -1;

	gpio_direction_output(pdata->scl_pin, 0);

	touchkey_send_vector(pdata, &wait_and_poll_end);
	touchkey_send_vector(pdata, &id_setup_1);
	if (touchkey_wait_transaction(pdata))
		return -1;

	touchkey_send_vector(pdata, &wait_and_poll_end);
	return 0;
}

static int touchkey_read_status(struct touchkey_platform_data *pdata)
{
	unsigned char target_status;

	touchkey_send_vector(pdata, &tsync_enable);

	touchkey_send_vector(pdata, &read_id_v1);
	touchkey_run_clk(pdata, 2);
	target_status = touchkey_read_byte(pdata);
	touchkey_run_clk(pdata, 1);

	touchkey_send_bits(pdata, 0, 1);
	touchkey_send_vector(pdata, &tsync_disable);

	return target_status;
}

static int touchkey_erase_target(struct touchkey_platform_data *pdata)
{
	touchkey_send_vector(pdata, &erase);
	if (touchkey_wait_transaction(pdata))
		return -1;

	touchkey_send_vector(pdata, &wait_and_poll_end);
	return 0;
}

static u16 touchkey_load_target(struct touchkey_platform_data *pdata,
					const u8 *target_data_output, int len)
{
	u16 checksum_data = 0;
	u8 addr;
	int i;

	for (i = 0, addr = 0; i < len; i++, addr += 2) {
		checksum_data += target_data_output[i];
		touchkey_send_bits(pdata, WRITE_BYTE_START, 4);
		touchkey_send_bits(pdata, addr, 7);
		touchkey_send_bits(pdata, target_data_output[i], 8);
		touchkey_send_bits(pdata, WRITE_BYTE_END, 3);
	}

	return checksum_data;
}

static int touchkey_program_target_block(struct touchkey_platform_data *pdata,
								u8 block)
{
	touchkey_send_vector(pdata, &tsync_enable);
	touchkey_send_vector(pdata, &set_block_num);
	touchkey_send_bits(pdata, block, 8);
	touchkey_send_bits(pdata, NUM_BLOCK_END, 3);
	touchkey_send_vector(pdata, &tsync_disable);
	touchkey_send_vector(pdata, &program_and_verify);

	if (touchkey_wait_transaction(pdata))
		return -1;

	touchkey_send_vector(pdata, &wait_and_poll_end);
	return 0;
}

static int touchkey_target_bank_checksum(struct touchkey_platform_data *pdata,
								u16 *checksum)
{
	touchkey_send_vector(pdata, &checksum_setup);
	if (touchkey_wait_transaction(pdata))
		return -1;

	touchkey_send_vector(pdata, &wait_and_poll_end);
	touchkey_send_vector(pdata, &tsync_enable);
	touchkey_send_vector(pdata, &read_checksum_v1);
	touchkey_run_clk(pdata, 2);
	*checksum = touchkey_read_byte(pdata) << 8;

	touchkey_run_clk(pdata, 1);
	touchkey_send_vector(pdata, &read_checksum_v2);
	touchkey_run_clk(pdata, 2);
	*checksum |= touchkey_read_byte(pdata);
	touchkey_run_clk(pdata, 1);
	touchkey_send_bits(pdata, 0x80, 1);
	touchkey_send_vector(pdata, &tsync_disable);

	return 0;
}

static void touchkey_reset_target(struct touchkey_platform_data *pdata)
{
	gpio_direction_input(pdata->scl_pin);
	gpio_direction_input(pdata->sda_pin);
	gpio_set_value(pdata->en_pin, 0);
	mdelay(300);
	touchkey_init_gpio(pdata);
}

static int touchkey_secure_target_flash(struct touchkey_platform_data *pdata)
{
	u8 addr;
	int i;

	for (i = 0, addr = 0; i < SECURITY_BYTES_PER_BANK; i++, addr += 2) {
		touchkey_send_bits(pdata, WRITE_BYTE_START, 4);
		touchkey_send_bits(pdata, addr, 7);
		touchkey_send_bits(pdata, 0xFF, 8);
		touchkey_send_bits(pdata, WRITE_BYTE_END, 3);
	}

	touchkey_send_vector(pdata, &secure);
	if (touchkey_wait_transaction(pdata))
		return -1;

	touchkey_send_vector(pdata, &wait_and_poll_end);
	return 0;
}

int touchkey_flash_firmware(struct touchkey_platform_data *pdata,
							const u8 *fw_data)
{
	u16 chksumtgt = 0;
	u16 chksumdat = 0;
	int i;

	gpio_direction_output(pdata->en_pin, 0);

	if (touchkey_issp_pwr_init(pdata)) {
		pr_err("%s: error powering up\n", __func__);
		goto error_trap;
	}

	if (touchkey_erase_target(pdata)) {
		pr_err("%s: error erasing flash\n", __func__);
		goto error_trap;
	}

	for (i = 0; i < BLOCKS_PER_BANK; i++) {
		touchkey_send_vector(pdata, &tsync_enable);
		touchkey_send_vector(pdata, &read_write_setup);
		chksumdat += touchkey_load_target(pdata,
					fw_data + i * BLOCK_SIZE, BLOCK_SIZE);

		if (touchkey_program_target_block(pdata, i)) {
			pr_err("%s: error programming flash\n", __func__);
			goto error_trap;
		}

		if (touchkey_read_status(pdata)) {
			pr_err("%s: error reading status\n", __func__);
			goto error_trap;
		}
	}

	/* security start */
	touchkey_send_vector(pdata, &tsync_enable);
	touchkey_send_vector(pdata, &read_write_setup);
	if (touchkey_secure_target_flash(pdata)) {
		pr_err("%s: error securing flash\n", __func__);
		goto error_trap;
	}

	if (touchkey_target_bank_checksum(pdata, &chksumtgt)) {
		pr_err("%s: error reading checksum\n", __func__);
		goto error_trap;
	}

	if (chksumtgt != chksumdat) {
		pr_err("%s: error powering up touchkey controller\n", __func__);
		goto error_trap;
	}

	touchkey_reset_target(pdata);
	return 0;

error_trap:
	gpio_direction_input(pdata->scl_pin);
	gpio_direction_input(pdata->sda_pin);
	gpio_set_value(pdata->en_pin, 0);
	mdelay(20);
	return -1;
}
