/* linux/arch/arm/plat-s5pc11x/dev-ipc.c
 *
 * S5PC1XX device definition file for IPC
 *
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 * Youngmok Song, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
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

#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_ipc_resource[] = {
	[0] = {
		.start = S5PC11X_PA_IPC,
		.end   = S5PC11X_PA_IPC + S5PC11X_SZ_IPC - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device s3c_device_ipc = {
	.name		  = "s3c-ipc",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_ipc_resource),
	.resource	  = s3c_ipc_resource,
};

