/* linux/drivers/mtd/onenand/samsung.h
 *
 * Generic Partition Layout for S5PV210
 *
 */


struct mtd_partition s3c_partition_info[] = {
	{
		.name		= "misc",
		.offset		= (768*SZ_1K),          /* for bootloader */
		.size		= (256*SZ_1K),
		.mask_flags	= MTD_CAP_NANDFLASH,
	},
	{
		.name		= "recovery",
		.offset		= MTDPART_OFS_APPEND,
		.size		= (5*SZ_1M),
	},
	{
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= (5*SZ_1M),
	},
	{
		.name		= "ramdisk",
		.offset		= MTDPART_OFS_APPEND,
		.size		= (3*SZ_1M),
	},
	{
		.name		= "system",
		.offset		= MTDPART_OFS_APPEND,
		.size		= (90*SZ_1M),
	},
	{
		.name		= "cache",
		.offset		= MTDPART_OFS_APPEND,
		.size		= (80*SZ_1M),
	},
	{
		.name		= "userdata",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
	}
};

