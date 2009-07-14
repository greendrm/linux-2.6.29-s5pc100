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
void fimc_select_camera(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg &= ~(S3C_CIGCTRL_TESTPATTERN_MASK | S3C_CIGCTRL_SELCAM_ITU_MASK);

	if (ctrl->cam->id == 0)
		cfg |= S3C_CIGCTRL_SELCAM_ITU_A;
	else
		cfg |= S3C_CIGCTRL_SELCAM_ITU_B;

	/* Interface selection */
	/* FIXME: Check whether SELCAM_ITU_A or B is necessary when we use MIPI */
	/* TODO: Input source selection is necessary */
	if (ctrl->cam->type == CAM_TYPE_MIPI)
		cfg |= S3C_CIGCTRL_SELCAM_MIPI;
	else if (ctrl->cam->type == CAM_TYPE_ITU) {
		if (ctrl->cam->id == 0)
			cfg |= S3C_CIGCTRL_SELCAM_ITU_A;
		else
			cfg |= S3C_CIGCTRL_SELCAM_ITU_B;
		/* switch to ITU interface */
		cfg |= S3C_CIGCTRL_SELCAM_ITU;
	} else
		dev_err(ctrl->dev, "invalid camera bus type selected\n");

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	/* configure input source format */
	if (ctrl->cam->type == CAM_TYPE_ITU) {
		cfg = readl(ctrl->regs + S3C_CISRCFMT);
		cfg |= ctrl->cam->fmt;
		writel(cfg, ctrl->regs + S3C_CISRCFMT);
	} else if (ctrl->cam->type == CAM_TYPE_MIPI) {
		cfg = readl(ctrl->regs + S3C_CSIIMGFMT);
		cfg |= ctrl->cam->fmt;
		writel(cfg, ctrl->regs + S3C_CSIIMGFMT);
	} else
		dev_err(ctrl->dev, "invalid camera bus type selected\n");
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

#if 0
int fimc_set_src_format(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_dst_format(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

#if 0
int fimc_set_src_path(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_dst_path(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_src_addr(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_dst_addr(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_src_crop(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	if (ctrl->out != NULL) {
		/* To Do : crop setting */
		cfg = readl(ctrl->regs + S3C_CIWDOFST);
		cfg = 0;
		writel(cfg, ctrl->regs + S3C_CIWDOFST);

		cfg = readl(ctrl->regs + S3C_CIWDOFST2);
		cfg = 0;
		writel(cfg, ctrl->regs + S3C_CIWDOFST2);
	} else if (ctrl->cap != NULL) {

	} else {
		dev_err(ctrl->dev, "[%s] Invalid case.\n", __FUNCTION__);
		ret = -EINVAL;
	}

	return ret;
}

int fimc_set_dst_crop(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_scaler(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_inupt_rotate(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_set_output_rotate(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_start_scaler(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}

int fimc_stop_scaler(struct fimc_control *ctrl)
{
	u32 cfg = 0;
	int ret = 0;

	dev_dbg(ctrl->dev, "[%s] called\n", __FUNCTION__);

	return ret;
}
#endif
