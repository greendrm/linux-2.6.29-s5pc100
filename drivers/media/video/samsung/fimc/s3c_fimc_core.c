/* linux/drivers/media/video/samsung/s3c_fimc_core.c
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

#include "s3c_fimc.h"

static struct s3c_fimc_camera test_pattern = {
	.id 		= S3C_FIMC_TPID,
	.mode		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
	.clockrate	= 0,
	.line_length	= 640,	/* FIXME: not sure about max linelength of test pattern */
	.width		= 640,
	.height		= 480,
#if 1
	.offset		= {
		.h1	= 0,
		.h2	= 0,
		.v1	= 0,
		.v2	= 0,
	},
#endif
	.polarity	= {
		.pclk	= 0,
		.vsync	= 0,
		.href	= 0,
		.hsync	= 0,
	},

	.initialized	= 0,
};

struct s3c_fimc_config s3c_fimc;

struct s3c_platform_fimc *to_fimc_plat(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return (struct s3c_platform_fimc *) pdev->dev.platform_data;
}

void s3c_fimc_register_camera(struct s3c_fimc_camera *cam)
{
	s3c_fimc.camera[cam->id] = cam;

	clk_disable(s3c_fimc.cam_clock);
	clk_set_rate(s3c_fimc.cam_clock, cam->clockrate);
	clk_enable(s3c_fimc.cam_clock);

	s3c_fimc_reset_camera();
}

void s3c_fimc_unregister_camera(struct s3c_fimc_camera *cam)
{
	int i = 0;

	for (i = 0; i < S3C_FIMC_MAX_CTRLS; i++) {
		if (s3c_fimc.ctrl[i].in_cam == cam)
			s3c_fimc.ctrl[i].in_cam = NULL;
	}

	s3c_fimc.camera[cam->id] = NULL;
}

void s3c_fimc_set_active_camera(struct s3c_fimc_control *ctrl, int id)
{
	ctrl->in_cam = s3c_fimc.camera[id];

	dev_info(ctrl->dev, "%s:requested id=%d\n",__FUNCTION__, id);
	
	if (ctrl->in_cam && id < S3C_FIMC_TPID)
		s3c_fimc_select_camera(ctrl);
}

static irqreturn_t s3c_fimc_irq(int irq, void *dev_id)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) dev_id;

	s3c_fimc_clear_irq(ctrl);
	s3c_fimc_check_fifo(ctrl);

	if (IS_CAPTURE(ctrl)) {
		dev_dbg(ctrl->dev, "irq is in capture state\n");

		if (s3c_fimc_frame_handler(ctrl) == S3C_FIMC_FRAME_SKIP)
			return IRQ_HANDLED;

		wake_up_interruptible(&ctrl->waitq);
	}

	return IRQ_HANDLED;
}

static
struct s3c_fimc_control *s3c_fimc_register_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct s3c_fimc_control *ctrl;
	struct resource *res;
	int i = S3C_FIMC_MAX_CTRLS - 1;
	int id = pdev->id;

	pdata = to_fimc_plat(&pdev->dev);

	ctrl = &s3c_fimc.ctrl[id];
	ctrl->id = id;
	ctrl->dev = &pdev->dev;
	ctrl->vd = &s3c_fimc_video_device[id];
	ctrl->rot90 = 0;
	ctrl->vd->minor = id;
	ctrl->out_frame.nr_frames = pdata->nr_frames;
	ctrl->out_frame.skip_frames = 0;
#if 0
	/* FIXME: would possibly crash in some circumstances */
	ctrl->scaler.line_length = pdata->camera[id]->line_length;
#endif
	sprintf(ctrl->name, "%s%d", S3C_FIMC_NAME, id);
	strcpy(ctrl->vd->name, ctrl->name);

	ctrl->open_lcdfifo = s3cfb_enable_local;
	ctrl->close_lcdfifo = s3cfb_enable_dma;

	atomic_set(&ctrl->in_use, 0);
	mutex_init(&ctrl->lock);
	mutex_init(&ctrl->v4l2_lock);
	init_waitqueue_head(&ctrl->waitq);

	/* get resource for io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err("failed to get io memory region\n");
		return NULL;
	}

	if (!pdata->shared_io) {
		/* request mem region */
		res = request_mem_region(res->start, res->end - res->start + 1, pdev->name);
		if (!res) {
			err("failed to request io memory region\n");
			return NULL;
		}

		/* ioremap for register block */
		ctrl->regs = ioremap(res->start, res->end - res->start + 1);
	} else {
		while (i >= 0 && ctrl->regs == NULL) {
			ctrl->regs = s3c_fimc.ctrl[i].regs;
			i--;
		}
	}

	if (!ctrl->regs) {
		err("failed to remap io region\n");
		return NULL;
	}

	/* irq */
	ctrl->irq = platform_get_irq(pdev, 0);

	if (request_irq(ctrl->irq, s3c_fimc_irq, IRQF_DISABLED, ctrl->name, ctrl))
		err("request_irq failed\n");

	s3c_fimc_reset(ctrl);

	return ctrl;
}

static int s3c_fimc_unregister_controller(struct platform_device *pdev)
{
	struct s3c_fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int id = pdev->id;

	ctrl = &s3c_fimc.ctrl[id];

	s3c_fimc_free_output_memory(&ctrl->out_frame);

	pdata = to_fimc_plat(ctrl->dev);

	if (!pdata->shared_io)
		iounmap(ctrl->regs);

	memset(ctrl, 0, sizeof(*ctrl));

	return 0;
}

static int s3c_fimc_mmap(struct file* filp, struct vm_area_struct *vma)
{
	struct s3c_fimc_control *ctrl = filp->private_data;
	struct s3c_fimc_out_frame *frame = &ctrl->out_frame;

	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, total_size = frame->buf_size;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;

	/* page frame number of the address for a source frame to be stored at. */
	pfn = __phys_to_pfn(frame->addr[vma->vm_pgoff].phys_y);

	if (size > total_size) {
		err("the size of mapping is too big\n");
		return -EINVAL;
	}

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		err("writable mapping must be shared\n");
		return -EINVAL;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		err("mmap fail\n");
		return -EINVAL;
	}

	return 0;
}

static u32 s3c_fimc_poll(struct file *filp, poll_table *wait)
{
	struct s3c_fimc_control *ctrl = filp->private_data;
	u32 mask = 0;

	poll_wait(filp, &ctrl->waitq, wait);

	if (IS_IRQ_HANDLING(ctrl))
		mask = POLLIN | POLLRDNORM;

	FSET_STOP(ctrl);

	return mask;
}

static
ssize_t s3c_fimc_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
	struct s3c_fimc_control *ctrl = filp->private_data;
	size_t end;

	if (IS_CAPTURE(ctrl)) {
		if (wait_event_interruptible(ctrl->waitq, IS_IRQ_HANDLING(ctrl)))
				return -ERESTARTSYS;

		FSET_STOP(ctrl);
	}

	end = min_t(size_t, ctrl->out_frame.buf_size, count);

	if (copy_to_user(buf, s3c_fimc_get_current_frame(ctrl), end))
		return -EFAULT;

	return end;
}

static
ssize_t s3c_fimc_write(struct file *filp, const char *b, size_t c, loff_t *offset)
{
	return 0;
}

/*
 * ctrl->id means FIMC[id]
 * camera->id means camera A/B/C
 * Openening device node is to open FIMC device
 * and after opening device, VIDIOC_S_INPUT is necessary
 * to make a choise between input camera devices
 * 	before VIDIOC_S_INPUT : pdata->camera is used
 * 	after VIDIOC_S_INPUT : s3c_fimc.camera is used
 */
static int s3c_fimc_open(struct file *filp)
{
	struct s3c_fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int minor, ret;

	minor = video_devdata(filp)->minor;
	ctrl = &s3c_fimc.ctrl[minor];
	pdata = to_fimc_plat(ctrl->dev);

	/* TODO: condition check for multiple open with same external camera
	 * TODO: condition check for multiple open with same controller
	 */
	/*
	 * Check for external camera device
	 * NOTE: scaler only feature should be implemented seperately
	 * 	so, no external camera no device node for camera
	 */
	if (!s3c_fimc.camera[pdata->default_cam]) {
		dev_err(ctrl->dev, "No external camera device\n");
		return -ENODEV;
	}

	/*
	 * Check for device power control
	 * For now we just check according to fimc number
	 * because of the default camera issue
	 * Don't forget to give proper power after VIDIOC_S_INPUT called
	 */
	if (!s3c_fimc.camera[pdata->default_cam]->cam_power) {
		dev_err(ctrl->dev, "No way to control camera[%d]'s power\n", ctrl->id);
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
	clk_set_rate(s3c_fimc.cam_clock, s3c_fimc.camera[pdata->default_cam]->clockrate);

	clk_enable(s3c_fimc.cam_clock);

	/* FIXME: Giving power */
	s3c_fimc.camera[pdata->default_cam]->cam_power(1);

	/*
	 * Default camera attach
	 * According to ctrl->in_cam (ctrl->id?)
	 * In this phase we don't know what user choose using
	 * VIDIOC_S_INPUT so just attach default camera here
	 * FIXME: This is just to be safe
	 * proper camera selection should be made 
	 * through VIDIOC_S_INPUT
	 */
	s3c_fimc_set_active_camera(ctrl, pdata->default_cam);

	/*
	 * Now Configuring external camera module(subdev)
	 * For now, we have "default camera" concept so,
	 * we directly access to the index of default camera's subdev
	 *
	 * int (*init)(struct v4l2_subdev *sd, u32 val);
	 * TODO: get val from platform data to work with special occasion
	 */
	ret = v4l2_subdev_call(ctrl->in_cam->sd, core, init, 0);

	if (ret == -ENOIOCTLCMD)
		dev_err(ctrl->dev, "%s:s_config subdev api not supported\n",
				__FUNCTION__);

	/* Apply things to interface register */
	s3c_fimc_reset(ctrl);
	filp->private_data = ctrl;

	mutex_unlock(&ctrl->lock);

	return 0;

resource_busy:
	mutex_unlock(&ctrl->lock);
	return ret;
}

static int s3c_fimc_release(struct file *filp)
{
	struct s3c_fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int minor;

	minor = video_devdata(filp)->minor;
	ctrl = &s3c_fimc.ctrl[minor];
	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	atomic_dec(&ctrl->in_use);
	filp->private_data = NULL;

	/* Shutdown the MCLK */
	clk_disable(s3c_fimc.cam_clock);

	/* FIXME: turning off actual working camera */
	s3c_fimc.camera[ctrl->in_cam->id]->cam_power(0);

	mutex_unlock(&ctrl->lock);
	
	printk(KERN_INFO "%s:successfully released\n", __FUNCTION__);
	return 0;
}

static const struct v4l2_file_operations s3c_fimc_fops = {
	.owner = THIS_MODULE,
	.open = s3c_fimc_open,
	.release = s3c_fimc_release,
	.ioctl = video_ioctl2,
	.read = s3c_fimc_read,
	.write = s3c_fimc_write,
	.mmap = s3c_fimc_mmap,
	.poll = s3c_fimc_poll,
};

static void s3c_fimc_vdev_release(struct video_device *vdev)
{
	kfree(vdev);
}

struct video_device s3c_fimc_video_device[S3C_FIMC_MAX_CTRLS] = {
	[0] = {
		.fops = &s3c_fimc_fops,
		.ioctl_ops = &s3c_fimc_v4l2_ops,
		.release  = s3c_fimc_vdev_release,
	},
	[1] = {
		.fops = &s3c_fimc_fops,
		.ioctl_ops = &s3c_fimc_v4l2_ops,
		.release  = s3c_fimc_vdev_release,
	},
	[2] = {
		.fops = &s3c_fimc_fops,
		.ioctl_ops = &s3c_fimc_v4l2_ops,
		.release  = s3c_fimc_vdev_release,
	},
};

static int s3c_fimc_init_global(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	int i;

	pdata = to_fimc_plat(&pdev->dev);

	/* camera clock */
	s3c_fimc.cam_clock = clk_get(&pdev->dev, "sclk_cam");
	if (IS_ERR(s3c_fimc.cam_clock)) {
		err("failed to get camera clock source\n");
		return -EINVAL;
	}

	s3c_fimc.dma_start = s3c_get_media_memory(S3C_MDEV_FIMC);
	s3c_fimc.dma_total = s3c_get_media_memsize(S3C_MDEV_FIMC);
	s3c_fimc.dma_current = s3c_fimc.dma_start;
	
	/* Registering external camera modules. re-arrange order to be sure */
	for (i = 0; i < S3C_FIMC_MAX_CAMS; i++) {
		if (!pdata->camera[i])
			break;
		/* Assign camera device to s3c_fimc */
		s3c_fimc.camera[pdata->camera[i]->id] = pdata->camera[i];
	}

	/* test pattern */
	/* TODO: make it as subdev? */
	s3c_fimc.camera[test_pattern.id] = &test_pattern;

	return 0;
}

/*
 * Assign v4l2 device and subdev to s3c_fimc
 * it is called per every fimc ctrl registering
 */
static int s3c_fimc_configure_subdev(struct platform_device *pdev, int id)
{
	struct s3c_platform_fimc *pdata;
	struct i2c_adapter *i2c_adap;
	struct i2c_board_info *i2c_info;
	struct v4l2_subdev *sd;
	unsigned short addr;
	char *name;

	pdata = to_fimc_plat(&pdev->dev);

	/* Subdev registration */
	if (pdata->camera[id]) {
		i2c_adap = i2c_get_adapter(pdata->camera[id]->i2c_busnum);
		if (!i2c_adap) {
			dev_info(&pdev->dev, "%s:subdev i2c_adapter missing-skip registration\n",
					__FUNCTION__);
		}

		i2c_info = pdata->camera[id]->info;	/* fetch I2C board info */
		if (!i2c_info) {
			dev_err(&pdev->dev, "%s:subdev i2c board info missing\n",
					__FUNCTION__);
			return -ENODEV;
		}

		name = i2c_info->type;
		if (!name) {
			dev_info(&pdev->dev, "%s:subdev i2c dirver name missing-skip registration\n",
					__FUNCTION__);
			return -ENODEV;
		}

		addr = i2c_info->addr;
		if (!addr) {
			dev_info(&pdev->dev, "%s:subdev i2c address missing-skip registration\n",
					__FUNCTION__);
			return -ENODEV;
		}

		/*
		 * NOTE: first time subdev being registered,
		 * s_config is called and try to initialize subdev device
		 * but in this point, we are not giving MCLK and power to subdev
		 * so nothing happens but pass platform data through
		 */
		sd = v4l2_i2c_new_subdev_board(&s3c_fimc.ctrl[id].v4l2_dev, i2c_adap,
				name, i2c_info, &addr);

		if (!sd) {
			dev_err(&pdev->dev, "%s:v4l2 subdev board registering failed\n",
					__FUNCTION__);
		}

		/* Assign probed subdev pointer to s3c_fimc */
		s3c_fimc.sd[pdata->camera[id]->id] = sd;

		/* Assign camera device to s3c_fimc */
		s3c_fimc.camera[pdata->camera[id]->id] = pdata->camera[id];

		/* Assign subdev to proper camera device pointer */
		s3c_fimc.camera[pdata->camera[id]->id]->sd = s3c_fimc.sd[pdata->camera[id]->id];
	}
	return 0;
}

static int __devinit s3c_fimc_probe(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct s3c_fimc_control *ctrl;
	struct clk *srclk;
	int ret;

	ctrl = s3c_fimc_register_controller(pdev);
	if (!ctrl) {
		err("cannot register fimc controller\n");
		goto err_fimc;
	}

	pdata = to_fimc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* fimc source clock */
	srclk = clk_get(&pdev->dev, pdata->srclk_name);
	if (IS_ERR(srclk)) {
		err("failed to get source clock of fimc\n");
		goto err_clk_io;
	}

	/* fimc clock */
	ctrl->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(ctrl->clock)) {
		err("failed to get fimc clock source\n");
		goto err_clk_io;
	}

	/* set parent clock */
	if (ctrl->clock->set_parent)
		ctrl->clock->set_parent(ctrl->clock, srclk);

	/* set clockrate for FIMC interface block */
	if (ctrl->clock->set_rate) {
		ctrl->clock->set_rate(ctrl->clock, pdata->clockrate);
		/* FIXME: dev_info */
		printk(KERN_INFO " FIMC set clock rate to %d\n", pdata->clockrate);
	}

	/* FIXME: is it really necessary to enable clock here? */
	clk_enable(ctrl->clock);

	/* V4L2 device-subdev registration */
	ret = v4l2_device_register(&pdev->dev, &ctrl->v4l2_dev);

	if (ret) {
		dev_err(ctrl->dev, "%s:v4l2 device register failed\n", __FUNCTION__);
		return ret;
	}
	/* assigning v4l2 device to controller */
	s3c_fimc.v4l2_dev[ctrl->id] = &ctrl->v4l2_dev;

	/* things to initialize once */
	/* FIXME: In case of not using fimc0, what happens? */
	if (ctrl->id == 0) {
		ret = s3c_fimc_init_global(pdev);
		if (ret)
			goto err_global;
	}

	/* v4l2 subdev configuration */
	ret = s3c_fimc_configure_subdev(pdev, ctrl->id);
	if (ret) {
		dev_err(ctrl->dev, "%s:subdev[%d] registering failed\n",
				__FUNCTION__, ctrl->id);
	}

	/* video device register */
	ret = video_register_device(ctrl->vd, VFL_TYPE_GRABBER, ctrl->id);
	if (ret) {
		err("cannot register video driver\n");
		goto err_video;
	}

	info("controller %d registered successfully\n", ctrl->id);

	/* FIXME: workaround for prior clk_en */
	/*clk_disable(ctrl->clock);*/

	return 0;

err_video:
	clk_put(s3c_fimc.cam_clock);

err_global:
	clk_disable(ctrl->clock);
	clk_put(ctrl->clock);

err_clk_io:
	s3c_fimc_unregister_controller(pdev);

err_fimc:
	return -EINVAL;

}

static int s3c_fimc_remove(struct platform_device *pdev)
{
	s3c_fimc_unregister_controller(pdev);

	return 0;
}

int s3c_fimc_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

int s3c_fimc_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver s3c_fimc_driver = {
	.probe		= s3c_fimc_probe,
	.remove		= s3c_fimc_remove,
	.suspend	= s3c_fimc_suspend,
	.resume		= s3c_fimc_resume,
	.driver		= {
		.name	= "s3c-fimc",
		.owner	= THIS_MODULE,
	},
};

static int s3c_fimc_register(void)
{
	platform_driver_register(&s3c_fimc_driver);

	return 0;
}

static void s3c_fimc_unregister(void)
{
	platform_driver_unregister(&s3c_fimc_driver);
}

//module_init(s3c_fimc_register);
late_initcall(s3c_fimc_register);
module_exit(s3c_fimc_unregister);

MODULE_AUTHOR("Dongsoo, Kim <dongsoo45.kim@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Camera Interface (FIMC) driver");
MODULE_LICENSE("GPL");
