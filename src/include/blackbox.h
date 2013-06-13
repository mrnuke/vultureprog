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
 * @defgroup debug_console Debug console
 *
 * \brief Types and definitions for debugging
 *
 * The debug console provides minimal functionality for debugging such as debug
 * levels, and printf-style functionality. Hardware-specific code should
 * implement @ref blackbox_init(). This function initializes a debug console.
 * How the debug console works, or what transport it uses is hardware-dependent.
 * For example, a debug console may be implemented over a UART or SPI bus, or
 * as a CDCACM interface over the main USB connection.
 *
 * Each board should also implement the minimal functionality for printf(). In
 * most cases, providing a _write() function to send stdout to the debug console
 * is sufficient.
 *
 * printk() should be used for debug messages, not printf(). The severity of
 * each message is specified with @ref log_level. The default log level is
 * configurable in config.h
 */

#ifndef BLACKBOX_H
#define BLACKBOX_H

/** @{ */
#include <stdio.h>
#include <config.h>

#if CONFIG_ENABLE_CONSOLE
#define printk(LEVEL, fmt, ...)					\
	do {							\
		if (CONFIG_LOGLEVEL >= LEVEL)			\
			printf(fmt, ##__VA_ARGS__);		\
								\
	} while(0)
#else				/* CONFIG_ENABLE_CONSOLE */
#define printk(LEVEL, fmt, ...)					\
	do { } while(0)
#endif				/* CONFIG_ENABLE_CONSOLE */

#define print_emerg(x, ...)		printk(LOG_EMERG,  x,	##__VA_ARGS__)
#define print_alert(x, ...)		printk(LOG_ALERT,  x,	##__VA_ARGS__)
#define print_crit(x, ...)		printk(LOG_CRIT,   x,	##__VA_ARGS__)
#define print_err(x, ...)		printk(LOG_ERR,    x,	##__VA_ARGS__)
#define print_warn(x, ...)		printk(LOG_WARN,   x,	##__VA_ARGS__)
#define print_notice(x, ...)		printk(LOG_NOTICE, x,	##__VA_ARGS__)
#define print_info(x, ...)		printk(LOG_INFO,   x,	##__VA_ARGS__)
#define print_debug(x, ...)		printk(LOG_DEBUG,  x,	##__VA_ARGS__)
#define print_spew(x, ...)		printk(LOG_SPEW,   x,	##__VA_ARGS__)

enum log_level {
	LOG_EMERG = 1,	/**< Firmware is unstable */
	LOG_ALERT,	/**< Action must be taken immediately*/
	LOG_CRIT,	/**< Critical conditions */
	LOG_ERR,	/**< Error conditions */
	LOG_WARN,	/**< Warning conditions */
	LOG_NOTICE,	/**< Normal, but significant conditions */
	LOG_INFO,	/**< Informational messages */
	LOG_DEBUG,	/**< Debug-level messages */
	LOG_SPEW,	/**< Way too many details */
};

/* Initialize the debugging subsystem */
void blackbox_init(void);

/** @} */

#endif				/* BLACKBOX_H */
