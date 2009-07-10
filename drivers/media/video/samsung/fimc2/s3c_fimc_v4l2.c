/* linux/drivers/media/video/samsung/s3c_fimc_v4l2.c
 *
 * V4L2 interface support file for Samsung Camera Interface (FIMC) driver
 *
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>

#include "s3c_fimc.h"

static int s3c_fimc_querycap(struct file *filp, void *fh,
					struct v4l2_capability *cap)
{
	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	strcpy(cap->driver, "Samsung FIMC Driver");
	strlcpy(cap->card, ctrl->vd->name, sizeof(cap->card));
	sprintf(cap->bus_info, "FIMC AHB-bus");

	cap->version = 0;
	cap->capabilities = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT \
				V4L2_CAP_VIDEO_OVERLAY | V4L2_CAP_STREAMING);

	return 0;
}

static int s3c_fimc_reqbufs(struct file *filp, void *fh, 
				struct v4l2_requestbuffers *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (b->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_reqbufs_capture(fh, b);
	} else if (b->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_reqbufs_output(fh, b);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

static int s3c_fimc_querybuf(struct file *filp, void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (b->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_querybuf_capture(fh, b);
	} else if (b->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_querybuf_output(fh, b);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

static int s3c_fimc_g_ctrl(struct file *filp, void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called. CID = %d\n", __FUNCTION__, c->id);

	if (ctrl->cap != NULL) {
		ret = s3c_fimc_g_ctrl_capture(fh, c);
	} else if(ctrl->out != NULL) {
		ret = s3c_fimc_g_ctrl_output(fh, c);
	} else {
		fimc_err(ctrl->log, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	return ret;
}

static int s3c_fimc_s_ctrl(struct file *filp, void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;	
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called. CID = %d, Value = %d\n", \
		__FUNCTION__, c->id, c->value);
#if 0
	switch (c->id) {
	case V4L2_CID_ROTATION:
		ret = s3c_fimc_rot_set_degree(ctrl, c->value);
		if (ret < 0) {
			fimc_err(ctrl->log, "V4L2_CID_ROTATION is failed.\n");
			ret = -EINVAL;
		}

		break;

	default:
		fimc_err(ctrl->log, "invalid control id: %d\n", c->id);
		ret = -EINVAL;
	}
#endif
	return ret;
}

static int s3c_fimc_cropcap(struct file *filp, void *fh, struct v4l2_cropcap *a)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (a->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_cropcap_capture(fh, a);
	} else if (a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_cropcap_output(fh, a);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

static int s3c_fimc_s_crop(struct file *filp, void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (a->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_s_crop_capture(fh, a);
	} else if (a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_s_crop_output(fh, a);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

static int s3c_fimc_streamon(struct file *filp, void *fh, enum v4l2_buf_type i)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (i == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_streamon_capture(fh);
	} else if (i == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_streamon_output(fh);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

static int s3c_fimc_streamoff(struct file *filp, void *fh, enum v4l2_buf_type i)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (i == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_streamoff_capture(fh);
	} else if (i == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_streamoff_output(fh);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

static int s3c_fimc_qbuf(struct file *filp, void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (b->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_qbuf_capture(fh);
	} else if (b->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_qbuf_output(fh);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

static int s3c_fimc_dqbuf(struct file *filp, void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (b->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ret = s3c_fimc_dqbuf_capture(fh);
	} else if (b->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		ret = s3c_fimc_dqbuf_output(fh);
	} else {
		fimc_err(ctrl->log, "V4L2_BUF_TYPE_VIDEO_CAPTURE and \
			V4L2_BUF_TYPE_VIDEO_OUTPUT are only supported.\n");
		ret = -EINVAL;
	}

	return ret;
}

const struct v4l2_ioctl_ops s3c_fimc_v4l2_ops = {
	.vidioc_querycap		= s3c_fimc_querycap,
	.vidioc_reqbufs			= s3c_fimc_reqbufs,
	.vidioc_querybuf		= s3c_fimc_querybuf,
	.vidioc_g_ctrl			= s3c_fimc_g_ctrl,
	.vidioc_s_ctrl			= s3c_fimc_s_ctrl,
	.vidioc_cropcap			= s3c_fimc_cropcap,
	.vidioc_s_crop			= s3c_fimc_s_crop,
	.vidioc_streamon		= s3c_fimc_streamon,
	.vidioc_streamoff		= s3c_fimc_streamoff,
	.vidioc_qbuf			= s3c_fimc_qbuf,
	.vidioc_dqbuf			= s3c_fimc_dqbuf,

	.vidioc_enum_fmt_vid_cap	= s3c_fimc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= s3c_fimc_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= s3c_fimc_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= s3c_fimc_try_fmt_vid_cap,
	.vidioc_g_input			= s3c_fimc_g_input,
	.vidioc_s_input			= s3c_fimc_s_input,
	.vidioc_g_output		= s3c_fimc_g_output,
	.vidioc_s_output		= s3c_fimc_s_output,
	.vidioc_enum_input		= s3c_fimc_enum_input,
	.vidioc_enum_output		= s3c_fimc_enum_output,
	.vidioc_s_parm			= s3c_fimc_s_parm,

	.vidioc_g_fmt_vid_out		= s3c_fimc_g_fmt_vid_out,
	.vidioc_s_fmt_vid_out		= s3c_fimc_s_fmt_vid_out,
	.vidioc_try_fmt_vid_out		= s3c_fimc_try_fmt_vid_out,
	.vidioc_g_fbuf			= s3c_fimc_g_fbuf,
	.vidioc_s_fbuf			= s3c_fimc_s_fbuf,

	.vidioc_try_fmt_vid_overlay	= s3c_fimc_try_fmt_overlay,
	.vidioc_g_fmt_vid_overlay	= s3c_fimc_g_fmt_vid_overlay,
	.vidioc_s_fmt_vid_overlay	= s3c_fimc_s_fmt_vid_overlay,
};

