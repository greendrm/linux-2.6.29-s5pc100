/* linux/arch/arm/plat-s5pc1xx/dev-spi.c
 *
 * Copyright 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <mach/map.h>

#include <plat/spi.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/irqs.h>

#if 1
#define dbg_printk(x...)	printk(x)
#else
#define dbg_printk(x...)	do{}while(0)
#endif

static int smi_getclcks(struct s3c_spi_mstr_info *smi)
{
	struct clk *cspi, *cp, *cm, *cf;

	cp = NULL;
	cm = NULL;
	cf = NULL;
	cspi = smi->clk;

	if(cspi == NULL){
		cspi = clk_get(&smi->pdev->dev, "spi");
		if(IS_ERR(cspi)){
			printk("Unable to get spi!\n");
			return -EBUSY;
		}
	}
	dbg_printk("%s:%d Got clk=spi\n", __func__, __LINE__);

	smi->clk = cspi;
	return 0;
}

static void smi_putclcks(struct s3c_spi_mstr_info *smi)
{
	if(smi->prnt_clk != NULL)
		clk_put(smi->prnt_clk);

	clk_put(smi->clk);
}

static int smi_enclcks(struct s3c_spi_mstr_info *smi)
{
	if(smi->prnt_clk != NULL)
		clk_enable(smi->prnt_clk);

	return clk_enable(smi->clk);
}

static void smi_disclcks(struct s3c_spi_mstr_info *smi)
{
	if(smi->prnt_clk != NULL)
		clk_disable(smi->prnt_clk);

	clk_disable(smi->clk);
}

static u32 smi_getrate(struct s3c_spi_mstr_info *smi)
{
	u32 ret;
	if(smi->prnt_clk != NULL){
		ret = clk_get_rate(smi->prnt_clk);
		return ret;
	}
	else{
		ret = clk_get_rate(smi->clk);
		return ret;
	}
}

static int smi_setrate(struct s3c_spi_mstr_info *smi, u32 r)
{
 /* We don't take charge of the Src Clock, yet */
	return 0;
}

/* SPI (0) */
static struct resource s3c_spi0_resource[] = {
	[0] = {
		.start = S3C_PA_SPI0,
		.end   = S3C_PA_SPI0 + S3C_SZ_SPI0 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SPI0,
		.end   = IRQ_SPI0,
		.flags = IORESOURCE_IRQ,
	}
};

static struct s3c_spi_mstr_info sspi0_mstr_info = {
	.pdev = NULL,
	.clk = NULL,
	.prnt_clk = NULL,
	.num_slaves = 0,
	.spiclck_get = smi_getclcks,
	.spiclck_put = smi_putclcks,
	.spiclck_en = smi_enclcks,
	.spiclck_dis = smi_disclcks,
	.spiclck_setrate = smi_setrate,
	.spiclck_getrate = smi_getrate,
};

static u64 s3c_device_spi0_dmamask = 0xffffffffUL;

struct platform_device s3c_device_spi0 = {
	.name		= "s3c-spi",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_spi0_resource),
	.resource	= s3c_spi0_resource,
	.dev		= {
		.dma_mask = &s3c_device_spi0_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
		.platform_data = &sspi0_mstr_info,
	}
};
EXPORT_SYMBOL(s3c_device_spi0);

/* SPI (1) */
static struct resource s3c_spi1_resource[] = {
	[0] = {
		.start = S3C_PA_SPI1,
		.end   = S3C_PA_SPI1 + S3C_SZ_SPI1 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SPI1,
		.end   = IRQ_SPI1,
		.flags = IORESOURCE_IRQ,
	}
};

static struct s3c_spi_mstr_info sspi1_mstr_info = {
	.pdev = NULL,
	.clk = NULL,
	.prnt_clk = NULL,
	.num_slaves = 0,
	.spiclck_get = smi_getclcks,
	.spiclck_put = smi_putclcks,
	.spiclck_en = smi_enclcks,
	.spiclck_dis = smi_disclcks,
	.spiclck_setrate = smi_setrate,
	.spiclck_getrate = smi_getrate,
};

static u64 s3c_device_spi1_dmamask = 0xffffffffUL;

struct platform_device s3c_device_spi1 = {
	.name		= "s3c-spi",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_spi1_resource),
	.resource	= s3c_spi1_resource,
	.dev		= {
		.dma_mask = &s3c_device_spi1_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
		.platform_data = &sspi1_mstr_info,
	}
};
EXPORT_SYMBOL(s3c_device_spi1);

/* SPI (2) */
static struct resource s3c_spi2_resource[] = {
	[0] = {
		.start = S3C_PA_SPI2,
		.end   = S3C_PA_SPI2 + S3C_SZ_SPI2 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SPI2,
		.end   = IRQ_SPI2,
		.flags = IORESOURCE_IRQ,
	}
};

static struct s3c_spi_mstr_info sspi2_mstr_info = {
	.pdev = NULL,
	.clk = NULL,
	.prnt_clk = NULL,
	.num_slaves = 0,
	.spiclck_get = smi_getclcks,
	.spiclck_put = smi_putclcks,
	.spiclck_en = smi_enclcks,
	.spiclck_dis = smi_disclcks,
	.spiclck_setrate = smi_setrate,
	.spiclck_getrate = smi_getrate,
};

static u64 s3c_device_spi2_dmamask = 0xffffffffUL;

struct platform_device s3c_device_spi2 = {
	.name		= "s3c-spi",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_spi2_resource),
	.resource	= s3c_spi2_resource,
	.dev		= {
		.dma_mask = &s3c_device_spi2_dmamask,
		.coherent_dma_mask = 0xffffffffUL,
		.platform_data = &sspi2_mstr_info,
	}
};
EXPORT_SYMBOL(s3c_device_spi2);

void __init s3cspi_set_slaves(unsigned id, int n, struct s3c_spi_pdata const *dat)
{
	struct s3c_spi_mstr_info *pinfo;

	if(id == 0)
	   pinfo = (struct s3c_spi_mstr_info *)s3c_device_spi0.dev.platform_data;
	else if(id == 1)
	   pinfo = (struct s3c_spi_mstr_info *)s3c_device_spi1.dev.platform_data;
	else if(id == 2)
	   pinfo = (struct s3c_spi_mstr_info *)s3c_device_spi2.dev.platform_data;
	else
	   return;

	pinfo->spd = kmalloc(n * sizeof (*dat), GFP_KERNEL);
	if(!pinfo->spd)
	   return;
	memcpy(pinfo->spd, dat, n * sizeof(*dat));

	pinfo->num_slaves = n;
}
