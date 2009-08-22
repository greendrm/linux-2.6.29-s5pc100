/* linux/arch/arm/plat-s5p64xx/s5p6442-clock.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P6442 based common clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/sysdev.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>

#include <plat/regs-clock.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>

/* fin_apll, fin_mpll and fin_epll are all the same clock, which we call
 * ext_xtal_mux for want of an actual name from the manual.
*/

struct clk clk_ext_xtal_mux = {
	.name		= "ext_xtal",
	.id		= -1,
	.rate		= XTAL_FREQ,
};

#define clk_fin_apll clk_ext_xtal_mux
#define clk_fin_mpll clk_ext_xtal_mux
#define clk_fin_epll clk_ext_xtal_mux

#define clk_fout_mpll	clk_mpll

struct clk_sources {
	unsigned int	nr_sources;
	struct clk	**sources;
};

struct clksrc_clk {
	struct clk		clk;
	unsigned int		mask;
	unsigned int		shift;

	struct clk_sources	*sources;

	unsigned int		divider_shift;
	void __iomem		*reg_divider;
	void __iomem		*reg_source;
};

struct clk clk_fout_apll = {
	.name		= "fout_apll",
	.id		= -1,
};

static struct clk *clk_src_apll_list[] = {
	[0] = &clk_fin_apll,
	[1] = &clk_fout_apll,
};

static struct clk_sources clk_src_apll = {
	.sources	= clk_src_apll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_apll_list),
};

struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,		
	},
	.shift		= S5P_CLKSRC0_APLL_SHIFT,
	.mask		= S5P_CLKSRC0_APLL_MASK,
	.sources	= &clk_src_apll,
	.reg_source	= S5P_CLK_SRC0,
};

struct clk clk_dout_a2m = {
	.name = "dout_a2m",
	.id = -1,
	.parent = &clk_mout_apll.clk,
};

static unsigned long s5p64xx_clk_doutapll_get_rate(struct clk *clk)
{
  	unsigned long rate = clk_get_rate(clk->parent);

	rate /= (((__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_APLL_MASK) >> S5P_CLKDIV0_APLL_SHIFT) + 1);

	return rate;
}

int s5p64xx_clk_doutapll_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *temp_clk = clk;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(temp_clk, rate);
	div = clk_get_rate(temp_clk->parent) / rate;

	val = __raw_readl(S5P_CLK_DIV0);
	val &=~ S5P_CLKDIV0_APLL_MASK;
	val |= (div - 1) << S5P_CLKDIV0_APLL_SHIFT;
	__raw_writel(val, S5P_CLK_DIV0);

	temp_clk->rate = rate;

	return 0;
}

struct clk clk_dout_apll = {
	.name = "dout_apll",
	.id = -1,
	.parent = &clk_mout_apll.clk,
	.get_rate = s5p64xx_clk_doutapll_get_rate,
	.set_rate = s5p64xx_clk_doutapll_set_rate,
};

static inline struct clksrc_clk *to_clksrc(struct clk *clk)
{
	return container_of(clk, struct clksrc_clk, clk);
}

int fout_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con0 = __raw_readl(S5P_EPLL_CON) & ~ ctrlbit;

	if(enable)
	   __raw_writel(epll_con0 | ctrlbit, S5P_EPLL_CON);
	else
	   __raw_writel(epll_con0, S5P_EPLL_CON);

	return 0;
}

unsigned long fout_get_rate(struct clk *clk)
{
	return clk->rate;
}

int fout_set_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}

struct clk clk_fout_epll = {
	.name		= "fout_epll",
	.id		= -1,
	.ctrlbit	= (1<<31),
	.enable		= fout_enable,
	.get_rate	= fout_get_rate,
	.set_rate	= fout_set_rate,
};

int mout_set_parent(struct clk *clk, struct clk *parent)
{
	int src_nr = -1;
	int ptr;
	u32 clksrc;
	struct clksrc_clk *sclk = to_clksrc(clk);
	struct clk_sources *srcs = sclk->sources;

	clksrc = __raw_readl(S5P_CLK_SRC0);

	for (ptr = 0; ptr < srcs->nr_sources; ptr++)
		if (srcs->sources[ptr] == parent) {
			src_nr = ptr;
			break;
		}

	if (src_nr >= 0) {
		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;
		__raw_writel(clksrc, S5P_CLK_SRC0);
		clk->parent = parent;
		return 0;
	}

	return -EINVAL;
}

static struct clk *clk_src_epll_list[] = {
	[0] = &clk_fin_epll,
	[1] = &clk_fout_epll,
};

static struct clk_sources clk_src_epll = {
	.sources	= clk_src_epll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_epll_list),
};

struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_EPLL_SHIFT,
	.mask		= S5P_CLKSRC0_EPLL_MASK,
	.sources	= &clk_src_epll,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_mpll_list[] = {
	[0] = &clk_fin_mpll,
	[1] = &clk_fout_mpll,
};

static struct clk_sources clk_src_mpll = {
	.sources	= clk_src_mpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_mpll_list),
};

struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MPLL_SHIFT,
	.mask		= S5P_CLKSRC0_MPLL_MASK,
	.sources	= &clk_src_mpll,
	.reg_source	= S5P_CLK_SRC0,
};


static struct clk *clk_src_muxd0_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_dout_a2m,
};

static struct clk_sources clk_src_muxd0 = {
	.sources	= clk_src_muxd0_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd0_list),
};

struct clksrc_clk clk_mout_d0 = {
	.clk = {
		.name		= "mout_d0",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MUX166_SHIFT,
	.mask		= S5P_CLKSRC0_MUX166_MASK,
	.sources	= &clk_src_muxd0,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_muxd1_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_dout_a2m,
};

static struct clk_sources clk_src_muxd1 = {
	.sources	= clk_src_muxd1_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd1_list),
};

struct clksrc_clk clk_mout_d1 = {
	.clk = {
		.name		= "mout_d1",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MUX133_SHIFT,
	.mask		= S5P_CLKSRC0_MUX133_MASK,
	.sources	= &clk_src_muxd1,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_muxd0sync_list[] = {
	[0] = &clk_mout_d0,
	[1] = &clk_dout_apll,
};

static struct clk_sources clk_src_muxd0sync = {
	.sources	= clk_src_muxd0sync_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd0sync_list),
};

struct clksrc_clk clk_mout_d0sync = {
	.clk = {
		.name		= "mout_d0sync",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC2_MUX166SYNC_SEL_SHIFT,
	.mask		= S5P_CLKSRC2_MUX166SYNC_SEL_MASK,
	.sources	= &clk_src_muxd0sync,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clk *clk_src_muxd1sync_list[] = {
	[0] = &clk_mout_d1,
	[1] = &clk_dout_apll,
};

static struct clk_sources clk_src_muxd1sync = {
	.sources	= clk_src_muxd1sync_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd1sync_list),
};

struct clksrc_clk clk_mout_d1sync = {
	.clk = {
		.name		= "mout_d1sync",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC2_MUX133SYNC_SEL_SHIFT,
	.mask		= S5P_CLKSRC2_MUX133SYNC_SEL_MASK,
	.sources	= &clk_src_muxd1sync,
	.reg_source	= S5P_CLK_SRC2,
};

struct clk clk_dout_d0 = {
	.name = "dout_d0",
	.id = -1,
	.parent = &clk_mout_d0sync.clk,
};

struct clk clk_dout_d1 = {
	.name = "dout_d1",
	.id = -1,
	.parent = &clk_mout_d1sync.clk,
};



static struct clk *clkset_uart_list[] = {
	&clk_mout_epll.clk,
	&clk_mout_mpll,
	NULL,
	NULL
};

static struct clk *clkset_mmc0_list[] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &clk_mout_mpll.clk,
        &clk_mout_epll.clk,
        NULL,
	&clk_fin_epll,
};

static struct clk *clkset_mmc1_list[] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &clk_mout_mpll.clk,
        &clk_mout_epll.clk,
        NULL,
	&clk_fin_epll,
};

static struct clk *clkset_mmc2_list[] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &clk_mout_mpll.clk,
        &clk_mout_epll.clk,
        NULL,
	&clk_fin_epll,
};

static struct clk_sources clkset_uart = {
	.sources	= clkset_uart_list,
	.nr_sources	= ARRAY_SIZE(clkset_uart_list),
};

static struct clk_sources clkset_mmc0 = {
        .sources        = clkset_mmc0_list,
        .nr_sources     = ARRAY_SIZE(clkset_mmc0_list),
};

static struct clk_sources clkset_mmc1 = {
        .sources        = clkset_mmc1_list,
        .nr_sources     = ARRAY_SIZE(clkset_mmc1_list),
};

static struct clk_sources clkset_mmc2 = {
        .sources        = clkset_mmc2_list,
        .nr_sources     = ARRAY_SIZE(clkset_mmc2_list),
};

/* The peripheral clocks are all controlled via clocksource followed
 * by an optional divider and gate stage. We currently roll this into
 * one clock which hides the intermediate clock from the mux.
 *
 * Note, the JPEG clock can only be an even divider...
 *
 * The scaler and LCD clocks depend on the S5P64XX version, and also
 * have a common parent divisor so are not included here.
 */

static unsigned long s5p64xx_getrate_clksrc(struct clk *clk)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	unsigned long rate = clk_get_rate(clk->parent);
	u32 clkdiv = __raw_readl(sclk->reg_divider);

	clkdiv >>= sclk->divider_shift;
	clkdiv &= 0xf;
	clkdiv++;

	rate /= clkdiv;
	return rate;
}

static int s5p64xx_setrate_clksrc(struct clk *clk, unsigned long rate)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	void __iomem *reg = sclk->reg_divider;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(clk, rate);
	div = clk_get_rate(clk->parent) / rate;

	val = __raw_readl(reg);
	val &= ~(0xf << sclk->divider_shift);
	val |= ((div - 1) << sclk->divider_shift);
	__raw_writel(val, reg);

	return 0;
}

static int s5p64xx_setparent_clksrc(struct clk *clk, struct clk *parent)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	struct clk_sources *srcs = sclk->sources;
	u32 clksrc = __raw_readl(sclk->reg_source);
	int src_nr = -1;
	int ptr;

	for (ptr = 0; ptr < srcs->nr_sources; ptr++)
		if (srcs->sources[ptr] == parent) {
			src_nr = ptr;
			break;
		}

	if (src_nr >= 0) {
		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;

		__raw_writel(clksrc, sclk->reg_source);
		return 0;
	}

	return -EINVAL;
}

static unsigned long s5p64xx_roundrate_clksrc(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate >= parent_rate) 
		rate = parent_rate;
	else {
		div = parent_rate / rate;
		if(parent_rate % rate)
			div++;

		if (div == 0)
			div = 1;
		if (div > 16)
			div = 16;

		rate = parent_rate / div;
	}

	return rate;
}

static struct clksrc_clk clk_mmc0 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 0,
		.ctrlbit	= S5P_CLKGATE_SCLK0_MMC0,
		.enable		= s5p64xx_clk_sclk0_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC0_SHIFT,
	.mask		= S5P_CLKSRC4_MMC0_MASK,
	.sources	= &clkset_mmc0,
	.divider_shift	= S5P_CLKDIV4_MMC0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source 	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc1 = {
	.clk 	= {
		.name		= "mmc_bus",
		.id		= 1,
		.ctrlbit	= S5P_CLKGATE_SCLK0_MMC1,
		.enable		= s5p64xx_clk_sclk0_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,		
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC1_SHIFT,
	.mask		= S5P_CLKSRC4_MMC1_MASK,
	.sources	= &clkset_mmc1,
	.divider_shift	= S5P_CLKDIV4_MMC1_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc2 = {
        .clk    = {
		.name		= "mmc_bus",
		.id		= 2,
		.ctrlbit	= S5P_CLKGATE_SCLK0_MMC2,
		.enable		= s5p64xx_clk_sclk0_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC2_SHIFT,
	.mask		= S5P_CLKSRC4_MMC2_MASK,
	.sources	= &clkset_mmc2,
	.divider_shift	= S5P_CLKDIV4_MMC2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

/* Clock initialisation code */

static struct clksrc_clk *init_parents[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
	&clk_mout_d0,
	&clk_mout_d1,
	&clk_mout_d0sync,
	&clk_mout_d1sync,
	&clk_mmc0,
	&clk_mmc1,
	&clk_mmc2,
};

static void __init_or_cpufreq s5p6442_set_clksrc(struct clksrc_clk *clk)
{
	struct clk_sources *srcs = clk->sources;
	u32 clksrc = __raw_readl(clk->reg_source);

	clksrc &= clk->mask;
	clksrc >>= clk->shift;

	if (clksrc > srcs->nr_sources || !srcs->sources[clksrc]) {
		printk(KERN_ERR "%s: bad source %d\n",
		       clk->clk.name, clksrc);
		return;
	}

	clk->clk.parent = srcs->sources[clksrc];

	printk(KERN_INFO "%s: source is %s (%d), rate is %ld\n",
	       clk->clk.name, clk->clk.parent->name, clksrc,
	       clk_get_rate(&clk->clk));
}

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

void __init_or_cpufreq s5p6442_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long fclk;
	unsigned long a2m;
	unsigned long hclkd0;
	unsigned long hclkd1;
	unsigned long pclkd0;
	unsigned long pclkd1;
	unsigned long epll;
	unsigned long apll;
	unsigned long mpll;
	unsigned int ptr;
	u32 clkdiv0;
	u32 clkdiv3;
	u32 mux_stat0;
	u32 mux_stat1;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	clkdiv0 = __raw_readl(S5P_CLK_DIV0);
	printk(KERN_DEBUG "%s: clkdiv0 = %08x\n", __func__, clkdiv0);

	clkdiv3 = __raw_readl(S5P_CLK_DIV3);
	printk(KERN_DEBUG "%s: clkdiv3 = %08x\n", __func__, clkdiv3);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

	apll = s5p64xx_get_pll(xtal, __raw_readl(S5P_APLL_CON), S5P64XX_PLL_APLL);
	mpll = s5p64xx_get_pll(xtal, __raw_readl(S5P_MPLL_CON), S5P64XX_PLL_MPLL);
	epll = s5p64xx_get_pll(xtal, __raw_readl(S5P_EPLL_CON), S5P64XX_PLL_EPLL);


	printk(KERN_INFO "S5P64XX: PLL settings, A=%ld.%ldMHz, M=%ld.%ldMHz," \
							" E=%ld.%ldMHz\n",
	       print_mhz(apll), print_mhz(mpll), print_mhz(epll));

	fclk = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_APLL);

	mux_stat1 = __raw_readl(S5P_CLK_MUX_STAT1);
	mux_stat0 = __raw_readl(S5P_CLK_MUX_STAT0);

	switch ((mux_stat1 & S5P_CLK_MUX_STAT1_MUX166SYNC_MASK) >> S5P_CLK_MUX_STAT1_MUX166SYNC_SHIFT) {
	case 0x1:	/* Asynchronous mode */
		switch ((mux_stat0 & S5P_CLK_MUX_STAT0_MUX166_MASK) >> S5P_CLK_MUX_STAT0_MUX166_SHIFT) {
		case 0x1:	/* MPLL source */
			hclkd0 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
			pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK83);
			break;
		case 0x2:	/* A2M source */
			a2m = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
			hclkd0 = a2m / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
			pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK83);
			break;
		default:
			break;
			
		}

		switch ((mux_stat0 & S5P_CLK_MUX_STAT0_MUX133_MASK) >> S5P_CLK_MUX_STAT0_MUX133_SHIFT) {
		case 0x1:	/* MPLL source */
			hclkd1 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
			pclkd1 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK66);
			break;
		case 0x2:	/* A2M source */
			a2m = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
			hclkd1 = a2m / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
			pclkd1 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK66);
			break;
		default:
			break;
			
		}

		break;

	case 0x2:	/* Synchronous mode */
		hclkd0 = fclk / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
		pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK83);
		hclkd1 = fclk / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
		pclkd1 = hclkd1 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK66);
		break;
	default:
		printk(KERN_ERR "failed to get sync/async mode status register\n");
		break;
		/* Synchronous mode */

	} 

	printk(KERN_INFO "S5P64XX: HCLKD0=%ld.%ldMHz, HCLKD1=%ld.%ldMHz," \
				" PCLKD0=%ld.%ldMHz, PCLKD1=%ld.%ldMHz\n",
			       print_mhz(hclkd0), print_mhz(hclkd1),
			       print_mhz(pclkd0), print_mhz(pclkd1));

	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_fout_apll.rate = apll;

	clk_f.rate = fclk;
	clk_hd0.rate = hclkd0;
	clk_pd0.rate = pclkd0;
	clk_hd1.rate = hclkd1;
	clk_pd1.rate = pclkd1;
	
	/* For backward compatibility */
	clk_h.rate = hclkd1;
	clk_p.rate = pclkd1;

	clk_set_parent(&clk_mmc0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc2.clk, &clk_mout_mpll.clk);
	
	for (ptr = 0; ptr < ARRAY_SIZE(init_parents); ptr++)
		s5p6442_set_clksrc(init_parents[ptr]);

        clk_set_rate(&clk_mmc0.clk, 50*MHZ);
        clk_set_rate(&clk_mmc1.clk, 50*MHZ);
        clk_set_rate(&clk_mmc2.clk, 50*MHZ);
}

static struct clk *clks[] __initdata = {
	&clk_ext_xtal_mux,
	&clk_mout_epll.clk,
	&clk_fout_epll,
	&clk_mout_mpll.clk,
	&clk_mmc0.clk,
	&clk_mmc1.clk,	
	&clk_mmc2.clk,
};

void __init s5p6442_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	for (ptr = 0; ptr < ARRAY_SIZE(clks); ptr++) {
		clkp = clks[ptr];
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

//	clk_mpll.parent = &clk_mout_mpll.clk;
	clk_epll.parent = &clk_mout_epll.clk;
}
