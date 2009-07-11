/* linux/drivers/media/video/samsung/s3c_fimc.h
 *
 * Header file for Samsung Camera Interface (FIMC) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _FIMC_H
#define _FIMC_H

#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/fb.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <plat/media.h>
#include <plat/fimc.h>
#endif

#define FIMC_NAME		"s3c-fimc"

#define FIMC_OUT_BUFF_NUM	3

#define FIMC0_RESERVED_MEM_ADDR_PHY		(u32) s3c_get_media_memory(S3C_MDEV_FIMC0)
#define FIMC0_RESERVED_MEM_SIZE			(u32) s3c_get_media_memsize(S3C_MDEV_FIMC0)

#define FIMC1_RESERVED_MEM_ADDR_PHY		(u32) s3c_get_media_memory(S3C_MDEV_FIMC1)
#define FIMC1_RESERVED_MEM_SIZE			(u32) s3c_get_media_memsize(S3C_MDEV_FIMC1)

#define FIMC2_RESERVED_MEM_ADDR_PHY		(u32) s3c_get_media_memory(S3C_MDEV_FIMC2)
#define FIMC2_RESERVED_MEM_SIZE			(u32) s3c_get_media_memsize(S3C_MDEV_FIMC2)

#define FIMC_YUV_SRC_MAX_WIDTH			1920
#define FIMC_YUV_SRC_MAX_HEIGHT			1080

/* debug macro */
#define FIMC_LOG_DEFAULT       			FIMC_LOG_WARN

#define FIMC_DEBUG(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_DEBUG) \
			printk(KERN_DEBUG FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while(0)

#define FIMC_INFO(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_INFO) \
			printk(KERN_INFO FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while (0)

#define FIMC_WARN(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_WARN) \
			printk(KERN_WARNING FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while (0)


#define FIMC_ERROR(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_ERR) \
			printk(KERN_ERR FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while (0)

#define fimc_dbg(level, fmt, ...)	FIMC_DEBUG(level, fmt, ##__VA_ARGS__)
#define fimc_info(level, fmt, ...)	FIMC_INFO(level, fmt, ##__VA_ARGS__)
#define fimc_warn(level, fmt, ...)	FIMC_WARN(level, fmt, ##__VA_ARGS__)
#define fimc_err(level, fmt, ...)	FIMC_ERROR(level, fmt, ##__VA_ARGS__)

/*
 * E N U M E R A T I O N S
 *
*/
enum fimc_status {
	FIMC_STREAMOFF,
	FIMC_READY_ON,
	FIMC_STREAMON,
	FIMC_READY_OFF,
	FIMC_ON_SLEEP,
	FIMC_OFF_SLEEP,
	FIMC_READY_RESUME,
};

enum fimc_log_level {
	FIMC_LOG_DEBUG	= 0x1,	/* Queue status */
	FIMC_LOG_INFO	= 0x3,	/* V4L2 API */
	FIMC_LOG_WARN	= 0x7,	/* Warning conditions */
	FIMC_LOG_ERR	= 0xF,	/* Error conditions */
};


enum fimc_fifo_state {
	FIFO_CLOSE,
	FIFO_SLEEP,
};

enum fimc_fimd_state {
	FIMD_OFF,
	FIMD_ON,
};

enum fimc_flip_t {
	FLIP_ORIGINAL	= 0x0,
	FLIP_X_AXIS	= 0x1,
	FLIP_Y_AXIS	= 0x2,
	FLIP_XY_AXIS	= 0x3,
};

/*
 * S T R U C T U R E S
 *
*/

/* for reserved memory */
struct fimc_meminfo {
	dma_addr_t	cap_base;	/* capture device buffer base */
	size_t		cap_len;	/* capture device buffer length */
	dma_addr_t	out_base;	/* output device buffer base */
	size_t		out_len;	/* output device buffer length */
};

/* for capture device */
struct fimc_capinfo {
	struct v4l2_pix_format	fmt;
	dma_addr_t		base[4];	/* phys. addr of pingpong */
	struct v4l2_crop	crop;
	enum fimc_flip_t	flip;
	int			rotate;		/* rotation: 90, 180, 270 */
};

/* for output/overlay device */
struct fimc_buf_idx {
	int			index;
	struct fimc_buf_idx	*next;
};

struct fimc_buf_set {
	dma_addr_t		base;
	size_t			length;
	enum videobuf_state	state;
	u32			flags;
	atomic_t		mapped_cnt;
};

struct fimc_outinfo {
	struct v4l2_crop 	crop;
	struct v4l2_window	win;
	struct v4l2_framebuffer	fbuf;
	u32			buf_num;
	u32			is_requested;
	struct fimc_buf_idx	idx;
	struct fimc_buf_set	buf[3];
	u32			in_queue[3];
	u32			out_queue[3];
	enum fimc_flip_t	flip;
	int			rotate;
};

struct s3cfb_window;

struct fimc_fbinfo {
	struct fb_fix_screeninfo	*fix;
	struct fb_var_screeninfo	*var;
	struct s3cfb_window		*win;

	/* lcd fifo control */
	int (*open_fifo)(int id, int ch, int (*do_priv)(void *), void *param);
	int (*close_fifo)(int id, int (*do_priv)(void *), void *param, int sleep);
};

/* fimc controller abstration */
struct fimc_control {
	int				id;		/* controller id */
	char				name[16];
	atomic_t			in_use;
	void __iomem			*regs;		/* register i/o */
	struct clk			*clock;		/* interface clock */

	/* kernel helpers */
	struct mutex			lock;		/* controller lock */
	struct mutex			v4l2_lock;
	spinlock_t			lock_in;
	spinlock_t			lock_out;
	wait_queue_head_t		wq;
	struct device			*dev;

	/* v4l2 related */
	struct video_device		*vd;
	struct v4l2_device		v4l2_dev;
	struct v4l2_subdev		sd;
	struct v4l2_cropcap		cropcap;

	/* fimc specific */
	struct s3c_platform_camera	*cam;		/* activated camera */
	struct fimc_fbinfo		*fb[5];		/* fimd info */
	struct fimc_capinfo		*cap;		/* capture dev info */
	struct fimc_outinfo		*out;		/* output dev info */
	enum fimc_status		status;
	enum fimc_log_level		log;
};

/* global */
struct fimc_global {
	struct fimc_control 	ctrl[3];
	struct v4l2_device	v4l2_dev[3];
	struct v4l2_subdev	*sd[3];
	struct fimc_meminfo	*mem;
};

/* scaler abstraction: local use recommended */
struct fimc_scaler {
	u32 bypass;
	u32 hfactor;
	u32 vfactor;
	u32 pre_hratio;
	u32 pre_vratio;
	u32 pre_dst_width;
	u32 pre_dst_height;
	u32 scaleup_h;
	u32 scaleup_v;
	u32 main_hratio;
	u32 main_vratio;
	u32 real_width;
	u32 real_height;
};

/*
 * E X T E R N S
 *
*/
extern int fimc_mapping_rot(struct fimc_control *ctrl, int degree);
extern int fimc_check_out_buf(struct fimc_control *ctrl, u32 num);
extern int fimc_init_out_buf(struct fimc_control *ctrl);

#endif /* _FIMC2_H */

