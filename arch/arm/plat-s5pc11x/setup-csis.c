/* linux/arch/arm/plat-s5pc11x/setup-csis.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5PC1XX MIPI-CSI2 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <plat/map.h>
#include <plat/regs-clock.h>
#include <asm/io.h>

struct platform_device; /* don't need the contents */

void s3c_csis_cfg_gpio(struct platform_device *dev)
{
}

void s3c_csis_cfg_phy_global(struct platform_device *dev, int on)
{
}

