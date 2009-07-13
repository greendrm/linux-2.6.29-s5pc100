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

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	/* To do : V4L2_MEMORY_USERPTR */
	if (b->memory != V4L2_MEMORY_MMAP) {
		dev_err(ctrl->dev, "V4L2_MEMORY_MMAP is only supported\n");
		return -EINVAL;
	}

	if (b->index > ctrl->out->buf_num ) {
		dev_err(ctrl->dev, "The index is out of bounds. \
			You requested %d buffers. But you set the index as %d.\n",
			ctrl->out->buf_num, b->index);
		return -EINVAL;
	}

	b->flags	= ctrl->out->buf[b->index].flags;
	b->m.offset	= b->index * PAGE_SIZE;
 	b->length	= ctrl->out->buf[b->index].length;

	return ret;
}

int fimc_g_ctrl_output(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

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

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

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
	u32 pixelformat = ctrl->out->pix.pixelformat;
	u32 rot_degree = ctrl->out->rotate;
	u32 max_w = 0, max_h = 0;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	if (pixelformat == V4L2_PIX_FMT_NV12) {
		max_w	= FIMC_SRC_MAX_W;
		max_h	= FIMC_SRC_MAX_H;
	} else if ((pixelformat == V4L2_PIX_FMT_RGB32) || \
			(pixelformat == V4L2_PIX_FMT_RGB565)) {
		if ((rot_degree == 90) || (rot_degree == 270)) {	/* Landscape */
			max_w	= ctrl->fb.lcd->height;
			max_h	= ctrl->fb.lcd->width;
		} else {						/* Portrait */
			max_w	= ctrl->fb.lcd->width;
			max_h	= ctrl->fb.lcd->height;
		}
	} else {
		dev_err(ctrl->dev, "V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_RGB32 \
				and V4L2_PIX_FMT_RGB565 are only supported.\n");
		return -EINVAL;
	}

	/* crop bounds */
	ctrl->cropcap.bounds.left	= 0;
	ctrl->cropcap.bounds.top	= 0;
	ctrl->cropcap.bounds.width	= max_w;
	ctrl->cropcap.bounds.height	= max_h;

	/* crop default values */
	ctrl->cropcap.defrect.left	= 0;
	ctrl->cropcap.defrect.top	= 0;
	ctrl->cropcap.defrect.width	= max_w;
	ctrl->cropcap.defrect.height	= max_h;

	/* crop pixel aspec values */
	/* To Do : Have to modify but I don't know the meaning. */
	ctrl->cropcap.pixelaspect.numerator	= 5;
	ctrl->cropcap.pixelaspect.denominator	= 3;

	a->bounds	= ctrl->cropcap.bounds;
	a->defrect	= ctrl->cropcap.defrect;
	a->pixelaspect	= ctrl->cropcap.pixelaspect;

	return ret;
}



int fimc_s_crop_output(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 max_w = 0, max_h = 0;	
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	/* Check arguments : widht and height */
	if ((a->c.width < 0) || (a->c.height < 0)) {
		dev_err(ctrl->dev, "The crop rect must be bigger than 0.\n");
		return -1;
	}


	if (ctrl->out->pix.pixelformat == V4L2_PIX_FMT_NV12) {
		max_w	= FIMC_SRC_MAX_W;
		max_h	= FIMC_SRC_MAX_H;
	} else if ((pixelformat == V4L2_PIX_FMT_RGB32) || \
			(pixelformat == V4L2_PIX_FMT_RGB565)) {
		if ((ctrl->out->rotate == 90) || (ctrl->out->rotate == 270)) {
			max_w	= ctrl->fb.lcd->height;
			max_h	= ctrl->fb.lcd->width;
		} else {
			max_w	= ctrl->fb.lcd->width;
			max_h	= ctrl->fb.lcd->height;
		}	
	}

	if ((a->c.width > max_w) || (a->c.height > max_h)) {
		fimc_err(ctrl->dev, "The crop rect width and height must be \
				smaller than %d and %d.\n", max_w, max_h);
		return -1;
	}

	/* Check arguments : left and top */
	if ((a->c.left < 0) || (a->c.top < 0)) {
		fimc_err(ctrl->dev, "The crop rect left and top must be \
				bigger than zero.\n");
		return -1;
	}
	
	if ((a->c.left > max_w) || (a->c.top > max_h)) {
		fimc_err(ctrl->dev, "The crop rect left and top must be \
				smaller than %d, %d.\n", max_w, max_h);
		return -1;
	}

	if ((a->c.left + a->c.width) > max_w) {
		fimc_err(ctrl->dev, "The crop rect must be in bound rect.\n");
		return -1;
	}
	
	if ((a->c.top + a->c.height) > max_h) {
		fimc_err(ctrl->dev, "The crop rect must be in bound rect.\n");
		return -1;
	}
















	ctrl->v4l2.crop_rect.left	= a->c.left;
	ctrl->v4l2.crop_rect.top	= a->c.top;
	ctrl->v4l2.crop_rect.width	= a->c.width;
	ctrl->v4l2.crop_rect.height	= a->c.height;

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

