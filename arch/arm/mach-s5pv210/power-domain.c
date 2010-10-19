/* linux/arch/arm/mach-s5pv210/power-domain.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Power domain support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <mach/power-domain.h>

#include <mach/regs-clock.h>
#include <plat/devs.h>

struct s5pv210_pd_data {
	struct regulator_desc desc;
	struct regulator_dev *dev;
	int microvolts;
	unsigned startup_delay;
	struct clk_should_be_running *clk_run;
	int ctrlbit;
};

struct clk_should_be_running {
	const char *clk_name;
	struct device *dev;
};

static struct regulator_consumer_supply s5pv210_pd_audio_supply[] = {
	REGULATOR_SUPPLY("pd", "s5pc1xx-iis.0"),
};

static struct regulator_consumer_supply s5pv210_pd_cam_supply[] = {
	REGULATOR_SUPPLY("pd", "s3c-fimc.0"),
	REGULATOR_SUPPLY("pd", "s3c-fimc.1"),
	REGULATOR_SUPPLY("pd", "s3c-fimc.2"),
	REGULATOR_SUPPLY("pd", "s3c-jpg"),
	REGULATOR_SUPPLY("pd", "s3c-csis"),
	REGULATOR_SUPPLY("pd", "s5p-rotator"),
};

static struct regulator_consumer_supply s5pv210_pd_tv_supply[] = {
	REGULATOR_SUPPLY("pd", "s5p-tvout"),
};

static struct regulator_consumer_supply s5pv210_pd_lcd_supply[] = {
	REGULATOR_SUPPLY("pd", "s3cfb"),
};

static struct regulator_consumer_supply s5pv210_pd_g3d_supply[] = {
	REGULATOR_SUPPLY("pd", "pvrsrvkm"),
};

static struct regulator_consumer_supply s5pv210_pd_mfc_supply[] = {
	REGULATOR_SUPPLY("pd", "s3c-mfc"),
};

static struct regulator_init_data s5pv210_pd_audio_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_audio_supply),
	.consumer_supplies	= s5pv210_pd_audio_supply,
};

static struct regulator_init_data s5pv210_pd_cam_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_cam_supply),
	.consumer_supplies	= s5pv210_pd_cam_supply,
};

static struct regulator_init_data s5pv210_pd_tv_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_tv_supply),
	.consumer_supplies	= s5pv210_pd_tv_supply,
};

static struct regulator_init_data s5pv210_pd_lcd_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_lcd_supply),
	.consumer_supplies	= s5pv210_pd_lcd_supply,
};

static struct regulator_init_data s5pv210_pd_g3d_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_g3d_supply),
	.consumer_supplies	= s5pv210_pd_g3d_supply,
};

static struct regulator_init_data s5pv210_pd_mfc_data = {
	.constraints = {
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(s5pv210_pd_mfc_supply),
	.consumer_supplies	= s5pv210_pd_mfc_supply,
};

struct clk_should_be_running s5pv210_pd_audio_clk[] = {
	{
		.clk_name	= "i2scdclk",
		.dev		= &s5pv210_device_iis0.dev,
	}, {
		/* end of the clock array */
	},
};

struct clk_should_be_running s5pv210_pd_cam_clk[] = {
	{
		.clk_name	= "sclk_fimc_lclk",
		.dev		= &s3c_device_fimc0.dev,
	}, {
		.clk_name	= "sclk_fimc_lclk",
		.dev		= &s3c_device_fimc1.dev,
	}, {
		.clk_name	= "sclk_fimc_lclk",
		.dev		= &s3c_device_fimc2.dev,
	}, {
		.clk_name	= "sclk_csis",
		.dev		= &s3c_device_csis.dev,
	}, {
		.clk_name	= "jpeg",
		.dev		= &s3c_device_jpeg.dev,
	}, {
		.clk_name	= "rotator",
		.dev		= &s5p_device_rotator.dev,
	}, {
		/* end of the clock array */
	},
};

struct clk_should_be_running s5pv210_pd_tv_clk[] = {
	{
		.clk_name	= "vp",
		.dev		= &s5p_device_tvout.dev,
	}, {
		.clk_name	= "mixer",
		.dev		= &s5p_device_tvout.dev,
	}, {
		.clk_name	= "tvenc",
		.dev		= &s5p_device_tvout.dev,
	}, {
		.clk_name	= "hdmi",
		.dev		= &s5p_device_tvout.dev,
	}, {
		/* end of the clock array */
	},
};

struct clk_should_be_running s5pv210_pd_lcd_clk[] = {
	{
		.clk_name	= "lcd",
		.dev		= &s3c_device_fb.dev,
	}, {
		.clk_name	= "dsim",
		.dev		= &s3c_device_fb.dev,
	}, {
		.clk_name	= "sclk_fimg2d",
		.dev		= &s3c_device_fb.dev,
	}, {
		/* end of the clock array */
	},
};

struct clk_should_be_running s5pv210_pd_g3d_clk[] = {
	{
		.clk_name	= "sclk",
		.dev		= &s3c_device_g3d.dev,
	}, {
		/* end of the clock array */
	},
};

struct clk_should_be_running s5pv210_pd_mfc_clk[] = {
	{
		.clk_name	= "mfc",
		.dev		= &s3c_device_mfc.dev,
	}, {
		/* end of the clock array */
	},
};

static struct s5pv210_pd_config s5pv210_pd_audio_pdata = {
	.supply_name = "pd_audio_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_audio_data,
	.clk_run = s5pv210_pd_audio_clk,
	.ctrlbit = S5PV210_PD_AUDIO,
};

static struct s5pv210_pd_config s5pv210_pd_cam_pdata = {
	.supply_name = "pd_cam_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_cam_data,
	.clk_run = s5pv210_pd_cam_clk,
	.ctrlbit = S5PV210_PD_CAM,
};

static struct s5pv210_pd_config s5pv210_pd_tv_pdata = {
	.supply_name = "pd_tv_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_tv_data,
	.clk_run = s5pv210_pd_tv_clk,
	.ctrlbit = S5PV210_PD_TV,
};

static struct s5pv210_pd_config s5pv210_pd_lcd_pdata = {
	.supply_name = "pd_lcd_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_lcd_data,
	.clk_run = s5pv210_pd_lcd_clk,
	.ctrlbit = S5PV210_PD_LCD,
};

static struct s5pv210_pd_config s5pv210_pd_g3d_pdata = {
	.supply_name = "pd_g3d_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_g3d_data,
	.clk_run = s5pv210_pd_g3d_clk,
	.ctrlbit = S5PV210_PD_G3D,
};

static struct s5pv210_pd_config s5pv210_pd_mfc_pdata = {
	.supply_name = "pd_mfc_supply",
	.microvolts = 5000000,
	.init_data = &s5pv210_pd_mfc_data,
	.clk_run = s5pv210_pd_mfc_clk,
	.ctrlbit = S5PV210_PD_MFC,
};

struct platform_device s5pv210_pd_audio = {
	.name          = "reg-s5pv210-pd",
	.id            = 0,
	.dev = {
		.platform_data = &s5pv210_pd_audio_pdata,
	},
};

struct platform_device s5pv210_pd_cam = {
	.name          = "reg-s5pv210-pd",
	.id            = 1,
	.dev = {
		.platform_data = &s5pv210_pd_cam_pdata,
	},
};

struct platform_device s5pv210_pd_tv = {
	.name          = "reg-s5pv210-pd",
	.id            = 2,
	.dev = {
		.platform_data = &s5pv210_pd_tv_pdata,
	},
};

struct platform_device s5pv210_pd_lcd = {
	.name          = "reg-s5pv210-pd",
	.id            = 3,
	.dev = {
		.platform_data = &s5pv210_pd_lcd_pdata,
	},
};

struct platform_device s5pv210_pd_g3d = {
	.name          = "reg-s5pv210-pd",
	.id            = 4,
	.dev = {
		.platform_data = &s5pv210_pd_g3d_pdata,
	},
};

struct platform_device s5pv210_pd_mfc = {
	.name          = "reg-s5pv210-pd",
	.id            = 5,
	.dev = {
		.platform_data = &s5pv210_pd_mfc_pdata,
	},
};

static int s5pv210_pd_pwr_done(int ctrl)
{
	unsigned int cnt;
	cnt = 1000;

	do {
		if (__raw_readl(S5P_BLK_PWR_STAT) & ctrl)
			return 0;
		udelay(1);
	} while (cnt-- > 0);

	return -ETIME;
}

static int s5pv210_pd_pwr_off(int ctrl)
{
	unsigned int cnt;
	cnt = 1000;

	do {
		if (!(__raw_readl(S5P_BLK_PWR_STAT) & ctrl))
			return 0;
		udelay(1);
	} while (cnt-- > 0);

	return -ETIME;
}

static int s5pv210_pd_ctrl(int ctrlbit, int enable)
{
	u32 pd_reg = __raw_readl(S5P_NORMAL_CFG);

	if (enable) {
		__raw_writel((pd_reg | ctrlbit), S5P_NORMAL_CFG);
		if (s5pv210_pd_pwr_done(ctrlbit))
			return -ETIME;
	} else {
		__raw_writel((pd_reg & ~(ctrlbit)), S5P_NORMAL_CFG);
		if (s5pv210_pd_pwr_off(ctrlbit))
			return -ETIME;
	}
	return 0;
}

static int s5pv210_pd_clk_enable(struct clk_should_be_running *clk_run)
{
	struct clk *clkp;
	int i;

	for (i = 0;; i++) {
		if (clk_run[i].clk_name == NULL)
			break;

		clkp = clk_get(clk_run[i].dev, clk_run[i].clk_name);

		if (IS_ERR(clkp)) {
			printk(KERN_ERR "unable to get clock %s\n",
					clk_run[i].clk_name);
		} else {
			clk_enable(clkp);
			clk_put(clkp);
		}
	}

	return 0;
}

static int s5pv210_pd_clk_disable(struct clk_should_be_running *clk_run)
{
	struct clk *clkp;
	int i;

	for (i = 0;; i++) {
		if (clk_run[i].clk_name == NULL)
			break;

		clkp = clk_get(clk_run[i].dev, clk_run[i].clk_name);

		if (IS_ERR(clkp)) {
			printk(KERN_ERR "unable to get clock %s\n",
					clk_run[i].clk_name);
		} else {
			clk_disable(clkp);
			clk_put(clkp);
		}
	}

	return 0;
}

static int s5pv210_pd_is_enabled(struct regulator_dev *dev)
{
	struct s5pv210_pd_data *data = rdev_get_drvdata(dev);

	return (__raw_readl(S5P_BLK_PWR_STAT) & data->ctrlbit) ? 1 : 0;
}

static int s5pv210_pd_enable(struct regulator_dev *dev)
{
	struct s5pv210_pd_data *data = rdev_get_drvdata(dev);
	int ret;

	if (data->clk_run)
		s5pv210_pd_clk_enable(data->clk_run);

	ret = s5pv210_pd_ctrl(data->ctrlbit, 1);
	if (ret < 0) {
		printk(KERN_ERR "failed to enable power domain\n");
		return ret;
	}

	if (data->clk_run)
		s5pv210_pd_clk_disable(data->clk_run);

	return 0;
}

static int s5pv210_pd_disable(struct regulator_dev *dev)
{
	struct s5pv210_pd_data *data = rdev_get_drvdata(dev);
	int ret;

	ret = s5pv210_pd_ctrl(data->ctrlbit, 0);
	if (ret < 0) {
		printk(KERN_ERR "faild to disable power domain\n");
		return ret;
	}

	return 0;
}

static int s5pv210_pd_enable_time(struct regulator_dev *dev)
{
	struct s5pv210_pd_data *data = rdev_get_drvdata(dev);

	return data->startup_delay;
}

static int s5pv210_pd_get_voltage(struct regulator_dev *dev)
{
	struct s5pv210_pd_data *data = rdev_get_drvdata(dev);

	return data->microvolts;
}

static int s5pv210_pd_list_voltage(struct regulator_dev *dev,
				      unsigned selector)
{
	struct s5pv210_pd_data *data = rdev_get_drvdata(dev);

	if (selector != 0)
		return -EINVAL;

	return data->microvolts;
}

static struct regulator_ops s5pv210_pd_ops = {
	.is_enabled = s5pv210_pd_is_enabled,
	.enable = s5pv210_pd_enable,
	.disable = s5pv210_pd_disable,
	.enable_time = s5pv210_pd_enable_time,
	.get_voltage = s5pv210_pd_get_voltage,
	.list_voltage = s5pv210_pd_list_voltage,
};

static int __devinit reg_s5pv210_pd_probe(struct platform_device *pdev)
{
	struct s5pv210_pd_config *config = pdev->dev.platform_data;
	struct s5pv210_pd_data *drvdata;
	int ret;

	drvdata = kzalloc(sizeof(struct s5pv210_pd_data), GFP_KERNEL);
	if (drvdata == NULL) {
		dev_err(&pdev->dev, "Failed to allocate device data\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.name = kstrdup(config->supply_name, GFP_KERNEL);
	if (drvdata->desc.name == NULL) {
		dev_err(&pdev->dev, "Failed to allocate supply name\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.type = REGULATOR_VOLTAGE;
	drvdata->desc.owner = THIS_MODULE;
	drvdata->desc.ops = &s5pv210_pd_ops;
	drvdata->desc.n_voltages = 1;

	drvdata->microvolts = config->microvolts;
	drvdata->startup_delay = config->startup_delay;

	drvdata->clk_run = config->clk_run;
	drvdata->ctrlbit = config->ctrlbit;

	drvdata->dev = regulator_register(&drvdata->desc, &pdev->dev,
					  config->init_data, drvdata);
	if (IS_ERR(drvdata->dev)) {
		ret = PTR_ERR(drvdata->dev);
		dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
		goto err_name;
	}

	platform_set_drvdata(pdev, drvdata);

	dev_dbg(&pdev->dev, "%s supplying %duV\n", drvdata->desc.name,
		drvdata->microvolts);

	return 0;

err_name:
	kfree(drvdata->desc.name);
err:
	kfree(drvdata);
	return ret;
}

static int __devexit reg_s5pv210_pd_remove(struct platform_device *pdev)
{
	struct s5pv210_pd_data *drvdata = platform_get_drvdata(pdev);

	regulator_unregister(drvdata->dev);
	kfree(drvdata->desc.name);
	kfree(drvdata);

	return 0;
}

static struct platform_driver regulator_s5pv210_pd_driver = {
	.probe		= reg_s5pv210_pd_probe,
	.remove		= __devexit_p(reg_s5pv210_pd_remove),
	.driver		= {
		.name		= "reg-s5pv210-pd",
		.owner		= THIS_MODULE,
	},
};

static int __init regulator_s5pv210_pd_init(void)
{
	return platform_driver_register(&regulator_s5pv210_pd_driver);
}
subsys_initcall(regulator_s5pv210_pd_init);

static void __exit regulator_s5pv210_pd_exit(void)
{
	platform_driver_unregister(&regulator_s5pv210_pd_driver);
}
module_exit(regulator_s5pv210_pd_exit);

MODULE_AUTHOR("HuiSung Kang <hs1218.kang@samsung.com>");
MODULE_DESCRIPTION("S5PV210 power domain regulator");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:reg-s5pv210-pd");
