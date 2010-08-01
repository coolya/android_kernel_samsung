/*
 * cmc623_tune.c
 *
 * Parameter read & save driver on param partition.
 *
 * COPYRIGHT(C) Samsung Electronics Co.Ltd. 2006-2010 All Right Reserved.  
 *
 */
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
// #include <linux/tegra_devices.h>
// #include <nvodm_services.h>
#include <linux/i2c.h>
#include "tune_cmc623.h"
#include <mach/gpio.h>
#include <mach/gpio-p1.h>
#include <plat/gpio-cfg.h>
#include <linux/delay.h>

#if 0  // dgahn.test: turn off AP LED
#define GPIO_PORT_LEDAP  'v'-'a'  // LED_AP: GPIO_PV4
#define GPIO_PIN_LEDAP 4
#endif

//#define CMC623_TUNING

//#define MDNIE_TUNING_TUNEFILE_PATH	"/sdcard/sd/p1/mdnie_tune"
//#define CMC623_PATH_TUNING_DATA       "/sdcard/cmc623_tune"
#define CMC623_PATH_TUNING_DATA3  "/sdcard/sd/p1/mdnie_tune"
#define CMC623_PATH_TUNING_DATA2  "/sdcard/sd/p1/1"
#define CMC623_PATH_TUNING_DATA   "/sdcard/sd/p1/cmc623_tune"

#define CMC623_ADDR		     0x70  /* slave addr for i2c */
#define CMC623_DEVICE_ADDR   (0x38 << 1)
#define CMC623_MAX_SETTINGS	 100
#define CMC623_I2C_SPEED_KHZ 400

#define klogi(fmt, arg...)  printk(KERN_INFO "%s: " fmt "\n" , __func__, ## arg)
#define kloge(fmt, arg...)  printk(KERN_ERR "%s(%d): " fmt "\n" , __func__, __LINE__, ## arg)

//static NvOdmServicesI2cHandle s_hOdmI2c;

#ifdef CMC623_TUNING
static Cmc623RegisterSet Cmc623_TuneSeq[CMC623_MAX_SETTINGS];
#endif

#if 1 
/*
static unsigned short cmc6230_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short cmc6230_ignore[] = { I2C_CLIENT_END };
static unsigned short cmc6230_probe[] = { 8, (0x38), I2C_CLIENT_END }; // 부팅  안됨 P1_LSJ : DE07
*/
//static unsigned short cmc6230_probe[] = { 8, (0x70), I2C_CLIENT_END }; // 부팅 OK 
//static unsigned short cmc6230_probe[] = { 8, (0x70 >> 1), I2C_CLIENT_END };    // Error 발생 
#else 
static unsigned short cmc6230_normal_i2c[] = {(CMC623_ADDR>>1),I2C_CLIENT_END};
static unsigned short cmc6230_ignore[] = {1,(CMC623_ADDR>>1),I2C_CLIENT_END};
static unsigned short cmc6230_probe[] = {I2C_CLIENT_END};

unsigned short qt602240_i2c_normal[] = {I2C_CLIENT_END};
unsigned short qt602240_i2c_ignore[] = {I2C_CLIENT_END};
unsigned short qt602240_i2c_probe[] = {2, QT602240_I2C_ADDR, I2C_CLIENT_END};
#endif 

#if 1 // P1_LSJ : DE04 : Add 
static struct i2c_client *g_client;
#define I2C_M_WR 0 /* for i2c */
#define I2c_M_RD 1 /* for i2c */

/* Each client has this additional data */
struct cmc623_data {
	struct i2c_client *client;
};

static struct cmc623_data * p_cmc623_data = NULL;

//static struct i2c_driver cmc623_i2c_driver;
//static struct i2c_client *cmc623_i2c_client = NULL;

/*
static struct i2c_client_address_data cmc623_addr_data = {
	.normal_i2c	= cmc6230_normal_i2c,
	.ignore		= cmc6230_ignore,
	.probe		= cmc6230_probe,
};
*/

//unsigned short *test[1];
//EXPORT_SYMBOL(test);
#endif 

#if 1  
static int cmc623_I2cWrite(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	int ret, i;
	u8 buf[length+1];
	struct i2c_msg msg[1];

	buf[0] = reg;
	for(i=0; i<length; i++)
    {   
		buf[i+1] = *(data++);
    }    

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = length+1;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
    {   
        return -EIO;
    }

	return 0;
}

bool cmc623_I2cWrite16( unsigned char Addr, unsigned long Data)
{
    int err;
    struct i2c_msg msg[1];
    unsigned char data[3];

	if(!p_cmc623_data)
		{
	    printk(KERN_ERR "p_cmc623_data is NULL\n");
        return -ENODEV;
		}
	g_client = p_cmc623_data->client;		

    if( (g_client == NULL))  
    {
        printk("cmc623_I2cWrite16 g_client is NULL\n");
        return -ENODEV;
    }

    if (!g_client->adapter) 
    {
        printk("cmc623_I2cWrite16 g_client->adapter is NULL\n");
        return -ENODEV;
    }
    
    data[0] = Addr;
    data[1] = ((Data >>8)&0xFF);
    data[2] = (Data)&0xFF;
    msg->addr = g_client->addr;
    msg->flags = I2C_M_WR;
    msg->len = 3;
    msg->buf = data;
    
    err = i2c_transfer(g_client->adapter, msg, 1);

    if (err >= 0) 
    {
        //printk("%s %d i2c transfer OK\n", __func__, __LINE__);/* add by inter.park */
        return 0;
    }

    printk("%s %d i2c transfer error:%d\n", __func__, __LINE__, err);/* add by inter.park */
    return err;    
}

#else 
NvBool cmc623_I2cWrite16( NvU8 Addr, NvU32 Data)
{
    NvBool RetVal = NV_TRUE;

    NvU8 WriteBuffer[3];
    NvOdmI2cStatus status = NvOdmI2cStatus_Success;    
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU32 DeviceAddr = (NvU32)CMC623_DEVICE_ADDR;

    s_hOdmI2c = NvOdmI2cPinMuxOpen(NvOdmIoModule_I2c, 1, NvOdmI2cPinMap_Config2);
    if (!s_hOdmI2c) {
        RetVal = NV_FALSE;
        kloge("NvOdmI2cPinMuxOpen Failed!\n"); 
        goto CMC623_I2cWrite16_exit;
    }

    WriteBuffer[0] = Addr & 0xFF;   // PMU offset
    WriteBuffer[1] = (NvU8)((Data >>  8) & 0xFF);
    WriteBuffer[2] = (NvU8)(Data & 0xFF);

    TransactionInfo.Address = DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 3;

    status = NvOdmI2cTransaction(s_hOdmI2c, &TransactionInfo, 1, 
                        CMC623_I2C_SPEED_KHZ, NV_WAIT_INFINITE);
    if (status == NvOdmI2cStatus_Success)
        RetVal = NV_TRUE;
    else {
        switch (status) {
            case NvOdmI2cStatus_Timeout:
                kloge("cmc623_I2cWrite16 Failed: Timeout"); 
                break;
             case NvOdmI2cStatus_SlaveNotFound:
                kloge("cmc623_I2cWrite16 Failed: SlaveNotFound");
             default:
                break;             
        }
        RetVal = NV_FALSE;
    }

CMC623_I2cWrite16_exit:
    NvOdmI2cClose(s_hOdmI2c);
    s_hOdmI2c = NULL;
    return RetVal;
}
#endif 

#if 1  // P1_LSJ : DE04
char cmc623_I2cRead(u8 reg, u8 *val, unsigned int len )
{
    int      err;
    struct   i2c_msg msg[1];
    
    unsigned char data[1];
    if( (g_client == NULL) || (!g_client->adapter) )
    {
        return -ENODEV;
    }
    
    msg->addr   = g_client->addr;
    msg->flags  = I2C_M_WR;
    msg->len    = 1;
    msg->buf    = data;
    *data       = reg;

    err = i2c_transfer(g_client->adapter, msg, 1);

    if (err >= 0) 
    {
        msg->flags = I2C_M_RD;
        msg->len   = len;
        msg->buf   = val;
        err = i2c_transfer(g_client->adapter, msg, 1);
    }

    if (err >= 0) 
    {
        return 0;
    }
    printk("%s %d i2c transfer error\n", __func__, __LINE__);/* add by inter.park */

    return err;

}

#else 
NvBool cmc623_I2cRead16(
    NvU8 Addr,
    NvU32 *Data)
{
    NvBool RetVal = NV_TRUE;

    NvU8 ReadBuffer[2];  // 2 bytes
    NvOdmI2cStatus status = NvOdmI2cStatus_Success;    
    NvOdmI2cTransactionInfo TransactionInfo[2];
    NvU32 DeviceAddr = (NvU32)CMC623_DEVICE_ADDR;

    s_hOdmI2c = NvOdmI2cPinMuxOpen(NvOdmIoModule_I2c, 1, NvOdmI2cPinMap_Config2);
    if (!s_hOdmI2c) {
        RetVal = NV_FALSE;
        goto CMC623_I2cRead16_exit;
    }

    // Write the PMU offset
    ReadBuffer[0] = Addr & 0xFF;

    TransactionInfo[0].Address = DeviceAddr;
    TransactionInfo[0].Buf = &ReadBuffer[0];
    TransactionInfo[0].Flags = NVODM_I2C_IS_WRITE/* | NVODM_I2C_USE_REPEATED_START*/;
    TransactionInfo[0].NumBytes = 1;    

    TransactionInfo[1].Address = (DeviceAddr | 0x1);
    TransactionInfo[1].Buf = &ReadBuffer[0];
    TransactionInfo[1].Flags = 0;
    TransactionInfo[1].NumBytes = 2;  // 2 bytes

    status = NvOdmI2cTransaction(s_hOdmI2c, &TransactionInfo[0], 2, 
                        CMC623_I2C_SPEED_KHZ, NV_WAIT_INFINITE);
    if (status == NvOdmI2cStatus_Success) {
        *Data = (ReadBuffer[0] << 8) | ReadBuffer[1];
        RetVal = NV_TRUE;
    }
    else
        RetVal = NV_FALSE;

CMC623_I2cRead16_exit:
    NvOdmI2cClose(s_hOdmI2c);
    s_hOdmI2c = NULL;
    return RetVal;
}
#endif 

#ifdef CMC623_TUNING

//static bool cmc623_tune(unsigned long num)
bool cmc623_tune(unsigned long num) // P1_LSJ : DE08
{
	unsigned long i;
    int           k=0; 
    
	//  num = NV_ARRAY_SIZE(Cmc623_TuneSeq);
	printk("========== Start of tuning CMC623 Jun  ==========\n");

	for (i=0; i<num; i++) 
    {
		printk("[%2d] Writing => reg: 0x%2x, data: 0x%4x\n", i+1, Cmc623_TuneSeq[i].RegAddr, Cmc623_TuneSeq[i].Data);

		if (0 > cmc623_I2cWrite16(Cmc623_TuneSeq[i].RegAddr, Cmc623_TuneSeq[i].Data)) 

        // cmc623_I2cWrite(struct i2c_client *client, u8 reg, u8 *data, u8 length)
        //if (!cmc623_I2cWrite16(Cmc623_TuneSeq[i].RegAddr, Cmc623_TuneSeq[i].Data)) 
        {
			printk("I2cWrite16 failed\n");
			return 0;
		}
        else 
        {
			printk("I2cWrite16 succeed\n");
        }

        if ( Cmc623_TuneSeq[i].RegAddr == CMC623_REG_SWRESET && Cmc623_TuneSeq[i].Data == 0xffff ) 
        {
			// NvOdmOsWaitUS(3000);  // 3ms  
			// msleep(3);
            mdelay(3);
		}
	}
	printk("==========  End of tuning CMC623 Jun  ==========\n");
	return 1;
}

//static int parse_text(char * src, int len, unsigned short * output)
static int parse_text(char * src, int len)
{
	int i,count, ret;
	int index=0;
	char * str_line[CMC623_MAX_SETTINGS];
	char * sstart;
	char * c;
	unsigned int data1, data2;

	c = src;
	count = 0;
	sstart = c;
    
	for(i=0; i<len; i++,c++) 
    {
		char a = *c;
		if(a=='\r' || a=='\n') 
        {
			if(c > sstart) 
            {
				str_line[count] = sstart;
				count++;
			}
			*c='\0';
			sstart = c+1;
		}
	}
    
	if(c > sstart) 
    {
		str_line[count] = sstart;
		count++;
	}

	printk("----------------------------- Total number of lines:%d\n", count);

	for(i=0; i<count; i++) 
    {
		printk("line:%d, [start]%s[end]\n", i, str_line[i]);
		ret = sscanf(str_line[i], "0x%x,0x%x\n", &data1, &data2);
		printk("Result => [0x%2x 0x%4x] %s\n", data1, data2, (ret==2)?"Ok":"Not available");
		if(ret == 2) 
        {   
            #if 1 // P1_LSJ : DE08 
			Cmc623_TuneSeq[index].RegAddr = (unsigned char)data1;
			Cmc623_TuneSeq[index++].Data  = (unsigned long)data2;
            #else 
			Cmc623_TuneSeq[index].RegAddr = (NvU8)data1;
			Cmc623_TuneSeq[index++].Data = (NvU32)data2;
            #endif 
		}
	}
	return index;
}

static int cmc623_load_data(void)
{
	struct file *filp;
	char	*dp;
	long	l, i ;
	loff_t  pos;
	int     ret, num;
	mm_segment_t fs;

	klogi("cmc623_load_data start!");

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(CMC623_PATH_TUNING_DATA, O_RDONLY, 0);
	if(IS_ERR(filp)) 
    {
		kloge("file open error:%d", filp);

		return -1;

		/*		
        filp = filp_open(CMC623_PATH_TUNING_DATA2, O_RDONLY, 0);
        if(IS_ERR(filp)) 
        {
            kloge("file open error2");

            filp = filp_open(CMC623_PATH_TUNING_DATA3, O_RDONLY, 0);
            if(IS_ERR(filp)) 
            {
                kloge("file open error3");
                return -1;
            }
        }
        */
	}

	l = filp->f_path.dentry->d_inode->i_size;
	klogi("Size of the file : %ld(bytes)", l);

	//dp = kmalloc(l, GFP_KERNEL);
	dp = kmalloc(l+10, GFP_KERNEL);		// add cushion
	if(dp == NULL) 
    {
		kloge("Out of Memory!");
		filp_close(filp, current->files);
		return -1;
	}
	pos = 0;
	memset(dp, 0, l);
    kloge("== Before vfs_read ======");
	ret = vfs_read(filp, (char __user *)dp, l, &pos);   // P1_LSJ : DE08 : 여기서 죽음 
    kloge("== After vfs_read ======");

	if(ret != l) 
    {
		kloge("<CMC623> Failed to read file (ret = %d)", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	for(i=0; i<l; i++)
    {   
		printk("%x ", dp[i]);
    }
	printk("\n");

	num = parse_text(dp, l);

	if(!num) 
    {
		kloge("Nothing to parse!");
		kfree(dp);
		return -1;
	}
		
	printk("------ Jun Total number of parsed lines: %d\n", num);
	cmc623_tune(num);

	kfree(dp);
	return num;
}

int CMC623_tuning_load_from_file(void)
{
	return cmc623_load_data();
}
EXPORT_SYMBOL(CMC623_tuning_load_from_file);

#endif	//CMC623_TUNING


int cmc623_test(void)
{
	int ret = 0;
    int k =0; 

	klogi("===== cmc623_test =====");
    
	if (gpio_is_valid(GPIO_CMC_BYPASS)) 
    {
		if (gpio_request(GPIO_CMC_BYPASS, "CMC_BYPASS"))
			printk(KERN_ERR "Filed to request GPIO_CMC_BYPASS!\n");
		gpio_direction_output(GPIO_CMC_BYPASS, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_CMC_BYPASS, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_BYPASS, GPIO_LEVEL_HIGH);
    mdelay(2);
    
	if (gpio_is_valid(GPIO_CMC_RST)) {
		if (gpio_request(GPIO_CMC_RST, "CMC_RST"))
			printk(KERN_ERR "Filed to request GPIO_CMC_RST!\n");
		gpio_direction_output(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_CMC_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_LOW);
    mdelay(4);
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
	mdelay(5);

    // a27  // P1_LSJ DE13
    // add, value // DNR:bypass, HDTR: bypass Gamma:저조도 down 2 더 낮게
    cmc623_I2cWrite16(0x00, 0x0000);    //BANK 0
    cmc623_I2cWrite16(0x01, 0x0040);    //algorithm selection
    cmc623_I2cWrite16(0x24, 0x0001);    
    cmc623_I2cWrite16(0x0b, 0x0184);    
    cmc623_I2cWrite16(0x12, 0x0000);
    cmc623_I2cWrite16(0x13, 0x0000);
    cmc623_I2cWrite16(0x14, 0x0000);
    cmc623_I2cWrite16(0x15, 0x0000);
    cmc623_I2cWrite16(0x16, 0x0000);
    cmc623_I2cWrite16(0x17, 0x0000);
    cmc623_I2cWrite16(0x18, 0x0000);
    cmc623_I2cWrite16(0x19, 0x0000);
    cmc623_I2cWrite16(0x0d, 0x1a08);    
    cmc623_I2cWrite16(0x0e, 0x0809);    
    cmc623_I2cWrite16(0x22, 0x0400);    
    cmc623_I2cWrite16(0x23, 0x0258);
    
    cmc623_I2cWrite16(0x2c, 0x003c);    //DNR 
    cmc623_I2cWrite16(0x2d, 0x0a08);    //DNR 
    cmc623_I2cWrite16(0x2e, 0x1010);    //DNR
    cmc623_I2cWrite16(0x2f, 0x0400);    //DNR
    cmc623_I2cWrite16(0x3a, 0x000C);    //HDTR DE, 
    cmc623_I2cWrite16(0x3b, 0x03ff);    //DE strength
    cmc623_I2cWrite16(0x42, 0x0001);    //max_Diff
    cmc623_I2cWrite16(0x49, 0x0400);    //PCC
    cmc623_I2cWrite16(0x4a, 0x7272);
    cmc623_I2cWrite16(0x4b, 0x8d8d);
    cmc623_I2cWrite16(0x4d, 0x01c0);
    cmc623_I2cWrite16(0xC8, 0x0000);    //scr
    cmc623_I2cWrite16(0xC9, 0x1000);
    cmc623_I2cWrite16(0xCA, 0xFFFF);
    cmc623_I2cWrite16(0xCB, 0xFFFF);
    cmc623_I2cWrite16(0xCC, 0x0000);
    cmc623_I2cWrite16(0xCD, 0xFFFF);
    cmc623_I2cWrite16(0xCE, 0x1000);
    cmc623_I2cWrite16(0xCF, 0xF0F0);
    cmc623_I2cWrite16(0xD0, 0x00FF);
    cmc623_I2cWrite16(0xD1, 0x00FF);
    cmc623_I2cWrite16(0xD2, 0x00FF);
    cmc623_I2cWrite16(0xD3, 0x00FF);
    
    cmc623_I2cWrite16(0x00, 0x0001);    //BANK 1
    cmc623_I2cWrite16(0x09, 0x0400);    
    cmc623_I2cWrite16(0x0a, 0x0258);
    cmc623_I2cWrite16(0x0b, 0x0400);
    cmc623_I2cWrite16(0x0c, 0x0258);
    cmc623_I2cWrite16(0x01, 0x0500);
    cmc623_I2cWrite16(0x06, 0x0074);
    cmc623_I2cWrite16(0x07, 0x2225);
    cmc623_I2cWrite16(0x65, 0x0008);
    cmc623_I2cWrite16(0x68, 0x0080);
    cmc623_I2cWrite16(0x6c, 0x0a28);
    cmc623_I2cWrite16(0x6d, 0x0b0a);
    cmc623_I2cWrite16(0x6e, 0xe1b3);
    
    cmc623_I2cWrite16(0x20, 0x0000);        //GAMMA 
    cmc623_I2cWrite16(0x21, 0x1800);
    cmc623_I2cWrite16(0x22, 0x1800);
    cmc623_I2cWrite16(0x23, 0x1800);
    cmc623_I2cWrite16(0x24, 0x1800);
    cmc623_I2cWrite16(0x25, 0x1800);
    cmc623_I2cWrite16(0x26, 0x1800);
    cmc623_I2cWrite16(0x27, 0x1800);
    cmc623_I2cWrite16(0x28, 0x1800);
    cmc623_I2cWrite16(0x29, 0x1800);
    cmc623_I2cWrite16(0x2A, 0x1800);
    cmc623_I2cWrite16(0x2B, 0x1800);    
    cmc623_I2cWrite16(0x2C, 0x1800);
    cmc623_I2cWrite16(0x2D, 0x1800);
    cmc623_I2cWrite16(0x2E, 0x1800);
    cmc623_I2cWrite16(0x2F, 0x1800);
    cmc623_I2cWrite16(0x30, 0x1800);
    cmc623_I2cWrite16(0x31, 0x9903);
    cmc623_I2cWrite16(0x32, 0x9c0a);
    cmc623_I2cWrite16(0x33, 0xa31c);
    cmc623_I2cWrite16(0x34, 0xa420);
    cmc623_I2cWrite16(0x35, 0xa420);
    cmc623_I2cWrite16(0x36, 0xa420);
    cmc623_I2cWrite16(0x37, 0xa420);
    cmc623_I2cWrite16(0x38, 0xFF00);
    cmc623_I2cWrite16(0x20, 0x0001);
    
    cmc623_I2cWrite16(0x00, 0x0000);
    cmc623_I2cWrite16(0x28, 0x0000);
    cmc623_I2cWrite16(0x09, 0x0000);
    cmc623_I2cWrite16(0x09, 0xffff);
    
    //delay 5ms
	mdelay(5);
    cmc623_I2cWrite16(0x26, 0x0001);
    
	klogi("===== cmc623_load_data START =====");
	// ret = cmc623_load_data();
	klogi("===== cmc623_load_data END =====");
	klogi("===== cmc623_test ret (%d)=====", ret);

	return ret;    
}
EXPORT_SYMBOL(cmc623_test);

int cmc623_gamma_set(void)  // P1_LSJ DE19
{
    int ret = 0;
    int k =0; 

    printk("**************************************\n");
    printk("**** < cmc623_gamma_set >        *****\n");
    printk("**************************************\n");

    if (gpio_is_valid(GPIO_CMC_BYPASS)) 
    {
        gpio_direction_output(GPIO_CMC_BYPASS, GPIO_LEVEL_LOW);
    }
    s3c_gpio_setpull(GPIO_CMC_BYPASS, S3C_GPIO_PULL_NONE);
    gpio_set_value(GPIO_CMC_BYPASS, GPIO_LEVEL_HIGH);
    mdelay(2);

    if (gpio_is_valid(GPIO_CMC_RST)) 
    {
        gpio_direction_output(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
    }
    s3c_gpio_setpull(GPIO_CMC_RST, S3C_GPIO_PULL_NONE);
    gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_LOW);
    mdelay(4);
    gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
    mdelay(5);

#if 0
	cmc623_I2cWrite16(0x00, 0x0000);    //BANK 0
	cmc623_I2cWrite16(0x01, 0x0040);    //algorithm selection
	cmc623_I2cWrite16(0x10, 0x001A);    // PCLK Polarity Sel
	cmc623_I2cWrite16(0x24, 0x0001);    // Polarity Sel    
	cmc623_I2cWrite16(0x0b, 0x0184);    // Clock Gating
	cmc623_I2cWrite16(0x12, 0x0000);    // Pad strength start
	cmc623_I2cWrite16(0x13, 0x0000);
	cmc623_I2cWrite16(0x14, 0x0000);
	cmc623_I2cWrite16(0x15, 0x0000);
	cmc623_I2cWrite16(0x16, 0x0000);
	cmc623_I2cWrite16(0x17, 0x0000);
	cmc623_I2cWrite16(0x18, 0x0000);
	cmc623_I2cWrite16(0x19, 0x0000);     // Pad strength end
	cmc623_I2cWrite16(0x0d, 0x1a0a);     // A-Stage clk
	cmc623_I2cWrite16(0x0e, 0x0a0b);     // B-stage clk    
	cmc623_I2cWrite16(0x22, 0x0400);     // H_Size
	cmc623_I2cWrite16(0x23, 0x0258);     // V_Size
	cmc623_I2cWrite16(0x2c, 0x0fff);    //DNR bypass
	cmc623_I2cWrite16(0x2e, 0x0000);    //DNR bypass
	cmc623_I2cWrite16(0x3a, 0x000c);    //HDTR on DE, 
	cmc623_I2cWrite16(0x3b, 0x03ff);    //DE strength
	cmc623_I2cWrite16(0x42, 0x0001);    //max_Diff
	cmc623_I2cWrite16(0x49, 0x0400);    //PCC
	cmc623_I2cWrite16(0x4a, 0x7272);
	cmc623_I2cWrite16(0x4b, 0x8d8d);
	cmc623_I2cWrite16(0x4d, 0x01c0);
	cmc623_I2cWrite16(0xC8, 0x0000);    //scr
	cmc623_I2cWrite16(0xC9, 0x1000);
	cmc623_I2cWrite16(0xCA, 0xFFFF);
	cmc623_I2cWrite16(0xCB, 0xFFFF);
	cmc623_I2cWrite16(0xCC, 0x0000);
	cmc623_I2cWrite16(0xCD, 0xFFFF);
	cmc623_I2cWrite16(0xCE, 0x1000);
	cmc623_I2cWrite16(0xCF, 0xF0F0);
	cmc623_I2cWrite16(0xD0, 0x00FF);
	cmc623_I2cWrite16(0xD1, 0x00FF);
	cmc623_I2cWrite16(0xD2, 0x00FF);
	cmc623_I2cWrite16(0xD3, 0x00FF);
	cmc623_I2cWrite16(0x00, 0x0001);    //BANK 1
	cmc623_I2cWrite16(0x09, 0x0400);    // H_Size
	cmc623_I2cWrite16(0x0a, 0x0258);    // V_Size
	cmc623_I2cWrite16(0x0b, 0x0400);    // H_Size
	cmc623_I2cWrite16(0x0c, 0x0258);    // V_Size
	cmc623_I2cWrite16(0x01, 0x0500);    // BF_Line
	cmc623_I2cWrite16(0x06, 0x0062);    // Refresh time
	cmc623_I2cWrite16(0x07, 0x2225);    // eDRAM
	cmc623_I2cWrite16(0x65, 0x0008);    // V_Sync cal.
	cmc623_I2cWrite16(0x68, 0x0080);    // TCON Polarity
	cmc623_I2cWrite16(0x6c, 0x0414);    // VLW,HLW
	cmc623_I2cWrite16(0x6d, 0x0506);    // VBP,VFP
	cmc623_I2cWrite16(0x6e, 0x1e32);    // HBP,HFP
	cmc623_I2cWrite16(0x20, 0x0000);		//GAMMA	11
	cmc623_I2cWrite16(0x21, 0x2000);
	cmc623_I2cWrite16(0x22, 0x2000);
	cmc623_I2cWrite16(0x23, 0x2000);
	cmc623_I2cWrite16(0x24, 0x2000);
	cmc623_I2cWrite16(0x25, 0x2000);
	cmc623_I2cWrite16(0x26, 0x2000);
	cmc623_I2cWrite16(0x27, 0x2000);
	cmc623_I2cWrite16(0x28, 0x2000);
	cmc623_I2cWrite16(0x29, 0x1f01);
	cmc623_I2cWrite16(0x2A, 0x1f01);
	cmc623_I2cWrite16(0x2B, 0x1f01);
	cmc623_I2cWrite16(0x2C, 0x1f01);
	cmc623_I2cWrite16(0x2D, 0x1f01);
	cmc623_I2cWrite16(0x2E, 0x1f01);
	cmc623_I2cWrite16(0x2F, 0x1f01);
	cmc623_I2cWrite16(0x30, 0x1f01);
	cmc623_I2cWrite16(0x31, 0x1f01);
	cmc623_I2cWrite16(0x32, 0x1f01);
	cmc623_I2cWrite16(0x33, 0x1f01);
	cmc623_I2cWrite16(0x34, 0xa20b);
	cmc623_I2cWrite16(0x35, 0xa20b);
	cmc623_I2cWrite16(0x36, 0x2000);
	cmc623_I2cWrite16(0x37, 0x1f08);
	cmc623_I2cWrite16(0x38, 0xFF00); 
	cmc623_I2cWrite16(0x20, 0x0001);    // GAMMA 11
	cmc623_I2cWrite16(0x00, 0x0000);
	cmc623_I2cWrite16(0x28, 0x0000);
	cmc623_I2cWrite16(0x09, 0x0000);
	cmc623_I2cWrite16(0x09, 0xffff);

	//delay 5ms
	mdelay(5);

	cmc623_I2cWrite16(0x26, 0x0001);
#else
    // add, value 
    cmc623_I2cWrite16(0x00, 0x0000);    //BANK 0
    cmc623_I2cWrite16(0x01, 0x0040);    //algorithm selection
    cmc623_I2cWrite16(0x24, 0x0001);    
    cmc623_I2cWrite16(0x0b, 0x0184);    
    cmc623_I2cWrite16(0x12, 0x0000);
    cmc623_I2cWrite16(0x13, 0x0000);
    cmc623_I2cWrite16(0x14, 0x0000);
    cmc623_I2cWrite16(0x15, 0x0000);
    cmc623_I2cWrite16(0x16, 0x0000);
    cmc623_I2cWrite16(0x17, 0x0000);
    cmc623_I2cWrite16(0x18, 0x0000);
    cmc623_I2cWrite16(0x19, 0x0000);
    cmc623_I2cWrite16(0x0d, 0x1a08);    
    cmc623_I2cWrite16(0x0e, 0x0809);    
    cmc623_I2cWrite16(0x22, 0x0400);    
    cmc623_I2cWrite16(0x23, 0x0258);
    
    cmc623_I2cWrite16(0x2c, 0x0fff);    //DNR bypass
    cmc623_I2cWrite16(0x2e, 0x0000);    //DNR bypass
    cmc623_I2cWrite16(0x3a, 0x0000);    //HDTR DE, 
    cmc623_I2cWrite16(0xC8, 0x0000);    //scr
    cmc623_I2cWrite16(0xC9, 0x2000);
    cmc623_I2cWrite16(0xCA, 0xFFFF);
    cmc623_I2cWrite16(0xCB, 0xFFFF);
    cmc623_I2cWrite16(0xCC, 0x0000);
    cmc623_I2cWrite16(0xCD, 0xFFFF);
    cmc623_I2cWrite16(0xCE, 0x1000);
    cmc623_I2cWrite16(0xCF, 0xECF0);
    cmc623_I2cWrite16(0xD0, 0x00FF);
    cmc623_I2cWrite16(0xD1, 0x00FF);
    cmc623_I2cWrite16(0xD2, 0x00FF);
    cmc623_I2cWrite16(0xD3, 0x00FF);
    
    cmc623_I2cWrite16(0x00, 0x0001);    //BANK 1
    cmc623_I2cWrite16(0x09, 0x0400);    
    cmc623_I2cWrite16(0x0a, 0x0258);
    cmc623_I2cWrite16(0x0b, 0x0400);
    cmc623_I2cWrite16(0x0c, 0x0258);
    cmc623_I2cWrite16(0x01, 0x0500);
    cmc623_I2cWrite16(0x06, 0x0074);
    cmc623_I2cWrite16(0x07, 0x2225);
    cmc623_I2cWrite16(0x65, 0x0008);
    cmc623_I2cWrite16(0x68, 0x0080);
	//
    cmc623_I2cWrite16(0x6c, 0x0414);
    cmc623_I2cWrite16(0x6d, 0x0506);
    cmc623_I2cWrite16(0x6e, 0x1e32);
//    cmc623_I2cWrite16(0x6c, 0x0a28);
//    cmc623_I2cWrite16(0x6d, 0x0b0a);
//    cmc623_I2cWrite16(0x6e, 0xe1b3);
    
    cmc623_I2cWrite16(0x20, 0x0000);    //GAMMA 
    cmc623_I2cWrite16(0x21, 0x1800);
    cmc623_I2cWrite16(0x22, 0x1800);
    cmc623_I2cWrite16(0x23, 0x1800);
    cmc623_I2cWrite16(0x24, 0x1800);
    cmc623_I2cWrite16(0x25, 0x1800);
    cmc623_I2cWrite16(0x26, 0x1800);
    cmc623_I2cWrite16(0x27, 0x1800);
    cmc623_I2cWrite16(0x28, 0x1800);
    cmc623_I2cWrite16(0x29, 0x1800);
    cmc623_I2cWrite16(0x2A, 0x1800);
    cmc623_I2cWrite16(0x2B, 0x1800);    
    cmc623_I2cWrite16(0x2C, 0x1800);
    cmc623_I2cWrite16(0x2D, 0x1800);
    cmc623_I2cWrite16(0x2E, 0x1800);
    cmc623_I2cWrite16(0x2F, 0x1800);
    cmc623_I2cWrite16(0x30, 0x1800);
    cmc623_I2cWrite16(0x31, 0x9903);
    cmc623_I2cWrite16(0x32, 0x9c0a);
    cmc623_I2cWrite16(0x33, 0xa31c);
    cmc623_I2cWrite16(0x34, 0xa420);
    cmc623_I2cWrite16(0x35, 0xa420);
    cmc623_I2cWrite16(0x36, 0xa420);
    cmc623_I2cWrite16(0x37, 0xa420);
    cmc623_I2cWrite16(0x38, 0xFF00);
    cmc623_I2cWrite16(0x20, 0x0001);
    
    cmc623_I2cWrite16(0x00, 0x0000);
    cmc623_I2cWrite16(0x28, 0x0000);
    cmc623_I2cWrite16(0x09, 0x0000);
    cmc623_I2cWrite16(0x09, 0xffff);
    
    //delay 5ms
    mdelay(5);
    cmc623_I2cWrite16(0x26, 0x0001);
#endif

	return ret;    
}
EXPORT_SYMBOL(cmc623_gamma_set);

int cmc623_gamma_set_DMB(void)  // P1_LSJ DE19
{
	int ret = 0;
    int k =0; 

    printk("**************************************\n");
    printk("**** < cmc623_gamma_set_DMB >    *****\n");
    printk("**************************************\n");
    
	if (gpio_is_valid(GPIO_CMC_BYPASS)) 
    {
		if (gpio_request(GPIO_CMC_BYPASS, "CMC_BYPASS"))
			printk(KERN_ERR "Filed to request GPIO_CMC_BYPASS!\n");
		gpio_direction_output(GPIO_CMC_BYPASS, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_CMC_BYPASS, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_BYPASS, GPIO_LEVEL_HIGH);
    mdelay(2);
    
	if (gpio_is_valid(GPIO_CMC_RST)) {
		if (gpio_request(GPIO_CMC_RST, "CMC_RST"))
			printk(KERN_ERR "Filed to request GPIO_CMC_RST!\n");
		gpio_direction_output(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_CMC_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_LOW);
    mdelay(4);
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
    mdelay(5);

    // add, value 
    cmc623_I2cWrite16(0x00, 0x0000);    //BANK 0
    cmc623_I2cWrite16(0x01, 0x0040);    //algorithm selection
    cmc623_I2cWrite16(0x24, 0x0001);    
    cmc623_I2cWrite16(0x0b, 0x0184);    
    cmc623_I2cWrite16(0x12, 0x0000);
    cmc623_I2cWrite16(0x13, 0x0000);
    cmc623_I2cWrite16(0x14, 0x0000);
    cmc623_I2cWrite16(0x15, 0x0000);
    cmc623_I2cWrite16(0x16, 0x0000);
    cmc623_I2cWrite16(0x17, 0x0000);
    cmc623_I2cWrite16(0x18, 0x0000);
    cmc623_I2cWrite16(0x19, 0x0000);
    cmc623_I2cWrite16(0x0d, 0x1a08);    
    cmc623_I2cWrite16(0x0e, 0x0809);    
    cmc623_I2cWrite16(0x22, 0x0400);    
    cmc623_I2cWrite16(0x23, 0x0258);
    
    cmc623_I2cWrite16(0x2c, 0x003c);    //DNR 
    cmc623_I2cWrite16(0x2d, 0x0a08);    //DNR 
    cmc623_I2cWrite16(0x2e, 0x1010);    //DNR
    cmc623_I2cWrite16(0x2f, 0x0400);    //DNR
    cmc623_I2cWrite16(0x3a, 0x000c);    //HDTR on DE, 
    cmc623_I2cWrite16(0x3b, 0x03ff);    //DE strength
    cmc623_I2cWrite16(0x42, 0x0001);    //max_Diff
    cmc623_I2cWrite16(0x49, 0x0400);    //PCC
    cmc623_I2cWrite16(0x4a, 0x7272);
    cmc623_I2cWrite16(0x4b, 0x8d8d);
    cmc623_I2cWrite16(0x4d, 0x01c0);
    cmc623_I2cWrite16(0xC8, 0x0000);    //scr
    cmc623_I2cWrite16(0xC9, 0x1000);
    cmc623_I2cWrite16(0xCA, 0xFFFF);
    cmc623_I2cWrite16(0xCB, 0xFFFF);
    cmc623_I2cWrite16(0xCC, 0x0000);
    cmc623_I2cWrite16(0xCD, 0xFFFF);
    cmc623_I2cWrite16(0xCE, 0x1000);
    cmc623_I2cWrite16(0xCF, 0xF0F0);
    cmc623_I2cWrite16(0xD0, 0x00FF);
    cmc623_I2cWrite16(0xD1, 0x00FF);
    cmc623_I2cWrite16(0xD2, 0x00FF);
    cmc623_I2cWrite16(0xD3, 0x00FF);
    
    cmc623_I2cWrite16(0x00, 0x0001);    //BANK 1
    cmc623_I2cWrite16(0x09, 0x0400);    
    cmc623_I2cWrite16(0x0a, 0x0258);
    cmc623_I2cWrite16(0x0b, 0x0400);
    cmc623_I2cWrite16(0x0c, 0x0258);
    cmc623_I2cWrite16(0x01, 0x0500);
    cmc623_I2cWrite16(0x06, 0x0074);
    cmc623_I2cWrite16(0x07, 0x2225);
    cmc623_I2cWrite16(0x65, 0x0008);
    cmc623_I2cWrite16(0x68, 0x0080);
    cmc623_I2cWrite16(0x6c, 0x0a28);
    cmc623_I2cWrite16(0x6d, 0x0b0a);
    cmc623_I2cWrite16(0x6e, 0xe1b3);
    
    cmc623_I2cWrite16(0x20, 0x0000);    //GAMMA 
    cmc623_I2cWrite16(0x21, 0x1800);
    cmc623_I2cWrite16(0x22, 0x1800);
    cmc623_I2cWrite16(0x23, 0x1800);
    cmc623_I2cWrite16(0x24, 0x1800);
    cmc623_I2cWrite16(0x25, 0x1800);
    cmc623_I2cWrite16(0x26, 0x1800);
    cmc623_I2cWrite16(0x27, 0x1800);
    cmc623_I2cWrite16(0x28, 0x1800);
    cmc623_I2cWrite16(0x29, 0x1800);
    cmc623_I2cWrite16(0x2A, 0x1800);
    cmc623_I2cWrite16(0x2B, 0x1800);    
    cmc623_I2cWrite16(0x2C, 0x1800);
    cmc623_I2cWrite16(0x2D, 0x1800);
    cmc623_I2cWrite16(0x2E, 0x1800);
    cmc623_I2cWrite16(0x2F, 0x1800);
    cmc623_I2cWrite16(0x30, 0x1800);
    cmc623_I2cWrite16(0x31, 0x9903);
    cmc623_I2cWrite16(0x32, 0x9c0a);
    cmc623_I2cWrite16(0x33, 0xa31c);
    cmc623_I2cWrite16(0x34, 0xa420);
    cmc623_I2cWrite16(0x35, 0xa420);
    cmc623_I2cWrite16(0x36, 0xa420);
    cmc623_I2cWrite16(0x37, 0xa420);
    cmc623_I2cWrite16(0x38, 0xFF00);
    cmc623_I2cWrite16(0x20, 0x0001);
    
    cmc623_I2cWrite16(0x00, 0x0000);
    cmc623_I2cWrite16(0x28, 0x0000);
    cmc623_I2cWrite16(0x09, 0x0000);
    cmc623_I2cWrite16(0x09, 0xffff);
    
    //delay 5ms
    mdelay(5);
    cmc623_I2cWrite16(0x26, 0x0001);
    
	return ret;    
}
EXPORT_SYMBOL(cmc623_gamma_set_DMB);

int cmc623_DMB_test(void) // P1_LSJ : DE11
{
	int ret = 0;

	klogi("===== cmc623_DMBtest =====");

	if (gpio_is_valid(GPIO_CMC_BYPASS)) {
		if (gpio_request(GPIO_CMC_BYPASS, "CMC_BYPASS"))
			printk(KERN_ERR "Filed to request GPIO_CMC_BYPASS!\n");
		gpio_direction_output(GPIO_CMC_BYPASS, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_CMC_BYPASS, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_BYPASS, GPIO_LEVEL_HIGH);
    mdelay(2);
    
	if (gpio_is_valid(GPIO_CMC_RST)) {
		if (gpio_request(GPIO_CMC_RST, "CMC_RST"))
			printk(KERN_ERR "Filed to request GPIO_CMC_RST!\n");
		gpio_direction_output(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_CMC_RST, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_LOW);
    mdelay(4);
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
	mdelay(5);

    // a17  mdelay(5);  
    // add, value 
    cmc623_I2cWrite16(0x00, 0x0000);    //BANK 0
    cmc623_I2cWrite16(0x01, 0x0040);    //algorithm selection
    cmc623_I2cWrite16(0x24, 0x0001);    
    cmc623_I2cWrite16(0x0b, 0x0184);    
    cmc623_I2cWrite16(0x12, 0x0000);
    cmc623_I2cWrite16(0x13, 0x0000);
    cmc623_I2cWrite16(0x14, 0x0000);
    cmc623_I2cWrite16(0x15, 0x0000);
    cmc623_I2cWrite16(0x16, 0x0000);
    cmc623_I2cWrite16(0x17, 0x0000);
    cmc623_I2cWrite16(0x18, 0x0000);
    cmc623_I2cWrite16(0x19, 0x0000);
    cmc623_I2cWrite16(0x0d, 0x1a08);    
    cmc623_I2cWrite16(0x0e, 0x0809);    
    cmc623_I2cWrite16(0x22, 0x0400);    
    cmc623_I2cWrite16(0x23, 0x0258);
    
    cmc623_I2cWrite16(0x2c, 0x0fff);    //DNR bypass
    cmc623_I2cWrite16(0x2e, 0x0000);    //DNR bypass
    cmc623_I2cWrite16(0x3a, 0x0000);    //HDTR DE, 
    cmc623_I2cWrite16(0xC8, 0x0000);    //scr
    cmc623_I2cWrite16(0xC9, 0x1800);
    cmc623_I2cWrite16(0xCA, 0xFFFF);
    cmc623_I2cWrite16(0xCB, 0xFFFF);
    cmc623_I2cWrite16(0xCC, 0x0000);
    cmc623_I2cWrite16(0xCD, 0xFFFF);
    cmc623_I2cWrite16(0xCE, 0x1000);
    cmc623_I2cWrite16(0xCF, 0xF0F0);
    cmc623_I2cWrite16(0xD0, 0x00FF);
    cmc623_I2cWrite16(0xD1, 0x00FF);
    cmc623_I2cWrite16(0xD2, 0x00FF);
    cmc623_I2cWrite16(0xD3, 0x00FF);
    
    cmc623_I2cWrite16(0x00, 0x0001);    //BANK 1
    cmc623_I2cWrite16(0x09, 0x0400);    
    cmc623_I2cWrite16(0x0a, 0x0258);
    cmc623_I2cWrite16(0x0b, 0x0400);
    cmc623_I2cWrite16(0x0c, 0x0258);
    cmc623_I2cWrite16(0x01, 0x0500);
    cmc623_I2cWrite16(0x06, 0x0074);
    cmc623_I2cWrite16(0x07, 0x2225);
    cmc623_I2cWrite16(0x65, 0x0008);
    cmc623_I2cWrite16(0x68, 0x0080);
    cmc623_I2cWrite16(0x6c, 0x0a28);
    cmc623_I2cWrite16(0x6d, 0x0b0a);
    cmc623_I2cWrite16(0x6e, 0xe1b3);
    
    cmc623_I2cWrite16(0x20, 0x0000);        //GAMMA 
    cmc623_I2cWrite16(0x21, 0x0100);
    cmc623_I2cWrite16(0x22, 0x0100);
    cmc623_I2cWrite16(0x23, 0x0100);
    cmc623_I2cWrite16(0x24, 0x0100);
    cmc623_I2cWrite16(0x25, 0x0100);
    cmc623_I2cWrite16(0x26, 0x0100);
    cmc623_I2cWrite16(0x27, 0x0100);
    cmc623_I2cWrite16(0x28, 0x0100);
    cmc623_I2cWrite16(0x29, 0xa318);
    cmc623_I2cWrite16(0x2A, 0xa318);
    cmc623_I2cWrite16(0x2B, 0xa318);    
    cmc623_I2cWrite16(0x2C, 0xa318);
    cmc623_I2cWrite16(0x2D, 0xa318);
    cmc623_I2cWrite16(0x2E, 0xa318);
    cmc623_I2cWrite16(0x2F, 0xa318);
    cmc623_I2cWrite16(0x30, 0xa318);
    cmc623_I2cWrite16(0x31, 0xa318);
    cmc623_I2cWrite16(0x32, 0xa318);
    cmc623_I2cWrite16(0x33, 0xa318);
    cmc623_I2cWrite16(0x34, 0xa318);
    cmc623_I2cWrite16(0x35, 0xa318);
    cmc623_I2cWrite16(0x36, 0xa318);
    cmc623_I2cWrite16(0x37, 0xa318);
    cmc623_I2cWrite16(0x38, 0xFF00);
    cmc623_I2cWrite16(0x20, 0x0001);
    
    cmc623_I2cWrite16(0x00, 0x0000);
    cmc623_I2cWrite16(0x28, 0x0000);
    cmc623_I2cWrite16(0x09, 0x0000);
    cmc623_I2cWrite16(0x09, 0xffff);
    
    //delay 5ms
	mdelay(5);
    cmc623_I2cWrite16(0x26, 0x0001);

	// ret = cmc623_load_data();
	klogi("===== cmc623_DMB_test ret (%d)=====", ret);
	return ret;    
}
EXPORT_SYMBOL(cmc623_DMB_test);

static int cmc623_tuning_set (void)  // P1_LSJ DE19
{
    int ret = 0;
    int k =0; 

    printk("**************************************\n");
    printk("**** < cmc623_tuning_set >       *****\n");
    printk("**************************************\n");

	cmc623_I2cWrite16(0x00, 0x0000);    //BANK 0
	cmc623_I2cWrite16(0x01, 0x0070);    //algorithm selection
	cmc623_I2cWrite16(0xb4, 0x4400);    //PWM ratio
	cmc623_I2cWrite16(0xb3, 0xffff);    //up/down step
	cmc623_I2cWrite16(0x10, 0x001A);    // PCLK Polarity Sel
	cmc623_I2cWrite16(0x24, 0x0001);    // Polarity Sel    
	cmc623_I2cWrite16(0x0b, 0x0184);    // Clock Gating
	cmc623_I2cWrite16(0x12, 0x0000);    // Pad strength start
	cmc623_I2cWrite16(0x13, 0x0000);
	cmc623_I2cWrite16(0x14, 0x0000);
	cmc623_I2cWrite16(0x15, 0x0000);
	cmc623_I2cWrite16(0x16, 0x0000);
	cmc623_I2cWrite16(0x17, 0x0000);
	cmc623_I2cWrite16(0x18, 0x0000);
	cmc623_I2cWrite16(0x19, 0x0000);     // Pad strength end
	cmc623_I2cWrite16(0x0f, 0x0010);     // PWM clock ratio
	cmc623_I2cWrite16(0x0d, 0x1a0a);     // A-Stage clk
	cmc623_I2cWrite16(0x0e, 0x0a0b);     // B-stage clk    
	cmc623_I2cWrite16(0x22, 0x0400);     // H_Size
	cmc623_I2cWrite16(0x23, 0x0258);     // V_Size
	cmc623_I2cWrite16(0x2c, 0x0fff);    //DNR bypass
	cmc623_I2cWrite16(0x2e, 0x0000);    //DNR bypass
	cmc623_I2cWrite16(0x3a, 0x000c);    //HDTR on DE, 
	cmc623_I2cWrite16(0x3b, 0x03ff);    //DE strength
	cmc623_I2cWrite16(0x42, 0x0001);    //max_Diff
	cmc623_I2cWrite16(0x49, 0x0400);    //PCC
	cmc623_I2cWrite16(0x4a, 0x7272);
	cmc623_I2cWrite16(0x4b, 0x8d8d);
	cmc623_I2cWrite16(0x4d, 0x01c0);
	cmc623_I2cWrite16(0xC8, 0x0000);    //scr
	cmc623_I2cWrite16(0xC9, 0x1000);
	cmc623_I2cWrite16(0xCA, 0xFFFF);
	cmc623_I2cWrite16(0xCB, 0xFFFF);
	cmc623_I2cWrite16(0xCC, 0x0000);
	cmc623_I2cWrite16(0xCD, 0xFFFF);
	cmc623_I2cWrite16(0xCE, 0x1000);
	cmc623_I2cWrite16(0xCF, 0xF0F0);
	cmc623_I2cWrite16(0xD0, 0x00FF);
	cmc623_I2cWrite16(0xD1, 0x00FF);
	cmc623_I2cWrite16(0xD2, 0x00FF);
	cmc623_I2cWrite16(0xD3, 0x00FF);
	cmc623_I2cWrite16(0x00, 0x0001);    //BANK 1
	cmc623_I2cWrite16(0x09, 0x0400);    // H_Size
	cmc623_I2cWrite16(0x0a, 0x0258);    // V_Size
	cmc623_I2cWrite16(0x0b, 0x0400);    // H_Size
	cmc623_I2cWrite16(0x0c, 0x0258);    // V_Size
	cmc623_I2cWrite16(0x01, 0x0500);    // BF_Line
	cmc623_I2cWrite16(0x06, 0x0062);    // Refresh time
	cmc623_I2cWrite16(0x07, 0x2225);    // eDRAM
	cmc623_I2cWrite16(0x65, 0x0008);    // V_Sync cal.
	cmc623_I2cWrite16(0x68, 0x0080);    // TCON Polarity
	cmc623_I2cWrite16(0x6c, 0x0414);    // VLW,HLW
	cmc623_I2cWrite16(0x6d, 0x0506);    // VBP,VFP
	cmc623_I2cWrite16(0x6e, 0x1e32);    // HBP,HFP
	cmc623_I2cWrite16(0x20, 0x0000);		//GAMMA	11
	cmc623_I2cWrite16(0x21, 0x2000);
	cmc623_I2cWrite16(0x22, 0x2000);
	cmc623_I2cWrite16(0x23, 0x2000);
	cmc623_I2cWrite16(0x24, 0x2000);
	cmc623_I2cWrite16(0x25, 0x2000);
	cmc623_I2cWrite16(0x26, 0x2000);
	cmc623_I2cWrite16(0x27, 0x2000);
	cmc623_I2cWrite16(0x28, 0x2000);
	cmc623_I2cWrite16(0x29, 0x1f01);
	cmc623_I2cWrite16(0x2A, 0x1f01);
	cmc623_I2cWrite16(0x2B, 0x1f01);
	cmc623_I2cWrite16(0x2C, 0x1f01);
	cmc623_I2cWrite16(0x2D, 0x1f01);
	cmc623_I2cWrite16(0x2E, 0x1f01);
	cmc623_I2cWrite16(0x2F, 0x1f01);
	cmc623_I2cWrite16(0x30, 0x1f01);
	cmc623_I2cWrite16(0x31, 0x1f01);
	cmc623_I2cWrite16(0x32, 0x1f01);
	cmc623_I2cWrite16(0x33, 0x1f01);
	cmc623_I2cWrite16(0x34, 0xa20b);
	cmc623_I2cWrite16(0x35, 0xa20b);
	cmc623_I2cWrite16(0x36, 0x2000);
	cmc623_I2cWrite16(0x37, 0x1f08);
	cmc623_I2cWrite16(0x38, 0xFF00); 
	cmc623_I2cWrite16(0x20, 0x0001);    // GAMMA 11
	cmc623_I2cWrite16(0x00, 0x0000);
	cmc623_I2cWrite16(0x28, 0x0000);
	cmc623_I2cWrite16(0x09, 0x0000);
	cmc623_I2cWrite16(0x09, 0xffff);

	//delay 5ms
	msleep(5);

	cmc623_I2cWrite16(0x26, 0x0001);

    printk("**** < end cmc623_tuning_set >       *****\n");

	return ret;    
}

// value: 0 ~ 1600
void tune_cmc623_pwm_brightness(int value)
{
	u32 reg, data;

	if(!p_cmc623_data)
		{
		printk(KERN_ERR "%s cmc623 is not initialized\n", __func__);
		}

	if(value<0)
		data = 0;
	else if(value>1600)
		data = 1600;
	else
		data = value;

	reg = 0x4000 | data;

	cmc623_I2cWrite16(0x00, 0x0000);		//bank0
	cmc623_I2cWrite16(0xB4, reg);			//pwn duty
	cmc623_I2cWrite16(0x28, 0x0000);
}
EXPORT_SYMBOL(tune_cmc623_pwm_brightness);

static int tune_cmc623_hw_rst()
{
    if (gpio_is_valid(GPIO_CMC_BYPASS)) 
    {
        gpio_direction_output(GPIO_CMC_BYPASS, GPIO_LEVEL_LOW);
    }
    s3c_gpio_setpull(GPIO_CMC_BYPASS, S3C_GPIO_PULL_NONE);
    gpio_set_value(GPIO_CMC_BYPASS, GPIO_LEVEL_HIGH);
    msleep(2);

    if (gpio_is_valid(GPIO_CMC_RST)) 
    {
        gpio_direction_output(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
    }
    s3c_gpio_setpull(GPIO_CMC_RST, S3C_GPIO_PULL_NONE);
    gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_LOW);
    msleep(4);
    gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_HIGH);
    msleep(5);

	return 0;
}

int tune_cmc623_suspend()
{
	printk(KERN_INFO "%s called\n", __func__);

	if(!p_cmc623_data)
		{
		printk(KERN_ERR "%s cmc623 is not initialized\n", __func__);
		return 0;
		}

	// 1.2V/1.8V/3.3V may be on

	// CMC623[0x07] := 0x0004
    cmc623_I2cWrite16(0x07, 0x0004);

	// SLEEPB(L6) <= LOW
	gpio_set_value(GPIO_CMC_SLEEP, GPIO_LEVEL_LOW);

	// BYPASSB(A2) <= LOW
	gpio_set_value(GPIO_CMC_BYPASS, GPIO_LEVEL_LOW);

	// wait 1ms
	msleep(1);

	// FAILSAFEB(E6) <= LOW
	gpio_set_value(GPIO_CMC_SHDN, GPIO_LEVEL_LOW);

	// 1.2V/3.3V off 
	gpio_set_value(GPIO_CMC_EN, GPIO_LEVEL_LOW);

	// FAILSAFEB(E6) <= HIGH
	gpio_set_value(GPIO_CMC_SHDN, GPIO_LEVEL_HIGH);

	// 1.8V off (optional, but all io lines shoud be all low or all high or all high-z while sleep for preventing leakage current)
	
	// Reset chip
	//gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_LOW);
    //mdelay(4);

	return 0;
}
EXPORT_SYMBOL(tune_cmc623_suspend);

int tune_cmc623_resume()
{
	printk(KERN_INFO "%s called\n", __func__);
		
	if(!p_cmc623_data)
		{
		printk(KERN_ERR "%s cmc623 is not initialized\n", __func__);
		return 0;
		}

	// 1.2V/1.8V/3.3V may be off

	// 1.2V/1.8V/3.3V on
	gpio_set_value(GPIO_CMC_EN, GPIO_LEVEL_HIGH);

	// FAILSAFEB(E6) <= HIGH            
	gpio_set_value(GPIO_CMC_SHDN, GPIO_LEVEL_HIGH);

	// SLEEPB(L6) <= HIGH
	gpio_set_value(GPIO_CMC_SLEEP, GPIO_LEVEL_HIGH);

	// BYPASSB(A2) <= HIGH
	gpio_set_value(GPIO_CMC_BYPASS, GPIO_LEVEL_HIGH);

	// RESETB(K6) <= HIGH
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_HIGH);

	// wait 0.1ms or above
	udelay(100);

	// RESETB(K6) <= LOW
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_LOW);

	// wait 4ms or above
	msleep(4);

	// RESETB(K6) <= HIGH
	gpio_set_value(GPIO_CMC_RST, GPIO_LEVEL_HIGH);

	// wait 0.3ms or above
	udelay(300);

	// set registers using I2C
	cmc623_tuning_set();
	
	return 0;
}
EXPORT_SYMBOL(tune_cmc623_resume);

static ssize_t tune_cmc623_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret=0;

#ifdef CMC623_TUNING
	klogi("");
	ret = cmc623_load_data();
#endif

	if(ret<0)
    {   
        return sprintf(buf, "FAIL\n");
    }
    else
    {   
        return sprintf(buf, "OK\n");
    }
}

static ssize_t tune_cmc623_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(tune, S_IRUGO | S_IWUSR, tune_cmc623_show, tune_cmc623_store);

static ssize_t register_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret=0;

    return sprintf(buf, "OK\n");
}

static ssize_t register_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	int ret;
	u32 data1, data2;
	
	printk("[cmc623] %s : %s\n", __func__, buf);
	ret = sscanf(buf, "0x%x 0x%x\n", &data1, &data2);
	if(ret == 2)
		{
		printk("addr:0x%04x, data:0x%04x\n", data1, data2);
		cmc623_I2cWrite16(data1, data2);
		}
	else
		{
		printk("parse error num:%d, data:0x%04x, data:0x%04x\n", ret, data1, data2);
		}

	return size;
}

static DEVICE_ATTR(register, S_IRUGO | S_IWUSR, register_show, register_store);

/*
static int cmc623_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;

	printk("==============================\n");
	printk("cmc623 attach START!!!        \n");
	printk("==============================\n");

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
    {
		return -ENOMEM;
    }    

	memset(c, 0, sizeof(struct i2c_client));
	// strncpy(c->name, cmc623_i2c_driver.driver.name, I2C_NAME_SIZE);
	strncpy(c->name, "sec_tune_cmc623_i2c" , I2C_NAME_SIZE);
    
	c->addr = addr;
	c->adapter = adap;
	// c->driver = &cmc623_i2c_driver;
	c->driver = &sec_tune_cmc623_i2c_driver;  // P1_LSJ : DE07 

	ret = i2c_attach_client(c);
	if (ret < 0) 
    {
		printk("cmc623 attach failed!!! (%d)\n", ret);
		return ret;
	}

	printk("cmc623 attach success!!!\n");

	cmc623_i2c_client = c;
	return 0;
}
*/

static int cmc623_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cmc623_data *data;
//	struct i2c_client *c;
	int ret = 0;

	printk("==============================\n");
	printk("cmc623 attach START!!!        \n");
	printk("==============================\n");

	data = kzalloc(sizeof(struct cmc623_data), GFP_KERNEL);
	if (!data)
    {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
    }    

	data->client = client;
	i2c_set_clientdata(client, data);

	dev_info(&client->dev, "cmc623 i2c probe success!!!\n");

	p_cmc623_data = data;

	tune_cmc623_hw_rst();
	cmc623_tuning_set();
	
//	ret = cmc623_gamma_set();                             // P1_LSJ : DE19
    printk("cmc623_gamma_set Return value  (%d)\n", ret);

//	cmc623_i2c_client = c;
	return 0;
}

static int __devexit cmc623_i2c_remove(struct i2c_client *client)
{
	struct cmc623_data *data = i2c_get_clientdata(client);

	p_cmc623_data = NULL;

	i2c_set_clientdata(client, NULL);

	kfree(data);

	dev_info(&client->dev, "cmc623 i2c remove success!!!\n");

	return 0;
}

static const struct i2c_device_id sec_tune_cmc623_ids[] = {
	{ "sec_tune_cmc623_i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sec_tune_cmc623_ids);

struct i2c_driver sec_tune_cmc623_i2c_driver =  
{
	.driver	= {
		.name	= "sec_tune_cmc623_i2c",
        .owner = THIS_MODULE,
	},
	.probe 		= cmc623_i2c_probe,
	.remove 	= __devexit_p(cmc623_i2c_remove),
	.id_table	= sec_tune_cmc623_ids,
};

extern struct class *sec_class;
struct device *tune_cmc623_dev;

static int __devinit cmc623_probe(struct platform_device *pdev)
{
	int ret=0;

    printk("**** < cmc623_probe >      ***********\n");

	tune_cmc623_dev = device_create(sec_class, NULL, 0, NULL, "sec_tune_cmc623");

	if (IS_ERR(tune_cmc623_dev)) 
    {
		printk("Failed to create device!");
		ret = -1;
	}
	if (device_create_file(tune_cmc623_dev, &dev_attr_tune) < 0) {
		printk("Failed to create device file!(%s)!\n", dev_attr_tune.attr.name);
		ret = -1;
	}
	if (device_create_file(tune_cmc623_dev, &dev_attr_register) < 0) {
		printk("Failed to create device file!(%s)!\n", dev_attr_register.attr.name);
		ret = -1;
	}
	
    printk("<sec_tune_cmc623_i2c_driver Add START> \n");
    ret = i2c_add_driver(&sec_tune_cmc623_i2c_driver);    // P1_LSJ : DE07 
    printk("cmc623_init Return value  (%d)\n", ret);

//	ret = cmc623_gamma_set();                             // P1_LSJ : DE19
//    printk("cmc623_gamma_set Return value  (%d)\n", ret);
    printk("<sec_tune_cmc623_i2c_driver Add END>   \n");

	return ret;
}

static int __devexit cmc623_remove(struct platform_device *pdev)
{
	i2c_del_driver(&sec_tune_cmc623_i2c_driver);

	return 0;
}

struct platform_driver sec_tune_cmc623 =  {
	.driver = {
		.name = "sec_tune_cmc623",
        .owner  = THIS_MODULE,
	},
    .probe  = cmc623_probe,
    .remove = cmc623_remove,
};

/*
static int cmc623_attach_adapter(struct i2c_adapter *adap)
{
	printk("%s\n", __func__);
	return i2c_probe(adap, &cmc623_addr_data, cmc623_attach);  // P1_LSJ : DE07
}
*/

static int __init cmc623_init(void)
{
	int ret=0;

    #if 0 // P1_LSJ : DE04   // dgahn.test: turn off AP LED
	klogi("***** System LED turned off");
	NvOdmServicesGpioHandle hGpio;
	NvOdmGpioPinHandle hLEDApGpioPin;

	hGpio = NvOdmGpioOpen();
	hLEDApGpioPin = NvOdmGpioAcquirePinHandle(hGpio, GPIO_PORT_LEDAP, GPIO_PIN_LEDAP);

	NvOdmGpioSetState(hGpio, hLEDApGpioPin, 0x0);  // set low
	NvOdmGpioConfig(hGpio, hLEDApGpioPin, NvOdmGpioPinMode_Output);

	NvOdmGpioClose(hGpio);
    #endif

    printk("**************************************\n");
    printk("**** < cmc623_init  >               **\n");
    printk("**************************************\n");

	return platform_driver_register(&sec_tune_cmc623);
}

static void __exit cmc623_exit(void)
{
	platform_driver_unregister(&sec_tune_cmc623);
}

// module_init(cmc623_init);
module_init(cmc623_init);
module_exit(cmc623_exit);

/* Module information */
MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Tuning CMC623 image converter");
MODULE_LICENSE("GPL");
