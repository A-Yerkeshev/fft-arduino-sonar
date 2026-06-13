#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fft.h"

static int s_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        s_failures++; \
    } \
} while (false)

#define RUN_TEST(fn) do { printf(#fn "\n"); fn(); } while (false)

/* -----------------------------------------------------------------------
 * Input space for fft_compute:
 *   FftInput.samples[n]: SensorSample (int16_t), 64 values, DC-centered
 *
 * Equivalence classes:
 *   Class A — Zero input: all samples = 0
 *   Class B — Noisy single tone: sinusoid at bin k + uniform noise
 *     Boundary B1: bin 1  (lowest non-zero frequency, 150 Hz)
 *     Boundary B2: bin 31 (Nyquist boundary, 4650 Hz)
 *   Class C — Noisy impulse at Hann window centre (n=32)
 *     C1: samples[32]=1023, rest = noise → flat spectrum
 *   Class D — Input validation boundary (q15_make rejection threshold)
 *     D1: one sample = -32768 → FFT_STATUS_INVALID
 *     D2: one sample = -32767 → FFT_STATUS_OK (just inside valid range)
 *   Class E — Two-tone + noise: dominant and secondary frequencies
 *     Boundary E1: dominant at bin 4  (600 Hz), secondary at bin 5  (750 Hz)
 *     Boundary E2: dominant at bin 20 (3000 Hz), secondary at bin 19 (2850 Hz)
 *
 * Validated noise levels:
 *   Class B: tone amplitude 800, noise ±800 — SNR 0 dB (1:1); FFT processing
 *            gain (~15 dB for N=64) lifts effective output SNR to ~15 dB
 *   Class C: impulse 1023 at window centre, surrounding noise ±200
 *   Class E: dominant amplitude 600, secondary 150, noise ±100
 *            dominant:secondary ratio 12 dB (4:1); SNR 15.6 dB (6:1)
 *
 * Validated frequency separation:
 *   Class B: single tone tested at bin 1 and bin 31 (4500 Hz apart)
 *   Class E: two tones separated by 1 bin = 150 Hz (150 Hz/bin × 1)
 *            — minimum resolvable separation for this FFT size
 *
 * Random seed: srand(42) per test for reproducibility.
 *
 * Cases deliberately not tested:
 *   - Re-entrancy (FFT_STATUS_REENTRANT): not triggerable single-threaded
 *   - DC input: Hann window shapes it into a bell; expected bin non-trivial
 *   - Exact magnitude values: Q15 rounding makes precise prediction impractical
 * ----------------------------------------------------------------------- */

/* Returns FftInput with a sinusoid at the given bin plus uniform noise. */
static FftInput make_tone(uint8_t bin, int16_t amplitude, int16_t noise_amp) {
    FftInput input;
    for (uint8_t n = 0; n < FFT_N; n++) {
        double   tone  = amplitude * sin(2.0 * M_PI * bin * n / FFT_N);
        int16_t  noise = (int16_t)((rand() % (2 * noise_amp + 1)) - noise_amp);
        input.samples[n] = (SensorSample)((int16_t)(tone + noise));
    }
    return input;
}

/* Returns FftInput with two sinusoids plus uniform noise. */
static FftInput make_two_tones(uint8_t bin1, int16_t amp1,
                                uint8_t bin2, int16_t amp2,
                                int16_t noise_amp) {
    FftInput input;
    for (uint8_t n = 0; n < FFT_N; n++) {
        double  sig   = amp1 * sin(2.0 * M_PI * bin1 * n / FFT_N)
                      + amp2 * sin(2.0 * M_PI * bin2 * n / FFT_N);
        int16_t noise = (int16_t)((rand() % (2 * noise_amp + 1)) - noise_amp);
        input.samples[n] = (SensorSample)((int16_t)(sig + noise));
    }
    return input;
}

/* Returns the index of the bin with the highest magnitude. */
static uint8_t peak_bin(FftResult result) {
    uint8_t peak = 0;
    for (uint8_t k = 1; k < FFT_BINS; k++) {
        if (result.bins[k] > result.bins[peak]) peak = k;
    }
    return peak;
}

/* A1: all samples zero → all bins zero; status OK. */
static void test_fft_compute_zero_input(void) {
    /* Setup */
    FftInput input;
    for (uint8_t n = 0; n < FFT_N; n++) input.samples[n] = 0;

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_OK);
    for (uint8_t k = 0; k < FFT_BINS; k++) {
        CHECK(result.bins[k] == 0);
    }
}

/* B1: noisy tone at bin 1 — lowest non-zero frequency (150 Hz), 0 dB SNR. */
static void test_fft_compute_noisy_tone_bin1(void) {
    /* Setup */
    srand(42);
    FftInput input = make_tone(1, 800, 800);

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_OK);
    CHECK(peak_bin(result) == 1);
}

/* B2: noisy tone at bin 31 — Nyquist boundary (4650 Hz), 0 dB SNR. */
static void test_fft_compute_noisy_tone_bin31(void) {
    /* Setup */
    srand(42);
    FftInput input = make_tone(31, 800, 800);

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_OK);
    CHECK(peak_bin(result) == 31);
}

/* C1: noisy impulse at Hann window centre (n=32) → approximately flat spectrum.
 * All bins should be non-zero and within a factor of 5 of each other. */
static void test_fft_compute_noisy_impulse_flat_spectrum(void) {
    /* Setup */
    srand(42);
    FftInput input;
    input.samples[32] = 1023;
    for (uint8_t n = 0; n < FFT_N; n++) {
        if (n != 32) input.samples[n] = (SensorSample)((rand() % 401) - 200);
    }

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_OK);
    FftBin min_b = result.bins[0], max_b = result.bins[0];
    for (uint8_t k = 1; k < FFT_BINS; k++) {
        if (result.bins[k] < min_b) min_b = result.bins[k];
        if (result.bins[k] > max_b) max_b = result.bins[k];
    }
    CHECK(min_b > 0);
    CHECK(max_b < (FftBin)(5 * min_b));
}

/* D1: sample = -32768 — the only value q15_make rejects → FFT_STATUS_INVALID. */
static void test_fft_compute_invalid_sample(void) {
    /* Setup */
    FftInput input;
    for (uint8_t n = 0; n < FFT_N; n++) input.samples[n] = 0;
    input.samples[0] = (SensorSample)(-32768);

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_INVALID);
}

/* D2: sample = -32767 — boundary just inside valid Q15 range → FFT_STATUS_OK. */
static void test_fft_compute_boundary_sample(void) {
    /* Setup */
    FftInput input;
    for (uint8_t n = 0; n < FFT_N; n++) input.samples[n] = 0;
    input.samples[0] = (SensorSample)(-32767);

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_OK);
}

/* E1: dominant at bin 4 (600 Hz) + secondary at bin 5 (750 Hz) + noise ±100.
 * 1-bin separation (150 Hz) — minimum resolvable separation for N=64. */
static void test_fft_compute_two_tones_dominant_low(void) {
    /* Setup */
    srand(42);
    FftInput input = make_two_tones(4, 600, 5, 150, 100);

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_OK);
    CHECK(peak_bin(result) == 4);
    CHECK(result.bins[4] > result.bins[5]);
    CHECK(result.bins[5] > 0);
}

/* E2: dominant at bin 20 (3000 Hz) + secondary at bin 19 (2850 Hz) + noise ±100.
 * 1-bin separation (150 Hz) — minimum resolvable separation for N=64. */
static void test_fft_compute_two_tones_dominant_high(void) {
    /* Setup */
    srand(42);
    FftInput input = make_two_tones(20, 600, 19, 150, 100);

    /* Run */
    FftResult result = fft_compute(input);

    /* Assert */
    CHECK(result.status == FftStatus::FFT_STATUS_OK);
    CHECK(peak_bin(result) == 20);
    CHECK(result.bins[20] > result.bins[19]);
    CHECK(result.bins[19] > 0);
}

int main(void) {
    printf("fft tests\n");
    RUN_TEST(test_fft_compute_zero_input);
    RUN_TEST(test_fft_compute_noisy_tone_bin1);
    RUN_TEST(test_fft_compute_noisy_tone_bin31);
    RUN_TEST(test_fft_compute_noisy_impulse_flat_spectrum);
    RUN_TEST(test_fft_compute_invalid_sample);
    RUN_TEST(test_fft_compute_boundary_sample);
    RUN_TEST(test_fft_compute_two_tones_dominant_low);
    RUN_TEST(test_fft_compute_two_tones_dominant_high);
    printf("%s\n", s_failures == 0 ? "TESTS PASSED" : "TESTS FAILED");
    return s_failures == 0 ? 0 : 1;
}
