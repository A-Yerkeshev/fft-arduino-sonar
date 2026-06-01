#pragma once
#include <stdint.h>

/* MODULE: hal_adc
 * PURPOSE: Collect a fixed-size frame of DC-centered samples from the sound sensor
 * HARDWARE: ADC pin A2, 10-bit resolution, Vref = Vcc = 5V
 */

/* --- Types --- */

/* Centered int16_t; range [-1023, +1023]; DC removed via frame mean subtraction. */
typedef int16_t AdcSample;

/* --- Constants --- */

#define HAL_ADC_PIN        ((uint8_t)  A2)    /* sound sensor pin; fixed for Arduino UNO board */
#define HAL_ADC_FRAME_SIZE ((uint8_t)  64)    /* samples per frame */
#define HAL_ADC_MAX_COUNT  ((uint16_t) 1023)  /* 2^10 - 1 */

/* --- Status --- */

enum class HalAdcStatus : uint8_t {
    HAL_ADC_STATUS_INVALID = 0,
    HAL_ADC_STATUS_OK,
};

/* --- Aggregate --- */

/* Return value of hal_adc_read */
typedef struct {
    AdcSample    samples[HAL_ADC_FRAME_SIZE]; /* DC-centered, range [-1023, +1023] */
    HalAdcStatus status;
} AdcFrame;

/* --- Functions --- */

/* PURPOSE: Configure ADC pin A2 as input.
 * RETURNS: void — pinMode() always succeeds on AVR for a valid fixed pin
 * EFFECTS: sets pin A2 to INPUT mode via Arduino pinMode()
 */
void hal_adc_init(void);

/* PURPOSE: Collect HAL_ADC_FRAME_SIZE samples from A2, subtract frame mean,
 *          return DC-centered results by value in an AdcFrame.
 * RETURNS: AdcFrame — .samples contains DC-centered readings; .status is
 *          HAL_ADC_STATUS_OK always (analogRead() cannot fail on AVR with a fixed pin)
 * EFFECTS: blocks ~6.6 ms (HAL_ADC_FRAME_SIZE=64 samples × 104 µs/sample = 6656 µs)
 * ASSUMES: hal_adc_init() has been called
 *          Vref = Vcc = 5V; ADC output range is [0, HAL_ADC_MAX_COUNT]
 *          sensor is unipolar (0 at rest); frame mean subtraction centers the signal
 *          but cannot recover the clipped negative half-cycle — harmonic distortion remains
 */
AdcFrame hal_adc_read(void);
