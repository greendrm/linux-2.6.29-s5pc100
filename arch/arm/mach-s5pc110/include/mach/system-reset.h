/* arch/arm/mach-s3c2410/include/mach/system-reset.h
 *
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2410 - System define for arch_reset() function
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <mach/hardware.h>
#include <linux/io.h>

#include <plat/regs-clock.h>
#include <linux/err.h>

extern void (*s5pc11x_reset_hook)(void);

static void
arch_reset(char mode)
{
	if (s5pc11x_reset_hook)
		s5pc11x_reset_hook();

}
