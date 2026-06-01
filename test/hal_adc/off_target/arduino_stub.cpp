#include "Arduino.h"
#include <string.h>

static uint16_t s_values[64] = {0};
static uint8_t  s_index      = 0;

void adc_stub_set(uint16_t const values[64]) {
    memcpy(s_values, values, sizeof(s_values));
    adc_stub_reset();
}

void     adc_stub_reset(void)    { s_index = 0; }
uint16_t analogRead(uint8_t)     { return s_values[s_index++]; }
void     pinMode(uint8_t, uint8_t) {}
