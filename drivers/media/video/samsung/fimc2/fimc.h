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
#include <media/videobuf-core.h>
#include <plat/media.h>
#include <plat/fimc.h>
#endif

#define FIMC_NAME		"s3c-fimc"

#define FIMC_DEVICES		3
#define FIMC_SUBDEVS		3
#define FIMC_MAXCAMS		4
#define FIMC_PHYBUFS		4
#define FIMC_OUTBUFS		3
#define FIMC_INQ_BUFS		3
#define FIMC_OUTQ_BUFS		3
#define FIMC_TPID		3

#define FIMC_SRC_MAX_W		1920
#define FIMC_SRC_MAX_H		1080

/*
 * V 4 L 2   F I M C   E X T E N S I O N S
 *
*/

/* CID extensions */
#define V4L2_CID_ROTATION		(V4L2_CID_PRIVATE_BASE + 0)

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

enum fimc_rot_flip {
	FIMC_0_NFLIP	= 0x00,
	FIMC_0_XFLIP	= 0x01,
	FIMC_0_YFLIP	= 0x02,
	FIMC_0_XYFLIP	= 0x03,
	FIMC_90_NFLIP	= 0x10,
	FIMC_90_XFLIP	= 0x11,
	FIMC_90_YFLIP	= 0x12,
	FIMC_90_XYFLIP	= 0x13,
	FIMC_180_NFLIP	= 0x03,
	FIMC_180_XFLIP	= 0x02,
	FIMC_180_YFLIP	= 0x01,
	FIMC_180_XYFLIP	= 0x00,
	FIMC_270_NFLIP	= 0x13,
	FIMC_270_XFLIP	= 0x12,
	FIMC_270_YFLIP	= 0x11,
	FIMC_270_XYFLIP	= 0x10,
};

enum fimc_input {
	FIMC_SRC_CAM,
	FIMC_SRC_MSDMA,
};

enum fimc_output {
	FIMC_DST_DMA,
	FIMC_DST_FIMD,
};

enum fimc_autoload {
	FIMC_AUTO_LOAD,
	FIMC_ONE_SHOT,
};

/*
 * S T R U C T U R E S
 *
*/

/* for reserved memory */
struct fimc_meminfo {
	dma_addr_t	base;		/* buffer base */
	size_t		len;		/* total length */
	dma_addr_t	curr;		/* current addr */
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

/* To do : remove s3cfb_window, s3cfb_lcd structures ---------------------- */
struct s3cfb_window {
	int			id;
	int			enabled;
	atomic_t		in_use;
	int			x;
	int			y;
	int			local_channel;
	int			dma_burst;
	unsigned int		pseudo_pal[16];
	int			(*suspend_fifo)(void);
	int			(*resume_fifo)(void);
};

struct s3cfb_lcd {
	int	width;
	int	height;
	int	bpp;
	int	freq;

	void 	(*init_ldi)(void);
};
/* ------------------------------------------------------------------------ */

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
	struct fimc_meminfo		mem;		/* for reserved mem */

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
	struct v4l2_cropcap		cropcap;

	/* fimc specific */
	struct s3c_platform_camera	*cam;		/* activated camera */
	struct fimc_capinfo		*cap;		/* capture dev info */
	struct fimc_outinfo		*out;		/* output dev info */
	struct fimc_fbinfo		fb;		/* fimd info */

	enum fimc_status		status;
};

/* global */
struct fimc_global {
	struct fimc_control 		ctrl[FIMC_DEVICES];
	struct s3c_platform_camera	*camera[FIMC_MAXCAMS];
	struct v4l2_device		v4l2_dev[FIMC_DEVICES];
	struct v4l2_subdev		*sd[FIMC_SUBDEVS];
	struct clk			*mclk;
	int				initialized;
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
extern struct video_device fimc_video_device[];
extern struct s3c_platform_fimc *to_fimc_plat(struct device *dev);
extern inline struct fimc_control *get_fimc(int id);

/* camera */
extern void fimc_select_camera(struct fimc_control *ctrl);

/* capture device */
extern int fimc_reqbufs_capture(void *fh, struct v4l2_requestbuffers *b);
extern int fimc_querybuf_capture(void *fh, struct v4l2_buffer *b);
extern int fimc_g_ctrl_capture(void *fh, struct v4l2_control *c);
extern int fimc_s_ctrl_capture(void *fh, struct v4l2_control *c);
extern int fimc_cropcap_capture(void *fh, struct v4l2_cropcap *a);
extern int fimc_s_crop_capture(void *fh, struct v4l2_crop *a);
extern int fimc_streamon_capture(void *fh);
extern int fimc_streamoff_capture(void *fh);
extern int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b);
extern int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b);

/* output device */
extern int fimc_reqbufs_output(void *fh, struct v4l2_requestbuffers *b);
extern int fimc_querybuf_output(void *fh, struct v4l2_buffer *b);
extern int fimc_g_ctrl_output(void *fh, struct v4l2_control *c);
extern int fimc_s_ctrl_output(void *fh, struct v4l2_control *c);
extern int fimc_cropcap_output(void *fh, struct v4l2_cropcap *a);
extern int fimc_s_crop_output(void *fh, struct v4l2_crop *a);
extern int fimc_streamon_output(void *fh);
extern int fimc_streamoff_output(void *fh);
extern int fimc_qbuf_output(void *fh, struct v4l2_buffer *b);
extern int fimc_dqbuf_output(void *fh, struct v4l2_buffer *b);
extern int fimc_g_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f);
extern int fimc_s_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f);
extern int fimc_try_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f);
extern int fimc_g_fbuf(struct file *filp, void *fh, struct v4l2_framebuffer *fb);
extern int fimc_s_fbuf(struct file *filp, void *fh, struct v4l2_framebuffer *fb);

/* overlay device */
extern int fimc_try_fmt_overlay(struct file *filp, void *fh, struct v4l2_format *f);
extern int fimc_g_fmt_vid_overlay(struct file *file, void *fh, struct v4l2_format *f);
extern int fimc_s_fmt_vid_overlay(struct file *file, void *fh, struct v4l2_format *f);

/* Configuration */
extern int fimc_mapping_rot(struct fimc_control *ctrl, int degree);
extern int fimc_check_out_buf(struct fimc_control *ctrl, u32 num);
extern int fimc_init_out_buf(struct fimc_control *ctrl);
extern int fimc_check_param(struct fimc_control *ctrl);
extern int fimc_set_param(struct fimc_control *ctrl);

extern int fimc_attach_in_queue(struct fimc_control *ctrl, u32 index);
extern int fimc_detach_in_queue(struct fimc_control *ctrl, int *index);
extern int fimc_attach_out_queue(struct fimc_control *ctrl, u32 index);
extern int fimc_detach_out_queue(struct fimc_control *ctrl, int *index);

/* Register access file */
extern void fimc_clear_irq(struct fimc_control *ctrl);
extern void fimc_set_int_enable(struct fimc_control *ctrl, u32 enable);
extern void fimc_reset(struct fimc_control *ctrl);
extern int fimc_set_format(struct fimc_control *ctrl);
extern int fimc_set_rot(struct fimc_control *ctrl);
extern int fimc_set_path(struct fimc_control *ctrl);
extern int fimc_set_src_addr(struct fimc_control *ctrl);
extern int fimc_set_dst_addr(struct fimc_control *ctrl);
extern int fimc_set_src_crop(struct fimc_control *ctrl);
extern int fimc_set_dst_crop(struct fimc_control *ctrl);
extern int fimc_set_scaler(struct fimc_control *ctrl);
extern int fimc_start_scaler(struct fimc_control *ctrl);
extern int fimc_stop_scaler(struct fimc_control *ctrl);

#endif /* _FIMC_H */

