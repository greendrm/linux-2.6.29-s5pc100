/* linux/drivers/media/video/samsung/fimc_cfg.c
 *
 * Configuration support file for Samsung Camera Interface (FIMC) driver
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
#include <asm/io.h>
#include <asm/uaccess.h>
#include <plat/media.h>

#include "fimc.h"

int fimc_mapping_rot(struct fimc_control *ctrl, int degree)
{
	switch (degree) {
	case 0:		/* fall through */
	case 90:	/* fall through */
	case 180:	/* fall through */
	case 270:
		ctrl->out->rotate = degree;
		break;

	default:
		dev_err(ctrl->dev, "Invalid rotate value : %d.", degree);
		return -EINVAL;
	}

	return 0;
}

int fimc_check_out_buf(struct fimc_control *ctrl, u32 num)
{
	struct s3cfb_lcd *lcd = &ctrl->fb;
	u32	pixfmt = ctrl->out->pix.pixelformat;
	u32	y_size, cbcr_size, rgb_size, total_size = 0;
	int 	ret = 0;

	if (pixfmt == V4L2_PIX_FMT_NV12) {
		y_size		= FIMC_SRC_MAX_W * FIMC_SRC_MAX_H;
		cbcr_size	= (y_size>>2);
		total_size	= PAGE_ALIGN(y_size + (cbcr_size<<1)) * num;
	} else if (pixfmt == V4L2_PIX_FMT_RGB32) {
		rgb_size	= PAGE_ALIGN(lcd->width * lcd->height * 4);
		total_size	= rgb_size * num;
	} else if (pixfmt == V4L2_PIX_FMT_RGB565) {
		rgb_size	= PAGE_ALIGN(lcd->width * lcd->height * 2);
		total_size	= rgb_size * num;

	} else {
		dev_err(ctrl->dev, "[%s]Invalid input(%d)\n", __FUNCTION__, num);
		ret = -1;
	}

	if (total_size > ctrl->mem.len)
		ret = -1;

	return ret;
}

static void fimc_set_out_addr(struct fimc_control *ctrl, u32 buf_size)
{
	u32 base = ctrl->mem.base;	
	u32 i;

	for (i = 0; i < FIMC_OUTBUFS; i++) {
		ctrl->out->buf[i].base		= base + buf_size * i;
		ctrl->out->buf[i].length	= buf_size;
		ctrl->out->buf[i].state		= VIDEOBUF_IDLE;
		ctrl->out->buf[i].flags		= 0x0;

		ctrl->out->in_queue[i]		= -1;
		ctrl->out->out_queue[i]		= -1;
	}
}

int fimc_init_out_buf(struct fimc_control *ctrl)
{
	struct s3cfb_lcd *lcd = &ctrl->fb;
	u32 pixfmt = ctrl->out->pix.pixelformat;
	u32 total_size, y_size, cb_size;

	if (pixfmt == V4L2_PIX_FMT_NV12) {
		y_size		= FIMC_SRC_MAX_W * FIMC_SRC_MAX_H;
		cb_size		= ((FIMC_SRC_MAX_W * FIMC_SRC_MAX_H)>>2);
		total_size	= PAGE_ALIGN(y_size + (cb_size<<1));
	} else if (pixfmt == V4L2_PIX_FMT_RGB32) {
		total_size	= PAGE_ALIGN((lcd->width * lcd->height)<<2);
	} else if (pixfmt == V4L2_PIX_FMT_RGB565) {
		total_size	= PAGE_ALIGN((lcd->width * lcd->height)<<1);
	} else {
		dev_err(ctrl->dev, "[%s]Invalid input(%d)\n", __FUNCTION__, pixfmt);
		return -EINVAL;
	}

	fimc_set_out_addr(ctrl, total_size);
	ctrl->out->is_requested	= 0;

	return 0;
}

#if 0
int s3c_fimc_attach_in_queue(struct FIMC_control *ctrl, u32 index)
{
	unsigned long		spin_flags;
	int			swap_queue[FIMC_OUTBUFS];
	int			i;

	fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->spin.lock_in, spin_flags);

	/* Backup original queue */
	for (i = 0; i < FIMC_OUTBUFS; i++) {
		swap_queue[i] = ctrl->incoming_queue[i];
	}

	/* Attach new index */
	ctrl->incoming_queue[0]		= index;
	ctrl->out->buf[index].buf_state	= BUF_QUEUED;
	ctrl->out->buf[index].buf_flag	= V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_QUEUED;

	/* Shift the origonal queue */
	for (i = 1; i < FIMC_OUTBUFS; i++) {
		ctrl->incoming_queue[i] = swap_queue[i-1];
	}

	spin_unlock_irqrestore(&ctrl->spin.lock_in, spin_flags);
	
	return 0;
}

int s3c_fimc_detach_in_queue(struct FIMC_control *ctrl, int *index)
{
	unsigned long		spin_flags;
	int			i, ret = 0;

	spin_lock_irqsave(&ctrl->spin.lock_in, spin_flags);

	/* Find last valid index in incoming queue. */
	for (i = (FIMC_OUTBUFS-1); i >= 0; i--) {
		if (ctrl->incoming_queue[i] != -1) {
			*index					= ctrl->incoming_queue[i];
			ctrl->incoming_queue[i]			= -1;
			ctrl->out->buf[*index].buf_state	= BUF_RUNNING;
			ctrl->out->buf[*index].buf_flag		= V4L2_BUF_FLAG_MAPPED;
			break;
		}
	}

	/* incoming queue is empty. */
	if (i < 0)
		ret = -1;
	else
		fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, *index);

	spin_unlock_irqrestore(&ctrl->spin.lock_in, spin_flags);
	
	return ret;
}

int s3c_fimc_attach_out_queue(struct FIMC_control *ctrl, u32 index)
{
	unsigned long		spin_flags;
	int			swap_queue[FIMC_OUTBUFS];
	int			i;

	fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->spin.lock_out, spin_flags);

	/* Backup original queue */
	for (i = 0; i < FIMC_OUTBUFS; i++) {
		swap_queue[i] = ctrl->outgoing_queue[i];
	}

	/* Attach new index */
	ctrl->outgoing_queue[0]		= index;
	ctrl->out->buf[index].buf_state	= BUF_DONE;
	ctrl->out->buf[index].buf_flag	= V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_DONE;

	/* Shift the origonal queue */
	for (i = 1; i < FIMC_OUTBUFS; i++) {
		ctrl->outgoing_queue[i] = swap_queue[i-1];
	}

	spin_unlock_irqrestore(&ctrl->spin.lock_out, spin_flags);

	return 0;
}

int s3c_fimc_detach_out_queue(struct FIMC_control *ctrl, int *index)
{
	unsigned long		spin_flags;
	int			i, ret = 0;

	spin_lock_irqsave(&ctrl->spin.lock_out, spin_flags);

	/* Find last valid index in outgoing queue. */
	for (i = (FIMC_OUTBUFS-1); i >= 0; i--) {
		if (ctrl->outgoing_queue[i] != -1) {
			*index					= ctrl->outgoing_queue[i];
			ctrl->outgoing_queue[i]			= -1;
			ctrl->out->buf[*index].buf_state	= BUF_DQUEUED;
			ctrl->out->buf[*index].buf_flag		= V4L2_BUF_FLAG_MAPPED;
			break;
		}
	}

	/* outgoing queue is empty. */
	if (i < 0)
		ret = -1;
	else
		fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, *index);
		

	spin_unlock_irqrestore(&ctrl->spin.lock_out, spin_flags);
	
	return ret;
}
#endif
