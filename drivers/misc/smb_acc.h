#ifndef __SMB_ACC_HEADER__
#define __SMB_ACC_HEADER__

#include "smb_i2c.h"
#include "smb380.h"

#define ACC_DEV_NAME "accelerometer"
#define ACC_DEV_MAJOR 241

/* smb ioctl command label */
#define IOCTL_SMB_GET_ACC_VALUE		0
#define DCM_IOC_MAGIC			's'
#define IOC_SET_ACCELEROMETER	_IO (DCM_IOC_MAGIC, 0x64)
#define BMA150_CALIBRATION		_IOWR(DCM_IOC_MAGIC,48,short)

#define SMB_POWER_OFF               0
#define SMB_POWER_ON                1

#endif
