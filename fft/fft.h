#pragma once
#define FFT_HEADER
#include "../decoupling_enforcer.h"
#include <stdint.h>
#include "../interfaces/adc_fft.h"

/* MODULE: fft
 * PURPOSE: Compute the magnitude spectrum of a fixed-size sample buffer
 *          using a Hann-windowed radix-2 DIT FFT
 * DEPENDS ON: interfaces/adc_fft.h
 */

/* --- Types --- */

/* Q15 fixed-point: [-1.0, 1.0) mapped to [-32768, 32767].
 * Used internally for twiddle factors and Hann coefficients only. */
typedef int16_t Q15;

/* Magnitude at one frequency bin: isqrt32(re^2 + im^2). */
typedef uint16_t FftBin;

/* --- Constants --- */

#define FFT_N    ((uint8_t) 64) /* transform size; must be power of 2; == AUDIO_FRAME_SIZE */
#define FFT_BINS ((uint8_t) 32) /* usable output bins (positive frequencies only) */

/* --- Status --- */

enum class FftStatus : uint8_t {
    FFT_STATUS_INVALID   = 0,
    FFT_STATUS_OK,
    FFT_STATUS_REENTRANT, /* fft_compute called while already running */
};

/* --- Aggregates --- */

/* Input to fft_compute. Passed by value — fft owns its copy of the samples. */
typedef struct {
    SensorSample samples[FFT_N]; /* DC-centered, range [-1023, +1023] */
} FftInput;

/* Output of fft_compute. Returned by value — caller owns the result. */
typedef struct {
    FftBin    bins[FFT_BINS]; /* magnitude at each frequency bin */
    FftStatus status;
} FftResult;

/* --- Functions --- */

/* PURPOSE: Apply Hann window then compute radix-2 DIT FFT magnitude spectrum of
 *          input.samples; return FFT_BINS magnitude values by value.
 * PARAMS:  input — FftInput containing FFT_N SensorSample values, DC-centered,
 *                  range [-1023, +1023]; mean removed by HAL
 * RETURNS: FftResult — .bins contains magnitude per bin; .status is FftStatus
 * EFFECTS: writes to module-static re[]/im[] buffers
 * ASSUMES: input.samples is DC-centered (unverifiable at runtime)
 * GUARDS:  AUDIO_FRAME_SIZE == FFT_N (static_assert in fft.cpp)
 *          no re-entrancy (runtime flag; returns FFT_STATUS_REENTRANT if violated)
 */
FftResult fft_compute(FftInput input);
