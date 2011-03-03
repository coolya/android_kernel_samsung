#/bin/bash

START=$(date +%s)

DEVICE="$1"

case "$DEVICE" in
	clean)
		make clean
		exit
		;;
	captivate)
		cfg=aries_captivate_defconfig
		;;
	galaxys)
		cfg=aries_galaxys_defconfig
		;;
	galaxysb)
		cfg=aries_galaxysb_defconfig
		;;
	vibrant)
		cfg=aries_vibrant_defconfig
		;;
	*)
		echo "Usage: $0 DEVICE"
		echo "Example: ./build.sh galaxys"
		echo "Supported Devices: captivate, galaxys, galaxysb, vibrant"
		exit 2
		;;
esac

export KBUILD_BUILD_VERSION="1"
echo "Using config ${cfg}"

make ${cfg}  || { echo "Failed to make config"; exit 1; }
make -j $(grep 'processor' /proc/cpuinfo | wc -l) || { echo "Failed to make kernel"; exit 1; }

echo -n "Copying Kernel and Modules to device tree..."
{
cp arch/arm/boot/zImage ../../../device/samsung/$DEVICE/zImage
cp drivers/net/tun.ko ../../../device/samsung/$DEVICE/tun.ko
cp drivers/net/wireless/bcm4329/bcm4329.ko ../../../device/samsung/$DEVICE/bcm4329.ko
cp fs/cifs/cifs.ko ../../../device/samsung/$DEVICE/cifs.ko
} || { echo "failed to copy kernel and modules"; exit 1; }
echo "done."

END=$(date +%s)
ELAPSED=$((END - START))
E_MIN=$((ELAPSED / 60))
E_SEC=$((ELAPSED - E_MIN * 60))
printf "Elapsed: "
[ $E_MIN != 0 ] && printf "%d min(s) " $E_MIN
printf "%d sec(s)\n" $E_SEC
