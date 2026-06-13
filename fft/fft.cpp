#include "fft.h"

/* Wrapper type for Q15 fixed-point values. Construct only via q15_make, which
 * rejects -32768 — the one value that causes overflow in Q15 multiplication. */
typedef struct { int16_t val; } Q15;

enum class Q15Status : uint8_t { Q15_STATUS_INVALID = 0, Q15_STATUS_OK };

/* Returned by q15_make — carries the validated Q15 value and construction status. */
typedef struct { Q15 val; Q15Status status; } Q15Result;

/* Validates that raw is within the safe Q15 range [-32767, 32767].
 * -32768 is the only int16_t value that overflows Q15 multiplication:
 * (-32768)*(-32768)>>15 = 32768, which exceeds int16_t max of 32767. */
static Q15Result q15_make(int16_t raw) {
    Q15Result result;
    result.status = Q15Status::Q15_STATUS_INVALID;
    if (raw < -32767) { return result; }
    result.val.val = raw;
    result.status = Q15Status::Q15_STATUS_OK;
    return result;
}

static_assert(FFT_N == AUDIO_FRAME_SIZE, "FFT transform size must equal ADC frame size");

/* Each butterfly stage rotates complex values by a frequency-specific angle to
 * separate the signal into individual frequency components. These tables hold
 * the cos and -sin of those rotation angles (2πk/64 for k=0..31), pre-computed
 * as Q15 integers so no floating-point is needed at runtime.
 * Stored in flash (PROGMEM) to preserve SRAM. Read via pgm_read_word(). */
static const Q15 TWIDDLE_COS[32] PROGMEM = {
    { 32767}, { 32609}, { 32137}, { 31356}, { 30273}, { 28898}, { 27245}, { 25329},
    { 23170}, { 20787}, { 18204}, { 15446}, { 12539}, {  9512}, {  6393}, {  3212},
    {     0}, { -3212}, { -6393}, { -9512}, {-12539}, {-15446}, {-18204}, {-20787},
    {-23170}, {-25329}, {-27245}, {-28898}, {-30273}, {-31356}, {-32137}, {-32609}
};

static const Q15 TWIDDLE_SIN[32] PROGMEM = {
    {     0}, { -3212}, { -6393}, { -9512}, {-12539}, {-15446}, {-18204}, {-20787},
    {-23170}, {-25329}, {-27245}, {-28898}, {-30273}, {-31356}, {-32137}, {-32609},
    {-32767}, {-32609}, {-32137}, {-31356}, {-30273}, {-28898}, {-27245}, {-25329},
    {-23170}, {-20787}, {-18204}, {-15446}, {-12539}, { -9512}, { -6393}, { -3212}
};

/* Without windowing, the FFT assumes the signal repeats perfectly at the frame
 * boundary — which it never does in practice, causing smeared frequency peaks.
 * Multiplying each sample by a Hann weight (bell-shaped, zero at both ends)
 * smooths the boundary and produces sharper, more accurate frequency peaks.
 * Pre-computed as Q15 integers; stored in flash (PROGMEM) to preserve SRAM. */
static const Q15 HANN[64] PROGMEM = {
    {     0}, {    79}, {   315}, {   705}, {  1247}, {  1935}, {  2761}, {  3719},
    {  4799}, {  5990}, {  7281}, {  8660}, { 10114}, { 11628}, { 13187}, { 14778},
    { 16383}, { 17989}, { 19580}, { 21139}, { 22653}, { 24107}, { 25486}, { 26777},
    { 27968}, { 29048}, { 30006}, { 30832}, { 31520}, { 32062}, { 32452}, { 32688},
    { 32767}, { 32688}, { 32452}, { 32062}, { 31520}, { 30832}, { 30006}, { 29048},
    { 27968}, { 26777}, { 25486}, { 24107}, { 22653}, { 21139}, { 19580}, { 17989},
    { 16384}, { 14778}, { 13187}, { 11628}, { 10114}, {  8660}, {  7281}, {  5990},
    {  4799}, {  3719}, {  2761}, {  1935}, {  1247}, {   705}, {   315}, {    79}
};

static Q15  s_re[FFT_N]; /* real part buffer; written per fft_compute call */
static Q15  s_im[FFT_N]; /* imaginary part buffer; zero-initialised each call */
/* Multiplies two Q15 values; right-shifts 15 to return the result to Q15 range.
 * Result is mathematically bounded to [-32766, 32766]: max product 32767*32767>>15 = 32766,
 * so the output is always a valid Q15 and needs no factory construction. */
static Q15 q15_mul(Q15 a, Q15 b) {
    return {(int16_t)((int32_t)a.val * (int32_t)b.val >> 15)};
}

typedef struct { Q15 re_u; Q15 im_u; Q15 re_v; Q15 im_v; } ButterflyOutput;

/* A butterfly takes two complex values u and v, combines them with a twiddle
 * factor w (a frequency-specific rotation), and writes two new values back
 * in their place. Running N/2 butterflies across the array once is one stage;
 * after six stages each slot holds energy for one specific frequency.
 * tr/ti are int32_t because each is a difference of two Q15 values — the result
 * can reach ±65534, which overflows int16_t before the /2 is applied. */
static ButterflyOutput fft_butterfly(Q15 re_u, Q15 im_u, Q15 re_v, Q15 im_v, Q15 wr, Q15 wi) {
    int32_t tr = (int32_t)q15_mul(wr, re_v).val - (int32_t)q15_mul(wi, im_v).val;
    int32_t ti = (int32_t)q15_mul(wr, im_v).val + (int32_t)q15_mul(wi, re_v).val;
    ButterflyOutput out;
    out.re_u = {(int16_t)((re_u.val + tr) >> 1)};
    out.im_u = {(int16_t)((im_u.val + ti) >> 1)};
    out.re_v = {(int16_t)((re_u.val - tr) >> 1)};
    out.im_v = {(int16_t)((im_u.val - ti) >> 1)};
    return out;
}

/* Runs all 6 FFT stages. Each stage applies N/2 butterflies across re[]/im[]
 * with twiddle factors that rotate by increasing fractions of a full cycle,
 * progressively separating the signal into individual frequency components.
 * re[]/im[] are modified in-place; passing by value would require returning a
 * 256-byte struct (2 × 64 × int16_t), warranting pass-by-reference instead.
 * ASSUME: fft_bit_reverse has been called first; FFT_N is a power of 2 (64). */
static void fft_stages(Q15 re[], Q15 im[]) {
    for (uint8_t stage = 0; stage < 6; stage++) {
        uint8_t half_span = (uint8_t)(1 << stage);
        uint8_t stride    = (uint8_t)(half_span << 1);
        uint8_t tw_step   = (uint8_t)(FFT_N / 2 / half_span);
        for (uint8_t group = 0; group < FFT_N / stride; group++) {
            for (uint8_t pair = 0; pair < half_span; pair++) {
                uint8_t u = group * stride + pair, v = u + half_span;
                Q15 wr = {(int16_t)pgm_read_word(&TWIDDLE_COS[pair * tw_step].val)};
                Q15 wi = {(int16_t)pgm_read_word(&TWIDDLE_SIN[pair * tw_step].val)};
                ButterflyOutput b = fft_butterfly(re[u], im[u], re[v], im[v], wr, wi);
                re[u] = b.re_u; im[u] = b.im_u; re[v] = b.re_v; im[v] = b.im_v;
            }
        }
    }
}

/* Integer square root via binary search. Returns r such that r² ≤ n < (r+1)².
 * hi = 65535 covers the full uint32_t input range — no assumption on the caller
 * is needed. Deterministic: at most 16 iterations (log2(65535)). */
static uint16_t isqrt32(uint32_t n) {
    if (n == 0) return 0;
    uint16_t lo = 1, hi = 65535;
    while (lo < hi) {
        uint16_t mid = (uint16_t)(lo + (hi - lo + 1) / 2);
        if ((uint32_t)mid * mid <= n) lo = mid;
        else hi = (uint16_t)(mid - 1);
    }
    return lo;
}

static bool s_busy = false; /* guards against re-entrant calls */

/* Reorders re[] and im[] from natural time order into the scrambled order that
 * the butterfly stages expect. Without this, each stage would need extra index
 * arithmetic to find its input pairs — doing it once upfront keeps the stages simple.
 * re[]/im[] are modified in-place; passing by value would require returning a
 * 256-byte struct (2 × 64 × int16_t), warranting pass-by-reference instead.
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
    for (uint8_t n = 0; n < FFT_N; n++) {
        Q15 hann     = {(int16_t)pgm_read_word(&HANN[n].val)};
        Q15Result sr = q15_make(input.samples[n]);
        if (sr.status != Q15Status::Q15_STATUS_OK) { s_busy = false; return result; }
        s_re[n] = q15_mul(hann, sr.val);
        s_im[n] = {0};
    }
    fft_bit_reverse(s_re, s_im);
    fft_stages(s_re, s_im);
    for (uint8_t k = 0; k < FFT_BINS; k++) {
        int32_t re = s_re[k].val, im = s_im[k].val;
        result.bins[k] = isqrt32((uint32_t)(re * re + im * im));
    }
    result.status = FftStatus::FFT_STATUS_OK;
    s_busy = false;
    return result;
}
