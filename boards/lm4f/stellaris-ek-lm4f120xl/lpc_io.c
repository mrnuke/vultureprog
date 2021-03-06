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

#include "lpc_io.h"

#include <qiprog.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/gpio.h>

/* LAD[3:0] */
#define LADPORT		GPIOB
#define LADPINS		(GPIO0 | GPIO1 | GPIO2 | GPIO3)
/* LCLK */
#define CLKPORT		GPIOC
#define CLKPIN		(GPIO5)
/* #LFRAME */
#define LFPORT		GPIOD
#define LFPIN		(GPIO2)
/* ID[3:0] */
#define IDPORT		GPIOE
#define IDPINS		(GPIO0 | GPIO1 | GPIO2 | GPIO3)
/* MODE and #CE pins */
#define CTLPORT		GPIOA
#define MODEPIN		(GPIO4)
#define CEPIN		(GPIO3)
#define RSTPIN		(GPIO6)

#include <blackbox.h>

/*
 * GPIOE[3:0] <-> LDAT[3:0]
 * GPIOC5 <-> CLK
 * GPIOD2 <-> #LFRAME
 */

void lpc_init(void)
{
	uint8_t pins;
	/* We use all GPIO ports from GPIOA to GPIOE */
	/* GPIOA will have been enabled by the UART */
	periph_clock_enable(RCC_GPIOB);
	periph_clock_enable(RCC_GPIOC);
	/* GPIOD will have been enabled by USB */
	periph_clock_enable(RCC_GPIOE);

	/* All unused pins are pulled high. We must be careful to not touch pins
	 * us ed by other peripherals. */
	pins = GPIO_ALL & ~(GPIO0 | GPIO1);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pins);
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_ALL);
	pins = GPIO4 | GPIO5 | GPIO6 | GPIO7;
	gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pins);
	pins = GPIO_ALL & ~(GPIO4 | GPIO5);
	gpio_mode_setup(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pins);
	gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_ALL);
	/*
	 * The default drive strength is 2mA. Depending on how fast we turn the
	 * bus and the layout of the tracks/wires, this may or may not be
	 * sufficient. 8mA drive gives more consistent results, even with long
	 * wires, so use this setting for the time being.
	 */
	gpio_mode_setup(LADPORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, LADPINS);
	gpio_set_output_config(LADPORT, GPIO_OTYPE_PP, GPIO_DRIVE_8MA, LADPINS);

	gpio_mode_setup(CLKPORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CLKPIN);
	gpio_set_output_config(CLKPORT, GPIO_OTYPE_PP, GPIO_DRIVE_8MA, CLKPIN);

	gpio_mode_setup(LFPORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LFPIN);
	gpio_set_output_config(LFPORT, GPIO_OTYPE_PP, GPIO_DRIVE_8MA, LFPIN);

	gpio_mode_setup(IDPORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, IDPINS);
	gpio_set_output_config(IDPORT, GPIO_OTYPE_PP, GPIO_DRIVE_2MA, IDPINS);
	/* Set ID pins to 0 (boot device) */
	gpio_clear(IDPORT, IDPINS);

	pins = RSTPIN | CEPIN | MODEPIN;
	gpio_mode_setup(CTLPORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pins);
	gpio_set_output_config(CTLPORT, GPIO_OTYPE_PP, GPIO_DRIVE_2MA, pins);

	/* Put chip in LPC mode, select #CE, and put chip in reset */
	gpio_clear(CTLPORT, pins);
	print_spew("This line is used to delay until RESET is released.\n");
	/* Release reset. the chip is now ready */
	gpio_set(CTLPORT, RSTPIN);
}

/**
 * @brief Switch LAD[3:0] pins to inputs
 */
static inline void lad_mode_in(void)
{
	GPIO_DIR(LADPORT) &= ~LADPINS;
}

/**
 * @brief Switch LAD[3:0] pins to outputs
 */
static inline void lad_mode_out(void)
{
	GPIO_DIR(LADPORT) |= LADPINS;
}

/**
 * @brief Write a nibble on the LAD[3:0] pins
 */
static inline void lad_write(uint8_t dat4)
{
	/*
	 * This works without bit shifting because our LAD pins are sequential
	 * and start from GPIO0. Thus, the lower nibble of dat4 maps to the LAD
	 * pins without needing shifting.
	 * The same reasoning applies in lad_read().
	 */
	gpio_write(LADPORT, LADPINS, dat4);
}

/**
 * @brief Read a nibble from the LAD[3:0] pins
 */
static inline uint8_t lad_read(void)
{
	/*
	 * Writing then reading to a GPIO port does not guarantee that the input
	 * data is latched after the output has been driven. This effect can be
	 * observed regardless of the clock frequency. A memory barrier does not
	 * solve the issue; however a delay of at least four "nop" ensures that
	 * the data is latched after the output has been written. The four "nop"
	 * delay is independent of the core clock.
	 */
	asm("nop"); asm("nop");asm("nop");asm("nop");
	return gpio_read(LADPORT, LADPINS);
}

/**
 * @brief Assert clock signal
 */
static inline void clk_high(void)
{
	gpio_set(CLKPORT, CLKPIN);
}

/**
 * @brief De-assert clock signal
 */
static inline void clk_low(void)
{
	gpio_clear(CLKPORT, CLKPIN);
}

/**
 * @brief Assert #LFRAME signal
 */
static inline void lframe_high(void)
{
	gpio_set(LFPORT, LFPIN);
}

/**
 * @brief De-assert #LFRAME signal
 */
static inline void lframe_low(void)
{
	gpio_clear(LFPORT, LFPIN);
}

/**
 * @brief Start an LPC frame (1st two clocks of an LPC frame)
 */
static inline void lpc_start_frame(uint8_t type)
{
	lad_mode_out();
	clk_low();

	/* 1: START */
	lframe_low();
	lad_write(0);
	clk_high();
	lframe_high();
	clk_low();

	/* 2: Cycle Type and direction */
	lad_write(type);
	clk_high();
	clk_low();
}

/**
 * @brief Write a 32-bit address on the LPC bus (clocks 3-10)
 */
static inline void lpc_send_address(uint32_t addr)
{
	/* Keep this out of a loop. We get a 40% throughput increase. */
	lad_write(addr >> 28);
	clk_high();
	clk_low();
	lad_write(addr >> 24);
	clk_high();
	clk_low();
	lad_write(addr >> 20);
	clk_high();
	clk_low();
	lad_write(addr >> 16);
	clk_high();
	clk_low();
	lad_write(addr >> 12);
	clk_high();
	clk_low();
	lad_write(addr >> 8);
	clk_high();
	clk_low();
	lad_write(addr >> 4);
	clk_high();
	clk_low();
	lad_write(addr >> 0);
	clk_high();
	clk_low();
}

/**
 * @brief Read a byte from the LPC bus (2 clocks)
 */
static inline uint8_t lpc_read8(void)
{
	uint8_t data;

	/* Least-Significant Nibble */
	clk_high();
	data = lad_read();
	clk_low();

	/* Most Significant Nibble */
	clk_high();
	data |= (lad_read() << 4);
	clk_low();

	return data;
}

/**
 * @brief Write a byte on the LPC bus (2 clocks)
 */
static inline void lpc_write8(uint8_t data)
{
	/* Least-Significant Nibble */
	lad_write(data);
	clk_high();
	clk_low();

	/* Most Significant Nibble */
	lad_write(data >> 4);
	clk_high();
	clk_low();
}

/**
 * @brief Turnaround the bus to the slave
 */
static inline uint8_t lpc_tar_to_slave(void)
{
	uint8_t readback;

	/* 11 - TAR0 */
	lad_write(0xf);
	lad_mode_in();
	clk_high();
	clk_low();

	/* 12 - TAR1 */
	clk_high();
	readback = lad_read();
	clk_low();

	return readback;
}

/**
 * @brief Turnaround the bus back to the host
 */
static inline uint8_t lpc_tar_to_host(void)
{
	uint8_t readback;

	/* 16 - TAR0 */
	clk_high();
	readback = lad_read();
	clk_low();
	lad_mode_out();

	/* 17 - TAR1 */
	lad_write(0xf);
	clk_high();
	clk_low();

	return readback;
}

/**
 * @brief Do an LPC memory read cycle
 */
qiprog_err lpc_mread(uint32_t addr, uint8_t * val8)
{
	uint8_t data;
	uint8_t tar1_12, rsync;

	/* 1-2: START, cycle type and direction */
	lpc_start_frame(0x4);

	/* 3-10: Address */
	lpc_send_address(addr);

	/* 11-12: TAR - turn the bus to the slave */
	tar1_12 = lpc_tar_to_slave();
	/*
	 * We don't have enough power to run with LPC timing. Some chips love to
	 * implement internal timing during the TAR cycle, rather than sample
	 * the clock. Since our clock is much slower than the chip's internal
	 * delay, it may think it got to the SYNC cycle. If we get a SYNC during
	 * the second clock of the TAR, then so be it; continue our LPC frame.
	 */
	if (!tar1_12)
		goto skip_sync_at_lpc_read;

	/* 13 - RSYNC */
	clk_high();
	if ((rsync = lad_read()) != 0) {
		*val8 = 0xff;
		return QIPROG_ERR_NO_RESPONSE;
	}
	clk_low();

 skip_sync_at_lpc_read:
	/* 14 - 15: Data byte */
	data = lpc_read8();

	/* 16-17 - TAR: turn the bus back to the host */
	lpc_tar_to_host();

	*val8 = data;
	return QIPROG_SUCCESS;
}

/**
 * @brief Do an LPC memory write cycle
 */
qiprog_err lpc_mwrite(uint32_t addr, uint8_t data)
{
	uint8_t tar1_12, rsync;

	/* 1-2: START, cycle type and direction */
	lpc_start_frame(0x6);

	/* 3-10 - Address */
	lpc_send_address(addr);

	/* 11-12 - Data byte */
	lpc_write8(data);

	/* 13-14: TAR - turn the bus to the slave */
	tar1_12 = lpc_tar_to_slave();
	if (!tar1_12)
		goto skip_sync_at_lpc_write;

	/* 15 - RSYNC */
	clk_high();
	if ((rsync = lad_read()) != 0)
		return QIPROG_ERR_NO_RESPONSE;

	clk_low();

 skip_sync_at_lpc_write:

	/* 16-17: TAR - turn the bus back to the host */
	lpc_tar_to_host();

	return QIPROG_SUCCESS;
}
