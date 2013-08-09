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


qiprog_err jedec_write_co3eb007(struct qiprog_device *dev);
qiprog_err jedec_probe(struct qiprog_device *dev, struct qiprog_chip_id *id,
		       uint32_t phys_base, uint32_t *cmd_mask);
qiprog_err jedec_chip_erase(struct qiprog_device *dev, uint32_t phys_base,
			    uint32_t cmd_mask);
qiprog_err jedec_sector_erase(struct qiprog_device *dev, uint32_t sector,
			      uint32_t cmd_mask);
qiprog_err jedec_program_byte(struct qiprog_device *dev, uint32_t addr,
				     uint8_t val, uint32_t mask);
