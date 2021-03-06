/* linux/arch/arm/plat-s5p64xx/include/plat/regs-gpio.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5P64XX - GPIO register definitions
 */

#ifndef __ASM_PLAT_S5P64XX_REGS_GPIO_H
#define __ASM_PLAT_S5P64XX_REGS_GPIO_H __FILE__

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
#include <plat/gpio-bank-j0.h>
#include <plat/gpio-bank-j1.h>
#include <plat/gpio-bank-j2.h>
#include <plat/gpio-bank-j3.h>
#include <plat/gpio-bank-j4.h>
#include <plat/gpio-bank-h0.h>
#include <plat/gpio-bank-h1.h>
#include <plat/gpio-bank-h2.h>
#include <plat/gpio-bank-h3.h>
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

#include <mach/map.h>

/* Base addresses for each of the banks */

#define S5P64XX_GPA0_BASE	(S5P64XX_VA_GPIO + 0x0000)
#define S5P64XX_GPA1_BASE	(S5P64XX_VA_GPIO + 0x0020)
#define S5P64XX_GPB_BASE	(S5P64XX_VA_GPIO + 0x0040)
#define S5P64XX_GPC0_BASE	(S5P64XX_VA_GPIO + 0x0060)
#define S5P64XX_GPC1_BASE	(S5P64XX_VA_GPIO + 0x0080)
#define S5P64XX_GPD0_BASE	(S5P64XX_VA_GPIO + 0x00A0)
#define S5P64XX_GPD1_BASE	(S5P64XX_VA_GPIO + 0x00C0)
#define S5P64XX_GPE0_BASE	(S5P64XX_VA_GPIO + 0x00E0)
#define S5P64XX_GPE1_BASE	(S5P64XX_VA_GPIO + 0x0100)
#define S5P64XX_GPF0_BASE	(S5P64XX_VA_GPIO + 0x0120)
#define S5P64XX_GPF1_BASE	(S5P64XX_VA_GPIO + 0x0140)
#define S5P64XX_GPF2_BASE	(S5P64XX_VA_GPIO + 0x0160)
#define S5P64XX_GPF3_BASE	(S5P64XX_VA_GPIO + 0x0180)
#define S5P64XX_GPG0_BASE	(S5P64XX_VA_GPIO + 0x01A0)
#define S5P64XX_GPG1_BASE	(S5P64XX_VA_GPIO + 0x01C0)
#define S5P64XX_GPG2_BASE	(S5P64XX_VA_GPIO + 0x01E0)

#define S5P64XX_GPJ0_BASE	(S5P64XX_VA_GPIO + 0x0200)
#define S5P64XX_GPJ1_BASE	(S5P64XX_VA_GPIO + 0x0220)
#define S5P64XX_GPJ2_BASE	(S5P64XX_VA_GPIO + 0x0240)
#define S5P64XX_GPJ3_BASE	(S5P64XX_VA_GPIO + 0x0260)
#define S5P64XX_GPJ4_BASE	(S5P64XX_VA_GPIO + 0x0280)

#define S5P64XX_GPH0_BASE	(S5P64XX_VA_GPIO + 0x0C00)
#define S5P64XX_GPH1_BASE	(S5P64XX_VA_GPIO + 0x0C20)
#define S5P64XX_GPH2_BASE	(S5P64XX_VA_GPIO + 0x0C40)
#define S5P64XX_GPH3_BASE	(S5P64XX_VA_GPIO + 0x0C60)

#define S5P64XX_MP01_BASE	(S5P64XX_VA_GPIO + 0x02A0)
#define S5P64XX_MP02_BASE	(S5P64XX_VA_GPIO + 0x02C0)
#define S5P64XX_MP03_BASE	(S5P64XX_VA_GPIO + 0x02E0)
#define S5P64XX_MP04_BASE	(S5P64XX_VA_GPIO + 0x0300)
#define S5P64XX_MP05_BASE	(S5P64XX_VA_GPIO + 0x0320)
#define S5P64XX_MP06_BASE	(S5P64XX_VA_GPIO + 0x0340)
#define S5P64XX_MP07_BASE	(S5P64XX_VA_GPIO + 0x0360)

#define S5P64XX_MP10_BASE	(S5P64XX_VA_GPIO + 0x0380)
#define S5P64XX_MP11_BASE	(S5P64XX_VA_GPIO + 0x03A0)
#define S5P64XX_MP12_BASE	(S5P64XX_VA_GPIO + 0x03C0)
#define S5P64XX_MP13_BASE	(S5P64XX_VA_GPIO + 0x03E0)
#define S5P64XX_MP14_BASE	(S5P64XX_VA_GPIO + 0x0400)
#define S5P64XX_MP15_BASE	(S5P64XX_VA_GPIO + 0x0420)
#define S5P64XX_MP16_BASE	(S5P64XX_VA_GPIO + 0x0440)
#define S5P64XX_MP17_BASE	(S5P64XX_VA_GPIO + 0x0460)
#define S5P64XX_MP18_BASE	(S5P64XX_VA_GPIO + 0x0480)


#endif /* __ASM_PLAT_S5P64XX_REGS_GPIO_H */

