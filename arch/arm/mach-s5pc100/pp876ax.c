/* arch/arm/mach-s5pc100/pp876ax.c
 * 
 * Copyright (C) 2011 Pointchips, inc.
 * 
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * 
 * This program is distributed in the hope that is will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABLILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Genernal Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <asm/io.h>

#include <linux/i2c-pb206x-platform.h>

/* s5pc100 specific header */
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/regs-mem.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-a0.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-bank-d1.h>

#define GPIO_A	S5PC1XX_GPB(1)
#define GPIO_B	S5PC1XX_GPB(2)
#define GPIO_C	S5PC1XX_GPD(1)
#define GPIO_D	S5PC1XX_GPD(2)
#define GPIO_E	S5PC1XX_GPA0(0)
#define GPIO_F	S5PC1XX_GPA0(1)

#define GPIO_A_IRQ	S3C_IRQ_GPIO(GPIO_A)
#define GPIO_B_IRQ	S3C_IRQ_GPIO(GPIO_B)
#define GPIO_C_IRQ	S3C_IRQ_GPIO(GPIO_C)
#define GPIO_D_IRQ	S3C_IRQ_GPIO(GPIO_D)
#define GPIO_E_IRQ	S3C_IRQ_GPIO(GPIO_E)
#define GPIO_F_IRQ	S3C_IRQ_GPIO(GPIO_F)

static const char name[] = "pp876ax_i2c_client";

static int gpio_configure(void) {
	int ret;

	printk("%s()\n", __func__);
	ret = gpio_request(S5PC1XX_GPB(1), "GPB");
	if (ret) {
		printk("%s: gpio(GPB(1) request error: %d\n", __func__, ret);
	}
	else {
		s3c_gpio_cfgpin(S5PC1XX_GPB(1), S5PC1XX_GPB1_GPIO_INT2_1);
		s3c_gpio_setpull(S5PC1XX_GPB(1), S3C_GPIO_PULL_NONE);
	}

	ret = gpio_request(S5PC1XX_GPB(2), "GPB");
	if (ret) {
		printk("%s: gpio(GPB(2) request error: %d\n", __func__, ret);
	}
	else {
		s3c_gpio_cfgpin(S5PC1XX_GPB(2), S5PC1XX_GPB2_GPIO_INT2_2);
		s3c_gpio_setpull(S5PC1XX_GPB(2), S3C_GPIO_PULL_NONE);
	}
		
	ret = gpio_request(S5PC1XX_GPD(1), "GPD");
	if (ret) {
		printk("%s: gpio(GPD(1) request error: %d\n", __func__, ret);
	}
	else {
		s3c_gpio_cfgpin(S5PC1XX_GPD(1), S5PC1XX_GPD_1_1_GPIO_INT7_1);
		s3c_gpio_setpull(S5PC1XX_GPD(1), S3C_GPIO_PULL_NONE);
	}

	ret = gpio_request(S5PC1XX_GPD(2), "GPD");
	if (ret) {
		printk("%s: gpio(GPD(2) request error: %d\n", __func__, ret);
	}
	else {
		s3c_gpio_cfgpin(S5PC1XX_GPD(2), S5PC1XX_GPD_1_2_GPIO_INT7_2);
		s3c_gpio_setpull(S5PC1XX_GPD(2), S3C_GPIO_PULL_NONE);
	}

	ret = gpio_request(S5PC1XX_GPA0(0), "GPA0");
	if (ret) {
		printk("%s: gpio(GPA0(0) request error: %d\n", __func__, ret);
	}
	else {
		s3c_gpio_cfgpin(S5PC1XX_GPA0(0), S5PC1XX_GPA0_0_GPIO_INT0_0);
		s3c_gpio_setpull(S5PC1XX_GPA0(0), S3C_GPIO_PULL_NONE);
	}

	ret = gpio_request(S5PC1XX_GPA0(1), "GPA0");
	if (ret) {
		printk("%s: gpio(GPA0(1) request error: %d\n", __func__, ret);
	}
	else {
		s3c_gpio_cfgpin(S5PC1XX_GPA0(1), S5PC1XX_GPA0_1_GPIO_INT0_1);
		s3c_gpio_setpull(S5PC1XX_GPA0(1), S3C_GPIO_PULL_NONE);
	}

	return 0;
}

static struct i2c_board_info i2c_2_devs[] __initdata = {
	{ 
		I2C_BOARD_INFO("pp876ax_i2c_client", (0x1c>>1)), 
		.irq = GPIO_A_IRQ,
	},
};
static struct i2c_board_info i2c_3_devs[] __initdata = {
	{ 
		I2C_BOARD_INFO("pp876ax_i2c_client", (0x1c>>1)), 
		.irq = GPIO_B_IRQ,
	},
};
static struct i2c_board_info i2c_4_devs[] __initdata = {
	{ 
		I2C_BOARD_INFO("pp876ax_i2c_client", (0x1c>>1)), 
		.irq = GPIO_C_IRQ,
	},
};
static struct i2c_board_info i2c_5_devs[] __initdata = {
	{ 
		I2C_BOARD_INFO("pp876ax_i2c_client", (0x1c>>1)), 
		.irq = GPIO_D_IRQ,
	},
};
static struct i2c_board_info i2c_6_devs[] __initdata = {
	{ 
		I2C_BOARD_INFO("pp876ax_i2c_client", (0x1c>>1)), 
		.irq = GPIO_E_IRQ,
	},
};
static struct i2c_board_info i2c_7_devs[] __initdata = {
	{ 
		I2C_BOARD_INFO("pp876ax_i2c_client", (0x1c>>1)), 
		.irq = GPIO_F_IRQ,
	},
};


int __init pp876ax_i2c_client_init(void)
{
	gpio_configure();

	i2c_register_board_info(2, i2c_2_devs, ARRAY_SIZE(i2c_2_devs));
	i2c_register_board_info(3, i2c_3_devs, ARRAY_SIZE(i2c_3_devs));
	i2c_register_board_info(4, i2c_4_devs, ARRAY_SIZE(i2c_4_devs));
	i2c_register_board_info(5, i2c_5_devs, ARRAY_SIZE(i2c_5_devs));
	i2c_register_board_info(6, i2c_6_devs, ARRAY_SIZE(i2c_6_devs));
	//i2c_register_board_info(7, i2c_7_devs, ARRAY_SIZE(i2c_7_devs));

	return 0;
}
