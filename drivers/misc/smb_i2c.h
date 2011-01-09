#ifndef __SMB_I2C_HEADER__
#define __SMB_I2C_HEADER__

char  i2c_acc_smb_read (u8, u8 *, unsigned int);
char  i2c_acc_smb_write(u8 reg, u8 *val);

int  i2c_acc_smb_init(void);
void i2c_acc_smb_exit(void);

#endif
