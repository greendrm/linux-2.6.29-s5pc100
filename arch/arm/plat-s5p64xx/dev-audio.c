/* linux/arch/arm/plat-s3c/dev-audio.c
 *
 * Copyright 2009 Wolfson Microelectronics
 *      Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>

#include <mach/map.h>
#include <mach/s3c-dma.h>

#include <plat/devs.h>
#include <plat/audio.h>
#include <plat/gpio-bank-c0.h>
#include <plat/gpio-bank-c1.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-clock.h>


static int s5p6442_cfg_i2s(struct platform_device *pdev)
{
	/* configure GPIO for i2s port */
	switch (pdev->id) {
	case 0:
		s3c_gpio_cfgpin(S5P64XX_GPC0(0), S5P64XX_GPC0_0_I2S_0_SCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC0(1), S5P64XX_GPC0_1_I2S_0_CDCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC0(2), S5P64XX_GPC0_2_I2S_0_LRCK);
		s3c_gpio_cfgpin(S5P64XX_GPC0(3), S5P64XX_GPC0_3_I2S_0_SDI);
		s3c_gpio_cfgpin(S5P64XX_GPC0(4), S5P64XX_GPC0_4_I2S_0_SDO);
		break;

	case 1:
		s3c_gpio_cfgpin(S5P64XX_GPC1(0), S5P64XX_GPC1_0_I2S_1_SCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC1(1), S5P64XX_GPC1_1_I2S_1_CDCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC1(2), S5P64XX_GPC1_2_I2S_1_LRCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC1(3), S5P64XX_GPC1_3_I2S_1_SDI);
		s3c_gpio_cfgpin(S5P64XX_GPC1(4), S5P64XX_GPC1_4_I2S_1_SDO);
		break;

	default:
		printk("Invalid Device %d!\n", pdev->id);
		return -EINVAL;
	}

	return 0;
}

static struct s3c_audio_pdata s3c_i2s_pdata = {
	.cfg_gpio = s5p6442_cfg_i2s,
};

static struct resource s5p6442_iis0_resource[] = {
	[0] = {
		.start = S5P64XX_PA_IIS0, /* V50 */
		.end   = S5P64XX_PA_IIS0 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device s5p6442_device_iis0 = {
	.name		  = "s5p-iis",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s5p6442_iis0_resource),
	.resource	  = s5p6442_iis0_resource,
	.dev = {
		.platform_data = &s3c_i2s_pdata,
	},
};
EXPORT_SYMBOL(s5p6442_device_iis0);

static struct resource s5p6442_iis1_resource[] = {
	[0] = {
		.start = S5P64XX_PA_IIS1,
		.end   = S5P64XX_PA_IIS1 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device s5p6442_device_iis1 = {
	.name		  = "s5p-iis",
	.id		  = 1,
	.num_resources	  = ARRAY_SIZE(s5p6442_iis1_resource),
	.resource	  = s5p6442_iis1_resource,
	.dev = {
		.platform_data = &s3c_i2s_pdata,
	},
};
EXPORT_SYMBOL(s5p6442_device_iis1);


/* PCM Controller platform_devices */

static int s5p6442_pcm_cfg_gpio(struct platform_device *pdev)
{
	switch (pdev->id) {
	case 0:
		s3c_gpio_cfgpin(S5P64XX_GPC0(0), S5P64XX_GPC0_0_PCM_0_SCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC0(1), S5P64XX_GPC0_1_PCM_0_EXTCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC0(2), S5P64XX_GPC0_2_PCM_0_FSYNC);
		s3c_gpio_cfgpin(S5P64XX_GPC0(3), S5P64XX_GPC0_3_PCM_0_SIN);
		s3c_gpio_cfgpin(S5P64XX_GPC0(4), S5P64XX_GPC0_4_PCM_0_SOUT);
		break;

	case 1:
		s3c_gpio_cfgpin(S5P64XX_GPC1(0), S5P64XX_GPC1_0_PCM_1_SCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC1(1), S5P64XX_GPC1_1_PCM_1_EXTCLK);
		s3c_gpio_cfgpin(S5P64XX_GPC1(2), S5P64XX_GPC1_2_PCM_1_FSYNC);
		s3c_gpio_cfgpin(S5P64XX_GPC1(3), S5P64XX_GPC1_3_PCM_1_SIN);
		s3c_gpio_cfgpin(S5P64XX_GPC1(4), S5P64XX_GPC1_4_PCM_1_SOUT);
		break;

	default:
		printk(KERN_DEBUG "Invalid PCM Controller number!");
		return -EINVAL;
	}

	return 0;
}

struct s3c_audio_pdata s3c_pcm_pdata = {
	.cfg_gpio = s5p6442_pcm_cfg_gpio,
};

static struct resource s5p6442_pcm0_resource[] = {
	[0] = {
		.start = S5P64XX_PA_PCM0,
		.end   = S5P64XX_PA_PCM0 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_PCM0_TX,
		.end   = DMACH_PCM0_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_PCM0_RX,
		.end   = DMACH_PCM0_RX,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s5p6442_device_pcm0 = {
	.name		  = "samsung-pcm",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s5p6442_pcm0_resource),
	.resource	  = s5p6442_pcm0_resource,
	.dev = {
		.platform_data = &s3c_pcm_pdata,
	},
};
EXPORT_SYMBOL(s5p6442_device_pcm0);

static struct resource s5p6442_pcm1_resource[] = {
	[0] = {
		.start = S5P64XX_PA_PCM1,
		.end   = S5P64XX_PA_PCM1 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_PCM1_TX,
		.end   = DMACH_PCM1_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_PCM1_RX,
		.end   = DMACH_PCM1_RX,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s5p6442_device_pcm1 = {
	.name		  = "samsung-pcm",
	.id		  = 1,
	.num_resources	  = ARRAY_SIZE(s5p6442_pcm1_resource),
	.resource	  = s5p6442_pcm1_resource,
	.dev = {
		.platform_data = &s3c_pcm_pdata,
	},
};
EXPORT_SYMBOL(s5p6442_device_pcm1);
