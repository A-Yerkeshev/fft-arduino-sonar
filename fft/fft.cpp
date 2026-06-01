#include "fft.h"

static_assert(FFT_N == AUDIO_FRAME_SIZE, "FFT transform size must equal ADC frame size");

static bool s_busy = false; /* guards against re-entrant calls */

FftResult fft_compute(FftInput input) {
    FftResult result;
    result.status = FftStatus::FFT_STATUS_INVALID;
    if (s_busy) { result.status = FftStatus::FFT_STATUS_REENTRANT; return result; }
    s_busy = true;
    (void)input;
    s_busy = false;
    return result;
}
