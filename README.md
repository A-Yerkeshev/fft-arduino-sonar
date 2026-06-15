# FFT Audio Analyzer — Arduino UNO

A real-time audio frequency analyzer running on an Arduino UNO microcontroller.
It listens to ambient sound through a microphone, identifies which frequencies are
present, and reports them over a USB serial connection — all on a device with 2 KB
of working memory and no operating system.

---

## What It Does

The device continuously captures short bursts of audio (64 samples, ~7 ms each),
runs a mathematical algorithm called the **Fast Fourier Transform (FFT)** to break
the sound into its constituent frequencies, and prints the result to a connected
computer. Think of it as a software equivalent of a graphic equalizer display.

The output covers frequencies from 150 Hz to 4 650 Hz in 32 bands, each 150 Hz wide.

---

## Hardware

| Component    | Connection |
|--------------|------------|
| Sound sensor | Pin A2     |
| Output       | USB Serial, 9600 baud |

---

## How the Code is Organised

The project is split into three independent modules, each with a single
responsibility. Modules communicate only through well-defined shared types — they
cannot reach into each other's internals.

```
Sound sensor (A2)
      │
   hal_adc     Reads 64 audio samples; removes the DC offset so the
      │         signal oscillates around zero
   fft          Applies a Hann window (to reduce noise artefacts) then
      │         runs the FFT; outputs 32 frequency magnitudes
   output       (planned) Formats and emits results over serial
```

A small header file (`decoupling_enforcer.h`) causes a compile-time error if any
module accidentally imports another module it should not know about. This acts as
an automated guard against tangled dependencies.

---

## Engineering Practices

**No dynamic memory allocation** — the microcontroller has only 2 KB of RAM. Memory
allocated at runtime (via `malloc` or C++ `new`) can fragment and cause unpredictable
failures. Every buffer in this project is declared at compile time with a fixed size.

**Integer-only arithmetic in time-critical paths** — the AVR chip has no
floating-point unit. All FFT calculations use a fixed-point number format (Q15:
integers that represent fractions by implicit scaling) so arithmetic stays fast and
deterministic.

**Lookup tables in flash** — the FFT requires 64 pre-computed trigonometric
coefficients. Storing them in program flash (using the AVR `PROGMEM` attribute)
instead of RAM saves 256 bytes of the precious 2 KB working memory.

**Explicit error handling** — every function that can fail returns a typed status
code. Callers must check it. There are no silent failures.

**Strong typing** — every distinct physical quantity (audio sample, FFT bin
magnitude, ADC count) has its own named type. The compiler rejects accidental
mix-ups between quantities.

**Hardware abstraction** — all ADC hardware calls (`analogRead`) are isolated in
`hal_adc.cpp`. The FFT algorithm and any other logic modules have no hardware
dependency, making them fully testable on a standard laptop.

---

## Testing

The project has two complementary layers of testing.

**Off-target tests** run on the development laptop (Linux, standard C++ compiler)
without any hardware connected. Hardware calls are replaced with simple stubs.
This allows the algorithm to be verified quickly and repeatedly during development.

**On-target tests** run the full pipeline on the actual Arduino UNO board using a
real microphone. This catches any differences between the laptop compiler and the
AVR compiler, and verifies that flash memory reads (`PROGMEM`) work correctly on
real hardware.

Test cases are designed using **equivalence partitioning** (grouping inputs that
should behave identically and testing one from each group) and **boundary value
analysis** (testing the exact edges of valid input ranges). Tests use the most
adverse conditions the algorithm can reliably handle rather than comfortable
margins — a harder test is strictly more valuable than an easy one.

A separate set of **performance sweep tests** empirically measure the algorithm's
accuracy limits by varying signal strength, noise level, and how close two
frequencies can be before they become indistinguishable.

```
test/
  fft/
    off_target/   unit tests + performance sweeps  →  make test / make perf
    on_target/    live microphone → FFT → Serial (Arduino sketch)
  hal_adc/
    off_target/   unit tests  →  make test
    on_target/    prints min/max sample per frame (Arduino sketch)
```

---

## How AI Was Used

This project was developed collaboratively with **Claude** (Anthropic), used as a
pair programmer and subject-matter resource throughout.

The process was strictly developer-led. At every step, the AI presented options
with their trade-offs; the developer made all decisions. Code was written in small
chunks (approximately 10 lines at a time) and reviewed before work continued.
No code was written autonomously.

Specific AI contributions:
- Explaining FFT theory (what butterflies and twiddle factors are, why windowing matters)
  in plain terms
- Identifying a subtle integer overflow edge case in the fixed-point multiplication
  and proposing the factory-function solution
- Generating repetitive boilerplate (pre-computed coefficient tables, test scaffolding)
- Suggesting the performance-sweep approach to empirically characterise accuracy limits

All architectural decisions — module structure, data types, algorithm parameters,
and testing strategy — were made by the developer.

---

## Running

**Off-target tests (no hardware needed):**
```sh
cd test/fft/off_target  && make test    # 8 unit tests
cd test/fft/off_target  && make perf    # accuracy characterisation sweeps
cd test/hal_adc/off_target && make test # 5 unit tests
```

**On-target (Arduino UNO):**
Open `test/fft/on_target/` in the Arduino IDE and upload to the board.
Open the Serial Monitor at 9600 baud. Each line shows one frequency band,
its centre frequency in Hz, and its current magnitude. The execution time
of the FFT is reported after each frame.
