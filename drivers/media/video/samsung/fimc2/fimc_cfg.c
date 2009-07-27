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

u32 fimc_mapping_rot_flip(u32 rot, u32 flip)
{
	u32 ret = 0;

	switch (rot) {
	case 0:
		if(flip & V4L2_CID_HFLIP)
			ret |= FIMC_XFLIP;

		if(flip & V4L2_CID_VFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 90:
		ret = FIMC_ROT;
		if(flip & V4L2_CID_HFLIP)
			ret |= FIMC_XFLIP;

		if(flip & V4L2_CID_VFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 180:
		ret = (FIMC_XFLIP | FIMC_YFLIP);
		if(flip & V4L2_CID_HFLIP)
			ret &= ~FIMC_XFLIP;

		if(flip & V4L2_CID_VFLIP)
			ret &= ~FIMC_YFLIP;
		break;

	case 270:
		ret = (FIMC_XFLIP | FIMC_YFLIP | FIMC_ROT);
		if(flip & V4L2_CID_HFLIP)
			ret &= ~FIMC_XFLIP;

		if(flip & V4L2_CID_VFLIP)
			ret &= ~FIMC_YFLIP;
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
