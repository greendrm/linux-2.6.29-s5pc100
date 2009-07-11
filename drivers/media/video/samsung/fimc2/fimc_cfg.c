/* linux/drivers/media/video/samsung/s3c_fimc_cfg.c
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

int s3c_fimc_mapping_rot(struct fimc_control *ctrl, int degree)
{
	switch (degree) {
	case 0:		/* fall through */
	case 90:	/* fall through */
	case 180:	/* fall through */
	case 270:
		ctrl->out.rotate = degree;
		break;

	default:
		dev_err(ctrl->dev, "Invalid rotate value : %d.", degree);
		return -EINVAL;
	}

	return 0;
}

int s3c_fimc_check_out_buf(struct fimc_control *ctrl, u32 num)
{
	struct s3cfb_lcd lcd =ctrl->fb;
	u32	pixelformat = ctrl->out->pix.pixelformat;
	u32	y_size, cbcr_size, rgb_size, total_size = 0;
	int 	ret = 0;

	if (pixelformat == V4L2_PIX_FMT_YUV420) {
		y_size	= FIMC_SRC_MAX_W * FIMC_SRC_MAX_H;
		cbcr_size	= (y_size>>2);
		total_size	= PAGE_ALIGN(y_size + (cbcr_size<<1)) * num;
	} else if (pixelformat == V4L2_PIX_FMT_RGB32) {
		rgb_size	= PAGE_ALIGN(lcd->width * lcd->height * 4);
		total_size	= rgb_size * num;
	} else if (pixelformat == V4L2_PIX_FMT_RGB565) {
		rgb_size	= PAGE_ALIGN(lcd->width * lcd->height * 2);
		total_size	= rgb_size * num;

	} else {
		dev_err(ctrl->dev, "[%s]Invalid input(%d)\n", __FUNCTION__, num);
		ret = -1;
	}

	if (total_size > FIMC_RESERVED_MEM_SIZE)
		ret = -1;

	return ret;
}

int s3c_fimc_init_out_buf(struct fimc_control *ctrl)
{
	u32 ycbcr_size, y_size, cb_size, rgb_size;
	u32 i;
#if 0
	ctrl->buf_info.reserved_mem_start = RP_RESERVED_MEM_ADDR_PHY;

	if (ctrl->v4l2.video_out_fmt.pixelformat == V4L2_PIX_FMT_YUV420) {	/* 720 x 576 YUV420 */
		y_size	= PAGE_ALIGN(FIMC_SRC_MAX_W * FIMC_SRC_MAX_H);
		cb_size	= PAGE_ALIGN((FIMC_SRC_MAX_W * FIMC_SRC_MAX_H)>>2);
		ycbcr_size	= y_size + cb_size *2;		

		for (i = 0; i < FIMC_BUFF_NUM; i++) {
			/* Initialize User Buffer */		
			ctrl->user_buf[i].buf_addr.phys_y	= ctrl->buf_info.reserved_mem_start	+ ycbcr_size * i;
			ctrl->user_buf[i].buf_addr.phys_cb	= ctrl->user_buf[i].buf_addr.phys_y	+ y_size;
			ctrl->user_buf[i].buf_addr.phys_cr	= ctrl->user_buf[i].buf_addr.phys_cb	+ cb_size;
			ctrl->user_buf[i].buf_state		= BUF_DONE;
			ctrl->user_buf[i].buf_flag		= 0x0;
			ctrl->user_buf[i].buf_length		= ycbcr_size;

			/* Initialize Driver Buffer which means the rotator output buffer */
			ctrl->driver_buf[i].buf_addr.phys_y	= ctrl->buf_info.reserved_mem_start 	+ ycbcr_size * FIMC_BUFF_NUM + ycbcr_size * i;
			ctrl->driver_buf[i].buf_addr.phys_cb	= ctrl->driver_buf[i].buf_addr.phys_y	+ y_size;
			ctrl->driver_buf[i].buf_addr.phys_cr	= ctrl->driver_buf[i].buf_addr.phys_cb	+ cb_size;
			ctrl->driver_buf[i].buf_state		= BUF_DONE;
			ctrl->driver_buf[i].buf_flag		= 0x0;
			ctrl->driver_buf[i].buf_length		= ycbcr_size;

			ctrl->incoming_queue[i]			= -1;
			ctrl->inside_queue[i]			= -1;
			ctrl->outgoing_queue[i]			= -1;
		}
	} else {
		rgb_size = PAGE_ALIGN((lcd->width * lcd->height)<<2);

		for (i = 0; i < FIMC_BUFF_NUM; i++) {
			/* Initialize User Buffer */
			ctrl->user_buf[i].buf_addr.phys_rgb	= ctrl->buf_info.reserved_mem_start	+ rgb_size * i;
			ctrl->user_buf[i].buf_state		= BUF_DONE;
			ctrl->user_buf[i].buf_flag		= 0x0;
			ctrl->user_buf[i].buf_length		= rgb_size;

			/* Initialize Driver Buffer which means the rotator output buffer */
			ctrl->driver_buf[i].buf_addr.phys_rgb	= ctrl->buf_info.reserved_mem_start	+ rgb_size * FIMC_BUFF_NUM + rgb_size * i;
			ctrl->driver_buf[i].buf_state		= BUF_DONE;
			ctrl->driver_buf[i].buf_flag		= 0x0;
			ctrl->driver_buf[i].buf_length		= rgb_size;

			ctrl->incoming_queue[i]			= -1;
			ctrl->inside_queue[i]			= -1;
			ctrl->outgoing_queue[i]			= -1;
		}
	}

	ctrl->buf_info.requested	= FALSE;
#endif
	return 0;
}

#if 0
int s3c_fimc_attach_in_queue(struct FIMC_control *ctrl, u32 index)
{
	unsigned long		spin_flags;
	int			swap_queue[FIMC_BUFF_NUM];
	int			i;

	fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->spin.lock_in, spin_flags);

	/* Backup original queue */
	for (i = 0; i < FIMC_BUFF_NUM; i++) {
		swap_queue[i] = ctrl->incoming_queue[i];
	}

	/* Attach new index */
	ctrl->incoming_queue[0]		= index;
	ctrl->user_buf[index].buf_state	= BUF_QUEUED;
	ctrl->user_buf[index].buf_flag	= V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_QUEUED;

	/* Shift the origonal queue */
	for (i = 1; i < FIMC_BUFF_NUM; i++) {
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
	for (i = (FIMC_BUFF_NUM-1); i >= 0; i--) {
		if (ctrl->incoming_queue[i] != -1) {
			*index					= ctrl->incoming_queue[i];
			ctrl->incoming_queue[i]			= -1;
			ctrl->user_buf[*index].buf_state	= BUF_RUNNING;
			ctrl->user_buf[*index].buf_flag		= V4L2_BUF_FLAG_MAPPED;
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
	int			swap_queue[FIMC_BUFF_NUM];
	int			i;

	fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->spin.lock_out, spin_flags);

	/* Backup original queue */
	for (i = 0; i < FIMC_BUFF_NUM; i++) {
		swap_queue[i] = ctrl->outgoing_queue[i];
	}

	/* Attach new index */
	ctrl->outgoing_queue[0]		= index;
	ctrl->user_buf[index].buf_state	= BUF_DONE;
	ctrl->user_buf[index].buf_flag	= V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_DONE;

	/* Shift the origonal queue */
	for (i = 1; i < FIMC_BUFF_NUM; i++) {
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
	for (i = (FIMC_BUFF_NUM-1); i >= 0; i--) {
		if (ctrl->outgoing_queue[i] != -1) {
			*index					= ctrl->outgoing_queue[i];
			ctrl->outgoing_queue[i]			= -1;
			ctrl->user_buf[*index].buf_state	= BUF_DQUEUED;
			ctrl->user_buf[*index].buf_flag		= V4L2_BUF_FLAG_MAPPED;
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
