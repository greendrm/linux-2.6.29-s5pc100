/* linux/arch/arm/plat-s5pc11x/setup-fb.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jonghun Han <jonghun.han@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5PC11X FIMD controller configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/fb.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <asm/io.h>
#include <mach/map.h>

struct platform_device; /* don't need the contents */

#if defined(CONFIG_FB_S3C_AMS320)

void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	writel(0x10, S5P_MDNIE_SEL);
	udelay(200);

	s3c_gpio_cfgpin(S5P64XX_GPH3(5), S3C_GPIO_SFN(1));	/* LCD RESET */
	s3c_gpio_cfgpin(S5P64XX_GPF3(5), S3C_GPIO_SFN(1));	/* SPI CS */
	s3c_gpio_cfgpin(S5P64XX_GPD1(0), S3C_GPIO_SFN(1));	/* SPI Clock */
	s3c_gpio_cfgpin(S5P64XX_GPD1(1), S3C_GPIO_SFN(1));	/* SPI Data */

	s3c_gpio_setpull(S5P64XX_GPH3(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPF3(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPD1(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPD1(1), S3C_GPIO_PULL_NONE);

	s3c_gpio_setpin(S5P64XX_GPD1(0), 0);
	s3c_gpio_setpin(S5P64XX_GPD1(1), 0);
}

int s3cfb_backlight_on(struct platform_device *pdev)
{
	return 0;
}

int s3cfb_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5P64XX_GPH3(5), "GPH3");
	if (err) {
		printk(KERN_ERR "failed to request GPH3 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5P64XX_GPH3(5), 1);
	mdelay(3);

	gpio_set_value(S5P64XX_GPH3(5), 0);
	mdelay(20);

	gpio_set_value(S5P64XX_GPH3(5), 1);

	gpio_free(S5P64XX_GPH3(5));

	return 0;
}

int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk)
{
	struct clk *clk = NULL;

	clk = clk_get(&pdev->dev, "fimd"); /* Core, Pixel source clock */
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get fimd clock source\n");
		return -EINVAL;
	}
	clk_enable(clk);

	*s3cfb_clk = clk;
	
	return 0;
}

int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk)
{
	clk_disable(*clk);
	clk_put(*clk);

	*clk = NULL;
	
	return 0;
}

void s3cfb_get_clk_name(char *clk_name)
{
	strcpy(clk_name, "fimd");
}

#endif

