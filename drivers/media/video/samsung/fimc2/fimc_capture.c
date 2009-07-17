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

static int fimc_init_camera(struct fimc_control *ctrl)
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

	if (!ctrl->cam)
		ctrl->cam = fimc->camera[pdata->default_cam];

	clk_set_rate(fimc->mclk, ctrl->cam->clk_rate);
	clk_enable(fimc->mclk);

	if (ctrl->cam->cam_power)
		ctrl->cam->cam_power(1);

//	fimc_set_active_camera(ctrl, ctrl->cam->id);

	ret = v4l2_subdev_call(ctrl->cam->sd, core, init, 0);
	if (ret == -ENOIOCTLCMD) {
		dev_err(ctrl->dev, "%s: s_config subdev api not supported\n",
			__FUNCTION__);
		return ret;
	}

	return 0;
}

int fimc_enum_input(struct file *file, void *fh, struct v4l2_input *inp)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = file->private_data;
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
	struct fimc_control *ctrl = file->private_data;
	struct s3c_platform_camera *cam = ctrl->cam;

	*i = (unsigned int) cam->id;

	return 0;
}

int fimc_s_input(struct file *file, void *fh, unsigned int i)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = file->private_data;

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

int fimc_enum_fmt_vid_capture(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct fimc_control *ctrl = file->private_data;
	int i = f->index;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		dev_err(ctrl->dev, "%s: invalid buffer type\n", __FUNCTION__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(f, 0, sizeof(*f));
	memcpy(f, &capture_fmts[i], sizeof(*f));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = file->private_data;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		dev_err(ctrl->dev, "%s: invalid buffer type\n", __FUNCTION__);
		return -EINVAL;
	}

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
	struct fimc_control *ctrl = file->private_data;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		dev_err(ctrl->dev, "%s: invalid buffer type\n", __FUNCTION__);
		return -EINVAL;
	}

	if (!ctrl->cap) {
		ctrl->cap = kzalloc(sizeof(*ctrl->cap), GFP_KERNEL);
		if (!ctrl->cap) {
			dev_err(ctrl->dev, "%s: no memory for " \
				"capture device info\n", __FUNCTION__);
			return -ENOMEM;
		}
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(&ctrl->cap->fmt, 0, sizeof(ctrl->cap->fmt));
	memcpy(&ctrl->cap->fmt, &f->fmt.pix, sizeof(ctrl->cap->fmt));

	mutex_unlock(&ctrl->v4l2_lock);

	/* TODO: change h/w states */
	fimc_init_camera(ctrl);

	return 0;
}

int fimc_try_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	return 0;
}

int fimc_reqbufs_capture(void *fh, struct v4l2_requestbuffers *b)
{
	return 0;
}

int fimc_querybuf_capture(void *fh, struct v4l2_buffer *b)
{
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

int fimc_streamon_capture(void *fh)
{
	return 0;
}

int fimc_streamoff_capture(void *fh)
{
	return 0;
}

int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b)
{
	return 0;
}

int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b)
{
	return 0;
}

