/* arch/arm/plat-s5pc11x/irq-eint.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5PC11X - Interrupt handling for IRQ_EINT(x)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <asm/hardware/vic.h>

#include <plat/regs-irqtype.h>

#include <mach/map.h>
#include <plat/cpu.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>

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
#define S5PC11X_EINTPEND(x)		(S5PC11X_EINT30PEND+x*04)	/* EINT0 ~  EINT31  */

#define eint_offset(irq)		((irq) < IRQ_EINT16_31 ? ((irq)-IRQ_EINT0) :  \
					((irq-S3C_IRQ_EINT_BASE)+IRQ_EINT16_31-IRQ_EINT0))
					
#define eint_irq_to_bit(irq)		(1 << (eint_offset(irq) & 0x7))

#define eint_conf_reg(irq)		((eint_offset(irq)) >> 3)
#define eint_filt_reg(irq)		((eint_offset(irq)) >> 2)
#define eint_mask_reg(irq)		((eint_offset(irq)) >> 3)
#define eint_pend_reg(irq)		((eint_offset(irq)) >> 3)

static inline void s3c_irq_eint_mask(unsigned int irq)
{
	u32 mask;

	mask = __raw_readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
	mask |= eint_irq_to_bit(irq);
	__raw_writel(mask, S5PC11X_EINTMASK(eint_mask_reg(irq)));
}

static void s3c_irq_eint_unmask(unsigned int irq)
{
	u32 mask;

	mask = __raw_readl(S5PC11X_EINTMASK(eint_mask_reg(irq)));
	mask &= ~(eint_irq_to_bit(irq));
	__raw_writel(mask, S5PC11X_EINTMASK(eint_mask_reg(irq)));
}

static inline void s3c_irq_eint_ack(unsigned int irq)
{
	__raw_writel(eint_irq_to_bit(irq), S5PC11X_EINTPEND(eint_pend_reg(irq)));
}

static void s3c_irq_eint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_eint_mask(irq);
	s3c_irq_eint_ack(irq);
}

static int s3c_irq_eint_set_type(unsigned int irq, unsigned int type)
{
	int offs = eint_offset(irq);
	int shift;
	u32 ctrl, mask;
	u32 newvalue = 0;

	switch (type) {
	case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No edge setting!\n");
		break;

	case IRQ_TYPE_EDGE_RISING:
		newvalue = S3C2410_EXTINT_RISEEDGE;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		newvalue = S3C2410_EXTINT_FALLEDGE;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		newvalue = S3C2410_EXTINT_BOTHEDGE;
		break;

	case IRQ_TYPE_LEVEL_LOW:
		newvalue = S3C2410_EXTINT_LOWLEV;
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		newvalue = S3C2410_EXTINT_HILEV;
		break;

	default:
		printk(KERN_ERR "No such irq type %d", type);
		return -1;
	}

	shift = (offs & 0x7) * 4;
	mask = 0x7 << shift;

	ctrl = __raw_readl(S5PC11X_EINTCON(eint_conf_reg(irq)));
	ctrl &= ~mask;
	ctrl |= newvalue << shift;
	__raw_writel(ctrl, S5PC11X_EINTCON(eint_conf_reg(irq)));

	if((0 <= offs) && (offs < 8))
		s3c_gpio_cfgpin(S5PC11X_GPH0(offs&0x7), 0xf<<((offs&0x7)*4));
	else if((8 <= offs) && (offs < 16))
		s3c_gpio_cfgpin(S5PC11X_GPH1(offs&0x7), 0xf<<((offs&0x7)*4));
	else if((16 <= offs) && (offs < 24))
		s3c_gpio_cfgpin(S5PC11X_GPH2(offs&0x7), 0xf<<((offs&0x7)*4));
	else if((24 <= offs) && (offs < 32))
		s3c_gpio_cfgpin(S5PC11X_GPH3(offs&0x7), 0xf<<((offs&0x7)*4));
	else
		printk(KERN_ERR "No such irq number %d", offs);

	return 0;
}


static struct irq_chip s3c_irq_eint = {
	.name		= "s3c-eint",
	.mask		= s3c_irq_eint_mask,
	.unmask		= s3c_irq_eint_unmask,
	.mask_ack	= s3c_irq_eint_maskack,
	.ack		= s3c_irq_eint_ack,
	.set_type	= s3c_irq_eint_set_type,
};

/* s3c_irq_demux_eint
 *
 * This function demuxes the IRQ from the group0 external interrupts,
 * from IRQ_EINT(16) to IRQ_EINT(31). It is designed to be inlined into
 * the specific handlers s3c_irq_demux_eintX_Y.
 */
static inline void s3c_irq_demux_eint(unsigned int start, unsigned int end)
{
	u32 status = __raw_readl(S5PC11X_EINTPEND((start >> 3)));
	u32 mask = __raw_readl(S5PC11X_EINTMASK((start >> 3)));
	unsigned int irq;

	status &= ~mask;
	status &= (1 << (end - start + 1)) - 1;

	for (irq = IRQ_EINT(start); irq <= IRQ_EINT(end); irq++) {
		if (status & 1)
			generic_handle_irq(irq);

		status >>= 1;
	}
}

static void s3c_irq_demux_eint16_31(unsigned int irq, struct irq_desc *desc)
{
	s3c_irq_demux_eint(16, 23);
	s3c_irq_demux_eint(24, 31);
}

/*---------------------------- EINT0 ~ EINT15 -------------------------------------*/
static void s3c_irq_vic_eint_mask(unsigned int irq)
{
	void __iomem *base = get_irq_chip_data(irq);
	
	s3c_irq_eint_mask(irq);
	
	irq &= 31;
	writel(1 << irq, base + VIC_INT_ENABLE_CLEAR);
}


static void s3c_irq_vic_eint_unmask(unsigned int irq)
{
	void __iomem *base = get_irq_chip_data(irq);
	
	s3c_irq_eint_unmask(irq);
	
	irq &= 31;
	writel(1 << irq, base + VIC_INT_ENABLE);
}


static inline void s3c_irq_vic_eint_ack(unsigned int irq)
{
	__raw_writel(eint_irq_to_bit(irq), S5PC11X_EINTPEND(eint_pend_reg(irq)));
}


static void s3c_irq_vic_eint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_vic_eint_mask(irq);
	s3c_irq_vic_eint_ack(irq);
}


static struct irq_chip s3c_irq_vic_eint = {
	.name	= "s3c_vic_eint",
	.mask	= s3c_irq_vic_eint_mask,
	.unmask	= s3c_irq_vic_eint_unmask,
	.mask_ack = s3c_irq_vic_eint_maskack,
	.ack = s3c_irq_vic_eint_ack,
	.set_type = s3c_irq_eint_set_type,
};


int __init s5pc11x_init_irq_eint(void)
{
	int irq;

	for (irq = IRQ_EINT0; irq <= IRQ_EINT15; irq++) {
		set_irq_chip(irq, &s3c_irq_vic_eint);
	}

	for (irq = IRQ_EINT(16); irq <= IRQ_EINT(31); irq++) {
		set_irq_chip(irq, &s3c_irq_eint);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler(IRQ_EINT16_31, s3c_irq_demux_eint16_31);
	return 0;
}

arch_initcall(s5pc11x_init_irq_eint);

