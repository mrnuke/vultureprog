/*
 * This file is part of the vultureprog project.
 *
 * Copyright (C) 2012-2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

#ifndef LPC_IO_H
#define LPC_IO_H

#include <qiprog.h>
#include <stdint.h>

void lpc_init(void);
qiprog_err lpc_mread(uint32_t addr, uint8_t * val8);
qiprog_err lpc_mwrite(uint32_t addr, uint8_t data);

#endif				/* LPC_IO_H */
