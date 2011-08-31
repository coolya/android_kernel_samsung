# Script written by Evan alias ytt3r to build TalonMTD more easily.
# Parse command line options
#/bin/bash

USAGE="Usage: Type './buildscript.sh captivatemtd' to build TalonMTD for captivate, build other devices in this fashion."
case $1 in
	captivatemtd)
		make aries_captivatemtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.3"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		initrd_files/mkshbootimg.py releasetools/boot.img arch/arm/boot/zImage initrd_files/ramdisk.img initrd_files/ramdisk-recovery.img
		cp drivers/net/tun.ko releasetools/system/lib/modules/tun.ko
		cp drivers/net/wireless/bcm4329/bcm4329.ko releasetools/system/lib/modules/bcm4329.ko
		cp fs/cifs/cifs.ko releasetools/system/lib/modules/cifs.ko
		cp drivers/staging/android/logger.ko releasetools/system/lib/modules/logger.ko
		cp crypto/ansi_cprng.ko releasetools/system/lib/modules/ansi_cprng.ko
		cp drivers/cpufreq/cpufreq_conservative.ko releasetools/system/lib/modules/cpufreq_conservative.ko
		cp drivers/scsi/scsi_wait_scan.ko releasetools/system/lib/modules/scsi_wait_scan.ko
		cp fs/fuse/fuse.ko releasetools/system/lib/modules/fuse.ko
		cd releasetools
		rm -f *.zip
		zip -r TalonCaptivate *
		cd ..
		echo "Finished."
		;;
	galaxysmtd)
		make aries_galaxysmtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.3"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		initrd_files/mkshbootimg.py releasetools/boot.img arch/arm/boot/zImage initrd_files/ramdisk.img initrd_files/ramdisk-recovery.img
		cp drivers/net/tun.ko releasetools/system/lib/modules/tun.ko
		cp drivers/net/wireless/bcm4329/bcm4329.ko releasetools/system/lib/modules/bcm4329.ko
		cp fs/cifs/cifs.ko releasetools/system/lib/modules/cifs.ko
		cp drivers/staging/android/logger.ko releasetools/system/lib/modules/logger.ko
		cp crypto/ansi_cprng.ko releasetools/system/lib/modules/ansi_cprng.ko
		cp drivers/cpufreq/cpufreq_conservative.ko releasetools/system/lib/modules/cpufreq_conservative.ko
		cp drivers/scsi/scsi_wait_scan.ko releasetools/system/lib/modules/scsi_wait_scan.ko
		cp fs/fuse/fuse.ko releasetools/system/lib/modules/fuse.ko
		cd releasetools
		rm -f *.zip
		zip -r TalonGalaxyS *
		cd ..
		echo "Finished."
		;;
	vibrantmtd)
		make aries_vibrantmtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.3"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		initrd_files/mkshbootimg.py releasetools/boot.img arch/arm/boot/zImage initrd_files/ramdisk.img initrd_files/ramdisk-recovery.img
		cp drivers/net/tun.ko releasetools/system/lib/modules/tun.ko
		cp drivers/net/wireless/bcm4329/bcm4329.ko releasetools/system/lib/modules/bcm4329.ko
		cp fs/cifs/cifs.ko releasetools/system/lib/modules/cifs.ko
		cp drivers/staging/android/logger.ko releasetools/system/lib/modules/logger.ko
		cp crypto/ansi_cprng.ko releasetools/system/lib/modules/ansi_cprng.ko
		cp drivers/cpufreq/cpufreq_conservative.ko releasetools/system/lib/modules/cpufreq_conservative.ko
		cp drivers/scsi/scsi_wait_scan.ko releasetools/system/lib/modules/scsi_wait_scan.ko
		cp fs/fuse/fuse.ko releasetools/system/lib/modules/fuse.ko
		cd releasetools
		rm -f *.zip
		zip -r TalonVibrant *
		cd ..
		echo "Finished."
		;;
	galaxysbmtd)
		make aries_galaxysbmtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.3"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		initrd_files/mkshbootimg.py releasetools/boot.img arch/arm/boot/zImage initrd_files/ramdisk.img initrd_files/ramdisk-recovery.img
		cp drivers/net/tun.ko releasetools/system/lib/modules/tun.ko
		cp drivers/net/wireless/bcm4329/bcm4329.ko releasetools/system/lib/modules/bcm4329.ko
		cp fs/cifs/cifs.ko releasetools/system/lib/modules/cifs.ko
		cp drivers/staging/android/logger.ko releasetools/system/lib/modules/logger.ko
		cp crypto/ansi_cprng.ko releasetools/system/lib/modules/ansi_cprng.ko
		cp drivers/cpufreq/cpufreq_conservative.ko releasetools/system/lib/modules/cpufreq_conservative.ko
		cp drivers/scsi/scsi_wait_scan.ko releasetools/system/lib/modules/scsi_wait_scan.ko
		cp fs/fuse/fuse.ko releasetools/system/lib/modules/fuse.ko
		cd releasetools
		rm -f *.zip
		zip -r TalonGalaxySB *
		cd ..
		echo "Finished."
		;;
	fascinatemtd)
		make aries_fascinatemtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.3"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		initrd_files/mkshbootimg.py releasetools/boot.img arch/arm/boot/zImage initrd_files/ramdisk.img initrd_files/ramdisk-recovery.img
		cp drivers/net/tun.ko releasetools/system/lib/modules/tun.ko
		cp drivers/net/wireless/bcm4329/bcm4329.ko releasetools/system/lib/modules/bcm4329.ko
		cp fs/cifs/cifs.ko releasetools/system/lib/modules/cifs.ko
		cp drivers/staging/android/logger.ko releasetools/system/lib/modules/logger.ko
		cp crypto/ansi_cprng.ko releasetools/system/lib/modules/ansi_cprng.ko
		cp drivers/cpufreq/cpufreq_conservative.ko releasetools/system/lib/modules/cpufreq_conservative.ko
		cp drivers/scsi/scsi_wait_scan.ko releasetools/system/lib/modules/scsi_wait_scan.ko
		cp fs/fuse/fuse.ko releasetools/system/lib/modules/fuse.ko
		cd releasetools
		rm -f *.zip
		zip -r TalonFascinate *
		cd ..
		echo "Finished."
		;;
	*)
		echo "You didn't provide an argument, or provided an unsupported argument. Keep in mind that arguments are case-sensitive. Please type the name of your device after the command, e.g. './buildscript.sh captivatemtd'."
	esac
