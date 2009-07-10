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

#ifndef _FIMC2_H
#define _FIMC2_H

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

#define FIMC2_NAME		"s3c-fimc"

#define FIMC_OUT_BUFF_NUM	3

#define FIMC0_RESERVED_MEM_ADDR_PHY		(unsigned int)s3c_get_media_memory(S3C_MDEV_FIMC0)
#define FIMC0_RESERVED_MEM_SIZE			(unsigned int)s3c_get_media_memsize(S3C_MDEV_FIMC0)

#define FIMC1_RESERVED_MEM_ADDR_PHY		(unsigned int)s3c_get_media_memory(S3C_MDEV_FIMC1)
#define FIMC1_RESERVED_MEM_SIZE			(unsigned int)s3c_get_media_memsize(S3C_MDEV_FIMC1)

#define FIMC2_RESERVED_MEM_ADDR_PHY		(unsigned int)s3c_get_media_memory(S3C_MDEV_FIMC2)
#define FIMC2_RESERVED_MEM_SIZE			(unsigned int)s3c_get_media_memsize(S3C_MDEV_FIMC2)

#define FIMC_YUV_SRC_MAX_WIDTH			1920
#define FIMC_YUV_SRC_MAX_HEIGHT			1080

/* debug macro */
#define FIMC_LOG_DEFAULT        FIMC_LOG_WARN

/*
 * E N U M E R A T I O N S
 *
*/
enum fimc_status {
	FIMC_STREAMOFF,
	FIMC_READY_ON,
	FIMC_STREAMON,
	FIMC_READY_OFF,
	FIMC_ON_SLEEP,
	FIMC_OFF_SLEEP,
	FIMC_READY_RESUME,
};

enum fimc_log_level {
	FIMC_LOG_DEBUG	= 0x1,		/* Queue status */
	FIMC_LOG_INFO	= 0x3,		/* V4L2 API */
	FIMC_LOG_WARN	= 0x7,		/* Warning conditions */
	FIMC_LOG_ERR	= 0xF,		/* Error conditions */
};


enum fimc_fifo_state {
	FIFO_CLOSE,
	FIFO_SLEEP,
};

enum fimc_fimd_state {
	FIMD_OFF,
	FIMD_ON,
};

#define FIMC_DEBUG(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_DEBUG) \
			printk(KERN_DEBUG FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while(0)

#define FIMC_INFO(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_INFO) \
			printk(KERN_INFO FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while (0)

#define FIMC_WARN(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_WARN) \
			printk(KERN_WARNING FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while (0)


#define FIMC_ERROR(level, fmt, ...) \
	do { \
		if (level <= FIMC_LOG_ERR) \
			printk(KERN_ERR FIMC2_NAME ": " fmt, ##__VA_ARGS__); \
	} while (0)

#define fimc_dbg(level, fmt, ...)	FIMC_DEBUG(level, fmt, ##__VA_ARGS__)
#define fimc_info(level, fmt, ...)	FIMC_INFO(level, fmt, ##__VA_ARGS__)
#define fimc_warn(level, fmt, ...)	FIMC_WARN(level, fmt, ##__VA_ARGS__)
#define fimc_err(level, fmt, ...)	FIMC_ERROR(level, fmt, ##__VA_ARGS__)

/*
 * E X T E R N S
 *
*/
extern int s3c_fimc_mapping_rot(struct fimc_control *ctrl, int degree);
extern int s3c_fimc_check_out_buf(struct fimc_control *ctrl, unsigned int num);
extern int s3c_fimc_init_out_buf(struct fimc_control *ctrl);

#endif /* _FIMC2_H */

