#pragma once
#define HAL_ADC_HEADER
#include "../decoupling_enforcer.h"
#include <stdint.h>
#include "../interfaces/adc_fft.h"

/* MODULE: hal_adc
 * PURPOSE: Collect a fixed-size frame of DC-centered samples from the sound sensor
 * DEPENDS ON: interfaces/adc_fft.h
 * HARDWARE: ADC pin A2, 10-bit resolution, Vref = Vcc = 5V
 */

/* --- Constants --- */

#define HAL_ADC_PIN       ((uint8_t)  A2)    /* sound sensor pin; fixed for Arduino UNO board */
#define HAL_ADC_MAX_COUNT ((uint16_t) 1023)  /* 2^10 - 1; ASSUME Vref = Vcc = 5V */

/* --- Status --- */

enum class HalAdcStatus : uint8_t {
    HAL_ADC_STATUS_INVALID = 0,
    HAL_ADC_STATUS_OK,
};

/* --- Aggregate --- */

/* Return value of hal_adc_read. */
typedef struct {
    SensorSample samples[AUDIO_FRAME_SIZE]; /* DC-centered, range [-1023, +1023] */
    HalAdcStatus status;
} AdcFrame;

/* --- Functions --- */

/* PURPOSE: Configure ADC pin A2 as input.
 * RETURNS: void — pinMode() always succeeds on AVR for a valid fixed pin
 * EFFECTS: sets pin A2 to INPUT mode via Arduino pinMode()
 */
void hal_adc_init(void);

/* PURPOSE: Collect AUDIO_FRAME_SIZE samples from A2, subtract frame mean,
 *          return DC-centered results by value in an AdcFrame.
 * RETURNS: AdcFrame — .samples contains DC-centered readings; .status is
 *          HAL_ADC_STATUS_OK always (analogRead() cannot fail on AVR with a fixed pin)
 * EFFECTS: blocks ~6.6 ms (AUDIO_FRAME_SIZE=64 samples × 104 µs/sample = 6656 µs)
 * ASSUMES: hal_adc_init() has been called
 *          Vref = Vcc = 5V; ADC output range is [0, HAL_ADC_MAX_COUNT]
 *          sensor is unipolar (0 at rest); frame mean subtraction centers the signal
 *          but cannot recover the clipped negative half-cycle — harmonic distortion remains
 */
AdcFrame hal_adc_read(void);
