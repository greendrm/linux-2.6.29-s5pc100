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
#include <linux/mm.h>
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
		.description	= "RGB-8-8-8, unpacked 24 bpp",
		.pixelformat	= V4L2_PIX_FMT_RGB32,		
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

static int fimc_init_camera(struct fimc_control *ctrl)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct s3c_platform_fimc *pdata;
	int ret;

	pdata = to_fimc_plat(ctrl->dev);
	if (pdata->default_cam >= FIMC_MAXCAMS) {
		dev_err(ctrl->dev, "[%s] invalid camera index\n", __FUNCTION__);
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

	if (ctrl->cam->initialized)
		return 0;

	clk_set_rate(ctrl->cam->clk, ctrl->cam->clk_rate);
	clk_enable(ctrl->cam->clk);

	if (ctrl->cam->cam_power)
		ctrl->cam->cam_power(1);

	/* subdev call for init */
	ret = v4l2_subdev_call(ctrl->cam->sd, core, init, 0);
	if (ret == -ENOIOCTLCMD) {
		dev_err(ctrl->dev, "[%s] s_config subdev api not supported\n",
			__FUNCTION__);
		return ret;
	}

	ctrl->cam->initialized = 1;

	return 0;
}

static int fimc_capture_scaler_info(struct fimc_control *ctrl)
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
		dev_err(ctrl->dev, "[%s] invalid source size\n", __FUNCTION__);
		return -EINVAL;
	}

	if (tx <= 0 || ty <= 0) {
		dev_err(ctrl->dev, "[%s] invalid target size\n", __FUNCTION__);
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

static int fimc_count_actual_buffers(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int i, count;

	count = 0;
	for (i = 0; i < cap->nr_bufs; i++) {
		if (cap->bufs[i].state == VIDEOBUF_QUEUED)
			count++;
	}

	return count > 3 ? 4 : (count < 2 ? 1 : 2);
}

static int fimc_update_inqueue(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int i, j;

	memset(cap->inqueue, -1, sizeof(cap->inqueue));

	j = 0;
	for (i = 0; i < cap->nr_bufs; i++) {
		if (cap->bufs[i].state == VIDEOBUF_QUEUED) {
			cap->inqueue[j] = i;
			j++;
		}
	}

	dev_dbg(ctrl->dev, "inqueue:  [%2d] [%2d] [%2d] [%2d] [%2d]\n", \
		cap->inqueue[0], cap->inqueue[1], cap->inqueue[2], \
		cap->inqueue[3], cap->inqueue[4]);

	return 0;
}

static int fimc_update_outqueue(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int i, j, actual;

	memset(cap->outqueue, -1, sizeof(cap->outqueue));
	actual = fimc_count_actual_buffers(ctrl);

	for (i = 0; i < actual; i++)
		cap->outqueue[i] = cap->inqueue[i];

	for (j = i; j < FIMC_PHYBUFS; j++)
		cap->outqueue[j] = cap->outqueue[j - actual];

	dev_dbg(ctrl->dev, "outqueue: [%2d] [%2d] [%2d] [%2d]\n", \
		cap->outqueue[0], cap->outqueue[1], \
		cap->outqueue[2], cap->outqueue[3]);

	return 0;
}

static int fimc_update_hwaddr(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	dma_addr_t base;
	int i;

	for (i = 0; i < FIMC_PHYBUFS; i++) {
		base = cap->bufs[cap->outqueue[i]].base;
		fimc_hwset_output_address(ctrl, i, base, &cap->fmt);
	}

	return 0;
}

int fimc_g_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct fimc_control *ctrl = fh;
	int ret;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);
	ret = subdev_call(ctrl, video, g_parm, a);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_s_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct fimc_control *ctrl = fh;
	int ret;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);
	ret = subdev_call(ctrl, video, s_parm, a);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_enum_input(struct file *file, void *fh, struct v4l2_input *inp)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = fh;
	struct s3c_platform_camera *cam;

	if (inp->index >= FIMC_MAXCAMS) {
		dev_err(ctrl->dev, "[%s] invalid input index\n", __FUNCTION__);
		return -EINVAL;
	}

	dev_dbg(ctrl->dev, "[%s] enuminput index %d\n", \
		__FUNCTION__, inp->index);

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

	dev_dbg(ctrl->dev, "[%s] g_input index %d\n", __FUNCTION__, *i);

	return 0;
}

int fimc_s_input(struct file *file, void *fh, unsigned int i)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = fh;

	if (i >= FIMC_MAXCAMS) {
		dev_err(ctrl->dev, "[%s] invalid input index\n", __FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	dev_dbg(ctrl->dev, "[%s] s_input index %d\n", __FUNCTION__, i);
	
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

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);

	memset(f, 0, sizeof(*f));
	memcpy(f, &capture_fmts[i], sizeof(*f));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = fh;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	if (!ctrl->cap) {
		dev_err(ctrl->dev, "[%s] no capture device info\n", \
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

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	/*
	 * The first time alloc for struct cap_info, and will be
	 * released at the file close.
	 * Anyone has better idea to do this?
	*/
	if (!cap) {
		cap = kzalloc(sizeof(*cap), GFP_KERNEL);
		if (!cap) {
			dev_err(ctrl->dev, "[%s] no memory for " \
				"capture device info\n", __FUNCTION__);
			return -ENOMEM;
		}

		for (i = 0; i < FIMC_CAPBUFS; i++) {
			cap->bufs[i].state = VIDEOBUF_NEEDS_INIT;
			cap->inqueue[i] = -1;
			cap->outqueue[i] = -1;
		}

		/* assign to ctrl */
		ctrl->cap = cap;
	} else {
		memset(cap, 0, sizeof(*cap));
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(&cap->fmt, 0, sizeof(cap->fmt));
	memcpy(&cap->fmt, &f->fmt.pix, sizeof(cap->fmt));

	if (cap->fmt.colorspace == V4L2_COLORSPACE_JPEG)
		ctrl->sc.bypass = 1;

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
		dev_err(ctrl->dev, "[%s] invalid memory type\n", __FUNCTION__);
		return -EINVAL;
	}

	if (!cap) {
		dev_err(ctrl->dev, "[%s] no capture device info\n", \
			__FUNCTION__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);

	/* buffer count correction */
	if (b->count > 4)
		b->count = 5;
	else if (b->count < 2)
		b->count = 2;

	cap->nr_bufs = b->count;

	dev_dbg(ctrl->dev, "[%s] requested %d buffers\n", \
		__FUNCTION__, b->count);

	/* free previous buffers */
	for (i = 0; i < FIMC_CAPBUFS; i++) {
		fimc_dma_free(ctrl, &cap->bufs[i].base, cap->bufs[i].length);
		cap->bufs[i].base = 0;
		cap->bufs[i].length = 0;
		cap->bufs[i].state = VIDEOBUF_NEEDS_INIT;
	}

	/* alloc buffers */
	for (i = 0; i < cap->nr_bufs; i++) {
		cap->bufs[i].base = fimc_dma_alloc(ctrl, \
					PAGE_ALIGN(cap->fmt.sizeimage));
		if (!cap->bufs[i].base) {
			dev_err(ctrl->dev, "[%s] no memory for " \
				"capture buffer\n", __FUNCTION__);
			goto err_alloc;
		}

		cap->bufs[i].length = PAGE_ALIGN(cap->fmt.sizeimage);
		cap->bufs[i].state = VIDEOBUF_PREPARED;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;

err_alloc:
	for (i = 0; i < cap->nr_bufs; i++) {
		if (cap->bufs[i].base) {
			fimc_dma_free(ctrl, &cap->bufs[i].base, \
					cap->bufs[i].length);
		}

		memset(&cap->bufs[i], 0, sizeof(cap->bufs[i]));
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

	mutex_lock(&ctrl->v4l2_lock);

	b->length = ctrl->cap->bufs[b->index].length;
	b->m.offset = b->index * PAGE_SIZE;

	dev_dbg(ctrl->dev, "[%s] querybuf %d bytes with offset: %d\n",\
		__FUNCTION__, b->length, b->m.offset);

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_ctrl_capture(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = fh;
	int ret = 0;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);

	/* First, get ctrl supported by subdev */
	ret = subdev_call(ctrl, core, g_ctrl, c);

	mutex_unlock(&ctrl->v4l2_lock);
	
	return ret;
}

int fimc_s_ctrl_capture(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = fh;
	int ret;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);

	if (c->id < V4L2_CID_PRIVATE_BASE) {
		/* If issued CID is not private based, try on subdev */
		ret = subdev_call(ctrl, core, s_ctrl, c);
		mutex_unlock(&ctrl->v4l2_lock);
		return ret;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_cropcap_capture(void *fh, struct v4l2_cropcap *a)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);

	if (!ctrl->cam) {
		dev_err(ctrl->dev, "%s: s_input should be done before crop\n", \
			__FUNCTION__);
		return -ENODEV;
	}

	/* crop limitations */
	cap->cropcap.bounds.left = 0;
	cap->cropcap.bounds.top = 0;
	cap->cropcap.bounds.width = ctrl->cam->width;
	cap->cropcap.bounds.height = ctrl->cam->height;

	/* crop default values */
	cap->cropcap.defrect.left = 0;
	cap->cropcap.defrect.top = 0;
	cap->cropcap.defrect.width = ctrl->cam->width;
	cap->cropcap.defrect.height = ctrl->cam->height;

	a->bounds = cap->cropcap.bounds;
	a->defrect = cap->cropcap.defrect;

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_crop_capture(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = fh;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);
	a->c = ctrl->cap->crop;
	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_s_crop_capture(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = fh;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	mutex_lock(&ctrl->v4l2_lock);
	ctrl->cap->crop = a->c;
	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_start_capture(struct fimc_control *ctrl)
{
	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	if (!ctrl->sc.bypass)
		fimc_hwset_start_scaler(ctrl);

	fimc_hwset_enable_capture(ctrl);

	return 0;
}

int fimc_stop_capture(struct fimc_control *ctrl)
{
	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	fimc_hwset_disable_capture(ctrl);
	fimc_hwset_stop_scaler(ctrl);

	return 0;
}

int fimc_streamon_capture(void *fh)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	int queued = 0, i, width, height;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	/* checking whether all buffers are in the QUEUED state */
	for (i = 0; i < FIMC_CAPBUFS; i++) {
		if (cap->bufs[i].state == VIDEOBUF_QUEUED)
			queued++;
	}

	if (cap->nr_bufs != queued) {
		dev_err(ctrl->dev, "[%s] all requested buffers should " \
			"be enqueued before streamon\n", __FUNCTION__);
		return -EINVAL;
	}

	width = min_t(int, cap->fmt.width, cap->crop.width);
	height = min_t(int, cap->fmt.height, cap->crop.height);
	ctrl->status = FIMC_READY_ON;
	cap->irq = FIMC_IRQ_NONE;

	fimc_hwset_enable_irq(ctrl, 0, 1);

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
	fimc_hwset_output_offset(ctrl, cap->fmt.pixelformat, \
				&cap->cropcap.bounds, &cap->crop);

	fimc_hwset_org_output_size(ctrl, width, height);

	fimc_start_capture(ctrl);
	ctrl->status = FIMC_STREAMON;
	
	return 0;
}

int fimc_streamoff_capture(void *fh)
{
	struct fimc_control *ctrl = fh;

	dev_dbg(ctrl->dev, "%s\n", __FUNCTION__);

	ctrl->status = FIMC_READY_OFF;

	fimc_stop_capture(ctrl);

	ctrl->status = FIMC_STREAMOFF;

	return 0;
}

int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;

	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "[%s] invalid memory type\n", __FUNCTION__);
		return -EINVAL;
	}

	if (cap->bufs[b->index].state == VIDEOBUF_QUEUED) {
		dev_err(ctrl->dev, "[%s] already in QUEUED state\n", \
			__FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	dev_dbg(ctrl->dev, "[%s] qbuf index %d\n", __FUNCTION__, b->index);

	ctrl->cap->bufs[b->index].state = VIDEOBUF_QUEUED;

	/* do not change the status although stop capture */
	if (ctrl->status == FIMC_STREAMON)
		fimc_stop_capture(ctrl);

	fimc_update_inqueue(ctrl);
	fimc_update_outqueue(ctrl);
	fimc_update_hwaddr(ctrl);

	wake_up_interruptible(&ctrl->wq);

	/* current ctrl->status means previous status before qbuf */
	if (ctrl->status == FIMC_STREAMON)
		fimc_start_capture(ctrl);

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	int ppnum;

	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "[%s] invalid memory type\n", __FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	ppnum = (fimc_hwget_frame_count(ctrl) + 2) % 4;
	b->index = cap->outqueue[ppnum];

	dev_dbg(ctrl->dev, "[%s] dqbuf index %d\n", __FUNCTION__, b->index);

	if (b->index < 0) {
		dev_err(ctrl->dev, "[%s] no available capture buffer\n", \
			__FUNCTION__);
		return -EINVAL;
	}

	cap->bufs[b->index].state = VIDEOBUF_ACTIVE;

	fimc_stop_capture(ctrl);
	fimc_update_inqueue(ctrl);
	fimc_update_outqueue(ctrl);
	fimc_update_hwaddr(ctrl);
	fimc_start_capture(ctrl);

	mutex_unlock(&ctrl->v4l2_lock);
	
	return 0;
}

