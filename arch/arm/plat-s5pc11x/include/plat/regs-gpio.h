/* linux/arch/arm/plat-s5pc11x/include/plat/regs-gpio.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com
 *
 * Based on plat-s3c64xx/include/plat/regs-gpio.h
 *
 * S5PC11X - GPIO register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_PLAT_S5PC11X_REGS_GPIO_H
#define __ASM_PLAT_S5PC11X_REGS_GPIO_H __FILE__

/* Base addresses for each of the banks */
#include <plat/gpio-bank-a0.h>
#include <plat/gpio-bank-a1.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-bank-c0.h>
#include <plat/gpio-bank-c1.h>
#include <plat/gpio-bank-d0.h>
#include <plat/gpio-bank-d1.h>
#include <plat/gpio-bank-e0.h>
#include <plat/gpio-bank-e1.h>
#include <plat/gpio-bank-f0.h>
#include <plat/gpio-bank-f1.h>
#include <plat/gpio-bank-f2.h>
#include <plat/gpio-bank-f3.h>
#include <plat/gpio-bank-g0.h>
#include <plat/gpio-bank-g1.h>
#include <plat/gpio-bank-g2.h>
#include <plat/gpio-bank-g3.h>
#include <plat/gpio-bank-h0.h>
#include <plat/gpio-bank-h1.h>
#include <plat/gpio-bank-h2.h>
#include <plat/gpio-bank-h3.h>
#include <plat/gpio-bank-i.h>
#include <plat/gpio-bank-j0.h>
#include <plat/gpio-bank-j1.h>
#include <plat/gpio-bank-j2.h>
#include <plat/gpio-bank-j3.h>
#include <plat/gpio-bank-j4.h>
#include <plat/gpio-bank-mp01.h>
#include <plat/gpio-bank-mp02.h>
#include <plat/gpio-bank-mp03.h>
#include <plat/gpio-bank-mp04.h>
#include <plat/gpio-bank-mp05.h>
#include <plat/gpio-bank-mp06.h>
#include <plat/gpio-bank-mp07.h>
#include <plat/gpio-bank-mp10.h>
#include <plat/gpio-bank-mp11.h>
#include <plat/gpio-bank-mp12.h>
#include <plat/gpio-bank-mp13.h>
#include <plat/gpio-bank-mp14.h>
#include <plat/gpio-bank-mp15.h>
#include <plat/gpio-bank-mp16.h>
#include <plat/gpio-bank-mp17.h>
#include <plat/gpio-bank-mp18.h>
#include <plat/gpio-bank-mp20.h>
#include <plat/gpio-bank-mp21.h>
#include <plat/gpio-bank-mp22.h>
#include <plat/gpio-bank-mp23.h>
#include <plat/gpio-bank-mp24.h>
#include <plat/gpio-bank-mp25.h>
#include <plat/gpio-bank-mp26.h>
#include <plat/gpio-bank-mp27.h>
#include <plat/gpio-bank-mp28.h>


#include <mach/map.h>

#define S5PC11X_GPA0_BASE	(S5PC11X_VA_GPIO + 0x0000)
#define S5PC11X_GPA1_BASE	(S5PC11X_VA_GPIO + 0x0020)
#define S5PC11X_GPB_BASE	(S5PC11X_VA_GPIO + 0x0040)
#define S5PC11X_GPC0_BASE	(S5PC11X_VA_GPIO + 0x0060)
#define S5PC11X_GPC1_BASE	(S5PC11X_VA_GPIO + 0x0080)
#define S5PC11X_GPD0_BASE	(S5PC11X_VA_GPIO + 0x00A0)
#define S5PC11X_GPD1_BASE	(S5PC11X_VA_GPIO + 0x00C0)
#define S5PC11X_GPE0_BASE	(S5PC11X_VA_GPIO + 0x00E0)
#define S5PC11X_GPE1_BASE	(S5PC11X_VA_GPIO + 0x0100)
#define S5PC11X_GPF0_BASE	(S5PC11X_VA_GPIO + 0x0120)
#define S5PC11X_GPF1_BASE	(S5PC11X_VA_GPIO + 0x0140)
#define S5PC11X_GPF2_BASE	(S5PC11X_VA_GPIO + 0x0160)
#define S5PC11X_GPF3_BASE	(S5PC11X_VA_GPIO + 0x0180)
#define S5PC11X_GPG0_BASE	(S5PC11X_VA_GPIO + 0x01A0)
#define S5PC11X_GPG1_BASE	(S5PC11X_VA_GPIO + 0x01C0)
#define S5PC11X_GPG2_BASE	(S5PC11X_VA_GPIO + 0x01E0)
#define S5PC11X_GPG3_BASE	(S5PC11X_VA_GPIO + 0x0200)
#define S5PC11X_GPH0_BASE	(S5PC11X_VA_GPIO + 0x0C00)
#define S5PC11X_GPH1_BASE	(S5PC11X_VA_GPIO + 0x0C20)
#define S5PC11X_GPH2_BASE	(S5PC11X_VA_GPIO + 0x0C40)
#define S5PC11X_GPH3_BASE	(S5PC11X_VA_GPIO + 0x0C60)
#define S5PC11X_GPI_BASE	(S5PC11X_VA_GPIO + 0x0220)
#define S5PC11X_GPJ0_BASE	(S5PC11X_VA_GPIO + 0x0240)
#define S5PC11X_GPJ1_BASE	(S5PC11X_VA_GPIO + 0x0260)
#define S5PC11X_GPJ2_BASE	(S5PC11X_VA_GPIO + 0x0280)
#define S5PC11X_GPJ3_BASE	(S5PC11X_VA_GPIO + 0x02A0)
#define S5PC11X_GPJ4_BASE	(S5PC11X_VA_GPIO + 0x02C0)
#define S5PC11X_MP01_BASE	(S5PC11X_VA_GPIO + 0x02E0)
#define S5PC11X_MP02_BASE	(S5PC11X_VA_GPIO + 0x0300)
#define S5PC11X_MP03_BASE	(S5PC11X_VA_GPIO + 0x0320)
#define S5PC11X_MP04_BASE	(S5PC11X_VA_GPIO + 0x0340)
#define S5PC11X_MP05_BASE	(S5PC11X_VA_GPIO + 0x0360)
#define S5PC11X_MP06_BASE	(S5PC11X_VA_GPIO + 0x0380)
#define S5PC11X_MP07_BASE	(S5PC11X_VA_GPIO + 0x03A0)
#define S5PC11X_MP10_BASE	(S5PC11X_VA_GPIO + 0x03C0)
#define S5PC11X_MP11_BASE	(S5PC11X_VA_GPIO + 0x03E0)
#define S5PC11X_MP12_BASE	(S5PC11X_VA_GPIO + 0x0400)
#define S5PC11X_MP13_BASE	(S5PC11X_VA_GPIO + 0x0420)
#define S5PC11X_MP14_BASE	(S5PC11X_VA_GPIO + 0x0440)
#define S5PC11X_MP15_BASE	(S5PC11X_VA_GPIO + 0x0460)
#define S5PC11X_MP16_BASE	(S5PC11X_VA_GPIO + 0x0480)
#define S5PC11X_MP17_BASE	(S5PC11X_VA_GPIO + 0x04A0)
#define S5PC11X_MP18_BASE	(S5PC11X_VA_GPIO + 0x04C0)
#define S5PC11X_MP20_BASE	(S5PC11X_VA_GPIO + 0x04E0)
#define S5PC11X_MP21_BASE	(S5PC11X_VA_GPIO + 0x0500)
#define S5PC11X_MP22_BASE	(S5PC11X_VA_GPIO + 0x0520)
#define S5PC11X_MP23_BASE	(S5PC11X_VA_GPIO + 0x0540)
#define S5PC11X_MP24_BASE	(S5PC11X_VA_GPIO + 0x0560)
#define S5PC11X_MP25_BASE	(S5PC11X_VA_GPIO + 0x0580)
#define S5PC11X_MP26_BASE	(S5PC11X_VA_GPIO + 0x05A0)
#define S5PC11X_MP27_BASE	(S5PC11X_VA_GPIO + 0x05C0)
#define S5PC11X_MP28_BASE	(S5PC11X_VA_GPIO + 0x05E0)

#define S5PC11X_GPIOREG(x)		(S5PC11X_VA_GPIO + (x))

#define S5PC11X_EINT30CON		S5PC11X_GPIOREG(0xE00)		/* EINT0  ~ EINT7  */
#define S5PC11X_EINT31CON		S5PC11X_GPIOREG(0xE04)		/* EINT8  ~ EINT15 */
#define S5PC11X_EINT32CON		S5PC11X_GPIOREG(0xE08)		/* EINT16 ~ EINT23 */
#define S5PC11X_EINT33CON		S5PC11X_GPIOREG(0xE0C)		/* EINT24 ~ EINT31 */
#define S5PC11X_EINTCON(x)		(S5PC11X_EINT30CON+x*0x4)	/* EINT0  ~ EINT31  */

#define S5PC11X_EINT30FLTCON0		S5PC11X_GPIOREG(0xE80)		/* EINT0  ~ EINT3  */
#define S5PC11X_EINT30FLTCON1		S5PC11X_GPIOREG(0xE84)
#define S5PC11X_EINT31FLTCON0		S5PC11X_GPIOREG(0xE88)		/* EINT8 ~  EINT11 */
#define S5PC11X_EINT31FLTCON1		S5PC11X_GPIOREG(0xE8C)
#define S5PC11X_EINT32FLTCON0		S5PC11X_GPIOREG(0xE90)
#define S5PC11X_EINT32FLTCON1		S5PC11X_GPIOREG(0xE94)
#define S5PC11X_EINT33FLTCON0		S5PC11X_GPIOREG(0xE98)
#define S5PC11X_EINT33FLTCON1		S5PC11X_GPIOREG(0xE9C)
#define S5PC11X_EINTFLTCON(x)		(S5PC11X_EINT30FLTCON0+x*0x4)	/* EINT0  ~ EINT31 */

#define S5PC11X_EINT30MASK		S5PC11X_GPIOREG(0xF00)		/* EINT30[0] ~  EINT30[7]  */
#define S5PC11X_EINT31MASK		S5PC11X_GPIOREG(0xF04)		/* EINT31[0] ~  EINT31[7] */
#define S5PC11X_EINT32MASK		S5PC11X_GPIOREG(0xF08)		/* EINT32[0] ~  EINT32[7] */
#define S5PC11X_EINT33MASK		S5PC11X_GPIOREG(0xF0C)		/* EINT33[0] ~  EINT33[7] */
#define S5PC11X_EINTMASK(x)		(S5PC11X_EINT30MASK+x*0x4)	/* EINT0 ~  EINT31  */

#define S5PC11X_EINT30PEND		S5PC11X_GPIOREG(0xF40)		/* EINT30[0] ~  EINT30[7]  */
#define S5PC11X_EINT31PEND		S5PC11X_GPIOREG(0xF44)		/* EINT31[0] ~  EINT31[7] */
#define S5PC11X_EINT32PEND		S5PC11X_GPIOREG(0xF48)		/* EINT32[0] ~  EINT32[7] */
#define S5PC11X_EINT33PEND		S5PC11X_GPIOREG(0xF4C)		/* EINT33[0] ~  EINT33[7] */
#define S5PC11X_EINTPEND(x)		(S5PC11X_EINT30PEND+x*0x4)	/* EINT0 ~  EINT31  */

#define eint_offset(irq)		((irq) < IRQ_EINT16_31 ? ((irq)-IRQ_EINT0) :  \
					(irq-S3C_IRQ_EINT_BASE))
					
#define eint_irq_to_bit(irq)		(1 << (eint_offset(irq) & 0x7))

#define eint_conf_reg(irq)		((eint_offset(irq)) >> 3)
#define eint_filt_reg(irq)		((eint_offset(irq)) >> 2)
#define eint_mask_reg(irq)		((eint_offset(irq)) >> 3)
#define eint_pend_reg(irq)		((eint_offset(irq)) >> 3)


#endif /* __ASM_PLAT_S5PC11X_REGS_GPIO_H */

