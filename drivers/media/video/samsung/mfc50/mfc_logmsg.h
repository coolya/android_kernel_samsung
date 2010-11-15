/*
 * drivers/media/video/samsung/mfc50/mfc_logmsg.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MFC_LOGMSG_H_
#define _MFC_LOGMSG_H_

/* debug macros */
#define MFC_DEBUG(fmt, ...)                         \
	do {                                            \
		printk(KERN_DEBUG                           \
			"%s: " fmt, __func__, ##__VA_ARGS__);   \
	} while (0)

#define MFC_ERROR(fmt, ...)                         \
	do {                                            \
		printk(KERN_ERR                             \
			"%s: " fmt, __func__, ##__VA_ARGS__);   \
	} while (0)

#define MFC_NOTICE(fmt, ...)                        \
	do {                                            \
		printk(KERN_NOTICE                          \
			"%s: " fmt, __func__, ##__VA_ARGS__);   \
	} while (0)

#define MFC_INFO(fmt, ...)                          \
	do {                                            \
		printk(KERN_INFO                            \
			"%s: " fmt, __func__, ##__VA_ARGS__);   \
	} while (0)

#define MFC_WARN(fmt, ...)                          \
	do {                                            \
		printk(KERN_WARNING                         \
			"%s: " fmt, __func__, ##__VA_ARGS__);   \
	} while (0)


#ifdef CONFIG_VIDEO_MFC50_DEBUG
#define mfc_debug(fmt, ...)     MFC_INFO(fmt, ##__VA_ARGS__)
#define mfc_debug_L0(fmt, ...)     MFC_INFO(fmt, ##__VA_ARGS__)
#else
#define mfc_debug(fmt, ...)
#define mfc_debug_L0(fmt, ...)
#endif

#if	defined(DEBUG_LEVEL_0)
#define mfc_debug_L0(fmt, ...)     MFC_INFO(fmt, ##__VA_ARGS__)
#define mfc_debug(fmt, ...)     MFC_INFO(fmt, ##__VA_ARGS__)
#elseif define(DEBUG_LEVEL_1)
#define mfc_debug(fmt, ...)     MFC_INFO(fmt, ##__VA_ARGS__)
#endif

#define mfc_err(fmt, ...)       MFC_ERROR(fmt, ##__VA_ARGS__)
#define mfc_notice(fmt, ...)    MFC_NOTICE(fmt, ##__VA_ARGS__)
#define mfc_info(fmt, ...)      MFC_INFO(fmt, ##__VA_ARGS__)
#define mfc_warn(fmt, ...)      MFC_WARN(fmt, ##__VA_ARGS__)

#endif /* _MFC_LOGMSG_H_ */
