/* sound/soc/s3c24xx/s5p-i2s.c
 *
 * ALSA SoC Audio Layer - S5P I2S driver
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <plat/regs-s3c2412-iis.h>
#include <plat/audio.h>

#include <mach/map.h>
#include <mach/dma.h>

#include "s3c-dma.h"
#include "s5p-i2s.h"

static struct s3c2410_dma_client s5p_dma_client_out = {
	.name		= "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s5p_dma_client_in = {
	.name		= "I2S PCM Stereo in"
};

static struct s3c_dma_params s5p_i2s_pcm_stereo_out[] = {
	[0] = {
		.channel	= DMACH_I2S0_OUT,
		.client		= &s5p_dma_client_out,
		.dma_size	= 4,
	},
	[1] = {
		.channel	= DMACH_I2S1_OUT,
		.client		= &s5p_dma_client_out,
		.dma_size	= 4,
	},
	[2] = {
		.channel	= DMACH_I2S2_OUT,
		.client		= &s5p_dma_client_out,
		.dma_size	= 4,
	},
};

static struct s3c_dma_params s5p_i2s_pcm_stereo_in[] = {
	[0] = {
		.channel	= DMACH_I2S0_IN,
		.client		= &s5p_dma_client_in,
		.dma_size	= 4,
	},
	[1] = {
		.channel	= DMACH_I2S1_IN,
		.client		= &s5p_dma_client_in,
		.dma_size	= 4,
	},
	[2] = {
		.channel	= DMACH_I2S2_IN,
		.client		= &s5p_dma_client_in,
		.dma_size	= 4,
	},
};

static struct s3c_i2sv2_info s5p_i2s[3];

static inline struct s3c_i2sv2_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

static int s5p_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 iismod = readl(i2s->regs + S3C2412_IISMOD);

	switch (clk_id) {
	case S5P_CLKSRC_PCLK:
		iismod &= ~S3C64XX_IISMOD_IMS_SYSMUX;
		break;

	case S5P_CLKSRC_MUX:
		iismod |= S3C64XX_IISMOD_IMS_SYSMUX;
		break;

	case S5P_CLKSRC_CDCLK:
		switch (dir) {
		case SND_SOC_CLOCK_IN:
			iismod |= S3C64XX_IISMOD_CDCLKCON;
			break;
		case SND_SOC_CLOCK_OUT:
			iismod &= ~S3C64XX_IISMOD_CDCLKCON;
			break;
		default:
			return -EINVAL;
		}
		break;

	default:
		return -EINVAL;
	}

	writel(iismod, i2s->regs + S3C2412_IISMOD);

	return 0;
}

struct clk *s5p_i2s_get_clock(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	u32 iismod = readl(i2s->regs + S3C2412_IISMOD);

	if (iismod & S3C64XX_IISMOD_IMS_SYSMUX)
		return i2s->iis_cclk;
	else
		return i2s->iis_pclk;
}
EXPORT_SYMBOL_GPL(s5p_i2s_get_clock);

#define S5P_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_KNOT)

#define S5P_I2S_FMTS \
	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE |\
	 SNDRV_PCM_FMTBIT_S24_LE)

struct snd_soc_dai s5p_i2s_dai[] = {
	{
		.name		= "s5p-i2s-v5",
		.id		= 0,
		.playback = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= S5P_I2S_RATES,
			.formats	= S5P_I2S_FMTS,
		},
		.capture = {
			 .channels_min	= 2,
			 .channels_max	= 2,
			 .rates		= S5P_I2S_RATES,
			 .formats	= S5P_I2S_FMTS,
		 },
		.ops = {
			.set_sysclk	= s5p_i2s_set_sysclk,	
		},
	},
	{
		.name		= "s5p-i2s",
		.id		= 1,
		.playback = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= S5P_I2S_RATES,
			.formats	= S5P_I2S_FMTS,
		},
		.capture = {
			 .channels_min	= 2,
			 .channels_max	= 2,
			 .rates		= S5P_I2S_RATES,
			 .formats	= S5P_I2S_FMTS,
		},
		.ops = {
			.set_sysclk	= s5p_i2s_set_sysclk,	
		},
	},
	{
		.name		= "s5p-i2s",
		.id		= 2,
		.playback = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= S5P_I2S_RATES,
			.formats	= S5P_I2S_FMTS,
		},
		.capture = {
			 .channels_min	= 2,
			 .channels_max	= 2,
			 .rates		= S5P_I2S_RATES,
			 .formats	= S5P_I2S_FMTS,
		},
		.ops = {
			.set_sysclk	= s5p_i2s_set_sysclk,	
		},
	},
};
EXPORT_SYMBOL_GPL(s5p_i2s_dai);

static __devinit int s5p_iis_dev_probe(struct platform_device *pdev)
{
	struct s3c_i2sv2_info *i2s;
	struct snd_soc_dai *dai;
	struct s3c_audio_pdata *i2s_pdata;
	int ret;

	if (pdev->id >= ARRAY_SIZE(s5p_i2s)) {
		dev_err(&pdev->dev, "id %d out of range\n", pdev->id);
		return -EINVAL;
	}

	i2s = &s5p_i2s[pdev->id];
	dai = &s5p_i2s_dai[pdev->id];
	dai->dev = &pdev->dev;

	i2s->dma_capture = &s5p_i2s_pcm_stereo_in[pdev->id];
	i2s->dma_playback = &s5p_i2s_pcm_stereo_out[pdev->id];

	i2s_pdata = pdev->dev.platform_data;
	if (i2s_pdata && i2s_pdata->cfg_gpio && i2s_pdata->cfg_gpio(pdev)) {
		dev_err(&pdev->dev, "Unable to configure gpio\n");
		return -EINVAL;
	}

	i2s->iis_cclk = clk_get(&pdev->dev, "audio-bus");
	if (IS_ERR(i2s->iis_cclk)) {
		dev_err(&pdev->dev, "failed to get audio-bus\n");
		ret = PTR_ERR(i2s->iis_cclk);
		goto err;
	}
	clk_enable(i2s->iis_cclk);

	ret = s3c_i2sv2_probe(pdev, dai, i2s, 0);
	if (ret)
		goto err_clk;

	ret = s3c_i2sv2_register_dai(dai);
	if (ret != 0)
		goto err_i2sv2;

	return 0;

err_i2sv2:
	/* Not implemented for I2Sv2 core yet */
err_clk:
	clk_put(i2s->iis_cclk);
err:
	return ret;
}

static __devexit int s5p_iis_dev_remove(struct platform_device *pdev)
{
	dev_err(&pdev->dev, "Device removal not yet supported\n");
	return 0;
}

static struct platform_driver s5p_iis_driver = {
	.probe  = s5p_iis_dev_probe,
	.remove = s5p_iis_dev_remove,
	.driver = {
		.name = "s5p-iis",
		.owner = THIS_MODULE,
	},
};

static int __init s5p_i2s_init(void)
{
	return platform_driver_register(&s5p_iis_driver);
}
module_init(s5p_i2s_init);

static void __exit s5p_i2s_exit(void)
{
	platform_driver_unregister(&s5p_iis_driver);
}
module_exit(s5p_i2s_exit);

/* Module information */
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("S5P I2S SoC Interface");
MODULE_LICENSE("GPL");
