/*
 * i2c ddc port
 */


extern int ddc_read(u8 subaddr, u8 *data, u16 len);
extern int ddc_write(u8 *data, u16 len);
