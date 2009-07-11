/* linux/drivers/media/video/samsung/s3c_fimc_cfg.c
 *
 * V4L2 Overlay device support file for Samsung Camera Interface (FIMC) driver
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

#include "fimc.h"

static int s3c_fimc_try_fmt_overlay(struct file *filp, void *fh,
					  struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

static int s3c_fimc_g_fmt_vid_overlay(struct file *file, void *fh,
					struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

static int s3c_fimc_s_fmt_vid_overlay(struct file *file, void *fh,
					struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

