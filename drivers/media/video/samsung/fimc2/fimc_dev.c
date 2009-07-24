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

struct fimc_global *fimc_dev;

dma_addr_t fimc_dma_alloc(struct fimc_control *ctrl, u32 bytes)
{
	dma_addr_t end, addr, *curr;

	mutex_lock(&ctrl->lock);

	end = ctrl->mem.base + ctrl->mem.size;
	curr = &ctrl->mem.curr;

	if (*curr + bytes > end) {
		addr = 0;
	} else {
		addr = *curr;
		*curr += bytes;
	}

	mutex_unlock(&ctrl->lock);

	return addr;
}

void fimc_dma_free(struct fimc_control *ctrl, u32 bytes)
{
	mutex_lock(&ctrl->lock);
	ctrl->mem.curr -= bytes;
	mutex_unlock(&ctrl->lock);
}

static inline u32 fimc_irq_out_dma(struct fimc_control *ctrl)
{
	int ret = -1;

	/* Attach done buffer to outgoing queue. */
	ret = fimc_attach_out_queue(ctrl, ctrl->out->idx.active);
	if (ret < 0)
		dev_err(ctrl->dev, "Failed: fimc_attach_out_queue.\n");

	ctrl->out->idx.active = -1;
	ctrl->status = FIMC_STREAMON_IDLE;

	return 1;
}

static inline u32 fimc_irq_out_fimd(struct fimc_control *ctrl)
{
	u32 prev, next, wakeup = 0;
	int ret = -1;

	/* Attach done buffer to outgoing queue. */
	if (ctrl->out->idx.prev != -1) {
		ret = fimc_attach_out_queue(ctrl, ctrl->out->idx.prev);
		if (ret < 0) {
			dev_err(ctrl->dev, "Failed: \
					fimc_attach_out_queue.\n");
		} else {
			ctrl->out->idx.prev = -1;
			wakeup = 1; /* To wake up fimc_v4l2_dqbuf(). */
		}
	}

	/* Update index structure. */
	if (ctrl->out->idx.next != -1) {
		ctrl->out->idx.active	= ctrl->out->idx.next;
		ctrl->out->idx.next	= -1;
	}

	/* Detach buffer from incomming queue. */
	ret =  fimc_detach_in_queue(ctrl, &next);
	if (ret == 0) {	/* There is a buffer in incomming queue. */
		prev = ctrl->out->idx.active;
		ctrl->out->idx.prev	= prev;
		ctrl->out->idx.next	= next;

		/* Set the address */
		fimc_outdev_set_src_addr(ctrl, ctrl->out->buf[next].base);
	}

	return wakeup;
}

static inline void fimc_irq_out(struct fimc_control *ctrl)
{
	u32 wakeup = 1;

	/* Interrupt pendding clear */
	fimc_hwset_clear_irq(ctrl);
	
	if (ctrl->out->fbuf.base)
		wakeup = fimc_irq_out_dma(ctrl);
	else if (ctrl->status != FIMC_READY_OFF)
		wakeup = fimc_irq_out_fimd(ctrl);

	if (wakeup == 1)
		wake_up_interruptible(&ctrl->wq);
}

static inline void fimc_irq_cap(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;

	fimc_hwset_clear_irq(ctrl);
	fimc_hwget_overflow_state(ctrl);

	if (cap->irq == FIMC_IRQ_NORMAL) {
		printk("%s:%d\n", __FUNCTION__, __LINE__);
		fimc_hwset_enable_lastirq(ctrl);
		fimc_hwset_disable_lastirq(ctrl);
		cap->irq = FIMC_IRQ_LAST;
	} else if (cap->irq == FIMC_IRQ_LAST) {
	printk("%s:%d\n", __FUNCTION__, __LINE__);
		wake_up_interruptible(&ctrl->wq);
	}
}

static irqreturn_t fimc_irq(int irq, void *dev_id)
{
	struct fimc_control *ctrl = (struct fimc_control *) dev_id;
	if (ctrl->cap) {
		fimc_irq_cap(ctrl);
	} else if (ctrl->out) {
		fimc_irq_out(ctrl);
	}
	
	return IRQ_HANDLED;
}

static
struct fimc_control *fimc_register_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct resource *res;
	int id, mdev_id, irq;

	id = pdev->id;
	mdev_id = S3C_MDEV_FIMC0 + id;
	pdata = to_fimc_plat(&pdev->dev);

	ctrl = get_fimc_ctrl(id);
	ctrl->id = id;
	ctrl->dev = &pdev->dev;
	ctrl->vd = &fimc_video_device[id];
	ctrl->vd->minor = id;
	ctrl->mem.base = s3c_get_media_memory(mdev_id);
	ctrl->mem.size = s3c_get_media_memsize(mdev_id);
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
		dev_err(ctrl->dev, "[%s] failed to get io memory region\n", \
			__FUNCTION__);
		return NULL;
	}

	/* request mem region */
	res = request_mem_region(res->start, res->end - \
					res->start + 1, pdev->name);
	if (!res) {
		dev_err(ctrl->dev, "[%s] failed to request io memory region\n", \
			__FUNCTION__);
		return NULL;
	}

	/* ioremap for register block */
	ctrl->regs = ioremap(res->start, res->end - res->start + 1);
	if (!ctrl->regs) {
		dev_err(ctrl->dev, "[%s] failed to remap io region\n", \
			__FUNCTION__);
		return NULL;
	}

	/* irq */
	irq = platform_get_irq(pdev, 0);
	if (request_irq(irq, fimc_irq, IRQF_DISABLED, ctrl->name, ctrl))
		dev_err(ctrl->dev, "[%s] request_irq failed\n", __FUNCTION__);

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


static void fimc_mmap_open(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= (pri_data / 0x10);
	u32 idx		= (pri_data % 0x10);

	atomic_inc(&dev->ctrl[id].out->buf[idx].mapped_cnt);
}

static void fimc_mmap_close(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= (pri_data / 0x10);
	u32 idx		= (pri_data % 0x10);

	atomic_dec(&dev->ctrl[id].out->buf[idx].mapped_cnt);
}

static struct vm_operations_struct fimc_mmap_ops = {
	.open	= fimc_mmap_open,
	.close	= fimc_mmap_close,
};

static int fimc_mmap(struct file* filp, struct vm_area_struct *vma)
{
	struct fimc_control *ctrl = filp->private_data;
	u32 start_phy_addr = 0;
	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, idx = vma->vm_pgoff;
	int pri_data = 0;

	if (ctrl->out) {	/* OUTPUT device */
		if (size > ctrl->out->buf[idx].length) {
			dev_err(ctrl->dev, "Requested mmap size is too big.\n");
			return -EINVAL;
		}

		pri_data = (ctrl->id * 0x10) + idx;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		vma->vm_flags |= VM_RESERVED;
		vma->vm_ops = &fimc_mmap_ops;
		vma->vm_private_data = (void *)pri_data;

		if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
			dev_err(ctrl->dev, "writable mapping must be shared\n");
			return -EINVAL;
		}

		start_phy_addr = ctrl->out->buf[idx].base;
		pfn = __phys_to_pfn(start_phy_addr);

		if (remap_pfn_range(vma, vma->vm_start, pfn, size, \
							vma->vm_page_prot)) {
			dev_err(ctrl->dev, "mmap fail\n");
			return -EINVAL;
		}

		vma->vm_ops->open(vma);

		ctrl->out->buf[idx].flags |= V4L2_BUF_FLAG_MAPPED;
	} else {		/* CAPTURE device */
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		vma->vm_flags |= VM_RESERVED;

		/* page frame number of the address for a source frame to be stored at. */
		pfn = __phys_to_pfn(ctrl->cap->bufs[vma->vm_pgoff].base);

		if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
			dev_err(ctrl->dev, "[%s] writable mapping must be shared\n", \
				__FUNCTION__);
			return -EINVAL;
		}

		if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
			dev_err(ctrl->dev, "[%s] mmap fail\n", __FUNCTION__);
			return -EINVAL;
		}
	}

	return 0;
}

static u32 fimc_poll(struct file *filp, poll_table *wait)
{
	struct fimc_control *ctrl = filp->private_data;
	struct fimc_capinfo *cap = ctrl->cap;
	u32 mask = 0, ret = 1;

	if (cap) {
		if (cap->inqueue[0] == -1) {
			ret = wait_event_interruptible_timeout(ctrl->wq, \
				cap->inqueue[0] != -1, FIMC_DQUEUE_TIMEOUT);
		}

		if (ret) {
			if (cap->irq == FIMC_IRQ_NONE) {
				cap->irq = FIMC_IRQ_NORMAL;
				poll_wait(filp, &ctrl->wq, wait);
			} else if (cap->irq == FIMC_IRQ_LAST) {
				cap->irq = FIMC_IRQ_NONE;
				mask = POLLIN | POLLRDNORM;
			}
		}
	}

	return mask;
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

static int fimc_wakeup_fifo(struct fimc_control *ctrl)
{
#if 0
	int ret = -1;

	/* Set the rot, pp param register. */
	ret = fimc_check_param(ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "fimc_check_param failed.\n");
		return -EINVAL;
	}

	if ( ctrl->rot.degree != 0) {
		ret = s3c_rp_rot_set_param(ctrl);
		if (ret < 0) {
			rp_err(ctrl->log_level, "s3c_rp_rot_set_param failed.\n");
			return -1;
		}
	}

	ret = s3c_rp_pp_set_param(ctrl);
	if (ret < 0) {
		rp_err(ctrl->log_level, "s3c_rp_pp_set_param failed.\n");
		return -1;
	}

	/* Start PP */
	ret = s3c_rp_pp_start(ctrl, ctrl->pp.buf_idx.run);
	if (ret < 0) {
		rp_err(ctrl->log_level, "Failed : s3c_rp_pp_start().\n");
		return -1;
	}

	ctrl->status = FIMC_STREAMON;
#endif

	return 0;
}

int fimc_wakeup(void)
{
#if 0
	struct fimc_control	*ctrl;
	int			ret = -1;

	ctrl = &s3c_rp;

	if (ctrl->status == FIMC_READY_RESUME) {
		ret = fimc_wakeup_fifo(ctrl);
		if (ret < 0) {
			dev_err(ctrl->dev, "s3c_rp_wakeup_fifo failed in %s.\n", __FUNCTION__);
			return -EINVAL;
		}
	}
#endif

	return 0;
}

static void fimc_sleep_fifo(struct fimc_control *ctrl)
{
#if 0
	if (ctrl->rot.status != ROT_IDLE)
		rp_err(ctrl->log_level, "[%s : %d] ROT status isn't idle.\n", __FUNCTION__, __LINE__);

	if ((ctrl->incoming_queue[0] != -1) || (ctrl->inside_queue[0] != -1))
		rp_err(ctrl->log_level, "[%s : %d] queue status isn't stable.\n", __FUNCTION__, __LINE__);

	if ((ctrl->pp.buf_idx.next != -1) || (ctrl->pp.buf_idx.prev != -1))
		rp_err(ctrl->log_level, "[%s : %d] PP status isn't stable.\n", __FUNCTION__, __LINE__);

	s3c_rp_pp_fifo_stop(ctrl, FIFO_SLEEP);
#endif
}

int fimc_sleep(void)
{
#if 0
	struct fimc_control *ctrl;

	ctrl = &s3c_rp;

	if (ctrl->status == FIMC_STREAMON) {
		fimc_sleep_fifo(ctrl);
	}

	ctrl->status		= FIMC_ON_SLEEP;
#endif
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

	ctrl->fb.open_fifo	= s3cfb_open_fifo;
	ctrl->fb.close_fifo	= s3cfb_close_fifo;

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_GET_LCD_WIDTH, \
					(unsigned long)&ctrl->fb.lcd_hres);
	if (ret < 0)
		dev_err(ctrl->dev,  "Fail: S3CFB_GET_LCD_WIDTH\n");

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_GET_LCD_HEIGHT, \
					(unsigned long)&ctrl->fb.lcd_vres);
	if (ret < 0)
		dev_err(ctrl->dev,  "Fail: S3CFB_GET_LCD_HEIGHT\n");

	ctrl->status		= FIMC_STREAMOFF;

#if 0
	/* To do : have to send ctrl to the fimd driver. */
	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_SET_SUSPEND_FIFO, (unsigned long)fimc_sleep);
	if (ret < 0)
		dev_err(ctrl->dev,  "s3cfb_direct_ioctl(S3CFB_SET_SUSPEND_FIFO) fail\n");

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_SET_RESUME_FIFO, (unsigned long)fimc_wakeup);
	if (ret < 0)
		dev_err(ctrl->dev,  "s3cfb_direct_ioctl(S3CFB_SET_SUSPEND_FIFO) fail\n");
#endif
	mutex_unlock(&ctrl->lock);

	return 0;

resource_busy:
	mutex_unlock(&ctrl->lock);
	return ret;
}

static int fimc_release(struct file *filp)
{
	struct fimc_control *ctrl = filp->private_data;
	struct s3c_platform_fimc *pdata;
	int ret = 0;

	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	atomic_dec(&ctrl->in_use);
	filp->private_data = NULL;

	/* FIXME: turning off actual working camera */
	if (ctrl->cam) {
		/* shutdown the MCLK */
		clk_disable(ctrl->cam->clk);

		/* shutdown */
		if (ctrl->cam->cam_power)
			ctrl->cam->cam_power(0);

		/* should be initialized at the next open */
		ctrl->cam->initialized = 0;
	} else if (ctrl->out) {
		if (ctrl->status != FIMC_STREAMOFF) {
			ret = fimc_outdev_stop_streaming(ctrl);
			if (ret < 0)
				dev_err(ctrl->dev, "Fail: fimc_stop_streaming\n");
			ctrl->status = FIMC_STREAMOFF;
		}
	}

	if (ctrl->cap) {
		kfree(ctrl->cap);
		ctrl->cap = NULL;
	}

	if (ctrl->out) {
		kfree(ctrl->out);
		ctrl->out = NULL;
	}

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
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[1] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[2] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
};

static int fimc_init_global(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct s3c_platform_camera *cam;
	int i;

	pdata = to_fimc_plat(&pdev->dev);

	/* Registering external camera modules. re-arrange order to be sure */
	for (i = 0; i < FIMC_MAXCAMS; i++) {
		cam = pdata->camera[i];
		if (!cam)
			break;

		/* mclk */
		cam->clk = clk_get(&pdev->dev, cam->clk_name);
		if (IS_ERR(cam->clk)) {
			dev_err(&pdev->dev, "[%s] failed to get mclk source\n", \
				__FUNCTION__);
			return -EINVAL;
		}

		/* Assign camera device to fimc */
		fimc_dev->camera[cam->id] = cam;
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
	struct s3c_platform_camera *cam;
	struct i2c_adapter *i2c_adap;
	struct i2c_board_info *i2c_info;
	struct v4l2_subdev *sd;
	struct fimc_control *ctrl;
	unsigned short addr;
	char *name;

	ctrl = get_fimc_ctrl(id);
	pdata = to_fimc_plat(&pdev->dev);
	cam = pdata->camera[id];

	/* Subdev registration */
	if (cam) {
		i2c_adap = i2c_get_adapter(cam->i2c_busnum);
		if (!i2c_adap) {
			dev_info(&pdev->dev, "subdev i2c_adapter " \
					"missing-skip registration\n");
		}

		i2c_info = cam->info;
		if (!i2c_info) {
			dev_err(&pdev->dev, "[%s] subdev i2c board info missing\n", \
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
				"[%s] v4l2 subdev board registering failed\n", \
				__FUNCTION__);
		}

		/* Assign camera device to fimc */
		fimc_dev->camera[cam->id] = cam;

		/* Assign subdev to proper camera device pointer */
		fimc_dev->camera[cam->id]->sd = sd;
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
			dev_err(&pdev->dev, "[%s] not enough memory\n", \
				__FUNCTION__);
			goto err_fimc;
		}
	}

	ctrl = fimc_register_controller(pdev);
	if (!ctrl) {
		dev_err(&pdev->dev, "[%s] cannot register fimc controller\n", \
			__FUNCTION__);
		goto err_fimc;
	}

	pdata = to_fimc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* fimc source clock */
	srclk = clk_get(&pdev->dev, pdata->srclk_name);
	if (IS_ERR(srclk)) {
		dev_err(&pdev->dev, "[%s] failed to get source clock of fimc\n", \
			__FUNCTION__);
		goto err_clk_io;
	}

	/* fimc clock */
	ctrl->clk = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(ctrl->clk)) {
		dev_err(&pdev->dev, "[%s] failed to get fimc clock source\n", \
			__FUNCTION__);
		goto err_clk_io;
	}

	/* set parent clock */
	if (ctrl->clk->set_parent)
		ctrl->clk->set_parent(ctrl->clk, srclk);

	/* set clockrate for fimc interface block */
	if (ctrl->clk->set_rate) {
		ctrl->clk->set_rate(ctrl->clk, pdata->clk_rate);
		dev_info(&pdev->dev, "fimc set clock rate to %d\n", \
				pdata->clk_rate);
	}

	clk_enable(ctrl->clk);

	/* V4L2 device-subdev registration */
	ret = v4l2_device_register(&pdev->dev, &ctrl->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "[%s] v4l2 device register failed\n", \
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
		dev_err(&pdev->dev, "[%s] subdev[%d] registering failed\n", \
			__FUNCTION__, ctrl->id);
	}

	/* video device register */
	ret = video_register_device(ctrl->vd, VFL_TYPE_GRABBER, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "[%s] cannot register video driver\n", \
			__FUNCTION__);
		goto err_global;
	}

	video_set_drvdata(ctrl->vd, ctrl);

	dev_info(&pdev->dev, "controller %d registered successfully\n", \
		ctrl->id);

	return 0;

err_global:
	clk_disable(ctrl->clk);
	clk_put(ctrl->clk);

err_clk_io:
	fimc_unregister_controller(pdev);

err_fimc:
	return -EINVAL;

}

static int fimc_remove(struct platform_device *pdev)
{
	fimc_unregister_controller(pdev);

	kfree(fimc_dev);
	fimc_dev = NULL;

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

