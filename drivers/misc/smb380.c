
#include "smb380.h"
#include <linux/kernel.h>

smb380_t *p_smb380;				/**< pointer to SMB380 device structure  */


int smb380_init(smb380_t *smb380) 
{
	int comres=0;
	unsigned char data;

	p_smb380 = smb380;															/* assign smb380 ptr */
	p_smb380->dev_addr = SMB380_I2C_ADDR;										/* preset SM380 I2C_addr */
	comres += p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, CHIP_ID__REG, &data, 1);	/* read Chip Id */
	
	p_smb380->chip_id = SMB380_GET_BITSLICE(data, CHIP_ID);			/* get bitslice */
		
	comres += p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ML_VERSION__REG, &data, 1); /* read Version reg */
	p_smb380->ml_version = SMB380_GET_BITSLICE(data, ML_VERSION);	/* get ML Version */
	p_smb380->al_version = SMB380_GET_BITSLICE(data, AL_VERSION);	/* get AL Version */
	
	
	printk("%s: I2C addr: %d, ML version %d, AL version %d", __FUNCTION__, p_smb380->dev_addr, p_smb380->ml_version, p_smb380->al_version);

	 //samsung add
	 ////  make sure the default value of the interrupt enable register 
	 data = 0x00;  // All interrupt disable
        comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, 0x0b, &data, 1); /* read Version reg */

	return comres;

}

int smb380_soft_reset() 
{
	int comres;
	unsigned char data=0;
	
	if (p_smb380==0) 
		return E_SMB_NULL_PTR;
	
	data = SMB380_SET_BITSLICE(data, SOFT_RESET, 1);
    comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, SOFT_RESET__REG, &data,1); 
	
	return comres;
}


int smb380_update_image() 
{
	int comres;
	unsigned char data=0;
	
	if (p_smb380==0) 
		return E_SMB_NULL_PTR;
	
	data = SMB380_SET_BITSLICE(data, UPDATE_IMAGE, 1);
    comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, UPDATE_IMAGE__REG, &data,1); 
	
	return comres;
}


int smb380_set_image (smb380regs_t *smb380Image) 
{
	int comres;
	unsigned char data;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;
    
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, EE_W__REG,&data, 1);
	data = SMB380_SET_BITSLICE(data, EE_W, SMB380_EE_W_ON);
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, EE_W__REG, &data, 1);
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, SMB380_IMAGE_BASE, (unsigned char*)smb380Image, SMB380_IMAGE_LEN);
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, EE_W__REG,&data, 1);
	data = SMB380_SET_BITSLICE(data, EE_W, SMB380_EE_W_OFF);
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, EE_W__REG, &data, 1);
	
	return comres;
}



int smb380_get_image(smb380regs_t *smb380Image)
{
	int comres;
	unsigned char data;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;
    
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, EE_W__REG,&data, 1);
	data = SMB380_SET_BITSLICE(data, EE_W, SMB380_EE_W_ON);
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, EE_W__REG, &data, 1);
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SMB380_IMAGE_BASE, (unsigned char *)smb380Image, SMB380_IMAGE_LEN);
	data = SMB380_SET_BITSLICE(data, EE_W, SMB380_EE_W_OFF);
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, EE_W__REG, &data, 1);
	
	return comres;
}

int smb380_get_offset(unsigned char xyz, unsigned short *offset) 
{
   int comres;
   unsigned char data;
   
   if (p_smb380==0)
   		return E_SMB_NULL_PTR;
   
   comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = SMB380_GET_BITSLICE(data, OFFSET_X_LSB);
   *offset = data;
   comres += p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, (OFFSET_X_MSB__REG+xyz), &data, 1);
   *offset |= (data<<2);
   
   return comres;
}


int smb380_set_offset(unsigned char xyz, unsigned short offset) 
{
   int comres;
   unsigned char data;
   
   if (p_smb380==0)
   		return E_SMB_NULL_PTR;
   
   comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = SMB380_SET_BITSLICE(data, OFFSET_X_LSB, offset);
   comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = (offset&0x3ff)>>2;
   comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, (OFFSET_X_MSB__REG+xyz), &data, 1);
   
   return comres;
}


int smb380_set_offset_eeprom(unsigned char xyz, unsigned short offset) 
{
   int comres;
   unsigned char data;
   
   if (p_smb380==0)
   		return E_SMB_NULL_PTR;   
   
   comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = SMB380_SET_BITSLICE(data, OFFSET_X_LSB, offset);
   comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, (SMB380_EEP_OFFSET+OFFSET_X_LSB__REG + xyz), &data, 1);   
   p_smb380->delay_msec(34);
   data = (offset&0x3ff)>>2;
   comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, (SMB380_EEP_OFFSET+ OFFSET_X_MSB__REG+xyz), &data, 1);
   p_smb380->delay_msec(34);
   
   return comres;
}




int smb380_set_ee_w(unsigned char eew)
{
    unsigned char data;
	int comres;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;
    
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, EE_W__REG,&data, 1);
	data = SMB380_SET_BITSLICE(data, EE_W, eew);
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, EE_W__REG, &data, 1);
	
	return comres;
}



int smb380_write_ee(unsigned char addr, unsigned char data) 
{	
	int comres;
	
	if (p_smb380==0) 			/* check pointers */
		return E_SMB_NULL_PTR;
    
	if (p_smb380->delay_msec == 0)
	    return E_SMB_NULL_PTR;
    
	comres = smb380_set_ee_w( SMB380_EE_W_ON );
	addr|=0x20;   /* add eeprom address offset to image address if not applied */
	comres += smb380_write_reg(addr, &data, 1 );
	p_smb380->delay_msec( SMB380_EE_W_DELAY );
	comres += smb380_set_ee_w( SMB380_EE_W_OFF);
	
	return comres;
}


int smb380_selftest(unsigned char st)
{
	int comres;
	unsigned char data;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SELF_TEST__REG, &data, 1);
	data = SMB380_SET_BITSLICE(data, SELF_TEST, st);
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, SELF_TEST__REG, &data, 1);  
	
	return comres;  

}


int smb380_set_range(char range) 
{			
   int comres = 0;
   unsigned char data;

   if (p_smb380==0)
	    return E_SMB_NULL_PTR;

   if (range<3) 
   {	
	 	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, RANGE__REG, &data, 1 );
	 	data = SMB380_SET_BITSLICE(data, RANGE, range);		  	
		comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, RANGE__REG, &data, 1);
   }
   
   return comres;

}


int smb380_get_range(unsigned char *range) 
{
	int comres = 0;
	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, RANGE__REG, range, 1 );

	*range = SMB380_GET_BITSLICE(*range, RANGE);
	
	return comres;

}


int smb380_set_mode(unsigned char mode) {
	
	int comres=0;
	unsigned char data1, data2;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	if (mode<4 || mode!=1) 
	{
		comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, WAKE_UP__REG, &data1, 1 );
		data1  = SMB380_SET_BITSLICE(data1, WAKE_UP, mode);		  
        comres += p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SLEEP__REG, &data2, 1 );
		data2  = SMB380_SET_BITSLICE(data2, SLEEP, (mode>>1));
    	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, WAKE_UP__REG, &data1, 1);
	  	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, SLEEP__REG, &data2, 1);
	  	p_smb380->mode = mode;
	} 
	
	return comres;
	
}



unsigned char smb380_get_mode(void) 
{
    if (p_smb380==0)
    	return E_SMB_NULL_PTR;	
	
	return p_smb380->mode;
	
}

int smb380_set_bandwidth(char bw) 
{
	int comres = 0;
	unsigned char data;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	if (bw<8) 
	{
  	  comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, RANGE__REG, &data, 1 );
	  data = SMB380_SET_BITSLICE(data, BANDWIDTH, bw);
	  comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, RANGE__REG, &data, 1 );
	}

	return comres;


}


int smb380_get_bandwidth(unsigned char *bw) 
{
	int comres = 1;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, BANDWIDTH__REG, bw, 1 );		

	*bw = SMB380_GET_BITSLICE(*bw, BANDWIDTH);
	
	return comres;
}

int smb380_set_wake_up_pause(unsigned char wup)
{
	int comres=0;
	unsigned char data;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, WAKE_UP_PAUSE__REG, &data, 1 );
	data = SMB380_SET_BITSLICE(data, WAKE_UP_PAUSE, wup);
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, WAKE_UP_PAUSE__REG, &data, 1 );
	
	return comres;
}

int smb380_get_wake_up_pause(unsigned char *wup)
{
    int comres = 1;
	unsigned char data;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, WAKE_UP_PAUSE__REG, &data,  1 );		
	
	*wup = SMB380_GET_BITSLICE(data, WAKE_UP_PAUSE);
	
	return comres;

}


int smb380_set_low_g_threshold(unsigned char th) 
{
	int comres;	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;		

	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, LG_THRES__REG, &th, 1);

	return comres;
}


int smb380_get_low_g_threshold(unsigned char *th)
{

	int comres=1;	
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;	

		comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, LG_THRES__REG, th, 1);		

	return comres;

}


int smb380_set_low_g_countdown(unsigned char cnt)
{
	int comres=0;
	unsigned char data;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, COUNTER_LG__REG, &data, 1 );
	data = SMB380_SET_BITSLICE(data, COUNTER_LG, cnt);
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, COUNTER_LG__REG, &data, 1 );
	
	return comres;
}


int smb380_get_low_g_countdown(unsigned char *cnt)
{
    int comres = 1;
	unsigned char data;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, COUNTER_LG__REG, &data,  1 );		
	*cnt = SMB380_GET_BITSLICE(data, COUNTER_LG);
	
	return comres;
}


int smb380_set_high_g_countdown(unsigned char cnt)
{
	int comres=1;
	unsigned char data;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

    comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, COUNTER_HG__REG, &data, 1 );
	data = SMB380_SET_BITSLICE(data, COUNTER_HG, cnt);
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, COUNTER_HG__REG, &data, 1 );
	
	return comres;
}


int smb380_get_high_g_countdown(unsigned char *cnt)
{
    int comres = 0;
	unsigned char data;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, COUNTER_HG__REG, &data,  1 );		
	
	*cnt = SMB380_GET_BITSLICE(data, COUNTER_HG);
	
	return comres;

}


int smb380_set_low_g_duration(unsigned char dur) 
{
	int comres=0;	
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;
		
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, LG_DUR__REG, &dur, 1);

	return comres;
}


int smb380_get_low_g_duration(unsigned char *dur) {
	
	int comres=0;	
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, LG_DUR__REG, dur, 1);				  
	
	return comres;
}


int smb380_set_high_g_threshold(unsigned char th) 
{
	int comres=0;	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, HG_THRES__REG, &th, 1);
	
	return comres;
}

int smb380_get_high_g_threshold(unsigned char *th)
{
	int comres=0;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, HG_THRES__REG, th, 1);		

	return comres;
}



int smb380_set_high_g_duration(unsigned char dur) 
{
	int comres=0;	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, HG_DUR__REG, &dur, 1);

	return comres;
}


int smb380_get_high_g_duration(unsigned char *dur) {	
	
	int comres=0;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;
			
    comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, HG_DUR__REG, dur, 1);		

	return comres;
}


int smb380_set_any_motion_threshold(unsigned char th) 
{
	int comres=0;	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, ANY_MOTION_THRES__REG, &th, 1);

	return comres;
}


int smb380_get_any_motion_threshold(unsigned char *th)
{
	int comres=0;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ANY_MOTION_THRES__REG, th, 1);		

	return comres;

}


int smb380_set_any_motion_count(unsigned char amc)
{
	int comres=0;	
	unsigned char data;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

 	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ANY_MOTION_DUR__REG, &data, 1 );
	data = SMB380_SET_BITSLICE(data, ANY_MOTION_DUR, amc);
	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, ANY_MOTION_DUR__REG, &data, 1 );
	
	return comres;
}


int smb380_get_any_motion_count(unsigned char *amc)
{
    int comres = 0;
	unsigned char data;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ANY_MOTION_DUR__REG, &data,  1 );		
	
	*amc = SMB380_GET_BITSLICE(data, ANY_MOTION_DUR);
	
	return comres;
}



int smb380_set_interrupt_mask(unsigned char mask) 
{
	int comres=0;
	unsigned char data[4];

	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	
	data[0] = mask & SMB380_CONF1_INT_MSK;
	data[2] = ((mask<<1) & SMB380_CONF2_INT_MSK);		

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SMB380_CONF1_REG, &data[1], 1);
	comres += p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SMB380_CONF2_REG, &data[3], 1);		
	
	data[1] &= (~SMB380_CONF1_INT_MSK);
	data[1] |= data[0];
	data[3] &=(~(SMB380_CONF2_INT_MSK));
	data[3] |= data[2];

	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, SMB380_CONF1_REG, &data[1], 1);
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, SMB380_CONF2_REG, &data[3], 1);

	return comres;	
}


int smb380_get_interrupt_mask(unsigned char *mask) 
{
	int comres=0;
	unsigned char data;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SMB380_CONF1_REG, &data,1);
	*mask = data & SMB380_CONF1_INT_MSK;
	p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SMB380_CONF2_REG, &data,1);
	*mask = *mask | ((data & SMB380_CONF2_INT_MSK)>>1);

	return comres;
}


int smb380_reset_interrupt(void) 
{	
	int comres=0;
	unsigned char data=(1<<RESET_INT__POS);	
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, RESET_INT__REG, &data, 1);
	return comres;

}


int smb380_read_accel_x(short *a_x) 
{
	int comres;
	unsigned char data[2];

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ACC_X_LSB__REG, data, 2);
	
	*a_x = SMB380_GET_BITSLICE(data[0],ACC_X_LSB) | SMB380_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN;
	*a_x = *a_x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	*a_x = *a_x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	
	return comres;
	
}


int smb380_read_accel_y(short *a_y) 
{
	int comres;
	unsigned char data[2];	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ACC_Y_LSB__REG, data, 2);
	
	*a_y = SMB380_GET_BITSLICE(data[0],ACC_Y_LSB) | SMB380_GET_BITSLICE(data[1],ACC_Y_MSB)<<ACC_Y_LSB__LEN;
	*a_y = *a_y << (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));
	*a_y = *a_y >> (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));
	
	return comres;
}


/** Z-axis acceleration data readout 
	\param *a_z pointer for 16 bit 2's complement data output (LSB aligned)
*/
int smb380_read_accel_z(short *a_z)
{
	int comres;
	unsigned char data[2];	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ACC_Z_LSB__REG, data, 2);
	
	*a_z = SMB380_GET_BITSLICE(data[0],ACC_Z_LSB) | SMB380_GET_BITSLICE(data[1],ACC_Z_MSB)<<ACC_Z_LSB__LEN;
	*a_z = *a_z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	*a_z = *a_z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	
	return comres;
}


int smb380_read_temperature(unsigned char * temp) 
{
	int comres;	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, TEMPERATURE__REG, temp, 1);
	
	return comres;

}


int smb380_read_accel_xyz(smb380acc_t * acc)
{
	int comres;
	unsigned char data[6];

	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ACC_X_LSB__REG, &data[0],6);
	
	acc->x = SMB380_GET_BITSLICE(data[0],ACC_X_LSB) | (SMB380_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN);
	acc->x = acc->x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	acc->x = acc->x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));

	acc->y = SMB380_GET_BITSLICE(data[2],ACC_Y_LSB) | (SMB380_GET_BITSLICE(data[3],ACC_Y_MSB)<<ACC_Y_LSB__LEN);
	acc->y = acc->y << (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	acc->y = acc->y >> (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	
/*	
	acc->z = SMB380_GET_BITSLICE(data[4],ACC_Z_LSB); 
	acc->z |= (SMB380_GET_BITSLICE(data[5],ACC_Z_MSB)<<ACC_Z_LSB__LEN);
	acc->z = acc->z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
*/	
	acc->z = SMB380_GET_BITSLICE(data[4],ACC_Z_LSB) | (SMB380_GET_BITSLICE(data[5],ACC_Z_MSB)<<ACC_Z_LSB__LEN);
	acc->z = acc->z << (sizeof(short)*8-(ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short)*8-(ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));
	
	return comres;
}


int smb380_get_interrupt_status(unsigned char * ist) 
{

	int comres=0;	

	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, SMB380_STATUS_REG, ist, 1);
	
	return comres;
}


int smb380_set_low_g_int(unsigned char onoff) {
	int comres;
	unsigned char data;
	
	if(p_smb380==0) 
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ENABLE_LG__REG, &data, 1);				
	
	data = SMB380_SET_BITSLICE(data, ENABLE_LG, onoff);
	
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, ENABLE_LG__REG, &data, 1);
	
	return comres;
}


int smb380_set_high_g_int(unsigned char onoff) 
{
	int comres;
	
	unsigned char data;
	
	if(p_smb380==0) 
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ENABLE_HG__REG, &data, 1);				
	
	data = SMB380_SET_BITSLICE(data, ENABLE_HG, onoff);
	
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, ENABLE_HG__REG, &data, 1);
	
	return comres;
}


int smb380_set_any_motion_int(unsigned char onoff) {
	int comres;
	
	unsigned char data;
	
	if(p_smb380==0) 
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, EN_ANY_MOTION__REG, &data, 1);				
	data = SMB380_SET_BITSLICE(data, EN_ANY_MOTION, onoff);
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, EN_ANY_MOTION__REG, &data, 1);
	
	return comres;

}


int smb380_set_alert_int(unsigned char onoff) 
{
	int comres;
	unsigned char data;
	
	if(p_smb380==0) 
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ALERT__REG, &data, 1);				
	data = SMB380_SET_BITSLICE(data, ALERT, onoff);
	
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, ALERT__REG, &data, 1);
	
	return comres;

}


int smb380_set_advanced_int(unsigned char onoff) 
{
	int comres;
	unsigned char data;

	if(p_smb380==0) 
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, ENABLE_ADV_INT__REG, &data, 1);				
	data = SMB380_SET_BITSLICE(data, EN_ANY_MOTION, onoff);

	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, ENABLE_ADV_INT__REG, &data, 1);
	
	return comres;
}


int smb380_latch_int(unsigned char latched) 
{
	int comres;
	unsigned char data;
	
	if(p_smb380==0) 
		return E_SMB_NULL_PTR;
	
	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, LATCH_INT__REG, &data, 1);				
	data = SMB380_SET_BITSLICE(data, LATCH_INT, latched);

	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, LATCH_INT__REG, &data, 1);
	
	return comres;
}


int smb380_set_new_data_int(unsigned char onoff) 
{
	int comres;
	unsigned char data;

	if(p_smb380==0) 
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, NEW_DATA_INT__REG, &data, 1);				
	data = SMB380_SET_BITSLICE(data, NEW_DATA_INT, onoff);
	comres += p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, NEW_DATA_INT__REG, &data, 1);
	
	return comres;
}


int smb380_pause(int msec) 
{
	if (p_smb380==0)
		return E_SMB_NULL_PTR;
	else
		p_smb380->delay_msec(msec);	

	return msec;
}


int smb380_read_reg(unsigned char addr, unsigned char *data, unsigned char len)
{
	int comres;
	
	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_READ_FUNC(p_smb380->dev_addr, addr, data, len);
	
	return comres;
}


int smb380_write_reg(unsigned char addr, unsigned char *data, unsigned char len) 
{
	int comres;

	if (p_smb380==0)
		return E_SMB_NULL_PTR;

	comres = p_smb380->SMB380_BUS_WRITE_FUNC(p_smb380->dev_addr, addr, data, len);

	return comres;
}
