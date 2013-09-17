#ifndef __ASM_ARCH_PXA3XX_NAND_H
#define __ASM_ARCH_PXA3XX_NAND_H

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

struct pxa3xx_nand_timing {
	unsigned int	tCH;  /* Enable signal hold time */
	unsigned int	tCS;  /* Enable signal setup time */
	unsigned int	tWH;  /* ND_nWE high duration */
	unsigned int	tWP;  /* ND_nWE pulse time */
	unsigned int	tRH;  /* ND_nRE high duration */
	unsigned int	tRP;  /* ND_nRE pulse width */
	unsigned int	tR;   /* ND_nWE high to ND_nRE low for read */
	unsigned int	tWHR; /* ND_nWE high to ND_nRE low for status read */
	unsigned int	tAR;  /* ND_ALE low to ND_nRE low delay */
};

struct pxa3xx_nand_cmdset {
	uint16_t	read1;
	uint16_t	read2;
	uint16_t	program;
	uint16_t	read_status;
	uint16_t	read_id;
	uint16_t	erase;
	uint16_t	reset;
	uint16_t	lock;
	uint16_t	unlock;
	uint16_t	lock_status;
	uint32_t	dual_plane_read;
	uint32_t	dual_plane_write;
	uint32_t	dual_plane_erase;
	uint32_t	dual_plane_randout;
};

struct pxa3xx_nand_flash {
	char		*name;
	uint16_t	chip_id;	/* chip id */
	uint16_t	ext_id;		/* Extend id */
	unsigned int	page_per_block; /* Pages per block (PG_PER_BLK) */
	unsigned int	page_size;	/* Page size in bytes (PAGE_SZ) */
	unsigned int	flash_width;	/* Width of Flash memory (DWIDTH_M) */
	unsigned int	dfc_width;	/* Width of flash controller(DWIDTH_C) */
	uint8_t         ecc_strength;   /* How strong ecc should be applied */
	unsigned int	num_blocks;	/* Number of physical blocks in Flash */

	struct pxa3xx_nand_timing *timing;	/* NAND Flash timing */

	uint8_t		has_dual_plane; /* Number of planes in Flash*/
};

/* The max num of chip select current support */
#define NUM_CHIP_SELECT		(2)
/*
 * The data flash bus is shared between the Static Memory
 * Controller and the Data Flash Controller,  the arbiter
 * controls the ownership of the bus
 */
#define PXA3XX_ARBI_EN		(1)
/* Newer controller support sending command in sperated stage */
#define PXA3XX_NAKED_CMD_EN	(1 << 1)
/* Tell driver whether enable DMA */
#define PXA3XX_DMA_EN		(1 << 2)
/* Tell driver whether keep configuration set in the boot loader */
#define PXA3XX_KEEP_CONFIG	(1 << 3)

#define PXA3XX_PDATA_MALLOC	(1 << 4)

struct pxa3xx_nand_platform_data {
	unsigned int				controller_attrs;

	/* indicate how many chip select would be used for this platform */
	int	cs_num;
	const struct mtd_partition		*parts[NUM_CHIP_SELECT];
	unsigned int				nr_parts[NUM_CHIP_SELECT];

	const struct pxa3xx_nand_flash * 	flash;
	size_t					num_flash;
};

extern void pxa3xx_set_nand_info(struct pxa3xx_nand_platform_data *info);
#endif /* __ASM_ARCH_PXA3XX_NAND_H */
