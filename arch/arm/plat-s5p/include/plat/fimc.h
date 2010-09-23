/* linux/arch/arm/plat-s5p/include/plat/fimc.h
 *
 * Copyright 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Platform header file for Samsung Camera Interface (FIMC) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __ASM_PLAT_FIMC_H
#define __ASM_PLAT_FIMC_H __FILE__ 

#include <linux/videodev2.h>

#define FIMC_SRC_MAX_W		1920
#define FIMC_SRC_MAX_H		1280

struct platform_device;

/* For exnternal camera device */
enum fimc_cam_type {
	CAM_TYPE_ITU	= 0,
	CAM_TYPE_MIPI	= 1,
};

enum fimc_cam_format {
	ITU_601_YCBCR422_8BIT	= (1 << 31),
	ITU_656_YCBCR422_8BIT	= (0 << 31),
	ITU_601_YCBCR422_16BIT	= (1 << 29),
	MIPI_CSI_YCBCR422_8BIT	= 0x1e,
	MIPI_CSI_RAW8		= 0x2a,
	MIPI_CSI_RAW10		= 0x2b,
	MIPI_CSI_RAW12		= 0x2c,
	MIPI_USER_DEF_PACKET_1	= 0x30,	/* User defined Byte-based packet 1 */
};

enum fimc_cam_order422 {
	CAM_ORDER422_8BIT_YCBYCR	= (0 << 14),
	CAM_ORDER422_8BIT_YCRYCB	= (1 << 14),
	CAM_ORDER422_8BIT_CBYCRY	= (2 << 14),
	CAM_ORDER422_8BIT_CRYCBY	= (3 << 14),
	CAM_ORDER422_16BIT_Y4CBCRCBCR	= (0 << 14),
	CAM_ORDER422_16BIT_Y4CRCBCRCB	= (1 << 14),
};

enum fimc_cam_index {
	CAMERA_PAR_A	= 0,
	CAMERA_PAR_B	= 1,
	CAMERA_CSI_C	= 2,
	CAMERA_PATTERN	= 3,	/* Not actual camera but test pattern */
	CAMERA_WB 	= 4,	/* Not actual camera but write back */
};

/* struct s3c_platform_camera: abstraction for input camera */
struct s3c_platform_camera {
	/*
	 * ITU cam A,B: 0,1
	 * CSI-2 cam C: 2
	 */
	enum fimc_cam_index		id;

	enum fimc_cam_type		type;		/* ITU or MIPI */
	enum fimc_cam_format		fmt;		/* input format */
	enum fimc_cam_order422		order422;	/* YCBCR422 order for ITU */
	u32				pixelformat;	/* default fourcc */

	int				i2c_busnum;
	struct i2c_board_info		*info;
	struct v4l2_subdev		*sd;

	const char			srclk_name[16];	/* source of mclk name */
	const char			clk_name[16];	/* mclk name */
	u32				clk_rate;	/* mclk ratio */
	struct clk			*srclk;		/* parent for mclk */
	struct clk			*clk;		/* mclk */
	int				line_length;	/* max length */
	int				width;		/* default resol */
	int				height;		/* default resol */
	struct v4l2_rect		window;		/* real cut region from source */

	int				mipi_lanes;	/* MIPI data lanes */
	int				mipi_settle;	/* MIPI settle */
	int				mipi_align;	/* MIPI data align: 24/32 */

	/* Polarity: 1 if inverted polarity used */
	int				inv_pclk;
	int				inv_vsync;
	int				inv_href;
	int				inv_hsync;

	int				initialized;

	/* Board specific power pin control */
	int				(*cam_power)(int onoff);
};

/* For camera interface driver */
struct s3c_platform_fimc {
	const char			srclk_name[16];		/* source of interface clock name */
	const char			clk_name[16];		/* interface clock name */
	const char			lclk_name[16];		/* interface clock name */
	u32				clk_rate;		/* clockrate for interface clock */
	enum fimc_cam_index		default_cam;		/* index of default cam */
	struct s3c_platform_camera	*camera[5];		/* FIXME */
	int				hw_ver;
	phys_addr_t			pmem_start;		/* starting physical address of memory region */
	size_t				pmem_size;		/* size of memory region */

	void				(*cfg_gpio)(struct platform_device *pdev);
	int				(*clk_on)(struct platform_device *pdev, struct clk *clk);
	int				(*clk_off)(struct platform_device *pdev, struct clk *clk);
};

extern void s3c_fimc0_set_platdata(struct s3c_platform_fimc *fimc);
extern void s3c_fimc1_set_platdata(struct s3c_platform_fimc *fimc);
extern void s3c_fimc2_set_platdata(struct s3c_platform_fimc *fimc);

/* defined by architecture to configure gpio */
extern void s3c_fimc0_cfg_gpio(struct platform_device *pdev);
extern void s3c_fimc1_cfg_gpio(struct platform_device *pdev);
extern void s3c_fimc2_cfg_gpio(struct platform_device *pdev);

/* platform specific clock functions */
extern int s3c_fimc_clk_on(struct platform_device *pdev, struct clk *clk);
extern int s3c_fimc_clk_off(struct platform_device *pdev, struct clk *clk);

#endif /*__ASM_PLAT_FIMC_H */
