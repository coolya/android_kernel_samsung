# arch/arm/plat-s5p/Makefile
#
# Copyright (c) 2009 Samsung Electronics Co., Ltd.
# 		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:= dummy.o
obj-				:=

# Core files

obj-y				+= dev-uart.o
obj-y				+= cpu.o
obj-y				+= clock.o
obj-y				+= irq.o
obj-y				+= devs.o
obj-y				+= bootmem.o
obj-$(CONFIG_S5P_EXT_INT)	+= irq-eint.o irq-eint-group.o

# PM support

# PM support

obj-$(CONFIG_PM)		+= pm.o
obj-$(CONFIG_PM)		+= irq-pm.o

ifndef CONFIG_S5P_HIGH_RES_TIMERS
obj-$(CONFIG_SYSTIMER_S5P)	+= systimer-s5p.o
else
ifdef CONFIG_HRT_RTC
obj-y				+= hr-time-rtc.o
endif
endif

obj-$(CONFIG_S5P_DEV_MFC)	+= dev-mfc.o
obj-$(CONFIG_S5P_SETUP_MFC)	+= setup-mfc.o
