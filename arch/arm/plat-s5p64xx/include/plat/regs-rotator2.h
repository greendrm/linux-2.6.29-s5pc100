/* linux/include/asm-arm/arch-s3c2410/regs-roator.h
 *
 * Copyright (c) 2004 Samsung Electronics 
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C Rotator CTRLler
*/

#ifndef __REGS_ROTATOR_H
#define __REGS_ROTATOR_H

/*************************************************************************
 * Register part
 ************************************************************************/
#define S3C_ROTATOR(x)	((x))
#define S3C_ROTATOR_CONFIG			S3C_ROTATOR(0x00)
#define S3C_ROTATOR_CTRL			S3C_ROTATOR(0x10)
#define S3C_ROTATOR_STATUS			S3C_ROTATOR(0x20)

#define S3C_ROTATOR_SRCBASEADDR0		S3C_ROTATOR(0x30)
#define S3C_ROTATOR_SRCBASEADDR1		S3C_ROTATOR(0x34)
#define S3C_ROTATOR_SRCBASEADDR2		S3C_ROTATOR(0x38)
#define S3C_ROTATOR_SRCIMGSIZE			S3C_ROTATOR(0x3C)
#define S3C_ROTATOR_SRC_XY			S3C_ROTATOR(0x40)
#define S3C_ROTATOR_SRCROTSIZE			S3C_ROTATOR(0x44)
#define S3C_ROTATOR_DSTBASEADDR0		S3C_ROTATOR(0x50)
#define S3C_ROTATOR_DSTBASEADDR1		S3C_ROTATOR(0x54)
#define S3C_ROTATOR_DSTBASEADDR2		S3C_ROTATOR(0x58)
#define S3C_ROTATOR_DSTIMGSIZE			S3C_ROTATOR(0x5C)
#define S3C_ROTATOR_DST_XY			S3C_ROTATOR(0x60)
#define S3C_ROTATOR_WRITE_PATTERN		S3C_ROTATOR(0x70)


/*************************************************************************
 * Macro part
 ************************************************************************/
#define S3C_ROT_SRC_WIDTH(x)				((x) << 0)
#define S3C_ROT_SRC_HEIGHT(x)				((x) << 16)
#define S3C_ROT_ROT_DEGREE(x)				((x) << 4)
#define S3C_ROT_SRC_X_OFFSET(x)				((x) << 0)
#define S3C_ROT_SRC_Y_OFFSET(x)				((x) << 16)
/*************************************************************************
 * Bit definition part
 ************************************************************************/
#define S3C_ROTATOR_IDLE				(0 << 0)

#define S3C_ROT_CONFIG_ENABLE_INT			(1 << 8)
#define S3C_ROT_CONFIG_STATUS_MASK			(3 << 0)

#define S3C_ROT_CTRL_PATTERN_WRITING			(1 << 16)

#define S3C_ROT_CTRL_INPUT_FMT_YCBCR420_3P		(0 << 8)
#define S3C_ROT_CTRL_INPUT_FMT_YCBCR420_2P		(1 << 8)
#define S3C_ROT_CTRL_INPUT_FMT_YCBCR422_1P		(3 << 8)
#define S3C_ROT_CTRL_INPUT_FMT_RGB565			(4 << 8)
#define S3C_ROT_CTRL_INPUT_FMT_RGB888			(6 << 8)
#define S3C_ROT_CTRL_INPUT_FMT_MASK			(7 << 8)

#define S3C_ROT_CTRL_DEGREE_BYPASS			(0 << 4)
#define S3C_ROT_CTRL_DEGREE_90				(1 << 4)
#define S3C_ROT_CTRL_DEGREE_180				(2 << 4)
#define S3C_ROT_CTRL_DEGREE_270				(3 << 4)
#define S3C_ROT_CTRL_DEGREE_MASK			(0xF << 4)

#define S3C_ROT_CTRL_FLIP_BYPASS			(0 << 6)
#define S3C_ROT_CTRL_FLIP_VERTICAL			(2 << 6)
#define S3C_ROT_CTRL_FLIP_HORIZONTAL			(3 << 6)

#define S3C_ROT_CTRL_START_ROTATE			(1 << 0)	
#define S3C_ROT_CTRL_START_MASK				(1 << 0)

#define S3C_ROT_STATREG_INT_PENDING			(1 << 8)
#define S3C_ROT_STATREG_STATUS_IDLE			(0 << 0)
#define S3C_ROT_STATREG_STATUS_BUSY			(2 << 0)
#define S3C_ROT_STATREG_STATUS_HAS_ONE_MORE_JOB 	(3 << 0)

#endif /* REGS_ROTATOR_H */
