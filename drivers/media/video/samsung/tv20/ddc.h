/* linux/drivers/media/video/samsung/tv20/ddc.h
 *
 * ddc i2c client driver header file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/* 
 * i2c ddc port 
 */


extern int ddc_read(u8 subaddr, u8 *data, u16 len);
extern int ddc_write(u8 *data, u16 len);
