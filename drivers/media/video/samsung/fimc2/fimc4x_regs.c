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
