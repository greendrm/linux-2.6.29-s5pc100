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

u32 fimc_mapping_rot_flip(u32 rot, u32 flip)
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


int fimc_get_scaler_factor(u32 src, u32 tar, u32 *ratio, u32 *shift)
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
