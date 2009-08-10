/*
 * TL2796 LCD Panel Driver for the Samsung Universal board
 *
 * Author: InKi Dae  <inki.dae@samsung.com>
 *
 * Derived from drivers/video/omap/lcd-apollon.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <plat/gpio-cfg.h>

#include "s3cfb.h"

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

const unsigned short SEQ_SETTING[] = {
	0x01, 0x00,
	0x21, 0x33,
	0x22, 0x08,
	0x23, 0x00,
	0x24, 0x33,
	0x25, 0x33,
	0x26, 0x06,
	0x27, 0x42,
	0x27, 0x42,
	0x2f, 0x02,
	0x05, 0x01,
	SLEEPMSEC, 400,
	0x04, 0x01,
	SLEEPMSEC, 20,

	0x06, 0x44,
	0x07, 0x04,
	0x08, 0x01,
	0x09, 0x04,
	0x0a, 0x11,
	0x0c, 0x00,
	0x0d, 0x14,
	0x0e, 0x00,
	0x0f, 0x1e,
	0x10, 0x00,
	0x1c, 0x08,
	0x1d, 0x05,
	0x1f, 0x00,

	0x30, 0x33,
	0x31, 0x39,
	0x32, 0x37,
	0x33, 0x59,
	0x34, 0x74,
	0x35, 0x74,
	0x36, 0x1c,
	0x37, 0x19,
	0x38, 0x1a,
	0x39, 0x23,
	0x3a, 0x1f,
	0x3b, 0x20,
	0x3c, 0x29,
	0x3d, 0x1b,
	0x3e, 0x20,
	0x3f, 0x39,
	0x40, 0x21,
	0x41, 0x29,
	
	0x04, 0x05,
	SLEEPMSEC, 40,

	0x04, 0x07,
};

/* FIXME: will be moved to platform data */
static struct s3cfb_lcd ams320 = {
	.width = 320,
	.height = 480,
	.bpp = 24,
	.freq = 60,
	.timing = {
		.h_fp = 64,
		.h_bp = 64,
		.h_sw = 2,
		.v_fp = 6,
		.v_fpe = 1,
		.v_bp = 6,
		.v_bpe = 1,
		.v_sw = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};

void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	ams320.init_ldi = NULL;
	ctrl->lcd = &ams320;
}

static int __init tl2796_init(void)
{
	return 0;
}

static void __exit tl2796_exit(void)
{

}

module_init(tl2796_init);
module_exit(tl2796_exit);
