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

#include "lpc_io.h"

#include <qiprog_usb_dev.h>

static struct qiprog_driver stellaris_lpc_drv;

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
	(void) dev;

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

	(void)dev;

	/*
	 * 0xffbc0000 seems to give us the device IDs, at least on the
	 * SST 49LF080A when the chip ID[0:3] pins are all strapped to 0.
	 * Whether this address is valid for any LPC ROM, or is an SST specific
	 * extension remains to be seen.
	 * The JEDEC sequence in flashrom and other software (milksop) does not
	 * use this address.
	 */
	ret |= lpc_mread(0xffbc0000, &mfg_id);
	ret |= lpc_mread(0xffbc0001, &dev_id);

	/* Send the ID back to the host */
	ids[0].id_method = 1;	/* FIXME: Use an enum */
	ids[0].vendor_id = mfg_id;
	ids[0].device_id = dev_id;

	/* We only allow connecting one chip. */
	ids[1].id_method = 0;

	/* Tell the host if we encountered an error or not in reading the ID */
	return ret;
}

static qiprog_err read8(struct qiprog_device *dev, uint32_t addr,
			uint8_t * data)
{
	(void)dev;
	(void)addr;

	/* Pretend we are reading data */
	*data = 0xa5;

	return QIPROG_SUCCESS;
}

static qiprog_err read16(struct qiprog_device *dev, uint32_t addr,
			 uint16_t * data)
{
	(void)dev;
	(void)addr;

	/* Pretend we are reading data */
	*data = 0xa55a;

	return QIPROG_SUCCESS;
}

static qiprog_err read32(struct qiprog_device *dev, uint32_t addr,
			 uint32_t * data)
{
	(void)dev;
	(void)addr;

	/* Pretend we are reading data */
	*data = 0xf05aa50f;

	return QIPROG_SUCCESS;
}

static qiprog_err write8(struct qiprog_device *dev, uint32_t addr, uint8_t data)
{
	(void)dev;
	(void)addr;
	(void)data;

	/* Pretend write was successful */
	return QIPROG_SUCCESS;
}

static qiprog_err write16(struct qiprog_device *dev, uint32_t addr,
			  uint16_t data)
{
	(void)dev;
	(void)addr;
	(void)data;

	/* Pretend write was successful */
	return QIPROG_SUCCESS;
}

static qiprog_err write32(struct qiprog_device *dev, uint32_t addr,
			  uint32_t data)
{
	(void)dev;
	(void)addr;
	(void)data;

	/* Pretend write was successful */
	return QIPROG_SUCCESS;
}

static struct qiprog_driver stellaris_lpc_drv = {
	.scan = NULL,		/* scan is not used */
	.dev_open = lpc_open,
	.get_capabilities = get_capabilities,
	.set_bus = set_bus,
	.read_chip_id = read_chip_id,
	.read8 = read8,
	.read16 = read16,
	.read32 = read32,
	.write8 = write8,
	.write16 = write16,
	.write32 = write32,
};

struct qiprog_device stellaris_lpc_dev = {
	.drv = &stellaris_lpc_drv,
};
