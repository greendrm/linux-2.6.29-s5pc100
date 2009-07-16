/* linux/drivers/media/video/samsung/fimc_core.c
 *
 * Core file for Samsung Camera Interface (FIMC) driver
 *
 * Dongsoo Kim, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/sec/
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * Note: This driver supports common i2c client driver style
 * which uses i2c_board_info for backward compatibility and
 * new v4l2_subdev as well.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <media/v4l2-device.h>

#include <asm/io.h>
#include <asm/memory.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <plat/fimc2.h>

#include "fimc.h"



void fimc_register_camera(struct s3c_platform_camera *cam)
{
	fimc_dev->camera[cam->id] = cam;

	clk_disable(fimc_dev->mclk);
	clk_set_rate(fimc_dev->mclk, cam->clk_rate);
	clk_enable(fimc_dev->mclk);

//	fimc_reset_camera();
}

void fimc_unregister_camera(struct s3c_platform_camera *cam)
{
	struct fimc_control *ctrl;
	int i;

	for (i = 0; i < FIMC_DEVICES; i++) {
		ctrl = get_fimc_ctrl(i);
		if (ctrl->cam == cam)
			ctrl->cam = NULL;
	}

	fimc_dev->camera[cam->id] = NULL;
}

void fimc_set_active_camera(struct fimc_control *ctrl, int id)
{
	ctrl->cam = fimc_dev->camera[id];

	dev_info(ctrl->dev, "requested id: %d\n", id);
	
	if (ctrl->cam && id < FIMC_TPID)
		fimc_select_camera(ctrl);
}

static irqreturn_t fimc_irq(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}

static
struct fimc_control *fimc_register_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct resource *res;
	int id = pdev->id, irq;

	pdata = to_fimc_plat(&pdev->dev);

	ctrl = get_fimc_ctrl(id);
	ctrl->id = id;
	ctrl->dev = &pdev->dev;
	ctrl->vd = &fimc_video_device[id];
	ctrl->vd->minor = id;
	ctrl->mem.base = s3c_get_media_memory(S3C_MDEV_FIMC);
	ctrl->mem.len = s3c_get_media_memsize(S3C_MDEV_FIMC);
	ctrl->mem.curr = ctrl->mem.base;
	ctrl->status = FIMC_STREAMOFF;

	sprintf(ctrl->name, "%s%d", FIMC_NAME, id);
	strcpy(ctrl->vd->name, ctrl->name);

	atomic_set(&ctrl->in_use, 0);
	mutex_init(&ctrl->lock);
	mutex_init(&ctrl->v4l2_lock);
	spin_lock_init(&ctrl->lock_in);
	spin_lock_init(&ctrl->lock_out);
	init_waitqueue_head(&ctrl->wq);

	/* get resource for io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(ctrl->dev, "%s: failed to get io memory region\n", \
			__FUNCTION__);
		return NULL;
	}

	/* request mem region */
	res = request_mem_region(res->start, res->end - \
					res->start + 1, pdev->name);
	if (!res) {
		dev_err(ctrl->dev, "%s: failed to request io memory region\n", \
			__FUNCTION__);
		return NULL;
	}

	/* ioremap for register block */
	ctrl->regs = ioremap(res->start, res->end - res->start + 1);
	if (!ctrl->regs) {
		dev_err(ctrl->dev, "%s: failed to remap io region\n", \
			__FUNCTION__);
		return NULL;
	}

	/* irq */
	irq = platform_get_irq(pdev, 0);
	if (request_irq(irq, fimc_irq, IRQF_DISABLED, ctrl->name, ctrl))
		dev_err(ctrl->dev, "%s: request_irq failed\n", __FUNCTION__);

	fimc_reset(ctrl);

	return ctrl;
}

static int fimc_unregister_controller(struct platform_device *pdev)
{
	struct fimc_control *ctrl;
	int id = pdev->id;

	ctrl = get_fimc_ctrl(id);
	iounmap(ctrl->regs);
	memset(ctrl, 0, sizeof(*ctrl));

	return 0;
}

static int fimc_mmap(struct file* filp, struct vm_area_struct *vma)
{
	return 0;
}

static u32 fimc_poll(struct file *filp, poll_table *wait)
{
	return 0;
}

static
ssize_t fimc_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
	return 0;
}

static
ssize_t fimc_write(struct file *filp, const char *b, size_t c, loff_t *offset)
{
	return 0;
}

static int fimc_open(struct file *filp)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int ret;

	ctrl = video_get_drvdata(video_devdata(filp));
	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	if (atomic_read(&ctrl->in_use)) {
		ret = -EBUSY;
		goto resource_busy;
	} else {
		atomic_inc(&ctrl->in_use);
	}

	/* Apply things to interface register */
	fimc_reset(ctrl);
	filp->private_data = ctrl;

	mutex_unlock(&ctrl->lock);

	return 0;

resource_busy:
	mutex_unlock(&ctrl->lock);
	return ret;
}

static int fimc_release(struct file *filp)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;

	ctrl = (struct fimc_control *) filp->private_data;
	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	atomic_dec(&ctrl->in_use);
	filp->private_data = NULL;

	/* Shutdown the MCLK */
	clk_disable(fimc_dev->mclk);

	/* FIXME: turning off actual working camera */
	if (ctrl->cam && fimc_dev->camera[ctrl->cam->id]->cam_power)
		fimc_dev->camera[ctrl->cam->id]->cam_power(0);

	mutex_unlock(&ctrl->lock);
	
	printk(KERN_INFO "successfully released\n");
	return 0;
}

static const struct v4l2_file_operations fimc_fops = {
	.owner = THIS_MODULE,
	.open = fimc_open,
	.release = fimc_release,
	.ioctl = video_ioctl2,
	.read = fimc_read,
	.write = fimc_write,
	.mmap = fimc_mmap,
	.poll = fimc_poll,
};

static void fimc_vdev_release(struct video_device *vdev)
{
	kfree(vdev);
}

struct video_device fimc_video_device[FIMC_DEVICES] = {
	[0] = {
		.fops = &fimc_fops,
//		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[1] = {
		.fops = &fimc_fops,
//		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[2] = {
		.fops = &fimc_fops,
//		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
};

static int fimc_init_global(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	int i;

	pdata = to_fimc_plat(&pdev->dev);

	/* mclk */
	fimc_dev->mclk = clk_get(&pdev->dev, pdata->mclk_name);
	if (IS_ERR(fimc_dev->mclk)) {
		dev_err(&pdev->dev, "%s: failed to get mclk source\n", \
			__FUNCTION__);
		return -EINVAL;
	}

	/* Registering external camera modules. re-arrange order to be sure */
	for (i = 0; i < FIMC_MAXCAMS; i++) {
		if (!pdata->camera[i])
			break;

		/* Assign camera device to fimc */
		fimc_dev->camera[pdata->camera[i]->id] = pdata->camera[i];
	}

	fimc_dev->initialized = 1;

	return 0;
}

/*
 * Assign v4l2 device and subdev to fimc
 * it is called per every fimc ctrl registering
 */
static int fimc_configure_subdev(struct platform_device *pdev, int id)
{
	struct s3c_platform_fimc *pdata;
	struct i2c_adapter *i2c_adap;
	struct i2c_board_info *i2c_info;
	struct v4l2_subdev *sd;
	struct fimc_control *ctrl;
	unsigned short addr;
	char *name;

	pdata = to_fimc_plat(&pdev->dev);
	ctrl = get_fimc_ctrl(id);

	/* Subdev registration */
	if (pdata->camera[id]) {
		i2c_adap = i2c_get_adapter(pdata->camera[id]->i2c_busnum);
		if (!i2c_adap) {
			dev_info(&pdev->dev, "subdev i2c_adapter " \
					"missing-skip registration\n");
		}

		i2c_info = pdata->camera[id]->info;
		if (!i2c_info) {
			dev_err(&pdev->dev, "%s: subdev i2c board info missing\n", \
				__FUNCTION__);
			return -ENODEV;
		}

		name = i2c_info->type;
		if (!name) {
			dev_info(&pdev->dev, "subdev i2c dirver name " \
					"missing-skip registration\n");
			return -ENODEV;
		}

		addr = i2c_info->addr;
		if (!addr) {
			dev_info(&pdev->dev, "subdev i2c address " \
					"missing-skip registration\n");
			return -ENODEV;
		}

		/*
		 * NOTE: first time subdev being registered,
		 * s_config is called and try to initialize subdev device
		 * but in this point, we are not giving MCLK and power to subdev
		 * so nothing happens but pass platform data through
		 */
		sd = v4l2_i2c_new_subdev_board(&ctrl->v4l2_dev, i2c_adap, \
				name, i2c_info, &addr);
		if (!sd) {
			dev_err(&pdev->dev, \
				"%s: v4l2 subdev board registering failed\n", \
				__FUNCTION__);
		}

		/* Assign camera device to fimc */
		fimc_dev->camera[pdata->camera[id]->id] = pdata->camera[id];
	}

	return 0;
}

static int __devinit fimc_probe(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct clk *srclk;
	int ret;

	if (!fimc_dev) {
		fimc_dev = kzalloc(sizeof(*fimc_dev), GFP_KERNEL);
		if (!fimc_dev) {
			dev_err(&pdev->dev, "%s: not enough memory\n", \
				__FUNCTION__);
			goto err_fimc;
		}
	}

	ctrl = fimc_register_controller(pdev);
	if (!ctrl) {
		dev_err(&pdev->dev, "%s: cannot register fimc controller\n", \
			__FUNCTION__);
		goto err_fimc;
	}

	pdata = to_fimc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* fimc source clock */
	srclk = clk_get(&pdev->dev, pdata->srclk_name);
	if (IS_ERR(srclk)) {
		dev_err(&pdev->dev, "%s: failed to get source clock of fimc\n", \
			__FUNCTION__);
		goto err_clk_io;
	}

	/* fimc clock */
	ctrl->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(ctrl->clock)) {
		dev_err(&pdev->dev, "%s: failed to get fimc clock source\n", \
			__FUNCTION__);
		goto err_clk_io;
	}

	/* set parent clock */
	if (ctrl->clock->set_parent)
		ctrl->clock->set_parent(ctrl->clock, srclk);

	/* set clockrate for fimc interface block */
	if (ctrl->clock->set_rate) {
		ctrl->clock->set_rate(ctrl->clock, pdata->clk_rate);
		dev_info(&pdev->dev, "fimc set clock rate to %d\n", \
				pdata->clk_rate);
	}

	clk_enable(ctrl->clock);

	/* V4L2 device-subdev registration */
	ret = v4l2_device_register(&pdev->dev, &ctrl->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "%s: v4l2 device register failed\n", \
			__FUNCTION__);
		goto err_clk_io;
	}

	/* things to initialize once */
	if (!fimc_dev->initialized) {
		ret = fimc_init_global(pdev);
		if (ret)
			goto err_global;
	}

	/* v4l2 subdev configuration */
	ret = fimc_configure_subdev(pdev, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "%s: subdev[%d] registering failed\n", \
			__FUNCTION__, ctrl->id);
	}

	/* video device register */
	ret = video_register_device(ctrl->vd, VFL_TYPE_GRABBER, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "%s: cannot register video driver\n", \
			__FUNCTION__);
		goto err_video;
	}

	video_set_drvdata(ctrl->vd, ctrl);

	dev_info(&pdev->dev, "controller %d registered successfully\n", \
		ctrl->id);

	return 0;

err_video:
	clk_put(fimc_dev->mclk);

err_global:
	clk_disable(ctrl->clock);
	clk_put(ctrl->clock);

err_clk_io:
	fimc_unregister_controller(pdev);

err_fimc:
	return -EINVAL;

}

static int fimc_remove(struct platform_device *pdev)
{
	fimc_unregister_controller(pdev);
	kfree(fimc_dev);

	return 0;
}

int fimc_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

int fimc_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver fimc_driver = {
	.probe		= fimc_probe,
	.remove		= fimc_remove,
	.suspend	= fimc_suspend,
	.resume		= fimc_resume,
	.driver		= {
		.name	= FIMC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int fimc_register(void)
{
	platform_driver_register(&fimc_driver);

	return 0;
}

static void fimc_unregister(void)
{
	platform_driver_unregister(&fimc_driver);
}

late_initcall(fimc_register);
module_exit(fimc_unregister);

MODULE_AUTHOR("Dongsoo, Kim <dongsoo45.kim@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Camera Interface (FIMC) driver");
MODULE_LICENSE("GPL");

