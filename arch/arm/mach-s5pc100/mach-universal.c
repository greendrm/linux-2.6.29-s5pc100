/*
 * linux/arch/arm/mach-s5pc100/mach-universal.c
 *
 *
 * Copyright (C) 2009 by Samsung Electronics
 * All rights reserved.
 *
 * @Author: InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * @History:
 * derived from linux/arch/arm/mach-s3c2410/mach-bast.c, written by
 * Ben Dooks <ben@simtec.co.uk>
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/gpio.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-rtc.h>
#include <plat/iic.h>
#include <plat/fimc.h>
#include <plat/fb.h>
#include <plat/csis.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s5pc100.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-k0.h>
#include <plat/gpio-bank-a1.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-bank-d.h>
#include <plat/regs-clock.h>
#include <plat/spi.h>

extern struct sys_timer s5pc1xx_timer;
extern void s5pc1xx_reserve_bootmem(void);

static struct s3c24xx_uart_clksrc universal_serial_clocks[] = {
#if defined(CONFIG_SERIAL_S5PC1XX_HSUART)
/* HS-UART Clock using SCLK */
        [0] = {
                .name           = "uclk1",
                .divisor        = 1,
                .min_baud       = 0,
                .max_baud       = 0,
        },
#else
        [0] = {
                .name           = "pclk",
                .divisor        = 1,
                .min_baud       = 0,
                .max_baud       = 0,
        },
#endif
};

static struct s3c2410_uartcfg universal_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
	},
        [2] = {
                .hwport      = 2,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
        },
        [3] = {
                .hwport      = 3,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
        },
};

#define LCD_BUS_NUM 	3
#define DISPLAY_CS	S5PC1XX_GPK3(5)
static struct spi_board_info spi_board_info[] __initdata = {
    	{
	    	.modalias	= "tl2796",
		.platform_data	= NULL,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

#if defined(CONFIG_FB_S3C_TL2796)
#define DISPLAY_CLK	S5PC1XX_GPK3(6)
#define DISPLAY_SI	S5PC1XX_GPK3(7)
static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,

	.num_chipselect	= 1,
};

static struct platform_device universal_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};

static void tl2796_gpio_setup(void)
{
	gpio_request(S5PC1XX_GPH1(7), "MLCD_RST");
	gpio_request(S5PC1XX_GPJ1(3), "MLCD_ON");
}

/* Configure GPIO pins for RGB Interface */
static void rgb_cfg_gpio(struct platform_device *dev)
{
	int i;

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF0(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF1(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF2(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 4; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF3(i), S3C_GPIO_SFN(2));
}

static int tl2796_power_on(struct platform_device *dev)
{
	/* set gpio data for MLCD_RST to HIGH */
	gpio_direction_output(S5PC1XX_GPH1(7), 1);
	/* set gpio data for MLCD_ON to HIGH */
	gpio_direction_output(S5PC1XX_GPJ1(3), 1);
	mdelay(25);

	/* set gpio data for MLCD_RST to LOW */
	gpio_direction_output(S5PC1XX_GPH1(7), 0);
	udelay(20);
	/* set gpio data for MLCD_RST to HIGH */
	gpio_direction_output(S5PC1XX_GPH1(7), 1);
	mdelay(20);

	return 0;
}

static int tl2796_reset(struct platform_device *dev)
{
	/* set gpio pin for MLCD_RST to LOW */
	gpio_direction_output(S5PC1XX_GPH1(7), 0);
	udelay(1);	/* Shorter than 5 usec */
	/* set gpio pin for MLCD_RST to HIGH */
	gpio_direction_output(S5PC1XX_GPH1(7), 1);
	mdelay(10);

	return 0;
}

static struct s3c_platform_fb fb_data __initdata = {
	.hw_ver = 0x50,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,

	.cfg_gpio =  rgb_cfg_gpio,
	.backlight_on = tl2796_power_on,
	.reset_lcd = tl2796_reset,
};
#endif

struct map_desc universal_iodesc[] = {};

static struct platform_device *universal_devices[] __initdata = {
#if defined(CONFIG_FB_S3C)
	&s3c_device_fb,
#endif
#if defined(CONFIG_FB_S3C_TL2796)
	&universal_spi_gpio,
#endif
};

static void __init universal_map_io(void)
{
	s5pc1xx_init_io(universal_iodesc, ARRAY_SIZE(universal_iodesc));
	s3c24xx_init_clocks(0);
#if defined(CONFIG_SERIAL_S5PC1XX_HSUART)
        writel((readl(S5P_CLK_DIV2) & ~(0xf << 0)), S5P_CLK_DIV2);
#endif
	s3c24xx_init_uarts(universal_uartcfgs, ARRAY_SIZE(universal_uartcfgs));
	s5pc1xx_reserve_bootmem();
}

static void __init universal_machine_init(void)
{
#if defined(CONFIG_FB_S3C_TL2796)
	tl2796_gpio_setup();
#endif
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
#if defined(CONFIG_FB_S3C)
	s3cfb_set_platdata(&fb_data);
#endif
	platform_add_devices(universal_devices, ARRAY_SIZE(universal_devices));
}

MACHINE_START(SMDKC100, "smdkc100")
	/* Maintainer: InKi Dae <inki.dae@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5PC1XX_PA_SDRAM + 0x100,

	.init_irq	= s5pc100_init_irq,
	.map_io		= universal_map_io,
	.init_machine	= universal_machine_init,
	.timer		= &s5pc1xx_timer,
MACHINE_END
