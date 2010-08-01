#ifndef TUNE2CMC623_HEADER
#define TUNE2CMC623_HEADER

#if defined(__cplusplus)
extern "C"
{
#endif

/** CMC623 registers **/


/* SFR Bank selection */
#define CMC623_REG_SELBANK   0x00  // Bank0

/* A stage configuration */
#define CMC623_REG_DNRHDTROVE 0x01
#define CMC623_REG_DITHEROFF 0x06
#define CMC623_REG_CLKCONT 0x10
#define CMC623_REG_CLKGATINGOFF 0x0a
#define CMC623_REG_INPUTIFCON 0x24
#define CMC623_REG_CLKMONCONT   0x11  // Clock Monitor Ctrl Register
#define CMC623_REG_HDRTCEOFF 0x3a
#define CMC623_REG_I2C 0x0d
#define CMC623_REG_BSTAGE 0x0e
#define CMC623_REG_CABCCTRL 0x7c
#define CMC623_REG_PWMCTRL 0xb4
#define CMC623_REG_OVEMAX 0x54

/* A stage image size */
#define CMC623_REG_1280 0x22
#define CMC623_REG_800 0x23

/* B stage image size */
#define CMC623_REG_SCALERINPH 0x09
#define CMC623_REG_SCALERINPV 0x0a
#define CMC623_REG_SCALEROUTH 0x0b
#define CMC623_REG_SCALEROUTV 0x0c

/* EDRAM configuration */
#define CMC623_REG_EDRBFOUT40 0x01
#define CMC623_REG_EDRAUTOREF 0x06
#define CMC623_REG_EDRACPARAMTIM 0x07

/* Vsync Calibartion */
#define CMC623_REG_CALVAL10 0x65

/* tcon output polarity */
#define CMC623_REG_TCONOUTPOL 0x68

/* tcon RGB configuration */
#define CMC623_REG_TCONRGB1 0x6c
#define CMC623_REG_TCONRGB2 0x6d
#define CMC623_REG_TCONRGB3 0x6e

/* Reg update */
#define CMC623_REG_REGMASK 0x28
#define CMC623_REG_SWRESET 0x09
#define CMC623_REG_RGBIFEN 0x26

#if 1  // P1_LSJ : DE04
typedef struct
{
    unsigned char RegAddr;
    unsigned long Data;  // data to write
}Cmc623RegisterSet;
#else 
typedef struct
{
    NvU8 RegAddr;
    NvU32 Data;  // data to write
}Cmc623RegisterSet;
#endif 

static Cmc623RegisterSet Cmc623_InitSeq[] =
{
    /* select SFR Bank0 */
    {CMC623_REG_SELBANK, 0x0000},

    /* A stage configuration */
    {CMC623_REG_DNRHDTROVE, 0x0030},
    {CMC623_REG_DITHEROFF, 0x0000},
    {CMC623_REG_CLKCONT, 0x0012},
    {CMC623_REG_CLKGATINGOFF, 0x0000},
    {CMC623_REG_INPUTIFCON, 0x0001},
    // {CMC623_REG_CLKMONCONT, 0x0081},
    {CMC623_REG_HDRTCEOFF, 0x001d},
    {CMC623_REG_I2C, 0x180a},
    {CMC623_REG_BSTAGE, 0x0a0b},
    {CMC623_REG_CABCCTRL, 0x0002},
    {CMC623_REG_PWMCTRL, 0x5000},
    
    // {CMC623_REG_OVEMAX, 0x8080},
    {CMC623_REG_1280, 0x0500},
    {CMC623_REG_800, 0x0320},

    /* select SFR Bank1 */
    {CMC623_REG_SELBANK, 0x0001},

    /* B stage image size */
    {CMC623_REG_SCALERINPH, 0x0500},
    {CMC623_REG_SCALERINPV, 0x0320},
    {CMC623_REG_SCALEROUTH, 0x0500},
    {CMC623_REG_SCALEROUTV, 0x0320},

    /* EDRAM configuration */
    {CMC623_REG_EDRBFOUT40, 0x0280},
    {CMC623_REG_EDRAUTOREF, 0x0059},
    {CMC623_REG_EDRACPARAMTIM, 0x2225},

    /* Vsync Calibartion */
    {CMC623_REG_CALVAL10, 0x000a},

    /* tcon output polarity */
    {CMC623_REG_TCONOUTPOL, 0x0008},

    /* tcon RGB configuration */
    {CMC623_REG_TCONRGB1, 0x0488},
    {CMC623_REG_TCONRGB2, 0x1504},
    {CMC623_REG_TCONRGB3, 0x8a22},

    /* Reg update */
    {CMC623_REG_SELBANK, 0x0000},  // select BANK0
    {CMC623_REG_REGMASK, 0x0000},
    {CMC623_REG_SWRESET, 0x0000},  // SW reset
    {CMC623_REG_SWRESET, 0xffff},  // !SW reset, (!note: sleep required after this)
    {CMC623_REG_RGBIFEN, 0x0001},  // enable RGB IF
};

static int cmc623_attach_adapter(struct i2c_adapter *adap); 

extern int tune_cmc623_suspend();
extern int tune_cmc623_resume();

#if defined(__cplusplus)
}
#endif

#endif  //TUNE2CMC623_HEADER
