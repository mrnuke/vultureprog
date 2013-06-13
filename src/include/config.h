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
 * @defgroup config General configuration options
 *
 * \brief Overall options that control the behavior of certain subsystems
 */

#ifndef CONFIG_H
#define CONFIG_H

/** @{ */

/* Enable console logging - Where the console logs is hardware-specific */
#define CONFIG_ENABLE_CONSOLE 1
/* Debug level */
#define CONFIG_LOGLEVEL LOG_SPEW

/** @} */
#endif				/* CONFIG_H */
