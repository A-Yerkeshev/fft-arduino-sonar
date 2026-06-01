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

/* All 64 values equal — mean equals that value, all centered samples must be zero. */
static void test_hal_adc_read_all_same(void) {
    /* Setup */
    uint16_t val = (uint16_t)(rand() % 1023 + 1); /* random value in [1, 1023] */
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

/* All values at maximum — mean = 1023, all centered samples must be zero. */
static void test_hal_adc_read_all_max(void) {
    /* Setup */
    uint16_t values[64];
    for (uint8_t i = 0; i < 64; i++) values[i] = 1023;
    adc_stub_set(values);

    /* Run */
    AdcFrame frame = hal_adc_read();

    /* Assert */
    for (uint8_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
        CHECK(frame.samples[i] == 0);
    }
}

/* One sample at 1023, rest at 0 — verifies mean subtraction on an asymmetric frame. */
static void test_hal_adc_read_one_max(void) {
    /* Setup */
    uint16_t values[64] = {0};
    values[0] = 1023;
    adc_stub_set(values);

    /* Run */
    AdcFrame frame = hal_adc_read();

    /* Assert — mean = 1023/64 = 15 (integer division) */
    CHECK(frame.samples[0] == (SensorSample)(1023 - 15));
    for (uint8_t i = 1; i < AUDIO_FRAME_SIZE; i++) {
        CHECK(frame.samples[i] == (SensorSample)(0 - 15));
    }
}

int main(void) {
    printf("hal_adc tests\n");
    RUN_TEST(test_hal_adc_read_all_same);
    RUN_TEST(test_hal_adc_read_all_max);
    RUN_TEST(test_hal_adc_read_one_max);
    printf("%s\n", s_failures == 0 ? "TESTS PASSED" : "TESTS FAILED");
    return s_failures == 0 ? 0 : 1;
}
