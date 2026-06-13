#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fft.h"

/* Performance tests — parameter sweeps that find the empirical accuracy
 * limits of the FFT algorithm. These print tables rather than asserting; they
 * describe what the algorithm can and cannot do at the boundary conditions. */

static FftInput make_tone(uint8_t bin, int16_t amplitude, int16_t noise_amp) {
    FftInput input;
    for (uint8_t n = 0; n < FFT_N; n++) {
        double   tone  = amplitude * sin(2.0 * M_PI * bin * n / FFT_N);
        int16_t  noise = (int16_t)((rand() % (2 * noise_amp + 1)) - noise_amp);
        input.samples[n] = (SensorSample)((int16_t)(tone + noise));
    }
    return input;
}

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

static uint8_t peak_bin(FftResult result) {
    uint8_t peak = 0;
    for (uint8_t k = 1; k < FFT_BINS; k++) {
        if (result.bins[k] > result.bins[peak]) peak = k;
    }
    return peak;
}

/* Sweep 1: SNR boundary for single-tone detection.
 * Fix tone at bin 8 (1200 Hz), amplitude 800. Increase noise until detection fails.
 * Reports the noise level at which peak_bin first stops returning bin 8. */
static void sweep_snr_boundary(void) {
    printf("\n--- Sweep 1: SNR boundary (tone bin 8, amplitude 800) ---\n");
    printf("  noise_amp | peak_bin | detected\n");
    printf("  ----------+----------+---------\n");
    for (int16_t noise = 50; noise <= 800; noise = (int16_t)(noise + 50)) {
        srand(42);
        FftInput  input  = make_tone(8, 800, noise);
        FftResult result = fft_compute(input);
        uint8_t   peak   = peak_bin(result);
        printf("  %9d | %8d | %s\n", noise, peak, peak == 8 ? "yes" : "NO");
    }
}

/* Sweep 2: Minimum frequency separation for two-tone discrimination.
 * Fix dominant at bin 16 (2400 Hz), amplitude 600; secondary amplitude 150; noise ±100.
 * Decrease secondary bin from 15 toward 1 — find where dominant detection breaks down. */
static void sweep_frequency_separation(void) {
    printf("\n--- Sweep 2: Frequency separation (dominant bin 16, secondary amp 150, noise ±100) ---\n");
    printf("  sec_bin | sep_bins | sep_Hz | peak_bin | detected\n");
    printf("  --------+----------+--------+----------+---------\n");
    for (int8_t sec = 15; sec >= 1; sec--) {
        srand(42);
        uint8_t   sep_bins = (uint8_t)(16 - sec);
        uint16_t  sep_hz   = (uint16_t)(sep_bins * 150u);
        FftInput  input    = make_two_tones(16, 600, (uint8_t)sec, 150, 100);
        FftResult result   = fft_compute(input);
        uint8_t   peak     = peak_bin(result);
        printf("  %7d | %8d | %6u | %8d | %s\n",
               sec, sep_bins, sep_hz, peak, peak == 16 ? "yes" : "NO");
    }
}

/* Sweep 3: Minimum amplitude ratio for dominant detection.
 * Fix bins 4 (600 Hz) and 20 (3000 Hz); dominant amplitude 600; noise ±100.
 * Increase secondary amplitude from 100 toward 550 — find where ranking flips. */
static void sweep_amplitude_ratio(void) {
    printf("\n--- Sweep 3: Amplitude ratio (dominant bin 4 amp 600, secondary bin 20, noise ±100) ---\n");
    printf("  sec_amp | ratio (dom:sec) | peak_bin | detected\n");
    printf("  --------+----------------+----------+---------\n");
    for (int16_t sec_amp = 100; sec_amp <= 590; sec_amp = (int16_t)(sec_amp + 50)) {
        srand(42);
        FftInput  input  = make_two_tones(4, 600, 20, sec_amp, 100);
        FftResult result = fft_compute(input);
        uint8_t   peak   = peak_bin(result);
        printf("  %7d | %14.2f | %8d | %s\n",
               sec_amp, 600.0 / sec_amp, peak, peak == 4 ? "yes" : "NO");
    }
}

int main(void) {
    printf("fft performance\n");
    sweep_snr_boundary();
    sweep_frequency_separation();
    sweep_amplitude_ratio();
    printf("\ndone\n");
    return 0;
}
