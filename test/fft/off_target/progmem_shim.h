#pragma once

/* Off-target stubs for AVR PROGMEM macros.
 * On Arduino, PROGMEM stores data in flash and pgm_read_word reads from flash.
 * On Linux host, data lives in normal memory — direct pointer dereference suffices.
 */
#define PROGMEM
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
