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
#include <linux/mm.h>
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
		dev_err(ctrl->dev, "Invalid rotate value : %d.\n", degree);
		return -EINVAL;
	}

	return 0;
}

int fimc_check_out_buf(struct fimc_control *ctrl, u32 num)
{
	struct s3cfb_lcd *lcd = ctrl->fb.lcd;
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
		dev_err(ctrl->dev, "[%s] Invalid buff num : %d\n", 
				__FUNCTION__, num);
		ret = -EINVAL;
	}

	if (total_size > ctrl->mem.len)
		ret = -EINVAL;

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
	struct s3cfb_lcd *lcd = ctrl->fb.lcd;
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
		dev_err(ctrl->dev, "[%s]Invalid pixelformt : %d\n", 
				__FUNCTION__, pixfmt);
		return -EINVAL;
	}

	fimc_set_out_addr(ctrl, total_size);
	ctrl->out->is_requested	= 0;

	return 0;
}

int fimc_check_param(struct fimc_control *ctrl)
{
	struct v4l2_rect src_rect, dst_rect, fimd_rect;
	int	ret = 0;

	if((ctrl->out->rotate != 90) && (ctrl->out->rotate != 270)) {
		src_rect.width		= ctrl->out->pix.width;
		src_rect.height		= ctrl->out->pix.height; 
		fimd_rect.width		= ctrl->fb.lcd->width;
		fimd_rect.height	= ctrl->fb.lcd->height;
	} else {	/* Landscape mode */
		src_rect.width		= ctrl->out->pix.height;
		src_rect.height		= ctrl->out->pix.width;
		fimd_rect.width		= ctrl->fb.lcd->height;
		fimd_rect.height	= ctrl->fb.lcd->width;
	}

	/* In case of OVERLAY device */
	dst_rect.width	= ctrl->out->win.w.width;
	dst_rect.height	= ctrl->out->win.w.height;
	dst_rect.top	= ctrl->out->win.w.top;
	dst_rect.left	= ctrl->out->win.w.left;
	
	/* To do : OUTPUT device */


	if ((dst_rect.left + dst_rect.width) > fimd_rect.width) {
		dev_err(ctrl->dev, "Horizontal position setting is failed.\n");
		dev_err(ctrl->dev, "\tleft = %d, width = %d, lcd width = %d, \n", 
			dst_rect.left, dst_rect.width, fimd_rect.width);
		ret = -EINVAL;
	} else if ((dst_rect.top + dst_rect.height) > fimd_rect.height) {
		dev_err(ctrl->dev, "Vertical position setting is failed.\n");
		dev_err(ctrl->dev, "\ttop = %d, height = %d, lcd height = %d, \n", 
			dst_rect.top, dst_rect.height, fimd_rect.height);		
		ret = -EINVAL;
	}

	return ret;
}

int fimc_set_param(struct fimc_control *ctrl)
{
#if 0
	int	ret = 0;

	fimc_set_envid(ctrl, FALSE);

	ret = fimc_set_pixelformat(ctrl);
	if (ret < 0) {
		rp_err(ctrl->log_level, "Cannot set the post processor pixelformat.\n");
		return -1;
	}

	ret = fimc_set_scaler(ctrl);
	if (ret < 0) {
		rp_err(ctrl->log_level, "Cannot set the post processor scaler.\n");
		return -1;
	}

	fimc_set_int_enable(ctrl, TRUE);
#endif
	return 0;
}

int fimc_attach_in_queue(struct fimc_control *ctrl, u32 index)
{
	unsigned long		spin_flags;
	int			swap_queue[FIMC_OUTBUFS];
	int			i;

	dev_dbg(ctrl->dev, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->lock_in, spin_flags);

	/* Backup original queue */
	for (i = 0; i < FIMC_OUTBUFS; i++) {
		swap_queue[i] = ctrl->out->in_queue[i];
	}

	/* Attach new index */
	ctrl->out->in_queue[0] = index;
	ctrl->out->buf[index].state = VIDEOBUF_QUEUED;
	ctrl->out->buf[index].flags = V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_QUEUED;

	/* Shift the origonal queue */
	for (i = 1; i < FIMC_OUTBUFS; i++) {
		ctrl->out->in_queue[i] = swap_queue[i-1];
	}

	spin_unlock_irqrestore(&ctrl->lock_in, spin_flags);
	
	return 0;
}

int fimc_detach_in_queue(struct fimc_control *ctrl, int *index)
{
	unsigned long		spin_flags;
	int			i, ret = 0;

	spin_lock_irqsave(&ctrl->lock_in, spin_flags);

	/* Find last valid index in incoming queue. */
	for (i = (FIMC_OUTBUFS-1); i >= 0; i--) {
		if (ctrl->out->in_queue[i] != -1) {
			*index = ctrl->out->in_queue[i];
			ctrl->out->in_queue[i] = -1;
			ctrl->out->buf[*index].state = VIDEOBUF_ACTIVE;
			ctrl->out->buf[*index].flags = V4L2_BUF_FLAG_MAPPED;
			break;
		}
	}

	/* incoming queue is empty. */
	if (i < 0)
		ret = -EINVAL;
	else
		dev_dbg(ctrl->dev, "[%s] index = %d\n", __FUNCTION__, *index);

	spin_unlock_irqrestore(&ctrl->lock_in, spin_flags);
	
	return ret;
}

int fimc_attach_out_queue(struct fimc_control *ctrl, u32 index)
{
	unsigned long		spin_flags;
	int			swap_queue[FIMC_OUTBUFS];
	int			i;

	dev_dbg(ctrl->dev, "[%s] index = %d\n", __FUNCTION__, index);

	spin_lock_irqsave(&ctrl->lock_out, spin_flags);

	/* Backup original queue */
	for (i = 0; i < FIMC_OUTBUFS; i++) {
		swap_queue[i] = ctrl->out->out_queue[i];
	}

	/* Attach new index */
	ctrl->out->out_queue[0]	= index;
	ctrl->out->buf[index].state = VIDEOBUF_DONE;
	ctrl->out->buf[index].flags = V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_DONE;

	/* Shift the origonal queue */
	for (i = 1; i < FIMC_OUTBUFS; i++) {
		ctrl->out->out_queue[i] = swap_queue[i-1];
	}

	spin_unlock_irqrestore(&ctrl->lock_out, spin_flags);

	return 0;
}

int fimc_detach_out_queue(struct fimc_control *ctrl, int *index)
{
	unsigned long		spin_flags;
	int			i, ret = 0;

	spin_lock_irqsave(&ctrl->lock_out, spin_flags);

	/* Find last valid index in outgoing queue. */
	for (i = (FIMC_OUTBUFS-1); i >= 0; i--) {
		if (ctrl->out->out_queue[i] != -1) {
			*index = ctrl->out->out_queue[i];
			ctrl->out->out_queue[i] = -1;
			ctrl->out->buf[*index].state = VIDEOBUF_IDLE;
			ctrl->out->buf[*index].flags = V4L2_BUF_FLAG_MAPPED;
			break;
		}
	}

	/* outgoing queue is empty. */
	if (i < 0) {
		ret = -EINVAL;
		dev_dbg(ctrl->dev, "[%s] outgoing queue : %d, %d, %d\n", 
			__FUNCTION__, ctrl->out->out_queue[0], 
			ctrl->out->out_queue[1], ctrl->out->out_queue[2]);
	} else
		dev_dbg(ctrl->dev, "[%s] index = %d\n", __FUNCTION__, *index);
		

	spin_unlock_irqrestore(&ctrl->lock_out, spin_flags);
	
	return ret;
}

int fimc_mapping_rot_flip(u32 rot, u32 flip)
{
	u32 ret = 0;

	if (rot == 0) {
		if(flip && V4L2_CID_HFLIP)
			ret |= 0x1;
		if(flip && V4L2_CID_VFLIP)
			ret |= 0x2;
	} else if (rot == 90) {
		if(flip && V4L2_CID_HFLIP)
			ret |= 0x1;
		if(flip && V4L2_CID_VFLIP)
			ret |= 0x2;
	} else if (rot == 180) {
		ret = 0x3;
		if(flip && V4L2_CID_HFLIP)
			ret &= ~0x1;
		if(flip && V4L2_CID_VFLIP)
			ret &= ~0x2;
	} else if (rot == 270) {
		ret = 0x13;
		if(flip && V4L2_CID_HFLIP)
			ret &= ~0x1;
		if(flip && V4L2_CID_VFLIP)
			ret &= ~0x2;
	}

	return ret;
}

static int fimc_get_scaler_factor(u32 src, u32 tar, u32 *ratio, u32 *shift)
{
	if (src >= tar * 64) {
		return -EINVAL;
	} else if (src >= tar * 32) {
		*ratio = 32;
		*shift = 5;
	} else if (src >= tar * 16) {
		*ratio = 16;
		*shift = 4;
	} else if (src >= tar * 8) {
		*ratio = 8;
		*shift = 3;
	} else if (src >= tar * 4) {
		*ratio = 4;
		*shift = 2;
	} else if (src >= tar * 2) {
		*ratio = 2;
		*shift = 1;
	} else {
		*ratio = 1;
		*shift = 0;
	}

	return 0;
}

int fimc_set_scaler(struct fimc_control *ctrl)
{
	struct fimc_scaler sc;
	struct v4l2_rect src, dst;
	u32 rot = 0, flip = 0, is_rotate = 0;
	int ret = 0;

	src.width = 0;
	src.height = 0;
	dst.width = 0;
	dst.height = 0;	

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		src.width	= ctrl->out->crop.c.width;
		src.height	= ctrl->out->crop.c.height;

		rot = ctrl->out->rotate;
		flip = ctrl->out->flip;

		is_rotate = fimc_mapping_rot_flip(rot, flip);
		if (is_rotate && 0x10) {	/* Landscape mode */
			if (ctrl->out->fbuf.base != 0) {
				dst.width	= ctrl->out->fbuf.fmt.height;
				dst.height	= ctrl->out->fbuf.fmt.width;
			} else {
				dst.width	= ctrl->out->win.w.height;
				dst.height	= ctrl->out->win.w.width;
			}
		} else {			/* Portrait mode */
			if (ctrl->out->fbuf.base != 0) {
				dst.width	= ctrl->out->fbuf.fmt.width;
				dst.height	= ctrl->out->fbuf.fmt.height;
			} else {
				dst.width	= ctrl->out->win.w.width;
				dst.height	= ctrl->out->win.w.height;
			}
		}
		
	} else if (ctrl->cap != NULL){
		/* To do */
	} else {
		dev_err(ctrl->dev, "[%s] Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	ret = fimc_get_scaler_factor(src.width, dst.width, \
			&sc.pre_hratio, &sc.hfactor);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail : fimc_get_scaler_factor(width).\n");
		return -EINVAL;
	}

	ret = fimc_get_scaler_factor(src.height, dst.height, \
			&sc.pre_vratio, &sc.vfactor);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail : fimc_get_scaler_factor(height).\n");
		return -EINVAL;
	}

	sc.pre_dst_width = src.width / sc.pre_hratio;
	sc.main_hratio = (src.width << 8) / (dst.width<<sc.hfactor);

	sc.pre_dst_height = src.height / sc.pre_vratio;
	sc.main_vratio = (src.height << 8) / (dst.height<<sc.vfactor);

	if ((src.width == dst.width) && (src.height == dst.height))
		sc.bypass = 1;

	sc.scaleup_h = (src.width >= dst.width) ? 1 : 0;
	sc.scaleup_v = (src.height >= dst.height) ? 1 : 0;

	sc.shfactor = 10 - (sc.hfactor + sc.vfactor);

	fimc_set_prescaler(ctrl, &sc);
	fimc_set_mainscaler(ctrl, &sc);

	return 0;
}

int fimc_set_src_crop(struct fimc_control *ctrl)
{
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		ret = fimc_set_src_dma_offset(ctrl);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail : fimc_set_src_dma_offset\n");
			ret = -EINVAL;
		}
		
		fimc_set_src_dma_size(ctrl);
	} else if (ctrl->cap != NULL) {

	} else {
		dev_err(ctrl->dev, "[%s] Invalid case.\n", __FUNCTION__);
		ret = -EINVAL;
	}

	return 0;
}

int fimc_set_dst_crop(struct fimc_control *ctrl)
{
	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		if (ctrl->out->fbuf.base != 0) {	/* DMA OUT */
			fimc_set_dst_dma_offset(ctrl);
		} else {				/* FIMD FIFO */
			/* fall through : See also fimc_fifo_start() */
		}

		fimc_set_dst_dma_size(ctrl);
	} else if (ctrl->cap != NULL) {

	} else {
		dev_err(ctrl->dev, "[%s] Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	return 0;
}

int fimc_set_path(struct fimc_control *ctrl)
{
	int ret = 0;
	u32 inpath = 0, outpath = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		inpath	= FIMC_SRC_MSDMA;

		if (ctrl->out->fbuf.base != NULL) {
			outpath =  FIMC_DST_DMA;
		} else { /* FIFO mode */
			outpath =  FIMC_DST_FIMD;
		}
		
		ret = fimc_set_src_path(ctrl, inpath);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail : fimc_set_src_path()\n");
			return -EINVAL;
		}

		ret = fimc_set_dst_path(ctrl, outpath);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail : fimc_set_dst_path()\n");
			return -EINVAL;
		}

	} else if (ctrl->cap != NULL) {
		/* To do */
		inpath	= FIMC_SRC_CAM;
		outpath = FIMC_DST_DMA;
		fimc_set_src_path(ctrl, inpath);
		fimc_set_dst_path(ctrl, outpath);
	} else {
		dev_err(ctrl->dev, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	return 0;
}

int fimc_set_rot(struct fimc_control *ctrl)
{
	u32 rot	= 0, flip = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		rot = ctrl->out->rotate;
		flip =  ctrl->out->flip;

		if (ctrl->out->fbuf.base != NULL) {
			fimc_set_in_rot(ctrl, rot, flip);
		} else { /* FIFO mode */
			fimc_set_out_rot(ctrl, rot, flip);		
		}
	} else if (ctrl->cap != NULL) {
		/* To do */
	} else {
		dev_err(ctrl->dev, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	return 0;
}

int fimc_set_format(struct fimc_control *ctrl)
{
	u32 pixfmt = 0;
	int ret = -1;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		pixfmt = ctrl->out->pix.pixelformat;		

		ret = fimc_set_src_format(ctrl, pixfmt);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail : fimc_set_src_format()\n");
			return -EINVAL;
		}

		if (ctrl->out->fbuf.base != NULL)
			pixfmt = ctrl->out->fbuf.fmt.pixelformat;
		else /* FIFO mode */
			pixfmt = V4L2_PIX_FMT_RGB32;

		ret = fimc_set_dst_format(ctrl, pixfmt);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail : fimc_set_dst_format()\n");
			return -EINVAL;
		}
	} else if (ctrl->cap != NULL) {
		pixfmt = ctrl->cap->fmt.pixelformat;
		/* To do : Capture device. */

	} else {
		dev_err(ctrl->dev, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}


	return 0;
}
