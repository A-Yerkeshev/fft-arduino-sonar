#include "fft.h"

static_assert(FFT_N == AUDIO_FRAME_SIZE, "FFT transform size must equal ADC frame size");

/* Each butterfly stage rotates complex values by a frequency-specific angle to
 * separate the signal into individual frequency components. These tables hold
 * the cos and -sin of those rotation angles (2πk/64 for k=0..31), pre-computed
 * as Q15 integers so no floating-point is needed at runtime.
 * Stored in flash (PROGMEM) to preserve SRAM. Read via pgm_read_word(). */
static const Q15 TWIDDLE_COS[32] PROGMEM = {
     32767,  32609,  32137,  31356,  30273,  28898,  27245,  25329,
     23170,  20787,  18204,  15446,  12539,   9512,   6393,   3212,
         0,  -3212,  -6393,  -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609
};

static const Q15 TWIDDLE_SIN[32] PROGMEM = {
         0,  -3212,  -6393,  -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539,  -9512,  -6393,  -3212
};

/* Without windowing, the FFT assumes the signal repeats perfectly at the frame
 * boundary — which it never does in practice, causing smeared frequency peaks.
 * Multiplying each sample by a Hann weight (bell-shaped, zero at both ends)
 * smooths the boundary and produces sharper, more accurate frequency peaks.
 * Pre-computed as Q15 integers; stored in flash (PROGMEM) to preserve SRAM. */
static const Q15 HANN[64] PROGMEM = {
         0,     79,    315,    705,   1247,   1935,   2761,   3719,
      4799,   5990,   7281,   8660,  10114,  11628,  13187,  14778,
     16383,  17989,  19580,  21139,  22653,  24107,  25486,  26777,
     27968,  29048,  30006,  30832,  31520,  32062,  32452,  32688,
     32767,  32688,  32452,  32062,  31520,  30832,  30006,  29048,
     27968,  26777,  25486,  24107,  22653,  21139,  19580,  17989,
     16384,  14778,  13187,  11628,  10114,   8660,   7281,   5990,
      4799,   3719,   2761,   1935,   1247,    705,    315,      79
};

static Q15  s_re[FFT_N]; /* real part buffer; written per fft_compute call */
static Q15  s_im[FFT_N]; /* imaginary part buffer; zero-initialised each call */
static bool s_busy = false; /* guards against re-entrant calls */

/* Reorders re[] and im[] from natural time order into the scrambled order that
 * the butterfly stages expect. Without this, each stage would need extra index
 * arithmetic to find its input pairs — doing it once upfront keeps the stages simple.
 * ASSUME: FFT_N is a power of 2 (64) */
static void fft_bit_reverse(Q15 re[], Q15 im[]) {
    uint8_t j = 0;
    for (uint8_t i = 1; i < FFT_N; i++) {
        uint8_t bit = FFT_N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            Q15 tmp;
            tmp = re[i]; re[i] = re[j]; re[j] = tmp;
            tmp = im[i]; im[i] = im[j]; im[j] = tmp;
        }
    }
}

FftResult fft_compute(FftInput input) {
    FftResult result;
    result.status = FftStatus::FFT_STATUS_INVALID;
    if (s_busy) { result.status = FftStatus::FFT_STATUS_REENTRANT; return result; }
    s_busy = true;
    (void)input;
    fft_bit_reverse(s_re, s_im);
    s_busy = false;
    return result;
}
