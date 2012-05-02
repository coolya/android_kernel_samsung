/*
 * max8893.h  --  Voltage regulation for the Maxim 8893
 *
 * based on max8660.h
 *
 * Copyright (C) 2009 Wolfram Sang, Pengutronix e.K.
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_REGULATOR_MAX8893_H
#define __LINUX_REGULATOR_MAX8893_H

#include <linux/regulator/machine.h>

enum {
	MAX8893_BUCK,
	MAX8893_LDO1,
	MAX8893_LDO2,
	MAX8893_LDO3,
	MAX8893_LDO4,
	MAX8893_LDO5,
	MAX8893_END,
};

/**
 * max8893_subdev_data - regulator subdev data
 * @id: regulator id
 * @initdata: regulator init data
 */
struct max8893_subdev_data {
	int				id;
	struct regulator_init_data	*initdata;
};

/**
 * max8893_platform_data - platform data for max8893
 * @num_subdevs: number of regulators used
 * @subdevs: pointer to regulators used
 */
struct max8893_platform_data {
	int num_subdevs;
	struct max8893_subdev_data *subdevs;
};
#endif
