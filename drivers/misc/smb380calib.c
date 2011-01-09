/*  $Date: 2007/08/07 16:06:00 $
 *  $Revision: 1.1 $ 
 */


/** \mainpage BMA150 / SMB380 In Line Calibration
 * Copyright (C) 2007 Bosch Sensortec GmbH
 *  \section intro_sec Introduction
 * BMA150 / SMB380 3-axis digital Accelerometer In Line Calibration Process
 * This software is compatible with all Bosch Sensortec SMB380 / BMA150 API >=1.1
 *
 * Author:	Lars.Beseke@bosch-sensortec.com
 *
 * 
 * \section disclaimer_sec Disclaimer
 *
 *
 *
 * Common:
 * Bosch Sensortec products are developed for the consumer goods industry. They may only be used 
 * within the parameters of the respective valid product data sheet.  Bosch Sensortec products are 
 * provided with the express understanding that there is no warranty of fitness for a particular purpose. 
 * They are not fit for use in life-sustaining, safety or security sensitive systems or any system or device 
 * that may lead to bodily harm or property damage if the system or device malfunctions. In addition, 
 * Bosch Sensortec products are not fit for use in products which interact with motor vehicle systems.  
 * The resale and/or use of products are at the purchaser’s own risk and his own responsibility. The 
 * examination of fitness for the intended use is the sole responsibility of the Purchaser. 
 *
 * The purchaser shall indemnify Bosch Sensortec from all third party claims, including any claims for 
 * incidental, or consequential damages, arising from any product use not covered by the parameters of 
 * the respective valid product data sheet or not approved by Bosch Sensortec and reimburse Bosch 
 * Sensortec for all costs in connection with such claims.
 *
 * The purchaser must monitor the market for the purchased products, particularly with regard to 
 * product safety and inform Bosch Sensortec without delay of all security relevant incidents.
 *
 * Engineering Samples are marked with an asterisk (*) or (e). Samples may vary from the valid 
 * technical specifications of the product series. They are therefore not intended or fit for resale to third 
 * parties or for use in end products. Their sole purpose is internal client testing. The testing of an 
 * engineering sample may in no way replace the testing of a product series. Bosch Sensortec 
 * assumes no liability for the use of engineering samples. By accepting the engineering samples, the 
 * Purchaser agrees to indemnify Bosch Sensortec from all claims arising from the use of engineering 
 * samples.
 *
 * Special:
 * This software module (hereinafter called "Software") and any information on application-sheets 
 * (hereinafter called "Information") is provided free of charge for the sole purpose to support your 
 * application work. The Software and Information is subject to the following terms and conditions: 
 *
 * The Software is specifically designed for the exclusive use for Bosch Sensortec products by 
 * personnel who have special experience and training. Do not use this Software if you do not have the 
 * proper experience or training. 
 *
 * This Software package is provided `` as is `` and without any expressed or implied warranties, 
 * including without limitation, the implied warranties of merchantability and fitness for a particular 
 * purpose. 
 *
 * Bosch Sensortec and their representatives and agents deny any liability for the functional impairment 
 * of this Software in terms of fitness, performance and safety. Bosch Sensortec and their 
 * representatives and agents shall not be liable for any direct or indirect damages or injury, except as 
 * otherwise stipulated in mandatory applicable law.
 * 
 * The Information provided is believed to be accurate and reliable. Bosch Sensortec assumes no 
 * responsibility for the consequences of use of such Information nor for any infringement of patents or 
 * other rights of third parties which may result from its use. No license is granted by implication or 
 * otherwise under any patent or patent rights of Bosch. Specifications mentioned in the Information are 
 * subject to change without notice.
 *
 * It is not allowed to deliver the source code of the Software to any third party without permission of 
 * Bosch Sensortec.
 */

/*! \file smb380calib.c
    \brief This file contains all function implementatios for the SMB380/BMA150 calibration process
        
*/

#include "smb380.h"
#include "smb380calib.h"

/** calculates new offset in respect to acceleration data and old offset register values
  \param orientation pass orientation one axis needs to be absolute 1 the others need to be 0
  \param *offset_x takes the old offset value and modifies it to the new calculated one
  \param *offset_y takes the old offset value and modifies it to the new calculated one
  \param *offset_z takes the old offset value and modifies it to the new calculated one
 */
int smb380_calc_new_offset(smb380acc_t orientation, smb380acc_t accel, 
                                             unsigned short *offset_x, unsigned short *offset_y, unsigned short *offset_z)
{
   short old_offset_x, old_offset_y, old_offset_z;
   short new_offset_x, new_offset_y, new_offset_z;   

   unsigned char  calibrated =0;

   old_offset_x = *offset_x;
   old_offset_y = *offset_y;
   old_offset_z = *offset_z;
   
   accel.x = accel.x - (orientation.x * 256);   
   accel.y = accel.y - (orientation.y * 256);
   accel.z = accel.z - (orientation.z * 256);
   	                                
   if ((accel.x > 4) | (accel.x < -4)) {           /* does x axis need calibration? */
	     
	 if ((accel.x <8) && accel.x > 0)              /* check for values less than quantisation of offset register */
	 	new_offset_x= old_offset_x -1;           
	 else if ((accel.x >-8) && (accel.x < 0))    
	   new_offset_x= old_offset_x +1;
     else 
       new_offset_x = old_offset_x - (accel.x/8);  /* calculate new offset due to formula */
     if (new_offset_x <0) 						/* check for register boundary */
	   new_offset_x =0;							/* <0 ? */
     else if (new_offset_x>1023)
	   new_offset_x=1023;                       /* >1023 ? */
     *offset_x = new_offset_x;     
	 calibrated = 1;
   }

   if ((accel.y > 4) | (accel.y<-4)) {            /* does y axis need calibration? */
		 if ((accel.y <8) && accel.y > 0)             /* check for values less than quantisation of offset register */
	 	new_offset_y= old_offset_y -1;
	 else if ((accel.y >-8) && accel.y < 0)
	   new_offset_y= old_offset_y +1;	    
     else 
       new_offset_y = old_offset_y - accel.y/8;  /* calculate new offset due to formula */
     
	 if (new_offset_y <0) 						/* check for register boundary */
	   new_offset_y =0;							/* <0 ? */
     else if (new_offset_y>1023)
       new_offset_y=1023;                       /* >1023 ? */
   
     *offset_y = new_offset_y;
     calibrated = 1;
   }

   if ((accel.z > 4) | (accel.z<-4)) {            /* does z axis need calibration? */
	   	 if ((accel.z <8) && accel.z > 0)             /* check for values less than quantisation of offset register */  
	 	new_offset_z= old_offset_z -1;
	 else if ((accel.z >-8) && accel.z < 0)
	   new_offset_z= old_offset_z +1;	     
     else 
       new_offset_z = old_offset_z - (accel.z/8);/* calculate new offset due to formula */
 
     if (new_offset_z <0) 						/* check for register boundary */
	   new_offset_z =0;							/* <0 ? */
     else if (new_offset_z>1023)
	   new_offset_z=1023;

	 *offset_z = new_offset_z;
	 calibrated = 1;
  }	   
  return calibrated;
}

/** reads out acceleration data and averages them, measures min and max
  \param orientation pass orientation one axis needs to be absolute 1 the others need to be 0
  \param num_avg numer of samples for averaging
  \param *min returns the minimum measured value
  \param *max returns the maximum measured value
  \param *average returns the average value
 */

int smb380_read_accel_avg(int num_avg, smb380acc_t *min, smb380acc_t *max, smb380acc_t *avg )
{
   long x_avg=0, y_avg=0, z_avg=0;   
   int comres=0;
   int i;
   smb380acc_t accel;		                /* read accel data */

   x_avg = 0; y_avg=0; z_avg=0;                  
   max->x = -512; max->y =-512; max->z = -512;
   min->x = 512;  min->y = 512; min->z = 512; 
     

	 for (i=0; i<num_avg; i++) {
	 	comres += smb380_read_accel_xyz(&accel);      /* read 10 acceleration data triples */
		if (accel.x>max->x)
			max->x = accel.x;
		if (accel.x<min->x) 
			min->x=accel.x;

		if (accel.y>max->y)
			max->y = accel.y;
		if (accel.y<min->y) 
			min->y=accel.y;

		if (accel.z>max->z)
			max->z = accel.z;
		if (accel.z<min->z) 
			min->z=accel.z;
		
		x_avg+= accel.x;
		y_avg+= accel.y;
		z_avg+= accel.z;

		smb380_pause(10);
     }
	 avg->x = x_avg /= num_avg;                             /* calculate averages, min and max values */
	 avg->y = y_avg /= num_avg;
	 avg->z = z_avg /= num_avg;
	 return comres;
}
	 

/** verifies the accerleration values to be good enough for calibration calculations
 \param min takes the minimum measured value
  \param max takes the maximum measured value
  \param takes returns the average value
  \return 1: min,max values are in range, 0: not in range
*/

int smb380_verify_min_max(smb380acc_t min, smb380acc_t max, smb380acc_t avg) 
{
	short dx, dy, dz;
	int ver_ok=1;
	
	dx =  max.x - min.x;    /* calc delta max-min */
	dy =  max.y - min.y;
	dz =  max.z - min.z;


	if (dx> 10 || dx<-10) 
	  ver_ok = 0;
	if (dy> 10 || dy<-10) 
	  ver_ok = 0;
	if (dz> 10 || dz<-10) 
	  ver_ok = 0;

	return ver_ok;
}	



/** overall calibration process. This function takes care about all other functions 
  \param orientation input for orientation [0;0;1] for measuring the device in horizontal surface up
  \param *tries takes the number of wanted iteration steps, this pointer returns the number of calculated steps after this routine has finished
  \return 1: calibration passed 2: did not pass within N steps 
*/
  
int smb380_calibrate(smb380acc_t orientation, int *tries)
{

	unsigned short offset_x, offset_y, offset_z;
	unsigned short old_offset_x, old_offset_y, old_offset_z;
	int need_calibration=0, min_max_ok=0;	
	int ltries;

	smb380acc_t min,max,avg;

	smb380_set_range(SMB380_RANGE_2G);
	smb380_set_bandwidth(SMB380_BW_25HZ);

	smb380_set_ee_w(1);  
	
	smb380_get_offset(0, &offset_x);
	smb380_get_offset(1, &offset_y);
	smb380_get_offset(2, &offset_z);

	old_offset_x = offset_x;
	old_offset_y = offset_y;
	old_offset_z = offset_z;
	ltries = *tries;

	do {

		smb380_read_accel_avg(10, &min, &max, &avg);  /* read acceleration data min, max, avg */

		min_max_ok = smb380_verify_min_max(min, max, avg);

		/* check if calibration is needed */
		if (min_max_ok)
			need_calibration = smb380_calc_new_offset(orientation, avg, &offset_x, &offset_y, &offset_z);		
		  
		if (*tries==0) /*number of maximum tries reached? */
			break;

		if (need_calibration) {   
			/* when needed calibration is updated in image */
			(*tries)--;
			smb380_set_offset(0, offset_x); 
   		 	smb380_set_offset(1, offset_y);
 	  	 	smb380_set_offset(2, offset_z);
			smb380_pause(20);
   		}
    } while (need_calibration || !min_max_ok);

	        		
    if (*tries>0 && *tries < ltries) {

		if (old_offset_x!= offset_x) 
			smb380_set_offset_eeprom(0, offset_x);

		if (old_offset_y!= offset_y) 
	   		smb380_set_offset_eeprom(1,offset_y);

		if (old_offset_z!= offset_z) 
	   		smb380_set_offset_eeprom(2, offset_z);
	}	
		
	smb380_set_ee_w(0);	    
    smb380_pause(20);
	*tries = ltries - *tries;

	return !need_calibration;
}



	

