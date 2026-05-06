# Digital Synthesizer Firmware Documentation

STM32-based monophonic bass synthesizer with a 16-step sequencer, analog-modeled filter, and SPI DAC output. The firmware is structured around two operating modes — **PROG** (programming) and **PLAY** — toggled by a dedicated button.

---

## Architecture Overview

```
ADC (5 knobs, DMA) ──► main loop
                            │
               ┌────────────┴────────────┐
            PLAY mode                PROG mode
               │                         │
    advance_sequencer()         display_sequencer()
    compute_next()              display_options()
    filter()                         │
    send_to_dac()              OLED via I2C
               │
         SPI DAC (DMA)
```

The audio engine runs entirely in the main loop, gated by `process_flag`. The flag is set at a fixed sample rate by TIM2's period-elapsed interrupt, giving a consistent ~22 kHz tick. GPIO interrupts handle all user input (joystick + buttons) with 200 ms software debounce.

---

## Peripherals

| Peripheral | Role |
|------------|------|
| **TIM2** | Audio tick interrupt — sets `process_flag` at sample rate |
| **TIM3** | ADC trigger (10 Hz via TRGO) |
| **ADC1** | 5-channel scan (DMA): cutoff, resonance, sweep, sweep rate, BPM |
| **SPI1** | 16-bit DAC output (DMA, software CS on PA6) |
| **I2C1** | SSD1306 OLED display (100 kHz) |
| **GPIOB** | Joystick (5 directions) + Octave + Accent/Slide buttons |

### ADC Channel Map

| Rank | Channel | Parameter |
|------|---------|-----------|
| 1 | ADC_CH9 | Filter cutoff (`a`) |
| 2 | ADC_CH4 | Filter resonance (`k`) |
| 3 | ADC_CH3 | Sweep depth |
| 4 | ADC_CH2 | Sweep rate |
| 5 | ADC_CH0 | BPM (read in PROG mode only) |

BPM is mapped as: `bpm = 150 - 90 * (adc[4] / 4095)`, giving a range of **60–150 BPM**.

---

## Audio Engine (`sound.c`)

### `init_buffer`

```c
void init_buffer(volatile struct buffer *buf, float freq);
```

Resets all synthesis state: phase, envelope, filter stages, sweep, and slide. Called on startup and whenever switching from PLAY back to PROG to ensure a clean restart.

---

### `compute_next`

```c
void compute_next(volatile struct buffer *buf, uint8_t env_start,
                  float sweep, float sweep_rate, float decay_rate,
                  float attack_rate, char shape, uint8_t sub);
```

Generates one audio sample. Called once per TIM2 tick in PLAY mode.

**Oscillator**

Phase accumulates at `freq / SAMPLE_RATE` per tick. Three waveforms are available, selected by `shape`:

| `shape` | Waveform |
|---------|----------|
| `'t'` | Sawtooth |
| `'s'` | Square |
| `'c'` | Combo (sawtooth + square, 90° offset, averaged) | 

**Sub-oscillator**

When `sub` is enabled, a square wave one octave below (`sub_state` flips each full cycle) is mixed in at 30% level:

```
raw_sig = 0.7 * osc + 0.3 * sub_osc
```

**Envelope**

Two-stage AR envelope (no sustain):

- **ATTACK**: `env_level += attack_rate` each sample until it reaches 1.0
- **DECAY**: `env_level *= decay_rate` each sample (exponential decay)

The envelope is retriggered by `advance_sequencer` setting `buf->state = ATTACK`.

**Frequency slide**

`current_freq` tracks `freq` via a one-pole lowpass: `current_freq += (freq - current_freq) * slide`. The `slide` coefficient is set per-step by the sequencer (0.001 for legato slides, 0.05 for normal transitions).

**Output**

```
curr_sample = OFFS + (AMP * env_level * raw_sig)
```

`OFFS` and `AMP` center and scale the signal for the DAC's unipolar input range.

---

### `filter`

```c
void filter(volatile struct buffer *buf, float a, float k);
```

A 3-pole lowpass ladder filter with resonance feedback, modeled after the Moog topology.

**Parameters**

- `a` — cutoff coefficient, clamped to [0, 0.5]. Mapped from ADC plus the current sweep value, so the filter opens as the sweep progresses.
- `k` — resonance, [0, 10]. Values approaching 10 push the filter toward self-oscillation.

**Signal path**

1. Normalize `curr_sample` to [-1, 1].
2. Extract a high-pass filtered feedback signal from `stage[2]` (prevents DC buildup in the resonance loop).
3. Apply input compensation (`1 + k * 0.5`) to maintain perceived volume at high resonance.
4. Soft-clip the feedback-summed input with a cubic waveshaper (`x * (1.5 - 0.5 * x²)`).
5. Run three cascaded one-pole lowpass stages.
6. Write `stage[2]` back to `curr_sample` (scaled to DAC range).

---

### `send_to_dac`

```c
void send_to_dac(SPI_HandleTypeDef *hspi, GPIO_TypeDef *GPIOx,
                 uint16_t GPIO_Pin, float sample);
```

Converts `sample` (in DAC units) to a 12-bit value, prepends the MCP4921-compatible control word `0x3000`, pulls CS low, and fires a DMA SPI transfer. CS is raised again in `HAL_SPI_TxCpltCallback` once the transfer completes.

The main loop checks `hspi1.State == HAL_SPI_STATE_READY` before calling this to avoid queueing a second transfer while one is in flight.

---

### `advance_sequencer`

```c
void advance_sequencer(volatile struct buffer *buf,
                       uint32_t *sample_counter,
                       uint32_t samples_per_step,
                       volatile uint16_t *note_index,
                       volatile uint16_t *octaves,
                       volatile uint16_t *slides,
                       volatile uint16_t *accents,
                       volatile uint16_t *sequence,
                       volatile uint16_t curr,
                       float sweep,
                       volatile int16_t active_pages);
```

Called every audio tick. Increments `sample_counter` and, when it reaches `samples_per_step`, advances to the next step.

`samples_per_step` is computed as:

```
samples_per_step = (SAMPLE_RATE * 60) / (bpm * 4)
```

Each step is a 16th note. At 120 BPM this is 5512 samples (~250 ms).

**Per-step logic**

- **Octave**: multiply the note frequency by 2 (up), 0.5 (down), or 1 (normal). A 1.6% correction factor (`1.016`) is applied to compensate for a slight hardware pitch offset.
- **Slide**: sets `buf->slide` to 0.001 (slow glide) if enabled, otherwise 0.05 (snappy transition).
- **Accent**: doubles the sweep depth for the current step, adding brightness.
- **Gate**: if `sequence[curr]` is non-zero the envelope is retriggered (`ATTACK`, `env_level = 0`) and `buf->sweep` is set. Steps with `sequence[curr] == 0` are rests — the envelope continues to decay uninterrupted.

The sequence wraps at `4 * active_pages` steps, so the effective sequence length is 4, 8, 12, or 16 steps depending on how many pages contain non-rest steps.

---

## Sequencer Data Model

The sequencer holds 16 steps, organized as 4 pages of 4 steps each. All arrays are 16 elements indexed as `step = cursor + 4 * page`.

| Array | Type | Meaning |
|-------|------|---------|
| `sequence[16]` | `uint16_t` | 0 = rest, 1 = active |
| `note_index[16]` | `uint16_t` | Index into `notes[]` table (0–11, chromatic) |
| `octaves[16]` | `int16_t` | -1 / 0 / 1 |
| `slides[16]` | `int16_t` | 0 = normal, 1 = slide |
| `accents[16]` | `int16_t` | 0 = normal, 1 = accent |

`active_pages` is recalculated each PROG loop by finding the highest non-rest step and deriving its page number.

---

## User Interface

### Modes

| Mode | Description |
|------|-------------|
| **PROG** | Sequencer/options editing. Audio engine is paused. |
| **PLAY** | Audio engine runs. ADC knobs are live. Display is not updated. |

Switched by `STATE_CHANGE_BTN`. Switching back to PROG resets `init_buffer` and `sample_counter`.

### PROG Menus

**OPTIONS** — global parameters:

- `options_cursor == 0`: oscillator shape (`t` / `s` / `c`), cycled with joystick up/down
- `options_cursor == 1`: sub-oscillator on/off, toggled with joystick up/down
- BPM is read live from ADC channel 5 (knob), displayed but not edited via joystick

Navigate right from OPTIONS to enter SEQUENCER. Navigate left from the first step of SEQUENCER to return to OPTIONS.

**SEQUENCER** — step editor:

- Joystick left/right moves `seq_cursor` (0–3) across steps; wraps to the next/previous page
- Joystick up/down increments/decrements `note_index` for the current step (wraps 0–11)
- Joystick center (push) toggles `sequence[step]` (rest / active)
- **OCTAVE_BTN**: cycles the step's octave through 0 → +1 → -1 → 0
- **ACCENT_SLIDE_BTN**: cycles through None → Slide → Accent → Slide+Accent → None

### Interrupt Debounce

All GPIO callbacks share a single `last_interrupt_time` timestamp and a 200 ms guard window. Only one button event is processed per 200 ms window, regardless of which pin triggered.

---

## Timing & Overrun Detection

If `process_flag` is still set when the next TIM2 interrupt fires, `overrun_count` is incremented. This indicates the main loop took longer than one sample period to process — a useful diagnostic for detecting compute budget overflows.

DMA interrupt priorities are set in `main()` after HAL initialization to ensure the audio DMA (Stream 0, priority 1) preempts the SPI DMA (Stream 2, priority 2).

---

## Build Notes

- FPU is explicitly enabled in software before `SystemClock_Config()` via `SCB->CPACR` and flush-to-zero / default-NaN FPSCR bits. This is required because STM32CubeIDE does not always generate the FPU enable code for F4 targets when HAL is initialized this early.
- The project targets an **STM32F4** running at **100 MHz** (HSI → PLL: M=8, N=100, P=2).
- All audio state lives in a single `volatile struct buffer` to simplify passing it across the ISR boundary.