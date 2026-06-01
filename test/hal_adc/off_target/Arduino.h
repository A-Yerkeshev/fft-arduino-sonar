#pragma once
#include <stdint.h>

/* Off-target stub for Arduino functions used by hal_adc.
 * The stub owns its internal state; tests interact via adc_stub_set / adc_stub_reset.
 */

#define INPUT ((uint8_t) 0)
#define A2    ((uint8_t) 2)

/* Load 64 values that analogRead will return in sequence; resets the read index. */
void adc_stub_set(uint16_t const values[64]);

/* Reset the read index to 0 so the next analogRead returns values[0] again. */
void adc_stub_reset(void);

uint16_t analogRead(uint8_t pin);
void     pinMode(uint8_t pin, uint8_t mode);
