/* Ensures that modules do not depend on each other
 * directly. Instead, they should be coupled through
 * an interface. Fails compilation if tight-coupling
 * is detected.
 * Included by every module header via #include "../decoupling_enforcer.h".
 * Each module header defines <MODULE>_HEADER before including this file.
 * Each module Makefile defines -DCOMPILING_<MODULE>.
 * To add a new rule: add one #if block below.
 */

/* fft must not include hal_adc.h */
#if defined(HAL_ADC_HEADER) && defined(COMPILING_FFT)
#error "Module boundary violation: fft must not include hal_adc.h — use interfaces/adc_fft.h"
#endif

/* hal_adc must not include fft.h */
#if defined(FFT_HEADER) && defined(COMPILING_HAL_ADC)
#error "Module boundary violation: hal_adc must not include fft.h"
#endif
