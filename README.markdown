Morse Code on the MSP430
========================

This is an [Energia](https://github.com/energia/Energia/) sketch for the
[TI MSP430 LaunchPad](http://processors.wiki.ti.com/index.php/MSP430_LaunchPad_(MSP-EXP430G2))
with the [MSP430G2553](http://www.ti.com/product/msp430g2553), which
provides encoding and decoding of Morse code.  Morse symbols received by
the UART are encoded and "sent" on the LaunchPad's green LED; symbols keyed
on pushbutton S2 are decoded and written to the microcontroller's UART.

This program relies on the MSP430G2553's hardware UART, and so requires the
LaunchPad's jumpers to be
[configured accordingly](https://github.com/energia/Energia/wiki/Serial-Communication#wiki-Hardware_Configuration).

## Note
This repository is superseded by [https://github.com/mshroyer/mcu-morse](https://github.com/mshroyer/mcu-morse), with updated code supporting the ATmega32u4 as well as other Arduino-compatible microcontrollers.