/* linux/drivers/media/video/samsung/s3c_fimc.h
 *
 * Header file for Samsung Camera Interface (FIMC) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3C_FIMC_H
#define _S3C_FIMC_H

#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <plat/media.h>
#include <plat/fimc.h>
#endif

#define S3C_FIMC_NAME	"s3c-fimc"

/* debug macro */
#define FIMC_LOG_DEFAULT        FIMC_LOG_WARN

#define FIMC_DEBUG(num, fmt, ...) \
	do { \
		if (num <= FIMC_LOG_DEBUG) \
			printk(KERN_DEBUG S3C_FIMC_NAME ": " \
				fmt, ##__VA_ARGS__); \
	} while(0)

#define FIMC_INFO(num, fmt, ...) \
	do { \
		if (num <= FIMC_LOG_INFO) \
			printk(KERN_INFO S3C_FIMC_NAME ": " \
				fmt, ##__VA_ARGS__); \
	} while (0)

#define FIMC_WARN(num, fmt, ...) \
	do { \
		if (num <= FIMC_LOG_WARN) \
			printk(KERN_WARNING S3C_FIMC_NAME ": " \
				fmt, ##__VA_ARGS__); \
	} while (0)


#define FIMC_ERROR(num, fmt, ...) \
	do { \
		if (num <= FIMC_LOG_ERR) \
			printk(KERN_ERR S3C_FIMC_NAME ": " \
				fmt, ##__VA_ARGS__); \
	} while (0)

#define fimc_dbg(num, fmt, ...)		FIMC_DEBUG(num, fmt, ##__VA_ARGS__)
#define fimc_info(num, fmt, ...)	FIMC_INFO(num, fmt, ##__VA_ARGS__)
#define fimc_warn(num, fmt, ...)	FIMC_WARN(num, fmt, ##__VA_ARGS__)
#define fimc_err(num, fmt, ...)		FIMC_ERROR(num, fmt, ##__VA_ARGS__)

#endif /* _S3C_FIMC_H */

