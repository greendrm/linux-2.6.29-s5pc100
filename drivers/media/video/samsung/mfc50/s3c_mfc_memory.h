/*
 * drivers/media/video/samsung/mfc50/s3c_mfc_memory.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C_MFC_MEMORY_H_
#define _S3C_MFC_MEMORY_H_

#include "s3c_mfc_common.h"
#include "s3c_mfc_types.h"

#ifdef CONFIG_VIDEO_MFC_MAX_INSTANCE
#define MFC_MAX_INSTANCE_NUM (CONFIG_VIDEO_MFC_MAX_INSTANCE)
#endif

/* DRAM memory start address */
#define MFC_DRAM1_START		(0x40000000)	/* mDDR, 0x4000_0000 ~ 0x4800_0000 (128MB) */

/* All buffer size have to be aligned to 64K */
#define FIRMWARE_CODE_SIZE	(0x60000)	/* 0x306c0(198,336 byte) ~ 0x60000(393,216) */
#define MFC_FW_SYSTEM_SIZE	(0x100000)	/* 1MB : 1x1024x1024 */
#define MFC_FW_BUF_SIZE		(0x96000)	/* 600KB : 600x1024 size per instance */
#define MFC_FW_TOTAL_BUF_SIZE	(MFC_FW_SYSTEM_SIZE + MFC_MAX_INSTANCE_NUM * MFC_FW_BUF_SIZE)

#define DESC_BUF_SIZE		(0x20000)	/* 128KB : 128x1024 */
#define RISC_BUF_SIZE		(0x80000)	/* 512KB : 512x1024 size per instance */
#define SHARED_MEM_SIZE		(0x1000)	/* 4KB   : 4x1024 size */


#define CPB_BUF_SIZE		(0x400000)	/* 4MB : 4x1024x1024 for decoder */

#define STREAM_BUF_SIZE		(0x200000)	/* 2MB : 2x1024x1024 for encoder */

#define ENC_UP_INTRA_PRED_SIZE	(0x10000)	/* 64KB : 64x1024 for encoder */

volatile unsigned char *s3c_mfc_get_fw_buf_virt_addr(void);
volatile unsigned char *s3c_mfc_get_data_buf_virt_addr(void);
volatile unsigned char *s3c_mfc_get_dpb_luma_buf_virt_addr(void);

unsigned int s3c_mfc_get_fw_buf_phys_addr(void);
unsigned int s3c_mfc_get_fw_context_phys_addr(int instNo);
unsigned int s3c_mfc_get_risc_buf_phys_addr(int instNo);
unsigned int s3c_mfc_get_data_buf_phys_addr(void);
unsigned int s3c_mfc_get_dpb_luma_buf_phys_addr(void);

#endif /* _S3C_MFC_MEMORY_H_ */
