ifeq ($(CONFIG_WIMAX_CMC7XX_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif
obj-$(CONFIG_WIMAX_CMC7XX)	+= control.o hardware.o send.o receive.o
obj-$(CONFIG_WIMAX_CMC7XX)	+= wimax_sdio.o download.o
