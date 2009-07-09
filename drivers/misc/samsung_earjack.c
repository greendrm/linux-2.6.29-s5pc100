/*
 * linux/drivers/misc/mirage_earjack.c
 *
 * Copyright (C) 2008 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <mach/gpio.h>

#if CONFIG_MACH_UNIVERSAL
#define EARJACK_DETECT_IRQ	IRQ_EINT(19)
#define EARJACK_DETECT_GPIO	S5PC1XX_GPH2(3)
#define EARJACK_SENDKEY_IRQ	IRQ_EINT(27)
#define EARJACK_SENDKEY_GPIO	S5PC1XX_GPH3(3)
#define EARJACK_DETECT_SIZE	1
#else
#endif

struct _earjack_data {
	struct device *dev;
	struct work_struct earjack_work;
	int earjack_attach;
};

static struct _earjack_data earjack_data = {
	.earjack_attach = 0,
};

static ssize_t earjack_detect_read(struct kobject *kobj,
			struct bin_attribute *bin_attr,
			char *buf, loff_t off, size_t count)
{
	count += sprintf(buf, "%d\n", earjack_data.earjack_attach);

	return count;
}

static struct bin_attribute earjack_attr = {
	.attr = {
		.name = "earjack_attach",
		.mode = S_IRUGO,
	},
	.size = EARJACK_DETECT_SIZE,
	.read = earjack_detect_read,
};

static int is_earjack_attached(void)
{
	int ret = 0;
	unsigned value;

	value = gpio_get_value(EARJACK_DETECT_GPIO);

	if (value)
		ret = 0;
	else
		ret = 1;

	return ret;
}

static void earjack_detect_work(struct work_struct *work)
{
	if (is_earjack_attached())
		earjack_data.earjack_attach = 1;
	else
		earjack_data.earjack_attach = 0;

	kobject_uevent(&earjack_data.dev->kobj, KOBJ_CHANGE);
}

static irqreturn_t earjack_detect_interrupt(int irq, void *dev_id)
{
	schedule_work(&earjack_data.earjack_work);

	return IRQ_HANDLED;
}

static irqreturn_t earjack_sendkey_interrupt(int irq, void *dev_id)
{
	unsigned value;

	value = gpio_get_value(EARJACK_SENDKEY_GPIO);

	if (value)
		;/* TODO, Key event */
	else
		;/* TODO, Key event */

	return IRQ_HANDLED;
}

static int earjack_resume(struct device *dev)
{
	if (is_earjack_attached())
		earjack_data.earjack_attach = 1;
	else
		earjack_data.earjack_attach = 0;

	kobject_uevent(&earjack_data.dev->kobj, KOBJ_CHANGE);

	return 0;
}

static struct platform_driver earjack_driver = {
	.driver = {
		.name = "earjack_detect",
		.owner = THIS_MODULE,
		.resume = earjack_resume,
	}
};

static struct platform_device *earjack_device;

static int __init earjack_init(void)
{
	platform_driver_register(&earjack_driver);
	earjack_device = platform_device_alloc("earjack_detect", -1);
	platform_device_add(earjack_device);

	earjack_data.dev = &earjack_device->dev;
	INIT_WORK(&earjack_data.earjack_work, earjack_detect_work);

	/* gpio setting */
	gpio_request(EARJACK_DETECT_GPIO, "EARJACK_DETECT");
	gpio_request(EARJACK_SENDKEY_GPIO, "EARJACK_SENDKEY");
	gpio_direction_input(EARJACK_DETECT_GPIO);
	gpio_direction_input(EARJACK_SENDKEY_GPIO);

	if (request_irq(EARJACK_DETECT_IRQ, earjack_detect_interrupt,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"earjack_detect", (void *)earjack_device)) {
		dev_err((void *)&earjack_device,
			"Failed to register earjack detect interrupt\n");
		return -EINVAL;
	}

	if (request_irq(EARJACK_SENDKEY_IRQ, earjack_sendkey_interrupt,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"earjack_sendkey", (void *)earjack_device)) {
		dev_err((void *)&earjack_device,
			"Failed to register earjack sendkey interrupt\n");
		free_irq(EARJACK_DETECT_IRQ, (void *)earjack_device);
		return -EINVAL;
	}

	/* for device manager
	 * /sys/devices/platform/earjack_detect/earjack_attach
	 */
	if (sysfs_create_bin_file(&earjack_device->dev.kobj,
				&earjack_attr)) {
		printk(KERN_ERR "sysfs_create_bin_file error\n");
		return -EINVAL;
	}

	/* check earjack when booting */
	if (is_earjack_attached())
		earjack_data.earjack_attach = 1;
	else
		earjack_data.earjack_attach = 0;

	return 0;
}

static void __exit earjack_exit(void)
{
	flush_scheduled_work();
	sysfs_remove_bin_file(&earjack_device->dev.kobj, &earjack_attr);
	free_irq(EARJACK_DETECT_IRQ, (void *)earjack_device);
	platform_device_unregister(earjack_device);
	platform_driver_unregister(&earjack_driver);
}

module_init(earjack_init);
module_exit(earjack_exit);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Mirage earjack detect driver");
MODULE_LICENSE("GPL");
