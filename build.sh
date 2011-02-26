#/bin/bash

START=$(date +%m)

DEVICE=$1

case "$DEVICE" in
	clean)
		make clean
		exit
		;;
	captivate)
		echo -e "\n";
		echo "Building Kernel for Samsung Captivate"
		echo -e "\n";
		make aries_captivate_defconfig
		;;
	galaxys)
		echo -e "\n";
		echo "Building Kernel for Samsung Galaxy S"
		echo -e "\n";
		make aries_galaxys_defconfig
		;;
	galaxysb)
		echo -e "\n";
		echo "Building Kernel for Samsung Galaxy S (b)"
		echo -e "\n";
		make aries_galaxys_gti9000b_defconfig
		DEVICE=galaxys
		;;
	vibrant)
		echo -e "\n";
		echo "Building Kernel for Samsung Vibrant"
		echo -e "\n";
		make aries_vibrant_defconfig
		;;
	*)
		echo -e "\n";
		echo "Usage: ./build.sh DEVICE"
		echo "Example: ./build.sh galaxys"
		echo "Supported Devices: captivate, galaxys, galaxysb, vibrant"
		echo -e "\n";
		exit
		;;
esac

export KBUILD_BUILD_VERSION="1"
make -j`grep 'processor' /proc/cpuinfo | wc -l`

echo -e "\n";
echo "Copying Kernel and Modules to device tree..."
cp arch/arm/boot/zImage ../../../device/samsung/$DEVICE/zImage
cp drivers/net/tun.ko ../../../device/samsung/$DEVICE/tun.ko
cp drivers/net/wireless/bcm4329/bcm4329.ko ../../../device/samsung/$DEVICE/bcm4329.ko
cp fs/cifs/cifs.ko ../../../device/samsung/$DEVICE/cifs.ko
echo "Done."

echo -e "\n";
END=$(date +%m)
echo "Elapsed: $((END - START)) minutes."
echo -e "\n";
