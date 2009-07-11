/* linux/drivers/media/video/samsung/fimc.h
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

#define FIMC_DEVICES		3
#define FIMC_SUBDEVS		3
#define FIMC_PHYBUFS		4
#define FIMC_OUTBUFS		3
#define FIMC_INQ_BUFS		3
#define FIMC_OUTQ_BUFS		3

#define FIMC_SRC_MAX_W		1920
#define FIMC_SRC_MAX_H		1080

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

enum fimc_fifo_state {
	FIFO_CLOSE,
	FIFO_SLEEP,
};

enum fimc_fimd_state {
	FIMD_OFF,
	FIMD_ON,
};


/*
 * S T R U C T U R E S
 *
*/

/* for reserved memory */
struct fimc_meminfo {
	dma_addr_t	base;	/* buffer base */
	size_t		len;	/* buffer length */
};

/* for capture device */
struct fimc_capinfo {
	struct v4l2_crop	crop;
	struct v4l2_pix_format	fmt;
	dma_addr_t		base[FIMC_PHYBUFS];

	/* flip: V4L2_CID_xFLIP, rotate: 90, 180, 270 */
	u32			flip;
	u32			rotate;
};

/* for output/overlay device */
struct fimc_buf_idx {
	int	prev;
	int	active;
	int	next;
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
	struct v4l2_pix_format	pix;
	struct v4l2_window	win;
	struct v4l2_framebuffer	fbuf;
	u32			buf_num;
	u32			is_requested;
	struct fimc_buf_idx	idx;
	struct fimc_buf_set	buf[FIMC_OUTBUFS];
	u32			in_queue[FIMC_INQ_BUFS];
	u32			out_queue[FIMC_OUTQ_BUFS];

	/* flip: V4L2_CID_xFLIP, rotate: 90, 180, 270 */
	u32			flip;
	u32			rotate;
};

struct s3cfb_window;
struct s3cfb_lcd;

struct fimc_fbinfo {
	struct fb_fix_screeninfo	*fix;
	struct fb_var_screeninfo	*var;
	struct s3cfb_window		*win;
	struct s3cfb_lcd		*lcd;

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
	struct fimc_fbinfo		*fb;		/* fimd info */
	struct fimc_capinfo		*cap;		/* capture dev info */
	struct fimc_outinfo		*out;		/* output dev info */
	enum fimc_status		status;
};

/* global */
struct fimc_global {
	struct fimc_control 	ctrl[FIMC_DEVICES];
	struct v4l2_device	v4l2_dev[FIMC_DEVICES];
	struct v4l2_subdev	*sd[FIMC_SUBDEVS];
	struct fimc_meminfo	mem[FIMC_DEVICES];
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
extern struct s3c_platform_fimc *to_fimc_plat(struct device *dev);
extern inline struct fimc_control *get_fimc(int id);
extern int fimc_mapping_rot(struct fimc_control *ctrl, int degree);
extern int fimc_check_out_buf(struct fimc_control *ctrl, u32 num);
extern int fimc_init_out_buf(struct fimc_control *ctrl);

#endif /* _FIMC_H */

