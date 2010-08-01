/*
 *  linux/arch/arm/plat-s5pc11x/max8998_consumer.c
 *
 *  CPU frequency scaling for S5PC110
 *
 *  Copyright (C) 2009 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#ifdef CONFIG_MACH_S5PC110_CRESPO
#include <mach/gpio-crespo.h>
#else
#include <mach/gpio-p1.h>
#endif
#include <plat/gpio-cfg.h>
#include <asm/gpio.h>
#include <mach/cpu-freq-v210.h>
#include <linux/platform_device.h>
#include <linux/regulator/max8998.h>
#include <linux/regulator/consumer.h>

//#include <mach/max8998_function.h>


#define DBG(fmt...)
//#define DBG printk

#define PMIC_ARM		0
#define PMIC_INT		1
#define PMIC_BOTH	2

#ifndef CONFIG_CPU_FREQ
//unsigned int S5PC11X_FREQ_TAB = 1;
#endif

#define RAMP_RATE 10 // 10mv/usec
unsigned int step_curr;

enum PMIC_VOLTAGE {
	VOUT_0_75, 
	VOUT_0_80, 
	VOUT_0_85, 
	VOUT_0_90, 
	VOUT_0_95, 
	VOUT_1_00, 
	VOUT_1_05, 
	VOUT_1_10, 
	VOUT_1_15, 
	VOUT_1_20, 
	VOUT_1_25, 
	VOUT_1_30, 
	VOUT_1_35, 
	VOUT_1_40, 
	VOUT_1_45, 
	VOUT_1_50 	
};


/* frequency voltage matching table */
static const unsigned int frequency_match_1GHZ[][4] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
#if 1
        {1000000, 1250, 1100, 0},
        {800000, 1200, 1100, 1},
        {400000, 1050, 1100, 2},
        {200000, 950, 1100, 4},
        {100000, 950, 1000, 5},
#else //just for dvs test
        {1000000, 1250, 1100, 0},
        {800000, 1250, 1100, 1},
        {400000, 1250, 1100, 2},
        {200000, 1250, 1100, 4},
        {100000, 950, 1000, 5},
#endif
};

static const unsigned int frequency_match_1DOT2GHZ[][4] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
        {1200000, 1300, 1150, 0},
        {1000000, 1250, 1100, 1},
        {800000, 1200, 1100, 2},
        {400000, 1050, 1100, 3},
        {200000, 950, 1100, 4},
        {100000, 950, 1000, 5},
};
const unsigned int (*frequency_match[2])[4] = {
        frequency_match_1GHZ,
        frequency_match_1DOT2GHZ,
};

/*  voltage table */
static const unsigned int voltage_table[16] = {
	750, 800, 850, 900, 950, 1000, 1050,
	1100, 1150, 1200, 1250, 1300, 1350,
	1400, 1450, 1500
};

extern unsigned int S5PC11X_FREQ_TAB;
//extern const unsigned int (*frequency_match[2])[4];

static struct regulator *Reg_Arm = NULL, *Reg_Int = NULL;

static unsigned int s_arm_voltage=0, s_int_voltage=0;
static unsigned long set1_gpio;
static unsigned long set2_gpio;
static unsigned long set3_gpio;

/*only 4 Arm voltages and 2 internal voltages possible*/
static const unsigned int dvs_volt_table_1DOT2GHZ[][3] = {
        {L0, DVSARM4, DVSINT2},// use 100mhz gpio con value        
        {L1, DVSARM1, DVSINT1},
        {L2, DVSARM2, DVSINT1},
        {L3, DVSARM3, DVSINT1},
        {L4, DVSARM4, DVSINT1},
        {L5, DVSARM4, DVSINT2},

};

static const unsigned int dvs_volt_table_1GHZ[][3] = {
        {L0, DVSARM1, DVSINT1},
        {L1, DVSARM2, DVSINT1},
        {L2, DVSARM3, DVSINT1},
        {L3, DVSARM4, DVSINT1},
        {L4, DVSARM4, DVSINT2},
};


const unsigned int (*dvs_volt_table[2])[3] = {
        dvs_volt_table_1GHZ,
        dvs_volt_table_1DOT2GHZ,
};

static unsigned int dvs_arm_voltage_set_1DOT2GHZ[][2] = {
	{DVSARM1, 1250},
	{DVSARM2, 1200},
	{DVSARM3, 1050},
	{DVSARM4, 1300}, //{DVSARM4, 950},
	{DVSINT1, 1100},
	{DVSINT2, 1150}, //{DVSINT2, 1000},
};

static unsigned int dvs_arm_voltage_set_1GHZ[][2] = {
	{DVSARM1, 1250},
	{DVSARM2, 1200},
	{DVSARM3, 1050},
	{DVSARM4, 950},
	{DVSINT1, 1100},
	{DVSINT2, 1000},
};


unsigned int (*dvs_arm_voltage_set_table[2])[2] = {
        dvs_arm_voltage_set_1GHZ,
        dvs_arm_voltage_set_1DOT2GHZ,
};

int saved_1dot2G_dvs_stat=1;
void dvs_set_for_1dot2Ghz (int onoff) {
	unsigned int (*dvs_arm_voltage_set)[2] = dvs_arm_voltage_set_table[S5PC11X_FREQ_TAB];
	//printk("KSOO dvs_set_for_1dot2Ghz %d\n", onoff);
	if (onoff == saved_1dot2G_dvs_stat) return;
	if (onoff) {
		max8998_set_dvsarm_direct(DVSARM4, 1300);
		max8998_set_dvsint_direct(DVSINT2, 1150);
		/*Change the table also*/
		dvs_arm_voltage_set[DVSARM4 - DVSARM1][1] = 1300;
		dvs_arm_voltage_set[DVSINT2 - DVSARM1][1] = 1150;
	} else {
		max8998_set_dvsarm_direct(DVSARM4, 950);
		max8998_set_dvsint_direct(DVSINT2, 1000);	
		/*Change the table also*/
		dvs_arm_voltage_set[DVSARM4 - DVSARM1][1] = 950;
		dvs_arm_voltage_set[DVSINT2 - DVSARM1][1] = 1000;
	}
	saved_1dot2G_dvs_stat = onoff;
	// need a udelay like 
	// int set_voltage_dvs(enum perf_level p_lv)
	// but, we may skip udelay, because this level will be used after some process execution.
	return;
}

static int set_max8998(unsigned int pwr, enum perf_level p_lv)
{
	int voltage;
	int pmic_val;
	int ret = 0;
	const unsigned int (*frequency_match_tab)[4] = frequency_match[S5PC11X_FREQ_TAB];	

	DBG("%s : p_lv = %d : pwr = %d \n", __FUNCTION__, p_lv,pwr);

	if(pwr == PMIC_ARM) {
		voltage = frequency_match_tab[p_lv][pwr + 1];

		if(voltage == s_arm_voltage)
			return ret;

		pmic_val = voltage * 1000;
		
		DBG("regulator_set_voltage =%d\n",voltage);
		/*set Arm voltage*/
		ret = regulator_set_voltage(Reg_Arm,pmic_val,pmic_val);
	        if(ret != 0)
        	{
			printk(KERN_ERR "%s: Cannot set VCC_ARM\n", __func__);
			return -EINVAL;
        	}
		/*put delay according to ramp rate*/
		//udelay(20);
		if(voltage > s_arm_voltage)
			udelay((voltage - s_arm_voltage)/RAMP_RATE);
		else
			udelay((s_arm_voltage - voltage)/RAMP_RATE);

		s_arm_voltage = voltage;	
		
	} else if(pwr == PMIC_INT) {
		voltage = frequency_match_tab[p_lv][pwr + 1];
		if(voltage == s_int_voltage)
                        return ret;

		pmic_val = voltage * 1000;

		DBG("regulator_set_voltage = %d\n",voltage);
		/*set Arm voltage*/
		ret = regulator_set_voltage(Reg_Int, pmic_val, pmic_val);
	        if(ret != 0)
        	{
			printk(KERN_ERR "%s: Cannot set VCC_INT\n", __func__);
			return -EINVAL;
        	}

		/*put delay according to ramp rate*/
		//udelay(20);
		if(voltage > s_int_voltage)
			udelay((voltage - s_int_voltage)/RAMP_RATE);
		else
			udelay((s_int_voltage - voltage)/RAMP_RATE);

                s_int_voltage = voltage;

	}else {
		printk("[error]: set_power, check mode [pwr] value\n");
		return -EINVAL;
	}
	return 0;
}

void set_pmic_gpio(void)
{
	/*set set1, set2, set3 of max8998 driver as 0*/
	/* set GPH0(3), GPH0(4) & GPH0(5) as low*/
	s3c_gpio_cfgpin(GPIO_BUCK_1_EN_A, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_BUCK_1_EN_A, 0);
	set1_gpio = GPIO_BUCK_1_EN_A;
	s3c_gpio_cfgpin(GPIO_BUCK_1_EN_B, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_BUCK_1_EN_B, 0);
	set2_gpio = GPIO_BUCK_1_EN_B;
	s3c_gpio_cfgpin(GPIO_BUCK_2_EN, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_BUCK_2_EN, 0);
	set3_gpio = GPIO_BUCK_2_EN;

}
EXPORT_SYMBOL_GPL(set_pmic_gpio);


int set_voltage(enum perf_level p_lv)
{
	DBG("%s : p_lv = %d\n", __FUNCTION__, p_lv);
	if(step_curr != p_lv) 
	{
		/*Commenting gpio initialisation*/
		//set_pmic_gpio();

		set_max8998(PMIC_ARM, p_lv);
		set_max8998(PMIC_INT, p_lv);
		step_curr = p_lv;
	}
	return 0;
}

EXPORT_SYMBOL(set_voltage);

int set_gpio_dvs(int armSet)
{
	DBG("set dvs with %d", armSet);
	switch(armSet) {
	case DVSARM1:
		gpio_set_value(set1_gpio, 0);
		gpio_set_value(set2_gpio, 0);
		break;
	case DVSARM2:
		gpio_set_value(set2_gpio, 0);
		gpio_set_value(set1_gpio, 1);
		break;
	case DVSARM3:
		gpio_set_value(set1_gpio, 0);
		gpio_set_value(set2_gpio, 1);
		break;
	case DVSARM4:
		gpio_set_value(set1_gpio, 1);
		gpio_set_value(set2_gpio, 1);
		break;
	case DVSINT1:
		gpio_set_value(set3_gpio, 0);
		break;
	case DVSINT2:
//		max8998_set_dvsint_direct(DVSINT2, dvs_arm_voltage_set[5][1]);	//1.0V
		gpio_set_value(set3_gpio, 1);
		break;
//	case DVSINT0:
//		max8998_set_dvsint_direct(DVSINT2, dvs_arm_voltage_set[0][1]);	//1.2V	
//		gpio_set_value(set3_gpio, 1);
//		break;		
	default:
		printk("Invalid parameters to %s",__FUNCTION__);
		return -EINVAL;
	}

	DBG("S5PV210_GPH0CON=%x,S5PV210_GPH0DAT=%x,S5PV210_GPH0PUD=%x\n",readl(S5PV210_GPH0CON),readl(S5PV210_GPH0DAT),readl(S5PV210_GPH0PUD));

	return 0;
}

int dvs_initilized=0;
int set_voltage_dvs(enum perf_level p_lv)
{
	unsigned int new_voltage, old_voltage; 
	const unsigned int (*dvs_volt_table_ptr)[3] = dvs_volt_table[S5PC11X_FREQ_TAB];
	unsigned int (*dvs_arm_voltage_set)[2] = dvs_arm_voltage_set_table[S5PC11X_FREQ_TAB];

	DBG("%s : p_lv = %d\n", __FUNCTION__, p_lv);
	if((step_curr == p_lv) && dvs_initilized)
		return 0;

	/*For arm vdd*/
	new_voltage = dvs_arm_voltage_set[dvs_volt_table_ptr[p_lv][1] - DVSARM1][1];
	old_voltage = dvs_arm_voltage_set[dvs_volt_table_ptr[step_curr][1] - DVSARM1][1];
	DBG("%s ARM VDD: new voltage = %u old voltage = %u\n",__func__,new_voltage,old_voltage);
	if(new_voltage != old_voltage){
		set_gpio_dvs(dvs_volt_table_ptr[p_lv][1]);
		if(new_voltage > old_voltage)
			udelay((new_voltage-old_voltage)/RAMP_RATE);
		else
			udelay((old_voltage-new_voltage)/RAMP_RATE);
	}
	/*For arm int*/
	new_voltage = dvs_arm_voltage_set[dvs_volt_table_ptr[p_lv][2] - DVSARM1][1];
	old_voltage = dvs_arm_voltage_set[dvs_volt_table_ptr[step_curr][2] - DVSARM1][1];
	DBG("%s ARM INT: new voltage = %u old voltage = %u\n",__func__,new_voltage,old_voltage);
	if(new_voltage != old_voltage){
		set_gpio_dvs(dvs_volt_table_ptr[p_lv][2]);
		if(new_voltage > old_voltage)
			udelay((new_voltage-old_voltage)/RAMP_RATE);
		else
			udelay((old_voltage-new_voltage)/RAMP_RATE);
	}

	step_curr = p_lv;
	//if (!dvs_initilized) dvs_initilized=1;
	//msleep(10);
	return 0;
}
EXPORT_SYMBOL(set_voltage_dvs);

void max8998_init(void)
{
	if(S5PC11X_FREQ_TAB) // for 1.2GHZ table
	{
		step_curr = L0;
		set_voltage_dvs(L2); //switch to 800MHZ
	}
	else // for 1GHZ table
	{
		step_curr = L0;
		set_voltage_dvs(L1); //switch to 800MHZ
	}
	if (!dvs_initilized) dvs_initilized=1;
}

EXPORT_SYMBOL(max8998_init);

static int max8998_consumer_probe(struct platform_device *pdev)
{
        int ret = 0;
	unsigned int (*dvs_arm_voltage_set)[2] = dvs_arm_voltage_set_table[S5PC11X_FREQ_TAB];

	DBG(" Function=%s \n",__func__);
        Reg_Arm = regulator_get(NULL, "vddarm");
        if (IS_ERR(Reg_Arm)) {
                printk(KERN_ERR "%s: cant get VCC_ARM\n", __func__);
                return PTR_ERR(Reg_Arm);
        }

	Reg_Int = regulator_get(NULL, "vddint");
        if (IS_ERR(Reg_Int)) {
                printk(KERN_ERR "%s: cant get VCC_INT\n", __func__);
                return PTR_ERR(Reg_Int);
        }

	
	/*initialise the dvs registers*/
	max8998_set_dvsarm_direct(DVSARM1, dvs_arm_voltage_set[0][1]);
	max8998_set_dvsarm_direct(DVSARM2, dvs_arm_voltage_set[1][1]);
	max8998_set_dvsarm_direct(DVSARM3, dvs_arm_voltage_set[2][1]);
	max8998_set_dvsarm_direct(DVSARM4, dvs_arm_voltage_set[3][1]);
	max8998_set_dvsint_direct(DVSINT1, dvs_arm_voltage_set[4][1]);
	max8998_set_dvsint_direct(DVSINT2, dvs_arm_voltage_set[5][1]);
	
	/*Initialize the pmic voltage*/
	max8998_init();
	
	return ret;	
}

static int max8998_consumer_remove(struct platform_device *pdev)
{
        int ret = 0;
	DBG(" Function=%s \n",__func__);
	if(Reg_Arm)
		regulator_put(Reg_Arm);

	if(Reg_Int)
		regulator_put(Reg_Int);
	
	Reg_Arm = NULL;
	Reg_Int = NULL;
	return ret;

}

static int pmic_controling_list;// a global variable to store ldo's status
static int s3c_consumer_suspend(struct platform_device *dev, pm_message_t state)
{
	int i ;
	
	pmic_controling_list = 0;
	
	for (i=MAX8998_LDO3; i<=MAX8998_LDO17;i++) {
		if (i!=MAX8998_LDO9 && i!=MAX8998_LDO3 && i!=MAX8998_LDO4) {
		if (max8998_ldo_is_enabled_direct(i)) {
			//printk("ksoo++++ ldo%d disabled", i);
			pmic_controling_list |= (0x1 << i);
			max8998_ldo_disable_direct(i);
		}
		}
	}
				

	//max8998_ldo_disable_direct(MAX8998_DCDC4);
	//max8998_ldo_disable_direct(MAX8998_DCDC3);
	max8998_ldo_disable_direct(MAX8998_DCDC2);
	max8998_ldo_disable_direct(MAX8998_DCDC1);
	return 0;
}

static int s3c_consumer_resume(struct platform_device *dev)
{
	int i, saved_control;
	
	max8998_ldo_enable_direct(MAX8998_DCDC1);
	max8998_ldo_enable_direct(MAX8998_DCDC2);
	//max8998_ldo_enable_direct(MAX8998_DCDC3);
	//max8998_ldo_enable_direct(MAX8998_DCDC4);
	
	// done in BL code.
	saved_control = pmic_controling_list;
	
	for (i=MAX8998_LDO3;i<=MAX8998_LDO17;i++) {
		if ((saved_control  >> i) & 0x1) max8998_ldo_enable_direct(i);
	}

	return 0;
}

static struct platform_driver max8998_consumer_driver = {
	.driver = {
		   .name = "max8998-consumer",
		   .owner = THIS_MODULE,
		   },
	.probe = max8998_consumer_probe,
	.remove = max8998_consumer_remove,
        .suspend = s3c_consumer_suspend, 
        .resume = s3c_consumer_resume,

};

static int max8998_consumer_init(void)
{
	return platform_driver_register(&max8998_consumer_driver);
}
late_initcall(max8998_consumer_init);

static void max8998_consumer_exit(void)
{
	platform_driver_unregister(&max8998_consumer_driver);
}
module_exit(max8998_consumer_exit);

MODULE_AUTHOR("Amit Daniel");
MODULE_DESCRIPTION("MAX 8998 consumer driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:max8998-consumer");
