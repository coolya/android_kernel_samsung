#ifndef __KR3DH_H__
#define __KR3DH_H__

#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
/* to debugg kr3dh  uncomment below macro*/
//#define KR3DH_DEBUG 

/* KR3DH I2C Address */
#define	SENS_ADD		(0x19 << 1)
#define DEBUG			1
#define I2C_M_WR		0x00
#define I2C_DF_NOTIFY		0x01


/* They used to connect i2c_acc_kr3dh_write. */
#define KR3DH_WR_FUNC_PTR char (* kr3dh_bus_write)(unsigned char, unsigned char *)

#define KR3DH_BUS_WRITE_FUNC(dev_addr, reg_addr, reg_data, len)\
          kr3dh_bus_write(reg_addr, reg_data)


/* They used to connect i2c_acc_kr3dh_read. */
#define KR3DH_RD_FUNC_PTR char (* kr3dh_bus_read)( unsigned char, unsigned char *, unsigned int)

#define KR3DH_BUS_READ_FUNC( dev_addr, reg_addr, reg_data, r_len )\
           kr3dh_bus_read( reg_addr , reg_data, r_len )


typedef struct  {
		short x, /**< holds x-axis acceleration data sign extended. Range -512 to 511. */
			  y, /**< holds y-axis acceleration data sign extended. Range -512 to 511. */
			  z; /**< holds z-axis acceleration data sign extended. Range -512 to 511. */
} kr3dhacc_t;

/* KR3DH IOCTL */
#define KR3DH_IOC_MAGIC 				'B'
#define KR3DH_SET_RANGE            	_IOWR(KR3DH_IOC_MAGIC,4, unsigned char)
#define KR3DH_SET_MODE             	_IOWR(KR3DH_IOC_MAGIC,6, unsigned char)
#define KR3DH_SET_BANDWIDTH            _IOWR(KR3DH_IOC_MAGIC,8, unsigned char)
#define KR3DH_READ_ACCEL_XYZ           _IOWR(KR3DH_IOC_MAGIC,46,kr3dhacc_t)
#define KR3DH_IOC_MAXNR            	48


/* KR3DH API error codes */

#define E_KR3DH_NULL_PTR		(char)-127
#define E_COMM_RES		    (char)-1
#define E_OUT_OF_RANGE		(char)-2



/* register write and read delays */

#define MDELAY_DATA_TYPE	unsigned int
#define KR3DH_EE_W_DELAY 28	/* delay after EEP write is 28 msec */

typedef struct  {
		unsigned char
		who_am_i,
		ctrl_reg1,
		ctrl_reg2,
		ctrl_reg3,
		ctrl_reg4,
		ctrl_reg5,
		hp_filter_reset,
		statur_reg,
		out_x,
		out_y,
		out_z,
		int1_cfg,
		int1_src,
		int1_ths,
		int1_duration,
		int2_cfg,
		int2_src,
		int2_ths,
		int2_duration;
} kr3dhregs_t;



typedef struct {
	kr3dhregs_t * image;		/**< pointer to kr3dhregs_t structure not mandatory */
	unsigned char mode;			/**< save current KR3DH operation mode */
	unsigned char dev_addr;   	/**< initializes KR3DH's I2C device address 0x10 */
	unsigned char int_mask;	  	/**< stores the current KR3DH API generated interrupt mask */
	KR3DH_WR_FUNC_PTR;		  	/**< function pointer to the SPI/I2C write function */
	KR3DH_RD_FUNC_PTR;		  	/**< function pointer to the SPI/I2C read function */
	void (*delay_msec)( MDELAY_DATA_TYPE ); /**< function pointer to a pause in mili seconds function */
	struct early_suspend early_suspend;		/**< suspend function */
} kr3dh_t;


#define WHO_AM_I		0x0F
#define CTRL_REG1		0x20
#define CTRL_REG2		0x21
#define CTRL_REG3		0x22
#define CTRL_REG4		0x23
#define CTRL_REG5		0x24
#define HP_FILTER_RESET	0x25
#define STATUS_REG		0x27
#define OUT_X_L			0x29
#define OUT_X_H			0x2A
#define OUT_Y_L			0x2B
#define OUT_Y_H			0x2C
#define OUT_Z_L			0x2D
#define OUT_Z_H			0x2E
#define INT1_CFG		0x30
#define INT1_SOURCE		0x31
#define INT1_THS		0x32
#define INT1_DURATION	0x33
#define INT2_CFG		0x34
#define INT2_SOURCE		0x35
#define INT2_THS		0x36
#define INT2_DURATION	0x37


/* range and bandwidth */

#define KR3DH_RANGE_2G			0 /**< sets range to 2G mode \see kr3dh_set_range() */
#define KR3DH_RANGE_4G			1 /**< sets range to 4G mode \see kr3dh_set_range() */
#define KR3DH_RANGE_8G			2 /**< sets range to 8G mode \see kr3dh_set_range() */

#define KR3DH_BW_50HZ			0 /**< sets bandwidth to 50HZ \see kr3dh_set_bandwidth() */
#define KR3DH_BW_100HZ			1 /**< sets bandwidth to 100HZ \see kr3dh_set_bandwidth() */
#define KR3DH_BW_400HZ			2 /**< sets bandwidth to 400HZ \see kr3dh_set_bandwidth() */

/* mode settings */

#define KR3DH_MODE_NORMAL      0
#define KR3DH_MODE_SLEEP       2
#define KR3DH_MODE_WAKE_UP     3


/* Function prototypes */

int kr3dh_init(kr3dh_t *);
static int kr3dh_accelerometer_resume( struct platform_device* );
static int kr3dh_accelerometer_suspend( struct platform_device* , pm_message_t );
static int kr3dh_accelerometer_probe( struct platform_device*);
int kr3dh_acc_start(void);
int kr3dh_read_accel_xyz(kr3dhacc_t *);

char  i2c_acc_kr3dh_read (u8, u8 *, unsigned int);
char  i2c_acc_kr3dh_write(u8 , u8 *);
int  i2c_acc_kr3dh_init(void);
void i2c_acc_kr3dh_exit(void);

int kr3dh_open (struct inode *, struct file *);
ssize_t kr3dh_read(struct file *, char *, size_t , loff_t *);
ssize_t kr3dh_write (struct file *, const char *, size_t , loff_t *);
int kr3dh_release (struct inode *, struct file *);
int kr3dh_ioctl(struct inode *, struct file *, unsigned int ,  unsigned long );
static void kr3dh_early_suspend(struct early_suspend *);
static void kr3dh_late_resume(struct early_suspend *);


#define KR3DH_MAJOR 	100

/* kr3dh ioctl command label */
#define IOCTL_KR3DH_GET_ACC_VALUE		0
#define DCM_IOC_MAGIC					's'
#define IOC_SET_ACCELEROMETER			_IO (DCM_IOC_MAGIC, 0x64)


#endif   // __BMA380_H__
