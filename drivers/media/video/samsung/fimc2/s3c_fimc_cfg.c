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

#include "s3c_fimc.h"

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
		fimc_err(ctrl->log, "Invalid rotate value : %d.", degree);
		return -EINVAL;
	}

	return 0;
}

int s3c_fimc_check_out_buf(struct s3c_rp_control *ctrl, unsigned int num)
{
	int 		ret = 0;
	unsigned int	y_buf_size, cbcr_buf_size, rgb_buf_size, total_size = 0;

	if (ctrl->v4l2.video_out_fmt.pixelformat == V4L2_PIX_FMT_YUV420) {
		y_buf_size	= PAGE_ALIGN(S3C_RP_YUV_SRC_MAX_WIDTH * S3C_RP_YUV_SRC_MAX_HEIGHT);
		cbcr_buf_size	= PAGE_ALIGN((S3C_RP_YUV_SRC_MAX_WIDTH * S3C_RP_YUV_SRC_MAX_HEIGHT)>>2);
		total_size	= (y_buf_size + (cbcr_buf_size<<1)) * num;
	} else if (ctrl->v4l2.video_out_fmt.pixelformat == V4L2_PIX_FMT_RGB32) {
		rgb_buf_size	= PAGE_ALIGN(ctrl->fimd.h_res * ctrl->fimd.v_res * 4);
		total_size	= rgb_buf_size * num;
	} else {
		rp_err(ctrl->log_level, "[%s : %d]Invalid input\n", __FUNCTION__, __LINE__);
		ret = -1;
	}

	if (total_size > RP_RESERVED_MEM_SIZE)
		ret = -1;

	return ret;
}

int s3c_fimc_init_out_buf(struct s3c_rp_control *ctrl)
{
	unsigned int ycbcr_buf_size, y_buf_size, cb_buf_size, rgb_buf_size;
	unsigned int i;

	ctrl->buf_info.reserved_mem_start = RP_RESERVED_MEM_ADDR_PHY;

	if (ctrl->v4l2.video_out_fmt.pixelformat == V4L2_PIX_FMT_YUV420) {	/* 720 x 576 YUV420 */
		y_buf_size	= PAGE_ALIGN(S3C_RP_YUV_SRC_MAX_WIDTH * S3C_RP_YUV_SRC_MAX_HEIGHT);
		cb_buf_size	= PAGE_ALIGN((S3C_RP_YUV_SRC_MAX_WIDTH * S3C_RP_YUV_SRC_MAX_HEIGHT)>>2);
		ycbcr_buf_size	= y_buf_size + cb_buf_size *2;		

		for (i = 0; i < S3C_RP_BUFF_NUM; i++) {
			/* Initialize User Buffer */		
			ctrl->user_buf[i].buf_addr.phys_y	= ctrl->buf_info.reserved_mem_start	+ ycbcr_buf_size * i;
			ctrl->user_buf[i].buf_addr.phys_cb	= ctrl->user_buf[i].buf_addr.phys_y	+ y_buf_size;
			ctrl->user_buf[i].buf_addr.phys_cr	= ctrl->user_buf[i].buf_addr.phys_cb	+ cb_buf_size;
			ctrl->user_buf[i].buf_state		= BUF_DONE;
			ctrl->user_buf[i].buf_flag		= 0x0;
			ctrl->user_buf[i].buf_length		= ycbcr_buf_size;

			/* Initialize Driver Buffer which means the rotator output buffer */
			ctrl->driver_buf[i].buf_addr.phys_y	= ctrl->buf_info.reserved_mem_start 	+ ycbcr_buf_size * S3C_RP_BUFF_NUM + ycbcr_buf_size * i;
			ctrl->driver_buf[i].buf_addr.phys_cb	= ctrl->driver_buf[i].buf_addr.phys_y	+ y_buf_size;
			ctrl->driver_buf[i].buf_addr.phys_cr	= ctrl->driver_buf[i].buf_addr.phys_cb	+ cb_buf_size;
			ctrl->driver_buf[i].buf_state		= BUF_DONE;
			ctrl->driver_buf[i].buf_flag		= 0x0;
			ctrl->driver_buf[i].buf_length		= ycbcr_buf_size;

			ctrl->incoming_queue[i]			= -1;
			ctrl->inside_queue[i]			= -1;
			ctrl->outgoing_queue[i]			= -1;
		}
	} else {
		rgb_buf_size = PAGE_ALIGN((ctrl->fimd.h_res * ctrl->fimd.v_res)<<2);

		for (i = 0; i < S3C_RP_BUFF_NUM; i++) {
			/* Initialize User Buffer */
			ctrl->user_buf[i].buf_addr.phys_rgb	= ctrl->buf_info.reserved_mem_start	+ rgb_buf_size * i;
			ctrl->user_buf[i].buf_state		= BUF_DONE;
			ctrl->user_buf[i].buf_flag		= 0x0;
			ctrl->user_buf[i].buf_length		= rgb_buf_size;

			/* Initialize Driver Buffer which means the rotator output buffer */
			ctrl->driver_buf[i].buf_addr.phys_rgb	= ctrl->buf_info.reserved_mem_start	+ rgb_buf_size * S3C_RP_BUFF_NUM + rgb_buf_size * i;
			ctrl->driver_buf[i].buf_state		= BUF_DONE;
			ctrl->driver_buf[i].buf_flag		= 0x0;
			ctrl->driver_buf[i].buf_length		= rgb_buf_size;

			ctrl->incoming_queue[i]			= -1;
			ctrl->inside_queue[i]			= -1;
			ctrl->outgoing_queue[i]			= -1;
		}
	}

	ctrl->buf_info.requested	= FALSE;

	return 0;
}

int s3c_fimc_attach_in_queue(struct s3c_rp_control *ctrl, unsigned int index)
{
	unsigned long		spin_flags;
	int			swap_queue[S3C_RP_BUFF_NUM];
	int			i;

	fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->spin.lock_in, spin_flags);

	/* Backup original queue */
	for (i = 0; i < S3C_RP_BUFF_NUM; i++) {
		swap_queue[i] = ctrl->incoming_queue[i];
	}

	/* Attach new index */
	ctrl->incoming_queue[0]		= index;
	ctrl->user_buf[index].buf_state	= BUF_QUEUED;
	ctrl->user_buf[index].buf_flag	= V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_QUEUED;

	/* Shift the origonal queue */
	for (i = 1; i < S3C_RP_BUFF_NUM; i++) {
		ctrl->incoming_queue[i] = swap_queue[i-1];
	}

	spin_unlock_irqrestore(&ctrl->spin.lock_in, spin_flags);
	
	return 0;
}

int s3c_fimc_detach_in_queue(struct s3c_rp_control *ctrl, int *index)
{
	unsigned long		spin_flags;
	int			i, ret = 0;

	spin_lock_irqsave(&ctrl->spin.lock_in, spin_flags);

	/* Find last valid index in incoming queue. */
	for (i = (S3C_RP_BUFF_NUM-1); i >= 0; i--) {
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

int s3c_fimc_attach_out_queue(struct s3c_rp_control *ctrl, unsigned int index)
{
	unsigned long		spin_flags;
	int			swap_queue[S3C_RP_BUFF_NUM];
	int			i;

	fimc_dbg(ctrl->log_level, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->spin.lock_out, spin_flags);

	/* Backup original queue */
	for (i = 0; i < S3C_RP_BUFF_NUM; i++) {
		swap_queue[i] = ctrl->outgoing_queue[i];
	}

	/* Attach new index */
	ctrl->outgoing_queue[0]		= index;
	ctrl->user_buf[index].buf_state	= BUF_DONE;
	ctrl->user_buf[index].buf_flag	= V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_DONE;

	/* Shift the origonal queue */
	for (i = 1; i < S3C_RP_BUFF_NUM; i++) {
		ctrl->outgoing_queue[i] = swap_queue[i-1];
	}

	spin_unlock_irqrestore(&ctrl->spin.lock_out, spin_flags);

	return 0;
}

int s3c_fimc_detach_out_queue(struct s3c_rp_control *ctrl, int *index)
{
	unsigned long		spin_flags;
	int			i, ret = 0;

	spin_lock_irqsave(&ctrl->spin.lock_out, spin_flags);

	/* Find last valid index in outgoing queue. */
	for (i = (S3C_RP_BUFF_NUM-1); i >= 0; i--) {
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

