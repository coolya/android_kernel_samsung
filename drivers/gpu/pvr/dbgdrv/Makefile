
ccflags-y = -DLINUX -D__linux__ -DANDROID -DPVR_BUILD_DIR="\"omap3430_android\""
ccflags-y += -Idrivers/gpu/pvr -Idrivers/gpu/pvr/omap3430 -Idrivers/video/omap2

dbgdrv-$(CONFIG_PVR_PDUMP) := \
	dbgdriv.o \
	handle.o \
	hostfunc.o \
	hotkey.o \
	ioctl.o \
	main.o

obj-$(CONFIG_PVR_PDUMP) := dbgdrv.o
