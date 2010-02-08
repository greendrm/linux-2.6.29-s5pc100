/*
 * s5p-i2s_sec.c  --  Secondary Fifo driver for I2S_v5
 *
 * (c) 2009 Samsung Electronics Co. Ltd
 *   - Jaswinder Singh Brar <jassi.brar@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <plat/regs-s3c2412-iis.h>

#include <mach/s3c-dma.h>

#include "s3c-dma.h"

static void __iomem *s5p_i2s0_regs;

static struct s3c2410_dma_client s5p_dma_client_outs = {
	.name		= "I2S_Sec PCM Stereo out"
};

static struct s3c_dma_params s5p_i2s_sec_pcm_out = {
	.channel	= DMACH_I2S0_OUT_S,
	.client		= &s5p_dma_client_outs,
	.dma_size	= 4,
};

static void s5p_snd_txctrl(int on)
{
	u32 iiscon;

	iiscon  = readl(s5p_i2s0_regs + S3C2412_IISCON);

	if (on) {
		iiscon |= S3C2412_IISCON_IIS_ACTIVE;
		iiscon  &= ~S3C2412_IISCON_TXCH_PAUSE;
		iiscon  &= ~S5P_IISCON_TXSDMAPAUSE;
		iiscon  |= S5P_IISCON_TXSDMACTIVE;
	} else {
		/* Stop TX only if Tx-P not active */
		if (!(iiscon & S3C2412_IISCON_TXDMA_ACTIVE)) {
			iiscon  |= S3C2412_IISCON_TXCH_PAUSE;
			/* Stop only if RX not active */
			if (!(iiscon & S3C2412_IISCON_RXDMA_ACTIVE))
				iiscon &= ~S3C2412_IISCON_IIS_ACTIVE;
		}
		iiscon  |= S5P_IISCON_TXSDMAPAUSE;
		iiscon  &= ~S5P_IISCON_TXSDMACTIVE;
	}

	writel(iiscon,  s5p_i2s0_regs + S3C2412_IISCON);
}

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s5p_snd_lrsync(void)
{
	u32 iiscon;
	unsigned long loops = msecs_to_loops(5);

	pr_debug("Entered %s\n", __func__);

	while (--loops) {
		iiscon = readl(s5p_i2s0_regs + S3C2412_IISCON);
		if (iiscon & S3C2412_IISCON_LRINDEX)
			break;

		cpu_relax();
	}

	if (!loops) {
		printk(KERN_ERR "%s: timeout\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int s5p_i2s_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dailink = rtd->dai;
	u32 iismod;

	dailink->cpu_dai->dma_data = &s5p_i2s_sec_pcm_out;

	iismod = readl(s5p_i2s0_regs + S3C2412_IISMOD);

	if (dailink->use_idma)
		iismod |= S5P_IISMOD_TXSLP;
	else
		iismod &= ~S5P_IISMOD_TXSLP;

	/* Copy the same bps as Primary */
	iismod &= ~S5P_IISMOD_BLCSMASK;
	iismod |= ((iismod & S5P_IISMOD_BLCPMASK) << 2);

	writel(iismod, s5p_i2s0_regs + S3C2412_IISMOD);

	return 0;
}

static int s5p_i2s_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	u32 iiscon, iisfic;

	iiscon = readl(s5p_i2s0_regs + S3C2412_IISCON);

	/* FIFOs must be flushed before enabling PSR and other MOD bits, so we do it here. */
	if (iiscon & S5P_IISCON_TXSDMACTIVE)
		return 0;

	iisfic = readl(s5p_i2s0_regs + S5P_IISFICS);
	iisfic |= S3C2412_IISFIC_TXFLUSH;
	writel(iisfic, s5p_i2s0_regs + S5P_IISFICS);

	do {
		cpu_relax();
	} while ((__raw_readl(s5p_i2s0_regs + S5P_IISFICS) >> 8) & 0x7f);

	iisfic = readl(s5p_i2s0_regs + S5P_IISFICS);
	iisfic &= ~S3C2412_IISFIC_TXFLUSH;
	writel(iisfic, s5p_i2s0_regs + S5P_IISFICS);

	return 0;
}

static int s5p_i2s_trigger(struct snd_pcm_substream *substream,
		int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = s5p_snd_lrsync();
		if (ret)
			goto exit_err;
		s5p_snd_txctrl(1);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		s5p_snd_txctrl(0);
		break;
	}

exit_err:
	return ret;
}

struct snd_soc_dai i2s_sec_fifo_dai = {
	.name = "i2s-sec-fifo",
	.id = 0,
	.ops = {
		.hw_params = s5p_i2s_hw_params,
		.startup   = s5p_i2s_startup,
		.trigger   = s5p_i2s_trigger,
	},
};
EXPORT_SYMBOL_GPL(i2s_sec_fifo_dai);

void s5p_i2s_sec_init(void *regs, dma_addr_t phys_base)
{
	s5p_i2s0_regs = regs;
	s5p_i2s_sec_pcm_out.dma_addr = phys_base + S5P_IISTXDS;
}

/* Module information */
MODULE_AUTHOR("Jaswinder Singh <jassi.brar@samsung.com>");
MODULE_DESCRIPTION("S5P I2S-SecFifo SoC Interface");
MODULE_LICENSE("GPL");
