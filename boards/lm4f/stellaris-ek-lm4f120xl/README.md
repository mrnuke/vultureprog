Stellaris Launchpad Vultureprog Booster Pack
============================================

Placeholder for more information


Debug console:
--------------

The debug console is implemented over UART0. UART0 of the Stellaris is connected
to the CDCACM interface on the debug chip. Just connect the debug USB cable and
use a terminal program to open the ACM port on the with 921600-8N1.

For example:
> $ picocom /dev/ttyACM0 -b921600
