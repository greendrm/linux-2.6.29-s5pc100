/* linux/arch/arm/plat-s5pc1xx/dev-csis.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5PC11X series device definition for MIPI-CSI2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <mach/map.h>

#include <plat/csis.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s5p_csis_resource[] = {
	[0] = {
		.start = S5PC11X_PA_CSIS,
		.end   = S5PC11X_PA_CSIS + S5PC11X_SZ_CSIS - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MIPICSI,
		.end   = IRQ_MIPICSI,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_csis = {
	.name		  = "s5p-csis",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s5p_csis_resource),
	.resource	  = s5p_csis_resource,
};

static struct s5p_platform_csis default_csis_data __initdata = {
	.clk_name = "mipi-csis",
};

void __init s5p_csis_set_platdata(struct s5p_platform_csis *pd)
{
	struct s5p_platform_csis *npd;

	if (!pd)
		pd = &default_csis_data;

	npd = kmemdup(pd, sizeof(struct s5p_platform_csis), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = s5p_csis_cfg_gpio;
	npd->cfg_phy_global = s5p_csis_cfg_phy_global;

	s5p_device_csis.dev.platform_data = npd;
}

