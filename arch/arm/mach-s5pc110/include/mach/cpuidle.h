/* arch/arm/mach-s5pc110/include/mach/cpuidle.h
 *
 * Copyright 2010 Samsung Electronics
 *	Jaecheol Lee <jc.lee@samsung>
 *
 * S5PC110 - CPUIDLE support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define NORMAL_MODE	0
#define LPAUDIO_MODE	1

extern int s5pc110_setup_lpaudio(unsigned int mode);
