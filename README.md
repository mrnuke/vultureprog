README
======

Under construction, please pardon the dust
------------------------------------------

This repository contains firmware for programmers for Parallel/LPC/FWH/SPI
flash chips.



Prerequisites
-------------

##### An arm-none-eabi toolchain

Any toolchain that understands the M4F core can be used. However, the toolchain
must also have the following features and libraries
 - Hard-float "-mfloat-abi=hard -mfpu=fpv4-sp-d16" compiled libraries
 - A minimal C library (newlib works great)
 - libnosys

The summon-arm-toolchain is a great tool which supports all of the above.

##### OpenOCD ( >= 0.7.0 ) with ti-icdi support

OpenOCD is used for "$ make flash" support. You may use alternatives such as
lm4flash if you desire.

If you use OpenOCD, you will need version 0.7.0 or later. Previous versions do
not have support for ti-icdi. If OpenOCD complains about an unsupported
interface or nonexistent script on "$ make flash", then most likely, it is
compiled without ti-icdi support. In that case you will have to rebuild it from
sources (sorry), and pass "--enable-ti-icdi" to the configure script.



First-time setup
----------------

Before building firmware images, the dependencies should be pulled in. Please
run:

> $ git submodule init

> $ git submodule update

Although some toolchains ship libopencm3 as a library, We include it here for
several reasons. The major one is that LM4F support was only recently added, and
it is very likely that your toolchain's libopencm3 will not include it. Second,
the main developer is also a libopencm3 contributor, and including the library
makes it very easy to send changes and bugfixes upstream and include them here.



Usage
-----

See HACKING.md for instructions on setting up a toolchain, debugger, and
programmer. Hardware specific code is placed under
> boards/[processor family]/[board name]/

It's also where the firmware files will be created.

There is no need to configure a buildsystem. Everything is makefile-based. To
build everything, just
> $ make

Once the build completes (no need to take a coffee break. It should finish
fast), you can load the firmware to the board.
> $ make flash -C boards/[processor family]/[board name]

Yes, it's that easy!



Troubleshooting
---------------

> arm-none-eabi/bin/ld: cannot find -lnosys

Your toolchain does not provide libnosys. Please update your toolchain.

> [file].elf uses VFP register arguments, [library].a does not

Your toolchain does not link against supporting libraries compiled with
"-mfloat-abi=hard -mfpu=fpv4-sp-d16". You have a number of options:

1. Update your toolchain to link against hard-float libraries.
2. Scour the makefiles (including libopencm3 submodule) and remove
    "-mfloat-abi=hard -mfpu=fpv4-sp-d16" from compiler and linker flags. Please
    note that this is not suitable for inclusion upstream.



Copyright and license
---------------------

Vultureprog is licensed under the terms of the GNU General Public License
(GPL), version 3 or later.

While some individual source code files are licensed under different licenses,
this does not change the fact that the program as a whole is licensed under the
terms of the GPLv3+.

Please see the individual source files for the full list of copyright holders.



Copyright notices
-----------------

A copyright notice indicating a range of years, must be interpreted as having
had copyrightable material added in each of those years.

For example:

> Copyright (C) 2010-2013 Contributor Name

is to be interpreted as

> Copyright (C) 2010,2011,2012,2013 Contributor Name
