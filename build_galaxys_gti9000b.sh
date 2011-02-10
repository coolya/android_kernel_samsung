#/bin/bash

case "$1" in
	clean)
		make clean
		exit
		;;
	*)
esac

	make aries_galaxys_gti9000b_defconfig
	export KBUILD_BUILD_VERSION="1"
	make -j`grep 'processor' /proc/cpuinfo | wc -l`
