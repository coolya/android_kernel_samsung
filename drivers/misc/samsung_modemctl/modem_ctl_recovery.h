#ifndef __MODEM_CTL_RECOVERY_H__
#define __MODEM_CTL_RECOVERY_H__

/* The below macros define the offsets from the base shared memory */
#define DPRAM_BOOT_MAGIC_ADDR		0x00
#define DPRAM_BOOT_TYPE_ADDR		0x04
#define DPRAM_MODEM_STATUS_ADDR		0x08
#define DPRAM_AP_STATUS_ADDR		0x0C
#define DPRAM_FIRMWARE_SIZE_ADDR	0x10
#define DPRAM_MODEM_STRING_MSG_ADDR	0x100
#define DPRAM_FIRMWARE_ADDR		0x1000

/* Max. length of the message from modem */
#define DPRAM_MODEM_MSG_SIZE		0x100

#define DPRAM_BOOT_MAGIC_RECOVERY_FOTA	0x56434552
#define DPRAM_BOOT_TYPE_DPRAM_DELTA	0x41544c44
#define DPRAM_BOOT_SEM_REQ		0x5555ffff

/* ioctl commands of updating modem binary */
#define IOCTL_MODEM_FW_UPDATE	_IO('D', 0x1)
#define IOCTL_MODEM_CHK_STAT	_IO('D', 0x2)
#define IOCTL_MODEM_PWROFF	_IO('D', 0x3)

/*
* All status values are kept through out the process.
* So the final status value for a successful job will be 0xB60x1164
* This means that magic code is	B6xxxxxx
* Job was started		xxxxx1xx
* Job is done 100%		xxxxxx64 (0x64 is 100 in hex)
* Job is completed		xxxx1xxx
* This way we can just check the final value and know the status of the job.
*/
#define STATUS_JOB_MAGIC_CODE	0xB6000000
#define STATUS_JOB_MAGIC_M	0xFF000000
#define STATUS_JOB_STARTED_M	0x00000100
#define STATUS_JOB_PROGRESS_M	0x000000FF
#define STATUS_JOB_COMPLETE_M	0x00001000
#define STATUS_JOB_DEBUG_M	0x000F0000
#define STATUS_JOB_ERROR_M	0x00F00000
#define STATUS_JOB_ENDED_M	(0x00F00000|0x00001000)


#define DPRAM_MEMORY_SIZE	0xFFF800

/* read modem delta file to the buffer of this type */
struct dpram_firmware {
	char *firmware;
	int size;
	int is_delta;
};

/* the progress status of modem updage */
struct stat_info {
	int pct;
	char msg[DPRAM_MODEM_MSG_SIZE];
};

/* Define Full modem update interface between AP and modem */
#define ONEDRAM_DL_SIGNATURE		0x4F4E
#define ONEDRAM_DL_SMD_SIGNATURE	0x605F
#define ONEDRAM_DL_COMPLETE		0x56781234
#define ONEDRAM_DL_CHECKSUM_ERR		0x4444
#define ONEDRAM_DL_ERASE_WRITE_ERR	0x77779999
#define ONEDRAM_DL_BOOT_UPDATE_ERR	0xBBBBEEEE
#define ONEDRAM_DL_REWRITE_FAIL_ERR	0xDDDDFFFF
#define ONEDRAM_DL_DONE_AND_RESET	0xDDDDAAAA
#define ONEDRAM_DL_LENGTH_CH_FAIL	0x55FF

#define ONEDRAM_DL_HEADER_OFFSET	0x0
#define ONEDRAM_DL_DATA_OFFSET		0x400

/* typedefs */
struct onedram_head_t {
	u16 signature;
	u8  is_boot_update;
	u8  is_nv_update;
	u32 length;
	u32 checksum;
} __packed;


#endif	/* __MODEM_CTL_RECOVERY_H__ */


