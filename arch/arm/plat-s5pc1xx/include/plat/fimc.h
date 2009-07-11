/* linux/arch/arm/plat-s5pc1xx/include/plat/fimc.h
 *
 * Platform header file for Samsung Camera Interface (FIMC) driver
 *
 * Dongsoo Kim, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/sec/
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FIMC_H
#define _FIMC_H

struct platform_device;

/* For exnternal camera device */
/*
 * struct s3c_fimc_window_offset
 * @h1:	left side offset of source
 * @h2: right side offset of source
 * @v1: upper side offset of source
 * @v2: lower side offset of source
*/
/* FIXME: migrate to v4l2 window type */
struct s3c_fimc_window_offset {
	int	h1;
	int	h2;
	int	v1;
	int	v2;
};

/*
 * struct s3c_fimc_polarity
 * @pclk:	1 if PCLK polarity is inverse
 * @vsync:	1 if VSYNC polarity is inverse
 * @href:	1 if HREF polarity is inverse
 * @hsync:	1 if HSYNC polarity is inverse
*/
struct s3c_fimc_polarity {
	u32	pclk;
	u32	vsync;
	u32	href;
	u32	hsync;
};

/*
 * E N U M E R A T I O N S
 *
*/
enum s3c_fimc_cam_t {
	CAM_TYPE_ITU	= 0,
	CAM_TYPE_MIPI	= 1,
};

enum s3c_fimc_cam_mode_t {
	ITU_601_YCBCR422_8BIT = (1 << 31),
	ITU_656_YCBCR422_8BIT = (0 << 31),
	ITU_601_YCBCR422_16BIT = (1 << 29),
	MIPI_CSI_YCBCR422_8BIT = 0x1e,
	MIPI_CSI_RAW8 = 0x2a,
	MIPI_CSI_RAW10 = 0x2b,
	MIPI_CSI_RAW12 = 0x2c,
};

enum s3c_fimc_order422_cam_t {
	CAM_ORDER422_8BIT_YCBYCR = (0 << 14),
	CAM_ORDER422_8BIT_YCRYCB = (1 << 14),
	CAM_ORDER422_8BIT_CBYCRY = (2 << 14),
	CAM_ORDER422_8BIT_CRYCBY = (3 << 14),
	CAM_ORDER422_16BIT_Y4CBCRCBCR = (0 << 14),
	CAM_ORDER422_16BIT_Y4CRCBCRCB = (1 << 14),
};

enum s3c_fimc_cam_index {
	CAMERA_PAR_A	= 0,
	CAMERA_PAR_B	= 1,
	CAMERA_CSI_C	= 2,
	CAMERA_PATTERN	= 3,	/* Not actual camera but test pattern */
};

/*
 * struct s3c_platform_cam: abstraction for input camera
 * @id:			cam id (0-2)
 * @type:		type of camera (ITU or MIPI)
 * @mode:		mode of input source
 * @order422:		YCBCR422 order
 * @clockrate:		camera clockrate
 * @width:		source width
 * @height:		source height
 * @offset:		offset information
 * @polarity		polarity information
 * @reset_type:		reset type (high or low)
 * @reset_delay:	delay time in microseconds (udelay)
 * @client:		i2c client
 * @initialized:	whether already initialized
*/
struct s3c_fimc_camera {
	/*
	 * ITU cam A,B: 0,1
	 * CSI-2 cam C: 2
	 */
	enum s3c_fimc_cam_index		id;

	enum s3c_fimc_cam_t		type;
	u32				pixelformat;
	enum s3c_fimc_cam_mode_t	mode;
	enum s3c_fimc_order422_cam_t	order422;

	int				i2c_busnum;
	struct i2c_board_info		*info;
	struct v4l2_subdev		*sd;

	u32				clockrate;	/* sclk_cam */
	int				line_length;	/* max length */
	int				width;	/* default resol */
	int				height;	/* default resol */
#if 1
	struct s3c_fimc_window_offset	offset;	/* FIXME: scaler */
#else
	struct v4l2_rect		rect;
#endif

	/* Polarity */
	struct s3c_fimc_polarity	polarity;

	int				initialized;

	/* Board specific power pin control */
	int		(*cam_power)(int onoff);
};

/* For camera interface driver */
struct s3c_platform_fimc {
	const char	srclk_name[16];
	const char	clk_name[16];
	u32		clockrate;	/* sclk_fimc */
	
	int		nr_frames;
	int		shared_io;

	enum s3c_fimc_cam_index	default_cam;	/* index of default cam */
	struct s3c_fimc_camera	*camera[4];	/* FIXME */

	void		(*cfg_gpio)(struct platform_device *dev);
};

extern void s3c_fimc0_set_platdata(struct s3c_platform_fimc *fimc);
extern void s3c_fimc1_set_platdata(struct s3c_platform_fimc *fimc);
extern void s3c_fimc2_set_platdata(struct s3c_platform_fimc *fimc);

/* defined by architecture to configure gpio */
extern void s3c_fimc0_cfg_gpio(struct platform_device *dev);
extern void s3c_fimc1_cfg_gpio(struct platform_device *dev);
extern void s3c_fimc2_cfg_gpio(struct platform_device *dev);

extern void s3c_fimc_reset_camera(void);

#endif /* _FIMC_H */

