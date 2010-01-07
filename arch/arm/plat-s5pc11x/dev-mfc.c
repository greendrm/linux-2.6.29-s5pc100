/* linux/arch/arm/plat-s5pc11x/dev-mfc50.c
 *
 * Copyright 2009 Samsung Electronics
 *	Changhwan Youn <chaos.youn@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5PC110 device definition for mfc device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <mach/map.h>
#include <asm/irq.h>
#include <plat/mfc.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_mfc_resources[] = {
	[0] = {
		.start  = S5PC11X_PA_MFC,
		.end    = S5PC11X_PA_MFC + S5PC11X_SZ_MFC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_MFC,
		.end    = IRQ_MFC,
		.flags  = IORESOURCE_IRQ,
	}
};

struct s3c_mfc_platdata s3c_mfc_def_platdata = {
	.buf_phy_base[0] = 0,
	.buf_phy_base[1] = 0,
	.buf_size[0] = 0,
	.buf_size[1] = 0,
};

struct platform_device s3c_device_mfc = {
	.name           = "s3c-mfc",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s3c_mfc_resources),
	.resource       = s3c_mfc_resources,
	.dev            = {
		.platform_data = &s3c_mfc_def_platdata,
	},
};

void __init s3c_mfc_set_platdata(struct s3c_mfc_platdata *pd)
{
	struct s3c_mfc_platdata *set = &s3c_mfc_def_platdata;

	if(pd) {
		set->buf_phy_base[0] = pd->buf_phy_base[0];
		set->buf_phy_base[1] = pd->buf_phy_base[1];
		set->buf_size[0] = pd->buf_size[0];
		set->buf_size[1] = pd->buf_size[1];
	}
}
