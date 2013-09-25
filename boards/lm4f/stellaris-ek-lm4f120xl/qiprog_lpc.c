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

#include "led.h"
#include "lpc_io.h"

#include <blackbox.h>
#include <qiprog_usb_dev.h>
#include <jedec_flash.h>
#include <stdbool.h>

static struct qiprog_driver stellaris_lpc_drv;
static uint32_t chip_size = 0;
static uint32_t block_size = 0;
static uint32_t sector_size = 0;
static bool auto_erase = false;

static uint32_t saved_chip_size = 0;

static void push_chip_size(void) {
	saved_chip_size = chip_size;
	chip_size = 0;
}

static void pop_chip_size(void) {
	chip_size = saved_chip_size;
	saved_chip_size = 0;
}

/**
 * @brief QiProg driver 'dev_open' member
 */
static qiprog_err lpc_open(struct qiprog_device *dev)
{
	(void)dev;

	/* Configure pins for LPC master mode */
	lpc_init();

	return QIPROG_SUCCESS;
}

/**
 * @brief QiProg driver 'get_capabilities' member
 */
static qiprog_err get_capabilities(struct qiprog_device *dev,
				   struct qiprog_capabilities *caps)
{
	(void)dev;

	caps->instruction_set = 0;
	caps->bus_master = QIPROG_BUS_LPC;
	caps->max_direct_data = 0;
	caps->voltages[0] = 3300;
	caps->voltages[1] = 0;
	return QIPROG_SUCCESS;
}

static qiprog_err set_bus(struct qiprog_device *dev, enum qiprog_bus bus)
{
	(void)dev;
	/*
	 * Don't worry about switching buses for now. If LPC is specified, and
	 * only LPC, then we are happy.
	 */
	if ((bus & QIPROG_BUS_LPC) && ((bus & ~QIPROG_BUS_LPC) == 0))
		return QIPROG_SUCCESS;
	else
		return QIPROG_ERR_ARG;
}

static qiprog_err read_chip_id(struct qiprog_device *dev,
			       struct qiprog_chip_id ids[9])
{
	qiprog_err ret = 0;
	uint8_t mfg_id, dev_id;
	uint32_t mask, old_size;

	(void)dev;

	led_on(LED_B);

	/*
	 * jedec_probe() uses physical addresses, so by setting chip_size to
	 * zero, we ensure we read and write to the physical address.
	 */
	old_size = chip_size;
	chip_size = 0;

	/*
	 * Run the JEDEC probing sequence. Most, if not all LPC chips support
	 * it.
	 * TODO: Do not worry about saving the command mask for now. We do not
	 * yet know if we want to use the mask we got from probing, or a mask
	 * supplied by the host.
	 */
	ret = jedec_probe(dev, ids, 0xffff0000, &mask);
	chip_size = old_size;

	/* We only allow connecting one chip. */
	ids[1].id_method = QIPROG_ID_INVALID;

	led_off(LED_B);

	/* Tell the host if we encountered an error or not in reading the ID */
	return ret;
}

static qiprog_err set_chip_size(struct qiprog_device *dev, uint8_t chip_idx,
				uint32_t size)
{
	(void) dev;

	/* Only one chip suported */
	if (chip_idx != 0)
		return QIPROG_ERR_ARG;

	chip_size = size;
	return QIPROG_SUCCESS;
}

static qiprog_err set_erase_size(struct qiprog_device *dev, uint8_t chip_idx,
				 enum qiprog_erase_type *types, uint32_t *sizes,
				 size_t num_sizes)
{
	size_t i;

	(void)dev;

	/* We only support one connected chip */
	if (chip_idx > 0)
		return QIPROG_ERR_ARG;

	/* Reset any previous values */
	block_size = sector_size = 0;
	/* Find the block and/or sector sizes */
	for (i = 0; i < num_sizes; i++) {
		if (types[i] == QIPROG_ERASE_TYPE_SECTOR) {
			sector_size = sizes[i];
			print_spew("Sector size set to %u\n", sector_size);
			continue;
		}
		if (types[i] == QIPROG_ERASE_TYPE_BLOCK) {
			block_size = sizes[i];
			print_spew("Block size set to %u\n", block_size);
			continue;
		}
	}

	if (!sector_size && !block_size) {
		print_err("No sector or block size specified\n");
		return QIPROG_ERR_ARG;
	}

	return QIPROG_SUCCESS;
}

static qiprog_err set_erase_command(struct qiprog_device *dev, uint8_t chip_idx,
				    enum qiprog_erase_cmd cmd,
				    enum qiprog_erase_subcmd subcmd,
				    uint16_t flags)
{
	(void)dev;

	/* We only support one connected chip */
	if (chip_idx > 0)
		return QIPROG_ERR_ARG;

	/* We only support JEDEC_ISA sequences at the moment */
	if ((cmd != QIPROG_ERASE_CMD_JEDEC_ISA) ||
	    (subcmd != QIPROG_ERASE_SUBCMD_DEFAULT)) {
		print_err("Unsupported erase command %u:%u\n", cmd, subcmd);
		return QIPROG_ERR_ARG;
	}

	auto_erase = (flags & QIPROG_ERASE_BEFORE_WRITE) ? true : false;

	print_spew("Using JEDEC ISA erase sequence %s\n", (auto_erase) ?
		   "with auto erase":"");
	return QIPROG_SUCCESS;
}

static qiprog_err set_custom_erase_command(struct qiprog_device *dev,
					   uint8_t chip_idx, uint32_t *addr,
					   uint8_t *data, size_t num_bytes)
{
	(void)dev;

	/* FIXME: This is a stub */
	(void)addr;
	(void)data;
	(void)num_bytes;

	/* We only support one connected chip */
	if (chip_idx > 0)
		return QIPROG_ERR_ARG;

	return QIPROG_ERR;
}

static qiprog_err set_write_command(struct qiprog_device *dev, uint8_t chip_idx,
				    enum qiprog_write_cmd cmd,
				    enum qiprog_write_subcmd subcmd)
{
	(void)dev;

	/* We only support one connected chip */
	if (chip_idx > 0)
		return QIPROG_ERR_ARG;

	/* We only support JEDEC_ISA sequences at the moment */
	if ((cmd != QIPROG_WRITE_CMD_JEDEC_ISA) ||
	    (subcmd != QIPROG_WRITE_SUBCMD_DEFAULT)) {
		print_err("Unsupported write command %u:%u\n", cmd, subcmd);
		return QIPROG_ERR_ARG;
	}

	print_spew("Using JEDEC ISA byte-program sequence\n");

	return QIPROG_SUCCESS;
}

static qiprog_err set_custom_write_command(struct qiprog_device *dev,
					   uint8_t chip_idx, uint32_t *addr,
					   uint8_t *data, size_t num_bytes)
{
	(void)dev;

	/* FIXME: This is a stub */
	(void)addr;
	(void)data;
	(void)num_bytes;

	/* We only support one connected chip */
	if (chip_idx > 0)
		return QIPROG_ERR_ARG;

	return QIPROG_ERR;
}

static qiprog_err read8(struct qiprog_device *dev, uint32_t addr,
			uint8_t * data)
{
	uint32_t base;
	qiprog_err ret = 0;

	(void)dev;

	base = 0xffffffff - chip_size + 1 + addr;

	led_on(LED_B);
	ret = lpc_mread(base, data);
	led_off(LED_B);

	return ret;
}

static qiprog_err read16(struct qiprog_device *dev, uint32_t addr,
			 uint16_t * data)
{
	uint32_t base;
	uint8_t *raw = (void *)data;
	qiprog_err ret = 0;

	(void)dev;

	base = 0xffffffff - chip_size + 1 + addr;

	led_on(LED_B);
	/* Read in little-endian order. FIXME: is this the final answer? */
	ret |= lpc_mread(base + 0, raw);
	ret |= lpc_mread(base + 1, raw + 1);
	led_off(LED_B);

	return ret;
}

static qiprog_err read32(struct qiprog_device *dev, uint32_t addr,
			 uint32_t * data)
{
	uint32_t base;
	uint8_t *raw = (void *)data;
	qiprog_err ret = 0;

	(void)dev;

	base = 0xffffffff - chip_size + 1 + addr;

	led_on(LED_B);
	/* Read in little-endian order. FIXME: is this the final answer? */
	ret |= lpc_mread(base + 0, raw + 0);
	ret |= lpc_mread(base + 1, raw + 1);
	ret |= lpc_mread(base + 2, raw + 2);
	ret |= lpc_mread(base + 3, raw + 3);
	led_off(LED_B);

	return QIPROG_SUCCESS;
}

static qiprog_err write8(struct qiprog_device *dev, uint32_t addr, uint8_t data)
{
	uint32_t base;
	qiprog_err ret = 0;

	(void)dev;

	base = 0xffffffff - chip_size + 1 + addr;

	led_on(LED_R);
	ret = lpc_mwrite(base, data);
	led_off(LED_R);

	return ret;
}

static qiprog_err write16(struct qiprog_device *dev, uint32_t addr,
			  uint16_t data)
{
	uint32_t base;
	qiprog_err ret = 0;

	(void)dev;

	base = 0xffffffff - chip_size + 1 + addr;

	led_on(LED_R);
	/* Write in little-endian order. FIXME: is this the final answer? */
	ret |= lpc_mwrite(base + 0, (data >> 0) & 0xff);
	ret |= lpc_mwrite(base + 1, (data >> 8) & 0xff);
	led_off(LED_R);

	return ret;
}

static qiprog_err write32(struct qiprog_device *dev, uint32_t addr,
			  uint32_t data)
{
	uint32_t base;
	qiprog_err ret = 0;

	(void)dev;

	base = 0xffffffff - chip_size + 1 + addr;

	led_on(LED_R);
	/* Write in little-endian order. FIXME: is this the final answer? */
	ret |= lpc_mwrite(base + 0, (data >> 0) & 0xff);
	ret |= lpc_mwrite(base + 1, (data >> 8) & 0xff);
	ret |= lpc_mwrite(base + 2, (data >> 16) & 0xff);
	ret |= lpc_mwrite(base + 3, (data >> 24) & 0xff);
	led_off(LED_R);

	return ret;
}

static qiprog_err set_address(struct qiprog_device *dev, uint32_t start,
			      uint32_t end)
{
	print_spew("Setting address range 0x%.8lx -> 0x%.8lx\n", start, end);
	dev->addr.end = end;
	/* Read and write pointers are reset when setting a new range */
	dev->addr.pread = dev->addr.pwrite = dev->addr.start = start;
	return QIPROG_SUCCESS;
}

static qiprog_err read(struct qiprog_device *dev, uint32_t where, void *dest,
		       uint32_t n)
{
	int ret = 0;
	size_t i;
	uint32_t req_len, base;

	/* Halt on overflow */
	if (chip_size < (where + n))
		return QIPROG_ERR;

	base = 0xffffffff - chip_size + 1 + where;

	(void)dev;

	req_len = dev->addr.end - where;
	n = (req_len > n) ? n : req_len;

	led_on(LED_B);
	for (i = 0; i < n; i++)
		ret |= lpc_mread(base++, dest + i);
	led_off(LED_B);

	/* Update the read pointer */
	dev->addr.pread += n;

	return ret;
}

static void erase(struct qiprog_device *dev, uint32_t start, uint32_t end)
{
	uint32_t erase_size, i, erase_base, i_start, i_end;

	if (!block_size && !sector_size) {
		print_err("Erase size not specified. Skipping auto erase.\n");
		auto_erase = false;
		return;
	}

	/* Prefer block erasers over sector erasers */
	erase_size = block_size ? block_size : sector_size;

	/* Convert address range to block/sector numbers */
	i_start = start / erase_size;
	i_end = (end - 1) / erase_size;

	/* See if the start address falls within our range */
	for (i = i_start; i <= i_end; i++) {
		erase_base = i * erase_size;
		if (erase_base >= start) {
			if (block_size) {
				print_err("Block eraser not implemented\n");
				return;
				/* jedec_block_erase(dev, erase_base, 0xffff);*/
				print_spew("Erasing block @ %u\n", erase_base);
			} else {
				jedec_sector_erase(dev, erase_base, 0xffff);
				print_spew("Erasing sector @ %u\n", erase_base);
			}
		}
	}
}

static qiprog_err write(struct qiprog_device *dev, uint32_t where, void *src,
			uint32_t n)
{
	int ret = 0;
	size_t i;
	uint32_t req_len, base;
	uint8_t *data = src;

	/* Halt on overflow */
	if (chip_size < (where + n))
		return QIPROG_ERR;

	base = 0xffffffff - chip_size + 1 + where;

	req_len = dev->addr.end - where;
	n = (req_len > n) ? n : req_len;

	/* JEDEC commands work with absolute addresses */
	push_chip_size();

	/* Erase if needed */
	if (auto_erase)
		erase(dev, base, base + n);

	/*
	 * A few things to note:
	 * We may want to optimize the loop to reduce the number of function
	 * calls and the bus idle time.
	 * We currently use a mask of 0xffff, but we may want to detect the
	 * correct mask and use that instead.
	 */
	led_on(LED_R);
	for (i = 0; i < n; i++)
		ret |= jedec_program_byte(dev, base++, data[i], 0xffff);
	led_off(LED_R);

	pop_chip_size();
	/* Update the write pointer */
	dev->addr.pwrite += n;

	return ret;
}

static struct qiprog_driver stellaris_lpc_drv = {
	.scan = NULL,		/* scan is not used */
	.dev_open = lpc_open,
	.get_capabilities = get_capabilities,
	.set_bus = set_bus,
	.read_chip_id = read_chip_id,
	.set_address = set_address,
	.set_chip_size = set_chip_size,
	.set_erase_size = set_erase_size,
	.set_erase_command = set_erase_command,
	.set_custom_erase_command = set_custom_erase_command,
	.set_write_command = set_write_command,
	.set_custom_write_command = set_custom_write_command,
	.read = read,
	.read8 = read8,
	.read16 = read16,
	.read32 = read32,
	.write = write,
	.write8 = write8,
	.write16 = write16,
	.write32 = write32,
};

struct qiprog_device stellaris_lpc_dev = {
	.drv = &stellaris_lpc_drv,
};
