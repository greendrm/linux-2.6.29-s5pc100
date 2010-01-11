
/* linux/arch/arm/plat-s5pc11x/include/plat/regs-g2d.h
 *
 * Driver file for Samsung 2D Accelerator(FIMG-2D)
 * Jinhee Hyeon, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _FIMG2D3X_H_
#define _FIMG2D3X_H_

#define G2DREG(x)	((x))

/* Registers */
#define G2D_SOFT_RESET_REG			G2DREG(0x000)
#define G2D_INTEN_REG				G2DREG(0x004)
#define G2D_INTC_PEND_REG			G2DREG(0x00c)
#define G2D_FIFO_STAT_REG			G2DREG(0x010)
#define G2D_AXI_ID_MODE_REG			G2DREG(0x014)
#define G2D_CACHECTL_REG			G2DREG(0x018)
#define G2D_BITBLT_START_REG			G2DREG(0x100)
#define G2D_BITBLT_COMMAND_REG			G2DREG(0x104)
#define G2D_ROTATE_REG				G2DREG(0x200)
#define G2D_SRC_MSK_DIRECT_REG			G2DREG(0x204)
#define G2D_DST_PAT_DIRECT_REG			G2DREG(0x208)
#define G2D_SRC_SELECT_REG			G2DREG(0x300)
#define G2D_SRC_BASE_ADDR_REG			G2DREG(0x304)
#define G2D_SRC_STRIDE_REG			G2DREG(0x308)
#define G2D_SRC_COLOR_MODE_REG			G2DREG(0x30c)
#define G2D_SRC_LEFT_TOP_REG			G2DREG(0x310)
#define G2D_SRC_RIGHT_BOTTOM_REG		G2DREG(0x314)
#define G2D_DST_SELECT_REG			G2DREG(0x400)
#define G2D_DST_BASE_ADDR_REG			G2DREG(0x404)
#define G2D_DST_STRIDE_REG			G2DREG(0x408)
#define G2D_DST_COLOR_MODE_REG			G2DREG(0x40c)
#define G2D_DST_LEFT_TOP_REG			G2DREG(0x410)
#define G2D_DST_RIGHT_BOTTOM_REG		G2DREG(0x414)
#define G2D_CW_LT_REG				G2DREG(0x600)
#define G2D_CW_RB_REG				G2DREG(0x604)
#define G2D_THIRD_OPERAND_REG			G2DREG(0x610)
#define G2D_ROP4_REG				G2DREG(0x614)
#define G2D_ALPHA_REG				G2DREG(0x618)
#define G2D_FG_COLOR_REG			G2DREG(0x700)
#define G2D_BG_COLOR_REG			G2DREG(0x704)
#define G2D_BS_COLOR_REG			G2DREG(0x708)
#define G2D_SRC_COLORKEY_CTRL_REG		G2DREG(0x710)
#define G2D_SRC_COLORKEY_DR_MIN_REG		G2DREG(0x714)
#define G2D_SRC_COLORKEY_DR_MAX_REG		G2DREG(0x718)
#define G2D_DST_COLORKEY_CTRL_REG		G2DREG(0x71c)
#define G2D_DST_COLORKEY_DR_MIN_REG		G2DREG(0x720)
#define G2D_DST_COLORKEY_DR_MAX_REG		G2DREG(0x724)

/* Bit Definitions */
#define G2D_SOFT_RESET			(1 << 0)
#define G2D_INT_EN			(1 << 0)
#define G2D_INTP_CMD_FIN		(1 << 0)
#define G2D_CMD_FIN			(1 << 0)
#define G2D_PATCACHE_CLEAR		(1 << 2)
#define G2D_SRCBUFFER_CLEAR		(1 << 1)
#define G2D_MASKBUFFER_CLEAR		(1 << 0)
#define G2D_SOFT_RESET			(1 << 0)
#define G2D_START_BITBLT		(1 << 0)
#define G2D_ENABLE_CW			(1 << 8)
#define G2D_ENABLE_STRETCH		(1 << 4)
#define G2D_ENABLE_MASK			(1 << 0)
#define G2D_SRC_X_DIR_NEGATIVE		(1 << 0)
#define G2D_SRC_Y_DIR_NEGATIVE		(1 << 1)
#define G2D_DST_X_DIR_NEGATIVE		(1 << 0)
#define G2D_DST_Y_DIR_NEGATIVE		(1 << 1)

#define G2D_COORDINATE_TOP_Y_SHIFT	(16)
#define G2D_COORDINATE_LEFT_X_SHIFT	(0)
#define G2D_COORDINATE_BOTTOM_Y_SHIFT	(16)
#define G2D_COORDINATE_RIGHT_X_SHIFT	(0)

#define G2D_MASKED_ROP_MASK		(0xff)
#define G2D_UNMASKED_ROP_MASK		(0xff)
#define G2D_MASKED_ROP3_SHIFT		(8)
#define G2D_UNMASKED_ROP3_SHIFT		(0)

#define G2D_ROP3_SRC_ONLY		(0xcc)
#define G2D_ROP3_DST_ONLY		(0xaa)
#define G2D_ROP3_3RD_ONLY		(0xf0)
#define G2D_ROP3_SRC_DST		(0x88)
#define G2D_ROP3_SRC_3RD		(0xfc)

#endif

