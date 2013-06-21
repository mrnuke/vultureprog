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

#include <qiprog_usb_dev.h>
static struct qiprog_driver stellaris_lpc_drv;

/**
 * @brief QiProg driver 'dev_open' member
 */
static qiprog_err lpc_init(struct qiprog_device *dev)
{
	(void) dev;

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

static qiprog_err set_bus(struct qiprog_device * dev, enum qiprog_bus bus)
{
	(void) dev;
	/*
	 * Don't worry about switching buses for now. If LPC is specified, and
	 * only LPC, then we are happy.
	 */
	if ((bus & QIPROG_BUS_LPC) && ((bus & ~QIPROG_BUS_LPC) == 0))
		return QIPROG_SUCCESS;
	else
		return QIPROG_ERR_ARG;
}

static struct qiprog_driver stellaris_lpc_drv = {
	.scan = NULL,		/* scan is not used */
	.dev_open = lpc_init,
	.get_capabilities = get_capabilities,
	.set_bus = set_bus,
};

struct qiprog_device stellaris_lpc_dev = {
	.drv = &stellaris_lpc_drv,
};
