/* Hardware test for hal_adc module.
 * Reads one frame per loop, prints min and max of centered samples over Serial.
 *
 * Expected behaviour:
 *   Silence:    min and max near 0 (+-5 to +-20, ADC noise only)
 *   Sound:      min negative, max positive, growing with volume
 *   Loud sound: min approaching -1023, max approaching +1023
 *
 * If min is always 0 or positive: centering is not working.
 * If both are always 0: wrong pin or sensor not connected.
 */

#include "hal_adc.h"

void setup() {
    Serial.begin(9600);
    hal_adc_init();
}

void loop() {
    AdcFrame frame = hal_adc_read();

    AdcSample min_val = frame.samples[0];
    AdcSample max_val = frame.samples[0];
    for (uint8_t i = 1; i < HAL_ADC_FRAME_SIZE; i++) {
        if (frame.samples[i] < min_val) min_val = frame.samples[i];
        if (frame.samples[i] > max_val) max_val = frame.samples[i];
    }

    Serial.print("min=");
    Serial.print(min_val);
    Serial.print("  max=");
    Serial.println(max_val);
    delay(200);
}
