/* 
 * drivers/media/video/samsung/mfc40/s3c_mfc_common.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C_MFC_COMMON_H_
#define _S3C_MFC_COMMON_H_

#include <plat/regs-mfc.h>

#include "s3c_mfc_interface.h"

#define BUF_ALIGN_UNIT (64)
#define Align(x, alignbyte) (((x)+(alignbyte)-1)/(alignbyte)*(alignbyte))

typedef enum
{
	MFCINST_STATE_NULL = 0,

	/* Instance is created */
	MFCINST_STATE_OPENED = 10,

	/* channel_set and init_codec is completed */
	MFCINST_STATE_DEC_INITIALIZE = 20,

	/* seq_start is completed */
	MFCINST_STATE_DEC_SEQ_START  = 30,
	
	MFCINST_STATE_DEC_EXE,
	MFCINST_STATE_DEC_EXE_DONE,
	
	/* Instance is initialized for encoding */
	MFCINST_STATE_ENC_INITIALIZE = 40, 
	MFCINST_STATE_ENC_EXE,
	MFCINST_STATE_ENC_EXE_DONE,

	MFCINST_STATE_RESET = 60,
	MFCINST_STATE_RESET_WAIT
} s3c_mfc_inst_state;

typedef enum
{
	MEM_STRUCT_LINEAR = 0,
	MEM_STRUCT_TILE_ENC  = 2, /* 16x16 */
	MEM_STRUCT_TILE_DEC  = 3  /* 64x32 */
} s3c_mfc_mem_type;

typedef enum
{
	MFC_POLLING_DMA_DONE = 1,
	MFC_POLLING_HEADER_DONE = 2,
	MFC_POLLING_OPERATION_DONE = 3,
	MFC_POLLING_FW_DONE = 4,
	MFC_INTR_FW_DONE = (1 << 5),
	MFC_INTR_DMA_DONE = (1 << 7),
	MFC_INTR_FRAME_DONE = (1 << 8),
	MFC_INTR_FRAME_FW_DONE = ((1 << 8) | (1 << 5))
} s3c_mfc_wait_done_type;


typedef enum
{
	DECODING_ONLY = 0,
	DECODING_DISPLAY = 1,
	DISPLAY_ONLY = 2
} s3c_mfc_display_status;

typedef enum
{
	MFC_RET_FRAME_NOT_SET = -1,
	MFC_RET_FRAME_NOT_CODED = 0,
	MFC_RET_FRAME_I_FRAME = 1,
	MFC_RET_FRAME_P_FRAME = 2,
	MFC_RET_FRAME_B_FRAME = 3
} s3c_mfc_frame_type;

typedef struct tag_mfc_inst_ctx
{
	u32 MfcSfr[S3C_FIMV_REG_COUNT];

	s32 InstNo;
	u32 DPBCnt;
	u32 totalDPBCnt;
	u32 extraDPB;
	u32 displayDelay;
	u32 postEnable;
	u32 endOfFrame;
	u32 img_width;
	u32 img_height;
	u32 dwAccess;  // for Power Management.
	u32 IsPackedPB;
	
	s3c_mfc_frame_type FrameType;
	MFC_CODEC_TYPE MfcCodecType;
	s3c_mfc_inst_state MfcState;

	u32 virt_stream_buffer;
} s3c_mfc_inst_ctx;

unsigned int s3c_mfc_get_codec_type(MFC_CODEC_TYPE    codec_type);
int s3c_mfc_get_fw_buf_offset(MFC_CODEC_TYPE codecType);
int s3c_mfc_get_fw_buf_size(MFC_CODEC_TYPE codecType);
void s3c_mfc_init_inst_no(void);
int s3c_mfc_get_inst_no(void);
void s3c_mfc_return_inst_no(int inst_no);
int s3c_mfc_set_state(s3c_mfc_inst_ctx *ctx, s3c_mfc_inst_state state);

void s3c_mfc_set_reset_state(void);
void s3c_mfc_clear_reset_state(int inst_no);
s3c_mfc_inst_state s3c_mfc_get_reset_state(int inst_no);


#endif /* _S3C_MFC_COMMON_H_ */

