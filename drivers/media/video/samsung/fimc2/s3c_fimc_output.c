/* linux/drivers/media/video/samsung/s3c_fimc_output.c
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
#include <asm/io.h>
#include <asm/uaccess.h>
#include <plat/media.h>

#include "s3c_fimc.h"

int s3c_fimc_reqbufs_output(void *fh, struct v4l2_requestbuffers *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);
#if 0
	if (ctrl->stream_status != FIMC_STREAMOFF) {
		fimc_err(ctrl->log, "FIMC is running.\n");
		return -EBUSY;
	}

	/* To do : V4L2_MEMORY_USERPTR */
	if (b->memory != V4L2_MEMORY_MMAP) {
		fimc_err(ctrl->log, "V4L2_MEMORY_MMAP is only supported\n");
		return -EINVAL;
	}

	if (ctrl->buf_info.requested == TRUE && b->count != 0 ) {
		fimc_err(ctrl->log, "Buffers were already requested.\n");
		return -EBUSY;
	}

	/* control user input */
	if (b->count > S3C_FIMC_OUT_BUFF_NUM) {
		fimc_warn(ctrl->log, "The buffer count is modified by driver \
				from %d to %d.\n", b->count, S3C_FIMC_OUT_BUFF_NUM);
		b->count = S3C_FIMC_OUT_BUFF_NUM;
	} 

	/* Initialize all buffers */
	ret = s3c_fimc_check_out_buf(ctrl, b->count);
	if (ret) {
		fimc_err(ctrl->log, "Reserved memory is not sufficient.\n");
		return -EINVAL;
	}
	
	ret = s3c_fimc_init_out_buf(ctrl);
	if (ret) {
		fimc_err(ctrl->log, "Cannot initialize the buffers\n");
		return -EINVAL;
	}

	if (b->count != 0) {	/* allocate buffers */
		ctrl->buf_info.requested = TRUE;
		
		for(i = 0; i < b->count; i++)
			ctrl->out_buf[i].buf_state = BUF_DQUEUED;
	} else {
		/* fall through */
		/* All buffers are initialized.  */
	}

	ctrl->buf_info.num = b->count;
#endif
	return 0;
}

int s3c_fimc_querybuf_output(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);
#if 0
	if (ctrl->stream_status != FIMC_STREAMOFF) {
		fimc_err(ctrl->log, "FIMC is running.\n");
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

int s3c_fimc_g_ctrl_output(void *fh, struct v4l2_control *c)
{
	switch (c->id) {
	case V4L2_CID_ROTATION:
		c->value = ctrl->out.rotate;
		break;

	default:
		fimc_err(ctrl->log, "Invalid control id: %d\n", c->id);
		return -EINVAL;
	}

	return 0;
}

int s3c_fimc_cropcap_output(void *fh, struct v4l2_cropcap *a)
{
#if 0
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;
	unsigned int max_width = 0, max_height = 0;
	unsigned int pixelformat = ctrl->v4l2.video_out_fmt.pixelformat;
	unsigned int is_rot = 0;
	unsigned int rot_degree = ctrl->s_ctrl.rot.degree;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	if (ctrl->stream_status != FIMC_STREAMOFF) {
		fimc_err(ctrl->log, "FIMC is running.\n");
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
		fimc_err(ctrl->log, "V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_RGB32 \
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

int s3c_fimc_s_crop_output(struct file *filp, void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

int s3c_fimc_streamon_output(struct file *filp, void *fh, enum v4l2_buf_type i)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

int s3c_fimc_streamoff_output(struct file *filp, void *fh, enum v4l2_buf_type i)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

int s3c_fimc_qbuf_output(struct file *filp, void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

int s3c_fimc_dqbuf_output(struct file *filp, void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

static int s3c_fimc_g_fmt_vid_out(struct file *filp, void *fh, 
						struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

static int s3c_fimc_s_fmt_vid_out(struct file *filp, void *fh, 
						struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

static int s3c_fimc_try_fmt_vid_out(struct file *filp, void *fh, 
						struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}


static int s3c_fimc_g_fbuf(struct file *filp, void *fh,
					struct v4l2_framebuffer *fb)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

static int s3c_fimc_s_fbuf(struct file *filp, void *fh,
					struct v4l2_framebuffer *fb)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	fimc_info(ctrl->log, "[%s] called\n", __FUNCTION__);

	return ret;
}

