#pragma once
#include <stdint.h>

/* CONTRACT: hal_adc → fft
 * Types shared between the hal_adc and fft modules.
 */

/* DC-centered audio sample; range [-1023, +1023]. */
typedef int16_t SensorSample;

/* Number of samples per frame. */
#define AUDIO_FRAME_SIZE ((uint8_t) 64)
