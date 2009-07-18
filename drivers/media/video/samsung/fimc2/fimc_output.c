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

	/* Validation check & Initialize all buffers */
	ret = fimc_check_out_buf(ctrl, b->count);
	if (ret)
		return ret;
	
	ret = fimc_init_out_buf(ctrl);
	if (ret)
		return ret;

	if (b->count != 0)	/* allocate buffers */
		ctrl->out->is_requested = 1;

	ctrl->out->buf_num = b->count;

	return 0;
}

int fimc_querybuf_output(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

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

	return 0;
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
		ret = fimc_set_rot_degree(ctrl, c->value);
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
	u32 pixelformat = ctrl->out->pix.pixelformat;
	u32 is_rotate = 0;
	u32 max_w = 0, max_h = 0;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	is_rotate = fimc_mapping_rot_flip(ctrl->out->rotate, ctrl->out->flip);
	if (pixelformat == V4L2_PIX_FMT_NV12) {
		max_w	= FIMC_SRC_MAX_W;
		max_h	= FIMC_SRC_MAX_H;
	} else if ((pixelformat == V4L2_PIX_FMT_RGB32) || \
			(pixelformat == V4L2_PIX_FMT_RGB565)) {
		if (is_rotate && 0x10) {		/* Landscape mode */
			max_w	= ctrl->fb.lcd->height;
			max_h	= ctrl->fb.lcd->width;
		} else {				/* Portrait */
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
	ctrl->cropcap.pixelaspect.numerator	= 16;
	ctrl->cropcap.pixelaspect.denominator	= 9;

	a->bounds	= ctrl->cropcap.bounds;
	a->defrect	= ctrl->cropcap.defrect;
	a->pixelaspect	= ctrl->cropcap.pixelaspect;

	return 0;
}

int fimc_s_crop_output(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 pixelformat = ctrl->out->pix.pixelformat;
	u32 max_w = 0, max_h = 0;	
	u32 is_rotate = 0;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	/* Check arguments : widht and height */
	if ((a->c.width < 0) || (a->c.height < 0)) {
		dev_err(ctrl->dev, "The crop rect must be bigger than 0.\n");
		return -EINVAL;
	}

	is_rotate = fimc_mapping_rot_flip(ctrl->out->rotate, ctrl->out->flip);
	if (pixelformat == V4L2_PIX_FMT_NV12) {
		max_w	= FIMC_SRC_MAX_W;
		max_h	= FIMC_SRC_MAX_H;
	} else if ((pixelformat == V4L2_PIX_FMT_RGB32) || \
			(pixelformat == V4L2_PIX_FMT_RGB565)) {
		if (is_rotate && 0x10) {		/* Landscape mode */
			max_w	= ctrl->fb.lcd->height;
			max_h	= ctrl->fb.lcd->width;
		} else {				/* Portrait */
			max_w	= ctrl->fb.lcd->width;
			max_h	= ctrl->fb.lcd->height;
		}	
	}

	if ((a->c.width > max_w) || (a->c.height > max_h)) {
		dev_err(ctrl->dev, "The crop rect width and height must be \
				smaller than %d and %d.\n", max_w, max_h);
		return -EINVAL;
	}

	/* Check arguments : left and top */
	if ((a->c.left < 0) || (a->c.top < 0)) {
		dev_err(ctrl->dev, "The crop rect left and top must be \
				bigger than zero.\n");
		return -EINVAL;
	}
	
	if ((a->c.left > max_w) || (a->c.top > max_h)) {
		dev_err(ctrl->dev, "The crop rect left and top must be \
				smaller than %d, %d.\n", max_w, max_h);
		return -EINVAL;
	}

	if ((a->c.left + a->c.width) > max_w) {
		dev_err(ctrl->dev, "The crop rect must be in bound rect.\n");
		return -EINVAL;
	}
	
	if ((a->c.top + a->c.height) > max_h) {
		dev_err(ctrl->dev, "The crop rect must be in bound rect.\n");
		return -EINVAL;
	}

	ctrl->out->crop.c.left		= a->c.left;
	ctrl->out->crop.c.top		= a->c.top;
	ctrl->out->crop.c.width		= a->c.width;
	ctrl->out->crop.c.height	= a->c.height;

	return 0;
}

int fimc_streamon_output(void *fh)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	ret = fimc_check_param(ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail: fimc_check_param\n");
		return ret;
	}

	ctrl->status = FIMC_READY_ON;

	ret = fimc_set_param(ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail: fimc_set_param\n");
		return ret;
	}

	return ret;
}

int fimc_streamoff_output(void *fh)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 i = 0;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	ctrl->status = FIMC_READY_OFF;

	ret = fimc_stop_streaming(ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail: fimc_stop_streaming\n");
		return -EINVAL;
	}

	ret = fimc_init_in_queue(ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail: fimc_init_in_queue\n");
		return -EINVAL;
	}

	ret = fimc_init_out_queue(ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail: fimc_init_out_queue\n");
		return -EINVAL;
	}

	/* Make all buffers DQUEUED state. */
	for(i = 0; i < FIMC_OUTBUFS; i++) {
		ctrl->out->buf[i].state	=	VIDEOBUF_IDLE;
		ctrl->out->buf[i].flags =	V4L2_BUF_FLAG_MAPPED;
	}

	ctrl->out->idx.prev	= -1;
	ctrl->out->idx.active	= -1;
	ctrl->out->idx.next	= -1;

	ctrl->status		= FIMC_STREAMOFF;

	return 0;
}

static int fimc_qbuf_output_dma(struct fimc_control *ctrl)
{
	dma_addr_t base = 0, dst_base = 0;
	u32 index = 0;
	int ret = -1;
	
	if ((ctrl->status == FIMC_READY_ON) || \
				(ctrl->status == FIMC_STREAMON_IDLE)) {
		ret =  fimc_detach_in_queue(ctrl, &index);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail: fimc_detach_in_queue\n");
			return -1;
		}

		base = ctrl->out->buf[index].base;
		fimc_set_src_addr(ctrl, base);

		dst_base = (dma_addr_t)ctrl->out->fbuf.base;
		fimc_set_dst_addr(ctrl, (dma_addr_t)dst_base, 0);
		fimc_set_dst_addr(ctrl, (dma_addr_t)dst_base, 1);
		fimc_set_dst_addr(ctrl, (dma_addr_t)dst_base, 2);
		fimc_set_dst_addr(ctrl, (dma_addr_t)dst_base, 3);

		ret = fimc_start_camif(ctrl);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail: fimc_start_camif\n");
			return -1;
		}

		ctrl->out->idx.active = index;
		ctrl->status = FIMC_STREAMON;
	}

	return 0;
}

static int fimc_qbuf_output_fifo(struct fimc_control *ctrl)
{
	dma_addr_t base = 0;
	u32 index = 0;
	int ret = -1;

	if (ctrl->status == FIMC_READY_ON) {
		ret =  fimc_detach_in_queue(ctrl, &index);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail: fimc_detach_in_queue\n");
			return -EINVAL;
		}

		base = ctrl->out->buf[index].base;
		fimc_set_src_addr(ctrl, base);

		ret = fimc_start_fifo(ctrl);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail: fimc_start_fifo\n");
			return -EINVAL;
		}

		ctrl->out->idx.active = index;
		ctrl->status = FIMC_STREAMON;
	}

	return 0;	
}

int fimc_qbuf_output(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	dma_addr_t dst_base = 0;
	int	ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	if (b->memory != V4L2_MEMORY_MMAP ) {
		dev_err(ctrl->dev, "V4L2_MEMORY_MMAP is only supported.\n");
		return -EINVAL;
	}

	if (b->index > ctrl->out->buf_num ) {
		dev_err(ctrl->dev, "The index is out of bounds. \
				You requested %d buffers. \
				But you set the index as %d.\n", \
				ctrl->out->buf_num, b->index);
		return -EINVAL;
	}

	/* Check the buffer state if the state is VIDEOBUF_IDLE. */
	if (ctrl->out->buf[b->index].state != VIDEOBUF_IDLE) {
		dev_err(ctrl->dev, "The index(%d) buffer must be \
				dequeued state(%d).\n", b->index, \
				ctrl->out->buf[b->index].state);
		return -EINVAL;
	}

	/* Attach the buffer to the incoming queue. */
	ret =  fimc_attach_in_queue(ctrl, b->index);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail: fimc_attach_in_queue\n");
		return -EINVAL;
	}

	dst_base = (dma_addr_t)ctrl->out->fbuf.base;
	if (dst_base) 
		ret = fimc_qbuf_output_dma(ctrl);
	else 
		ret = fimc_qbuf_output_fifo(ctrl);

	return ret;
}

int fimc_dqbuf_output(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int index = -1, ret = -1;

	ret = fimc_detach_out_queue(ctrl, &index);
	if (ret < 0) {
		ret = wait_event_interruptible_timeout(ctrl->wq, \
			(ctrl->out->out_queue[0] != -1), FIMC_DQUEUE_TIMEOUT);
		if (ret == 0) {
			dev_err(ctrl->dev, "[0] out_queue is empty.\n");
			return -EINVAL;
		} else if (ret == -ERESTARTSYS) {
			fimc_print_signal(ctrl);
		} else {
			/* Normal case */
			ret = fimc_detach_out_queue(ctrl, &index);
			if (ret < 0) {
				dev_err(ctrl->dev, "[1] out_queue is empty.\n");
				fimc_dump_context(ctrl);
				return -EINVAL;
			}
		}
	}

	b->index = index;

	dev_info(ctrl->dev, "[%s] dqueued idx = %d\n", __FUNCTION__, b->index);

	return ret;
}

int fimc_g_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	f->fmt.pix = ctrl->out->pix;

	return ret;
}

int fimc_try_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 format = f->fmt.pix.pixelformat;
	int ret = 0;

	dev_info(ctrl->dev, "[%s] called. width(%d), height(%d)\n", \
			__FUNCTION__, f->fmt.pix.width, f->fmt.pix.height);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	/* Check pixel format */
	if ((format != V4L2_PIX_FMT_NV12) && (format != V4L2_PIX_FMT_RGB32) \
		&& (format != V4L2_PIX_FMT_RGB565)) {
		dev_warn(ctrl->dev, "Supported format : V4L2_PIX_FMT_NV12 and \
				V4L2_PIX_FMT_RGB32 and V4L2_PIX_FMT_RGB565.\n");
		dev_warn(ctrl->dev, "Changed format : V4L2_PIX_FMT_NV12.\n");
		f->fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		ret = -EINVAL;
	}

	if (format == V4L2_PIX_FMT_NV12) {
		if (f->fmt.pix.width > FIMC_SRC_MAX_W) {
			dev_warn(ctrl->dev, "The width is changed %d -> %d.\n", \
				f->fmt.pix.width, FIMC_SRC_MAX_W);
			f->fmt.pix.width = FIMC_SRC_MAX_W;
		}
	} else if ((format == V4L2_PIX_FMT_RGB32) || (format == V4L2_PIX_FMT_RGB565)) {
		/* fall through : We cannot check max size. */
		/* Because rotation will be called after VIDIOC_S_FMT. */
	}


	/* Fill the return value. */
	if (format == V4L2_PIX_FMT_NV12) {
		f->fmt.pix.bytesperline	= (f->fmt.pix.width * 3)>>1;
	} else if (format == V4L2_PIX_FMT_RGB32) {
		f->fmt.pix.bytesperline	= f->fmt.pix.width<<2;
	} else if (format == V4L2_PIX_FMT_RGB565) {
		f->fmt.pix.bytesperline	= f->fmt.pix.width<<1;
	} else {
		/* dummy value*/
		f->fmt.pix.bytesperline	= f->fmt.pix.width;
	}

	f->fmt.pix.sizeimage	= f->fmt.pix.bytesperline * f->fmt.pix.height;
	f->fmt.pix.colorspace	= V4L2_COLORSPACE_SMPTE170M;

	return ret;
}

int fimc_s_fmt_vid_out(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	/* Check stream status */
	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	ret = fimc_try_fmt_vid_out(filp, fh, f);
	if (ret < 0)
		return ret;

	ctrl->out->pix = f->fmt.pix;

	return ret;
}

