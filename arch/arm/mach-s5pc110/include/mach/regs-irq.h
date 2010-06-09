/* arch/arm/mach-s5pc110/include/mach/regs-irq.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com
 *
 * Based on mach-s3c2410/include/mach/regs-irq.h
 *
 * S5PC11X - IRQ register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_REGS_IRQ_H
#define __ASM_ARCH_REGS_IRQ_H __FILE__

#include <asm/hardware/vic.h>

/* interrupt controller */
#define S5PC110_VIC0REG(x)          		((x) + S3C_VA_VIC0)
#define S5PC110_VIC1REG(x)          		((x) + S3C_VA_VIC1)
#define S5PC110_VIC2REG(x)         		((x) + S3C_VA_VIC2)
#define S5PC110_VIC3REG(x)         		((x) + S3C_VA_VIC3)
#endif /* __ASM_ARCH_REGS_IRQ_H */
