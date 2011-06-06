/*
 * Driver for CE147 (5MP Camera) from NEC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/rtc.h>
#include <linux/completion.h>
#include <media/v4l2-device.h>
#include <media/v4l2-i2c-drv.h>
#include <media/ce147_platform.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

/* #define MDNIE_TUNING */

#define	CE147_DRIVER_NAME	"CE147"

#define	FORMAT_FLAGS_COMPRESSED		0x3
#define	SENSOR_JPEG_SNAPSHOT_MEMSIZE	0x360000

/* #define CE147_DEBUG */
/* #define CE147_INFO */

#ifdef CE147_DEBUG
#define	ce147_msg	dev_err
#else
#define	ce147_msg	dev_dbg
#endif

#ifdef CE147_INFO
#define	ce147_info	dev_err
#else
#define	ce147_info	dev_dbg
#endif

/* Default resolution & pixelformat. plz ref ce147_platform.h */
#define	DEFAULT_PIX_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */
#define	DEFUALT_MCLK		24000000
#define	POLL_TIME_MS		10

/* Camera ISP command */
#define	CMD_VERSION			0x00
#define	DATA_VERSION_FW			0x00
#define	DATA_VERSION_DATE		0x01
#define	CMD_GET_BATCH_REFLECTION_STATUS	0x02
#define	DATA_VERSION_SENSOR		0x03
#define	CMD_HD_PREVIEW			0x03
#define	CMD_SET_WB			0x04
#define	DATA_VERSION_AF			0x05
#define	CMD_SET_FLASH_MANUAL		0x06
#define	CMD_SET_EXIF_CTRL		0x07
#define	CMD_AE_WB_LOCK			0x11
#define	CMD_SET_ANTI_BANDING		0x14
#define	CMD_SET_WB_AUTO			0x1A
#define	CMD_SET_AUTO_FOCUS_MODE		0x20
#define	CMD_START_AUTO_FOCUS_SEARCH	0x23
#define	CMD_CHECK_AUTO_FOCUS_SEARCH	0x24
#define	CMD_STOP_LENS_MOVEMENT		0x35
#define	CMD_SET_EFFECT			0x3D
#define	CMD_SET_TOUCH_AUTO_FOCUS	0x4D
#define	CMD_START_OT			0x50
#define	CMD_CHECK_OT			0x51
#define	CMD_PREVIEW_SIZE		0x54
#define	CMD_FPS				0x5A
#define	CMD_SET_ANTI_SHAKE		0x5B
#define	CMD_SET_DATA			0x65
#define	CMD_DATA_OUT_REQ		0x66
#define	CMD_PREVIEW			0x6B
#define	CMD_PREVIEW_STATUS		0x6C
#define	CMD_CAPTURE_SIZE		0x73
#define	CMD_BUFFERING_CAPTURE		0x74
#define	CMD_SET_SMART_AUTO		0x82
#define	CMD_GET_SMART_AUTO_STATUS	0x83
#define	CMD_SET_WDR			0x88
#define	CMD_JPEG_SIZE			0x8E
#define	CMD_JPEG_BUFFERING		0x8F
#define	CMD_JPEG_CONFIG			0x90
#define	CMD_JPEG_BUFFERING2		0x92
#define	CMD_SET_FACE_DETECTION		0x9A
#define	CMD_SET_FACE_LOCK		0x9C
#define	CMD_INFO_EXIF			0xA0
#define	CMD_INFO_MODEL			0xA1
#define	CMD_INFO_ROT			0xA2
#define	CMD_INFO_LONGITUDE_LATITUDE	0xA3
#define	CMD_INFO_ALTITUDE		0xA4
#define	CMD_SET_FLASH			0xB2
#define	CMD_SET_DZOOM			0xB9
#define	CMD_GET_DZOOM_LEVEL		0xBA
#define	CMD_SET_EFFECT_SHOT		0xC0
#define	DATA_VERSION_GAMMA		0xE0
#define	DATA_VERSION_SENSOR_MAKER	0xE0
#define	CMD_CHECK_DATALINE		0xEC
#define	CMD_INIT			0xF0
#define	CMD_FW_INFO			0xF2
#define	CMD_FWU_UPDATE			0xF3
#define	CMD_FW_UPDATE			0xF4
#define	CMD_FW_STATUS			0xF5
#define	CMD_FW_DUMP			0xFB
#define	CMD_GPS_TIMESTAMP		0xA7

#define	CE147_FW_F2_PATH	"/system/firmware/CE147F02.bin"
#define   FACTORY_CHECK

/* { FW	Maj, FW	Min, PRM Maj, PRM Min }	*/
static unsigned	char	MAIN_SW_FW[4]		= { 0x0, 0x0, 0x0, 0x0 };
/* { Year, Month, Date } */
static int		MAIN_SW_DATE_INFO[3]	= { 0x0, 0x0, 0x0 };

static unsigned char ce147_buf_set_dzoom[31] = {
	0xff, 0xe7, 0xd3, 0xc2, 0xb4, 0xa7, 0x9c, 0x93, 0x8b, 0x83,
	0x7c, 0x76, 0x71, 0x6c, 0x67, 0x63, 0x5f, 0x5b, 0x58, 0x55,
	0x52, 0x4f, 0x4d, 0x4a, 0x48, 0x46, 0x44, 0x42, 0x41, 0x40,
	0x3f
};
static int DZoom_State;

enum ce147_oprmode {
	CE147_OPRMODE_VIDEO = 0,
	CE147_OPRMODE_IMAGE = 1,
};

/* Declare Funtion */
static int ce147_set_awb_lock(struct v4l2_subdev *sd, int lock);
static int ce147_set_iso(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ce147_set_metering(struct v4l2_subdev *sd,
				struct v4l2_control *ctrl);
static int ce147_set_ev(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ce147_set_slow_ae(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ce147_set_gamma(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ce147_set_effect(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int ce147_set_white_balance(struct v4l2_subdev *sd,
				struct v4l2_control *ctrl);
static int ce147_s_ext_ctrl(struct v4l2_subdev *sd,
				struct v4l2_ext_control *ctrl);

enum {
	AUTO_FOCUS_FAILED,
	AUTO_FOCUS_DONE,
	AUTO_FOCUS_CANCELLED,
};

enum af_operation_status {
	AF_NONE = 0,
	AF_START,
	AF_CANCEL,
};

enum ce147_frame_size {
	CE147_PREVIEW_QCIF = 0,
	CE147_PREVIEW_QVGA,
	CE147_PREVIEW_592x480,
	CE147_PREVIEW_VGA,
	CE147_PREVIEW_D1,
	CE147_PREVIEW_WVGA,
	CE147_PREVIEW_720P,
	CE147_PREVIEW_VERTICAL_QCIF,
	CE147_CAPTURE_VGA,	/* 640 x 480 */
	CE147_CAPTURE_WVGA,	/* 800 x 480 */
	CE147_CAPTURE_W1MP,	/* 1600	x 960 */
	CE147_CAPTURE_2MP,	/* UXGA	 - 1600 x 1200 */
	CE147_CAPTURE_W2MP,	/* 35mm Academy Offset Standard 1.66
				   - 2048 x 1232, 2.4MP	*/
	CE147_CAPTURE_3MP,	/* QXGA	 - 2048 x 1536 */
	CE147_CAPTURE_W4MP,	/* WQXGA - 2560 x 1536 */
	CE147_CAPTURE_5MP,	/* 2560	x 1920 */
};

struct ce147_enum_framesize {
	/* mode	is 0 for preview, 1 for capture */
	enum ce147_oprmode mode;
	unsigned int index;
	unsigned int width;
	unsigned int height;
};

static struct ce147_enum_framesize ce147_framesize_list[] = {
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_QCIF,	 176,  144 },
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_QVGA,	 320,  240 },
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_592x480,	 592,  480 },
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_VGA,	 640,  480 },
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_D1,	 720,  480 },
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_WVGA,	 800,  480 },
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_720P,	1280,  720 },
	{ CE147_OPRMODE_VIDEO, CE147_PREVIEW_VERTICAL_QCIF,	144,	176},
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_VGA,	 640,  480 },
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_WVGA,	 800,  480 },
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_W1MP,	1600,  960 },
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_2MP,	1600, 1200 },
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_W2MP,	2048, 1232 },
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_3MP,	2048, 1536 },
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_W4MP,	2560, 1536 },
	{ CE147_OPRMODE_IMAGE, CE147_CAPTURE_5MP,	2560, 1920 },
};

struct ce147_version {
	unsigned int major;
	unsigned int minor;
};

struct ce147_date_info {
	unsigned int year;
	unsigned int month;
	unsigned int date;
};

enum ce147_runmode {
	CE147_RUNMODE_NOTREADY,
	CE147_RUNMODE_IDLE,
	CE147_RUNMODE_READY,
	CE147_RUNMODE_RUNNING,
};

struct ce147_firmware {
	unsigned int addr;
	unsigned int size;
};

/* Camera functional setting values configured by user concept */
struct ce147_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;		/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;		/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;		/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;		/* Color FX (AKA Color tone) */
	unsigned int contrast;		/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int glamour;
};

struct ce147_jpeg_param {
	unsigned int enable;
	unsigned int quality;
	unsigned int main_size;	 /* Main JPEG file size */
	unsigned int thumb_size; /* Thumbnail file size */
	unsigned int main_offset;
	unsigned int thumb_offset;
	unsigned int postview_offset;
} ;

struct ce147_position {
	int x;
	int y;
} ;

struct ce147_gps_info {
	unsigned char ce147_gps_buf[8];
	unsigned char ce147_altitude_buf[4];
	unsigned long gps_timeStamp;
	char gps_processingmethod[50];
};

struct ce147_sensor_maker {
	unsigned int maker;
	unsigned int optical;
};

struct ce147_version_af {
	unsigned int low;
	unsigned int high;
};

struct ce147_gamma {
	unsigned int rg_low;
	unsigned int rg_high;
	unsigned int bg_low;
	unsigned int bg_high;
};

#if 0
struct tm {
   int	   tm_sec;	   /* seconds */
   int	   tm_min;	   /* minutes */
   int	   tm_hour;	   /* hours */
   int	   tm_mday;	   /* day of the month */
   int	   tm_mon;	   /* month */
   int	   tm_year;	   /* year */
   int	   tm_wday;	   /* day of the week */
   int	   tm_yday;	   /* day in the year */
   int	   tm_isdst;	   /* daylight saving time */

   long	int tm_gmtoff;	   /* Seconds east of UTC. */
   const char *tm_zone;	   /* Timezone abbreviation. */
};
#endif

struct gps_info_common {
	unsigned int	direction;
	unsigned int	dgree;
	unsigned int	minute;
	unsigned int	second;
};

struct ce147_state {
	struct ce147_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct ce147_userset userset;
	struct ce147_jpeg_param jpeg;
	struct ce147_version fw;
	struct ce147_version prm;
	struct ce147_date_info dateinfo;
	struct ce147_firmware fw_info;
	struct ce147_position position;
	struct ce147_sensor_maker sensor_info;
	struct ce147_version_af af_info;
	struct ce147_gamma gamma;
	struct v4l2_streamparm strm;
	struct ce147_gps_info gpsInfo;
	struct mutex ctrl_lock;
	struct completion af_complete;
	enum ce147_runmode runmode;
	enum ce147_oprmode oprmode;
	int framesize_index;
	int sensor_version;
	int freq;	/* MCLK in Hz */
	int fps;
	int ot_status;
	int sa_status;
	int anti_banding;
	int preview_size;
	unsigned int fw_dump_size;
	int hd_preview_on;
	int pre_focus_mode;
	enum af_operation_status af_status;
	struct ce147_version main_sw_fw;
	struct ce147_version main_sw_prm;
	struct ce147_date_info main_sw_dateinfo;
	int exif_orientation_info;
	int check_dataline;
	int hd_slow_ae;
	int hd_gamma;
	int iso;
	int metering;
	int ev;
	int effect;
	int wb;
	struct tm *exifTimeInfo;
#if defined(CONFIG_ARIES_NTT) /* Modify	NTTS1 */
	int disable_aeawb_lock;
#endif
	int exif_ctrl;
	int thumb_null;
};

static int condition;

static const struct v4l2_fmtdesc capture_fmts[] = {
	{
		.index		= 0,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_COMPRESSED,
		.description	= "JPEG	+ Postview",
		.pixelformat	= V4L2_PIX_FMT_JPEG,
	},
};

static inline struct ce147_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ce147_state, sd);
}

/**
 * ce147_i2c_write_multi: Write (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 *
 * Returns 0 on success, <0 on error
 */
static int ce147_i2c_write_multi(struct i2c_client *client, unsigned char cmd,
		unsigned char *w_data, unsigned int w_len)
{
	int retry_count = 1;
	unsigned char buf[w_len	+ 1];
	struct i2c_msg msg = { client->addr, 0, w_len + 1, buf };

	int ret = -1;

	buf[0] = cmd;
	memcpy(buf + 1, w_data, w_len);

#ifdef CE147_DEBUG
	{
		int j;
		pr_debug("W: ");
		for (j = 0; j <= w_len; j++)
			pr_debug("0x%02x ", buf[j]);
		pr_debug("\n");
	}
#endif

	while (retry_count--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)
			break;
		msleep(POLL_TIME_MS);
	}

	return (ret == 1) ? 0 : -EIO;
}

/**
 * ce147_i2c_read_multi: Read (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 * @r_data: buffer where data is read
 * @r_len: number of bytes to read
 *
 * Returns 0 on success, <0 on error
 */
static int ce147_i2c_read_multi(struct i2c_client *client, unsigned char cmd,
		unsigned char *w_data, unsigned int w_len,
		unsigned char *r_data, unsigned int r_len)
{
	unsigned char buf[w_len + 1];
	struct i2c_msg msg = { client->addr, 0, w_len + 1, buf };
	int ret = -1;
	int retry_count = 1;

	buf[0] = cmd;
	memcpy(buf + 1, w_data, w_len);

#ifdef CE147_DEBUG
	{
		int j;
		pr_debug("R: ");
		for (j = 0; j <= w_len; j++)
			pr_debug("0x%02x ", buf[j]);
		pr_debug("\n");
	}
#endif

	while (retry_count--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)
			break;
		msleep(POLL_TIME_MS);
	}

	if (ret < 0)
		return -EIO;

	msg.flags = I2C_M_RD;
	msg.len	= r_len;
	msg.buf	= r_data;

	retry_count = 1;
	while (retry_count--) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)
			break;
		msleep(POLL_TIME_MS);
	}

	return (ret == 1) ? 0 : -EIO;
}

/**
 * ce147_power_en: Enable or disable the camera sensor power
 * Use only for updating camera firmware
 * Returns 0 forever
 */
static int ce147_power_en(int onoff, struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_platform_data *pdata;

	pdata =	client->dev.platform_data;

	if (onoff == 1)
		pdata->power_en(1);
	else
		pdata->power_en(0);

	return 0;
}

/**
 *  This function checks the status of the camera sensor by polling
 *  through the 'cmd' command register.
 *
 *  'polling_interval' is the delay between successive polling events
 *
 *  The function returns if
 *	o 'timeout' (in ms) expires
 *	o the data read	from device matches 'value'
 *
 *  On success it returns the time taken for successful wait.
 *  On failure, it returns -EBUSY.
 *  On I2C related failure, it returns -EIO.
 */
static int ce147_waitfordone_timeout(struct i2c_client *client,
					unsigned char cmd, unsigned char value,
					int timeout, int polling_interval)
{
	int err;
	unsigned char cam_status = 0xFF;
	unsigned long jiffies_start = jiffies;
	unsigned long jiffies_timeout =
				jiffies_start + msecs_to_jiffies(timeout);

	if (polling_interval < 0)
		polling_interval = POLL_TIME_MS;

	while (time_before(jiffies, jiffies_timeout)) {
		cam_status = 0xFF;
		err = ce147_i2c_read_multi(client,
						cmd, NULL, 0, &cam_status, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_read "
				"i2c error \n", __func__);
			return -EIO;
		}

		ce147_msg(&client->dev, "Status check returns %02x\n",
				cam_status);

		if (cam_status == value)
			break;

		msleep(polling_interval);
	}

	if (cam_status != value) {
		dev_err(&client->dev, "%s: failed: i2c_read "
				"time out occured \n", __func__);
		return -EBUSY;
	}
	else
		return jiffies_to_msecs(jiffies - jiffies_start);
}

static int ce147_get_batch_reflection_status(struct v4l2_subdev *sd)
{
	int err;
	int end_cnt = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_batch_data[1] = { 0x00 };
	unsigned char ce147_batch_ref_status = 0x00;

	err = ce147_i2c_write_multi(client, 0x01, ce147_buf_batch_data, 1);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write "
				"forget_batch_reflection_status\n", __func__);
		return -EIO;
	}

	/* TODO: This code needs timeout API for do-while */
	do {
		msleep(10);
		err = ce147_i2c_read_multi(client,
			CMD_GET_BATCH_REFLECTION_STATUS, NULL, 0,
			&ce147_batch_ref_status, 1);

		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_read "
					"for get_batch_reflection_status\n",
					__func__);
			return -EIO;
		}
		end_cnt++;
	} while	(ce147_batch_ref_status	&& end_cnt < 200);

	if (end_cnt > 5) {
		ce147_msg(&client->dev, "%s: count(%d) status(%02x)\n",
				__func__, end_cnt, ce147_batch_ref_status);
	}

	if (ce147_batch_ref_status != 0x00) {
		dev_err(&client->dev, "%s: failed: "
				"to get_batch_reflection_status\n", __func__);
		return -EINVAL;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_read_fw_bin(const char *path, char *fwBin, int *fwSize)
{
	char *buffer = NULL;
	unsigned int file_size = 0;

	struct file *filep = NULL;
	mm_segment_t old_fs;

	filep = filp_open(path, O_RDONLY, 0) ;

	if (filep && (filep != ERR_PTR(-ENOENT))) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		file_size = filep->f_op->llseek(filep, 0, SEEK_END);
		filep->f_op->llseek(filep, 0, SEEK_SET);

		buffer = kmalloc(file_size + 1, GFP_KERNEL);

		filep->f_op->read(filep, buffer, file_size, &filep->f_pos);
		buffer[file_size] = '\0';

		filp_close(filep, current->files);

		set_fs(old_fs);

		pr_debug("File size : %d\n", file_size);
	} else
		return -EINVAL;

	memcpy(fwBin, buffer, file_size);
	*fwSize	= file_size;

	kfree(buffer);

	return 0;
}

static int ce147_get_main_sw_fw_version(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);
	char fw_data[20] = { 0, };
	int fw_size = 0;
	int main_sw_fw_prm_offset = 4;
	int main_sw_date_offset = 10;
	int err = 0;
#if 0
	int i;
#endif

	pr_debug("ce147_get_main_sw_fw_version Enter\n");

	if ((MAIN_SW_DATE_INFO[0] == 0x00) && (MAIN_SW_DATE_INFO[1] == 0x00)
			&& (MAIN_SW_DATE_INFO[2] == 0x00)) {
		err = ce147_read_fw_bin(CE147_FW_F2_PATH, fw_data, &fw_size);
		if (err < 0) {
			pr_debug("fail : read main_sw_version\n");
			return err;
		}

#if 0
		pr_debug("fw_size : %d\n", fw_size);

		for (i = 0; i < fw_size; i++)
			pr_debug("fw_data : %x\n", fw_data[i]);
#endif

		/* Main	SW FW/PRM info */
		MAIN_SW_FW[0] = fw_data[main_sw_fw_prm_offset];
		MAIN_SW_FW[1] = fw_data[main_sw_fw_prm_offset + 1];
		MAIN_SW_FW[2] = fw_data[main_sw_fw_prm_offset + 2];
		MAIN_SW_FW[3] = fw_data[main_sw_fw_prm_offset + 3];

		/* Main	SW Date info */
		MAIN_SW_DATE_INFO[0] = (2000 + fw_data[main_sw_date_offset]);
		MAIN_SW_DATE_INFO[1] = fw_data[main_sw_date_offset + 1];
		MAIN_SW_DATE_INFO[2] = fw_data[main_sw_date_offset + 2];

		pr_debug("fw M:%d m:%d |prm M:%d m:%d\n",
				MAIN_SW_FW[0], MAIN_SW_FW[1],
				MAIN_SW_FW[2], MAIN_SW_FW[3]);
		pr_debug("y. m. d = %d.%d.%d\n",
				MAIN_SW_DATE_INFO[0], MAIN_SW_DATE_INFO[1],
				MAIN_SW_DATE_INFO[2]);
	} else
		pr_debug("already read main sw version\n");

	state->main_sw_fw.major = MAIN_SW_FW[0];
	state->main_sw_fw.minor = MAIN_SW_FW[1];
	state->main_sw_prm.major = MAIN_SW_FW[2];
	state->main_sw_prm.minor = MAIN_SW_FW[3];
	state->main_sw_dateinfo.year = MAIN_SW_DATE_INFO[0];
	state->main_sw_dateinfo.month = MAIN_SW_DATE_INFO[1];
	state->main_sw_dateinfo.date = MAIN_SW_DATE_INFO[2];

	return 0;
}

static int ce147_load_fw(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int ce147_reglen_init = 1;
	unsigned char ce147_regbuf_init[1] = { 0x00 };
	int err;

	/* Just before this function call, we enable the power and clock. Hence
	 * we need to wait for some time before we can start communicating
	 * with the sensor.
	 */
	msleep(10);

	err = ce147_i2c_write_multi(client, CMD_INIT,
					ce147_regbuf_init, ce147_reglen_init);
	if (err < 0)
		return -EIO;

	/* At least 700ms delay required to load the firmware
	 * for ce147 camera ISP */
	msleep(700);

	state->runmode = CE147_RUNMODE_IDLE;

	return 0;
}


static int ce147_get_version(struct v4l2_subdev *sd, int object_id,
				unsigned char version_info[])
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char cmd_buf[1] = { 0x00 };
	unsigned int cmd_len = 1;
	unsigned int info_len = 4;
	int err;

	switch (object_id) {
	case DATA_VERSION_FW:
	case DATA_VERSION_DATE:
	case DATA_VERSION_SENSOR:
	case DATA_VERSION_SENSOR_MAKER:
	case DATA_VERSION_AF:
		cmd_buf[0] = object_id;
		break;
	default:
		return -EINVAL;
	}

	err = ce147_i2c_read_multi(client, CMD_VERSION, cmd_buf, cmd_len,
					version_info, info_len);
	if (err < 0)
		return -EIO;

	return 0;
}

static int ce147_get_fw_version(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);
	unsigned char version_info[4] = { 0x00,	0x00, 0x00, 0x00 };
	int err = -1;

	err = ce147_get_version(sd, DATA_VERSION_FW, version_info);

	if (err < 0)
		return err;

	state->fw.minor	= version_info[0];
	state->fw.major	= version_info[1];

	state->prm.minor = version_info[2];
	state->prm.major = version_info[3];

	return 0;
}

static int ce147_get_dateinfo(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);
	unsigned char version_info[4] = { 0x00,	0x00, 0x00, 0x00 };
	int err = -1;

	err = ce147_get_version(sd, DATA_VERSION_DATE, version_info);

	if (err < 0)
		return err;

	state->dateinfo.year  = version_info[0] - 'A' + 2007;
	state->dateinfo.month = version_info[1] - 'A' + 1;
	state->dateinfo.date  = version_info[2];

	return 0;
}

static int ce147_get_sensor_version(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);
	unsigned char version_info[4] = { 0x00, 0x00, 0x00, 0x00 };
	int err = -1;

	err = ce147_get_version(sd, DATA_VERSION_SENSOR, version_info);

	if (err < 0)
		return err;

	state->sensor_version = version_info[0];

	return 0;
}

static int ce147_get_sensor_maker_version(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);
	unsigned char version_info[4] = { 0x00, 0x00, 0x00, 0x00 };
	int err = -1;

	err = ce147_get_version(sd, DATA_VERSION_SENSOR_MAKER, version_info);

	if (err < 0)
		return err;

	state->sensor_info.maker = version_info[0];
	state->sensor_info.optical = version_info[1];

	return 0;
}

static int ce147_get_af_version(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);
	unsigned char version_info[4] =	{ 0x00, 0x00, 0x00, 0x00 };
	int err = -1;

	err = ce147_get_version(sd, DATA_VERSION_AF, version_info);

	if (err < 0)
		return err;

	/* pr_debug("ce147_get_af_version: data0: 0x%02x, data1: 0x%02x\n",
			version_info[0], version_info[1]); */

	state->af_info.low = version_info[1];
	state->af_info.high = version_info[0];

	return 0;
}

static int ce147_get_gamma_version(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	unsigned char gamma_info[2] = {	0x00, 0x00 };
	unsigned int info_len = 2;
	int err = -1;

	unsigned char rg_low_buf[2] = {	0x0C, 0x00 };
	unsigned char rg_high_buf[2] = { 0x0D, 0x00 };
	unsigned char bg_low_buf[2] = {	0x0E, 0x00 };
	unsigned char bg_high_buf[2] = { 0x0F, 0x00 };
	unsigned int buf_len = 2;


	err = ce147_i2c_read_multi(client, DATA_VERSION_GAMMA,
				rg_low_buf, buf_len, gamma_info, info_len);
	if (err < 0)
		return -EIO;

	state->gamma.rg_low = gamma_info[1];
	/* pr_debug("ce147_get_gamma_version1: data1: 0x%02x, data1: 0x%02x\n",
			gamma_info[0], gamma_info[1]); */

	err = ce147_i2c_read_multi(client, DATA_VERSION_GAMMA,
			rg_high_buf, buf_len, gamma_info, info_len);
	if (err < 0)
		return -EIO;

	state->gamma.rg_high = gamma_info[1];
	/* pr_debug("ce147_get_gamma_version1: data1: 0x%02x, data1: 0x%02x\n",
			gamma_info[0], gamma_info[1]); */

	err = ce147_i2c_read_multi(client, DATA_VERSION_GAMMA,
			bg_low_buf, buf_len, gamma_info, info_len);
	if (err < 0)
		return -EIO;

	state->gamma.bg_low = gamma_info[1];
	/* pr_debug("ce147_get_gamma_version1: data1: 0x%02x, data1: 0x%02x\n",
			gamma_info[0], gamma_info[1]); */

	err = ce147_i2c_read_multi(client, DATA_VERSION_GAMMA,
			bg_high_buf, buf_len, gamma_info, info_len);
	if (err < 0)
		return -EIO;

	state->gamma.bg_high = gamma_info[1];
	/* pr_debug("ce147_get_gamma_version1: data1: 0x%02x, data1: 0x%02x\n",
			gamma_info[0], gamma_info[1]); */

	return 0;
}

#ifndef	MDNIE_TUNING
static int ce147_update_fw(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	unsigned char *mbuf = NULL;
	unsigned char *fw_buf[4];
	int fw_size[4];
	int index = 0;
	int i = 0;
	int err;

	const unsigned int packet_size = 129; /* Data 128 + Checksum 1 */
	unsigned int packet_num, k, j = 0, l = 0;
	unsigned char res = 0x00;
	unsigned char data[129];
	unsigned char data2[129];

	/* dev_err(&client->dev, "%s: ce147_fw: buf = 0x%p, len = %d\n",
		__func__, (void*)state->fw_info.addr, state->fw_info.size); */

	mbuf = vmalloc(state->fw_info.size);

	if (NULL == mbuf)
		return -ENOMEM;

	if (copy_from_user(mbuf, (void *)state->fw_info.addr,
				state->fw_info.size)) {
		err = -EFAULT;
		goto out;
	}

	/* The firmware	buffer is now copied to mbuf,
	 * so the firmware code is now in mbuf.
	 * We can use mbuf with i2c_tranfer call
	 */
	for (i = 0; i < 4; i++) {
		if (index > state->fw_info.size - 4) {
			dev_err(&client->dev, "%s:Error size parameter\n",
					__func__);
			break;
		}
		memcpy(fw_size + i, mbuf + index, 4);
		index += 4;
		fw_buf[i] = mbuf + index;
		index += ((fw_size[i] - 1) & (~0x3)) + 4;
		dev_err(&client->dev, "%s: [%d] fw_size = %d, fw_buf = 0x%p\n",
				__func__, i, fw_size[i], fw_buf[i]);
	}

	err = ce147_power_en(1, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(on)\n",
				__func__);
		err = -EIO;
		goto out;
	}

	msleep(100);

	/* [1] set fw updater info */
	err = ce147_i2c_write_multi(client, CMD_FW_INFO, fw_buf[0], 4);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for 0xf2, "
				"fw_size[0]: %d, fw_buf[0]: 0x%02x\n",
				__func__, fw_size[0],
				(unsigned int)(fw_buf[0]));
		err = -EIO;
		goto out;
	}
	msleep(100);

	/* pr_debug("ce147_update_fw: i2c_write for 0xf2, "
			"fw_size[0]: %d, fw_buf[0]: 0x%02x\n",
			fw_size[0], fw_buf[0]); */

	packet_num = *(fw_buf[0]) + (*(fw_buf[0]+1)<<8);

	/* [2] update firmware */
	for (k = 0; k < packet_num; k++) {
		memcpy(&data[0], fw_buf[1]+j, packet_size);
		err = ce147_i2c_read_multi(client, CMD_FWU_UPDATE,
				data, packet_size, &res, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: fail: i2c_read for 0xf3, "
					"data: 0x%02x\n", __func__, data[0]);
			err = -EIO;
			goto out;
		}
		msleep(10);
		j += 129;
		/* pr_debug("ce147_update_fw: i2c_read for 0xf3, "
				"data: 0x%02x, count: %d\n", data[0], k); */
	}

	k = 0;
	/* [3] get fw status */
	do {
		msleep(100);

		err = ce147_i2c_read_multi(client, CMD_FW_STATUS, NULL,	0,
				&res, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: fail: i2c_read for 0xf5",
					__func__);
			err = -EIO;
			goto out;
		}
		/* pr_debug("ce147_update_fw: i2c_read for 0xf5, "
				"data: 0x%02x\n", res); */

		k++;
		if (k == 500)
			break;
	} while (res !=	0x05);

	msleep(500);

	fw_size[2] = 4;

	/* [4] set fw updater info */
	err = ce147_i2c_write_multi(client, CMD_FW_INFO, fw_buf[2], fw_size[2]);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for 0xf2, "
				"fw_size[2]: %d, fw_buf[2]: 0x%02x\n",
				__func__, fw_size[2],
				(unsigned int)(fw_buf[2]));
		err = -EIO;
		goto out;
	}
	msleep(100);

	/* pr_debug("ce147_update_fw: i2c_write for 0xf2, fw_size[2]: %d,
	   fw_buf[2]: 0x%02x\n", fw_size[2], fw_buf[2]); */

	packet_num = *(fw_buf[2]) + (*(fw_buf[2] + 1) << 8);

	/* pr_debug("ce147_update_fw: packet_num: %d\n", packet_num); */

	j = 0;

	/* [5] update firmware */
	for (l = 0; l < packet_num; l++) {
		memcpy(&data2[0], fw_buf[3] + j, packet_size);
		err = ce147_i2c_write_multi(client, CMD_FW_UPDATE,
						data2, packet_size);
		if (err < 0) {
				dev_err(&client->dev, "%s: fail: i2c_read "
						"for 0xf4, data:2 0x%02x\n",
							__func__, data2[0]);
			err = -EIO;
			goto out;
		}

		/* pr_debug("ce147_update_fw: i2c_write for 0xf4, "
				"data2:	0x%02x, count: %d\n", data2[0], l); */

		msleep(10);
		j += 129;
	}

	l = 0;
	/* [6] get fw status */
	do {
		msleep(100);

		err = ce147_i2c_read_multi(client, CMD_FW_STATUS,
				NULL, 0, &res, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: fail: i2c_read for 0xf5",
					__func__);
			err = -EIO;
			goto out;
		}
		/* pr_debug("ce147_update_fw: i2c_read for 0xf5, "
				"data: 0x%02x\n", res); */

		l++;
		if (l == 500)
			break;
	} while (res != 0x06);

	vfree(mbuf);

	err = ce147_power_en(0, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(off)\n",
					__func__);
		return -EIO;
	}

	dev_err(&client->dev, "%s: ce147_power_en(off)\n", __func__);

	return 0;
out:
	vfree(mbuf);

	return err;
}

#else
unsigned short *test[1];
EXPORT_SYMBOL(test);
extern void mDNIe_txtbuf_to_parsing(void);
extern void mDNIe_txtbuf_to_parsing_for_lightsensor(void);
extern void mDNIe_txtbuf_to_parsing_for_backlight(void);
static int ce147_update_fw(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	unsigned char *mbuf = NULL;
	unsigned char *fw_buf[4];
	int fw_size[4];
	int index = 0;
	int i =	0;
	int err;

	const unsigned int packet_size = 129; /* Data 128 + Checksum 1 */
	unsigned int packet_num, k, j, l = 0;
	unsigned char res = 0x00;
	unsigned char data[129];
	unsigned char data2[129];

	/* dev_err(&client->dev, "%s: ce147_fw: buf = 0x%p, len = %d\n",
			__func__, (void*)state->fw_info.addr,
			state->fw_info.size); */

	mbuf = vmalloc(state->fw_info.size);

	if (!mbuf)
		return -ENOMEM;

	if (copy_from_user(mbuf, (void *)state->fw_info.addr,
					state->fw_info.size)) {
		vfree(mbuf);
		return -EFAULT;
	}

	/* The firmware	buffer is now copied to mbuf,
	 * so the firmware code	is now in mbuf.
	 * We can use mbuf with i2c_tranfer call */
	for (i = 0; i < 4; i++) {
		if (index > state->fw_info.size - 4) {
			dev_err(&client->dev, "%s:Error	size parameter\n",
					__func__);
			break;
		}
		memcpy(fw_size + i, mbuf + index, 4);
		index += 4;
		fw_buf[i] = mbuf + index;
		index += ((fw_size[i] -	1) & (~0x3)) + 4;
		dev_err(&client->dev, "%s: [%d] fw_size = %d, fw_buf = 0x%p\n",
				__func__, i, fw_size[i], fw_buf[i]);
	}

	test[0] = fw_buf[0];

	for (j = 0; j < fw_size[0]; j++) {
		pr_debug("ce147_update_fw: , fw_size[0]: %d, test[0]: 0x%x\n",
				fw_size[0], test[0][j]);
		test[0][j] = ((test[0][j] & 0xff00) >> 8) |
				((test[0][j] & 0x00ff) << 8);
		pr_debug("ce147_update_fw: , test1[0]: 0x%x\n", test[0][j]);
	}

	/* for mdnie tuning */

	mDNIe_txtbuf_to_parsing();
	/* mDNIe_txtbuf_to_parsing_for_lightsensor(); */
	/* mDNIe_txtbuf_to_parsing_for_backlight(); */

	return 0;
}
#endif

static int ce147_dump_fw(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	unsigned char *mbuf = NULL;
	unsigned char *fw_buf[4];
	int fw_size[4];
	int index = 0;
	int i = 0;
	int err;

	const unsigned int packet_size = 129; /* Data 128 + Checksum 1 */
	unsigned int packet_num, k, j =	0, l = 0;
	unsigned char res = 0x00;
	unsigned char data[129];
	unsigned char data2[130];
	unsigned char addr[4] = { 0x03,	0x00, 0x00, 0x01 };
	unsigned int addr_len = 4;
	unsigned char dump[1] = { 0x00 };

	/* dev_err(&client->dev, "%s: ce147_fw: buf = 0x%p, len = %d\n",
		__func__, (void*)state->fw_info.addr, state->fw_info.size); */

	mbuf = vmalloc(state->fw_info.size);

	if (NULL == mbuf)
		return -ENOMEM;

	if (copy_from_user(mbuf, (void *)state->fw_info.addr,
					state->fw_info.size)) {
		err = -EFAULT;
		goto out;
	}

	/* The firmware	buffer is now copied to mbuf,
	 * so the firmware code is now in mbuf.
	 * We can use mbuf with i2c_tranfer call */
	for (i = 0; i <	4; i++) {
		if (index > state->fw_info.size	- 4) {
			dev_err(&client->dev, "%s:Error size parameter\n",
					__func__);
			break;
		}
		memcpy(fw_size+i, mbuf + index, 4);
		index += 4;
		fw_buf[i] = mbuf + index;
		index += ((fw_size[i] -	1) & (~0x3)) + 4;
		dev_err(&client->dev, "%s: [%d] fw_size	= %d, fw_buf = 0x%p\n",
				__func__, i, fw_size[i], fw_buf[i]);
	}

	err = ce147_power_en(1, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(on)\n",
				__func__);
		err = -EIO;
		goto out;
	}

	msleep(100);

	/* [1] set fw updater info */
	err = ce147_i2c_write_multi(client, CMD_FW_INFO, fw_buf[0], 4);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for 0xf2, "
				"fw_size[0]: %d, fw_buf[0]: 0x%02x\n",
					__func__, fw_size[0],
					(unsigned int)(fw_buf[0]));
		err = -EIO;
		goto out;
	}
	msleep(100);

	/* pr_debug("ce147_update_fw: i2c_write for 0xf2, fw_size[0]: %d, "
			"fw_buf[0]: 0x%02x\n", fw_size[0], fw_buf[0]); */

	packet_num = *(fw_buf[0]) + (*(fw_buf[0] + 1) << 8);

	/* [2] update firmware */
	for (k = 0; k < packet_num; k++) {
		memcpy(&data[0], fw_buf[1] + j, packet_size);
		err = ce147_i2c_read_multi(client, CMD_FWU_UPDATE,
				data, packet_size, &res, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: fail: i2c_read for 0xf3, "
					"data: 0x%02x\n", __func__, data[0]);
			err = -EIO;
			goto out;
		}
		msleep(10);
		j += 129;
		/* pr_debug("ce147_update_fw: i2c_read for 0xf3, data: 0x%02x, "
				"count:	%d\n", data[0], k); */
	}

	k = 0;
	/* [3] get fw status */
	do {
		msleep(100);

		err = ce147_i2c_read_multi(client, CMD_FW_STATUS, NULL,	0,
						&res, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: fail: i2c_read for 0xf5",
					__func__);
			err = -EIO;
			goto out;
		}
		/* pr_debug("ce147_update_fw: i2c_read for 0xf5, "
				"data: 0x%02x\n", res); */

		k++;
		if (k == 500)
			break;
	} while (res != 0x05);

	msleep(500);

	/* [4] change from dump mode */
	err = ce147_i2c_write_multi(client, CMD_FW_DUMP, dump, 1);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for 0xfb, 0x00",
				__func__);
		err = -EIO;
		goto out;
	}
	msleep(100);

	dump[0] = 0x02;

	/* [5] check fw	mode is in dump	mode */
	err = ce147_i2c_read_multi(client, CMD_FW_DUMP, dump, 1, &res, 1);
	if (err < 0) {
		dev_err(&client->dev, "%s: fail: i2c_read for 0xfb", __func__);
		err = -EIO;
		goto out;
	}

	if (res != 1) {
		dev_err(&client->dev, "%s: fail: res is %x", __func__, res);
		err = -EIO;
		goto out;
	}

	msleep(100);

	/* [6] set dump start address */
	err = ce147_i2c_write_multi(client, CMD_FW_DUMP, addr, addr_len);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for 0xfb, 0x03",
				__func__);
		err = -EIO;
		goto out;
	}
	msleep(100);

	j = 0;

	packet_num = *(fw_buf[2]) + (*(fw_buf[2] + 1) << 8);
	/* pr_debug("ce147_update_fw: i2c_read for 0xfb, packet_num: %d\n",
			packet_num); */

	dump[0] = 0x04;

	/* [7] dump firmware data */
	for (l = 0; l < packet_num; l++) {
		err = ce147_i2c_read_multi(client, CMD_FW_DUMP, dump, 1,
						data2, packet_size + 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: fail: i2c_read for 0xfb, "
					"0x04\n", __func__);
			err = -EIO;
			goto out;
		}
		memcpy(fw_buf[3] + j, &data2[0], packet_size - 1);

		msleep(10);
		j += 129;
		/* pr_debug("ce147_update_fw: i2c_read for 0xfb, count: %d\n",
				l); */
	}

	state->fw_dump_size = packet_num * packet_size;

	if (copy_to_user((void *)(state->fw_info.addr), fw_buf[3],
				state->fw_dump_size)) {
		err = -EIO;
		goto out;
	}

	vfree(mbuf);

	err = ce147_power_en(0, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(off)\n",
				__func__);
		return -EIO;
	}

	dev_err(&client->dev, "%s: ce147_power_en(off)\n", __func__);

	return 0;
out:
	vfree(mbuf);

	return err;
}

static int ce147_check_dataline(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_check_dataline[2] = { 0x01, 0x01 };
	unsigned int ce147_len_check_dataline = 2;

	err = ce147_i2c_write_multi(client, CMD_CHECK_DATALINE,
			ce147_buf_check_dataline, ce147_len_check_dataline);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"check_dataline\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_check_dataline_stop(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_check_dataline[2] = { 0x00, 0x00 };
	unsigned int ce147_len_check_dataline = 2;

	err = ce147_i2c_write_multi(client, CMD_CHECK_DATALINE,
			ce147_buf_check_dataline, ce147_len_check_dataline);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"check_dataline stop\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_preview_size(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int index = state->framesize_index;
	/* Default VGA resolution = { 0x04, 0x01 } */
	unsigned char ce147_regbuf_preview_size[2] = { 0x04, 0x01 };
	unsigned int ce147_reglen_preview_size = 2;
	unsigned char ce147_regbuf_hd_preview[1] = { 0x00 };
	unsigned int ce147_reglen_hd_preview = 1;

	ce147_msg(&client->dev, "%s: index = %d\n", __func__, index);

	switch (index) {
	case CE147_PREVIEW_QCIF:
		ce147_regbuf_preview_size[0] = 0x1E;
		break;
	case CE147_PREVIEW_QVGA:
		ce147_regbuf_preview_size[0] = 0x02;
		break;

	case CE147_PREVIEW_592x480:
		ce147_regbuf_preview_size[0] = 0x24;
		break;

	case CE147_PREVIEW_VGA:
		ce147_regbuf_preview_size[0] = 0x04;
		break;
	case CE147_PREVIEW_WVGA:
		ce147_regbuf_preview_size[0] = 0x13;
		break;
	case CE147_PREVIEW_D1:
		ce147_regbuf_preview_size[0] = 0x20;
		break;
	case CE147_PREVIEW_720P:
		ce147_regbuf_preview_size[0] = 0x16;
		ce147_regbuf_preview_size[1] = 0x02;
		break;
	case CE147_PREVIEW_VERTICAL_QCIF:
		ce147_regbuf_preview_size[0] = 0x26;
		break;
	default:
		/* When	running in image capture mode, the call comes here.
		 * Set the default video resolution - CE147_PREVIEW_VGA
		 */
		ce147_msg(&client->dev,	"Setting preview resoution as VGA "
				"for image capture mode\n");
		break;
	}

	if (index == CE147_PREVIEW_720P) {
		ce147_regbuf_hd_preview[0] = 0x01;
		state->hd_preview_on = 1;
		pr_info("%s: preview_size is HD (%d)\n",
				__func__, state->hd_preview_on);
		err = ce147_i2c_write_multi(client, CMD_HD_PREVIEW,
				ce147_regbuf_hd_preview,
				ce147_reglen_hd_preview);
		if (err < 0)
			return -EIO;
	} else {
		state->hd_preview_on = 0;
		pr_info("%s: preview_size is not HD (%d)\n",
				__func__, state->hd_preview_on);
		err = ce147_i2c_write_multi(client, CMD_HD_PREVIEW,
				ce147_regbuf_hd_preview,
				ce147_reglen_hd_preview);
		if (err < 0)
			return -EIO;
	}
	mdelay(5);

	err = ce147_i2c_write_multi(client, CMD_PREVIEW_SIZE,
			ce147_regbuf_preview_size, ce147_reglen_preview_size);
	if (err < 0) {
		pr_info("%s: preview_size is not HD (%d)\n",
				__func__, state->hd_preview_on);
		return -EIO;
	}

	ce147_msg(&client->dev, "Done\n");

	return err;
}

static int ce147_set_frame_rate(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_regbuf_fps[2] = { 0x1E, 0x00 };
	unsigned int ce147_reglen_fps =	2;

	switch (state->fps) {
	case FRAME_RATE_7:
		ce147_regbuf_fps[0] = 0x07;
		break;

	case FRAME_RATE_10:
		ce147_regbuf_fps[0] = 0x0A;
		break;

	case FRAME_RATE_15:
		ce147_regbuf_fps[0] = 0x0F;
		break;

	case FRAME_RATE_20:
		ce147_regbuf_fps[0] = 0x14;
		break;

	case FRAME_RATE_30:
		ce147_regbuf_fps[0] = 0x1E;
		break;

	case FRAME_RATE_60:
		ce147_regbuf_fps[0] = 0x3C;
		break;

	case FRAME_RATE_120:
		ce147_regbuf_fps[0] = 0x78;
		break;

	default:		
		return -EINVAL;
	}

	err = ce147_i2c_write_multi(client, CMD_FPS, ce147_regbuf_fps,
					ce147_reglen_fps);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write "
				"for set_frame_rate\n", __func__);
		return -EIO;
	}
#if 0 /* remove	batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_reflection"
				"_status for set_frame_rate\n", __func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_anti_banding(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_regbuf_anti_banding[1] = { 0x02 };
	unsigned int ce147_reglen_anti_banding = 1;

	switch (state->anti_banding) {
	case ANTI_BANDING_OFF:
		ce147_regbuf_anti_banding[0] = 0x00;
		break;

	case ANTI_BANDING_AUTO:
		ce147_regbuf_anti_banding[0] = 0x01;
		break;

	case ANTI_BANDING_50HZ:
		ce147_regbuf_anti_banding[0] = 0x02;
		break;

	case ANTI_BANDING_60HZ:
	default:
		ce147_regbuf_anti_banding[0] = 0x03;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_ANTI_BANDING,
			ce147_regbuf_anti_banding, ce147_reglen_anti_banding);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"anti_banding\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_preview_stop(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int ce147_reglen_preview = 1;
	unsigned char ce147_regbuf_preview_stop[1] = { 0x00 };
	int err;

	/* pr_debug("%s: (%d)\n", __func__, state->runmode); */

	if (CE147_RUNMODE_RUNNING == state->runmode) {
		err = ce147_i2c_write_multi(client, CMD_PREVIEW,
				ce147_regbuf_preview_stop,
				ce147_reglen_preview);
		if (err	< 0) {
			dev_err(&client->dev, "%s: failed: i2c_write "
					"for preview_stop\n", __func__);
			return -EIO;
		}

		msleep(POLL_TIME_MS);
		err = ce147_waitfordone_timeout(client, CMD_PREVIEW_STATUS,
						0x00, 3000, POLL_TIME_MS);
		if (err	< 0) {
			dev_err(&client->dev, "%s: Wait	for preview_stop "
					"failed\n", __func__);
			return err;
		}
		ce147_msg(&client->dev, "%s: preview_stop - wait time %d ms\n",
				__func__, err);

		state->runmode = CE147_RUNMODE_READY;
	}
	return 0;
}

static int ce147_set_dzoom(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct ce147_state *state = to_state(sd);
	int err;
	int count;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_get_dzoom_status[2] = { 0x00, 0x00 };
	unsigned int ce147_len_get_dzoom_status = 2;

	if (CE147_RUNMODE_RUNNING == state->runmode) {
		err = ce147_i2c_write_multi(client, CMD_SET_DZOOM,
				&ce147_buf_set_dzoom[ctrl->value], 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write "
					"for set_dzoom\n", __func__);
			return -EIO;
		}

		/* TODO: This code needs to use
		 * ce147_waitfordone_timeout() API
		 */
		for (count = 0; count <	300; count++) {
			err = ce147_i2c_read_multi(client, CMD_GET_DZOOM_LEVEL,
					NULL, 0, ce147_buf_get_dzoom_status,
					ce147_len_get_dzoom_status);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: i2c_read "
						"for set_dzoom\n", __func__);
				return -EIO;
			}
			if (ce147_buf_get_dzoom_status[1] == 0x00)
				break;
		}
	}

	DZoom_State = ctrl->value;

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_preview_start(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	struct v4l2_control ctrl;
	int ce147_reglen_preview = 1;
	unsigned char ce147_regbuf_preview_start[1] = { 0x01 };

	int count;
	unsigned char ce147_buf_get_dzoom_status[2] = { 0x00, 0x00 };
	unsigned int ce147_len_get_dzoom_status = 2;

	if (!state->pix.width || !state->pix.height || !state->fps)
		return -EINVAL;

	/* This	is for 15 testmode */
	if (state->check_dataline) {
		err = ce147_check_dataline(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: Could not check data"
					"line.\n", __func__);
			return -EIO;
		}
	} else { /* Normal preview sequence */
		/* Stop	it if it is already running */
		err = ce147_set_preview_stop(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: Could not stop the "
					"running preview.\n", __func__);
			return -EIO;
		}

		err = ce147_set_preview_size(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: Could not set "
					"preview size\n", __func__);
			return -EIO;
		}

		if (DZoom_State	!= 0) {
			err = ce147_i2c_write_multi(client, CMD_SET_DZOOM,
					&ce147_buf_set_dzoom[DZoom_State], 1);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: i2c_write "
						"for set_dzoom in "
						"preview_start\n", __func__);
				return -EIO;
			}

			for (count = 0;	count < 300; count++) {
				err = ce147_i2c_read_multi(client,
						CMD_GET_DZOOM_LEVEL, NULL, 0,
						ce147_buf_get_dzoom_status,
						ce147_len_get_dzoom_status);
				if (err	< 0) {
					dev_err(&client->dev, "%s: failed: "
							"i2c_read for set_dzoom"
							"in preview_start\n",
								__func__);
					return -EIO;
				}
				if (ce147_buf_get_dzoom_status[1] == 0x00)
					break;
			}
		}

		err = ce147_set_anti_banding(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: Could not set anti "
					"banding\n", __func__);
			return -EIO;
		}

		err = ce147_set_frame_rate(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: Could not set fps\n",
					__func__);
			return -EIO;
		}

		if (state->runmode != CE147_RUNMODE_READY) {
			/* iso */
			ctrl.value = state->iso;
			err = ce147_set_iso(sd, &ctrl);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: ce147_set_"
						"iso, err %d\n", __func__, err);
				return -EIO;
			}

			/* metering */
			ctrl.value = state->metering;
			err = ce147_set_metering(sd, &ctrl);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: ce147_set_"
						"metering, err %d\n",
							__func__, err);
				return -EIO;
			}

			/* ev */
			ctrl.value = state->ev;
			err = ce147_set_ev(sd, &ctrl);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: ce147_set_"
						"ev, err %d\n", __func__, err);
				return -EIO;
			}

			/* effect */
			ctrl.value = state->effect;
			err = ce147_set_effect(sd, &ctrl);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: ce147_set_"
						"effect, err %d\n",
							__func__, err);
				return -EIO;
			}

			/* wb */
			ctrl.value = state->wb;
			err = ce147_set_white_balance(sd, &ctrl);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: ce147_set_"
						"white_balance,	err %d\n",
							__func__, err);
				return -EIO;
			}
		}
		/* slow	ae */
		ctrl.value = state->hd_slow_ae;
		err = ce147_set_slow_ae(sd, &ctrl);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: ce147_set_slow_ae, "
					"err %d\n", __func__, err);
			return -EIO;
		}

		/* RGB gamma */
		ctrl.value = state->hd_gamma;
		err = ce147_set_gamma(sd, &ctrl);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: ce147_set_gamma, "
					"err %d\n", __func__, err);
			return -EIO;
		}

		/* batch reflection */
		err = ce147_get_batch_reflection_status(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: ce147_get_batch_"
					"reflection_status for "
					"set_frame_rate\n", __func__);
			return -EIO;
		}

		/* Release AWB unLock */
		err = ce147_set_awb_lock(sd, 0);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: ce147_set_awb_lock, "
					"err %d\n", __func__, err);
			return -EIO;
		}

		/* Start preview */
		err = ce147_i2c_write_multi(client, CMD_PREVIEW,
						ce147_regbuf_preview_start,
						ce147_reglen_preview);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"preview_start\n", __func__);
			return -EIO;
		}

		err = ce147_waitfordone_timeout(client,	CMD_PREVIEW_STATUS,
				0x08, 3000, POLL_TIME_MS);
		if (err < 0) {
			dev_err(&client->dev, "%s: Wait	for preview_start "
					"failed\n", __func__);
			return err;
		}
		ce147_msg(&client->dev,	"%s: preview_start - wait time %d ms\n",
				__func__, err);
	}

	state->runmode = CE147_RUNMODE_RUNNING;

	return 0;
}

static int ce147_set_capture_size(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	int index = state->framesize_index;
	/* Default 5 MP = { 0x08, 0x00,	0x01, 0x00 } */
	unsigned char ce147_regbuf_capture_size[4] = { 0x0B, 0x00, 0x01, 0x00 };
	unsigned int ce147_reglen_capture_size = 4;

	switch (index) {
	case CE147_CAPTURE_VGA: /* 640x480 */
		ce147_regbuf_capture_size[0] = 0x04;
		break;
	case CE147_CAPTURE_WVGA: /* 800x480 */
		ce147_regbuf_capture_size[0] = 0x13;
		break;
	case CE147_CAPTURE_W1MP: /* 1600x960 */
		ce147_regbuf_capture_size[0] = 0x0E;
		break;
	case CE147_CAPTURE_2MP: /* 1600x1200 */
		ce147_regbuf_capture_size[0] = 0x08;
		break;
	case CE147_CAPTURE_W2MP: /* 2048x1232 */
		ce147_regbuf_capture_size[0] = 0x0F;
		break;
	case CE147_CAPTURE_3MP: /* 2048x1536 */
		ce147_regbuf_capture_size[0] = 0x09;
		break;
	case CE147_CAPTURE_W4MP: /* 2560x1536 */
		ce147_regbuf_capture_size[0] = 0x15;
		break;
	case CE147_CAPTURE_5MP: /* 2560x1920 */
		ce147_regbuf_capture_size[0] = 0x0B;
		break;
	default:
		/* The framesize index was not set properly.
		 * Check s_fmt call - it must be for video mode. */
		return -EINVAL;
	}

	/* Set capture image size */
	err = ce147_i2c_write_multi(client, CMD_CAPTURE_SIZE,
			ce147_regbuf_capture_size, ce147_reglen_capture_size);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write "
				"for capture_resolution\n", __func__);
		return -EIO;
	}

	/* This is for postview */
	if (ce147_regbuf_capture_size[0] < 0x0C) {
		state->preview_size = CE147_PREVIEW_VGA;
		/* pr_debug("%s: preview_size is VGA (%d)\n",
				__func__, state->preview_size); */
	} else {
		state->preview_size = CE147_PREVIEW_WVGA;
		/* pr_debug("%s: preview_size is WVGA (%d)\n",
				__func__, state->preview_size); */
	}

	/* pr_debug("%s: 0x%02x\n", __func__, index); */

	return 0;
}

static int ce147_set_ae_awb(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_ae_awb[1] = { 0x00 };

	switch (ctrl->value) {
	case AE_LOCK_AWB_UNLOCK:
		ce147_buf_set_ae_awb[0]	= 0x01;
		break;

	case AE_UNLOCK_AWB_LOCK:
		ce147_buf_set_ae_awb[0]	= 0x10;
		break;

	case AE_LOCK_AWB_LOCK:
		ce147_buf_set_ae_awb[0]	= 0x11;
		break;

	case AE_UNLOCK_AWB_UNLOCK:
	default:
		ce147_buf_set_ae_awb[0]	= 0x00;
			break;
	}
	err = ce147_i2c_write_multi(client, CMD_AE_WB_LOCK,
					ce147_buf_set_ae_awb, 1);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for set_effect\n",
						__func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

/**
 *  lock: 1 to lock, 0 to unlock
 */
static int ce147_set_awb_lock(struct v4l2_subdev *sd, int lock)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_regbuf_awb_lock[1] = { 0x11 };
	unsigned int ce147_reglen_awb_lock = 1;

	if (lock)
		ce147_regbuf_awb_lock[0] = 0x11;
	else
		ce147_regbuf_awb_lock[0] = 0x00;

	err = ce147_i2c_write_multi(client, CMD_AE_WB_LOCK,
			ce147_regbuf_awb_lock, ce147_reglen_awb_lock);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for awb_lock\n",
				__func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_capture_cmd(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_regbuf_buffering_capture[1] = { 0x00 };
	unsigned int ce147_reglen_buffering_capture = 1;

	err = ce147_i2c_write_multi(client, CMD_BUFFERING_CAPTURE,
			ce147_regbuf_buffering_capture,
			ce147_reglen_buffering_capture);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write "
				"for buffering_capture\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}
static int ce147_set_exif_ctrl(struct v4l2_subdev *sd , int onoff)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_regbuf_exif_ctrl[2] = { 0x10, 0x00 };
	unsigned int ce147_reglen_exif_ctrl = 2;

	if ((onoff == 0) && (state->thumb_null == 0))
		ce147_regbuf_exif_ctrl[1] = 0x00;
	else if ((onoff	== 1) && (state->thumb_null == 0))
		ce147_regbuf_exif_ctrl[1] = 0x01;
	else if ((onoff	== 0) && (state->thumb_null == 0))
		ce147_regbuf_exif_ctrl[1] = 0x02;
	else if ((onoff	== 0) && (state->thumb_null == 1))
		ce147_regbuf_exif_ctrl[1] = 0x03;
	else if ((onoff	== 1) && (state->thumb_null == 1))
		ce147_regbuf_exif_ctrl[1] = 0x04;

	err = ce147_i2c_write_multi(client, CMD_SET_EXIF_CTRL,
			ce147_regbuf_exif_ctrl, ce147_reglen_exif_ctrl);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write "
				"for ce147_reglen_exif_ctrl\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_capture_exif(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	struct rtc_time gps_timestamp;

	unsigned char ce147_regbuf_exif[7] = {
		0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00
	};
	unsigned int ce147_reglen_exif = 7;

	unsigned char ce147_regbuf_timestamp[7] = {
		0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00
	};
	unsigned int ce147_reglen_timestamp = 7;

	unsigned char ce147_regbuf_rot[1] = { 0x01 };
	unsigned int ce147_reglen_rot =	1;

	unsigned char ce147_model_name[130] = {	0x00, };
	unsigned int ce147_reglen_model	= 130;

	unsigned char ce147_gps_processing[130] = { 0x00, };
	unsigned int ce147_reglen_gps_processing = 130;
#if !defined(CONFIG_ARIES_NTT)
	unsigned char ce147_str_model[9] = "GT-I9000\0";
#else /* Modify	NTTS1 */
	unsigned char ce147_str_model[7] = "SC-02B\0";
#endif
#if 0
	struct timeval curr_time;
	struct rtc_time time;
#endif
	ce147_model_name[0] = 0x06;
	ce147_model_name[1] = 0x09;

	memcpy(ce147_model_name + 2, ce147_str_model, sizeof(ce147_str_model));

	ce147_gps_processing[0] = 0x10;
	ce147_gps_processing[1] = 0x32;

	memcpy(ce147_gps_processing + 2, state->gpsInfo.gps_processingmethod,
			sizeof(state->gpsInfo.gps_processingmethod));

#if 0
	do_gettimeofday(&curr_time);
	rtc_time_to_tm(curr_time.tv_sec, &time);

	time.tm_year += 1900;
	time.tm_mon += 1;

	ce147_regbuf_exif[0] = (time.tm_year & 0x00FF);
	ce147_regbuf_exif[1] = (time.tm_year & 0xFF00) >> 8;
	ce147_regbuf_exif[2] = time.tm_mon;
	ce147_regbuf_exif[3] = time.tm_mday;
	ce147_regbuf_exif[4] = time.tm_hour;
	ce147_regbuf_exif[5] = time.tm_min;
	ce147_regbuf_exif[6] = time.tm_sec;
#else
	state->exifTimeInfo->tm_year += 1900;
	state->exifTimeInfo->tm_mon += 1;
	ce147_regbuf_exif[0] = (state->exifTimeInfo->tm_year & 0x00FF);
	ce147_regbuf_exif[1] = (state->exifTimeInfo->tm_year & 0xFF00) >> 8;
	ce147_regbuf_exif[2] = state->exifTimeInfo->tm_mon;
	ce147_regbuf_exif[3] = state->exifTimeInfo->tm_mday;
	ce147_regbuf_exif[4] = state->exifTimeInfo->tm_hour;
	ce147_regbuf_exif[5] = state->exifTimeInfo->tm_min;
	ce147_regbuf_exif[6] = state->exifTimeInfo->tm_sec;
#endif

	rtc_time_to_tm(state->gpsInfo.gps_timeStamp, &gps_timestamp);
	gps_timestamp.tm_year += 1900;
	gps_timestamp.tm_mon += 1;

	pr_debug("====!! Exif Time YEAR: %d, MONTH: %d, "
			"DAY: %d, HOUR:	%d, MIN: %d, SEC: %d\n",
				gps_timestamp.tm_year, gps_timestamp.tm_mon,
				gps_timestamp.tm_mday, gps_timestamp.tm_hour,
				gps_timestamp.tm_min, gps_timestamp.tm_sec);

	ce147_regbuf_timestamp[0] = (gps_timestamp.tm_year & 0x00FF);
	ce147_regbuf_timestamp[1] = (gps_timestamp.tm_year & 0xFF00) >> 8;
	ce147_regbuf_timestamp[2] = gps_timestamp.tm_mon;
	ce147_regbuf_timestamp[3] = gps_timestamp.tm_mday;
	ce147_regbuf_timestamp[4] = gps_timestamp.tm_hour;
	ce147_regbuf_timestamp[5] = gps_timestamp.tm_min;
	ce147_regbuf_timestamp[6] = gps_timestamp.tm_sec;


	pr_debug("Exif Time YEAR: %ld, MONTH: %d, DAY: %d, "
			"HOUR: %d, MIN:	%d, SEC: %d\n",
			state->exifTimeInfo->tm_year,
			state->exifTimeInfo->tm_mon,
			state->exifTimeInfo->tm_mday,
			state->exifTimeInfo->tm_hour,
			state->exifTimeInfo->tm_min,
			state->exifTimeInfo->tm_sec);

	ce147_regbuf_rot[0] = state->exif_orientation_info;

	err = ce147_i2c_write_multi(client, CMD_INFO_MODEL, ce147_model_name,
					ce147_reglen_model);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for exif model "
				"name\n", __func__);
		return -EIO;
	}

	err = ce147_i2c_write_multi(client, CMD_INFO_EXIF, ce147_regbuf_exif,
					ce147_reglen_exif);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for exif\n",
				__func__);
		return -EIO;
	}

	err = ce147_i2c_write_multi(client, CMD_INFO_ROT, ce147_regbuf_rot,
					ce147_reglen_rot);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for exif\n",
				__func__);
		return -EIO;
	}

	err = ce147_i2c_write_multi(client, CMD_INFO_LONGITUDE_LATITUDE,
			state->gpsInfo.ce147_gps_buf,
			sizeof(state->gpsInfo.ce147_gps_buf));
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for gps "
				"longitude latitude\n", __func__);
		return -EIO;
	}

	err = ce147_i2c_write_multi(client, CMD_INFO_ALTITUDE,
			state->gpsInfo.ce147_altitude_buf,
			sizeof(state->gpsInfo.ce147_altitude_buf));
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for gps "
				"altitude\n", __func__);
		return -EIO;
	}

	err = ce147_i2c_write_multi(client, CMD_GPS_TIMESTAMP,
			ce147_regbuf_timestamp, ce147_reglen_timestamp);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for gps "
				"timestamp\n", __func__);
		return -EIO;
	}

	err = ce147_i2c_write_multi(client, CMD_INFO_MODEL,
			ce147_gps_processing, ce147_reglen_gps_processing);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for gps method\n",
				__func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_jpeg_quality(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

#if 0
	unsigned char ce147_regbuf_jpeg_comp_level[7] = {
		0x00, 0xF4, 0x01, 0x90,	0x01, 0x05, 0x01
	};
	unsigned int ce147_reglen_jpeg_comp_level = 7;

	unsigned int comp_ratio = 1500;

	int err;

	if (state->jpeg.quality < 0)
		state->jpeg.quality = 0;
	if (state->jpeg.quality > 100)
		state->jpeg.quality = 100;

	comp_ratio -= (100 - state->jpeg.quality) * 10;
	ce147_regbuf_jpeg_comp_level[1]	= comp_ratio & 0xFF;
	ce147_regbuf_jpeg_comp_level[2]	= (comp_ratio & 0xFF00) >> 8;

	ce147_msg(&client->dev, "Quality = %d, Max value = %d\n",
			state->jpeg.quality, comp_ratio);

	/* 10% range for final JPEG image size */
	comp_ratio = (comp_ratio * 9) / 10;
	ce147_regbuf_jpeg_comp_level[3]	= comp_ratio & 0xFF;
	ce147_regbuf_jpeg_comp_level[4]	= (comp_ratio & 0xFF00) >> 8;

	ce147_msg(&client->dev, "Quality = %d, Min Comp Ratio = %d\n",
			state->jpeg.quality, comp_ratio);
#endif

	unsigned char ce147_regbuf_jpeg_comp_level[7] = {
		0x00, 0xA4, 0x06, 0x78,	0x05, 0x05, 0x01
	};
	unsigned int ce147_reglen_jpeg_comp_level = 7;
	unsigned int quality = state->jpeg.quality;
	unsigned int compressionRatio =	0;
	unsigned int minimumCompressionRatio = 0;
	int err;

	if (quality >= 91 && quality <= 100) { /* 91 ~ 100 */
		compressionRatio = 17; /* 17% */
	} else if (quality >= 81 && quality <= 90) { /* 81 ~ 90 */
		compressionRatio = 16; /* 16% */
	} else if (quality >= 71 && quality <= 80) { /* 71 ~ 80 */
		compressionRatio = 15; /* 15% */
	} else if (quality >= 61 && quality <= 70) { /* 61 ~ 70 */
		compressionRatio = 14; /* 14% */
	} else if (quality >= 51 && quality <= 60) { /* 51 ~ 60 */
		compressionRatio = 13; /* 13% */
	} else if (quality >= 41 && quality <= 50) { /* 41 ~ 50 */
		compressionRatio = 12; /* 12% */
	} else if (quality >= 31 && quality <= 40) { /* 31 ~ 40 */
		compressionRatio = 11; /* 11% */
	} else if (quality >= 21 && quality <= 30) { /* 21 ~ 30 */
		compressionRatio = 10; /* 10% */
	} else if (quality >= 11 && quality <= 20) { /* 11 ~ 20 */
		compressionRatio = 9; /* 9% */
	} else if (quality >= 1 && quality <= 10) { /* 1 ~ 10 */
		compressionRatio = 8; /* 8% */
	} else {
		dev_err(&client->dev, "%s: Invalid Quality(%d)\n",
				__func__, quality);
		return -1;
	}

	/* ex) if compression ratio is 17%, minimum compression ratio is 14%*/
	minimumCompressionRatio = compressionRatio - 3;
	ce147_regbuf_jpeg_comp_level[1]	= (compressionRatio * 100) & 0xFF;
	ce147_regbuf_jpeg_comp_level[2]	= ((compressionRatio * 100) & 0xFF00)
						>> 8;
	ce147_regbuf_jpeg_comp_level[3]	= (minimumCompressionRatio * 100)
						& 0xFF;
	ce147_regbuf_jpeg_comp_level[4]	= ((minimumCompressionRatio * 100)
						& 0xFF00) >> 8;

	err = ce147_i2c_write_multi(client, CMD_JPEG_CONFIG,
			ce147_regbuf_jpeg_comp_level,
			ce147_reglen_jpeg_comp_level);

	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"jpeg_comp_level\n", __func__);
		return -EIO;
	}

	return 0;
}

static int ce147_set_jpeg_config(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int preview_size = state->preview_size;

	unsigned char ce147_regbuf_set_lump[2] = { 0x00, 0x04};
	unsigned int ce147_reglen_set_lump = 2;

	/* unsigned char ce147_regbuf_set_lump2[1] = {0x00};
	unsigned int ce147_reglen_set_lump2 = 1; */

	err = ce147_set_jpeg_quality(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_set_jpeg_quality\n",
				__func__);
		return -EIO;
	}

	if (preview_size != CE147_PREVIEW_VGA) {
		/* pr_debug("[5B] ce147_set_jpeg_config: preview_size is WVGA "
		   "(%d)\n", preview_size); */
		ce147_regbuf_set_lump[1] = 0x13;
	}

	/* if (!state->thumb_null) */
	err = ce147_i2c_write_multi(client, CMD_JPEG_BUFFERING,
			ce147_regbuf_set_lump, ce147_reglen_set_lump);
	/* else if (state->thumb_null) */
		/* err = ce147_i2c_write_multi(client, CMD_JPEG_BUFFERING2,
			ce147_regbuf_set_lump2, ce147_reglen_set_lump2); */

	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for set_lump\n",
				__func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_get_snapshot_data(struct v4l2_subdev *sd)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char cmd_buf_framesize[1] = { 0x00 };
	unsigned int cmd_len_framesize = 1;

	unsigned char cmd_buf_setdata[2] = { 0x02, 0x00 };
	unsigned int cmd_len_setdata = 2;

	unsigned char jpeg_status[3] = { 0x00, 0x00, 0x00 };
	unsigned char jpeg_status_len =	3;

	unsigned char jpeg_framesize[4]	= { 0x00, 0x00, 0x00, 0x00 };
	unsigned int jpeg_framesize_len	= 4;

	if (state->jpeg.enable) {
		/* Get main JPEG size */
		cmd_buf_framesize[0] = 0x00;
		err = ce147_i2c_read_multi(client, CMD_JPEG_SIZE,
				cmd_buf_framesize, cmd_len_framesize,
				jpeg_framesize,	jpeg_framesize_len);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_read for "
					"jpeg_framesize\n", __func__);
			return -EIO;
		}
		state->jpeg.main_size =	jpeg_framesize[1]
						| (jpeg_framesize[2] << 8)
						| (jpeg_framesize[3] << 16);

		ce147_info(&client->dev, "%s: JPEG main	filesize = %d bytes\n",
				__func__, state->jpeg.main_size);

		/* Get Thumbnail size */
		if (!state->thumb_null)	{
			cmd_buf_framesize[0] = 0x01;
			err = ce147_i2c_read_multi(client, CMD_JPEG_SIZE,
				cmd_buf_framesize, cmd_len_framesize,
				jpeg_framesize, jpeg_framesize_len);
			if (err	< 0) {
				dev_err(&client->dev, "%s: failed: i2c_read "
						"for jpeg_framesize\n",
							__func__);
				return -EIO;
			}
			state->jpeg.thumb_size = jpeg_framesize[1]
						| (jpeg_framesize[2] << 8)
						| (jpeg_framesize[3] << 16);
		} else
			state->jpeg.thumb_size = 0;

		ce147_msg(&client->dev, "%s: JPEG thumb filesize = %d bytes\n",
				__func__, state->jpeg.thumb_size);

		state->jpeg.main_offset	= 0;
		state->jpeg.thumb_offset = 0x271000;
		state->jpeg.postview_offset = 0x280A00;
	}

	if (state->jpeg.enable)
		cmd_buf_setdata[0] = 0x02;
	else
		cmd_buf_setdata[0] = 0x01;
	/* Set Data out */
	err = ce147_i2c_read_multi(client, CMD_SET_DATA,
			cmd_buf_setdata, cmd_len_setdata,
			jpeg_status, jpeg_status_len);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_read for set_data\n",
				__func__);
		return -EIO;
	}
	ce147_msg(&client->dev, "%s:JPEG framesize (after set_data_out)	= "
			"0x%02x.%02x.%02x\n", __func__,
				jpeg_status[2],	jpeg_status[1], jpeg_status[0]);

	/* 0x66 */
	err = ce147_i2c_read_multi(client, CMD_DATA_OUT_REQ, NULL, 0,
					jpeg_status, jpeg_status_len);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_read for "
				"set_data_request\n", __func__);
		return -EIO;
	}
	ce147_msg(&client->dev, "%s:JPEG framesize (after set_data_request) "
			"= 0x%02x.%02x.%02x\n", __func__,
				jpeg_status[2], jpeg_status[1], jpeg_status[0]);

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_capture_config(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	/*
	 *  1. Set image size
	 */
	err = ce147_set_capture_size(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"capture_resolution\n", __func__);
		return -EIO;
	}

	/*
	 * Set DZoom
	 */
	if (DZoom_State	!= 0) {
		ctrl->value = DZoom_State;
		err = ce147_set_dzoom(sd, ctrl);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: Could not set Zoom "
					"in Capture_start\n", __func__);
			return -EIO;
		}
	}

	/*
	 * Set AWB Lock
	 */
	err = ce147_set_awb_lock(sd, 1);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_set_awb_lock, "
				"err %d\n", __func__, err);
		return -EIO;
	}
	/*
	 * 2. Set Capture Command
	 */
	err = ce147_set_capture_cmd(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: set_capture_cmd failed\n",
				__func__);
		return err;
	}

	return 0;
}

static int ce147_set_capture_start(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	/*
	 * Right after ce147_set_capture_config,
	 * 3. Wait for capture to complete for ce147_set_capture_cmd()
	 * in ce147_set_capture_config()
	 */
	err = ce147_waitfordone_timeout(client, 0x6C, 0x00, 3000, POLL_TIME_MS);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: Wait for "
				"buffering_capture\n", __func__);
		return err;
	}
	ce147_msg(&client->dev, "%s: buffering_capture - wait time %d ms\n",
			__func__, err);


	err = ce147_set_exif_ctrl(sd, state->exif_ctrl);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: set_capture_cmd failed\n",
				__func__);
		return err;
	}


	if (state->jpeg.enable) {
		/*
		 * 4. Set EXIF information
		 */
		err = ce147_set_capture_exif(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"exif\n", __func__);
			return -EIO;
		}

		/*
		 * 6. Set JPEG Encoding parameters
		 */
		err = ce147_set_jpeg_config(sd);
		if (err < 0) {
			dev_err(&client->dev, "%s: Setting JPEG encoding "
					"parameters\n", __func__);
			return err;
		}
		/*
		 * 7. Wait for encoding to complete
		 */
		err = ce147_waitfordone_timeout(client, 0x6C, 0x00, 3000,
							POLL_TIME_MS);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: Wait for "
					"jpeg_encoding\n", __func__);
			return err;
		}
		ce147_msg(&client->dev, "%s: jpeg_encoding - wait time %d ms\n",
				__func__, err);
	}
	/*
	 * 8. Get JPEG Main Data
	 */
	err = ce147_get_snapshot_data(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: get_snapshot_data\n",
				__func__);
		return err;
	}
	/*
	 * 9. Wait for done
	 */
	err = ce147_waitfordone_timeout(client, 0x61, 0x00, 3000, POLL_TIME_MS);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: Wait for data_transfer\n",
				__func__);
		return err;
	}
	ce147_msg(&client->dev, "%s: data_transfer - wait time %d ms\n",
			__func__, err);

	return 0;
}

static int ce147_get_focus_mode(struct i2c_client *client, unsigned char cmd,
					unsigned char *value)
{
	int err;
	int count;

	unsigned char ce147_buf_get_af_status[1] = { 0x00 };

	/* set focus mode: AF or MACRO */
	err = ce147_i2c_write_multi(client, cmd, value, 1);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"get_focus_mode\n", __func__);
		return -EIO;
	}
	/* check whether af data is valid or not */
	for (count = 0; count < 600; count++) {
		msleep(10);
		ce147_buf_get_af_status[0] = 0xFF;
		err = ce147_i2c_read_multi(client, CMD_CHECK_AUTO_FOCUS_SEARCH,
				NULL, 0, ce147_buf_get_af_status, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_read for	"
					"get_focus_mode\n", __func__);
			return -EIO;
		}
		if ((ce147_buf_get_af_status[0] & 0x01) == 0x00)
			break;
	}

	if ((ce147_buf_get_af_status[0]	& 0x01)	!= 0x00)
		return -EBUSY;
	else
		return ce147_buf_get_af_status[0] & 0x01;
}

static int ce147_set_af_softlanding(struct v4l2_subdev *sd)
{
	int err;
	int count;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_buf_get_af_status[1] = { 0x00 };
	unsigned char ce147_buf_set_af_land[1] = { 0x08 };
	unsigned int ce147_len_set_af_land = 1;

	if (state->runmode > CE147_RUNMODE_IDLE) {
		/* make	lens landing mode */
		err = ce147_i2c_write_multi(client, CMD_SET_AUTO_FOCUS_MODE,
				ce147_buf_set_af_land, ce147_len_set_af_land);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"auto_focus\n", __func__);
			return -EIO;
		}
		/* check whether af data is valid or not */
		for (count = 0; count <	600; count++) {
			msleep(10);
			ce147_buf_get_af_status[0] = 0xFF;
			err = ce147_i2c_read_multi(client,
					CMD_CHECK_AUTO_FOCUS_SEARCH, NULL, 0,
					ce147_buf_get_af_status, 1);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: i2c_read "
						"for get_focus_mode\n",
							__func__);
				return -EIO;
			}
			if ((ce147_buf_get_af_status[0]) == 0x08)
				break;
		}
	}
	return 0;
}

static int ce147_set_flash(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_flash[2] = { 0x03, 0x00 };
	unsigned int ce147_len_set_flash = 2;
	unsigned char ce147_buf_set_flash_manual[2] = { 0x00, 0x00 };
	unsigned int ce147_len_set_flash_manual = 2;

	switch (ctrl->value) {
	case FLASH_MODE_OFF:
		ce147_buf_set_flash[1] = 0x00;
		break;

	case FLASH_MODE_AUTO:
		ce147_buf_set_flash[1] = 0x02;
		break;

	case FLASH_MODE_ON:
		ce147_buf_set_flash[1] = 0x01;
		break;

	case FLASH_MODE_TORCH:
		ce147_buf_set_flash_manual[1] = 0x01;
		break;

	default:
		ce147_buf_set_flash[1] = 0x00;
		break;
	}

	/* need	to modify flash off for	torch mode */
	if (ctrl->value == FLASH_MODE_OFF) {
		err = ce147_i2c_write_multi(client, CMD_SET_FLASH_MANUAL,
				ce147_buf_set_flash_manual,
				ce147_len_set_flash_manual);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"set_flash\n", __func__);
			return -EIO;
		}
	}

	err = ce147_i2c_write_multi(client, CMD_SET_FLASH,
			ce147_buf_set_flash, ce147_len_set_flash);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for set_flash\n",
				__func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done, flash: 0x%02x\n",
			__func__, ce147_buf_set_flash[1]);

	return 0;
}

static int ce147_set_effect(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_effect[2] = { 0x05, 0x00 };
	unsigned int ce147_len_set_effect = 2;

	switch (ctrl->value) {
	case IMAGE_EFFECT_NONE:
		ce147_buf_set_effect[1] = 0x00;
		break;

	case IMAGE_EFFECT_BNW:
		ce147_buf_set_effect[1] = 0x01;
		break;

	case IMAGE_EFFECT_SEPIA:
		ce147_buf_set_effect[1] = 0x03;
		break;

	case IMAGE_EFFECT_AQUA:
		ce147_buf_set_effect[1] = 0x0D;
		break;

	case IMAGE_EFFECT_ANTIQUE:
		ce147_buf_set_effect[1] = 0x06;
		break;

	case IMAGE_EFFECT_NEGATIVE:
		ce147_buf_set_effect[1] = 0x05;
		break;

	case IMAGE_EFFECT_SHARPEN:
		ce147_buf_set_effect[1] = 0x04;
		break;

	default:
		ce147_buf_set_effect[1] = 0x00;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT,
			ce147_buf_set_effect, ce147_len_set_effect);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for set_effect\n",
				__func__);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for set_effect\n", __func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_saturation(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_saturation[2] = { 0x06, 0x00 };
	unsigned int ce147_len_set_saturation = 2;

	switch (ctrl->value) {
	case SATURATION_MINUS_2:
		ce147_buf_set_saturation[1] = 0x01;
		break;

	case SATURATION_MINUS_1:
		ce147_buf_set_saturation[1] = 0x02;
		break;

	case SATURATION_DEFAULT:
		ce147_buf_set_saturation[1] = 0x03;
		break;

	case SATURATION_PLUS_1:
		ce147_buf_set_saturation[1] = 0x04;
		break;

	case SATURATION_PLUS_2:
		ce147_buf_set_saturation[1] = 0x05;
		break;

	default:
		ce147_buf_set_saturation[1] = 0x03;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT,
			ce147_buf_set_saturation, ce147_len_set_saturation);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"set_saturation\n", __func__);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for set_saturation\n",
				__func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done, saturation: 0x%02x\n",
			__func__, ce147_buf_set_saturation[1]);

	return 0;
}

static int ce147_set_contrast(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_contrast[2] = { 0x07, 0x00 };
	unsigned int ce147_len_set_contrast = 2;

	switch (ctrl->value) {
	case CONTRAST_MINUS_2:
		ce147_buf_set_contrast[1] = 0x01;
		break;

	case CONTRAST_MINUS_1:
		ce147_buf_set_contrast[1] = 0x02;
		break;

	case CONTRAST_DEFAULT:
		ce147_buf_set_contrast[1] = 0x03;
		break;

	case CONTRAST_PLUS_1:
		ce147_buf_set_contrast[1] = 0x04;
		break;

	case CONTRAST_PLUS_2:
		ce147_buf_set_contrast[1] = 0x05;
		break;

	default:
		ce147_buf_set_contrast[1] = 0x03;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT,
			ce147_buf_set_contrast, ce147_len_set_contrast);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"set_contrast\n", __func__);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for set_contrast\n",
				__func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done, contrast: 0x%02x\n",
			__func__, ce147_buf_set_contrast[1]);

	return 0;
}

static int ce147_set_sharpness(struct v4l2_subdev *sd,
				struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_saturation[2] = { 0x02, 0x00 };
	unsigned int ce147_len_set_saturation = 2;

	switch (ctrl->value) {
	case SHARPNESS_MINUS_2:
		ce147_buf_set_saturation[1] = 0x01;
		break;

	case SHARPNESS_MINUS_1:
		ce147_buf_set_saturation[1] = 0x02;
		break;

	case SHARPNESS_DEFAULT:
		ce147_buf_set_saturation[1] = 0x03;
		break;

	case SHARPNESS_PLUS_1:
		ce147_buf_set_saturation[1] = 0x04;
		break;

	case SHARPNESS_PLUS_2:
		ce147_buf_set_saturation[1] = 0x05;
		break;

	default:
		ce147_buf_set_saturation[1] = 0x03;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT,
			ce147_buf_set_saturation, ce147_len_set_saturation);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"set_saturation\n", __func__);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for set_saturation\n",
				__func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done, sharpness: 0x%02x\n",
			__func__, ce147_buf_set_saturation[1]);

	return 0;
}

static int ce147_set_wdr(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_wdr[1] = { 0x00 };
	unsigned int ce147_len_set_wdr = 1;

	switch (ctrl->value) {
	case WDR_ON:
		ce147_buf_set_wdr[0] = 0x01;
		break;

	case WDR_OFF:
	default:
		ce147_buf_set_wdr[0] = 0x00;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_WDR,
					ce147_buf_set_wdr, ce147_len_set_wdr);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for set_wdr\n",
				__func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done, wdr: 0x%02x\n",
			__func__, ce147_buf_set_wdr[0]);

	return 0;
}

static int ce147_set_anti_shake(struct v4l2_subdev *sd,
				struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_anti_shake[1] = { 0x00 };
	unsigned int ce147_len_set_anti_shake = 1;

	switch (ctrl->value) {
	case ANTI_SHAKE_STILL_ON:
		ce147_buf_set_anti_shake[0] = 0x01;
		break;

	case ANTI_SHAKE_MOVIE_ON:
		ce147_buf_set_anti_shake[0] = 0x10;
		break;

	case ANTI_SHAKE_OFF:
	default:
		ce147_buf_set_anti_shake[0] = 0x00;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_ANTI_SHAKE,
			ce147_buf_set_anti_shake, ce147_len_set_anti_shake);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for anti_shake\n",
				__func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done, AHS: 0x%02x\n",
			__func__, ce147_buf_set_anti_shake[0]);

	return 0;
}

static int ce147_set_continous_af(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_caf[1] = { 0x02 };
	unsigned char ce147_buf_start_af_search[1] = { 0x00 };
	unsigned int ce147_len_start_af_search = 1;
#if 0
	unsigned char ce147_buf_set_af[1] = { 0x00 };
#endif
	unsigned char ce147_buf_stop_lens_movement[1] = { 0x00 };

	/* need	to set face_detection with noline */

	if (ctrl->value) {
		err = ce147_get_focus_mode(client, CMD_SET_AUTO_FOCUS_MODE,
						ce147_buf_set_caf);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: "
					"start_continous_af\n", __func__);
			return -EIO;
		}

		/* start af search */
		err = ce147_i2c_write_multi(client, CMD_START_AUTO_FOCUS_SEARCH,
				ce147_buf_start_af_search,
				ce147_len_start_af_search);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"start_continous_af\n", __func__);
			return -EIO;
		}
	} else {
		err = ce147_get_focus_mode(client, CMD_STOP_LENS_MOVEMENT,
				ce147_buf_stop_lens_movement);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: stop_continous_af\n",
					__func__);
			return -EIO;
		}
#if 0
		err = ce147_get_focus_mode(client, CMD_SET_AUTO_FOCUS_MODE,
				ce147_buf_set_af);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: stop_continous_af\n",
					__func__);
			return -EIO;
		}
#endif
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_object_tracking(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	int count;
	unsigned short x;
	unsigned short y;

	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_object_tracking[7] = { 0x00, };
	unsigned int ce147_len_set_object_tracking = 7;
	unsigned char ce147_buf_check_object_tracking[9] = { 0x00, };
	unsigned int ce147_len_check_object_tracking = 9;
	unsigned char ce147_buf_stop_lens[1] = { 0x00 };

	/* get x,y touch position */
	x = state->position.x;
	y = state->position.y;

	if (OT_START) {
		ce147_buf_set_object_tracking[3] = (x & 0x00FF);
		ce147_buf_set_object_tracking[4] = ((x & 0xFF00) >> 8);
		ce147_buf_set_object_tracking[5] = (y & 0x00FF);
		ce147_buf_set_object_tracking[6] = ((y & 0xFF00) >> 8);

		err = ce147_i2c_write_multi(client, CMD_START_OT,
						ce147_buf_set_object_tracking,
						ce147_len_set_object_tracking);
		if (err	< 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"object_tracking\n", __func__);
			return -EIO;
		}

		/* Read	status whether AF Tracking is successful or fail */
		for (count = 0; count < 300; count++) {
			msleep(10);
			ce147_buf_check_object_tracking[0] = 0xFF;
			err = ce147_i2c_read_multi(client, CMD_CHECK_OT,
					NULL, 0,
					ce147_buf_check_object_tracking,
					ce147_len_check_object_tracking);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: i2c_read "
						"for object_tracking\n",
							__func__);
				return -EIO;
			}
			if (ce147_buf_check_object_tracking[0] == 0x02
				|| ce147_buf_check_object_tracking[0] == 0x03)
				break;
		}

		/* OT status: searching	an object in progess */
		if (ce147_buf_check_object_tracking[0] == 0x01) {
			state->ot_status = 1;
		} else if (ce147_buf_check_object_tracking[0] == 0x02) {
			/* OT status: an object is detected successfully */
			err = ce147_set_continous_af(sd, ctrl);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: "
						"ce147_start_continous_af for "
						"object_tracking\n", __func__);
				return -EIO;
			}
			state->ot_status = 2;
		} else if (ce147_buf_check_object_tracking[0] == 0x03) {
			/* OT status: an object detecting is failed */
			state->ot_status = 3;
		}
	} else {
		err = ce147_get_focus_mode(client, CMD_STOP_LENS_MOVEMENT,
				ce147_buf_stop_lens);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: "
					"ce147_start_continous_af for "
					"object_tracking\n", __func__);
			return -EIO;
		}

		err = ce147_i2c_write_multi(client, CMD_START_OT,
				ce147_buf_set_object_tracking,
				ce147_len_set_object_tracking);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"object_tracking\n", __func__);
			return -EIO;
		}
	}
	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_get_object_tracking(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;

	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_check_object_tracking[9] = { 0x00, };
	unsigned int ce147_len_check_object_tracking = 9;

	ce147_buf_check_object_tracking[0] = 0xFF;
	err = ce147_i2c_read_multi(client, CMD_CHECK_OT, NULL, 0,
			ce147_buf_check_object_tracking,
			ce147_len_check_object_tracking);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_read for "
				"object_tracking\n", __func__);
		return -EIO;
	}

	/* OT status: searching an object in progess */
	if (ce147_buf_check_object_tracking[0] == 0x01)	{
		state->ot_status = 1;
	} else if (ce147_buf_check_object_tracking[0] == 0x02) {
		/* OT status: an object	is detected successfully */
		state->ot_status = 2;
	} else if (ce147_buf_check_object_tracking[0] == 0x03) {
		/* OT status: an object	detecting is failed */
		state->ot_status = 3;
	} else if (ce147_buf_check_object_tracking[0] == 0x04) {
		/* OT status: detected object is missing  */
		state->ot_status = 4;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_face_detection(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_fd[3] = { 0x00, 0x00, 0x00 };
	unsigned int ce147_len_set_fd =	3;

	switch (ctrl->value) {
	case FACE_DETECTION_ON:
		ce147_buf_set_fd[0] = 0x03;
		ce147_buf_set_fd[1] = 0x01;
		ce147_buf_set_fd[2] = 0x0A;
		break;

	case FACE_DETECTION_ON_BEAUTY:
		ce147_buf_set_fd[0] = 0x01;
		ce147_buf_set_fd[1] = 0x01;
		ce147_buf_set_fd[2] = 0x0A;
		break;

	case FACE_DETECTION_NOLINE:
		ce147_buf_set_fd[0] = 0x03;
		ce147_buf_set_fd[1] = 0x00;
		ce147_buf_set_fd[2] = 0x0A;
		break;

	case FACE_DETECTION_OFF:
	default:
		ce147_buf_set_fd[0] = 0x00;
		ce147_buf_set_fd[1] = 0x00;
		ce147_buf_set_fd[2] = 0x00;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_FACE_DETECTION,
					ce147_buf_set_fd, ce147_len_set_fd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"face_detection\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_smart_auto(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_smart_auto[1] = { 0x00 };
	unsigned int ce147_len_set_smart_auto = 1;

	if (ctrl->value == SMART_AUTO_ON) {
		ce147_buf_set_smart_auto[0] = 0x01;
		err = ce147_i2c_write_multi(client, CMD_SET_SMART_AUTO,
				ce147_buf_set_smart_auto,
				ce147_len_set_smart_auto);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"smart_auto\n", __func__);
			return -EIO;
		}
#if 0
		err = ce147_set_continous_af(sd, CAF_START);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: CAF_START for "
					"smart_auto\n", __func__);
			return -EIO;
		}
#endif
	} else {
#if 0
		err = ce147_set_continous_af(sd, CAF_STOP);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: CAF_START for "
					"smart_auto\n", __func__);
			return -EIO;
		}
#endif

		ce147_buf_set_smart_auto[0] = 0x00;
		err = ce147_i2c_write_multi(client, CMD_SET_SMART_AUTO,
				ce147_buf_set_smart_auto,
				ce147_len_set_smart_auto);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"smart_auto\n", __func__);
			return -EIO;
		}
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_get_smart_auto_status(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;

	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_smart_auto_status[2] = {0x00, 0x00};

	ce147_buf_smart_auto_status[0] = 0xFF;
	err = ce147_i2c_read_multi(client, CMD_GET_SMART_AUTO_STATUS, NULL, 0,
					ce147_buf_smart_auto_status, 2);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_read for auto_status\n",
				__func__);
		return -EIO;
	}

	if (ce147_buf_smart_auto_status[0] == 0x00
		|| ce147_buf_smart_auto_status[0] == 0x01) {
		state->sa_status = SMART_AUTO_STATUS_AUTO;
	} else {
		switch (ce147_buf_smart_auto_status[1]) {
		case 0x00:
			state->sa_status = SMART_AUTO_STATUS_LANDSCAPE;
			break;

		case 0x01:
			state->sa_status = SMART_AUTO_STATUS_PORTRAIT;
			break;

		case 0x02:
			state->sa_status = SMART_AUTO_STATUS_NIGHT;
			break;

		case 0x03:
			state->sa_status = SMART_AUTO_STATUS_PORTRAIT_NIGHT;
			break;

		case 0x04:
			state->sa_status = SMART_AUTO_STATUS_MACRO;
			break;

		case 0x05:
			state->sa_status = SMART_AUTO_STATUS_PORTRAIT_BACKLIT;
			break;

		case 0x06:
			state->sa_status = SMART_AUTO_STATUS_PORTRAIT_ANTISHAKE;
			break;

		case 0x07:
			state->sa_status = SMART_AUTO_STATUS_ANTISHAKE;
			break;
		}
	}

	ce147_msg(&client->dev, "%s: done(smartauto_status:%d)\n",
			__func__, state->sa_status);

	return 0;
}

static int ce147_set_touch_auto_focus(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	unsigned short x;
	unsigned short y;

	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_touch_af[11] = { 0x00, };
	unsigned int ce147_len_set_touch_af = 11;

#if defined(CONFIG_ARIES_NTT) /* Modify	NTTS1 */
	err = ce147_set_awb_lock(sd, 0);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_set_awb_lock, "
				"err %d\n", __func__, err);
		return -EIO;
	}
#endif
	/* get x,y touch position */
	x = state->position.x;
	y = state->position.y;

	if (ctrl->value	== TOUCH_AF_START) {
		ce147_buf_set_touch_af[0] = 0x01;
		ce147_buf_set_touch_af[1] = 0x03;
		ce147_buf_set_touch_af[2] = 0x00;
		ce147_buf_set_touch_af[3] = ((x - 0x32) & 0x00FF);
		ce147_buf_set_touch_af[4] = (((x - 0x32) & 0xFF00) >> 8);
		ce147_buf_set_touch_af[5] = ((y - 0x32) & 0x00FF);
		ce147_buf_set_touch_af[6] = (((y - 0x32) & 0xFF00) >> 8);
		ce147_buf_set_touch_af[7] = ((x + 0x32) & 0x00FF);
		ce147_buf_set_touch_af[8] = (((x + 0x32) & 0xFF00) >> 8);
		ce147_buf_set_touch_af[9] = ((y + 0x32) & 0x00FF);
		ce147_buf_set_touch_af[10] = (((y + 0x32) & 0xFF00) >> 8);
	}

	err = ce147_i2c_write_multi(client, CMD_SET_TOUCH_AUTO_FOCUS,
			ce147_buf_set_touch_af, ce147_len_set_touch_af);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"touch_auto_focus\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_focus_mode(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_focus_mode[1] = { 0x00 };

	switch (ctrl->value) {
	case FOCUS_MODE_MACRO:
	case FOCUS_MODE_MACRO_DEFAULT:
		ce147_buf_set_focus_mode[0] = 0x01;
		break;

	/* case FOCUS_MODE_FD:
		break; */

	case FOCUS_MODE_AUTO:
	case FOCUS_MODE_AUTO_DEFAULT:
	/* case FOCUS_MODE_FD_DEFAULT: */
	default:
		ce147_buf_set_focus_mode[0] = 0x00;
		break;
	}
#if 0
	if (state->hd_preview_on == 1)
		ce147_buf_set_focus_mode[0] = 0x07;
#endif
	/* if (ctrl->value != FOCUS_MODE_FD) { */
	if ((state->pre_focus_mode != ce147_buf_set_focus_mode[0])
		|| (ctrl->value == FOCUS_MODE_MACRO_DEFAULT)
		|| (ctrl->value == FOCUS_MODE_AUTO_DEFAULT)) {
		/* || (ctrl->value == FOCUS_MODE_FD_DEFAULT)) */
#if defined(CONFIG_ARIES_NTT) /* Modify	NTTS1 */
		ce147_msg(&client->dev, "%s: unlock\n", __func__);
		err = ce147_set_awb_lock(sd, 0);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: ce147_set_awb_"
					"unlock, err %d\n", __func__, err);
			return -EIO;
		}
#endif
		/* pr_debug("[5B] ce147_set_focus_mode: %d\n",
				ce147_buf_set_focus_mode[0]); */
		err = ce147_get_focus_mode(client, CMD_SET_AUTO_FOCUS_MODE,
						ce147_buf_set_focus_mode);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: get_focus_mode\n",
					__func__);
			return -EIO;
		}
	}
	state->pre_focus_mode = ce147_buf_set_focus_mode[0];
	/* } */

	return 0;
}

static int ce147_set_vintage_mode(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_vintage_mode[4] = {
		0x10, 0x01, 0x32, 0x00
	};
	unsigned int ce147_len_set_vintage_mode	= 4;

	switch (ctrl->value) {
	case VINTAGE_MODE_OFF:
		ce147_buf_set_vintage_mode[1] = 0x00;
		ce147_buf_set_vintage_mode[2] = 0x00;
		ce147_buf_set_vintage_mode[3] = 0x00;
		break;

	case VINTAGE_MODE_NORMAL:
		ce147_buf_set_vintage_mode[3] = 0x00;
		break;

	case VINTAGE_MODE_WARM:
		ce147_buf_set_vintage_mode[3] = 0x02;
		break;

	case VINTAGE_MODE_COOL:
		ce147_buf_set_vintage_mode[3] = 0x01;
		break;

	case VINTAGE_MODE_BNW:
		ce147_buf_set_vintage_mode[3] = 0x03;
		break;

	default:
		ce147_buf_set_vintage_mode[1] = 0x00;
		ce147_buf_set_vintage_mode[2] = 0x00;
		ce147_buf_set_vintage_mode[3] = 0x00;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT_SHOT,
			ce147_buf_set_vintage_mode,
			ce147_len_set_vintage_mode);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"vintage_mode\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_face_beauty(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_face_beauty[4] = { 0x00, 0x00, 0x00, 0x00 };
	unsigned int ce147_len_set_face_beauty = 4;

	switch (ctrl->value) {
	case BEAUTY_SHOT_ON:
		ce147_buf_set_face_beauty[1] = 0x01;
		ce147_buf_set_face_beauty[2] = 0x32;
		ce147_buf_set_face_beauty[3] = 0x01;
		break;

	case BEAUTY_SHOT_OFF:
	default:
		break;
	}

	/* Need	to set face detection as 'face beauty on' mode. */
	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT_SHOT,
			ce147_buf_set_face_beauty, ce147_len_set_face_beauty);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"set_face_beauty\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}


static int ce147_set_white_balance(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_wb_auto[1] = { 0x01 };
	unsigned char ce147_buf_set_wb[2] = { 0x10, 0x00 };
	unsigned int ce147_len_set_wb_auto = 1;
	unsigned int ce147_len_set_wb =	2;

	switch (ctrl->value) {
	case WHITE_BALANCE_AUTO:
		ce147_buf_set_wb_auto[0] = 0x00;
		break;

	case WHITE_BALANCE_SUNNY:
		ce147_buf_set_wb[1] = 0x00;
		break;

	case WHITE_BALANCE_CLOUDY:
		ce147_buf_set_wb[1] = 0x01;
		break;

	case WHITE_BALANCE_TUNGSTEN:
		ce147_buf_set_wb[1] = 0x02;
		break;

	case WHITE_BALANCE_FLUORESCENT:
		ce147_buf_set_wb[1] = 0x03;
		break;

	default:
		dev_err(&client->dev, "%s: failed: to set_white_balance, "
				"enum: %d\n", __func__, ctrl->value);
		return -EINVAL;
	}

	if (ctrl->value	!= WHITE_BALANCE_AUTO) {
		err = ce147_i2c_write_multi(client, CMD_SET_WB,
				ce147_buf_set_wb, ce147_len_set_wb);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_write for "
					"white_balance\n", __func__);
			return -EIO;
		}
	}
	err = ce147_i2c_write_multi(client, CMD_SET_WB_AUTO,
			ce147_buf_set_wb_auto, ce147_len_set_wb_auto);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"white_balance\n", __func__);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for white_balance\n",
					__func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_ev(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_buf_set_ev[2] = { 0x02, 0x00 };
	unsigned int ce147_len_set_ev =	2;
	unsigned int ce147_ev_offset = 13;

	switch (ctrl->value) {
	case EV_MINUS_4:
		ce147_buf_set_ev[1] = 0x02;
		break;

	case EV_MINUS_3:
		ce147_buf_set_ev[1] = 0x03;
		break;

	case EV_MINUS_2:
		ce147_buf_set_ev[1] = 0x04;
		break;

	case EV_MINUS_1:
		ce147_buf_set_ev[1] = 0x05;
		break;

	case EV_DEFAULT:
		ce147_buf_set_ev[1] = 0x06;
		break;

	case EV_PLUS_1:
		ce147_buf_set_ev[1] = 0x07;
		break;

	case EV_PLUS_2:
		ce147_buf_set_ev[1] = 0x08;
		break;

	case EV_PLUS_3:
		ce147_buf_set_ev[1] = 0x09;
		break;

	case EV_PLUS_4:
		ce147_buf_set_ev[1] = 0x0A;
		break;

	default:
		dev_err(&client->dev, "%s: failed: to set_ev, enum: %d\n",
				__func__, ctrl->value);
		return -EINVAL;
	}

	if (state->hd_preview_on) { /* This is for HD REC preview */
		ce147_buf_set_ev[1] += ce147_ev_offset;
	}
	/* pr_debug("ce147_set_ev: set_ev:, data: 0x%02x\n",
		   ce147_buf_set_ev[1]); */

	err = ce147_i2c_write_multi(client, CMD_SET_WB,
			ce147_buf_set_ev, ce147_len_set_ev);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for set_ev, "
				"HD preview(%d)\n",
					__func__, state->hd_preview_on);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for set_ev\n", __func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_metering(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_buf_set_metering[2] = { 0x00, 0x00 };
	unsigned int ce147_len_set_metering = 2;

	switch (ctrl->value) {
	case METERING_MATRIX:
		ce147_buf_set_metering[1] = 0x02;
		break;

	case METERING_CENTER:
		ce147_buf_set_metering[1] = 0x00;
		break;

	case METERING_SPOT:
		ce147_buf_set_metering[1] = 0x01;
		break;

	default:
		dev_err(&client->dev, "%s: failed: to set_photometry, "
				"enum: %d\n", __func__, ctrl->value);
		return -EINVAL;
	}

	if (state->hd_preview_on)
		ce147_buf_set_metering[1] = 0x03;

	err = ce147_i2c_write_multi(client, CMD_SET_WB,
			ce147_buf_set_metering, ce147_len_set_metering);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"set_photometry\n", __func__);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for set_photometry\n",
					__func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_set_iso(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_iso[2] = { 0x01, 0x00 };
	unsigned int ce147_len_set_iso = 2;

	ce147_msg(&client->dev, "%s: Enter : iso %d\n", __func__, ctrl->value);

	switch (ctrl->value) {
	case ISO_AUTO:
		ce147_buf_set_iso[1] = 0x06;
		break;

	case ISO_50:
		ce147_buf_set_iso[1] = 0x07;
		break;

	case ISO_100:
		ce147_buf_set_iso[1] = 0x08;
		break;

	case ISO_200:
		ce147_buf_set_iso[1] = 0x09;
		break;

	case ISO_400:
		ce147_buf_set_iso[1] = 0x0A;
		break;

	case ISO_800:
		ce147_buf_set_iso[1] = 0x0B;
		break;

	case ISO_1600:
		ce147_buf_set_iso[1] = 0x0C;
		break;

	/* This is additional setting for Sports' scene mode */
	case ISO_SPORTS:
		ce147_buf_set_iso[1] = 0x12;
		break;

	/* This is additional setting for 'Night' scene mode */
	case ISO_NIGHT:
		ce147_buf_set_iso[1] = 0x17;
		break;

	/* This is additional setting for video recording mode */
	case ISO_MOVIE:
		ce147_buf_set_iso[1] = 0x02;
		break;

	default:
		dev_err(&client->dev, "%s: failed: to set_iso, enum: %d\n",
				__func__, ctrl->value);
		return -EINVAL;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_WB,
			ce147_buf_set_iso, ce147_len_set_iso);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for set_iso\n",
				__func__);
		return -EIO;
	}
#if 0 /* remove batch */
	err = ce147_get_batch_reflection_status(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_get_batch_"
				"reflection_status for set_iso\n", __func__);
		return -EIO;
	}
#endif
	ce147_msg(&client->dev, "%s: done, iso: 0x%02x\n",
			__func__, ce147_buf_set_iso[1]);

	return 0;
}

static int ce147_set_gamma(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_buf_set_gamma[2] = { 0x01, 0x00 };
	unsigned int ce147_len_set_gamma = 2;

	unsigned char ce147_buf_set_uv[2] = { 0x03, 0x00 };
	unsigned int ce147_len_set_uv =	2;

	if (ctrl->value == GAMMA_ON) {
		if (state->hd_preview_on) {
			ce147_buf_set_gamma[1] = 0x01;
			ce147_buf_set_uv[1] = 0x01;
		}
	}

	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT,
			ce147_buf_set_gamma, ce147_len_set_gamma);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"ce147_set_gamma\n", __func__);
		return -EIO;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_EFFECT,
			ce147_buf_set_uv, ce147_len_set_uv);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"ce147_set_gamma\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done, gamma: 0x%02x, uv: 0x%02x, hd: %d\n",
			__func__, ce147_buf_set_gamma[1], ce147_buf_set_uv[1],
			state->hd_preview_on);

	return 0;
}

static int ce147_set_slow_ae(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	unsigned char ce147_buf_set_slow_ae[2] = { 0x03, 0x00 };
	unsigned int ce147_len_set_slow_ae = 2;

	if (ctrl->value == SLOW_AE_ON)
		if (state->hd_preview_on)
			ce147_buf_set_slow_ae[1] = 0x02;

	err = ce147_i2c_write_multi(client, CMD_SET_WB,
			ce147_buf_set_slow_ae, ce147_len_set_slow_ae);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"ce147_set_slow_ae\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done, slow_ae: 0x%02x, hd: %d\n",
			__func__, ce147_buf_set_slow_ae[1],
			state->hd_preview_on);

	return 0;
}

static int ce147_set_face_lock(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char ce147_buf_set_fd_lock[1] = { 0x00 };
	unsigned int ce147_len_set_fd_lock = 1;

	switch (ctrl->value) {
	case FACE_LOCK_ON:
		ce147_buf_set_fd_lock[0] = 0x01;
		break;

	case FIRST_FACE_TRACKING:
		ce147_buf_set_fd_lock[0] = 0x02;
		break;

	case FACE_LOCK_OFF:
	default:
		ce147_buf_set_fd_lock[0] = 0x00;
		break;
	}

	err = ce147_i2c_write_multi(client, CMD_SET_FACE_LOCK,
			ce147_buf_set_fd_lock, ce147_len_set_fd_lock);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"face_lock\n", __func__);
		return -EIO;
	}

	ce147_msg(&client->dev, "%s: done\n", __func__);

	return 0;
}

static int ce147_start_auto_focus(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int err;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char ce147_buf_set_af[1] = { 0x00 };
	unsigned int ce147_len_set_af =	1;
	struct ce147_state *state = to_state(sd);

	ce147_msg(&client->dev, "%s\n", __func__);

	/* start af */
	err = ce147_i2c_write_multi(client, CMD_START_AUTO_FOCUS_SEARCH,
			ce147_buf_set_af, ce147_len_set_af);
	state->af_status = AF_START;
	INIT_COMPLETION(state->af_complete);

	if (err < 0) {
		dev_err(&client->dev, "%s: failed: i2c_write for "
				"auto_focus\n", __func__);
		return -EIO;
	}

	return 0;
}

static int ce147_stop_auto_focus(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int err;
	unsigned char ce147_buf_set_af = 0x00;

	ce147_msg(&client->dev, "%s\n", __func__);
	/* stop af */
	err = ce147_i2c_write_multi(client, CMD_STOP_LENS_MOVEMENT,
			&ce147_buf_set_af, 1);
	if (err < 0)
		dev_err(&client->dev, "%s: failed: i2c_write for auto_focus\n",
				__func__);

	if (state->af_status != AF_START) {
		/* we weren't in the middle auto focus operation, we're done */
		dev_dbg(&client->dev,
			"%s: auto focus not in progress, done\n", __func__);

		return 0;
	}

	state->af_status = AF_CANCEL;

	mutex_unlock(&state->ctrl_lock);
	wait_for_completion(&state->af_complete);
	mutex_lock(&state->ctrl_lock);

	return err;
}

static int ce147_get_auto_focus_status(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	unsigned char ce147_buf_get_af_status[1] = { 0x00 };
	int count;
	int err;

	ce147_msg(&client->dev, "%s\n", __func__);

	if (state->af_status == AF_NONE) {
		dev_err(&client->dev,
				"%s: auto focus never started, returning 0x2\n",
				__func__);
		pr_debug("%s: AUTO_FOCUS_CANCELLED\n", __func__);
		ctrl->value = AUTO_FOCUS_CANCELLED;
		return 0;
	}

	/* status check	whether AF searching is	successful or not */
	for (count = 0;	count < 600; count++) {
		mutex_unlock(&state->ctrl_lock);
		msleep(10);
		mutex_lock(&state->ctrl_lock);

		if (state->af_status == AF_CANCEL) {
			dev_err(&client->dev,
				"%s: AF is cancelled while doing\n", __func__);
			ctrl->value = AUTO_FOCUS_CANCELLED;
			goto out;
		}

		ce147_buf_get_af_status[0] = 0xFF;
		err = ce147_i2c_read_multi(client,
				CMD_CHECK_AUTO_FOCUS_SEARCH, NULL, 0,
				ce147_buf_get_af_status, 1);
		if (err < 0) {
			dev_err(&client->dev, "%s: failed: i2c_read "
					"for auto_focus\n", __func__);
			return -EIO;
		}
		if (ce147_buf_get_af_status[0] == 0x05)
			continue;
		if (ce147_buf_get_af_status[0] == 0x00
				|| ce147_buf_get_af_status[0] == 0x02
				|| ce147_buf_get_af_status[0] == 0x04)
			break;

#if defined(CONFIG_ARIES_NTT) /* Modify	NTTS1 */
		if ((ctrl->value == 2) && !state->disable_aeawb_lock) {
			err = ce147_set_awb_lock(sd, 1);
			if (err < 0) {
				dev_err(&client->dev, "%s: failed: "
						"ce147_set_awb_lock, err %d\n",
						__func__, err);
				return -EIO;
			}
		}
#endif
	}

	ctrl->value = FOCUS_MODE_AUTO_DEFAULT;

	if (ce147_buf_get_af_status[0] == 0x02) {
		ctrl->value = AUTO_FOCUS_DONE;
	} else {
		ce147_set_focus_mode(sd, ctrl);
		ctrl->value = AUTO_FOCUS_FAILED;
		goto out;
	}
	ce147_msg(&client->dev, "%s: done\n", __func__);

out:
	state->af_status = AF_NONE;
	complete(&state->af_complete);

	/* pr_debug("ce147_get_auto_focus_status is called"); */
	return 0;
}

static void ce147_init_parameters(struct v4l2_subdev *sd)
{
	struct ce147_state *state = to_state(sd);

	/* Set initial values for the sensor stream parameters */
	state->strm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	state->strm.parm.capture.timeperframe.numerator = 1;
	state->strm.parm.capture.capturemode = 0;

	/* state->framesize_index = CE147_PREVIEW_VGA; */
	/*state->fps = 30;*/ /* Default value */

	state->jpeg.enable = 0;
	state->jpeg.quality = 100;
	state->jpeg.main_offset = 0;
	state->jpeg.main_size = 0;
	state->jpeg.thumb_offset = 0;
	state->jpeg.thumb_size = 0;
	state->jpeg.postview_offset = 0;

}

static int ce147_get_fw_data(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int err = -EINVAL;

	err = ce147_power_en(1, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(on)\n",
				__func__);
		return -EIO;
	}

	err = ce147_load_fw(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Camera Initialization\n",
				__func__);
		return -EIO;
	}

	/* pr_debug("ce147_get_fw_data:	ce147_load_fw is ok\n"); */

	ce147_init_parameters(sd);

	/* pr_debug("ce147_get_fw_data:	ce147_init_parameters is ok\n"); */

	err = ce147_get_fw_version(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading firmware version\n",
				__func__);
		return -EIO;
	}

	/* pr_debug("ce147_get_fw_data:	ce147_get_fw_version is ok\n");	*/

	err = ce147_get_dateinfo(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading dateinfo\n",
				__func__);
		return -EIO;
	}

	/* pr_debug("ce147_get_fw_data:	ce147_get_dateinfo is ok\n"); */

	err = ce147_get_sensor_version(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading sensor info\n",
				__func__);
		return -EIO;
	}

	/* pr_debug("ce147_get_fw_data:	ce147_get_sensor_version is ok\n"); */

	err = ce147_get_sensor_maker_version(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading maker info\n",
				__func__);
		return -EIO;
	}

	err = ce147_get_af_version(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading af info\n",
				__func__);
		return -EIO;
	}

	err = ce147_get_gamma_version(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading camera gamma info\n",
				__func__);
		return -EIO;
	}

	err = ce147_power_en(0, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(off)\n",
				__func__);
		return -EIO;
	}

	/* pr_debug("ce147_get_fw_data:	ce147_power_en is ok\n"); */

	ce147_info(&client->dev, "FW  Version: %d.%d\n",
			state->fw.major, state->fw.minor);
	ce147_info(&client->dev, "PRM Version: %d.%d\n",
			state->prm.major, state->prm.minor);
	ce147_info(&client->dev, "Date(y.m.d): %d.%d.%d\n",
			state->dateinfo.year, state->dateinfo.month,
			state->dateinfo.date);
	ce147_info(&client->dev, "Sensor version: %d\n",
			state->sensor_version);

	return 0;
}

/* s1_camera [ Defense process by ESD input ] _[ */
static int ce147_reset(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;

	dev_err(&client->dev, "%s: Enter\n", __func__);

	err = ce147_power_en(0, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(off)\n",
				__func__);
		return -EIO;
	}

	mdelay(5);

	err = ce147_power_en(1, sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: failed: ce147_power_en(off)\n",
				__func__);
		return -EIO;
	}

	err = ce147_load_fw(sd);		/* ce147_init(sd); */
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Camera Initialization\n",
				__func__);
		return -EIO;
	}

	return 0;
}
/* _] */

#if 0
/* Sample code */
static const char *ce147_querymenu_wb_preset[] = {
	"WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL
};
#endif

static struct v4l2_queryctrl ce147_controls[] = {
#if 0
	/* Sample code */
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type =	V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(ce147_querymenu_wb_preset) - 2,
		.step =	1,
		.default_value = 0,
	},
#endif
};

const char **ce147_ctrl_get_menu(u32 id)
{
	switch (id) {
#if 0
	/* Sample code */
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return ce147_querymenu_wb_preset;
#endif
	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *ce147_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ce147_controls); i++)
		if (ce147_controls[i].id == id)
			return &ce147_controls[i];

	return NULL;
}

static int ce147_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ce147_controls); i++) {
		if (ce147_controls[i].id == qc->id) {
			memcpy(qc, &ce147_controls[i],
					sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int ce147_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	ce147_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm,	&qctrl, ce147_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from	host and return EINVAL if not supported	clock
 * frequency is expected
 *	freq : in Hz
 *	flag : not supported for now
 */
static int ce147_s_crystal_freq(struct v4l2_subdev *sd, u32 freq, u32 flags)
{
	int err = -EINVAL;

	return err;
}

static int ce147_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	return err;
}

static int ce147_get_framesize_index(struct v4l2_subdev *sd);
static int ce147_set_framesize_index(struct v4l2_subdev *sd,
					unsigned int index);
/* Information received:
 * width, height
 * pixel_format -> to be handled in the	upper layer
 *
 * */
static int ce147_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int framesize_index = -1;

	if (fmt->fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG
			&& fmt->fmt.pix.colorspace != V4L2_COLORSPACE_JPEG) {
		dev_err(&client->dev, "%s: mismatch in pixelformat and "
				"colorspace\n", __func__);
		return -EINVAL;
	}

	state->pix.width = fmt->fmt.pix.width;
	state->pix.height = fmt->fmt.pix.height;
	state->pix.pixelformat = fmt->fmt.pix.pixelformat;

	if (fmt->fmt.pix.colorspace == V4L2_COLORSPACE_JPEG)
		state->oprmode = CE147_OPRMODE_IMAGE;
	else
		state->oprmode = CE147_OPRMODE_VIDEO;

	framesize_index	= ce147_get_framesize_index(sd);

	ce147_msg(&client->dev, "%s:framesize_index = %d\n",
		       __func__, framesize_index);

	err = ce147_set_framesize_index(sd, framesize_index);
	if (err < 0) {
		dev_err(&client->dev, "%s: set_framesize_index failed\n",
			      __func__);
		return -EINVAL;
	}

	if (state->pix.pixelformat == V4L2_PIX_FMT_JPEG)
		state->jpeg.enable = 1;
	else
		state->jpeg.enable = 0;

	if (state->oprmode == CE147_OPRMODE_VIDEO) {
		if (framesize_index == CE147_PREVIEW_720P)
			state->hd_preview_on = 1;
		else
			state->hd_preview_on = 0;
	}

	return 0;
}

static int ce147_enum_framesizes(struct	v4l2_subdev *sd,
					struct v4l2_frmsizeenum *fsize)
{
	struct ce147_state *state = to_state(sd);
	int num_entries	= sizeof(ce147_framesize_list)
				/ sizeof(struct ce147_enum_framesize);
	struct ce147_enum_framesize *elem;
	int index = 0;
	int i = 0;

	/* The camera interface	should read this value, this is the resolution
	 * at which the sensor would provide framedata to the camera i/f
	 * In case of image capture,
	 * this returns	the default camera resolution (VGA)
	 */
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	if (state->pix.pixelformat == V4L2_PIX_FMT_JPEG)
		index =	CE147_PREVIEW_VGA;
	else
		index =	state->framesize_index;

	for (i = 0; i < num_entries; i++) {
		elem = &ce147_framesize_list[i];
		if (elem->index == index) {
			fsize->discrete.width =
				ce147_framesize_list[index].width;
			fsize->discrete.height =
				ce147_framesize_list[index].height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ce147_enum_frameintervals(struct v4l2_subdev *sd,
					struct v4l2_frmivalenum *fival)
{
	int err = 0;

	return err;
}

static int ce147_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int num_entries;

	num_entries = sizeof(capture_fmts) / sizeof(struct v4l2_fmtdesc);

	if (fmtdesc->index >= num_entries)
		return -EINVAL;

	memset(fmtdesc, 0, sizeof(*fmtdesc));
	memcpy(fmtdesc, &capture_fmts[fmtdesc->index], sizeof(*fmtdesc));

	return 0;
}

static int ce147_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int num_entries;
	int i;

	num_entries = sizeof(capture_fmts) / sizeof(struct v4l2_fmtdesc);

	for (i = 0; i < num_entries; i++) {
		if (capture_fmts[i].pixelformat == fmt->fmt.pix.pixelformat)
			return 0;
	}

	return -EINVAL;
}

/** Gets current FPS value */
static int ce147_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct ce147_state *state = to_state(sd);
	int err = 0;

	state->strm.parm.capture.timeperframe.numerator = 1;
	state->strm.parm.capture.timeperframe.denominator = state->fps;

	memcpy(param, &state->strm, sizeof(param));

	return err;
}

/** Sets the FPS value */
static int ce147_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);

	if (param->parm.capture.timeperframe.numerator
			!= state->strm.parm.capture.timeperframe.numerator
		|| param->parm.capture.timeperframe.denominator
			!= state->strm.parm.capture.timeperframe.denominator) {

		int fps = 0;
		int fps_max = 30;

		if (param->parm.capture.timeperframe.numerator
				&& param->parm.capture.timeperframe.denominator)
			fps = (int)(param->parm.capture.timeperframe.denominator
					/ param->parm.capture
					.timeperframe.numerator);
		else
			fps = 0;

		if (fps	<= 0 || fps > fps_max) {
			dev_err(&client->dev, "%s: Framerate %d not supported, "
					"setting it to %d fps.\n",
					__func__, fps, fps_max);
			fps = fps_max;
		}

		state->strm.parm.capture.timeperframe.numerator = 1;
		state->strm.parm.capture.timeperframe.denominator = fps;

		state->fps = fps;
	}

	/* Don't set the fps value, just update it in the state
	 * We will set the resolution and fps in the start operation
	 * (preview/capture) call */

	return err;
}

/* This	function is called from the g_ctrl api
 *
 * This	function should	be called only after the s_fmt call,
 * which sets the required width/height value.
 *
 * It checks a list of available frame sizes and returns the
 * most	appropriate index of the frame size.
 *
 * Note: The index is not the index of the entry in the list. It is
 * the value of the member 'index' of the particular entry. This is
 * done	to add additional layer of error checking.
 *
 * The list is stored in an increasing order (as far as possible).
 * Hene	the first entry (searching from	the beginning) where both the
 * width and height is more than the required value is returned.
 * In case of no match, we return the last entry (which is supposed
 * to be the largest resolution supported.)
 *
 * It returns the index (enum ce147_frame_size) of the framesize entry.
 */
static int ce147_get_framesize_index(struct v4l2_subdev *sd)
{
	int i =	0;
	struct ce147_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_enum_framesize *frmsize;

	ce147_msg(&client->dev, "%s: Requested Res: %dx%d\n",
			__func__, state->pix.width, state->pix.height);

	/* Check for video/image mode */
	for (i = 0; i < (sizeof(ce147_framesize_list)
				/ sizeof(struct	ce147_enum_framesize));	i++) {
		frmsize	= &ce147_framesize_list[i];

		if (frmsize->mode != state->oprmode)
			continue;

		if (state->oprmode == CE147_OPRMODE_IMAGE) {
			/* In case of image capture mode,
			 * if the given image resolution is not supported,
			 * return the next higher image resolution. */
			/* pr_debug("frmsize->width(%d) state->pix.width(%d) "
					"frmsize->height(%d) "
					"state->pix.height(%d)\n",
						frmsize->width,
						state->pix.width,
						frmsize->height,
						state->pix.height); */
			if (frmsize->width == state->pix.width
				&& frmsize->height == state->pix.height) {
				/* pr_debug("frmsize->index(%d)\n",
						frmsize->index); */
				return frmsize->index;
			}
		} else {
			/* In case of video mode,
			 * if the given video resolution is not matching, use
			 * the default rate (currently CE147_PREVIEW_VGA).
			 */
			/* pr_debug("frmsize->width(%d) state->pix.width(%d) "
					"frmsize->height(%d) "
					"state->pix.height(%d)\n",
						frmsize->width,
						state->pix.width,
						frmsize->height,
						state->pix.height); */
			if (frmsize->width == state->pix.width
				&& frmsize->height == state->pix.height) {
				/* pr_debug("frmsize->index(%d)\n",
						   frmsize->index); */
				return frmsize->index;
			}
		}
	}
	/* If it fails, return the default value. */
	return (state->oprmode == CE147_OPRMODE_IMAGE)
			? CE147_CAPTURE_3MP : CE147_PREVIEW_VGA;
}


/* This	function is called from the s_ctrl api
 * Given the index, it checks if it is a valid index.
 * On success, it returns 0.
 * On Failure, it returns -EINVAL
 */
static int ce147_set_framesize_index(struct v4l2_subdev *sd, unsigned int index)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int i = 0;

	for (i = 0; i < (sizeof(ce147_framesize_list)
				/ sizeof(struct	ce147_enum_framesize)); i++) {
		if (ce147_framesize_list[i].index == index
			&& ce147_framesize_list[i].mode == state->oprmode) {
			state->framesize_index = ce147_framesize_list[i].index;
			state->pix.width = ce147_framesize_list[i].width;
			state->pix.height = ce147_framesize_list[i].height;
			ce147_info(&client->dev, "%s: Camera Res: %dx%d\n",
				       __func__, state->pix.width,
				       state->pix.height);
			return 0;
		}
	}

	return -EINVAL;
}

static int ce147_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	struct ce147_userset userset = state->userset;
	int err = -ENOIOCTLCMD;

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;

	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		err = 0;
		break;

	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;

	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;

	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.sharpness;
		err = 0;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_SIZE:
		ctrl->value = state->jpeg.main_size;
		err = 0;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_OFFSET:
		ctrl->value = state->jpeg.main_offset;
		err = 0;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_SIZE:
		ctrl->value = state->jpeg.thumb_size;
		err = 0;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_OFFSET:
		ctrl->value = state->jpeg.thumb_offset;
		err = 0;
		break;

	case V4L2_CID_CAM_JPEG_POSTVIEW_OFFSET:
		ctrl->value = state->jpeg.postview_offset;
		err = 0;
		break;

	case V4L2_CID_CAM_JPEG_MEMSIZE:
		ctrl->value = SENSOR_JPEG_SNAPSHOT_MEMSIZE;
		err = 0;
		break;

	/* need to be modified */
	case V4L2_CID_CAM_JPEG_QUALITY:
		ctrl->value = state->jpeg.quality;
		err = 0;
		break;

	case V4L2_CID_CAMERA_OBJ_TRACKING_STATUS:
		err = ce147_get_object_tracking(sd, ctrl);
		ctrl->value = state->ot_status;
		break;

	case V4L2_CID_CAMERA_SMART_AUTO_STATUS:
		err = ce147_get_smart_auto_status(sd, ctrl);
		ctrl->value = state->sa_status;
		break;

	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		err = ce147_get_auto_focus_status(sd, ctrl);
		break;

	case V4L2_CID_CAM_DATE_INFO_YEAR:
		ctrl->value = state->dateinfo.year;
		err = 0;
		break;

	case V4L2_CID_CAM_DATE_INFO_MONTH:
		ctrl->value = state->dateinfo.month;
		err = 0;
		break;

	case V4L2_CID_CAM_DATE_INFO_DATE:
		ctrl->value = state->dateinfo.date;
		err = 0;
		break;

	case V4L2_CID_CAM_SENSOR_VER:
		ctrl->value = state->sensor_version;
		err = 0;
		break;

	case V4L2_CID_CAM_FW_MINOR_VER:
		ctrl->value = state->fw.minor;
		err = 0;
		break;

	case V4L2_CID_CAM_FW_MAJOR_VER:
		ctrl->value = state->fw.major;
		err = 0;
		break;

	case V4L2_CID_CAM_PRM_MINOR_VER:
		ctrl->value = state->prm.minor;
		err = 0;
		break;

	case V4L2_CID_CAM_PRM_MAJOR_VER:
		ctrl->value = state->prm.major;
		err = 0;
		break;

	case V4L2_CID_CAM_SENSOR_MAKER:
		ctrl->value = state->sensor_info.maker;
		err = 0;
		break;

	case V4L2_CID_CAM_SENSOR_OPTICAL:
		ctrl->value = state->sensor_info.optical;
		err = 0;
		break;

	case V4L2_CID_CAM_AF_VER_LOW:
		ctrl->value = state->af_info.low;
		err = 0;
		break;

	case V4L2_CID_CAM_AF_VER_HIGH:
		ctrl->value = state->af_info.high;
		err = 0;
		break;

	case V4L2_CID_CAM_GAMMA_RG_LOW:
		ctrl->value = state->gamma.rg_low;
		err = 0;
		break;

	case V4L2_CID_CAM_GAMMA_RG_HIGH:
		ctrl->value = state->gamma.rg_high;
		err = 0;
		break;

	case V4L2_CID_CAM_GAMMA_BG_LOW:
		ctrl->value = state->gamma.bg_low;
		err = 0;
		break;

	case V4L2_CID_CAM_GAMMA_BG_HIGH:
		ctrl->value = state->gamma.bg_high;
		err = 0;
		break;

	case V4L2_CID_CAM_GET_DUMP_SIZE:
		ctrl->value = state->fw_dump_size;
		err = 0;
		break;

	case V4L2_CID_MAIN_SW_DATE_INFO_YEAR:
		ctrl->value = state->main_sw_dateinfo.year;
		err = 0;
		break;

	case V4L2_CID_MAIN_SW_DATE_INFO_MONTH:
		ctrl->value = state->main_sw_dateinfo.month;
		err = 0;
		break;

	case V4L2_CID_MAIN_SW_DATE_INFO_DATE:
		ctrl->value = state->main_sw_dateinfo.date;
		err = 0;
		break;

	case V4L2_CID_MAIN_SW_FW_MINOR_VER:
		ctrl->value = state->main_sw_fw.minor;
		err = 0;
		break;

	case V4L2_CID_MAIN_SW_FW_MAJOR_VER:
		ctrl->value = state->main_sw_fw.major;
		err = 0;
		break;

	case V4L2_CID_MAIN_SW_PRM_MINOR_VER:
		ctrl->value = state->main_sw_prm.minor;
		err = 0;
		break;

	case V4L2_CID_MAIN_SW_PRM_MAJOR_VER:
		ctrl->value = state->main_sw_prm.major;
		err = 0;
		break;

	default:
		dev_err(&client->dev, "%s: no such ctrl\n", __func__);
		break;
	}

	mutex_unlock(&state->ctrl_lock);

	return err;
}

static int ce147_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int err = -ENOIOCTLCMD;
	int value = ctrl->value;

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK:
		err = ce147_set_ae_awb(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		err = ce147_set_flash(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		if (state->runmode != CE147_RUNMODE_RUNNING) {
			state->ev = ctrl->value;
			err = 0;
		} else
			err = ce147_set_ev(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		if (state->runmode != CE147_RUNMODE_RUNNING) {
			state->wb = ctrl->value;
			err = 0;
		} else
			err = ce147_set_white_balance(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_EFFECT:
		if (state->runmode != CE147_RUNMODE_RUNNING) {
			state->effect =	ctrl->value;
			err = 0;
		} else
			err = ce147_set_effect(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_ISO:
		if (state->runmode != CE147_RUNMODE_RUNNING) {
			state->iso = ctrl->value;
			err = 0;
		} else
			err = ce147_set_iso(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_METERING:
		if (state->runmode != CE147_RUNMODE_RUNNING) {
			state->metering	= ctrl->value;
			err = 0;
		} else
			err = ce147_set_metering(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_CONTRAST:
		err = ce147_set_contrast(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SATURATION:
		err = ce147_set_saturation(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SHARPNESS:
		err = ce147_set_sharpness(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_WDR:
		err = ce147_set_wdr(sd,	ctrl);
		break;

	case V4L2_CID_CAMERA_ANTI_SHAKE:
		err = ce147_set_anti_shake(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_FACE_DETECTION:
		err = ce147_set_face_detection(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SMART_AUTO:
		err = ce147_set_smart_auto(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_FOCUS_MODE:
		err = ce147_set_focus_mode(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_VINTAGE_MODE:
		err = ce147_set_vintage_mode(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_BEAUTY_SHOT:
		err = ce147_set_face_beauty(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK:
		err = ce147_set_face_lock(sd, ctrl);
		break;

	/* need to be modified */
	case V4L2_CID_CAM_JPEG_QUALITY:
		if (ctrl->value < 0 || ctrl->value > 100)
			err = -EINVAL;
		else {
			state->jpeg.quality = ctrl->value;
			err = 0;
		}
		break;

	case V4L2_CID_CAMERA_ZOOM:
		err = ce147_set_dzoom(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		err = ce147_set_touch_auto_focus(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_CAF_START_STOP:
		err = ce147_set_continous_af(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		state->position.x = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		state->position.y = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP:
		err = ce147_set_object_tracking(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		if (value == AUTO_FOCUS_ON)
			err = ce147_start_auto_focus(sd, ctrl);
		else if (value == AUTO_FOCUS_OFF)
			err = ce147_stop_auto_focus(sd);
		else {
			err = -EINVAL;
			dev_err(&client->dev,
					"%s: bad focus value requestion %d\n",
					__func__, value);
		}
		break;

	case V4L2_CID_CAMERA_FRAME_RATE:
		state->fps = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_ANTI_BANDING:
		state->anti_banding = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_SET_GAMMA:
		state->hd_gamma = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_SET_SLOW_AE:
		state->hd_slow_ae = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_CAPTURE:
		err = ce147_set_capture_start(sd, ctrl);
		break;

	case V4L2_CID_CAM_CAPTURE:
		err = ce147_set_capture_config(sd, ctrl);
		break;

	/* Used to start / stop preview operation.
	* This call can be modified to START/STOP operation,
	* which can be used in image capture also */
	case V4L2_CID_CAM_PREVIEW_ONOFF:
		if (ctrl->value)
			err = ce147_set_preview_start(sd);
		else
			err = ce147_set_preview_stop(sd);
		break;

	case V4L2_CID_CAM_UPDATE_FW:
		err = ce147_update_fw(sd);
		break;

	case V4L2_CID_CAM_SET_FW_ADDR:
		state->fw_info.addr = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAM_SET_FW_SIZE:
		state->fw_info.size = ctrl->value;
		err = 0;
		break;

#if defined(CONFIG_ARIES_NTT) /* Modify	NTTS1 */
	case V4L2_CID_CAMERA_AE_AWB_DISABLE_LOCK:
		state->disable_aeawb_lock = ctrl->value;
		err = 0;
		break;
#endif

	case V4L2_CID_CAM_FW_VER:
		err = ce147_get_fw_data(sd);
		break;

	case V4L2_CID_CAM_DUMP_FW:
		err = ce147_dump_fw(sd);
		break;

	case V4L2_CID_CAMERA_BATCH_REFLECTION:
		err = ce147_get_batch_reflection_status(sd);
		break;

	case V4L2_CID_CAMERA_EXIF_ORIENTATION:
		state->exif_orientation_info = ctrl->value;
		err = 0;
		break;

	/* s1_camera [ Defense process by ESD input ] _[ */
	case V4L2_CID_CAMERA_RESET:
		dev_err(&client->dev, "%s: V4L2_CID_CAMERA_RESET\n", __func__);
		pr_debug("======ESD\n");
		err = ce147_reset(sd);
		break;
	/* _] */

	case V4L2_CID_CAMERA_CHECK_DATALINE:
		state->check_dataline =	ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE_STOP:
		err = ce147_check_dataline_stop(sd);
		break;

	case V4L2_CID_CAMERA_THUMBNAIL_NULL:
		state->thumb_null = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_LENS_SOFTLANDING:
		ce147_set_af_softlanding(sd);
		err = 0;

	default:
		dev_err(&client->dev, "%s: no such control\n", __func__);
		break;
	}

	if (err < 0)
		dev_err(&client->dev, "%s: vidioc_s_ctrl failed	%d, "
				"s_ctrl: id(%d), value(%d)\n",
					__func__, err, (ctrl->id - V4L2_CID_PRIVATE_BASE),
					ctrl->value);

	mutex_unlock(&state->ctrl_lock);

	return err;
}
static int ce147_s_ext_ctrls(struct v4l2_subdev *sd,
				struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int ret = 0;
	int i;

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		ret = ce147_s_ext_ctrl(sd, ctrl);

		if (ret) {
			ctrls->error_idx = i;
			break;
		}
	}

	return ret;
}

static int ce147_s_ext_ctrl(struct v4l2_subdev *sd,
				struct v4l2_ext_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int err = -ENOIOCTLCMD;
	unsigned long temp = 0;
	char *temp2;
	struct gps_info_common *tempGPSType = NULL;

	state->exif_ctrl = 0;

	switch (ctrl->id) {

	case V4L2_CID_CAMERA_GPS_LATITUDE:
		tempGPSType = (struct gps_info_common *)ctrl->reserved2[1];
		state->gpsInfo.ce147_gps_buf[0] = tempGPSType->direction;
		state->gpsInfo.ce147_gps_buf[1] = tempGPSType->dgree;
		state->gpsInfo.ce147_gps_buf[2] = tempGPSType->minute;
		state->gpsInfo.ce147_gps_buf[3] = tempGPSType->second;

		if ((tempGPSType->direction == 0)
				&& (tempGPSType->dgree == 0)
				&& (tempGPSType->minute == 0)
				&& (tempGPSType->second == 0))
			condition = 1;
		else
			condition = 0;

		err = 0;
		break;

	case V4L2_CID_CAMERA_GPS_LONGITUDE:
		tempGPSType = (struct gps_info_common *)ctrl->reserved2[1];
		state->gpsInfo.ce147_gps_buf[4] = tempGPSType->direction;
		state->gpsInfo.ce147_gps_buf[5] = tempGPSType->dgree;
		state->gpsInfo.ce147_gps_buf[6] = tempGPSType->minute;
		state->gpsInfo.ce147_gps_buf[7] = tempGPSType->second;

		if ((tempGPSType->direction == 0)
				&& (tempGPSType->dgree == 0)
				&& (tempGPSType->minute == 0)
				&& (tempGPSType->second == 0))
			condition = 1;
		else
			condition = 0;

		err = 0;
		break;

	case V4L2_CID_CAMERA_GPS_ALTITUDE:
		tempGPSType = (struct gps_info_common *)ctrl->reserved2[1];
		state->gpsInfo.ce147_altitude_buf[0] = tempGPSType->direction;
		state->gpsInfo.ce147_altitude_buf[1] = (tempGPSType->dgree)
								& 0x00ff;
		state->gpsInfo.ce147_altitude_buf[2] = ((tempGPSType->dgree)
								& 0xff00) >> 8;
		state->gpsInfo.ce147_altitude_buf[3] = tempGPSType->minute;

		err = 0;
		break;

	case V4L2_CID_CAMERA_GPS_TIMESTAMP:
		/* state->gpsInfo.gps_timeStamp	=
			(struct tm*)ctrl->reserved2[1]; */
		temp = *((unsigned long	*)ctrl->reserved2[1]);
		state->gpsInfo.gps_timeStamp = temp;
		err = 0;
		break;

	case V4L2_CID_CAMERA_EXIF_TIME_INFO:
		state->exifTimeInfo = (struct tm *)ctrl->reserved2[1];
		err = 0;
		break;

	case V4L2_CID_CAMERA_GPS_PROCESSINGMETHOD:
		temp2 = ((char *)ctrl->reserved2[1]);
		strcpy(state->gpsInfo.gps_processingmethod, temp2);
		err = 0;
		break;
	}

	if (condition)
		state->exif_ctrl = 1;

	if (err < 0)
		dev_err(&client->dev, "%s: vidioc_s_ext_ctrl failed %d\n",
				__func__, err);

	return err;
}

#ifdef FACTORY_CHECK
ssize_t camtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s \n", __func__);
	char * sensorname = "NG";
	sensorname = "SONY_IMX072ES_CE147";
	return sprintf(buf,"%s\n", sensorname);
}


ssize_t camtype_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	printk(KERN_NOTICE "%s:%s\n", __func__, buf);

	return size;
}

static DEVICE_ATTR(camtype,0644, camtype_show, camtype_store);

extern struct class *sec_class;
struct device *sec_cam_dev = NULL;
#endif

static int ce147_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	int err = -EINVAL;

	err = ce147_load_fw(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Camera Initialization\n",
				__func__);
		return -EIO;
	}

	ce147_init_parameters(sd);

	err = ce147_get_fw_version(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading firmware version\n",
				__func__);
		return -EIO;
	}

	err = ce147_get_dateinfo(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading dateinfo\n",
				__func__);
		return -EIO;
	}

	err = ce147_get_sensor_version(sd);
	if (err < 0) {
		dev_err(&client->dev, "%s: Failed: Reading sensor info\n",
				__func__);
		return -EIO;
	}
	pr_debug("fw M:%d m:%d |prm M:%d m:%d\n",
			MAIN_SW_FW[0], MAIN_SW_FW[1],
			MAIN_SW_FW[2], MAIN_SW_FW[3]);
	pr_debug("y. m. d = %d.%d.%d\n",
			MAIN_SW_DATE_INFO[0], MAIN_SW_DATE_INFO[1],
			MAIN_SW_DATE_INFO[2]);

	err = ce147_get_main_sw_fw_version(sd);
	if (err < 0) {
		/*dev_err(&client->dev, "%s: Failed: Reading Main	SW Version\n",
				__func__);*/
		return -EIO;
	}

	pr_debug("FW  Version: %d.%d\n", state->fw.major, state->fw.minor);
	pr_debug("PRM Version: %d.%d\n", state->prm.major, state->prm.minor);
	pr_debug("Date(y.m.d): %d.%d.%d\n", state->dateinfo.year,
			state->dateinfo.month, state->dateinfo.date);
	pr_debug("Sensor version: %d\n", state->sensor_version);

	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time
 * therefor, it is not necessary to be initialized on probe time.
 * except for version checking
 * NOTE: version checking is optional
 */
static int ce147_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ce147_state *state = to_state(sd);
	struct ce147_platform_data *pdata;

	pdata =	client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_PIX_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = DEFUALT_MCLK;	/* 24MHz default */
	else
		state->freq = pdata->freq;

	return 0;
}

static const struct v4l2_subdev_core_ops ce147_core_ops = {
	.init		= ce147_init,		/* initializing API */
	.s_config	= ce147_s_config,	/* Fetch platform data */
	.queryctrl	= ce147_queryctrl,
	.querymenu	= ce147_querymenu,
	.g_ctrl		= ce147_g_ctrl,
	.s_ctrl		= ce147_s_ctrl,
	.s_ext_ctrls	= ce147_s_ext_ctrls,
};

static const struct v4l2_subdev_video_ops ce147_video_ops = {
	.s_crystal_freq		= ce147_s_crystal_freq,
	.g_fmt			= ce147_g_fmt,
	.s_fmt			= ce147_s_fmt,
	.enum_framesizes	= ce147_enum_framesizes,
	.enum_frameintervals	= ce147_enum_frameintervals,
	.enum_fmt		= ce147_enum_fmt,
	.try_fmt		= ce147_try_fmt,
	.g_parm			= ce147_g_parm,
	.s_parm			= ce147_s_parm,
};

static const struct v4l2_subdev_ops ce147_ops = {
	.core	= &ce147_core_ops,
	.video	= &ce147_video_ops,
};

/*
 * ce147_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we	just register subdev device
 */
static int ce147_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct ce147_state *state;
	struct v4l2_subdev *sd;

	state = kzalloc(sizeof(struct ce147_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	mutex_init(&state->ctrl_lock);
	init_completion(&state->af_complete);

	state->runmode = CE147_RUNMODE_NOTREADY;
	state->pre_focus_mode = -1;
	state->af_status = -2;

	sd = &state->sd;
	strcpy(sd->name, CE147_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &ce147_ops);

#ifdef FACTORY_CHECK
	{
		static bool  camtype_init = false;
		if (sec_cam_dev == NULL)
		{
			sec_cam_dev = device_create(sec_class, NULL, 0, NULL, "sec_cam");
			if (IS_ERR(sec_cam_dev))
				pr_err("Failed to create device(sec_lcd_dev)!\n");
		}
	
		if (sec_cam_dev != NULL && camtype_init == false)
		{
			camtype_init = true;
			if (device_create_file(sec_cam_dev, &dev_attr_camtype) < 0)
				pr_err("Failed to create device file(%s)!\n", dev_attr_camtype.attr.name);
		}
	}
#endif

	ce147_info(&client->dev, "5MP camera CE147 loaded.\n");

	return 0;
}

static int ce147_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ce147_state *state = to_state(sd);

	ce147_set_af_softlanding(sd);

	v4l2_device_unregister_subdev(sd);
	mutex_destroy(&state->ctrl_lock);
	kfree(to_state(sd));

	ce147_info(&client->dev, "Unloaded camera sensor CE147.\n");

	return 0;
}

static const struct i2c_device_id ce147_id[] = {
	{ CE147_DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ce147_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name	= CE147_DRIVER_NAME,
	.probe	= ce147_probe,
	.remove	= ce147_remove,
	.id_table = ce147_id,
};

MODULE_DESCRIPTION("NEC CE147-NEC 5MP camera driver");
MODULE_AUTHOR("Tushar Behera <tushar.b@samsung.com>");
MODULE_LICENSE("GPL");
