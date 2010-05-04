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

#ifdef CONFIG_FB_S3C_LTE480WV
void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	writel(0x2, S5P_MDNIE_SEL);

	/* drive strength to max */
	writel(0xffffffff, S5PC11X_GPF0_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF1_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF2_BASE + 0xc);
	writel(0x000000ff, S5PC11X_GPF3_BASE + 0xc);
}

int s3cfb_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 1);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 0);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}
int s3cfb_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPH0(6), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPH0(6), 1);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 0);
	mdelay(10);

	gpio_set_value(S5PC11X_GPH0(6), 1);
	mdelay(10);

	gpio_free(S5PC11X_GPH0(6));

	return 0;
}

int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk)
{
	struct clk *clk = NULL, *sclk_parent = NULL, *sclk = NULL;
	u32 rate = 0;
	int err;

	clk = clk_get(&pdev->dev, "fimd"); 
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get fimd clock source\n");
		goto err_clk1;
	}
	clk_enable(clk);

	sclk = clk_get(&pdev->dev, "sclk_fimd");
	if (IS_ERR(sclk)) {
		dev_err(&pdev->dev, "failed to get sclk for fimd\n");
		goto err_clk2;
	}

	if (sclk->set_parent) {
		sclk_parent = clk_get(&pdev->dev, "mout_mpll");
		if (IS_ERR(sclk_parent)) {
			dev_err(&pdev->dev, "failed to get parent of fimd sclk\n");
			goto err_clk3;
		}

		sclk->parent = sclk_parent;

		err = sclk->set_parent(sclk, sclk_parent);
		if (err) {
			dev_err(&pdev->dev, "failed to set parent of fimd sclk\n");
			goto err_clk4;
		}

		if (sclk->round_rate)
			rate = sclk->round_rate(sclk, 170000000);

		if (!rate)
			rate = 166700000;
		
		if (sclk->set_rate) {
			sclk->set_rate(sclk, rate);
			dev_dbg(&pdev->dev, "set fimd sclk rate to %d\n", rate);
		}

		clk_put(sclk_parent);
		clk_enable(sclk);
	}
	
	*s3cfb_clk = sclk;

	return 0;

err_clk4:
	clk_put(sclk_parent);

err_clk3:
	clk_put(sclk);
		
err_clk2:
	clk_disable(clk);
	clk_put(clk);

err_clk1:
	return -EINVAL;
}

int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk)
{
	struct clk *hclk = NULL;
	clk_disable(*clk);
	clk_put(*clk);
	*clk = NULL;

	hclk = clk_get(&pdev->dev, "fimd");
	clk_disable(hclk);
	
	return 0;
}

void s3cfb_get_clk_name(char *clk_name)
{
	strcpy(clk_name, "sclk_fimd");
}

#elif defined(CONFIG_FB_S3C_TL2796)

void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	writel(0x2, S5P_MDNIE_SEL);

	/* drive strength to max */
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x12c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x14c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x16c);
	writel(0x000000ff, S5PC11X_VA_GPIO + 0x18c);

	s3c_gpio_cfgpin(S5PC11X_GPB(4), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(5), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(6), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(7), S3C_GPIO_SFN(1));

	s3c_gpio_setpull(S5PC11X_GPB(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(7), S3C_GPIO_PULL_NONE);

	gpio_request(S5PC11X_GPH0(5), "GPH0");
	gpio_direction_output(S5PC11X_GPH0(5), 1);
}

int s3cfb_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 1);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 0);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}

int s3cfb_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPH0(6), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPH0(6), 1);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 0);
	mdelay(10);

	gpio_set_value(S5PC11X_GPH0(6), 1);
	mdelay(10);

	gpio_free(S5PC11X_GPH0(6));

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

#elif defined(CONFIG_FB_S3C_HT101HD1)

void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	writel(0x2, S5P_MDNIE_SEL);

	/* drive strength to max */
	writel(0xffffffff, S5PC11X_GPF0_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF1_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF2_BASE + 0xc);
	writel(0x000003ff, S5PC11X_GPF3_BASE + 0xc);
}

int s3cfb_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");
	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	err = gpio_request(S5PC11X_GPB(6), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPB for "
			"lcd backlight control\n");
		return err;
	}

	err = gpio_request(S5PC11X_GPB(7), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPB for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPB(6), 1); /* Power on (SPI1_MISO) */
	mdelay(100);

	gpio_direction_output(S5PC11X_GPD0(3), 1); /* BL pwm High */
	mdelay(10);

	gpio_direction_output(S5PC11X_GPB(7), 1); /* LED_EN (SPI1_MOSI) */
	mdelay(10);

	gpio_free(S5PC11X_GPD0(3));
	gpio_free(S5PC11X_GPB(6));
	gpio_free(S5PC11X_GPB(7));

	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev) {
	return 0;
}

int s3cfb_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPH0(6), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPH0(6), 1);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 0);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 1);
	mdelay(10);

	gpio_free(S5PC11X_GPH0(6));

	return 0;
}

int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk)
{
	struct clk *clk = NULL, *sclk_parent = NULL, *sclk = NULL;
	u32 rate = 0;
	int err;

	clk = clk_get(&pdev->dev, "fimd"); /* Core clock */
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get fimd clock source\n");
		goto err_clk1;
	}
	clk_enable(clk);

	sclk = clk_get(&pdev->dev, "sclk_fimd");
	if (IS_ERR(sclk)) {
		dev_err(&pdev->dev, "failed to get sclk for fimd\n");
		goto err_clk2;
	}

	if (sclk->set_parent) {
		sclk_parent = clk_get(&pdev->dev, "mout_mpll");
		if (IS_ERR(sclk_parent)) {
			dev_err(&pdev->dev, "failed to get parent of fimd sclk\n");
			goto err_clk3;
		}

		sclk->parent = sclk_parent;

		err = sclk->set_parent(sclk, sclk_parent);
		if (err) {
			dev_err(&pdev->dev, "failed to set parent of fimd sclk\n");
			goto err_clk4;
		}

		if (sclk->round_rate)
			rate = sclk->round_rate(sclk, 133400000);

		if (!rate)
			rate = 133400000;
		
		if (sclk->set_rate) {
			sclk->set_rate(sclk, rate);
			dev_dbg(&pdev->dev, "set fimd sclk rate to %d\n", rate);
		}

		clk_put(sclk_parent);
		clk_enable(sclk);
	}
	
	*s3cfb_clk = sclk;

	return 0;

err_clk4:
	clk_put(sclk_parent);

err_clk3:
	clk_put(sclk);
		
err_clk2:
	clk_disable(clk);
	clk_put(clk);

err_clk1:
	return -EINVAL;
}

int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk)
{
	struct clk *sclk = NULL;

	sclk = clk_get(&pdev->dev, "sclk_fimd");
	if (IS_ERR(sclk)) {
		dev_err(&pdev->dev, "failed to get sclk for fimd\n");
		return -EINVAL;
	}
	clk_disable(sclk);
	clk_put(sclk);
	
	clk_disable(*clk);
	clk_put(*clk);

	*clk = NULL;
	
	return 0;
}

void s3cfb_get_clk_name(char *clk_name)
{
	strcpy(clk_name, "sclk_fimd");
}

#endif

