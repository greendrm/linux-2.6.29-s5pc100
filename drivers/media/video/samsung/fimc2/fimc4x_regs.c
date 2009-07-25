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
#include <linux/videodev2.h>
#include <asm/io.h>
#include <mach/map.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-h3.h>
#include <plat/regs-fimc.h>
#include <plat/fimc2.h>

#include "fimc.h"

struct fimc_limit fimc_limits[FIMC_DEVICES] = {
	{
		.pre_dst_w	= 3264,
		.bypass_w	= 8192,
		.trg_h_no_rot	= 3264,
		.trg_h_rot	= 1280,
		.real_w_no_rot	= 8192,
		.real_h_rot	= 1280,
	}, {
		.pre_dst_w	= 1280,
		.bypass_w	= 8192,
		.trg_h_no_rot	= 1280,
		.trg_h_rot	= 8192,
		.real_w_no_rot	= 8192,
		.real_h_rot	= 768,
	}, {
		.pre_dst_w	= 1440,
		.bypass_w	= 8192,
		.trg_h_no_rot	= 1440,
		.trg_h_rot	= 0,
		.real_w_no_rot	= 8192,
		.real_h_rot	= 0,
	},
};

int fimc_hwset_camera_source(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	u32 cfg = 0;

	/* for now, we support only ITU601 8 bit mode */
	cfg |= S3C_CISRCFMT_ITU601_8BIT;
	cfg |= (cam->fmt | cam->order422);
	cfg |= S3C_CISRCFMT_SOURCEHSIZE(cam->width);
	cfg |= S3C_CISRCFMT_SOURCEVSIZE(cam->height);

	writel(cfg, ctrl->regs + S3C_CISRCFMT);
	
	return 0;
}

int fimc_hwset_enable_irq(struct fimc_control *ctrl, int overflow, int level)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg &= ~(S3C_CIGCTRL_IRQ_OVFEN | S3C_CIGCTRL_IRQ_LEVEL);
	cfg |= S3C_CIGCTRL_IRQ_ENABLE;

	if (overflow)
		cfg |= S3C_CIGCTRL_IRQ_OVFEN;

	if (level)
		cfg |= S3C_CIGCTRL_IRQ_LEVEL;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_disable_irq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg &= ~(S3C_CIGCTRL_IRQ_OVFEN | S3C_CIGCTRL_IRQ_ENABLE);
	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_clear_irq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg |= S3C_CIGCTRL_IRQ_CLR;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_reset(struct fimc_control *ctrl)
{
	u32 cfg = 0;

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

	return 0;
}

int fimc_hwget_overflow_state(struct fimc_control *ctrl)
{
	u32 cfg, status, flag;

	status = readl(ctrl->regs + S3C_CISTATUS);
	flag = S3C_CISTATUS_OVFIY | S3C_CISTATUS_OVFICB | S3C_CISTATUS_OVFICR;

	if (status & flag) {
		cfg = readl(ctrl->regs + S3C_CIWDOFST);
		cfg |= (S3C_CIWDOFST_CLROVFIY | S3C_CIWDOFST_CLROVFICB | \
			S3C_CIWDOFST_CLROVFICR);
		writel(cfg, ctrl->regs + S3C_CIWDOFST);

		cfg = readl(ctrl->regs + S3C_CIWDOFST);
		cfg &= ~(S3C_CIWDOFST_CLROVFIY | S3C_CIWDOFST_CLROVFICB | \
			S3C_CIWDOFST_CLROVFICR);
		writel(cfg, ctrl->regs + S3C_CIWDOFST);

		return 1;
	}

	return 0;
}

int fimc_hwset_camera_offset(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	struct v4l2_rect *rect = &cam->window;
	u32 cfg, h1, h2, v1, v2;

	if (!cam) {
		dev_err(ctrl->dev, "[%s] no active camera\n", \
			__FUNCTION__);
		return -ENODEV;
	}

	h1 = rect->left;
	h2 = cam->width - rect->width - rect->left;
	v1 = rect->top;
	v2 = cam->height - rect->height - rect->top;
	
	cfg = readl(ctrl->regs + S3C_CIWDOFST);
	cfg &= ~(S3C_CIWDOFST_WINHOROFST_MASK | S3C_CIWDOFST_WINVEROFST_MASK);
	cfg |= S3C_CIWDOFST_WINHOROFST(h1);
	cfg |= S3C_CIWDOFST_WINVEROFST(v1);
	cfg |= S3C_CIWDOFST_WINOFSEN;
	writel(cfg, ctrl->regs + S3C_CIWDOFST);

	cfg = 0;
	cfg |= S3C_CIWDOFST2_WINHOROFST2(h2);
	cfg |= S3C_CIWDOFST2_WINVEROFST2(v2);
	writel(cfg, ctrl->regs + S3C_CIWDOFST2);

	return 0;
}

int fimc_hwset_camera_polarity(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	u32 cfg;

	if (!cam) {
		dev_err(ctrl->dev, "[%s] no active camera\n", \
			__FUNCTION__);
		return -ENODEV;
	}

	cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg &= ~(S3C_CIGCTRL_INVPOLPCLK | S3C_CIGCTRL_INVPOLVSYNC | \
		 S3C_CIGCTRL_INVPOLHREF | S3C_CIGCTRL_INVPOLHSYNC);

	if (cam->inv_pclk)
		cfg |= S3C_CIGCTRL_INVPOLPCLK;

	if (cam->inv_vsync)
		cfg |= S3C_CIGCTRL_INVPOLVSYNC;

	if (cam->inv_href)
		cfg |= S3C_CIGCTRL_INVPOLHREF;

	if (cam->inv_hsync)
		cfg |= S3C_CIGCTRL_INVPOLHSYNC;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_camera_type(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	u32 cfg;

	if (!cam) {
		dev_err(ctrl->dev, "[%s] no active camera\n", \
			__FUNCTION__);
		return -ENODEV;
	}

	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg &= ~(S3C_CIGCTRL_TESTPATTERN_MASK | S3C_CIGCTRL_SELCAM_ITU_MASK | \
		S3C_CIGCTRL_SELCAM_MASK);

	/* Interface selection */
	if (cam->type == CAM_TYPE_MIPI) {
		cfg |= S3C_CIGCTRL_SELCAM_MIPI;
		writel(cam->fmt, ctrl->regs + S3C_CSIIMGFMT);
	} else if (cam->type == CAM_TYPE_ITU) {
		if (cam->id == CAMERA_PAR_A)
			cfg |= S3C_CIGCTRL_SELCAM_ITU_A;
		else
			cfg |= S3C_CIGCTRL_SELCAM_ITU_B;
		/* switch to ITU interface */
		cfg |= S3C_CIGCTRL_SELCAM_ITU;
	} else {
		dev_err(ctrl->dev, "[%s] invalid camera bus type selected\n", \
			__FUNCTION__);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_output_size(struct fimc_control *ctrl, int width, int height)
{
	u32 cfg= readl(ctrl->regs + S3C_CITRGFMT);

	cfg &= ~(S3C_CITRGFMT_TARGETH_MASK | S3C_CITRGFMT_TARGETV_MASK);

	cfg |= S3C_CITRGFMT_TARGETHSIZE(width);
	cfg |= S3C_CITRGFMT_TARGETVSIZE(height);

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

int fimc_hwset_output_colorspace(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_CITRGFMT);
	cfg &= ~S3C_CITRGFMT_OUTFORMAT_MASK;

	switch (pixelformat) {
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_RGB32:
		cfg |= S3C_CITRGFMT_OUTFORMAT_RGB;
		break;

	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
		cfg |= S3C_CITRGFMT_OUTFORMAT_YCBCR422_1PLANE;
		break;

	case V4L2_PIX_FMT_YUV422P:
		cfg |= S3C_CITRGFMT_OUTFORMAT_YCBCR422;
		break;

	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		cfg |= S3C_CITRGFMT_OUTFORMAT_YCBCR420;
		break;

	default:
		dev_err(ctrl->dev, "[%s] invalid pixel format\n", __FUNCTION__);
		break;
	}

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

/* FIXME */
int fimc_hwset_output_rot_flip(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	u32 cfg, val;

	cfg = readl(ctrl->regs + S3C_CITRGFMT);
	cfg &= ~S3C_CITRGFMT_FLIP_MASK;
	cfg &= ~S3C_CITRGFMT_OUTROT90_CLOCKWISE;

	val = fimc_mapping_rot_flip(rot, flip);

	if (val & 0x10)
		cfg |= S3C_CITRGFMT_OUTROT90_CLOCKWISE;

	if (val & 0x01)
		cfg |= S3C_CITRGFMT_FLIP_X_MIRROR;

	if (val & 0x02)
		cfg |= S3C_CITRGFMT_FLIP_Y_MIRROR;

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

int fimc_hwset_output_area(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = 0;

	cfg = S3C_CITAREA_TARGET_AREA(width * height);
	writel(cfg, ctrl->regs + S3C_CITAREA);

	return 0;
}

int fimc_hwset_enable_lastirq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIOCTRL);

	cfg |= S3C_CIOCTRL_LASTIRQ_ENABLE;
	writel(cfg, ctrl->regs + S3C_CIOCTRL);

	return 0;
}

int fimc_hwset_disable_lastirq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIOCTRL);

	cfg &= ~S3C_CIOCTRL_LASTIRQ_ENABLE;
	writel(cfg, ctrl->regs + S3C_CIOCTRL);

	return 0;
}

int fimc_hwset_prescaler(struct fimc_control *ctrl)
{
	struct fimc_scaler *sc = &ctrl->sc;
	u32 cfg = 0, shfactor;

	shfactor = 10 - (sc->hfactor + sc->vfactor);

	cfg |= S3C_CISCPRERATIO_SHFACTOR(shfactor);
	cfg |= S3C_CISCPRERATIO_PREHORRATIO(sc->pre_hratio);
	cfg |= S3C_CISCPRERATIO_PREVERRATIO(sc->pre_vratio);

	writel(cfg, ctrl->regs + S3C_CISCPRERATIO);

	cfg = 0;
	cfg |= S3C_CISCPREDST_PREDSTWIDTH(sc->pre_dst_width);
	cfg |= S3C_CISCPREDST_PREDSTHEIGHT(sc->pre_dst_height);

	writel(cfg, ctrl->regs + S3C_CISCPREDST);

	return 0;
}

int fimc_hwset_output_address(struct fimc_control *ctrl, int id,
				dma_addr_t base, struct v4l2_pix_format *fmt)
{
	dma_addr_t addr_y = 0, addr_cb = 0, addr_cr = 0;

	switch (fmt->pixelformat) {
	/* 1 plane formats */
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_RGB32:	/* fall through */
	case V4L2_PIX_FMT_YUYV:		/* fall through */
	case V4L2_PIX_FMT_UYVY:		/* fall through */
	case V4L2_PIX_FMT_VYUY:		/* fall through */
	case V4L2_PIX_FMT_YVYU:		/* fall through */
		addr_y = base;
		break;

	/* 2 plane formats */
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		addr_y = base;
		addr_cb = base + (fmt->width * fmt->height);
		break;

	/* 3 plane formats */
	case V4L2_PIX_FMT_YUV422P:
		addr_y = base;
		addr_cb = addr_y + (fmt->width * fmt->height);
		addr_cr = addr_cb + (fmt->width * fmt->height / 2);
		break;

	case V4L2_PIX_FMT_YUV420:
		addr_y = base;
		addr_cb = addr_y + (fmt->width * fmt->height);
		addr_cr = addr_cb + (fmt->width * fmt->height / 4);
		break;

	default:
		dev_err(ctrl->dev, "[%s] invalid pixel format (%08x)\n", \
			__FUNCTION__, fmt->pixelformat);
		break;
	}

	writel(addr_y, ctrl->regs + S3C_CIOYSA(id));
	writel(addr_cb, ctrl->regs + S3C_CIOCBSA(id));
	writel(addr_cr, ctrl->regs + S3C_CIOCRSA(id));

	return 0;
}

int fimc_hwset_output_yuv(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_CIOCTRL);
	cfg &= ~(S3C_CIOCTRL_ORDER2P_MASK | S3C_CIOCTRL_ORDER422_MASK | \
		S3C_CIOCTRL_YCBCR_PLANE_MASK);

	switch (pixelformat) {
	/* 1 plane formats */
	case V4L2_PIX_FMT_YUYV:
		cfg |= S3C_CIOCTRL_ORDER422_YCBYCR;
		break;

	case V4L2_PIX_FMT_UYVY:
		cfg |= S3C_CIOCTRL_ORDER422_CBYCRY;
		break;

	case V4L2_PIX_FMT_VYUY:
		cfg |= S3C_CIOCTRL_ORDER422_CRYCBY;
		break;

	case V4L2_PIX_FMT_YVYU:
		cfg |= S3C_CIOCTRL_ORDER422_YCRYCB;
		break;

	/* 2 plane formats */
	case V4L2_PIX_FMT_NV12:		/* fall through */
	case V4L2_PIX_FMT_NV16:
		cfg |= S3C_CIOCTRL_ORDER2P_LSB_CBCR;
		cfg |= S3C_CIOCTRL_YCBCR_2PLANE;
		break;

	case V4L2_PIX_FMT_NV21:		/* fall through */
	case V4L2_PIX_FMT_NV61:
		cfg |= S3C_CIOCTRL_ORDER2P_MSB_CBCR;
		cfg |= S3C_CIOCTRL_YCBCR_2PLANE;
		break;

	/* 3 plane formats */
	case V4L2_PIX_FMT_YUV422P:	/* fall through */
	case V4L2_PIX_FMT_YUV420:
		cfg |= S3C_CIOCTRL_YCBCR_3PLANE;
		break;
	}

	writel(cfg, ctrl->regs + S3C_CIOCTRL);

	return 0;
}

int fimc_hwset_input_rot(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	u32 cfg, val;

	cfg = readl(ctrl->regs + S3C_CITRGFMT);
	cfg &= ~S3C_CITRGFMT_INROT90_CLOCKWISE;

	val = fimc_mapping_rot_flip(rot, flip);

	if (val & 0x10)
		cfg |= S3C_CITRGFMT_INROT90_CLOCKWISE;

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

int fimc_hwset_scaler(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~(S3C_CISCCTRL_SCALERBYPASS | \
		S3C_CISCCTRL_SCALEUP_H | S3C_CISCCTRL_SCALEUP_V | \
		S3C_CISCCTRL_MAIN_V_RATIO_MASK | S3C_CISCCTRL_MAIN_H_RATIO_MASK);
	cfg |= (S3C_CISCCTRL_CSCR2Y_WIDE | S3C_CISCCTRL_CSCY2R_WIDE);

	if (ctrl->sc.bypass)
		cfg |= S3C_CISCCTRL_SCALERBYPASS;

	if (ctrl->sc.scaleup_h)
		cfg |= S3C_CISCCTRL_SCALEUP_H;

	if (ctrl->sc.scaleup_v)
		cfg |= S3C_CISCCTRL_SCALEUP_V;

	cfg |= S3C_CISCCTRL_MAINHORRATIO(ctrl->sc.main_hratio);
	cfg |= S3C_CISCCTRL_MAINVERRATIO(ctrl->sc.main_vratio);

	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_enable_lcdfifo(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg |= S3C_CISCCTRL_LCDPATHEN_FIFO;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_disable_lcdfifo(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~S3C_CISCCTRL_LCDPATHEN_FIFO;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwget_frame_count(struct fimc_control *ctrl)
{
	int num;

	num = S3C_CISTATUS_GET_FRAME_COUNT(readl(ctrl->regs + S3C_CISTATUS));

	dev_dbg(ctrl->dev, "[%s] frame count: %d\n", __FUNCTION__, num);
	
	return num;
}

int fimc_hwget_frame_end(struct fimc_control *ctrl)
{
        unsigned long timeo = jiffies;
	u32 cfg;

        timeo += 20;    /* waiting for 100ms */
	while (time_before(jiffies, timeo)) {
		cfg = readl(ctrl->regs + S3C_CISTATUS);
		
		if (S3C_CISTATUS_GET_FRAME_END(cfg)) {
			cfg &= ~S3C_CISTATUS_FRAMEEND;
			writel(cfg, ctrl->regs + S3C_CISTATUS);
			break;
		}
		cond_resched();
	}

	return 0;
}

int fimc_hwget_last_frame_end(struct fimc_control *ctrl)
{
        unsigned long timeo = jiffies;
	u32 cfg;

        timeo += 20;    /* waiting for 100ms */
	while (time_before(jiffies, timeo)) {
		cfg = readl(ctrl->regs + S3C_CISTATUS);
		
		if (S3C_CISTATUS_GET_LAST_CAPTURE_END(cfg)) {
			cfg &= ~S3C_CISTATUS_LASTCAPTUREEND;
			writel(cfg, ctrl->regs + S3C_CISTATUS);
			break;
		}
		cond_resched();
	}

	return 0;
}

int fimc_hwset_start_scaler(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg |= S3C_CISCCTRL_SCALERSTART;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_stop_scaler(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~S3C_CISCCTRL_SCALERSTART;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_input_rgb(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_INRGB_FMT_RGB_MASK;

	if (pixelformat == V4L2_PIX_FMT_RGB32) {
		cfg |= S3C_CISCCTRL_INRGB_FMT_RGB888;
	} else if (pixelformat == V4L2_PIX_FMT_RGB565) {
		cfg |= S3C_CISCCTRL_INRGB_FMT_RGB565;
	}

	writel(cfg, ctrl->regs + S3C_CISCCTRL);
	
	return 0;
}

int fimc_hwset_output_rgb(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_OUTRGB_FMT_RGB_MASK;

	if (pixelformat == V4L2_PIX_FMT_RGB32) {
		cfg |= S3C_CISCCTRL_OUTRGB_FMT_RGB888;
	} else if (pixelformat == V4L2_PIX_FMT_RGB565) {
		cfg |= S3C_CISCCTRL_OUTRGB_FMT_RGB565;
	}

	writel(cfg, ctrl->regs + S3C_CISCCTRL);
	
	return 0;
}

int fimc_hwset_ext_rgb(struct fimc_control *ctrl, int enable)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_EXTRGB_EXTENSION;

	if (enable)
		cfg |= S3C_CISCCTRL_EXTRGB_EXTENSION;

	writel(cfg, ctrl->regs + S3C_CISCCTRL);
	
	return 0;
}

int fimc_hwset_enable_capture(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIIMGCPT);
	cfg &= ~S3C_CIIMGCPT_IMGCPTEN_SC;
	cfg |= S3C_CIIMGCPT_IMGCPTEN;

	if (!ctrl->sc.bypass)
		cfg |= S3C_CIIMGCPT_IMGCPTEN_SC;

	writel(cfg, ctrl->regs + S3C_CIIMGCPT);

	return 0;
}

int fimc_hwset_disable_capture(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIIMGCPT);

	cfg &= ~(S3C_CIIMGCPT_IMGCPTEN_SC | S3C_CIIMGCPT_IMGCPTEN);

	writel(cfg, ctrl->regs + S3C_CIIMGCPT);

	return 0;
}

int fimc_hwset_input_address(struct fimc_control *ctrl, dma_addr_t base, \
						struct v4l2_pix_format *fmt)
{
	dma_addr_t addr_y = 0, addr_cb = 0, addr_cr = 0;

	switch (fmt->pixelformat) {
	/* 1 plane formats */
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_RGB32:	/* fall through */
	case V4L2_PIX_FMT_YUYV:		/* fall through */
	case V4L2_PIX_FMT_UYVY:		/* fall through */
	case V4L2_PIX_FMT_VYUY:		/* fall through */
	case V4L2_PIX_FMT_YVYU:		/* fall through */
		addr_y = base;
		break;

	  /* 2 plane formats */
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		addr_y = base;
		addr_cb = addr_y + (fmt->width * fmt->height);
		break;

	  /* 3 plane formats */
	case V4L2_PIX_FMT_YUV422P:
		addr_y = base;
		addr_cb = addr_y + (fmt->width * fmt->height);
		addr_cr = addr_cb + (fmt->width * fmt->height / 2);
		break;

	case V4L2_PIX_FMT_YUV420:
		addr_y = base;
		addr_cb = addr_y + (fmt->width * fmt->height);
		addr_cr = addr_cb + (fmt->width * fmt->height / 4);
		break;

	default:
		dev_err(ctrl->dev, "[%s] invalid pixel format\n", __FUNCTION__);
		break;
	}

	writel(addr_y, ctrl->regs + S3C_CIIYSA0);
	writel(addr_cb, ctrl->regs + S3C_CIICBSA0);
	writel(addr_cr, ctrl->regs + S3C_CIICRSA0);

	return 0;
}

int fimc_hwset_enable_autoload(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg |= S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_disable_autoload(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg &= ~S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_real_input_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);
	cfg &= ~ (S3C_CIREAL_ISIZE_HEIGHT_MASK | S3C_CIREAL_ISIZE_WIDTH_MASK);

	cfg |= S3C_CIREAL_ISIZE_WIDTH(width);
	cfg |= S3C_CIREAL_ISIZE_HEIGHT(height);

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_addr_change_enable(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg &= ~S3C_CIREAL_ISIZE_ADDR_CH_DISABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_addr_change_disable(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg |= S3C_CIREAL_ISIZE_ADDR_CH_DISABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_input_burst_cnt(struct fimc_control *ctrl, u32 cnt)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_BURST_CNT_MASK;

	if (cnt > 4) 
		cnt = 4;
	else if (cnt == 0)
		cnt = 4;

	cfg |= S3C_MSCTRL_SUCCESSIVE_COUNT(cnt);
	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_colorspace(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_INFORMAT_RGB;

	/* Color format setting */
	if (pixelformat == V4L2_PIX_FMT_NV12) {
		cfg |= S3C_MSCTRL_INFORMAT_YCBCR420;
	} else if ((pixelformat == V4L2_PIX_FMT_RGB32) || \
					(pixelformat == V4L2_PIX_FMT_RGB565)) {
		cfg |= S3C_MSCTRL_INFORMAT_RGB;
	} else {
		dev_err(ctrl->dev, "[%s]Invalid pixelformt : %d\n", 
				__FUNCTION__, pixelformat);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_yuv(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~(S3C_MSCTRL_2PLANE_SHIFT_MASK | S3C_MSCTRL_C_INT_IN_2PLANE | \
						S3C_MSCTRL_ORDER422_YCBYCR);

	switch (pixelformat) {
	case V4L2_PIX_FMT_NV12:
		cfg |= S3C_MSCTRL_C_INT_IN_2PLANE;
		break;

	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	default:
		break;
	}

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_flip(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	u32 cfg, val;

	cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~(S3C_MSCTRL_FLIP_X_MIRROR | S3C_MSCTRL_FLIP_Y_MIRROR);
	val = fimc_mapping_rot_flip(rot, flip);

	if(val & 0x01)
		cfg |= S3C_MSCTRL_FLIP_X_MIRROR;

	if(val & 0x02)
		cfg |= S3C_MSCTRL_FLIP_Y_MIRROR;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_source(struct fimc_control *ctrl, enum fimc_input path)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_INPUT_MASK;

	if (path == FIMC_SRC_MSDMA) {
		cfg |= S3C_MSCTRL_INPUT_MEMORY;
	} else if (path == FIMC_SRC_CAM) {
		cfg |= S3C_MSCTRL_INPUT_EXTCAM;
	}

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;

}

int fimc_hwset_start_input_dma(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg |= S3C_MSCTRL_ENVID;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_stop_input_dma(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_ENVID;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_output_offset(struct fimc_control *ctrl, u32 pixelformat,
				struct v4l2_rect *bound, struct v4l2_rect *crop)
{
	u32 cfg_y = 0, cfg_cb = 0;

	if (crop->left || crop->top || (bound->width != crop->width) || \
		(bound->height != crop->height)) {
		if (pixelformat == V4L2_PIX_FMT_RGB32) {
			cfg_y |= S3C_CIOYOFF_HORIZONTAL(crop->left * 4);
			cfg_y |= S3C_CIOYOFF_VERTICAL(crop->top);
		} else if (pixelformat == V4L2_PIX_FMT_RGB565) {
			cfg_y |= S3C_CIOYOFF_HORIZONTAL(crop->left * 2);
			cfg_y |= S3C_CIOYOFF_VERTICAL(crop->top);
		}
	}

	writel(cfg_y, ctrl->regs + S3C_CIOYOFF);
	writel(cfg_cb, ctrl->regs + S3C_CIOCBOFF);

	return 0;
}

int fimc_hwset_input_offset(struct fimc_control *ctrl, u32 pixelformat,
				struct v4l2_rect *bound, struct v4l2_rect *crop)
{
	u32 cfg_y = 0, cfg_cb = 0;

	if (crop->left || crop->top || \
		(bound->width != crop->width) || (bound->height != crop->height)) {
		if (pixelformat == V4L2_PIX_FMT_NV12) {
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop->left);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop->top);
			cfg_cb |= S3C_CIICBOFF_HORIZONTAL(crop->left);
			cfg_cb |= S3C_CIICBOFF_VERTICAL(crop->top / 2);
		} else if (pixelformat == V4L2_PIX_FMT_RGB32) {
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop->left * 4);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop->top);
		} else if (pixelformat == V4L2_PIX_FMT_RGB565) {
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop->left * 2);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop->top);
		}
	}

	writel(cfg_y, ctrl->regs + S3C_CIIYOFF);
	writel(cfg_cb, ctrl->regs + S3C_CIICBOFF);

	return 0;
}

int fimc_hwset_org_input_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = 0;

	cfg |= S3C_ORGISIZE_HORIZONTAL(width);
	cfg |= S3C_ORGISIZE_VERTICAL(height);

	writel(cfg, ctrl->regs + S3C_ORGISIZE);

	return 0;
}

int fimc_hwset_org_output_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = 0;

	cfg |= S3C_ORGOSIZE_HORIZONTAL(width);
	cfg |= S3C_ORGOSIZE_VERTICAL(height);

	writel(cfg, ctrl->regs + S3C_ORGOSIZE);

	return 0;
}

int fimc_hwset_ext_output_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = readl(ctrl->regs + S3C_CIEXTEN);

	cfg &= ~S3C_CIEXTEN_TARGETH_EXT_MASK;
	cfg &= ~S3C_CIEXTEN_TARGETV_EXT_MASK;
	cfg |= S3C_CIEXTEN_TARGETH_EXT(width);
	cfg |= S3C_CIEXTEN_TARGETV_EXT(height);

	writel(cfg, ctrl->regs + S3C_CIEXTEN);

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

static void fimc_reset_cfg(struct fimc_control *ctrl)
{
	int i;
	u32 cfg[][2] = {
		{ 0x018, 0x00000000 }, { 0x01c, 0x00000000 },
		{ 0x020, 0x00000000 }, { 0x024, 0x00000000 },
		{ 0x028, 0x00000000 }, { 0x02c, 0x00000000 },
		{ 0x030, 0x00000000 }, { 0x034, 0x00000000 },
		{ 0x038, 0x00000000 }, { 0x03c, 0x00000000 },
		{ 0x040, 0x00000000 }, { 0x044, 0x00000000 },
		{ 0x048, 0x00000000 }, { 0x04c, 0x00000000 },
		{ 0x050, 0x00000000 }, { 0x054, 0x00000000 },
		{ 0x058, 0x18000000 }, { 0x05c, 0x00000000 },
		{ 0x0c0, 0x00000000 }, { 0x0c4, 0xffffffff },
		{ 0x0d0, 0x00100080 }, { 0x0d4, 0x00000000 },
		{ 0x0d8, 0x00000000 }, { 0x0dc, 0x00000000 },
		{ 0x0f8, 0x00000000 }, { 0x0fc, 0x04000000 },
		{ 0x168, 0x00000000 }, { 0x16c, 0x00000000 },
		{ 0x170, 0x00000000 }, { 0x174, 0x00000000 },
		{ 0x178, 0x00000000 }, { 0x17c, 0x00000000 },
		{ 0x180, 0x00000000 }, { 0x184, 0x00000000 },
		{ 0x188, 0x00000000 }, { 0x18c, 0x00000000 },
		{ 0x194, 0x0000001e },
	};

	for (i = 0; i < sizeof(cfg) / 8; i++)
		writel(cfg[i][1], ctrl->regs + cfg[i][0]);
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

	fimc_reset_cfg(ctrl);
}

