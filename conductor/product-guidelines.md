# Product Guidelines

## Development Philosophy
- **Clarity & Composability:** Prioritize modular, readable code that reflects the "shader-like" philosophy. Components should be easy to understand in isolation and simple to combine into complex pipelines.
- **Deterministic Fixed-Point:** Every operation must be deterministic. Avoid floating-point arithmetic and dynamic memory allocation in the per-frame hot path.

## API Design
- **Fluent & Declarative:** Provide a builder-style API that allows users to declare the rendering pipeline in a readable, sequential manner.
- **Strong Typing:** Every core scalar or coordinate is wrapped in a `Strong<T, Tag>` template to prevent unit confusion and ensure math correctness at compile-time.
- **Balanced Configuration:** Support runtime modulation of parameters and signal sources, while maintaining a stable or semi-static pipeline structure to ensure performance.
- **Transform Encapsulation:** Public Transform APIs must never expose internal implementation types like `MappedSignal`. They must accept base Signal types and handle mapping/state privately.

## Strong Type System

| Type | Format | Definition | Usage |
| :--- | :--- | :--- | :--- |
| `f16` | Unsigned 0.16 | 16-bit integer [0, 1.0) | Angles (turns), alpha, scaling factors. |
| `sf16` | Signed 0.16 | 32-bit integer [-1.0, 1.0] | Signals, oscillators, trig results (sin/cos). |
| `fl::u16x16` | Unsigned 16.16 | 32-bit integer | Unsigned 2D/coordinate intermediates when needed. |
| `fl::s16x16` | Signed 16.16 | 32-bit integer | Spatial UV coordinates (allows tiling/zoom). |
| `PatternNormU16`| Unsigned 16-bit | [0, 65535] | Universal currency for pattern intensities. |
| `fl::s24x8` | Signed 24.8 | 32-bit integer | Internal lattice-aligned pattern calculations. |
| `fl::u24x8` | Unsigned 24.8 | 32-bit integer | Unsigned noise-domain sampling coordinates. |
| `UV` | 2x `fl::s16x16` | Normalized 2D vector | Standard spatial type passed between pipeline steps. |

`fl::u16x16` and `fl::u24x8` are both ratio/range types and are not bounded to `[0, 1]`; choose by domain:
- `fl::u16x16/fl::s16x16`: transform and UV composition where 16 fractional bits are needed.
- `fl::u24x8/fl::s24x8`: noise/lattice internals where 8 fractional bits are sufficient and grid alignment is primary.

### Rules
1. **No Implicit Casting:** Never cast between strong types or to raw integers without using the `raw()` helper.
2. **Semantic Boundaries:** Use `sf16` for any value that can go negative (signals), and `f16` for values that must wrap or stay positive (angles).
3. **Internal vs External:** `fl::s24x8` is an implementation detail for patterns; external APIs should only expose `UV`.
4. **`fl::*` Naming:** `fl::u16x16`/`fl::s16x16`/`fl::u24x8`/`fl::s24x8` denote ratio/range fixed-point values and are not implicitly constrained to `[0, 1]`.

## Signal & Timing Rules

### Timing Terminology
- **Elapsed Time (`TimeMillis elapsedMs`):** Canonical time source for scalar signal sampling.
- **Progress (`f16 progress`):** Scene-normalized progress used where mapped signal APIs require it.

### Signal Kinds
- **Periodic (`SignalKind::PERIODIC`):**
  - Factories: `sine`, `noise`.
  - Base signature must be: `(phaseVelocity, phaseOffset)`.
  - Bounded overloads may also accept `(phaseVelocity, floor, ceiling)` and `(phaseVelocity, phaseOffset, floor, ceiling)`.
  - Waveform `phaseVelocity` must be interpreted through the magnitude domain, where `constant(1000)` = 1 Hz and `constant(500)` = 0.5 Hz.
  - `phaseOffset` is a signed turn offset wrapped into the phase domain.
- **Aperiodic (`SignalKind::APERIODIC`):**
  - Factories: `linear`, `quadraticIn`, `quadraticOut`, `quadraticInOut`.
  - Signature must be: `(duration, loopMode)`.
  - `LoopMode::RESET` wraps relative time with modulo.
  - `duration == 0` emits `0`.

### Value Constraints
- **Normalization:** `Sf16Signal` public outputs are clamped to `[-1, 1]`.
- **Range Coverage:** Periodic samplers (`sine`, `noise`) must span the full signed range by default.
- **Bounded Modulation:** Use `smap(signal, floorSignal, ceilingSignal)` to constrain and rescale a signal inside dynamically sampled unipolar bounds.
  - `floorSignal` and `ceilingSignal` must be interpreted through `magnitudeRange()`.
  - If sampled bounds cross, they must be reordered for that sample before remapping.
- **Phase Rate:** Waveform phase accumulation must treat phase velocity as a unipolar magnitude with a 1 Hz full-scale maximum.

### Signal Responsibilities
- `Sf16Signal` wraps waveform evaluation and time routing (`PERIODIC` vs `APERIODIC`).
- Accumulation logic is separated into `SignalAccumulators.h` and applied explicitly (not via mapped-signal mode flags).
- Scalar signals are absolute by contract (no scalar `absolute`/relative mode).
- **Signed Convention:** Signals always emit signed values. Unsigned parameter domains must be produced by range mapping, not by signal factories.
- **Range Mapping Rule:** Signed ranges map directly from signal output. Unsigned ranges must first remap signal `[-1, 1] -> [0, 1]` before interpolating to target min/max.
- **Placement:** This conversion policy must live in `Range` helpers (`mapSigned` / `mapUnsigned`) so all transforms inherit one consistent behaviour.

### Transform Signal Rules
- Transform public constructors must accept base signal types, not `MappedSignal`.
- Transform internals should map signals immediately and store mapped forms.
- `MappedSignal` has no absolute/relative mode.
- `ZoomTransform` scale mapping is absolute by design.

## Dual-Core Rendering Contract

1. **Single Logical Scene Owner:** Scene timing and scene transitions must be owned by one `SceneManager` on core 0.
2. **Compile on Scene Change, Not Per Frame:** Scene/layer/pattern graphs may be compiled when a scene is created or replaced. The per-frame render path must not rebuild `ColourMap` chains.
3. **Per-Core Sampler Instances:** Dual-core rendering must use independently compiled sampler instances derived from the same logical scene definition. Do not construct separate randomized scenes per core.
4. **`advanceFrame()` Is the Mutable Phase:** Signal sampling, accumulator advancement, depth updates, and cached parameter updates belong in `advanceFrame(progress, elapsedMs)`.
5. **Sampling Is Read-Only:** `layer()`, transform `operator()`, and final sample functions must be read-only during render. They must not sample signals, mutate shared state, or read live time sources.
6. **No Per-Frame `ColourMap` Handoff:** Do not copy or move `ColourMap`/`fl::function` objects across cores during frame rendering. Synchronize by frame ownership and slice indices, not by passing function objects around.
7. **Safe RP2040 Startup:** Core 1 must not dereference shared display/renderer state before it is fully constructed and published.

## Implementation Standards
- **Error Handling:** Lean on predictable, documented overflow behavior (wrap or saturation). Use deterministic math rules to ensure consistent results across platforms.
- **Documentation:** Focus on high-quality `README.md` files for each major module that explain the architecture, math, and usage examples. Inline comments should be used sparingly and only to explain "why" complex logic is necessary.
- **Performance:** While clarity is key, ensure that inner loops are free of virtual dispatch and heap churn. Scene compilation may allocate on scene changes; the active frame loop must not.
- **Code Organization:** Every module should expose its public headers from `module/` (e.g., `renderer/pipeline/patterns/NoisePattern.h`) while moving implementation `.cpp` files into a sibling `module/src/` package (e.g., `renderer/pipeline/patterns/src/NoisePattern.cpp`). Platform-specific translation units and tests should continue to include the `.cpp` files in their new `src/` location, and PlatformIO filters must be updated to ignore the implementation package when a build target targets a different entry point. This layout keeps headers and implementation logically separated without disturbing the existing include paths.

## Verification and Safety
- **Build Verification:** Before finishing any track or committing significant changes, **YOU MUST** verify that the firmware builds and uploads successfully to the `teensy41_matrix` environment.
- **Hardware Targets:** Always verify compilation for both `native` (for unit tests) and `teensy41_matrix` (for hardware deployment).
- **RP2040 Changes:** Any change to dual-core scheduling, `SceneManager`, `Scene`, `Layer`, `UVPattern`, or the display handoff must also be verified against `seeed_xiao_rp2040`.
- **Unit Tests:** Write and run unit tests for all new logic (patterns, math, transforms) in the `native` environment.
- **No Floating Point:** Do not use `float` or `double` in the rendering pipeline. Use fixed-point math exclusively.
