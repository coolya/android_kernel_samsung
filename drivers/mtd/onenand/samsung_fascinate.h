/* linux/drivers/mtd/onenand/samsung_fascinate.h
 *
 * Partition Layout for Samsung Fascinate
 *
 */

struct mtd_partition s3c_partition_info[] = {

 /*This is partition layout from the oneNAND it SHOULD match the pitfile on the second page of the NAND.
   It will work if it doesn't but beware to write below the adress 0x01200000 there are the bootloaders.
   Currently we won't map them, but we should keep that in mind for later things like flashing bootloader
   from Linux. There is a partition 'efs' starting @ 0x00080000 40 256K pages long, it contains data for
   the modem like IMSI we don't touch it for now, but we need the data from it, we create a partition
   for that and copy the data from it. For this you need a image from it and mount it as vfat or copy
   it on a kernel with rfs support on the phone.
   
   Partitions on the lower NAND adresses:
   
   0x00000000 - 0x0003FFFF = first stage bootloader
   0x00040000 - 0x0007FFFF = PIT for second stage bootloader
   0x00080000 - 0x00A7FFFF = EFS: IMSI and NVRAM for the modem
   0x00A80000 - 0x00BBFFFF = second stage bootloader
   0x00BC0000 - 0x00CFFFFF = backup of the second stage bootloader (should be loaded if the other fails, unconfirmed!)
   0x00D00000 - 0x011FFFFF = PARAM.lfs config the bootloader
   
   #########################################################################################
   #########################################################################################
   ###### NEVER TOUCH THE FIRST 2 256k PAGES! THEY CONTAIN THE FIRST STAGE BOOTLOADER ######
   #########################################################################################
   #########################################################################################*/ 
                                                                   
        {
		.name		= "boot",
		.offset		= (72*SZ_256K),
		.size		= (30*SZ_256K), //101
	},
	{
		.name		= "recovery",
		.offset		= (102*SZ_256K),
		.size		= (30*SZ_256K), //131
	},
	{	
		.name		= "system",
		.offset		= (132*SZ_256K),
		.size		= (1074*SZ_256K), //1205
	},
	{	
		.name		= "userdata",
		.offset		= (1206*SZ_256K),
		.size		= (2056*SZ_256K), //3261
	},
	{       /* we should consider moving this before the modem at the end
	           that would allow us to change the partitions before without
	           loosing ths sensible data*/
		.name		= "efs",
		.offset		= (3902*SZ_256K),
		.size		= (50*SZ_256K), //3951
	},
	{       /* the modem firmware has to be mtd5 as the userspace samsung ril uses
	           this device hardcoded, but I placed it at the end of the NAND to be
	           able to change the other partition layout without moving it */
		.name		= "radio",
		.offset		= (3902*SZ_256K),
		.size		= (60*SZ_256K), //4011
	},
	{
		.name		= "cache",
		.offset		= (3262*SZ_256K),
		.size		= (640*SZ_256K), //3901
	},

};


/* INFORMATIONS TAKEN FROM EPIC BOOTLOADER

==== PARTITION INFORMATION ====
 ID         : IBL+PBL (0x0)
 ATTR       : RO SLC (0x1002)
 FIRST_UNIT : 0
 NO_UNITS   : 1
===============================
 ID         : PIT (0x1)
 ATTR       : RO SLC (0x1002)
 FIRST_UNIT : 1
 NO_UNITS   : 1
===============================
 ID         : EFS (0x14)
 ATTR       : RW STL SLC (0x1101)
 FIRST_UNIT : 2
 NO_UNITS   : 40
===============================
 ID         : SBL (0x3)
 ATTR       : RO SLC (0x1002)
 FIRST_UNIT : 42
 NO_UNITS   : 5
===============================
 ID         : SBL2 (0x4)
 ATTR       : RO SLC (0x1002)
 FIRST_UNIT : 47
 NO_UNITS   : 5
===============================
 ID         : PARAM (0x15)
 ATTR       : RW STL SLC (0x1101)
 FIRST_UNIT : 52
 NO_UNITS   : 20
===============================
 ID         : KERNEL (0x6)
 ATTR       : RO SLC (0x1002)
 FIRST_UNIT : 72
 NO_UNITS   : 30
===============================
 ID         : RECOVERY (0x7)
 ATTR       : RO SLC (0x1002)
 FIRST_UNIT : 102
 NO_UNITS   : 30
===============================
 ID         : FACTORYFS (0x16)
 ATTR       : RW STL SLC (0x1101)
 FIRST_UNIT : 132
 NO_UNITS   : 1074
===============================
 ID         : DATAFS (0x17)
 ATTR       : RW STL SLC (0x1101)
 FIRST_UNIT : 1206
 NO_UNITS   : 2056
===============================
 ID         : CACHE (0x18)
 ATTR       : RW STL SLC (0x1101)
 FIRST_UNIT : 3262
 NO_UNITS   : 700
===============================
 ID         : MODEM (0xb)
 ATTR       : RO SLC (0x1002)
 FIRST_UNIT : 3962
 NO_UNITS   : 50
===============================

*/