/* linux/arch/arm/plat-s5pc11x/setup-mfc.c
 *
 * Copyright 2010 Samsung Electronics
 *	Changhwan Youn <chaos.youn@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5PC11X MFC configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <plat/media.h>
#include <plat/mfc.h>

struct s3c_mfc_platdata mfc_platdata __initdata;

void __init s3c_mfc_get_reserve_memory_info(void)
{
	mfc_platdata.buf_phy_base[0] = s3c_get_media_memory_node(S3C_MDEV_MFC, 0);
	mfc_platdata.buf_phy_base[1] = s3c_get_media_memory_node(S3C_MDEV_MFC, 1);
	mfc_platdata.buf_size[0] = s3c_get_media_memsize_node(S3C_MDEV_MFC, 0);
	mfc_platdata.buf_size[1] = s3c_get_media_memsize_node(S3C_MDEV_MFC, 1);
	s3c_mfc_set_platdata(&mfc_platdata);
}
