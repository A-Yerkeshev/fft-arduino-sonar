#include <stdio.h>
#include <stdlib.h>
#include "Arduino.h"
#include "hal_adc.h"

/* Running count of failed assertions across all tests. */
static int s_failures = 0;

/* Evaluates cond; logs file/line and increments s_failures if false.
 * do-while(false) ensures the macro is safe inside if/else without braces. */
#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        s_failures++; \
    } \
} while (false)

/* Prints the test name then runs it; failed CHECKs appear indented beneath. */
#define RUN_TEST(fn) do { printf(#fn "\n"); fn(); } while (false)

/* -----------------------------------------------------------------------
 * Input space for hal_adc_read:
 *   Raw ADC value per sample: [0, 1023] (10-bit ADC, Vref = Vcc = 5V)
 *
 * Equivalence classes:
 *   Class A — Uniform frame: all 64 samples equal
 *     Boundary A1: all = 0    (ADC minimum)
 *     Boundary A2: all = 1023 (ADC maximum)
 *     Interior A3: all = random value in [1, 1022]
 *   Class B — Single outlier: one sample differs, rest uniform
 *     Boundary B1: outlier = 1023, rest = 0    → max positive centered output (1008)
 *     Boundary B2: outlier = 0,    rest = 1023 → max negative centered output (-1007)
 *
 * Cases deliberately not tested:
 *   - Two or more outliers: mean arithmetic is identical; B1/B2 cover the extremes
 *   - hal_adc_init: calls pinMode() which always succeeds on AVR; no logic to test
 * ----------------------------------------------------------------------- */

/* A1: all samples = 0 — mean = 0, all centered outputs must be 0. */
static void test_hal_adc_read_uniform_min(void) {
    /* Setup */
    uint16_t values[64] = {0};
    adc_stub_set(values);

    /* Run */
    AdcFrame frame = hal_adc_read();

    /* Assert */
    CHECK(frame.status == HalAdcStatus::HAL_ADC_STATUS_OK);
    for (uint8_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        CHECK(frame.samples[i] == 0);
    }
}

/* A2: all samples = 1023 — mean = 1023, all centered outputs must be 0. */
static void test_hal_adc_read_uniform_max(void) {
    /* Setup */
    uint16_t values[64];
    for (uint8_t i = 0; i < 64; i++) values[i] = 1023;
    adc_stub_set(values);

    /* Run */
    AdcFrame frame = hal_adc_read();

    /* Assert */
    CHECK(frame.status == HalAdcStatus::HAL_ADC_STATUS_OK);
    for (uint8_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        CHECK(frame.samples[i] == 0);
    }
}

/* A3: all samples equal some interior value — mean equals that value, all outputs 0. */
static void test_hal_adc_read_uniform_mid(void) {
    /* Setup */
    uint16_t val = (uint16_t)(rand() % 1022 + 1); /* random value in [1, 1022] */
    uint16_t values[64];
    for (uint8_t i = 0; i < 64; i++) values[i] = val;
    adc_stub_set(values);

    /* Run */
    AdcFrame frame = hal_adc_read();

    /* Assert */
    CHECK(frame.status == HalAdcStatus::HAL_ADC_STATUS_OK);
    for (uint8_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        CHECK(frame.samples[i] == 0);
    }
}

/* B1: one sample = 1023, rest = 0 — maximum positive centered output.
 * mean = 1023/64 = 15 (integer division); samples[0] = 1008, rest = -15. */
static void test_hal_adc_read_outlier_max(void) {
    /* Setup */
    uint16_t values[64] = {0};
    values[0] = 1023;
    adc_stub_set(values);

    /* Run */
    AdcFrame frame = hal_adc_read();

    /* Assert */
    CHECK(frame.status == HalAdcStatus::HAL_ADC_STATUS_OK);
    CHECK(frame.samples[0] == (SensorSample)(1023 - 15));
    for (uint8_t i = 1; i < AUDIO_FRAME_SIZE; i++) {
        CHECK(frame.samples[i] == (SensorSample)(0 - 15));
    }
}

/* B2: one sample = 0, rest = 1023 — maximum negative centered output.
 * mean = (63 * 1023) / 64 = 1007 (integer division); samples[0] = -1007, rest = 16. */
static void test_hal_adc_read_outlier_min(void) {
    /* Setup */
    uint16_t values[64];
    for (uint8_t i = 0; i < 64; i++) values[i] = 1023;
    values[0] = 0;
    adc_stub_set(values);

    /* Run */
    AdcFrame frame = hal_adc_read();

    /* Assert */
    CHECK(frame.status == HalAdcStatus::HAL_ADC_STATUS_OK);
    CHECK(frame.samples[0] == (SensorSample)(0 - 1007));
    for (uint8_t i = 1; i < AUDIO_FRAME_SIZE; i++) {
        CHECK(frame.samples[i] == (SensorSample)(1023 - 1007));
    }
}

int main(void) {
    printf("hal_adc tests\n");
    RUN_TEST(test_hal_adc_read_uniform_min);
    RUN_TEST(test_hal_adc_read_uniform_max);
    RUN_TEST(test_hal_adc_read_uniform_mid);
    RUN_TEST(test_hal_adc_read_outlier_max);
    RUN_TEST(test_hal_adc_read_outlier_min);
    printf("%s\n", s_failures == 0 ? "TESTS PASSED" : "TESTS FAILED");
    return s_failures == 0 ? 0 : 1;
}
