/*  $Date: 2007/08/07 16:06:00 $
 *  $Revision: 1.1 $ 
 */


/*
* Copyright (C) 2007 Bosch Sensortec GmbH
*
* SMB380 calibration routine based on SMB380 API
* 
* Usage: calibration of SMB380/BMA150 acceleration sensor
*
* Author:	Lars.Beseke@bosch-sensortec.com
*
* Disclaimer
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


/*! \file smb380calib.h
    \brief This file contains all function headers for the SMB380/BMA150 calibration process
        
*/


#ifndef __SMB380_CALIBRATION__
#define __SMB380_CALIBRATION__


#define SMB380_CALIBRATION_MAX_TRIES 10


/** calculates new offset in respect to acceleration data and old offset register values
  \param orientation pass orientation one axis needs to be absolute 1 the others need to be 0
  \param *offset_x takes the old offset value and modifies it to the new calculated one
  \param *offset_y takes the old offset value and modifies it to the new calculated one
  \param *offset_z takes the old offset value and modifies it to the new calculated one
 */

int smb380_calibrate(smb380acc_t, int *);

/** reads out acceleration data and averages them, measures min and max
  \param orientation pass orientation one axis needs to be absolute 1 the others need to be 0
  \param num_avg numer of samples for averaging
  \param *min returns the minimum measured value
  \param *max returns the maximum measured value
  \param *average returns the average value
 */
int smb380_read_accel_avg(int, smb380acc_t *, smb380acc_t *, smb380acc_t * );

/** verifies the accerleration values to be good enough for calibration calculations
 \param min takes the minimum measured value
  \param max takes the maximum measured value
  \param takes returns the average value
  \return 1: min,max values are in range, 0: not in range
*/
int smb380_verify_min_max(smb380acc_t , smb380acc_t , smb380acc_t );

/** smb380_calibration routine
  \param orientation pass orientation one axis needs to be absolute 1 the others need to be 0
  \param tries number of iterative passes
  \param *min, *max, *avg returns minimum, maximum and average offset 
 */
int smb380_calc_new_offset(smb380acc_t, smb380acc_t, unsigned short *, unsigned short *, unsigned short *);

/** overall calibration process. This function takes care about all other functions 
  \param orientation input for orientation [0;0;1] for measuring the device in horizontal surface up
  \param *tries takes the number of wanted iteration steps, this pointer returns the number of calculated steps after this routine has finished
  \return 1: calibration passed 2: did not pass within N steps 
*/
int smb380_store_calibration(unsigned short, unsigned short, unsigned short);


#endif // endif __SMB380_CALIBRATION__
