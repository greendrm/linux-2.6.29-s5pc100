/*
 * drivers/media/video/samsung/mfc50/s3c_mfc_opr.h
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

#ifndef _S3C_MFC_OPR_H_
#define _S3C_MFC_OPR_H_

#include "s3c_mfc_errorno.h"
#include "s3c_mfc_interface.h"
#include "s3c_mfc_types.h"

#define     INT_MFC_FW_DONE		(0x1 << 5)
#define     INT_MFC_DMA_DONE		(0x1 << 7)
#define     INT_MFC_FRAME_DONE		(0x1 << 8)
/* Interrupt on/off (0x500) */
#define     INT_ENABLE_BIT		(0 << 0)
#define     INT_DISABLE_BIT		(1 << 0)
/* Interrupt mode (0x504) */
#define     INT_LEVEL_BIT		(0 << 0)
#define     INT_PULSE_BIT		(1 << 0)

/* Command Types */
#define		MFC_CHANNEL_SET		0
#define		MFC_CHANNEL_READ	1
#define		MFC_CHANNEL_END		2
#define		MFC_INIT_CODEC		3
#define		MFC_FRAME_RUN		4
#define		MFC_SLEEP		6
#define		MFC_WAKEUP		7

/* DPB Count */
#define		NUM_MPEG4_DPB		(2)
#define		NUM_POST_DPB		(3)
#define		NUM_VC1_DPB		(4)

#define		STUFF_BYTE_SIZE		(4)
#define 	NAL_START_CODE		(0x00000001)
#define		AVAILABLE_SLICES_MAX	50

#define MPEG4_TIME_RES 1000

int s3c_mfc_load_firmware(void);

SSBSIP_MFC_ERROR_CODE s3c_mfc_init_hw(void);
SSBSIP_MFC_ERROR_CODE s3c_mfc_init_encode(s3c_mfc_inst_ctx * mfc_ctx,
					  s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_encode_header(s3c_mfc_inst_ctx * mfc_ctx,
					    s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_set_enc_ref_buffer(s3c_mfc_inst_ctx * mfc_ctx,
						 s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_exe_encode(s3c_mfc_inst_ctx * mfc_ctx,
					 s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_init_decode(s3c_mfc_inst_ctx * mfc_ctx,
					  s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_set_dec_frame_buffer(s3c_mfc_inst_ctx * mfc_ctx,
						   s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_exe_decode(s3c_mfc_inst_ctx * mfc_ctx,
					 s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_get_config(s3c_mfc_inst_ctx * mfc_ctx,
					 s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_set_config(s3c_mfc_inst_ctx * mfc_ctx,
					 s3c_mfc_args * args);
SSBSIP_MFC_ERROR_CODE s3c_mfc_deinit_hw(s3c_mfc_inst_ctx * mfc_ctx);
SSBSIP_MFC_ERROR_CODE s3c_mfc_set_sleep(void);
SSBSIP_MFC_ERROR_CODE s3c_mfc_set_wakeup(void);

void s3c_mfc_clear_int(void);
void s3c_mfc_clear_ch_id(unsigned int int_type);
void dump_sfrs(void);

#endif /* _S3C_MFC_OPR_H_ */
