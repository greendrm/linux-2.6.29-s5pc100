/* linux/arch/arm/plat-s5p64xx/clock.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P64XX Base clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>

/* definition for cpu freq */

#define ARM_PLL_CON 	S3C_APLL_CON
#define ARM_CLK_DIV	S3C_CLK_DIV0

#define ARM_DIV_RATIO_BIT		0
#define ARM_DIV_MASK			(0xf<<ARM_DIV_RATIO_BIT)
#define HCLK_DIV_RATIO_BIT		8
#define HCLK_DIV_MASK			(0xf<<HCLK_DIV_RATIO_BIT)

#define READ_ARM_DIV    		((__raw_readl(ARM_CLK_DIV)&ARM_DIV_MASK) + 1)
#define PLL_CALC_VAL(MDIV,PDIV,SDIV)	((1<<31)|(MDIV)<<16 |(PDIV)<<8 |(SDIV))
#define GET_ARM_CLOCK(baseclk)		s3c6400_get_pll(__raw_readl(S3C_APLL_CON),baseclk)

#define INIT_XTAL			12 * MHZ

static const u32 s3c_cpu_clock_table[][3] = {
	{532*MHZ, (0<<ARM_DIV_RATIO_BIT), (3<<HCLK_DIV_RATIO_BIT)},
	{266*MHZ, (1<<ARM_DIV_RATIO_BIT), (1<<HCLK_DIV_RATIO_BIT)},
	{133*MHZ, (3<<ARM_DIV_RATIO_BIT), (0<<HCLK_DIV_RATIO_BIT)},
	//{APLL, DIVarm, DIVhclk}	
};

unsigned long s3c_fclk_get_rate(void)
{
	unsigned long apll_con;
	unsigned long clk_div0_tmp;
	unsigned long m = 0;
	unsigned long p = 0;
	unsigned long s = 0;
	unsigned long ret;

	apll_con = __raw_readl(S5P_APLL_CON);
	clk_div0_tmp = __raw_readl(S5P_CLK_DIV0) & 0xf;

	m = (apll_con >> 16) & 0x3ff;
	p = (apll_con >> 8) & 0x3f;
	s = apll_con & 0x3;

	ret = (m * (INIT_XTAL / (p * (1 << s))));

	return (ret / (clk_div0_tmp + 1));
}

unsigned long s3c_fclk_round_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}

int s3c_fclk_set_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}

static int inline s5p64xx_gate(void __iomem *reg,
				struct clk *clk,
				int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	u32 con;

	con = __raw_readl(reg);

	if (enable)
		con |= ctrlbit;
	else
		con &= ~ctrlbit;

	__raw_writel(con, reg);
	return 0;
}


static int s5p64xx_clk_ip0_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP0, clk, enable);
}

static int s5p64xx_clk_ip1_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP1, clk, enable);
}

static int s5p64xx_clk_ip2_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP2, clk, enable);
}

static int s5p64xx_clk_ip3_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP3, clk, enable);
}

static int s5p64xx_clk_ip4_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP4, clk, enable);
}

int s5p64xx_clk_sclk0_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_SCLK0, clk, enable);
}

int s5p64xx_clk_sclk1_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_SCLK1, clk, enable);
}

static struct clk init_clocks_disable[] = {
#if 0	
	{
		.name		= "nand",
		.id		= -1,
		.parent		= &clk_h,
	}, {
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S3C_CLKCON_PCLK_TSADC,
	}, {
		.name		= "i2c",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S3C_CLKCON_PCLK_IIC0,
	}, {
		.name		= "iis_v40",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S3C_CLKCON_PCLK_IIS2,
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S3C_CLKCON_PCLK_SPI0,
	}, {
		.name		= "spi",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S3C_CLKCON_PCLK_SPI1,
	}, {
		.name		= "sclk_spi_48",
		.id		= 0,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S3C_CLKCON_SCLK0_SPI0_48,
	}, {
		.name		= "sclk_spi_48",
		.id		= 1,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S3C_CLKCON_SCLK0_SPI1_48,
	}, {
		.name		= "48m",
		.id		= 0,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S3C_CLKCON_SCLK0_MMC0_48,
	}, {
		.name		= "48m",
		.id		= 1,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S3C_CLKCON_SCLK0_MMC1_48,
	}, {
		.name		= "48m",
		.id		= 2,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S3C_CLKCON_SCLK0_MMC2_48,
	}, {
		.name    	= "otg",
		.id	   	= -1,
		.parent  	= &clk_h,
		.enable  	= s5p64xx_hclk0_ctrl,
		.ctrlbit 	= S3C_CLKCON_HCLK0_USB
	}, {
		.name    	= "post",
		.id	   	= -1,
		.parent  	= &clk_h,
		.enable  	= s5p64xx_hclk0_ctrl,
		.ctrlbit 	= S3C_CLKCON_HCLK0_POST0
	},
#endif	

};

static struct clk init_clocks[] = {
	
	{
		.name		= "systimer",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SYSTIMER,
	}, {
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_WDT,
	}, {
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_RTC,
	}, {
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART0,
	}, {
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART1,
	}, {
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART2,
	}, {
		.name		= "lcd",
		.id		= -1,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_FIMD,
	}, {
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_TSADC,
	}, {
		.name		= "keypad",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_KEYIF,
	}, {
		.name		= "hsmmc",
		.id		= 0,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC0,
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC1,
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC2,
	},
};

static struct clk *clks[] __initdata = {
	&clk_ext,
	&clk_epll,
};

void __init s5p64xx_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));

	clkp = init_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {

		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}

		(clkp->enable)(clkp, 0);
	}
}
