/* arch/arm/mach-s5pc110/button-smdkc110.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com/
 *
 * Button driver for S5PC110 (Use only testing)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <mach/cpuidle.h>

static int previous_idle_mode;
static int irq_fix;

static struct regulator *usb_host_regulator;

#if !defined(CONFIG_CPU_IDLE)
#define s5pc110_setup_lpaudio(x)	NULL
#endif

static void change_idle_mode(struct work_struct *dummy)
{
	int ret = 0;
#if defined(CONFIG_CPU_IDLE)
	if (previous_idle_mode == NORMAL_MODE) {
		ret = s5pc110_setup_lpaudio(LPAUDIO_MODE);
		previous_idle_mode = LPAUDIO_MODE;
	} else {
		ret = s5pc110_setup_lpaudio(NORMAL_MODE);
		previous_idle_mode = NORMAL_MODE;
	}
	if (ret)
		printk(KERN_ERR "Error changing cpuidle device\n");

#endif

#if 0
	if (previous_idle_mode == NORMAL_MODE) {
		ret = s5pc110_setup_lpaudio(LPAUDIO_MODE);
		previous_idle_mode = LPAUDIO_MODE;
	}

	if (regulator_is_enabled(usb_host_regulator))
		regulator_disable(usb_host_regulator);
	else
		regulator_enable(usb_host_regulator);

#endif
}

static DECLARE_WORK(idlemode, change_idle_mode);

static irqreturn_t
s3c_button_interrupt(int irq, void *dev_id)
{
	printk(KERN_DEBUG "Button Interrupt occure\n");

	irq_fix++;

	if (irq_fix > 1)
		schedule_work(&idlemode);

	return IRQ_HANDLED;
}

static struct irqaction s3c_button_irq = {
	.name		= "s3c button Tick",
	.flags		= IRQF_SHARED ,
	.handler	= s3c_button_interrupt,
};

static unsigned int s3c_button_gpio_init(void)
{
	u32 err;

	err = gpio_request(S5PC11X_GPH3(7), "GPH3");
	if (err) {
		printk(KERN_ERR "gpio request error : %d\n", err);
	} else {
		s3c_gpio_cfgpin(S5PC11X_GPH3(7), S5PC11X_GPH3_7_EXT_INT33_7);
		s3c_gpio_setpull(S5PC11X_GPH3(7), S3C_GPIO_PULL_NONE);
	}

	return err;
}

static void __init s3c_button_init(void)
{

	printk(KERN_INFO "SMDKC110 Button init function \n");

	previous_idle_mode = NORMAL_MODE;
	irq_fix = 0;
	if (s3c_button_gpio_init()) {
		printk(KERN_ERR "%s failed\n", __func__);
		return;
	}

	set_irq_type(IRQ_EINT(31), IRQ_TYPE_EDGE_FALLING);
	set_irq_wake(IRQ_EINT(31), 1);
	setup_irq(IRQ_EINT(31), &s3c_button_irq);
#if 0
	usb_host_regulator = regulator_get(NULL, "usb host");
	if (IS_ERR(usb_host_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "usb host");
		return PTR_ERR(usb_host_regulator);
	}
	regulator_enable(usb_host_regulator);
#endif
}


late_initcall(s3c_button_init);
