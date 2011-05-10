#################################################
# Makefile for TVOut for S5PC100
# 2010 (C) Samsung Electronics
# Author : sangpil moon <sangpil.moon@samsung.com>
#################################################

obj-$(CONFIG_VIDEO_TV20)		+= s5p_tvout.o

s5p_tvout-y	+=	ddc.o

ifeq ($(CONFIG_HDMI_HPD), y)
s5p_tvout-y	+=	hpd.o
endif

ifeq ($(CONFIG_HDMI_CEC), y)
s5p_tvout-y	+=	cec.o \
		        s5pv210/cec_s5pv210.o
endif

ifeq ($(CONFIG_CPU_S5PC100), y)
s5p_tvout-y	+=	s5pc100/hdcp_s5pc100.o \
			s5pc100/hdmi_s5pc100.o  \
			s5pc100/sdout_s5pc100.o  \
			s5pc100/tv_power_s5pc100.o  \
			s5pc100/vmixer_s5pc100.o  \
			s5pc100/vprocessor_s5pc100.o \
			s5pc100/tv_clock_s5pc100.o
endif


ifeq ($(CONFIG_CPU_S5PV210), y)
s5p_tvout-y	+=	s5pv210/hdcp_s5pv210.o \
			s5pv210/hdmi_s5pv210.o  \
			s5pv210/sdout_s5pv210.o  \
			s5pv210/tv_power_s5pv210.o  \
			s5pv210/vmixer_s5pv210.o  \
			s5pv210/vprocessor_s5pv210.o
endif


s5p_tvout-y	+=	s5p_stda_tvout_if.o \
			s5p_stda_grp.o \
			s5p_stda_hdmi.o \
			s5p_stda_video_layer.o \
			s5p_tv_v4l2.o \
			s5p_tv_base.o

ifeq ($(CONFIG_VIDEO_TV20_DEBUG), y)
EXTRA_CFLAGS += -DDEBUG
endif
