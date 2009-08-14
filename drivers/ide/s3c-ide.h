/* linux/drivers/ide/s3c-ide.h
 *
 * Copyright (C) 2009 Samsung Electronics
 *      http://samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

typedef enum {
	ATA_CMD_STOP, ATA_CMD_START, ATA_CMD_ABORT, ATA_CMD_CONTINUE
} ATA_TRANSFER_CMD;

typedef enum {
	PIO_CPU, PIO_DMA, MULTIWORD_DMA, UDMA
} ATA_MODE;

typedef enum {
	IDLE, BUSYW, PREP, BUSYR, PAUSER, PAUSEW, PAUSER2
} BUS_STATE;

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
#define NUM_DESCRIPTORS         PRD_ENTRIES
#else
#define NUM_DESCRIPTORS         2
#endif

typedef struct {
	ulong addr;		/* Used to block on state transitions */
	ulong len;		/* Power Managers device structure */
} dma_queue_t;

typedef struct {
	ide_hwif_t *hwif;
#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	ide_drive_t *drive;
	uint index;		/* current queue index */
	uint queue_size;	/* total queue size requested */
	dma_queue_t table[NUM_DESCRIPTORS];
	uint pseudo_dma;	/* in DMA session */
#endif
	struct platform_device *dev;
	int irq;
	ulong piotime[5];
	ulong udmatime[5];
} s3c_ide_hwif_t;
