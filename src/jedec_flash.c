/*
 * This file is part of the vultureprog project.
 *
 * Copyright (C) 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file jedec.c Utilities for JEDEC-compliant chips
 *
 * These utilities provide a way to identify and manipulate JEDEC-compliant
 * chips. JEDEC-compliant chips are not necessarily Common Flash Interface (CFI)
 * compliant.
 */
#include <qiprog.h>
#include <jedec_flash.h>
#include <stdbool.h>

/** @private */
enum jedec_cmd {
	JEDEC_CMD_BYTE_PROGRAM = 0xa0,
	JEDEC_CMD_ERASE = 0x80,
	JEDEC_CMD_ERASE_SECTOR = 0x30,
	JEDEC_CMD_ERASE_BLOCK = 0x50,
	JEDEC_CMD_ERASE_CHIP = 0x10,
	JEDEC_CMD_ENTER_ID_READ = 0x90,
	JEDEC_CMD_EXIT_ID_READ = 0xF0,
};

/* Check one byte for odd parity */
/** @private */
static bool is_odd_parity(uint8_t val)
{
	val = (val ^ (val >> 4)) & 0xf;
	val = (val ^ (val >> 2)) & 0x3;
	return (val ^ (val >> 1)) & 0x1;
}

/** @private */
static qiprog_err jedec_wait_ready(struct qiprog_device *dev, uint32_t addr)
{
	unsigned int i = 0;
	uint8_t tmp1, tmp2;

	qiprog_read8(dev, addr, &tmp1);
	tmp1 &= 0x40;

	while (i++ < 0xFFFFFFF) {
		qiprog_read8(dev, addr, &tmp2);
		tmp2 &= 0x40;
		if (tmp1 == tmp2) {
			break;
		}
		tmp1 = tmp2;
	}
	if (i > 0x100000) {
		return QIPROG_ERR_TIMEOUT;
	}
	return QIPROG_SUCCESS;
}

/** @private */
static qiprog_err jedec_send_cmd(struct qiprog_device *dev, uint32_t base,
				 uint32_t mask, enum jedec_cmd cmd)
{
	qiprog_err ret;
	const uint32_t offset1 = 0x5555;
	const uint32_t offset2 = 0x2aaa;

	/* Send the entire sequence at once, even if one transfer fails */
	ret = qiprog_write8(dev, base | (offset1 & mask), 0xaa);
	ret |= qiprog_write8(dev, base | (offset2 & mask), 0x55);
	ret |= qiprog_write8(dev, base | (offset1 & mask), cmd);
	return ret;
}

/**
 * @brief Probe a JEDEC-compliant chip
 *
 * Probe for a JEDEC-compliant flash chip. The chip is expected to respond to
 * commands written to phys_base on the chip's address bus. This may or may not
 * be where a flash chip would be mapped in an x86 address space, for instance.
 * In most cases, probing at 0xffff0000 (newer LPC/FWH chips), and 0x00000000
 * (older LPC/AA-Mux/Parallel chips) should be sufficient to identify the chip.
 *
 * Different chips will respond to different length commands. For example, some
 * need the command sent at base + 0x5555, while others expect it at base +
 * 0x555. To accommodate a variety of chips, the command is masked until the
 * chip responds with its ID. This mask is returned in cmd_mask, and needs to be
 * passed to other jedec_*() functions for the chip to be operated correctly.
 *
 * This function returns QIPROG_SUCCESS when something on the bus responds. It
 * does not necessarily indicate a JEDEC-compliant chip was found. When a chip
 * can not be detected id->id_method will be set to QIPROG_ID_INVALID.
 *
 * @param[in] dev Device to operate on
 * @param[out] id Location where to store device id, if found.
 * @param[in] phys_base Where, in the chip address space to probe
 * @param[out] cmd_mask The mask that was found to work when probing this chip
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err jedec_probe(struct qiprog_device *dev, struct qiprog_chip_id *id,
		       uint32_t phys_base, uint32_t *cmd_mask)
{
	int i;
	qiprog_err ret;
	uint8_t vid, pid;
	uint16_t id16;
	uint32_t mask, base;

	/*
	 * Start probing with a 16-bit mask, then move down to 8 bit in the hope
	 * that something will respond.
	 */
	vid = pid = 0;
	id->id_method = QIPROG_ID_INVALID;
	for (i = 16; i >= 8; i--) {
		/* Mask the first 'i' bits */
		mask = (1 << i) - 1;
		/* Mask off those bits from our base */
		base = phys_base & ~mask;
		ret = jedec_send_cmd(dev, base, mask, JEDEC_CMD_ENTER_ID_READ);
		if (ret != QIPROG_SUCCESS)
			continue;

		/* Try to extract the ID */
		ret = qiprog_read16(dev, base, &id16);
		if (ret != QIPROG_SUCCESS)
			continue;

		/* And see if it's valid */
		vid = id16 & 0xff;
		pid = (id16 >> 8) & 0xff;
		if (!is_odd_parity(vid) || !is_odd_parity(pid))
			continue;

		/* We found it! */
		id->id_method = QIPROG_ID_METH_JEDEC;
		break;
	}

	/*
	 * If we couldn't find a valid chip, id->id_method will be
	 * QIPROG_ID_INVALID to indicate this. Not having responded to our
	 * probes is fine.
	 */
	id->vendor_id = vid;
	id->device_id = pid;
	*cmd_mask = mask;

	/* We MUST reach this line to get the chip out of read ID mode */
	jedec_send_cmd(dev, base, mask, JEDEC_CMD_EXIT_ID_READ);

	/*
	 * Irrespective of having found a chip, We should always return
	 * gracefully. The host will figure out no chip was found by the
	 * QIPROG_ID_INVALID value of the id_method field.
	 */
	return QIPROG_SUCCESS;
}

/**
 * @brief Program (write) a byte to a  JEDEC-compliant chip
 *
 * @param[in] dev Device to operate on
 * @param[in] addr Address to program
 * @param[in] val Value to write
 * @param[in] mask The mask that was found to work when probing this chip
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err jedec_program_byte(struct qiprog_device *dev, uint32_t addr,
				     uint8_t val, uint32_t mask)
{
	qiprog_err ret;
	uint32_t base = addr & ~mask;

	ret = jedec_send_cmd(dev, base, mask, JEDEC_CMD_BYTE_PROGRAM);
	if (ret != QIPROG_SUCCESS)
		return ret;

	ret = qiprog_write8(dev, addr, val);
	return ret | jedec_wait_ready(dev, base);
}

/**
 * @brief Perform a chip-erase on a JEDEC-compliant chip
 *
 * @param[in] dev Device to operate on
 * @param[in] base Base address of the chip
 * @param[in] mask The mask that was found to work when probing this chip
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err jedec_chip_erase(struct qiprog_device *dev, uint32_t base,
			    uint32_t mask)
{
	qiprog_err ret;

	ret = jedec_send_cmd(dev, base, mask, JEDEC_CMD_ERASE);
	ret |= jedec_send_cmd(dev, base, mask, JEDEC_CMD_ERASE_CHIP);
	return jedec_wait_ready(dev, base);
}

/**
 * @brief Perform a sector-erase on a JEDEC-compliant chip
 *
 * @param[in] dev Device to operate on
 * @param[in] sector Base address of the sector
 * @param[in] mask The mask that was found to work when probing this chip
 *
 * @return QIPROG_SUCCESS on success, or a QIPROG_ERR code otherwise.
 */
qiprog_err jedec_sector_erase(struct qiprog_device *dev, uint32_t sector,
			      uint32_t mask)
{
	qiprog_err ret;
	uint32_t base = sector & ~mask;
	const uint32_t offset1 = 0x5555;
	const uint32_t offset2 = 0x2aaa;

	ret = jedec_send_cmd(dev, base, mask, JEDEC_CMD_ERASE);
	ret |= qiprog_write8(dev, base | (offset1 & mask), 0xaa);
	ret |= qiprog_write8(dev, base | (offset2 & mask), 0x55);
	ret |= qiprog_write8(dev, base | sector, JEDEC_CMD_ERASE_SECTOR);
	return jedec_wait_ready(dev, base);
}
