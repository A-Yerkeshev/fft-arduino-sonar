#include "hal_adc.h"
#include "Arduino.h"

/* Fills raw[] with AUDIO_FRAME_SIZE ADC readings from HAL_ADC_PIN. */
static void hal_adc_collect_raw(uint16_t raw[AUDIO_FRAME_SIZE]) {
    for (uint8_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        raw[i] = (uint16_t)analogRead(HAL_ADC_PIN);
    }
}

/* Returns the frame mean — the DC component of the unipolar sensor signal. */
static uint16_t hal_adc_compute_mean(uint16_t const raw[AUDIO_FRAME_SIZE]) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < AUDIO_FRAME_SIZE; i++) sum += raw[i];
    return (uint16_t)(sum / AUDIO_FRAME_SIZE);
}

/* Subtracts the DC mean from each sample so the signal is centered around zero. */
static void hal_adc_center(uint16_t const raw[AUDIO_FRAME_SIZE],
                            uint16_t mean, SensorSample out[AUDIO_FRAME_SIZE]) {
    for (uint8_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        out[i] = (SensorSample)((int16_t)raw[i] - (int16_t)mean);
    }
}

void hal_adc_init(void) {
    pinMode(HAL_ADC_PIN, INPUT);
}

AdcFrame hal_adc_read(void) {
    uint16_t raw[AUDIO_FRAME_SIZE];
    AdcFrame result;
    result.status = HalAdcStatus::HAL_ADC_STATUS_INVALID;

    hal_adc_collect_raw(raw);
    uint16_t mean = hal_adc_compute_mean(raw);
    hal_adc_center(raw, mean, result.samples);
    result.status = HalAdcStatus::HAL_ADC_STATUS_OK;

    return result;
}
