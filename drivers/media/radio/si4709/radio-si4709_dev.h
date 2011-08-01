#ifndef _Si4709_DEV_H
#define _Si4709_DEV_H

#include <linux/i2c.h>
#include "radio-si4709_common.h"

typedef struct
{
    int power_state;
    int seek_state;
}dev_state_t;

typedef struct
{
  u8 curr_rssi;
  u8 curr_rssi_th;
  u8 curr_snr;
}rssi_snr_t;

typedef struct																				
{	
	u8 part_number;																
	u16 manufact_number;
} device_id;

typedef struct														
{
	u8  chip_version;
	u8 	device;
	u8  firmware_version;
}chip_id;

typedef struct
{
	u16 rssi_th;
	u8 fm_band;
	u8 fm_chan_spac;
	u8 fm_vol;
} sys_config2;

typedef struct
{
	u8 smmute;
	u8 smutea;
	u8 volext;
	u8 sksnr;
	u8 skcnt;
} sys_config3;

typedef struct
{
	u8 rdsr;
	u8 stc;
	u8 sfbl;
	u8 afcrl;
	u8 rdss;
	u8 blera;
	u8 st;
	u16 rssi;
} status_rssi;

typedef struct														
{
	u16 dsmute :1;
	u16 dmute:1;
	u16 mono:1;
	u16 rds_mode:1;
	u16 sk_mode:1;
	u16 seek_up:1;
	u16 seek:1;
	u16 power_disable:1;
	u16 power_enable:1;
} power_config;

typedef struct
{
	u16 rdsa;
	u16 rdsb;
	u16 rdsc;
	u16 rdsd;
	u8  curr_rssi;
	u32 curr_channel;
	u8 blera;
	u8 blerb;
	u8 blerc;
	u8 blerd;
}radio_data_t;

#define NUM_SEEK_PRESETS    20

#define WAIT_OVER        	0
#define SEEK_WAITING     	1
#define NO_WAIT             2
#define TUNE_WAITING   		4
#define RDS_WAITING      	5
#define SEEK_CANCEL      	6

/*dev settings*/
/*band*/
#define BAND_87500_108000_kHz   1
#define BAND_76000_108000_kHz   2
#define BAND_76000_90000_kHz    3

/*channel spacing*/
#define CHAN_SPACING_200_kHz   20        /*US*/
#define CHAN_SPACING_100_kHz   10		 /*Europe,Japan*/	
#define CHAN_SPACING_50_kHz    5

/*DE-emphasis Time Constant*/
#define DE_TIME_CONSTANT_50   1          /*Europe,Japan,Australia*/
#define DE_TIME_CONSTANT_75   0		     /*US*/

extern int Si4709_dev_wait_flag;

#ifdef RDS_INTERRUPT_ON_ALWAYS
extern int Si4709_RDS_flag;
extern int RDS_Data_Available;
extern int RDS_Data_Lost;
extern int RDS_Groups_Available_till_now;
extern struct workqueue_struct *Si4709_wq;
extern struct work_struct Si4709_work;
#endif

/*Function prototypes*/
extern int Si4709_dev_init(struct i2c_client *);
extern int Si4709_dev_exit(void);

extern void Si4709_dev_mutex_init(void); 

extern int Si4709_dev_suspend(void);
extern int Si4709_dev_resume(void);

extern int Si4709_dev_powerup(void);
extern int Si4709_dev_powerdown(void);

extern int Si4709_dev_band_set(int);
extern int Si4709_dev_ch_spacing_set(int);

extern int Si4709_dev_chan_select(u32);
extern int Si4709_dev_chan_get(u32*);

extern int Si4709_dev_seek_up(u32*);
extern int Si4709_dev_seek_down(u32*);
extern int Si4709_dev_seek_auto(u32*);

extern int Si4709_dev_RSSI_seek_th_set(u8);
extern int Si4709_dev_seek_SNR_th_set(u8);
extern int Si4709_dev_seek_FM_ID_th_set(u8);
extern int Si4709_dev_cur_RSSI_get(rssi_snr_t*);
extern int Si4709_dev_VOLEXT_ENB(void);
extern int Si4709_dev_VOLEXT_DISB(void);
extern int Si4709_dev_volume_set(u8);
extern int Si4709_dev_volume_get(u8*);
extern int Si4709_dev_DSMUTE_ON(void);
extern int Si4709_dev_DSMUTE_OFF(void);
extern int Si4709_dev_MUTE_ON(void);
extern int Si4709_dev_MUTE_OFF(void);
extern int Si4709_dev_MONO_SET(void);
extern int Si4709_dev_STEREO_SET(void);
extern int Si4709_dev_rstate_get(dev_state_t*);
extern int Si4709_dev_RDS_data_get(radio_data_t*);
extern int Si4709_dev_RDS_ENABLE(void);
extern int Si4709_dev_RDS_DISABLE(void);
extern int Si4709_dev_RDS_timeout_set(u32);
extern int Si4709_dev_device_id(device_id *);          	
extern int Si4709_dev_chip_id(chip_id *);					
extern int Si4709_dev_sys_config2(sys_config2 *);
extern int Si4709_dev_sys_config3(sys_config3 *);
extern int Si4709_dev_power_config(power_config *);		
extern int Si4709_dev_AFCRL_get(u8*);
extern int Si4709_dev_DE_set(u8);
extern int Si4709_dev_status_rssi(status_rssi *status);
extern int Si4709_dev_sys_config2_set(sys_config2 *sys_conf2);
extern int Si4709_dev_sys_config3_set(sys_config3 *sys_conf3);
extern int Si4709_dev_reset_rds_data(void);

#ifdef RDS_INTERRUPT_ON_ALWAYS
extern void Si4709_work_func(struct work_struct*);
#endif
#endif

