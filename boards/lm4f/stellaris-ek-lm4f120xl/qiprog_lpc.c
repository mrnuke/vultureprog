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

static struct qiprog_driver stellaris_lpc_drv;
static uint32_t chip_size = 0;

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
	uint32_t req_len;

	(void)dev;

	req_len = dev->addr.end - where;
	n = (req_len > n) ? n : req_len;

	led_on(LED_B);
	for (i = 0; i < n; i++)
		ret |= lpc_mread(where++, dest + i);
	led_off(LED_B);

	/* Update the read pointer */
	dev->addr.pread = where;

	return ret;
}

static qiprog_err write(struct qiprog_device *dev, uint32_t where, void *src,
			uint32_t n)
{
	int ret = 0;
	size_t i;
	uint32_t req_len;
	uint8_t *data = src;

	req_len = dev->addr.end - where;
	n = (req_len > n) ? n : req_len;

	/*
	 * A few things to note:
	 * We may want to optimize the loop to reduce the number of function
	 * calls and the bus idle time.
	 * We currently use a mask of 0xffff, but we may want to detect the
	 * correct mask and use that instead.
	 */
	led_on(LED_R);
	for (i = 0; i < n; i++)
		ret |= jedec_program_byte(dev, where++, data[i], 0xffff);
	led_off(LED_R);

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
