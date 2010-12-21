#/bin/bash

case "$1" in
	clean)
		make clean
		exit
		;;
	*)
esac

	make aries_vibrant_defconfig
	export KBUILD_BUILD_VERSION="1"
	make -j`grep 'processor' /proc/cpuinfo | wc -l`
