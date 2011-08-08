# Script written by Evan alias ytt3r to build TalonMTD more easily.
# Parse command line options
#/bin/bash

USAGE="Usage: Type './buildscript.sh captivate' to build TalonMTD for captivate, build other devices in this fashion."
case $1 in
	captivate)
		make aries_captivatemtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.1"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		cd releasetools
		cp arch/arm/boot/zImage releasetools
		cd releasetools
		rm -f *.zip
		zip -r TalonCaptivate *
		cd ..
		echo "Finished."
		;;
	galaxys)
		make aries_galaxysmtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.1"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		cd releasetools
		cp arch/arm/boot/zImage releasetools
		cd releasetools
		rm -f *.zip
		zip -r TalonGalaxyS *
		cd ..
		echo "Finished."
		;;
	vibrant)
		make aries_vibrantmtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.1"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		cd releasetools
		cp arch/arm/boot/zImage releasetools
		cd releasetools
		rm -f *.zip
		zip -r TalonVibrant *
		cd ..
		echo "Finished."
		;;
	galaxysb)
		make aries_galaxysbmtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.1"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		cd releasetools
		cp arch/arm/boot/zImage releasetools
		cd releasetools
		rm -f *.zip
		zip -r TalonGalaxySB *
		cd ..
		echo "Finished."
		;;
	fascinate)
		make aries_fascinatemtd_defconfig
		export KBUILD_BUILD_VERSION="TalonMTD 0.0.1"
		make -j`grep 'processor' /proc/cpuinfo | wc -l`
		cd releasetools
		cp arch/arm/boot/zImage releasetools
		cd releasetools
		rm -f *.zip
		zip -r TalonFascinate *
		cd ..
		echo "Finished."
		;;
	*)
		echo "You didn't provide an argument, or provided an unsupported argument. Keep in mind that arguments are case-sensitive. Please type the name of your device after the command, e.g. './buildscript.sh captivate'."
	esac
