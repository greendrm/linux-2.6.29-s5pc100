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
#include <linux/fb.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <plat/media.h>
#include <plat/fimc2.h>

#include "fimc.h"

dma_addr_t fimc_dma_alloc(struct fimc_control *ctrl, u32 bytes)
{
	dma_addr_t end, addr, *curr;

	mutex_lock(&ctrl->lock);

	end = ctrl->mem.base + ctrl->mem.size;
	curr = &ctrl->mem.curr;

	if (*curr + bytes > end) {
		addr = 0;
	} else {
		addr = *curr;
		*curr += bytes;
	}

	mutex_unlock(&ctrl->lock);

	return addr;
}

void fimc_dma_free(struct fimc_control *ctrl, u32 bytes)
{
	mutex_lock(&ctrl->lock);
	ctrl->mem.curr -= bytes;
	mutex_unlock(&ctrl->lock);
}

void fimc_set_active_camera(struct fimc_control *ctrl, enum fimc_cam_index id)
{
	ctrl->cam = fimc_dev->camera[id];

	dev_info(ctrl->dev, "requested id: %d\n", id);
	
	if (ctrl->cam && id < FIMC_TPID)
		fimc_select_camera(ctrl);
}

int fimc_init_camera(struct fimc_control *ctrl)
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

	/*
	 * ctrl->cam may not be null if already s_input called,
	 * otherwise, that should be default_cam if ctrl->cam is null.
	*/
	if (!ctrl->cam)
		ctrl->cam = fimc->camera[pdata->default_cam];

	clk_set_rate(ctrl->cam->clk, ctrl->cam->clk_rate);
	clk_enable(ctrl->cam->clk);

	if (ctrl->cam->cam_power)
		ctrl->cam->cam_power(1);

	/* subdev call for init */
	ret = v4l2_subdev_call(ctrl->cam->sd, core, init, 0);
	if (ret == -ENOIOCTLCMD) {
		dev_err(ctrl->dev, "%s: s_config subdev api not supported\n",
			__FUNCTION__);
		return ret;
	}

	ctrl->cam->initialized = 1;
	fimc_set_active_camera(ctrl, ctrl->cam->id);

	return 0;
}

int fimc_set_rot_degree(struct fimc_control *ctrl, int degree)
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
		dev_err(ctrl->dev, "[%s] Invalid pixelformt : %d\n", 
				__FUNCTION__, pixfmt);
		ret = -EINVAL;
	}

	if (total_size > ctrl->mem.size) {
		dev_err(ctrl->dev, "Reserved memory is not sufficient.\n");
		ret = -EINVAL;
	}

	return ret;
}

static void fimc_set_buff_addr(struct fimc_control *ctrl, u32 buf_size)
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

	fimc_set_buff_addr(ctrl, total_size);
	ctrl->out->is_requested	= 0;

	return 0;
}

int fimc_check_param(struct fimc_control *ctrl)
{
	struct v4l2_rect src_rect, dst_rect, fimd_rect;
	int	ret = 0;

	/* check flip */
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
	int ret = 0;

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running.\n");
		return -EBUSY;
	}

	fimc_set_int_enable(ctrl, 1);

	ret = fimc_set_format(ctrl);
	if (ret < 0)
		return -EINVAL;

	ret = fimc_set_path(ctrl);
	if (ret < 0)
		return -EINVAL;

	ret = fimc_set_rot(ctrl);
	if (ret < 0)
		return -EINVAL;

	ret = fimc_set_src_crop(ctrl);
	if (ret < 0)
		return -EINVAL;

	ret = fimc_set_dst_crop(ctrl);
	if (ret < 0)
		return -EINVAL;

	ret = fimc_set_scaler(ctrl);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

int fimc_init_in_queue(struct fimc_control *ctrl)
{
	unsigned long	spin_flags;
	unsigned int	i;

	spin_lock_irqsave(&ctrl->lock_in, spin_flags);

	/* Init incoming queue */
	for (i = 0; i < FIMC_OUTBUFS; i++) {
		ctrl->out->in_queue[i] = -1;
	}

	spin_unlock_irqrestore(&ctrl->lock_in, spin_flags);
	
	return 0;
}

int fimc_init_out_queue(struct fimc_control *ctrl)
{
	unsigned long	spin_flags;
	unsigned int	i;

	spin_lock_irqsave(&ctrl->lock_out, spin_flags);

	/* Init incoming queue */
	for (i = 0; i < FIMC_OUTBUFS; i++) {
		ctrl->out->out_queue[i] = -1;
	}

	spin_unlock_irqrestore(&ctrl->lock_out, spin_flags);
	
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

	switch (rot) {
	case 0:
		if(flip & V4L2_CID_HFLIP)
			ret |= 0x1;

		if(flip & V4L2_CID_VFLIP)
			ret |= 0x2;
		break;

	case 90:
		if(flip & V4L2_CID_HFLIP)
			ret |= 0x1;

		if(flip & V4L2_CID_VFLIP)
			ret |= 0x2;
		break;

	case 180:
		ret = 0x3;
		if(flip & V4L2_CID_HFLIP)
			ret &= ~0x1;

		if(flip & V4L2_CID_VFLIP)
			ret &= ~0x2;
		break;

	case 270:
		ret = 0x13;
		if(flip & V4L2_CID_HFLIP)
			ret &= ~0x1;

		if(flip & V4L2_CID_VFLIP)
			ret &= ~0x2;
		break;
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
			&ctrl->sc.pre_hratio, &ctrl->sc.hfactor);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail : fimc_get_scaler_factor(width).\n");
		return -EINVAL;
	}

	ret = fimc_get_scaler_factor(src.height, dst.height, \
			&ctrl->sc.pre_vratio, &ctrl->sc.vfactor);
	if (ret < 0) {
		dev_err(ctrl->dev, "Fail : fimc_get_scaler_factor(height).\n");
		return -EINVAL;
	}

	ctrl->sc.pre_dst_width = src.width / ctrl->sc.pre_hratio;
	ctrl->sc.main_hratio = (src.width << 8) / (dst.width<<ctrl->sc.hfactor);

	ctrl->sc.pre_dst_height = src.height / ctrl->sc.pre_vratio;
	ctrl->sc.main_vratio = (src.height << 8) / (dst.height<<ctrl->sc.vfactor);

	if ((src.width == dst.width) && (src.height == dst.height))
		ctrl->sc.bypass = 1;

	ctrl->sc.scaleup_h = (src.width >= dst.width) ? 1 : 0;
	ctrl->sc.scaleup_v = (src.height >= dst.height) ? 1 : 0;

	ctrl->sc.shfactor = 10 - (ctrl->sc.hfactor + ctrl->sc.vfactor);

	fimc_set_prescaler(ctrl);
	fimc_set_mainscaler(ctrl);

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

int fimc_start_camif(void *param)
{
	struct fimc_control *ctrl = (struct fimc_control *)param;
	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		fimc_start_scaler(ctrl);
		fimc_enable_capture(ctrl);
		fimc_enable_input_dma(ctrl);
	} else if (ctrl->cap != NULL) {

	} else {
		dev_err(ctrl->dev, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	return 0;
}

int fimc_stop_camif(struct fimc_control *ctrl)
{
	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		fimc_disable_input_dma(ctrl);		
		fimc_stop_scaler(ctrl);
		fimc_disable_capture(ctrl);
	} else if (ctrl->cap != NULL) {

	} else {
		dev_err(ctrl->dev, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	return 0;
}

static int fimc_stop_fifo(struct fimc_control *ctrl)
{
	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);


	return 0;
}

int fimc_stop_streaming(struct fimc_control *ctrl)
{
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out->fbuf.base != 0) {	/* DMA OUT */
		ret = wait_event_interruptible_timeout(ctrl->wq, \
				(ctrl->status == FIMC_STREAMON_IDLE), \
				FIMC_ONESHOT_TIMEOUT);
		if (ret == 0) {
			dev_err(ctrl->dev, "Fail : %s\n", __FUNCTION__);
		} else if (ret == -ERESTARTSYS) {
			fimc_print_signal(ctrl);
		}
		
		fimc_stop_camif(ctrl);
	} else {				/* FIMD FIFO */
		fimc_stop_fifo(ctrl);
	}

	return ret;
}

void fimc_dump_context(struct fimc_control *ctrl)
{
	u32 i = 0;

	for (i = 0; i < FIMC_OUTBUFS; i++) {
		dev_err(ctrl->dev, "in_queue[%d] : %d\n", i, \
				ctrl->out->in_queue[i]);
	}

	for (i = 0; i < FIMC_OUTBUFS; i++) {
		dev_err(ctrl->dev, "out_queue[%d] : %d\n", i, \
				ctrl->out->out_queue[i]);
	}
	
	dev_err(ctrl->dev, "state : prev = %d, active = %d, next = %d\n", \
		ctrl->out->idx.prev, ctrl->out->idx.active, ctrl->out->idx.next);
}

void fimc_print_signal(struct fimc_control *ctrl)
{
	if (signal_pending(current)) {
		dev_dbg(ctrl->dev, ".pend=%.8lx shpend=%.8lx\n",
			current->pending.signal.sig[0], 
			current->signal->shared_pending.signal.sig[0]);
	} else {
		dev_dbg(ctrl->dev, ":pend=%.8lx shpend=%.8lx\n",
			current->pending.signal.sig[0], 
			current->signal->shared_pending.signal.sig[0]);
	}
}

static 
int fimc_fimd_rect(const struct fimc_control *ctrl, struct v4l2_rect *fimd_rect)
{
	switch (ctrl->out->rotate) {
	case 0:
		fimd_rect->left		= ctrl->out->win.w.left;
		fimd_rect->top		= ctrl->out->win.w.top;
		fimd_rect->width	= ctrl->out->win.w.width;
		fimd_rect->height	= ctrl->out->win.w.height;

		break;

	case 90:
		fimd_rect->left		= ctrl->fb.lcd->width - 
					(ctrl->out->win.w.top \
						+ ctrl->out->win.w.height);
		fimd_rect->top		= ctrl->out->win.w.left;
		fimd_rect->width	= ctrl->out->win.w.height;
		fimd_rect->height	= ctrl->out->win.w.width;

		break;

	case 180:
		fimd_rect->left		= ctrl->fb.lcd->width - 
					(ctrl->out->win.w.left \
						+ ctrl->out->win.w.width);
		fimd_rect->top		= ctrl->fb.lcd->height - 
					(ctrl->out->win.w.top \
						+ ctrl->out->win.w.height);
		fimd_rect->width	= ctrl->out->win.w.width;
		fimd_rect->height	= ctrl->out->win.w.height;

		break;

	case 270:
		fimd_rect->left		= ctrl->out->win.w.top;
		fimd_rect->top		= ctrl->fb.lcd->height - 
					(ctrl->out->win.w.left \
						+ ctrl->out->win.w.width);
		fimd_rect->width	= ctrl->out->win.w.height;
		fimd_rect->height	= ctrl->out->win.w.width;

		break;

	default:
		dev_err(ctrl->dev, "Rotation degree is inavlid.\n");
		return -EINVAL;

		break;
	}

	return 0;
}

int fimc_start_fifo(struct fimc_control *ctrl)
{
	struct v4l2_rect		fimd_rect;
	struct fb_var_screeninfo	var;	
	struct s3cfb_user_window	window;
	int ret = -1;
	u32 id = ctrl->id;

	memset(&fimd_rect, 0, sizeof(struct v4l2_rect));

	ret = fimc_fimd_rect(ctrl, &fimd_rect);
	if (ret < 0) {
		dev_err(ctrl->dev, "fimc_fimd_rect fail\n");
		return -EINVAL;
	}

	/* Get WIN var_screeninfo  */
	ret = s3cfb_direct_ioctl(id, FBIOGET_VSCREENINFO, (unsigned long)&var);
	if (ret < 0) {
		dev_err(ctrl->dev, "direct_ioctl(FBIOGET_VSCREENINFO) fail\n");
		return -EINVAL;
	}

	/* Don't allocate the memory. */
	ret = s3cfb_direct_ioctl(id, FBIO_ALLOC, 0);
	if (ret < 0) {
		dev_err(ctrl->dev, "direct_ioctl(FBIO_ALLOC) fail\n");
		return -EINVAL;
	}

	/* Update WIN size  */
	var.xres = fimd_rect.width;
	var.yres = fimd_rect.height;
	ret = s3cfb_direct_ioctl(id, FBIOPUT_VSCREENINFO, (unsigned long)&var);
	if (ret < 0) {
		dev_err(ctrl->dev, "direct_ioctl(FBIOPUT_VSCREENINFO) fail\n");
		return -EINVAL;
	}

	/* Update WIN position */
	window.x = fimd_rect.left;
	window.y = fimd_rect.top;
	ret = s3cfb_direct_ioctl(id, S3CFB_WIN_POSITION, (unsigned long)&window);
	if (ret < 0) {
		dev_err(ctrl->dev, "direct_ioctl(S3CFB_WIN_POSITION) fail\n");
		return -EINVAL;
	}

	/* Open WIN FIFO */
	ret = ctrl->fb.open_fifo(id, 0, fimc_start_camif, (void *)ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "FIMD FIFO close fail\n");
		return -EINVAL;
	}

	return 0;
}

