/* linux/drivers/media/video/samsung/fimc_capture.c
 *
 * V4L2 Capture device support file for Samsung Camera Interface (FIMC) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <plat/media.h>
#include <plat/fimc2.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "fimc.h"

/* subdev handling macro */
#define subdev_call(ctrl, o, f, args...) \
	v4l2_subdev_call(ctrl->cam->sd, o, f, ##args)

const static struct v4l2_fmtdesc capture_fmts[] = {
	{
		.index		= 0,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-5-6-5",
		.pixelformat	= V4L2_PIX_FMT_RGB565,		
	}, {
		.index		= 1,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-8-8-8",
		.pixelformat	= V4L2_PIX_FMT_RGB24,		
	}, {
		.index		= 2,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,		
	}, {
		.index		= 3,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, CbYCrY",
		.pixelformat	= V4L2_PIX_FMT_UYVY,		
	}, {
		.index		= 4,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, CrYCbY",
		.pixelformat	= V4L2_PIX_FMT_VYUY,		
	}, {
		.index		= 5,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, YCrYCb",
		.pixelformat	= V4L2_PIX_FMT_YVYU,		
	}, {
		.index		= 6,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,		
	}, {
		.index		= 7,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,		
	}, {
		.index		= 8,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21,		
	}, {
		.index		= 9,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,		
	}, {
		.index		= 10,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61,		
	}, {
		.index		= 11,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420,		
	},
};

void fimc_set_active_camera(struct fimc_control *ctrl, enum fimc_cam_index id)
{
	ctrl->cam = fimc_dev->camera[id];

	dev_info(ctrl->dev, "requested id: %d\n", id);
	
	if (ctrl->cam && id < FIMC_TPID)
		fimc_select_camera(ctrl);
}

int fimc_init_camera(struct fimc_control *ctrl)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct s3c_platform_fimc *pdata;
	int ret;

	pdata = to_fimc_plat(ctrl->dev);
	if (pdata->default_cam >= FIMC_MAXCAMS) {
		dev_err(ctrl->dev, "%s: invalid camera index\n", __FUNCTION__);
		return -EINVAL;
	}

	if (!fimc->camera[pdata->default_cam]) {
		dev_err(ctrl->dev, "no external camera device\n");
		return -ENODEV;
	}

	/*
	 * ctrl->cam may not be null if already s_input called,
	 * otherwise, that should be default_cam if ctrl->cam is null.
	*/
	if (!ctrl->cam)
		ctrl->cam = fimc->camera[pdata->default_cam];

	clk_set_rate(ctrl->cam->clk, ctrl->cam->clk_rate);
	clk_enable(ctrl->cam->clk);

	if (ctrl->cam->cam_power)
		ctrl->cam->cam_power(1);

	/* subdev call for init */
	ret = v4l2_subdev_call(ctrl->cam->sd, core, init, 0);
	if (ret == -ENOIOCTLCMD) {
		dev_err(ctrl->dev, "%s: s_config subdev api not supported\n",
			__FUNCTION__);
		return ret;
	}

	ctrl->cam->initialized = 1;
	fimc_set_active_camera(ctrl, ctrl->cam->id);

	return 0;
}

int fimc_capture_scaler_info(struct fimc_control *ctrl)
{
	struct fimc_scaler *sc = &ctrl->sc;
	struct v4l2_rect *window = &ctrl->cam->window;
	int tx, ty, sx, sy;

	sx = window->width;
	sy = window->height;
	tx = ctrl->cap->fmt.width;
	ty = ctrl->cap->fmt.height;

	sc->real_width = sx;
	sc->real_height = sy;

	if (sx <= 0 || sy <= 0) {
		dev_err(ctrl->dev, "%s: invalid source size\n", __FUNCTION__);
		return -EINVAL;
	}

	if (tx <= 0 || ty <= 0) {
		dev_err(ctrl->dev, "%s: invalid target size\n", __FUNCTION__);
		return -EINVAL;
	}

	fimc_get_scaler_factor(sx, tx, &sc->pre_hratio, &sc->hfactor);
	fimc_get_scaler_factor(sy, ty, &sc->pre_vratio, &sc->vfactor);

	sc->pre_dst_width = sx / sc->pre_hratio;
	sc->pre_dst_height = sy / sc->pre_vratio;

	sc->main_hratio = (sx << 8) / (tx << sc->hfactor);
	sc->main_vratio = (sy << 8) / (ty << sc->vfactor);

	sc->scaleup_h = (tx >= sx) ? 1 : 0;
	sc->scaleup_v = (ty >= sy) ? 1 : 0;
	
	return 0;
}

int fimc_enum_input(struct file *file, void *fh, struct v4l2_input *inp)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = fh;
	struct s3c_platform_camera *cam;

	if (inp->index >= FIMC_MAXCAMS) {
		dev_err(ctrl->dev, "%s: invalid input index\n", __FUNCTION__);
		return -EINVAL;
	}

	cam = fimc->camera[inp->index];

	mutex_lock(&ctrl->v4l2_lock);

	strcpy(inp->name, cam->info->type);
	inp->type = V4L2_INPUT_TYPE_CAMERA;

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_input(struct file *file, void *fh, unsigned int *i)
{
	struct fimc_control *ctrl = fh;

	*i = (unsigned int) ctrl->cam->id;

	return 0;
}

int fimc_s_input(struct file *file, void *fh, unsigned int i)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = fh;

	if (i >= FIMC_MAXCAMS) {
		dev_err(ctrl->dev, "%s: invalid input index\n", __FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	ctrl->cam = fimc->camera[i];

	mutex_unlock(&ctrl->v4l2_lock);
	fimc_init_camera(ctrl);

	return 0;
}

int fimc_enum_fmt_vid_capture(struct file *file, void *fh, 
					struct v4l2_fmtdesc *f)
{
	struct fimc_control *ctrl = fh;
	int i = f->index;

	mutex_lock(&ctrl->v4l2_lock);

	memset(f, 0, sizeof(*f));
	memcpy(f, &capture_fmts[i], sizeof(*f));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = fh;

	if (!ctrl->cap) {
		dev_err(ctrl->dev, "%s: no capture device info\n", \
			__FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(&f->fmt.pix, 0, sizeof(f->fmt.pix));
	memcpy(&f->fmt.pix, &ctrl->cap->fmt, sizeof(f->fmt.pix));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_s_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	int i;

	/*
	 * The first time alloc for struct cap_info, and will be
	 * released at the file close.
	 * Anyone has better idea to do this?
	*/
	if (!cap) {
		cap = kzalloc(sizeof(*cap), GFP_KERNEL);
		if (!cap) {
			dev_err(ctrl->dev, "%s: no memory for " \
				"capture device info\n", __FUNCTION__);
			return -ENOMEM;
		}

		for (i = 0; i < FIMC_CAPBUFS; i++)
			cap->buf[i].state = VIDEOBUF_NEEDS_INIT;

		/* assign to ctrl */
		ctrl->cap = cap;
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(&cap->fmt, 0, sizeof(cap->fmt));
	memcpy(&cap->fmt, &f->fmt.pix, sizeof(cap->fmt));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_try_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	return 0;
}

int fimc_reqbufs_capture(void *fh, struct v4l2_requestbuffers *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	int i;

	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "%s: invalid memory type\n", __FUNCTION__);
		return -EINVAL;
	}

	if (!cap) {
		dev_err(ctrl->dev, "%s: no capture device info\n", \
			__FUNCTION__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);

	/* buffer count correction */
	if (b->count > 3)
		b->count = 5;
	else if (b->count < 2)
		b->count = 2;

	cap->nr_bufs = b->count;

	/* alloc buffers */
	for (i = 0; i < cap->nr_bufs; i++) {
		cap->buf[i].base = fimc_dma_alloc(ctrl, cap->fmt.sizeimage);
		if (!cap->buf[i].base) {
			dev_err(ctrl->dev, "%s: no memory for " \
				"capture buffer\n", __FUNCTION__);
			goto err_alloc;
		}

		cap->buf[i].length = cap->fmt.sizeimage;
		cap->buf[i].state = VIDEOBUF_PREPARED;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;

err_alloc:
	for (i = 0; i < cap->nr_bufs; i++) {
		if (cap->buf[i].base)
			fimc_dma_free(ctrl, cap->fmt.sizeimage);

		memset(&cap->buf[i], 0, sizeof(cap->buf[i]));
	}

	return -ENOMEM;
}

int fimc_querybuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "fimc is running\n");
		return -EBUSY;
	}

	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "%s: invalid memory type\n", __FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	b->length = ctrl->cap->fmt.sizeimage;
	b->m.offset = b->index * PAGE_SIZE;

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_ctrl_capture(void *fh, struct v4l2_control *c)
{
	return 0;
}

int fimc_s_ctrl_capture(void *fh, struct v4l2_control *c)
{
	return 0;
}

int fimc_cropcap_capture(void *fh, struct v4l2_cropcap *a)
{
	return 0;
}

int fimc_s_crop_capture(void *fh, struct v4l2_crop *a)
{
	return 0;
}

int fimc_start_capture(struct fimc_control *ctrl)
{
	fimc_hwset_start_scaler(ctrl);
	fimc_hwset_enable_capture(ctrl);

	return 0;
}

int fimc_stop_capture(struct fimc_control *ctrl)
{
	fimc_hwset_disable_capture(ctrl);
	fimc_hwset_stop_scaler(ctrl);

	return 0;
}

int fimc_streamon_capture(void *fh)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	int queued = 0, i;

	/* checking whether all buffers are in the QUEUED state */
	for (i = 0; i < FIMC_CAPBUFS; i++) {
		if (cap->buf[i].state == VIDEOBUF_QUEUED)
			queued++;
	}

	if (cap->nr_bufs != queued) {
		dev_err(ctrl->dev, "%s: all requested buffers should " \
			"be enqueued before streamon\n", __FUNCTION__);
		return -EINVAL;
	}

	if (!ctrl->cam || !ctrl->cam->initialized)
		fimc_init_camera(ctrl);

	fimc_hwset_camera_source(ctrl);
	fimc_hwset_camera_offset(ctrl);
	fimc_hwset_camera_type(ctrl);
	fimc_hwset_camera_polarity(ctrl);
	fimc_capture_scaler_info(ctrl);
	fimc_hwset_prescaler(ctrl);
	fimc_hwset_scaler(ctrl);
	fimc_hwset_output_colorspace(ctrl, cap->fmt.pixelformat);

	if (cap->fmt.pixelformat == V4L2_PIX_FMT_RGB32 || \
		cap->fmt.pixelformat == V4L2_PIX_FMT_RGB565)
		fimc_hwset_output_rgb(ctrl, cap->fmt.pixelformat);
	else
		fimc_hwset_output_yuv(ctrl, cap->fmt.pixelformat);
	
	fimc_hwset_output_size(ctrl, cap->fmt.width, cap->fmt.height);
	fimc_hwset_output_area(ctrl, cap->fmt.width, cap->fmt.height);
	fimc_hwset_org_output_size(ctrl, cap->fmt.width, cap->fmt.height);

	/* 
	 * we can make sure that all requested buffers are in QUEUED state and,
	 * all QUEUED buffers are placed from 0 index sequentially
	*/
	for (i = 0; i < cap->nr_bufs; i++)
		fimc_hwset_output_address(ctrl, i, cap->buf[i].base, &cap->fmt);

	fimc_start_capture(ctrl);
	
	return 0;
}

int fimc_streamoff_capture(void *fh)
{
	struct fimc_control *ctrl = fh;

	fimc_stop_capture(ctrl);

	return 0;
}

int fimc_count_capbuf(struct fimc_capinfo *cap, enum videobuf_state state)
{
	int cnt = 0, i;

	for (i = 0; i < FIMC_CAPBUFS; i++) {
		if (cap->buf[i].state == state)
			cnt++;
	}

	return cnt;
}

int fimc_find_capbuf(struct fimc_capinfo *cap, 
			enum videobuf_state state, int index)
{
	int cnt, i;

	cnt = 0;
	for (i = 0; i < FIMC_CAPBUFS; i++) {
		if (cap->buf[i].state == state)
			cnt++;

		if (cnt - 1 == index)
			return i;
	}

	return -ENOENT;
}

int fimc_rearrange_capbufs(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int qbufs, actual, i, j, bi, ref;

	qbufs = fimc_count_capbuf(cap, VIDEOBUF_QUEUED);
	actual = get_actual_bufnum(qbufs);

	for (i = 0; i < actual; i++) {
		bi = fimc_find_capbuf(cap, VIDEOBUF_QUEUED, i);
		fimc_hwset_output_address(ctrl, i, \
			cap->buf[bi].base, &cap->fmt);
	}

	for (j = i; j < FIMC_PHYBUFS; j++) {
		ref = j - actual;
		fimc_hwset_output_address(ctrl, j, \
			cap->buf[ref].base, &cap->fmt);
	}

	return 0;
}

int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;

	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "%s: invalid memory type\n", __FUNCTION__);
		return -EINVAL;
	}

	if (cap->buf[b->index].state == VIDEOBUF_QUEUED) {
		dev_err(ctrl->dev, "%s: already in QUEUE state\n", \
			__FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	if (fimc_count_capbuf(cap, VIDEOBUF_QUEUED) < cap->nr_bufs) {
		fimc_stop_capture(ctrl);
		ctrl->cap->buf[b->index].state = VIDEOBUF_QUEUED;
		fimc_rearrange_capbufs(ctrl);
		fimc_start_capture(ctrl);
	} else {
		dev_err(ctrl->dev, "%s: no space for this qbuf\n", \
			__FUNCTION__);
		mutex_unlock(&ctrl->v4l2_lock);
		return -ENOENT;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	int i;

	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "%s: invalid memory type\n", __FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	if (fimc_count_capbuf(cap, VIDEOBUF_DONE) > 1) {
		fimc_stop_capture(ctrl);
		i = fimc_hwget_frame_count(ctrl);
		ctrl->cap->buf[i].state = VIDEOBUF_IDLE;
		fimc_rearrange_capbufs(ctrl);
		fimc_start_capture(ctrl);		
	} else {
		dev_err(ctrl->dev, "%s: no buffer for this dqbuf\n", \
			__FUNCTION__);
		mutex_unlock(&ctrl->v4l2_lock);
		return -ENOENT;
	}

	mutex_unlock(&ctrl->v4l2_lock);
	
	return 0;
}

