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

#ifndef LED_H
#define LED_H

#include <libopencm3/lm4f/gpio.h>

/* This is how the RGB LED is connected on the stellaris launchpad */
#define RGB_PORT	GPIOF

enum stellaris_led {
	LED_R = GPIO1,
	LED_G = GPIO3,
	LED_B = GPIO2,
};

static inline void led_on(enum stellaris_led led)
{
	gpio_set(RGB_PORT, led);
}

static inline void led_off(enum stellaris_led led)
{
	gpio_clear(RGB_PORT, led);
}

void led_init(void);

#endif				/* LED_H */
