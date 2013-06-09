##
## This file is part of the vultureprog project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

PREFIX		?= arm-none-eabi
#PREFIX		?= arm-elf

TARGETS = stm32/f1 stm32/f2 stm32/f4 stm32/l1 lpc13xx lpc17xx lpc43xx lm3s lm4f efm32/efm32tg efm32/efm32g efm32/efm32lg efm32/efm32gg

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
# Do not print "Entering directory ...".
MAKEFLAGS += --no-print-directory
endif

all: build

build: lib boards

lib:
	$(Q)if [ ! "`ls -A libopencm3`" ] ; then \
		echo "######## ERROR ########"; \
		echo "\tlibopencm3 is not initialized."; \
		echo "\tPlease run:"; \
		echo "\t$$ git submodule init"; \
		echo "\t$$ git submodule update"; \
		echo "\tbefore running make."; \
		echo "######## ERROR ########"; \
		exit 1; \
		fi
	$(Q)$(MAKE) -C libopencm3

BOARD_DIRS:=$(sort $(dir $(wildcard $(addsuffix /*/Makefile,$(addprefix boards/,$(TARGETS))))))
$(BOARD_DIRS): lib
	@printf "  BUILD   $@\n";
	$(Q)$(MAKE) --directory=$@

boards: $(BOARD_DIRS)
	$(Q)true

# Bleh http://www.makelinux.net/make3/make3-CHP-6-SECT-1#make3-CHP-6-SECT-1
clean:
	$(Q)$(MAKE) -C libopencm3 clean
	$(Q)for i in $(BOARD_DIRS); do \
		if [ -d $$i ]; then \
			printf "  CLEAN   $$i\n"; \
			$(MAKE) -C $$i clean SRCLIBDIR=$(SRCLIBDIR) || exit $?; \
		fi; \
	done

.PHONY: build lib boards $(BOARD_DIRS) install clean

