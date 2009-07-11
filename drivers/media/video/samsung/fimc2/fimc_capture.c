/* linux/drivers/media/video/samsung/s3c_fimc_capture.c
 *
 * V4L2 Capture device support file for Samsung Camera Interface (FIMC) driver
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

int fimc_reqbufs_capture(void *fh, struct v4l2_requestbuffers *b)
{
	return 0;
}

int fimc_querybuf_capture(void *fh, struct v4l2_buffer *b)
{
	return 0;
}

int fimc_g_ctrl_capture(void *fh, struct v4l2_control *c)
{
	return 0;
}

int fimc_s_ctrl_capture(void *fh, struct v4l2_control *c)
{
	return 0;
}

int fimc_cropcap_capture(void *fh, struct v4l2_cropcap *a)
{
	return 0;
}

int fimc_s_crop_capture(void *fh, struct v4l2_crop *a)
{
	return 0;
}

int fimc_streamon_capture(void *fh)
{
	return 0;
}

int fimc_streamoff_capture(void *fh)
{
	return 0;
}

int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b)
{
	return 0;
}

int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b)
{
	return 0;
}

