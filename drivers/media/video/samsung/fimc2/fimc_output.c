/* linux/drivers/media/video/samsung/fimc_output.c
 *
 * V4L2 Output device support file for Samsung Camera Interface (FIMC) driver
 *
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
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
#include <media/videobuf-core.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <plat/media.h>

#include "fimc.h"

int fimc_reqbufs_output(void *fh, struct v4l2_requestbuffers *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 i;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	/* To do : V4L2_MEMORY_USERPTR */
	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "V4L2_MEMORY_MMAP is only supported\n");
		return -EINVAL;
	}

	if (ctrl->out->is_requested == 1 && b->count != 0 ) {
		dev_err(ctrl->dev, "Buffers were already requested.\n");
		return -EBUSY;
	}

	/* control user input */
	if (b->count > FIMC_OUTBUFS) {
		dev_warn(ctrl->dev, "The buffer count is modified by driver \
				from %d to %d.\n", b->count, FIMC_OUTBUFS);
		b->count = FIMC_OUTBUFS;
	} 

	/* Initialize all buffers */
	ret = fimc_check_out_buf(ctrl, b->count);
	if (ret) {
		dev_err(ctrl->dev, "Reserved memory is not sufficient.\n");
		return -EINVAL;
	}
	
	ret = fimc_init_out_buf(ctrl);
	if (ret) {
		dev_err(ctrl->dev, "Cannot initialize the buffers\n");
		return -EINVAL;
	}

	if (b->count != 0) {	/* allocate buffers */
		ctrl->out->is_requested = 1;
		
		for(i = 0; i < b->count; i++)
			ctrl->out->buf[i].state = VIDEOBUF_IDLE;
	} else {
		/* Fall through : All buffers are initialized.  */
	}

	ctrl->out->buf_num = b->count;

	return 0;
}



int fimc_querybuf_output(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);
#if 0
	if (ctrl->stream_status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	if (b->memory != V4L2_MEMORY_MMAP ) {
		rp_err(ctrl->log, "V4L2_MEMORY_MMAP is only supported.\n");
		return -EINVAL;
	}

	if (b->index > ctrl->buf_info.num ) {
		rp_err(ctrl->log, "The index is out of bounds. \
			You requested %d buffers. But you set the index as %d.\n",
			ctrl->buf_info.num, b->index);
		return -EINVAL;
	}
	
	b->flags	= ctrl->out_buf[b->index].buf_flag;
	b->m.offset	= b->index * PAGE_SIZE;
 	b->length	= ctrl->out_buf[b->index].buf_length;
#endif
	return ret;
}

int fimc_g_ctrl_output(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;

	switch (c->id) {
	case V4L2_CID_ROTATION:
		c->value = ctrl->out->rotate;
		break;

	default:
		dev_err(ctrl->dev, "Invalid control id: %d\n", c->id);
		return -EINVAL;
	}

	return 0;
}

int fimc_s_ctrl_output(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	switch (c->id) {
	case V4L2_CID_ROTATION:
		ret = fimc_mapping_rot(ctrl, c->value);
		break;

	default:
		dev_err(ctrl->dev, "Invalid control id: %d\n", c->id);
		ret = -EINVAL;
	}

	return ret;
}

int fimc_cropcap_output(void *fh, struct v4l2_cropcap *a)
{

	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;
#if 0
	unsigned int max_width = 0, max_height = 0;
	unsigned int pixelformat = ctrl->v4l2.video_out_fmt.pixelformat;
	unsigned int is_rot = 0;
	unsigned int rot_degree = ctrl->s_ctrl.rot.degree;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->stream_status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	if (pixelformat == V4L2_PIX_FMT_NV12) {
		max_width	= S3C_FIMC_YUV_SRC_MAX_WIDTH;
		max_height	= S3C_FIMC_YUV_SRC_MAX_HEIGHT;
	} else if ((pixelformat == V4L2_PIX_FMT_RGB32) || \
			(pixelformat == V4L2_PIX_FMT_RGB565)) {
		if ((rot_degree == ROT_90) || (rot_degree == ROT_270))
			is_rot = 1;

		if (is_rot == 1) {	/* Landscape */
			max_width	= ctrl->fimd.v_res;
			max_height	= ctrl->fimd.h_res;
		} else {		/* Portrait */
			max_width	= ctrl->fimd.h_res;
			max_height	= ctrl->fimd.v_res;
		}
	} else {
		dev_err(ctrl->dev, "V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_RGB32 \
				and V4L2_PIX_FMT_RGB565 are only supported..\n");
		return -EINVAL;
	}

	/* crop bounds */
	ctrl->v4l2.crop_bounds.left	= 0;
	ctrl->v4l2.crop_bounds.top	= 0;
	ctrl->v4l2.crop_bounds.width	= max_width;
	ctrl->v4l2.crop_bounds.height	= max_height;

	/* crop default values */
	ctrl->v4l2.crop_defrect.left	= 0;
	ctrl->v4l2.crop_defrect.top	= 0;
	ctrl->v4l2.crop_defrect.width	= max_width;
	ctrl->v4l2.crop_defrect.height	= max_height;

	/* crop pixel aspec values */
	/* To Do : Have to modify but I don't know the meaning. */
	ctrl->v4l2.pixelaspect.numerator	= 5;
	ctrl->v4l2.pixelaspect.denominator	= 3;

	a->bounds	= ctrl->v4l2.crop_bounds;
	a->defrect	= ctrl->v4l2.crop_defrect;
	a->pixelaspect	= ctrl->v4l2.pixelaspect;
#endif
	return ret;
}

int fimc_s_crop_output(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_streamon_output(void *fh)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_streamoff_output(void *fh)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_qbuf_output(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_dqbuf_output(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_g_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_s_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_try_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}


int fimc_g_fbuf(struct file *filp, void *fh, struct v4l2_framebuffer *fb)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_s_fbuf(struct file *filp, void *fh, struct v4l2_framebuffer *fb)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

