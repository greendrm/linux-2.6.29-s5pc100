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
#include <linux/platform_device.h>
#include <media/v4l2-device.h>

#include <asm/io.h>
#include <asm/memory.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <plat/fimc2.h>

#include "fimc.h"

static struct fimc_global *fimc;

struct s3c_platform_fimc *to_fimc_plat(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return (struct s3c_platform_fimc *) pdev->dev.platform_data;
}

inline struct fimc_control *get_fimc(int id)
{
	return &fimc->ctrl[id];
}

void fimc_register_camera(struct s3c_platform_camera *cam)
{
	fimc->camera[cam->id] = cam;

	clk_disable(fimc->mclk);
	clk_set_rate(fimc->mclk, cam->clk_rate);
	clk_enable(fimc->mclk);

//	fimc_reset_camera();
}

void fimc_unregister_camera(struct s3c_platform_camera *cam)
{
	struct fimc_control *ctrl;
	int i;

	for (i = 0; i < FIMC_DEVICES; i++) {
		ctrl = get_fimc(i);
		if (ctrl->cam == cam)
			ctrl->cam = NULL;
	}

	fimc->camera[cam->id] = NULL;
}

void fimc_set_active_camera(struct fimc_control *ctrl, int id)
{
	ctrl->cam = fimc->camera[id];

	dev_info(ctrl->dev, "requested id: %d\n", id);
	
//	if (ctrl->cam && id < FIMC_TPID)
//		fimc_select_camera(ctrl);
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

	ctrl = get_fimc(id);
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
		dev_err(ctrl->dev, "failed to get io memory region\n");
		return NULL;
	}

	/* request mem region */
	res = request_mem_region(res->start, res->end - \
					res->start + 1, pdev->name);
	if (!res) {
		dev_err(ctrl->dev, "failed to request io memory region\n");
		return NULL;
	}

	/* ioremap for register block */
	ctrl->regs = ioremap(res->start, res->end - res->start + 1);
	if (!ctrl->regs) {
		dev_err(ctrl->dev, "failed to remap io region\n");
		return NULL;
	}

	/* irq */
	irq = platform_get_irq(pdev, 0);
	if (request_irq(irq, fimc_irq, IRQF_DISABLED, ctrl->name, ctrl))
		dev_err(ctrl->dev, "request_irq failed\n");

//	fimc_reset(ctrl);

	return ctrl;
}

static int fimc_unregister_controller(struct platform_device *pdev)
{
	struct fimc_control *ctrl;
	int id = pdev->id;

	ctrl = get_fimc(id);
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

/*
 * ctrl->id means FIMC[id]
 * camera->id means camera A/B/C
 * Openening device node is to open FIMC device
 * and after opening device, VIDIOC_S_INPUT is necessary
 * to make a choise between input camera devices
 * 	before VIDIOC_S_INPUT: pdata->camera is used
 * 	after VIDIOC_S_INPUT: fimc->camera is used
 */
static int fimc_open(struct file *filp)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int minor, ret;

	minor = video_devdata(filp)->minor;

	/* TODO: remove get fimc with minor
	 * how to get the proper fimc controller?
	*/
	ctrl = get_fimc(minor);

	pdata = to_fimc_plat(ctrl->dev);

	/* TODO: condition check for multiple open with same external camera
	 * TODO: condition check for multiple open with same controller
	 */
	/*
	 * Check for external camera device
	 * NOTE: scaler only feature should be implemented seperately
	 * 	so, no external camera no device node for camera
	 */
	if (!fimc->camera[pdata->default_cam]) {
		dev_err(ctrl->dev, "no external camera device\n");
		return -ENODEV;
	}

	/*
	 * Check for device power control
	 * For now we just check according to fimc number
	 * because of the default camera issue
	 * Don't forget to give proper power after VIDIOC_S_INPUT called
	 */
	if (!fimc->camera[pdata->default_cam]->cam_power) {
		dev_err(ctrl->dev, "no way to control camera[%d]'s power\n", \
			ctrl->id);
		return -ENODEV;
	}

	mutex_lock(&ctrl->lock);

	if (atomic_read(&ctrl->in_use)) {
		ret = -EBUSY;
		goto resource_busy;
	} else {
		atomic_inc(&ctrl->in_use);
	}

	/*
	 * Giving MCLK to camera
	 * clk_enable(camera clock)
	 * clk_set_rate(camera clock, expecting sensor clock)
	 */
	/*
	 * FIXME: default camera policy is necessary
	 * by now, default external camera id follows controller's id
	 */
	clk_set_rate(fimc->mclk, fimc->camera[pdata->default_cam]->clk_rate);
	clk_enable(fimc->mclk);

	/* FIXME: Giving power */
	fimc->camera[pdata->default_cam]->cam_power(1);

	/*
	 * Default camera attach
	 * According to ctrl->cam (ctrl->id?)
	 * In this phase we don't know what user choose using
	 * VIDIOC_S_INPUT so just attach default camera here
	 * FIXME: This is just to be safe
	 * proper camera selection should be made 
	 * through VIDIOC_S_INPUT
	 */
	fimc_set_active_camera(ctrl, pdata->default_cam);

	/*
	 * Now Configuring external camera module(subdev)
	 * For now, we have "default camera" concept so,
	 * we directly access to the index of default camera's subdev
	 *
	 * int (*s_config)(struct v4l2_subdev *sd, int irq, void *platform_data);
	 */
	ret = v4l2_subdev_call(ctrl->cam->sd, core, s_config,
			ctrl->cam->info->irq,
			ctrl->cam->info->platform_data);

	if (ret == -ENOIOCTLCMD)
		dev_err(ctrl->dev, "s_config subdev api not supported\n");

	/* Apply things to interface register */
//	fimc_reset(ctrl);
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
	clk_disable(fimc->mclk);

	/* FIXME: turning off actual working camera */
	fimc->camera[ctrl->cam->id]->cam_power(0);

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

	if (fimc->initialized)
		return 0;

	pdata = to_fimc_plat(&pdev->dev);

	/* mclk */
	fimc->mclk = clk_get(&pdev->dev, pdata->mclk_name);
	if (IS_ERR(fimc->mclk)) {
		dev_err(&pdev->dev, "failed to get mclk source\n");
		return -EINVAL;
	}

	/* Registering external camera modules. re-arrange order to be sure */
	for (i = 0; i < FIMC_MAXCAMS; i++) {
		if (!pdata->camera[i])
			break;

		/* Assign camera device to fimc */
		fimc->camera[pdata->camera[i]->id] = pdata->camera[i];
	}

	fimc->initialized = 1;

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
	ctrl = get_fimc(id);

	/* Subdev registration */
	if (pdata->camera[id]) {
		i2c_adap = i2c_get_adapter(pdata->camera[id]->i2c_busnum);
		if (!i2c_adap) {
			dev_info(&pdev->dev, "subdev i2c_adapter " \
					"missing-skip registration\n");
		}

		i2c_info = pdata->camera[id]->info;
		name = pdata->camera[id]->info->type;
		if (!name) {
			dev_info(&pdev->dev, "subdev i2c driver name " \
					"missing-skip registration\n");
		}

		addr = pdata->camera[id]->info->addr;
		if (!addr) {
			dev_info(&pdev->dev, "subdev i2c address " \
					"missing-skip registration\n");
		}

		/* NOTE: first time subdev being registered,
		 * s_config is called and try to initialize subdev device
		 * but in this point, we are not giving MCLK and power to subdev
		 * so nothing happens.
		 * try v4l2_subdev_call with s_config core to initialize after
		 * giving power source
		 */
		sd = v4l2_i2c_new_subdev_board(&ctrl->v4l2_dev, i2c_adap, \
				name, i2c_info, &addr);
		if (!sd) {
			dev_err(&pdev->dev, \
				"v4l2 subdev board registering failed\n");
		}

		/* Assign probed subdev pointer to fimc */
		fimc->sd[pdata->camera[id]->id] = sd;

		/* Assign camera device to fimc */
		fimc->camera[pdata->camera[id]->id] = pdata->camera[id];

		/* Assign subdev to proper camera device pointer */
		fimc->camera[pdata->camera[id]->id]->sd = \
					fimc->sd[pdata->camera[id]->id];
	}

	return 0;
}

static int __devinit fimc_probe(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct clk *srclk;
	int ret;

	fimc = kzalloc(sizeof(*fimc), GFP_KERNEL);
	if (!fimc) {
		dev_err(&pdev->dev, "not enough memory\n");
		goto err_fimc;
	}

	ctrl = fimc_register_controller(pdev);
	if (!ctrl) {
		dev_err(&pdev->dev, "cannot register fimc controller\n");
		goto err_fimc;
	}

	pdata = to_fimc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* fimc source clock */
	srclk = clk_get(&pdev->dev, pdata->srclk_name);
	if (IS_ERR(srclk)) {
		dev_err(&pdev->dev, "failed to get source clock of fimc\n");
		goto err_clk_io;
	}

	/* fimc clock */
	ctrl->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(ctrl->clock)) {
		dev_err(&pdev->dev, "failed to get fimc clock source\n");
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
		dev_err(&pdev->dev, "v4l2 device register failed\n");
		goto err_clk_io;
	}

	/* things to initialize once */
	ret = fimc_init_global(pdev);
	if (ret)
		goto err_global;

	/* v4l2 subdev configuration */
	ret = fimc_configure_subdev(pdev, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "subdev[%d] registering failed\n", \
			ctrl->id);
	}

	/* video device register */
	ret = video_register_device(ctrl->vd, VFL_TYPE_GRABBER, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "cannot register video driver\n");
		goto err_video;
	}

	dev_info(&pdev->dev, "controller %d registered successfully\n", \
		ctrl->id);

	return 0;

err_video:
	clk_put(fimc->mclk);

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
	kfree(fimc);

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

