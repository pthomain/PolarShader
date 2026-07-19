# Signals

A **signal** is a time-varying scalar in `[-1, 1]` that drives a parameter — a transform's rotation speed,
a pattern's scale, and so on. Signals are what make a scene move. All animation goes through them, so
motion stays explicit and deterministic (see [Core Concepts](Core-Concepts.md)).

Sources live in `src/renderer/pipeline/signals/Signals.h`.

## Output contract

- Every signal emits values in `[-1, 1]`.
- Periodic factories span the full `[-1, 1]` by default.
- Use **`smap(signal, floor, ceiling)`** to remap a signal into a bounded interval before it's converted
  back to signed `sf16`. `floor`/`ceiling` are themselves sampled through the unsigned magnitude domain;
  if the sampled floor exceeds the ceiling, the bounds are swapped for that sample.

## Periodic factories

`sine`, `noise`, `triangle`, `square`, `sawtooth`.

- Base signature: `(phaseVelocity, phaseOffset)`.
- Bounded overloads: `(phaseVelocity, floor, ceiling)` and `(phaseVelocity, phaseOffset, floor, ceiling)`
  — these apply `smap()` internally.
- `phaseVelocity` is sampled through the magnitude domain, where `constant(1000)` = 1 Hz and
  `constant(500)` = 0.5 Hz (a 1 Hz full-scale maximum).
- `phaseOffset` is a signed turn offset wrapped into the phase domain.

## Aperiodic factories

`linear`, `quadraticIn`, `quadraticOut`, `quadraticInOut`.

- Shared signature: `(duration, loopMode)`.
- The waveform receives a relative time derived from `duration` and `LoopMode`.
  - `LoopMode::RESET` wraps by `elapsedMs % duration`.
  - `duration == 0` emits `0`.

## Other factories

- **`constant`** — a static value (parameterized in permille, 0–1000).
- **`cRandom`** — seeded random (no period).

## Phase vs angle, motion as integration

Signals feed integrators rather than being applied directly frame-by-frame:

- **`f16` angle samples** are wrapped turn-domain angles (`0..65535 → 0..1` turn).
- **`PhaseAccumulator`** holds a higher-precision integrated phase for smooth periodic motion, avoiding
  quantization jitter and hidden precision loss.

Integrating phase (instead of recomputing angles each frame) gives smooth motion under variable `dt` and
correct wrap semantics. See [Core Concepts › Phase vs angle](Core-Concepts.md#phase-vs-angle-and-motion-as-integration).
