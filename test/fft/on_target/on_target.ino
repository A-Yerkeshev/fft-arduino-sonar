/* On-target integration test for the fft module.
 * Reads one audio frame per loop from the sound sensor (pin A2 via hal_adc),
 * computes the FFT, and reports all 32 bin magnitudes over Serial.
 *
 * This exercises the full hal_adc → fft pipeline on real hardware and verifies:
 *   - pgm_read_word() reads PROGMEM twiddle/Hann tables via AVR LPM instruction
 *   - fft_compute() produces a plausible spectrum from live microphone input
 *   - Execution time fits within the frame period (~6.6 ms acquisition + FFT)
 *
 * Expected output at 9600 baud:
 *   fft on-target
 *   free SRAM: NNN bytes
 *   --- (one block per frame, ~2 per second) ---
 *   bin[ 0]   0 Hz  mag=  0
 *   bin[ 1] 150 Hz  mag= 87
 *   ...
 *   bin[31] 4650 Hz  mag=  3
 *   fft_compute: NNN us
 */

#include "hal_adc.h"
#include "fft.h"

/* Standard AVR idiom: distance between top of heap and bottom of stack. */
static int free_ram(void) {
    extern int __heap_start, *__brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

/* Fills FftInput from an AdcFrame. Both hold AUDIO_FRAME_SIZE == FFT_N samples. */
static FftInput frame_to_fft_input(AdcFrame frame) {
    FftInput input;
    for (uint8_t n = 0; n < FFT_N; n++) {
        input.samples[n] = frame.samples[n];
    }
    return input;
}

/* Prints all FFT_BINS bins with their frequency in Hz and magnitude.
 * Bin k → frequency k × 150 Hz (sample_rate ≈ 9615 Hz, FFT_N = 64). */
static void report_all_bins(FftResult result) {
    for (uint8_t k = 0; k < FFT_BINS; k++) {
        uint16_t freq_hz = (uint16_t)(k * 150u);
        Serial.print("bin[");
        if (k < 10) Serial.print(" ");
        Serial.print(k);
        Serial.print("] ");
        if (freq_hz < 1000) Serial.print(" ");
        Serial.print(freq_hz);
        Serial.print(" Hz  mag=");
        Serial.println(result.bins[k]);
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("fft on-target");
    Serial.print("free SRAM: ");
    Serial.print(free_ram());
    Serial.println(" bytes");
    hal_adc_init();
}

void loop() {
    AdcFrame  frame   = hal_adc_read();
    FftInput  input   = frame_to_fft_input(frame);

    uint32_t  t0      = micros();
    FftResult result  = fft_compute(input);
    uint32_t  elapsed = micros() - t0;

    Serial.println("---");
    if (result.status != FftStatus::FFT_STATUS_OK) {
        Serial.println("ERROR: fft_compute failed");
    } else {
        report_all_bins(result);
    }
    Serial.print("fft_compute: ");
    Serial.print(elapsed);
    Serial.println(" us");

    delay(5000);
}
