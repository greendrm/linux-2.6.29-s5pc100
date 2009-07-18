/* linux/drivers/media/video/samsung/s3c_fimc4x_regs.c
 *
 * Register interface file for Samsung Camera Interface (FIMC) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include <mach/map.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-h3.h>
#include <plat/regs-fimc.h>
#include <plat/fimc2.h>

#include "fimc.h"

void fimc_clear_irq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg |= S3C_CIGCTRL_IRQ_CLR;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);
}


void fimc_set_int_enable(struct fimc_control *ctrl, u32 enable)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (enable == 1) {
		cfg |= (S3C_CIGCTRL_IRQ_ENABLE | S3C_CIGCTRL_IRQ_LEVEL);
	} else {
		cfg &= ~S3C_CIGCTRL_IRQ_ENABLE;
	}

	writel(cfg, ctrl->regs + S3C_CIGCTRL);
}

void fimc_reset(struct fimc_control *ctrl)
{
	u32 cfg = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	cfg = readl(ctrl->regs + S3C_CISRCFMT);
	cfg |= S3C_CISRCFMT_ITU601_8BIT;
	writel(cfg, ctrl->regs + S3C_CISRCFMT);

	/* s/w reset */
	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg |= (S3C_CIGCTRL_SWRST);
	writel(cfg, ctrl->regs + S3C_CIGCTRL);
	mdelay(1);

	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg &= ~S3C_CIGCTRL_SWRST;
	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	/* in case of ITU656, CISRCFMT[31] should be 0 */
	if ((ctrl->cap != NULL) && (ctrl->cam->fmt == ITU_656_YCBCR422_8BIT)) {
		cfg = readl(ctrl->regs + S3C_CISRCFMT);
		cfg &= ~S3C_CISRCFMT_ITU601_8BIT;
		writel(cfg, ctrl->regs + S3C_CISRCFMT);
	}
}

/*
 * 1. Configure camera input we are to use:
 * for now we use cam->id to identify camera A and B
 * but need to be changed to go with VIDIOC_S_INPUT
 * - Parallel interface : camera A (SELCAM_ITU_A & SELCAM_ITU)
 *   			camera B (SELCAM_ITU_B & SELCAM_ITU)
 *   			test pattern (TESTPATTERN_*)
 * - Serial interface : camera C (SELCAM_MIPI)
 *   			test pattern (TESTPATTERN_*)
 * 2. Configure input camera's format:
 * 	S3C_CISRCFMT for ITU & S3C_CSIIMGFMT 
 */
int fimc_select_camera(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	u32 cfg;

	if (!cam) {
		dev_err(ctrl->dev, "%s: no active camera\n", \
			__FUNCTION__);
		return -ENODEV;
	}

	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg &= ~(S3C_CIGCTRL_TESTPATTERN_MASK | S3C_CIGCTRL_SELCAM_ITU_MASK | \
		S3C_CIGCTRL_SELCAM_MASK);

	/* Interface selection */
	if (cam->type == CAM_TYPE_MIPI) {
		cfg |= S3C_CIGCTRL_SELCAM_MIPI;
	} else if (cam->type == CAM_TYPE_ITU) {
		if (cam->id == CAMERA_PAR_A)
			cfg |= S3C_CIGCTRL_SELCAM_ITU_A;
		else
			cfg |= S3C_CIGCTRL_SELCAM_ITU_B;
		/* switch to ITU interface */
		cfg |= S3C_CIGCTRL_SELCAM_ITU;
	} else {
		dev_err(ctrl->dev, "%s: invalid camera bus type selected\n", \
			__FUNCTION__);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

void fimc_reset_camera(void)
{
	void __iomem *regs = ioremap(S5PC1XX_PA_FIMC0, SZ_4K);
	u32 cfg;

#if (CONFIG_VIDEO_FIMC_CAM_RESET == 1)
	cfg = readl(regs + S3C_CIGCTRL);
	cfg |= S3C_CIGCTRL_CAMRST_A;
	writel(cfg, regs + S3C_CIGCTRL);
	udelay(200);

	cfg = readl(regs + S3C_CIGCTRL);
	cfg &= ~S3C_CIGCTRL_CAMRST_A;
	writel(cfg, regs + S3C_CIGCTRL);
	udelay(2000);
#else
	cfg = readl(regs + S3C_CIGCTRL);
	cfg &= ~S3C_CIGCTRL_CAMRST_A;
	writel(cfg, regs + S3C_CIGCTRL);
	udelay(200);

	cfg = readl(regs + S3C_CIGCTRL);
	cfg |= S3C_CIGCTRL_CAMRST_A;
	writel(cfg, regs + S3C_CIGCTRL);
	udelay(2000);
#endif

#if (CONFIG_VIDEO_FIMC_CAM_CH == 1)
	cfg = readl(S5PC1XX_GPH3CON);
	cfg &= ~S5PC1XX_GPH3_CONMASK(6);
	cfg |= S5PC1XX_GPH3_OUTPUT(6);
	writel(cfg, S5PC1XX_GPH3CON);

	cfg = readl(S5PC1XX_GPH3DAT);
	cfg &= ~(0x1 << 6);
	writel(cfg, S5PC1XX_GPH3DAT);
	udelay(200);

	cfg |= (0x1 << 6);
	writel(cfg, S5PC1XX_GPH3DAT);
	udelay(2000);
#endif

	iounmap(regs);
}


int fimc_set_src_format(struct fimc_control *ctrl, u32 pixfmt)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	/* Set CSCR2Y & CSCY2R */
	cfg |= (S3C_CISCCTRL_CSCR2Y_WIDE | S3C_CISCCTRL_CSCY2R_WIDE);
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	/* Color format setting */
	if (pixfmt == V4L2_PIX_FMT_NV12) {
		cfg = readl(ctrl->regs + S3C_MSCTRL);
		cfg &= ~(S3C_MSCTRL_BURST_CNT_MASK | S3C_MSCTRL_2PLANE_SHIFT_MASK \
			| S3C_MSCTRL_C_INT_IN_2PLANE | S3C_MSCTRL_INFORMAT_RGB);
		cfg |= ((0x4<<S3C_MSCTRL_BURST_CNT) | S3C_MSCTRL_C_INT_IN_2PLANE \
			| S3C_MSCTRL_INFORMAT_YCBCR420);
		writel(cfg, ctrl->regs + S3C_MSCTRL);
	} else if (pixfmt == V4L2_PIX_FMT_RGB32) {
		cfg = readl(ctrl->regs + S3C_MSCTRL);
		cfg &= ~(S3C_MSCTRL_BURST_CNT_MASK | S3C_MSCTRL_C_INT_IN_2PLANE \
				| S3C_MSCTRL_INFORMAT_RGB);
		cfg |= (S3C_MSCTRL_INFORMAT_RGB | (0x4<<S3C_MSCTRL_BURST_CNT));
		writel(cfg, ctrl->regs + S3C_MSCTRL);
	
		cfg = readl(ctrl->regs + S3C_CISCCTRL);
		cfg &= ~(S3C_CISCCTRL_INRGB_FMT_RGB666 | \
				S3C_CISCCTRL_INRGB_FMT_RGB888);
		cfg |= (S3C_CISCCTRL_INRGB_FMT_RGB888 | \
				S3C_CISCCTRL_EXTRGB_EXTENSION);
		writel(cfg, ctrl->regs + S3C_CISCCTRL);
	} else if (pixfmt == V4L2_PIX_FMT_RGB565) {
		cfg = readl(ctrl->regs + S3C_MSCTRL);
		cfg &= ~(S3C_MSCTRL_BURST_CNT_MASK | S3C_MSCTRL_C_INT_IN_2PLANE \
				| S3C_MSCTRL_INFORMAT_RGB);
		cfg |= (S3C_MSCTRL_INFORMAT_RGB | \
				(0x4<<S3C_MSCTRL_BURST_CNT));
		writel(cfg, ctrl->regs + S3C_MSCTRL);
	
		cfg = readl(ctrl->regs + S3C_CISCCTRL);
		cfg &= ~(S3C_CISCCTRL_INRGB_FMT_RGB666 | \
				S3C_CISCCTRL_INRGB_FMT_RGB888);
		cfg |= (S3C_CISCCTRL_INRGB_FMT_RGB565 | \
				S3C_CISCCTRL_EXTRGB_EXTENSION);
		writel(cfg, ctrl->regs + S3C_CISCCTRL);
	} else {
		dev_err(ctrl->dev, "[%s]Invalid pixelformt : %d\n", 
				__FUNCTION__, pixfmt);
		return -EINVAL;
	}

	/* To do : Interlace setting in CISCCTRL. */

	return 0;
}

int fimc_set_dst_format(struct fimc_control *ctrl, u32 pixfmt)
{
	u32 cfg = readl(ctrl->regs + S3C_CITRGFMT);

	/* Color format setting */
	if (pixfmt == V4L2_PIX_FMT_NV12) {
		cfg &= ~S3C_CITRGFMT_OUTFORMAT_RGB;
		cfg |= S3C_CITRGFMT_OUTFORMAT_YCBCR420;
		writel(cfg, ctrl->regs + S3C_CITRGFMT);

		cfg = readl(ctrl->regs + S3C_CIOCTRL);
		cfg &= ~S3C_CIOCTRL_ORDER2P_MASK;
		cfg |= S3C_CIOCTRL_YCBCR_2PLANE;
		writel(cfg, ctrl->regs + S3C_CIOCTRL);

		/* To do : Check output path whether it is DMA out. */

	} else if (pixfmt == V4L2_PIX_FMT_RGB32) {
		cfg |= S3C_CITRGFMT_OUTFORMAT_RGB;
		writel(cfg, ctrl->regs + S3C_CITRGFMT);
	
		cfg = readl(ctrl->regs + S3C_CISCCTRL);
		cfg &= ~(S3C_CISCCTRL_OUTRGB_FMT_RGB666 | \
				S3C_CISCCTRL_OUTRGB_FMT_RGB888);
		cfg |= (S3C_CISCCTRL_OUTRGB_FMT_RGB888);
		writel(cfg, ctrl->regs + S3C_CISCCTRL);
	} else if (pixfmt == V4L2_PIX_FMT_RGB565) {
		cfg |= S3C_CITRGFMT_OUTFORMAT_RGB;
		writel(cfg, ctrl->regs + S3C_CITRGFMT);
	
		cfg = readl(ctrl->regs + S3C_CISCCTRL);
		cfg &= ~(S3C_CISCCTRL_OUTRGB_FMT_RGB666 | \
				S3C_CISCCTRL_OUTRGB_FMT_RGB888);
		cfg |= (S3C_CISCCTRL_OUTRGB_FMT_RGB565);
		writel(cfg, ctrl->regs + S3C_CISCCTRL);
	} else {
		dev_err(ctrl->dev, "[%s]Invalid pixelformt : %d\n", 
				__FUNCTION__, pixfmt);
		return -EINVAL;
	}

	return 0;
}

int fimc_set_in_rot(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	int rot_flip = 0;
	u32 cfg_r = readl(ctrl->regs + S3C_CITRGFMT);
	u32 cfg_f = readl(ctrl->regs + S3C_MSCTRL);

	cfg_r &= ~S3C_CITRGFMT_INROT90_CLOCKWISE;
	cfg_f &= ~S3C_MSCTRL_FLIP_MASK;	

	rot_flip = fimc_mapping_rot_flip(rot, flip);

	if (rot_flip && 0x10)
		cfg_r |= S3C_CITRGFMT_INROT90_CLOCKWISE;

	if(rot_flip && V4L2_CID_HFLIP)
		cfg_f |= S3C_MSCTRL_FLIP_X_MIRROR;

	if(rot_flip && V4L2_CID_VFLIP)
		cfg_f |= S3C_MSCTRL_FLIP_Y_MIRROR;

	writel(cfg_r, ctrl->regs + S3C_CITRGFMT);
	writel(cfg_f, ctrl->regs + S3C_MSCTRL);	

	return 0;
}

int fimc_set_out_rot(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	int rot_flip = 0;
	u32 cfg = readl(ctrl->regs + S3C_CITRGFMT);

	cfg &= ~(S3C_CITRGFMT_OUTROT90_CLOCKWISE | S3C_CITRGFMT_FLIP_MASK);

	rot_flip = fimc_mapping_rot_flip(rot, flip);

	if (rot_flip && 0x10)
		cfg |= S3C_CITRGFMT_OUTROT90_CLOCKWISE;

	if(rot_flip && V4L2_CID_HFLIP)
		cfg |= S3C_CITRGFMT_FLIP_X_MIRROR;

	if(rot_flip && V4L2_CID_VFLIP)
		cfg |= S3C_CITRGFMT_FLIP_Y_MIRROR;

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

static void fimc_set_autoload(struct fimc_control *ctrl, u32 autoload)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);
	cfg &= ~S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE;

	if (autoload == FIMC_ONE_SHOT) {
		/* Fall through */
	} else if (autoload == FIMC_AUTO_LOAD) {
		cfg |= S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE;
	}

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);
}

int fimc_set_src_path(struct fimc_control *ctrl, u32 path)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_INPUT_MASK;

	if (path == FIMC_SRC_MSDMA) {
		cfg |= S3C_MSCTRL_INPUT_MEMORY;
	} else if (path == FIMC_SRC_CAM) {
		cfg |= S3C_MSCTRL_INPUT_EXTCAM;
	} else {
		dev_err(ctrl->dev, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_set_dst_path(struct fimc_control *ctrl, u32 path)
{
	u32 autoload = 0;
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_LCDPATHEN_FIFO;

	if (path == FIMC_DST_DMA) {
		autoload = FIMC_ONE_SHOT;
		fimc_set_autoload(ctrl, autoload);
	} else if (path == FIMC_DST_FIMD) {
		cfg |= S3C_CISCCTRL_LCDPATHEN_FIFO;

		autoload = FIMC_AUTO_LOAD;		
		fimc_set_autoload(ctrl, autoload);
	} else {
		dev_err(ctrl->dev, "[%s]Invalid case.\n", __FUNCTION__);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

static int fimc_check_src_offset(struct v4l2_rect *crop, struct v4l2_rect *pix)
{
	/* To do : offset validation check. */
	
	return 0;
}

int fimc_set_src_dma_offset(struct fimc_control *ctrl)
{
	int ret = 0;
	u32 cfg_y = 0, cfg_cb = 0, cfg_cr = 0, offset_en = 0, pixfmt = 0;
	struct v4l2_rect crop, pix;
	
	if (ctrl->out->crop.type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		dev_err(ctrl->dev, "[%s] Invalid case.\n", __FUNCTION__);
		ret = -EINVAL;
	}
	
	crop.left	= ctrl->out->crop.c.left;
	crop.top	= ctrl->out->crop.c.top;
	crop.width	= ctrl->out->crop.c.width;
	crop.height	= ctrl->out->crop.c.height;

	pix.width	= ctrl->out->pix.width;
	pix.height	= ctrl->out->pix.height;
	pixfmt		= ctrl->out->pix.pixelformat;
	
	if (crop.left || crop.top) {
		offset_en = 1;
	} else if ((crop.width != pix.width) || (crop.height != pix.height)) {
		offset_en = 1;
	}

	if (offset_en) {
		ret = fimc_check_src_offset(&crop, &pix);
		if (ret < 0) {
			dev_err(ctrl->dev, "Fail : fimc_check_src_offset()\n");
			ret = -EINVAL;
		}

		if (pixfmt == V4L2_PIX_FMT_NV12) {
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop.left);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop.top);
			cfg_cb |= S3C_CIICBOFF_HORIZONTAL(crop.left);
			cfg_cb |= S3C_CIICBOFF_VERTICAL(crop.top / 2);
		} else if (pixfmt == V4L2_PIX_FMT_RGB32) {
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop.left * 4);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop.top);
		} else if (pixfmt == V4L2_PIX_FMT_RGB565) {
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop.left * 2);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop.top);
		}
	}

	writel(cfg_y, ctrl->regs + S3C_CIIYOFF);
	writel(cfg_cb, ctrl->regs + S3C_CIICBOFF);
	writel(cfg_cr, ctrl->regs + S3C_CIICROFF);

	return 0;
}

void fimc_set_src_dma_size(struct fimc_control *ctrl)
{
	u32 cfg_o = 0, cfg_r = 0;
	u32 width_r = 0, height_r = 0;

	cfg_o |= S3C_ORGISIZE_HORIZONTAL(ctrl->out->pix.width);
	cfg_o |= S3C_ORGISIZE_VERTICAL(ctrl->out->pix.height);

	width_r		= ctrl->out->pix.width - ctrl->out->crop.c.left;
	height_r	= ctrl->out->pix.height - ctrl->out->crop.c.top;
	cfg_r |= S3C_CIREAL_ISIZE_WIDTH(width_r);
	cfg_r |= S3C_CIREAL_ISIZE_HEIGHT(height_r);

	writel(cfg_o, ctrl->regs + S3C_ORGISIZE);
	writel(cfg_r, ctrl->regs + S3C_CIREAL_ISIZE);
	
}

void fimc_set_dst_dma_offset(struct fimc_control *ctrl)
{
	u32 cfg_y = 0, cfg_cb = 0, cfg_cr = 0;
	
	writel(cfg_y, ctrl->regs + S3C_CIOYOFF);
	writel(cfg_cb, ctrl->regs + S3C_CIOCBOFF);
	writel(cfg_cr, ctrl->regs + S3C_CIOCROFF);
}

void fimc_set_dst_dma_size(struct fimc_control *ctrl)
{
	int is_rotate = 0;
	u32 cfg = 0;
	struct v4l2_rect rect;

	
	is_rotate = fimc_mapping_rot_flip(ctrl->out->rotate, ctrl->out->flip);
	if (is_rotate && 0x10) {	/* Landscape mode */
		rect.width	= ctrl->out->fbuf.fmt.height;
		rect.height	= ctrl->out->fbuf.fmt.width;
	} else {			/* Portrait mode */
		rect.width	= ctrl->out->fbuf.fmt.width;
		rect.height	= ctrl->out->fbuf.fmt.height;
	}

	/* TargetHsize, TargetVsize setting */
	cfg = readl(ctrl->regs + S3C_CITRGFMT);
	cfg &= ~S3C_CITRGFMT_TARGETH_MASK;
	cfg &= ~S3C_CITRGFMT_TARGETV_MASK;
	cfg |= S3C_CITRGFMT_TARGETHSIZE(rect.width);
	cfg |= S3C_CITRGFMT_TARGETVSIZE(rect.height);
	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	/* CITAREA setting */
	cfg = 0;
	cfg = S3C_CITAREA_TARGET_AREA(rect.width * rect.height);
	writel(cfg, ctrl->regs + S3C_CITAREA);

	/* ORGOSIZE */
	cfg = 0;
	cfg |= S3C_ORGOSIZE_HORIZONTAL(rect.width);
	cfg |= S3C_ORGOSIZE_VERTICAL(rect.height);
	writel(cfg, ctrl->regs + S3C_ORGOSIZE);

	/* TargetHsize_ext, TargetVsize_ext setting */
	cfg = readl(ctrl->regs + S3C_CIEXTEN);
	cfg &= ~S3C_CIEXTEN_TARGETH_EXT_MASK;
	cfg &= ~S3C_CIEXTEN_TARGETV_EXT_MASK;
	cfg |= S3C_CIEXTEN_TARGETH_EXT(rect.width);
	cfg |= S3C_CIEXTEN_TARGETV_EXT(rect.height);
	writel(cfg, ctrl->regs + S3C_CIEXTEN);
}

void fimc_set_prescaler(struct fimc_control *ctrl)
{
	u32 cfg = 0;

	cfg |= S3C_CISCPRERATIO_SHFACTOR(ctrl->sc.shfactor);
	cfg |= S3C_CISCPRERATIO_PREHORRATIO(ctrl->sc.pre_hratio);
	cfg |= S3C_CISCPRERATIO_PREVERRATIO(ctrl->sc.pre_vratio);

	writel(cfg, ctrl->regs + S3C_CISCPRERATIO);

	cfg = 0;
	cfg |= S3C_CISCPREDST_PREDSTWIDTH(ctrl->sc.pre_dst_width);
	cfg |= S3C_CISCPREDST_PREDSTHEIGHT(ctrl->sc.pre_dst_height);

	writel(cfg, ctrl->regs + S3C_CISCPREDST);
}

void fimc_set_mainscaler(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~(S3C_CISCCTRL_SCALERBYPASS | \
		S3C_CISCCTRL_SCALEUP_H | S3C_CISCCTRL_SCALEUP_V | \
		S3C_CISCCTRL_MAIN_V_RATIO_MASK | S3C_CISCCTRL_MAIN_H_RATIO_MASK);

	if (ctrl->sc.bypass)
		cfg |= S3C_CISCCTRL_SCALERBYPASS;

	if (ctrl->sc.scaleup_h)
		cfg |= S3C_CISCCTRL_SCALEUP_H;

	if (ctrl->sc.scaleup_v)
		cfg |= S3C_CISCCTRL_SCALEUP_V;

	cfg |= S3C_CISCCTRL_MAINHORRATIO(ctrl->sc.main_hratio);
	cfg |= S3C_CISCCTRL_MAINVERRATIO(ctrl->sc.main_vratio);

	writel(cfg, ctrl->regs + S3C_CISCCTRL);
}

void fimc_set_src_addr(struct fimc_control *ctrl, dma_addr_t base)
{
	struct v4l2_rect rect;
	dma_addr_t phys_cbcr = 0;
	u32 cfg = 0;
	u32 pixfmt = ctrl->out->pix.pixelformat;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);
	cfg |= S3C_CIREAL_ISIZE_ADDR_CH_DISABLE;
	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	if (pixfmt == V4L2_PIX_FMT_NV12) {
		rect.width = ctrl->out->pix.width;
		rect.height = ctrl->out->pix.height;
		phys_cbcr = base + (rect.width * rect.height);
		writel(base, ctrl->regs + S3C_CIIYSA0);
		writel(phys_cbcr, ctrl->regs + S3C_CIICBSA0);
	} else if (pixfmt == V4L2_PIX_FMT_RGB32) {
		writel(base, ctrl->regs + S3C_CIIYSA0);
	} else if (pixfmt == V4L2_PIX_FMT_RGB565) {
		writel(base, ctrl->regs + S3C_CIIYSA0);
	}

	cfg &= ~S3C_CIREAL_ISIZE_ADDR_CH_DISABLE;
	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);
}

void fimc_set_dst_addr(struct fimc_control *ctrl, dma_addr_t base, u32 idx)
{
	struct v4l2_rect rect;
	dma_addr_t phys_cbcr = 0;
	u32 pixfmt = ctrl->out->fbuf.fmt.pixelformat;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (pixfmt == V4L2_PIX_FMT_NV12) {
		rect.width = ctrl->out->fbuf.fmt.width;
		rect.height = ctrl->out->fbuf.fmt.height;
		phys_cbcr = base + (rect.width * rect.height);
		writel(base, ctrl->regs + S3C_CIOYSA(idx));
		writel(phys_cbcr, ctrl->regs + S3C_CIOCBSA(idx));
	} else if (pixfmt == V4L2_PIX_FMT_RGB32) {
		writel(base, ctrl->regs + S3C_CIOYSA(idx));
	} else if (pixfmt == V4L2_PIX_FMT_RGB565) {
		writel(base, ctrl->regs + S3C_CIOYSA(idx));
	}

}

void fimc_start_scaler(struct fimc_control *ctrl)
{
	u32 cfg = 0;

	if (!ctrl->sc.bypass) {
		cfg = readl(ctrl->regs + S3C_CISCCTRL);
		cfg |= S3C_CISCCTRL_SCALERSTART;
		writel(cfg, ctrl->regs + S3C_CISCCTRL);
	}
}

void fimc_stop_scaler(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~S3C_CISCCTRL_SCALERSTART;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);
}

void fimc_enable_capture(struct fimc_control *ctrl)
{
	u32 cfg = 0;

	cfg = readl(ctrl->regs + S3C_CIIMGCPT);
	cfg |= S3C_CIIMGCPT_IMGCPTEN;
	if (!ctrl->sc.bypass)
		cfg |= S3C_CIIMGCPT_IMGCPTEN_SC;
	writel(cfg, ctrl->regs + S3C_CIIMGCPT);
}

void fimc_disable_capture(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIIMGCPT);

	cfg &= ~(S3C_CIIMGCPT_IMGCPTEN | S3C_CIIMGCPT_IMGCPTEN_SC);
	writel(cfg, ctrl->regs + S3C_CIIMGCPT);
}

void fimc_enable_input_dma(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);

	cfg |= S3C_MSCTRL_ENVID;
	writel(cfg, ctrl->regs + S3C_MSCTRL);
}

void fimc_disable_input_dma(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);

	cfg &= ~S3C_MSCTRL_ENVID;
	writel(cfg, ctrl->regs + S3C_MSCTRL);
}

