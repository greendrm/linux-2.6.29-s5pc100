/* linux/arch/arm/mach-s5p6442/dma.c
 *
 * Copyright (c) 2003-2005,2006 Samsung Electronics
 *
 * S5P6442 DMA selection
 *
 * http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>

#include <asm/dma.h>
#include <mach/dma.h>
#include <asm/io.h>

#include <plat/dma.h>
#include <plat/cpu.h>


/* DMAC */
#define MAP0(x) { \
		[0]	= (x) | DMA_CH_VALID,	\
		[1]	= (x) | DMA_CH_VALID,	\
		[2]	= (x) | DMA_CH_VALID,	\
		[3]	= (x) | DMA_CH_VALID,	\
		[4]	= (x) | DMA_CH_VALID,	\
		[5]     = (x) | DMA_CH_VALID,	\
		[6]	= (x) | DMA_CH_VALID,	\
		[7]     = (x) | DMA_CH_VALID,	\
	}


/* DMA request sources  */
#define S3C_DMA0_UART0CH0	0
#define S3C_DMA0_UART0CH1	1
#define S3C_DMA0_UART1CH0	2
#define S3C_DMA0_UART1CH1	3
#define S3C_DMA0_UART2CH0	4
#define S3C_DMA0_UART2CH1	5
#define S3C_DMA0_UART3CH0	6
#define S3C_DMA0_UART3CH1	7
#define S3C_DMA0_PCM0_TX	10
#define S3C_DMA0_PCM0_RX	11
#define S3C_DMA0_I2S0_TX	12
#define S3C_DMA0_I2S0_RX	13
#define S3C_DMA0_SPI0_TX	17
#define S3C_DMA0_SPI0_RX	16
#define S3C_DMA0_GPS		24
#define S3C_DMA0_PWM		29
#define S3C_DMA0_EXTERNAL	31

#define S3C_DMA_M2M		0


static struct s3c_dma_map __initdata s5p6442_dma_mappings[] = {

	[DMACH_I2S_IN] = {
		.name		= "i2s0-in",
		.channels	= MAP0(S3C_DMA0_I2S0_RX),
		.hw_addr.from	= S3C_DMA0_I2S0_RX,
	},
	[DMACH_I2S_OUT] = {
		.name		= "i2s0-out",
		.channels	= MAP0(S3C_DMA0_I2S0_TX),
		.hw_addr.to	= S3C_DMA0_I2S0_TX,
	},
	[DMACH_SPI0_IN] = {
		.name		= "spi0-in",
		.channels	= MAP0(S3C_DMA0_SPI0_RX),
		.hw_addr.from	= S3C_DMA0_SPI0_RX,
	},
	[DMACH_SPI0_OUT] = {
		.name		= "spi0-out",
		.channels	= MAP0(S3C_DMA0_SPI0_TX),
		.hw_addr.to	= S3C_DMA0_SPI0_TX,
	},
	[DMACH_AC97_PCM_OUT] = {
		.name		= "ac97-pcm-out",
		.channels	= MAP0(S3C_DMA0_PCM0_TX),
		.hw_addr.to	= S3C_DMA0_PCM0_TX,
	},
	[DMACH_AC97_PCM_IN] = {
		.name		= "ac97-pcm-in",
		.channels	= MAP0(S3C_DMA0_PCM0_RX),
		.hw_addr.from	= S3C_DMA0_PCM0_RX,
	},
	[DMACH_3D_M2M] = {
		.name		= "3D-M2M",
		.channels	= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
        [DMACH_I2S_V40_IN] = {           
		.name           = "i2s-v40-in", 
		.channels       = MAP0(S3C_DMA0_I2S0_RX),
		.hw_addr.from   = S3C_DMA0_I2S0_RX,     
		.sdma_sel       = 1 << S3C_DMA0_I2S0_RX,
	},                                                                               
	[DMACH_I2S_V40_OUT] = {            
		.name           = "i2s-v40-out", 
		.channels       = MAP0(S3C_DMA0_I2S0_TX), 
		.hw_addr.to     = S3C_DMA0_I2S0_TX,      
		.sdma_sel       = 1 << S3C_DMA0_I2S0_TX,
	}, 
};

static void s5p6442_dma_select(struct s3c2410_dma_chan *chan,
			       struct s3c_dma_map *map)
{
	chan->map = map;
}

static struct s3c_dma_selection __initdata s5p6442_dma_sel = {
	.select		= s5p6442_dma_select,
	.dcon_mask	= 0,
	.map		= s5p6442_dma_mappings,
	.map_size	= ARRAY_SIZE(s5p6442_dma_mappings),
};

static int __init s5p6442_dma_add(struct sys_device *sysdev)
{
	s3c_dma_init(S3C_DMA_CHANNELS, IRQ_MDMA, 0x1000);
	return s3c_dma_init_map(&s5p6442_dma_sel);
}

static struct sysdev_driver s5p6442_dma_driver = {
	.add	= s5p6442_dma_add,
};

static int __init s5p6442_dma_init(void)
{
	return sysdev_driver_register(&s5p6442_sysclass, &s5p6442_dma_driver);
}

arch_initcall(s5p6442_dma_init);


