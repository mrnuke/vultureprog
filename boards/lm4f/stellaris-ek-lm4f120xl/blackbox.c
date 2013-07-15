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

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/uart.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/lm4f/nvic.h>

#include <blackbox.h>

/**
 * \brief Initialize the debugging subsystem.
 */
void blackbox_init(void)
{
	/* Enable GPIOA in run mode. */
	periph_clock_enable(RCC_GPIOA);
	/* Mux PA1 (Tx) to UART0 (alternate function 1) */
	gpio_set_af(GPIOA, 1, GPIO1);

	/* Enable the UART clock */
	periph_clock_enable(RCC_UART0);
	/* We need a brief delay before we can access UART config registers */
	__asm__("nop");
	/* Disable the UART while we mess with its setings */
	uart_disable(UART0);
	/* Configure the UART clock source as precision internal oscillator */
	uart_clock_from_piosc(UART0);
	/* Set communication parameters */
	uart_set_baudrate(UART0, 921600);
	uart_set_databits(UART0, 8);
	uart_set_parity(UART0, UART_PARITY_NONE);
	uart_set_stopbits(UART0, 1);
	/* Enable FIFOs */
	uart_enable_fifo(UART0);
	/* Now that we're done messing with the settings, enable the UART */
	uart_enable(UART0);
}

void hard_fault_handler(void)
{
	uint32_t reg32;

	print_emerg("Hard fault occured\n");

	/*
	 * We can't do anything about hard faulting, but we can send info
	 * through the console
	 */
	reg32 = SCB_HFSR;
	if (reg32 & SCB_HFSR_FORCED)
		print_emerg("FORCED: Fault escalated to hard fault\n");
	if (reg32 & SCB_HFSR_VECTTBL)
		print_emerg("VECT\n");

	reg32 = SCB_CFSR;
	if (reg32 & SCB_CFSR_DIVBYZERO)
		print_emerg("DIV0: Divide-by-Zero Usage Fault\n");
	if (reg32 & SCB_CFSR_UNALIGNED)
		print_emerg("UNALIGN: Unaligned Access Usage Fault\n");
	if (reg32 & SCB_CFSR_NOCP)
		print_emerg("NOCP: No Coprocessor Usage Fault\n");
	if (reg32 & SCB_CFSR_INVPC)
		print_emerg("INVCP: Invalid PC Load Usage Fault\n");
	if (reg32 & SCB_CFSR_INVSTATE)
		print_emerg("INVSTAT: Invalid State Usage Fault\n");
	if (reg32 & SCB_CFSR_UNDEFINSTR)
		print_emerg("UNDEF: Undefined Instruction Usage Fault\n");
	if (reg32 & SCB_CFSR_BFARVALID)
		print_emerg("BFARV: Bus Fault Address Register Valid\n");
	if (reg32 & (1 << 13) )
		print_emerg("BLSPERR: Bus Fault on Floating-Point Lazy State Preservation\n");
	if (reg32 & SCB_CFSR_STKERR)
		print_emerg("BSTKE: Stack Bus Fault\n");
	if (reg32 & SCB_CFSR_UNSTKERR)
		print_emerg("BUSTKE: Unstack Bus Fault\n");
	if (reg32 & SCB_CFSR_IMPRECISERR)
		print_emerg("IMPRE: Imprecise Data Bus Error\n");
	if (reg32 & SCB_CFSR_PRECISERR)
		print_emerg("PRECISE: Precise Data Bus Error\n");
	if (reg32 & SCB_CFSR_IBUSERR)
		print_emerg("IBUS: Instruction Bus Error\n");
	if (reg32 & SCB_CFSR_MMARVALID)
		print_emerg("MMARV:Memory Management Fault Address Register Valid\n");
	if (reg32 & (1 << 5) )
		print_emerg("MLSPERR:Memory Management Fault on Floating-Point Lazy State Preservation\n");
	if (reg32 & SCB_CFSR_MSTKERR)
		print_emerg("MSTKE: Stack Access Violation\n");
	if (reg32 & SCB_CFSR_MUNSTKERR)
		print_emerg("MUSTKE: Unstack Access Violation\n");
	if (reg32 & SCB_CFSR_DACCVIOL)
		print_emerg("DERR: Data Access Violation\n");
	if (reg32 & SCB_CFSR_IACCVIOL)
		print_emerg("IERR: Instruction Access Violation\n");
	while (1) ;
}

static void send_uart(char *ptr, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (ptr[i] == '\n')
			uart_send_blocking(UART0, '\r');
		uart_send_blocking(UART0, ptr[i]);
	}
}

/*
 * Route debug output to UART0
 */
void print_blackbox(const char *format, ...)
{
	char buffer[256];
	int len;
	va_list args;
	va_start(args, format);
	len = vsnprintf(buffer, sizeof(buffer), format, args);
	send_uart(buffer, len);
	va_end(args);
}
