/* linux/arch/arm/plat-s5pc11x/include/plat/mfc.h
 *
 * Copyright 2010 Samsung Electronics
 *	Changhwan Youn <chaos.youn@samsung.com>
 *	http://samsungsemi.com/
 *
 * Samsung Mfc device descriptions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3C_MFC_H
#define _S3C_MFC_H

#include <linux/types.h>

struct s3c_mfc_platdata {
	dma_addr_t buf_phy_base[2];
	size_t buf_size[2];
};

extern void __init s3c_mfc_get_reserve_memory_info(void);
extern void __init s3c_mfc_set_platdata(struct s3c_mfc_platdata *pd);
#endif
