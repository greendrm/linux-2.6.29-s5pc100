/* linux/arch/arm/plat-s5p64xx/dev-spi.c
 *
 * Copyright (C) 2009 Samsung Electronics Ltd.
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <mach/dma.h>
#include <mach/map.h>
#include <mach/gpio.h>

#include <plat/s5p-spi.h>
#include <plat/spi.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>

static char *spi_src_clks[] = {"pclk", "spiext"};

/* SPI Controller platform_devices */

/* Since we emulate multi-cs capability, we do not touch the GPB-1.
 * The emulated CS is toggled by board specific mechanism, as it can
 * be either some immediate GPIO or some signal out of some other
 * chip in between ... or some yet another way.
 * We simply do not assume anything about CS.
 */
static int s5p64xx_spi_cfg_gpio(struct platform_device *pdev)
{
	switch (pdev->id) {
	case 0:
		s3c_gpio_cfgpin(S5P64XX_GPB(0), S5P64XX_GPB0_SPI_0_CLK);
		s3c_gpio_cfgpin(S5P64XX_GPB(2), S5P64XX_GPB2_SPI_0_MISO);
		s3c_gpio_cfgpin(S5P64XX_GPB(3), S5P64XX_GPB3_SPI_0_MOSI);
		s3c_gpio_setpull(S5P64XX_GPB(0), S3C_GPIO_PULL_UP);
		s3c_gpio_setpull(S5P64XX_GPB(2), S3C_GPIO_PULL_UP);
		s3c_gpio_setpull(S5P64XX_GPB(3), S3C_GPIO_PULL_UP);
		break;

	default:
		dev_err(&pdev->dev, "Invalid SPI Controller number!");
		return -EINVAL;
	}

	return 0;
}

static struct resource s5p64xx_spi0_resource[] = {
	[0] = {
		.start = S5P64XX_PA_SPI0,
		.end   = S5P64XX_PA_SPI0 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_SPI0_TX,
		.end   = DMACH_SPI0_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_SPI0_RX,
		.end   = DMACH_SPI0_RX,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.start = IRQ_SPI0,
		.end   = IRQ_SPI0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct s3c64xx_spi_cntrlr_info s5p64xx_spi0_pdata = {
	.cfg_gpio = s5p64xx_spi_cfg_gpio,
	.fifo_lvl_mask = 0x1ff,
	.rx_lvl_offset = 15,
};

static u64 spi_dmamask = DMA_BIT_MASK(32);

struct platform_device s5p64xx_device_spi0 = {
	.name		  = "s3c64xx-spi",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s5p64xx_spi0_resource),
	.resource	  = s5p64xx_spi0_resource,
	.dev = {
		.dma_mask		= &spi_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data = &s5p64xx_spi0_pdata,
	},
};
EXPORT_SYMBOL(s5p64xx_device_spi0);

void __init s3c64xx_spi_set_info(int cntrlr, int src_clk_nr, int num_cs)
{
	/* Reject invalid configuration */
	if (!num_cs || src_clk_nr < 0
			|| src_clk_nr > S5P64XX_SPI_SRCCLK_SPIEXT) {
		printk(KERN_ERR "%s: Invalid SPI configuration\n", __func__);
		return;
	}

	switch (cntrlr) {
	case 0:
		s5p64xx_spi0_pdata.num_cs = num_cs;
		s5p64xx_spi0_pdata.src_clk_nr = src_clk_nr;
		s5p64xx_spi0_pdata.src_clk_name = spi_src_clks[src_clk_nr];
		break;
	default:
		printk(KERN_ERR "%s: Invalid SPI controller(%d)\n",
							__func__, cntrlr);
		return;
	}
}
