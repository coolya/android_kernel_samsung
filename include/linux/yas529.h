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

#ifndef YAS529_H_
#define YAS529_H_
#define YAS_I2C_DEVICE_NAME     "yas529"
#define GEOMAGNETIC_DEVICE_NAME         "yas529"
#define GEOMAGNETIC_INPUT_NAME          "geomagnetic"
#define GEOMAGNETIC_INPUT_RAW_NAME      "geomagnetic_raw"

struct yas529_platform_data {
	int reset_line;
	int reset_asserted;
};

#endif /* YAS529_H_ */
