/* 
 *  Title : Optical Sensor(light sensor + proximity sensor) driver for GP2AP002A00F   
 *  Date  : 2009.02.27
 *  Name  : ms17.kim
 *
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
//#include <plat/regs-gpio.h> /* TODO */
#include <linux/wakelock.h>

#include <linux/input.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>

//#include <linux/regulator/max8998_function.h>




#include "gp2a_froyo.h"

#include "../../../arch/arm/mach-s5pv210/include/mach/gpio-aries.h"
#include <linux/slab.h>

/*********** for debug **********************************************************/
#if 0 
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/





/* global var */
struct class *lightsensor_class;

struct device *switch_cmd_dev;

int autobrightness_mode = OFF;
static bool light_enable = OFF;
static bool proximity_enable = OFF;
static state_type cur_state = LIGHT_INIT;

static int adc_value_buf[ADC_BUFFER_NUM] = {0, 0, 0, 0, 0, 0};

//static int state_change_count = 0;

static short proximity_value = 0;
static int cur_adc_value = 0;

static bool lightsensor_test = 0;

static struct wake_lock prx_wake_lock;

//static bool light_init_check = false;
//static int light_init_check_count = 0;

//static int light_init_period = 4;
static ktime_t timeA,timeB; /* timeSub; */


static struct i2c_driver opt_i2c_driver;
static struct i2c_client *opt_i2c_client = NULL;

struct opt_state{
	struct i2c_client	*client;	
};

struct opt_state *opt_state;


static ssize_t proximity_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static int proximity_onoff(u8 onoff);
	
static struct file_operations proximity_fops = {
	.owner  = THIS_MODULE,
	.open   = proximity_open,
    	.release = proximity_release,
    	.read = proximity_read,
};
                 
static struct miscdevice proximity_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "proximity",
    .fops   = &proximity_fops,
};

static ssize_t light_read(struct file *filp, double *lux, size_t count, loff_t *f_pos);

static struct file_operations light_fops = {
	.owner  = THIS_MODULE,
	.open   = light_open,
	.release = light_release,
	.read = light_read,
};
                 
static struct miscdevice light_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "light",
    .fops   = &light_fops,
};

short gp2a_get_proximity_value()
{
	  return proximity_value;

}
EXPORT_SYMBOL(gp2a_get_proximity_value);

short gp2a_get_proximity_enable(void)
{
	  return proximity_enable;

}
EXPORT_SYMBOL(gp2a_get_proximity_enable);

bool gp2a_get_lightsensor_status()
{
	  return light_enable;

}
EXPORT_SYMBOL(gp2a_get_lightsensor_status);


/*****************************************************************************************
 *  
 *  function    : gp2a_work_func_light 
 *  description : This function is for light sensor. It has a few state.
 *                "STATE_0" means that circumstance in which you are now is clear as day.
 *                The more value is increased, the more circumstance is dark. 
 *                 
 */

static int buffering = 2;

extern int backlight_level;

#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT
int pre_val = -1;
extern int current_gamma_value;
extern u16 *pmDNIe_Gamma_set[];

typedef enum
{
	mDNIe_UI_MODE,
	mDNIe_VIDEO_MODE,
	mDNIe_VIDEO_WARM_MODE,
	mDNIe_VIDEO_COLD_MODE,
	mDNIe_CAMERA_MODE,
	mDNIe_NAVI
}Lcd_mDNIe_UI;

extern Lcd_mDNIe_UI current_mDNIe_UI;

extern void mDNIe_Mode_set_for_backlight(u16 *buf);

int value_buf[4] = {0};


int IsChangedADC(int val)
{
	int j = 0;
	int ret = 0;

	int adc_index = 0;
	static int adc_index_count = 0;


	adc_index = (adc_index_count)%4;		
	adc_index_count++;

	if(pre_val == -1) //ADC buffer initialize 
	{
		for(j = 0; j<4; j++)
			value_buf[j] = val;

		pre_val = 0;
	}
    else
    {
    	value_buf[adc_index] = val;
	}

	ret = ((value_buf[0] == value_buf[1])&& \
			(value_buf[1] == value_buf[2])&& \
			(value_buf[2] == value_buf[3]))? 1 : 0;

	
	if(adc_index_count == 4)
		adc_index_count = 0;

	return ret;
}
#endif

static void gp2a_work_func_light(struct work_struct *work)
{
	int adc=0;
	state_type level_state = LIGHT_INIT;

	/* read adc data from s5p110 */
	adc = lightsensor_get_adcvalue();
	gprintk("Optimized adc = %d \n",adc);
	gprintk("cur_state = %d\n",cur_state);
	gprintk("light_enable = %d\n",light_enable);
#if 1 //add 150lux
	if(adc >= 2100)
	{
		level_state = LIGHT_LEVEL5;
		buffering = 5;
	}
	else if(adc >= 1900 && adc < 2100)
	{
		if(buffering == 5)
		{	
			level_state = LIGHT_LEVEL5;
			buffering = 5;
		}
		else if((buffering == 1)||(buffering == 2)||(buffering == 3)||(buffering == 4))
		{
			level_state = LIGHT_LEVEL4;
			buffering = 4;
		}
	}

	else if(adc >= 1800 && adc < 1900)
	{
		level_state = LIGHT_LEVEL4;
		buffering = 4;
	}

	else if(adc >= 1200 && adc < 1800)
	{
		if((buffering == 4)||(buffering == 5))
		{	
			level_state = LIGHT_LEVEL4;
			buffering = 4;
		}
		else if((buffering == 1)||(buffering == 2)||(buffering == 3))
		{
			level_state = LIGHT_LEVEL3;
			buffering = 3;
		}
	}
	
	else if(adc >= 800 && adc < 1200)
	{
		level_state = LIGHT_LEVEL3;
		buffering = 3;
	}

	else if(adc >= 600 && adc < 800)
	{
		if((buffering == 3)||(buffering == 4)||(buffering == 5))
		{	
			level_state = LIGHT_LEVEL3;
			buffering = 3;
		}
		else if((buffering == 1)||(buffering == 2))
		{
			level_state = LIGHT_LEVEL2;
			buffering = 2;
		}
	}

	else if(adc >= 400 && adc < 600)
	{
		level_state = LIGHT_LEVEL2;
		buffering = 2;
	}
	
	else if(adc >= 250 && adc < 400)
	{
		if((buffering == 2)||(buffering == 3)||(buffering == 4)||(buffering == 5))
		{	
			level_state = LIGHT_LEVEL2;
			buffering = 2;
		}
		else if(buffering == 1)
		{
			level_state = LIGHT_LEVEL1;
			buffering = 1;
		}
	}

	else if(adc < 250)
	{
		level_state = LIGHT_LEVEL1;
		buffering = 1;
	}
#endif
	if((backlight_level > 5)&&(!lightsensor_test))
	{
		gprintk("backlight_level = %d\n", backlight_level); //Temp
		cur_state = level_state;	
	}
#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT
	if(autobrightness_mode)
	{
		if((pre_val!=1)&&(current_gamma_value == 24)&&(level_state == LIGHT_LEVEL5)&&(current_mDNIe_UI == mDNIe_UI_MODE))
		{	
			mDNIe_Mode_set_for_backlight(pmDNIe_Gamma_set[1]);
			pre_val = 1;
			gprintk("mDNIe_Mode_set_for_backlight - pmDNIe_Gamma_set[1]\n" );
		}
	}
#endif
		
	
	gprintk("cur_state = %d\n",cur_state); //Temp
}

int lightsensor_get_adcvalue(void)
{
	int i = 0;
	int j = 0;
	unsigned int adc_total = 0;
	static int adc_avr_value = 0;
	unsigned int adc_index = 0;
	static unsigned int adc_index_count = 0;
	unsigned int adc_max = 0;
	unsigned int adc_min = 0;
	int value =0;

	//get ADC
	value = s3c_adc_get_adc_data(ADC_CHANNEL);
	gprintk("adc = %d \n",value);
	cur_adc_value = value;
	
	adc_index = (adc_index_count++)%ADC_BUFFER_NUM;		

	if(cur_state == LIGHT_INIT) //ADC buffer initialize (light sensor off ---> light sensor on)
	{
		for(j = 0; j<ADC_BUFFER_NUM; j++)
			adc_value_buf[j] = value;
	}
    else
    {
    	adc_value_buf[adc_index] = value;
	}
	
	adc_max = adc_value_buf[0];
	adc_min = adc_value_buf[0];

	for(i = 0; i <ADC_BUFFER_NUM; i++)
	{
		adc_total += adc_value_buf[i];

		if(adc_max < adc_value_buf[i])
			adc_max = adc_value_buf[i];
					
		if(adc_min > adc_value_buf[i])
			adc_min = adc_value_buf[i];
	}
	adc_avr_value = (adc_total-(adc_max+adc_min))/(ADC_BUFFER_NUM-2);
	
	if(adc_index_count == ADC_BUFFER_NUM-1)
		adc_index_count = 0;

	return adc_avr_value;
}


/*****************************************************************************************
 *  
 *  function    : gp2a_timer_func 
 *  description : This function is for light sensor. 
 *                it operates every a few seconds. 
 *                 
 */

static enum hrtimer_restart gp2a_timer_func(struct hrtimer *timer)
{
	struct gp2a_data *gp2a = container_of(timer, struct gp2a_data, timer);
	ktime_t light_polling_time;			
	
	queue_work(gp2a_wq, &gp2a->work_light);
	//hrtimer_start(&gp2a->timer,ktime_set(LIGHT_PERIOD,0),HRTIMER_MODE_REL);
	light_polling_time = ktime_set(0,0);
	light_polling_time = ktime_add_us(light_polling_time,500000);
	hrtimer_start(&gp2a->timer,light_polling_time,HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}




/*****************************************************************************************
 *  
 *  function    : gp2a_work_func_prox 
 *  description : This function is for proximity sensor (using B-1 Mode ). 
 *                when INT signal is occured , it gets value from VO register.   
 *
 *                 
 */
#if 0
static void gp2a_work_func_prox(struct work_struct *work)
{
	struct gp2a_data *gp2a = container_of(work, struct gp2a_data, work_prox);
	
	unsigned char value;
	unsigned char int_val=REGS_PROX;
	unsigned char vout=0;

	/* Read VO & INT Clear */
	
	gprintk("[PROXIMITY] %s : \n",__func__);

	if(INT_CLEAR)
	{
		int_val = REGS_PROX | (1 <<7);
	}
	opt_i2c_read((u8)(int_val),&value,1);
	vout = value & 0x01;
	printk(KERN_INFO "[PROXIMITY] value = %d \n",vout);



	/* Report proximity information */
	proximity_value = vout;

	
	if(proximity_value ==0)
	{
		timeB = ktime_get();
		
		timeSub = ktime_sub(timeB,timeA);
		printk(KERN_INFO "[PROXIMITY] timeSub sec = %d, timeSub nsec = %d \n",timeSub.tv.sec,timeSub.tv.nsec);
		
		if (timeSub.tv.sec>=3 )
		{
		    wake_lock_timeout(&prx_wake_lock,HZ/2);
			printk(KERN_INFO "[PROXIMITY] wake_lock_timeout : HZ/2 \n");
		}
		else
			printk(KERN_INFO "[PROXIMITY] wake_lock is already set \n");

	}

	if(USE_INPUT_DEVICE)
	{
    	input_report_abs(gp2a->input_dev,ABS_DISTANCE,(int)vout);
    	input_sync(gp2a->input_dev);
	
	
		mdelay(1);
	}

	/* Write HYS Register */

	if(!vout)
	{
		value = 0x40;


	}
	else
	{
		value = 0x23;

	}
	opt_i2c_write((u8)(REGS_HYS),&value);

	/* Forcing vout terminal to go high */

	//value = 0x18;
	//opt_i2c_write((u8)(REGS_CON),&value);


	/* enable INT */

	enable_irq(gp2a->irq);

	/* enabling VOUT terminal in nomal operation */

	value = 0x00;

	opt_i2c_write((u8)(REGS_CON),&value);

}
#endif

irqreturn_t gp2a_irq_handler(int irq, void *dev_id)
{
	char value;
	value = gpio_get_value(GPIO_PS_VOUT);

	wake_lock_timeout(&prx_wake_lock, 3*HZ);
	printk("[PROXIMITY] IRQ_HANDLED %d \n", value);
	return IRQ_HANDLED;
}

static int opt_i2c_init(void) 
{
	
	
	if( i2c_add_driver(&opt_i2c_driver))
	{
		printk("i2c_add_driver failed \n");
		return -ENODEV;
	}

	return 0;
}


int opt_i2c_read(u8 reg, u8 *val, unsigned int len )
{

	int err;
	u8 buf[1];
	struct i2c_msg msg[2];


	buf[0] = reg; 

	msg[0].addr = opt_i2c_client->addr;
	msg[0].flags = 1;
	
	msg[0].len = 2;
	msg[0].buf = buf;
	err = i2c_transfer(opt_i2c_client->adapter, msg, 1);

	
	*val = buf[1];
	
    if (err >= 0) return 0;

    printk("%s %d i2c transfer error\n", __func__, __LINE__);
    return err;
}

int opt_i2c_write( u8 reg, u8 *val )
{
    int err = 0;
    struct i2c_msg msg[1];
    unsigned char data[2];
    int retry = 10;

    if( (opt_i2c_client == NULL) || (!opt_i2c_client->adapter) ){
        return -ENODEV;
    }

    while(retry--)
    {
        data[0] = reg;
        data[1] = *val;

        msg->addr = opt_i2c_client->addr;
        msg->flags = I2C_M_WR;
        msg->len = 2;
        msg->buf = data;

        err = i2c_transfer(opt_i2c_client->adapter, msg, 1);

        if (err >= 0) return 0;
    }
    printk("%s %d i2c transfer error\n", __func__, __LINE__);
    return err;
}



void gp2a_chip_init(void)
{
	/* Power On */
	//LDO PS_ON high : LEDA_3.0V
	int err = 1;

	gprintk("\n");

	err = gpio_request(GPIO_PS_ON, "PS_ON");

	if (err) {
		printk(KERN_ERR "failed to request GPJ1(4) for "
			"LEDA_3.0V control\n");
		return;
	}
	gpio_direction_output(GPIO_PS_ON, 1);
	gpio_set_value(GPIO_PS_ON, 1);
	gpio_free(GPIO_PS_ON);

	s3c_gpio_slp_cfgpin(GPIO_PS_ON, S3C_GPIO_SLP_PREV);
    	//s3c_gpio_slp_setpull_updown(GPIO_PS_ON, S3C_GPIO_PULL_NONE);

	mdelay(1);
	
	/* set INT 	*/
	s3c_gpio_cfgpin(GPIO_PS_VOUT, S3C_GPIO_SFN(GPIO_PS_VOUT_AF));
	s3c_gpio_setpull(GPIO_PS_VOUT, S3C_GPIO_PULL_NONE);
	set_irq_type(IRQ_GP2A_INT, IRQ_TYPE_EDGE_BOTH);
		
	
}


/*****************************************************************************************
 *  
 *  function    : gp2a_on 
 *  description : This function is power-on function for optical sensor.
 *
 *  int type    : Sensor type. Two values is available (PROXIMITY,LIGHT).
 *                it support power-on function separately.
 *                
 *                 
 */
 
 

void gp2a_on(struct gp2a_data *gp2a, int type)
{
	ktime_t light_polling_time;
	gprintk(KERN_INFO "[OPTICAL] gp2a_on(%d)\n",type);
	if(type == PROXIMITY)
	{
		#if 0
		gprintk("[PROXIMITY] go nomal mode : power on \n");
		value = 0x18;
		opt_i2c_write((u8)(REGS_CON),&value);

		value = 0x40;
		opt_i2c_write((u8)(REGS_HYS),&value);

		value = 0x01;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		
		gprintk("enable irq for proximity\n");
		enable_irq(gp2a ->irq);

		value = 0x00;
		opt_i2c_write((u8)(REGS_CON),&value);
		
		value = 0x01;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		proximity_enable =1;
		#endif	
	}
	if(type == LIGHT)
	{
		gprintk(KERN_INFO "[LIGHT_SENSOR] timer start for light sensor\n");
		light_polling_time = ktime_set(0,0);
		light_polling_time = ktime_add_us(light_polling_time,500000);
	    //hrtimer_start(&gp2a->timer,ktime_set(LIGHT_PERIOD,0),HRTIMER_MODE_REL);
	    hrtimer_start(&gp2a->timer,light_polling_time,HRTIMER_MODE_REL);
		light_enable = ON;
	}
}

/*****************************************************************************************
 *  
 *  function    : gp2a_off 
 *  description : This function is power-off function for optical sensor.
 *
 *  int type    : Sensor type. Three values is available (PROXIMITY,LIGHT,ALL).
 *                it support power-on function separately.
 *                
 *                 
 */

static void gp2a_off(struct gp2a_data *gp2a, int type)
{
	gprintk(KERN_INFO "[OPTICAL] gp2a_off(%d)\n",type);
	if(type == PROXIMITY || type == ALL)
	{

		#if 0
		gprintk("[PROXIMITY] go power down mode  \n");
		
		//gprintk("disable irq for proximity \n");
		//disable_irq(gp2a ->irq);
		
		value = 0x00;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		
		proximity_enable =0;
		proximity_value = 0;
		#endif
	}

	if(type ==LIGHT)
	{
		gprintk("[LIGHT_SENSOR] timer cancel for light sensor\n");
		hrtimer_cancel(&gp2a->timer);
		light_enable = OFF;
	}
}




/*					 */
/*					 */
/*					 */
/* for  test mode 	start */
/*					 */
/*					 */
static int AdcToLux_Testmode(int adc)
{
	unsigned int lux = 0;

	gprintk("[%s] adc:%d\n",__func__,adc);
	/*temporary code start*/
	
	if(adc >= 1800)
		lux = 100000;
	
	else if(adc >= 800 && adc < 1800){
		lux = 5000;
	}
	else if(adc < 800){
		lux = 10;
	}
	/*temporary code end*/
	
	return lux;
}


static ssize_t lightsensor_file_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int adc = 0;
	int sum = 0;
	int i = 0;

	gprintk("called %s \n",__func__);

	if(!light_enable)
	{
		for(i = 0; i < 10; i++)
		{
			adc = lightsensor_get_adcvalue();
			msleep(20);
			sum += adc;
		}
		adc = sum/10;
		gprintk("called %s  - subdued alarm(adc : %d)\n",__func__,adc);
		return sprintf(buf,"%d\n",adc);
	}
	else
	{
		gprintk("called %s  - *#0589#\n",__func__);
		#if 1
	return sprintf(buf,"%d\n",cur_adc_value);
		#else 
		adc = s3c_adc_get_adc_data(ADC_CHANNEL);
		return sprintf(buf,"%d\n",adc);
		#endif
	}
}
static ssize_t lightsensor_file_state_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	gprintk("called %s \n",__func__);
	sscanf(buf, "%d", &value);

	if(value==1)
	{
		autobrightness_mode = ON;
		printk("[brightness_mode] BRIGHTNESS_MODE_SENSOR\n");
	}
	else if(value==0) 
	{
		autobrightness_mode = OFF;
		printk("[brightness_mode] BRIGHTNESS_MODE_USER\n");
		#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT
		if(pre_val==1)
		{
			mDNIe_Mode_set_for_backlight(pmDNIe_Gamma_set[2]);
		}	
		pre_val = -1;
		#endif
	}

	return size;
}

/* for light sensor on/off control from platform */
static ssize_t lightsensor_file_cmd_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	gprintk("called %s \n",__func__);

	return sprintf(buf,"%u\n",light_enable);
}
static ssize_t lightsensor_file_cmd_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	int value;
    sscanf(buf, "%d", &value);

	printk(KERN_INFO "[LIGHT_SENSOR] in lightsensor_file_cmd_store, input value = %d \n",value);

	if(value==1)
	{
		//gp2a_on(gp2a,LIGHT);	Light sensor is always on
		lightsensor_test = 1;
		value = ON;
		printk(KERN_INFO "[LIGHT_SENSOR] *#0589# test start, input value = %d \n",value);
	}
	else if(value==0) 
	{
		//gp2a_off(gp2a,LIGHT);	Light sensor is always on
		lightsensor_test = 0;
		value = OFF;
		printk(KERN_INFO "[LIGHT_SENSOR] *#0589# test end, input value = %d \n",value);
	}

	/* temporary test code for proximity sensor */
	else if(value==3 && proximity_enable == OFF)
	{
		gp2a_on(gp2a,PROXIMITY);
		printk("[PROXIMITY] Temporary : Power ON\n");


	}
	else if(value==2 && proximity_enable == ON)
	{
		gp2a_off(gp2a,PROXIMITY);
		printk("[PROXIMITY] Temporary : Power OFF\n");

	}
	/* for factory simple test mode */
	if(value==7 && light_enable == OFF)
	{
	//	light_init_period = 2;
		gp2a_on(gp2a,LIGHT);
		value = 7;
	}

	return size;
}

static DEVICE_ATTR(lightsensor_file_cmd,0666, lightsensor_file_cmd_show, lightsensor_file_cmd_store);
static DEVICE_ATTR(lightsensor_file_state,0666, lightsensor_file_state_show, lightsensor_file_state_store);
/*					 */
/*					 */
/* for  test mode 	end  */
/*					 */
/*					 */



static int gp2a_opt_probe( struct platform_device* pdev )
{
	
	struct gp2a_data *gp2a;
	int irq;
	int ret;
       u8 value;

	/* allocate driver_data */
	gp2a = kzalloc(sizeof(struct gp2a_data),GFP_KERNEL);
	if(!gp2a)
	{
		pr_err("kzalloc error\n");
		return -ENOMEM;

	}

	/* hrtimer Settings */

	hrtimer_init(&gp2a->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gp2a->timer.function = gp2a_timer_func;

    gp2a_wq = create_singlethread_workqueue("gp2a_wq");
    if (!gp2a_wq)
	    return -ENOMEM;
  //  INIT_WORK(&gp2a->work_prox, gp2a_work_func_prox);
    INIT_WORK(&gp2a->work_light, gp2a_work_func_light);
	
	gprintk("Workqueue Settings complete\n");

	ret = misc_register(&light_device);
	if(ret) {
		pr_err(KERN_ERR "misc_register failed - light\n");
	}


	/* set platdata */
	platform_set_drvdata(pdev, gp2a);

	/* set sysfs for light sensor */

	lightsensor_class = class_create(THIS_MODULE, "lightsensor");
	if (IS_ERR(lightsensor_class))
		pr_err("Failed to create class(lightsensor)!\n");

	switch_cmd_dev = device_create(lightsensor_class, NULL, 0, NULL, "switch_cmd");
	if (IS_ERR(switch_cmd_dev))
		pr_err("Failed to create device(switch_cmd_dev)!\n");

	if (device_create_file(switch_cmd_dev, &dev_attr_lightsensor_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_lightsensor_file_cmd.attr.name);

	if (device_create_file(switch_cmd_dev, &dev_attr_lightsensor_file_state) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_lightsensor_file_state.attr.name);
	dev_set_drvdata(switch_cmd_dev,gp2a);

	gp2a_off(gp2a,LIGHT);
	
	
	/* init i2c */
	opt_i2c_init();

	if(opt_i2c_client == NULL)
	{
		pr_err("opt_probe failed : i2c_client is NULL\n"); 
		return -ENODEV;
	}
	else
		printk("opt_i2c_client : (0x%p)\n",opt_i2c_client);
	

	/* Input device Settings */
	if(USE_INPUT_DEVICE)
	{
		gp2a->input_dev = input_allocate_device();
		if (gp2a->input_dev == NULL) 
		{
			pr_err("Failed to allocate input device\n");
			return -ENOMEM;
		}
		gp2a->input_dev->name = "proximity";
	
		set_bit(EV_SYN,gp2a->input_dev->evbit);
		set_bit(EV_ABS,gp2a->input_dev->evbit);
		
        input_set_abs_params(gp2a->input_dev, ABS_DISTANCE, 0, 1, 0, 0);
		
	
		ret = input_register_device(gp2a->input_dev);
		if (ret) 
		{
			pr_err("Unable to register %s input device\n", gp2a->input_dev->name);
			input_free_device(gp2a->input_dev);
			kfree(gp2a);
			return -1;
		}

	}

	/* misc device Settings */
	ret = misc_register(&proximity_device);
	if(ret) {
		pr_err(KERN_ERR "misc_register failed - prox \n");
	}

	/* wake lock init */
	wake_lock_init(&prx_wake_lock, WAKE_LOCK_SUSPEND, "prx_wake_lock");


	/* set platdata */
	platform_set_drvdata(pdev, gp2a);

	/* ktime init */

	timeA = ktime_set(0,0);
	timeB = ktime_set(0,0);
	


	/* GP2A Regs INIT SETTINGS */
	
	value = 0x00;
	opt_i2c_write((u8)(REGS_OPMOD),&value);

	/* INT Settings */	
	irq = IRQ_GP2A_INT;
	gp2a->irq = -1;
	ret = request_irq(irq, gp2a_irq_handler, IRQF_DISABLED, "proximity_int", gp2a);
	
	if (ret) {
		printk("unable to request irq proximity_int err:: %d\n", ret);
		return ret;
	}       
       disable_irq(IRQ_GP2A_INT);
	gp2a->irq = irq;
	gprintk("INT Settings complete\n");
	
	/* maintain power-down mode before using sensor */

	printk("gp2a_opt_probe is OK!!\n");
	
	return 0;
}

static int gp2a_opt_suspend( struct platform_device* pdev, pm_message_t state )
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);
	
	gprintk("[%s] : \n",__func__);

	if(light_enable)
	{
		#if 1
		gprintk("[%s] : hrtimer_cancle \n",__func__);
		hrtimer_cancel(&gp2a->timer);
		#endif
	}

	if(!proximity_enable)
	{
		s3c_gpio_cfgpin(GPIO_PS_ON, S3C_GPIO_OUTPUT);
		//s3c_gpio_cfgpin(GPIO_ALS_SDA_28V, S3C_GPIO_OUTPUT);
		//s3c_gpio_cfgpin(GPIO_ALS_SCL_28V, S3C_GPIO_OUTPUT);
		gpio_set_value(GPIO_PS_ON, 0);
		//gpio_set_value(GPIO_ALS_SDA_28V, 0);
		//gpio_set_value(GPIO_ALS_SCL_28V, 0);
	}



	return 0;
}

static int gp2a_opt_resume( struct platform_device* pdev )
{

	//struct gp2a_data *gp2a = platform_get_drvdata(pdev);
	u8 value;

	s3c_gpio_cfgpin(GPIO_PS_ON, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_PS_ON, 1);
	gprintk("[%s] : \n",__func__);
	/* wake_up source handler */
	if(proximity_enable)
	{
		#if 0
		value = 0x18;
		opt_i2c_write((u8)(REGS_CON),&value);

		value = 0x40;
		opt_i2c_write((u8)(REGS_HYS),&value);
		
		
		value = 0x01;
		opt_i2c_write((u8)(REGS_OPMOD),&value);

		enable_irq(gp2a->irq);
		
		value = 0x00;
		opt_i2c_write((u8)(REGS_CON),&value);

	       wake_lock_timeout(&prx_wake_lock,3 * HZ);
		timeA = ktime_get();
		printk("[%s] : wake_lock_timeout 3 Sec \n",__func__);
		#endif
		/*
		if(!gpio_get_value(GPIO_PS_VOUT))
		{
			gp2a_irq_handler(gp2a->irq,gp2a);
			

		}
		*/

	
	}
       else
       {
            	value = 0x00;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
       }

//	cur_state = LIGHT_INIT;
//	light_init_check = false;
	
	if(light_enable)
	{
		#if 0
		gprintk("[%s] : hrtimer_start \n",__func__);
	       light_polling_time = ktime_set(0,0);
		light_polling_time = ktime_add_us(light_polling_time,500000);
		hrtimer_start(&gp2a->timer,light_polling_time,HRTIMER_MODE_REL);
		#endif
	}

	return 0;
}

static double StateToLux(state_type state)
{
	double lux = 0;
	
	gprintk("[%s] cur_state:%d\n",__func__,state);

	if(state== LIGHT_LEVEL5){
		lux = 15000.0;
	}
	else if(state == LIGHT_LEVEL4){
		lux = 9000.0;
	}
	else if(state == LIGHT_LEVEL3){
		lux = 5000.0;
	}
	else if(state == LIGHT_LEVEL2){
		lux = 1000.0;
	}
	else if(state == LIGHT_LEVEL1){
		lux = 6.0;
	}

	else {
		lux = 5000.0;
		gprintk("[%s] cur_state fail\n",__func__);
	}
	return lux;
}

static int light_open(struct inode *ip, struct file *fp)
{
	struct gp2a_data *gp2a = dev_get_drvdata(switch_cmd_dev);

	gprintk("[%s] \n",__func__);
	gp2a_on(gp2a,LIGHT);
	return 0;

}

static ssize_t light_read(struct file *filp, double *lux, size_t count, loff_t *f_pos)
{
	double lux_val;

	lux_val = StateToLux(cur_state);
	//printk("GP2A: light_read(): cur_state = %d\n",cur_state);
	put_user(lux_val, lux);
	
	return 1;
}

static int light_release(struct inode *ip, struct file *fp)
{
	struct gp2a_data *gp2a = dev_get_drvdata(switch_cmd_dev);

	gp2a_off(gp2a,LIGHT);	
	gprintk("[%s] \n",__func__);
	return 0;
}

static int proximity_onoff(u8 onoff)
{
	u8 value;
       int i;
       
	if(onoff)
	{
        	for(i=1;i<5;i++)
        	{
        		opt_i2c_write((u8)(i),&gp2a_original_image[i]);
        	}
		proximity_enable = 1;
              enable_irq(IRQ_GP2A_INT);
	}
	else
	{
              disable_irq(IRQ_GP2A_INT);
		value = 0x00;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		proximity_enable = 0;
	}
	return 0;
}

static int proximity_open(struct inode *ip, struct file *fp)
{
	proximity_onoff(1);
	
	gprintk("[%s] \n",__func__);
	return 0;

}

static ssize_t proximity_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char value;
	//printk("[%s] \n",__func__);
	value = gpio_get_value(GPIO_PS_VOUT);
	//printk("[%s]  %d\n",__func__, value);
	put_user(value, buf);
	return 1;
}

static int proximity_release(struct inode *ip, struct file *fp)
{
	gprintk("[%s] \n",__func__);
	proximity_onoff(0);
	return 0;
}

static int opt_i2c_remove(struct i2c_client *client)
{
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;
	opt_i2c_client = client;
	return 0;
}

static int opt_i2c_probe(struct i2c_client *client,  const struct i2c_device_id *id)
{
	struct opt_state *opt;

	opt = kzalloc(sizeof(struct opt_state), GFP_KERNEL);
	if (opt == NULL) {		
		printk("failed to allocate memory \n");
		return -ENOMEM;
	}
	
	opt->client = client;
	i2c_set_clientdata(client, opt);
	
	/* rest of the initialisation goes here. */
	
	printk("GP2A opt i2c attach success!!!\n");

	opt_i2c_client = client;

	return 0;
}


static const struct i2c_device_id opt_device_id[] = {
	{"gp2a", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, opt_device_id);

static struct i2c_driver opt_i2c_driver = {
	.driver = {
		.name = "gp2a",
		.owner= THIS_MODULE,
	},
	.probe		= opt_i2c_probe,
	.remove		= opt_i2c_remove,
	.id_table	= opt_device_id,
};


static struct platform_driver gp2a_opt_driver = {
	.probe 	 = gp2a_opt_probe,
	.suspend = gp2a_opt_suspend,
	.resume  = gp2a_opt_resume,
	.driver  = {
		.name = "gp2a-opt",
		.owner = THIS_MODULE,
	},
};

static int __init gp2a_opt_init(void)
{
	int ret;
	
	gp2a_chip_init();
	ret = platform_driver_register(&gp2a_opt_driver);
	return ret;
	
	
}
static void __exit gp2a_opt_exit(void)
{
	struct gp2a_data *gp2a = dev_get_drvdata(switch_cmd_dev);
    if (gp2a_wq)
		destroy_workqueue(gp2a_wq);

//	free_irq(IRQ_GP2A_INT,gp2a);
	
	if(USE_INPUT_DEVICE)
		input_unregister_device(gp2a->input_dev);
	kfree(gp2a);

	platform_driver_unregister(&gp2a_opt_driver);
}


module_init( gp2a_opt_init );
module_exit( gp2a_opt_exit );

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for gp2ap002a00f");
MODULE_LICENSE("GPL");

