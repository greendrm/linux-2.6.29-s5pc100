/* linux/arch/arm/plat-s5pc11x/dev-fb.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jonghun Han <jonghun.han@samsung.com>
 *	http://samsungsemi.com/
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5PC11X series device definition for Samsung Display Controller (FIMD)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <mach/map.h>

#include <plat/fb.h>
#include <plat/devs.h>
#include <plat/irqs.h>

static struct resource s3cfb_resource[] = {
	[0] = {
		.start = S5PC11X_PA_LCD,
		.end   = S5PC11X_PA_LCD + S5PC11X_SZ_LCD - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_LCD1,
		.end   = IRQ_LCD1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_LCD0,
		.end   = IRQ_LCD0,
		.flags = IORESOURCE_IRQ,
	},

};

static u64 fb_dma_mask = 0xffffffffUL;

struct platform_device s3c_device_fb = {
	.name		  = "s3cfb",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3cfb_resource),
	.resource	  = s3cfb_resource,
	.dev              = {
		.dma_mask		= &fb_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

static struct s3c_platform_fb default_fb_data __initdata = {
#if defined (CONFIG_CPU_S5PC110_EVT1)
	.hw_ver	= 0x62,
#else
	.hw_ver	= 0x60,
#endif
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,
};

void __init s3cfb_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	int i;

	if (!pd)
		pd = &default_fb_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;
#if defined (CONFIG_CPU_S5PC110)
		npd->nr_buffers[npd->default_win] = CONFIG_FB_S3C_YPANSTEP;
#else
		npd->nr_buffers[npd->default_win] = CONFIG_FB_S3C_YPANSTEP + 1;
#endif
		s3cfb_get_clk_name(npd->clk_name);
		npd->cfg_gpio = s3cfb_cfg_gpio;
		npd->backlight_on = s3cfb_backlight_on;
		npd->backlight_off = s3cfb_backlight_off;
		npd->reset_lcd = s3cfb_reset_lcd;
		npd->clk_on = s3cfb_clk_on;
		npd->clk_off = s3cfb_clk_off;

		s3c_device_fb.dev.platform_data = npd;
	}
}

