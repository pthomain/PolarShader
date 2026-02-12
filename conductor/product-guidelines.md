# Product Guidelines

## Development Philosophy
- **Clarity & Composability:** Prioritize modular, readable code that reflects the "shader-like" philosophy. Components should be easy to understand in isolation and simple to combine into complex pipelines.
- **Deterministic Fixed-Point:** Every operation must be deterministic. Avoid floating-point arithmetic and dynamic memory allocation in the hot path.

## API Design
- **Fluent & Declarative:** Provide a builder-style API that allows users to declare the rendering pipeline in a readable, sequential manner.
- **Strong Typing:** Every core scalar or coordinate is wrapped in a `Strong<T, Tag>` template to prevent unit confusion and ensure math correctness at compile-time.
- **Balanced Configuration:** Support runtime modulation of parameters and signal sources, while maintaining a stable or semi-static pipeline structure to ensure performance.
- **Transform Encapsulation:** Public Transform APIs must never expose internal implementation types like `MappedSignal`. They must accept base Signal types and handle mapping/state privately.

## Strong Type System

| Type | Format | Definition | Usage |
| :--- | :--- | :--- | :--- |
| `SQ0_16` | Unsigned 0.16 | 16-bit integer [0, 1.0) | Angles (turns), alpha, scaling factors. |
| `SQ0_16` | Signed 0.16 | 32-bit integer [-1.0, 1.0] | Signals, oscillators, trig results (sin/cos). |
| `SQ16_16` | Signed 16.16 | 32-bit integer | Spatial UV coordinates (allows tiling/zoom). |
| `PatternNormU16`| Unsigned 16-bit | [0, 65535] | Universal currency for pattern intensities. |
| `SQ24_8` | Signed 24.8 | 32-bit integer | Internal lattice-aligned pattern calculations. |
| `UV` | 2x Q16.16 | Normalized 2D vector | Standard spatial type passed between pipeline steps. |

### Rules
1. **No Implicit Casting:** Never cast between strong types or to raw integers without using the `raw()` helper.
2. **Semantic Boundaries:** Use `SQ0_16` for any value that can go negative (signals), and `SQ0_16` for values that must wrap or stay positive (angles).
3. **Internal vs External:** `SQ24_8` is an implementation detail for patterns; external APIs should only expose `UV`.

## Signal & Timing Rules

### Timing Terminology
- **Elapsed Time (`TimeMillis elapsedMs`):** Canonical time source for scalar signal sampling.
- **Progress (`SQ0_16 progress`):** Scene-normalized progress used where mapped signal APIs require it.

### Signal Kinds
- **Periodic (`SignalKind::PERIODIC`):**
  - Factories: `sine`, `noise`.
  - Signature must be: `(speed, amplitude, offset, phaseOffset)`.
  - `speed` is signed turns-per-second, scene-duration independent.
  - `phaseOffset` is normalized turns (`0..1`).
- **Aperiodic (`SignalKind::APERIODIC`):**
  - Factories: `linear`, `quadraticIn`, `quadraticOut`, `quadraticInOut`.
  - Signature must be: `(duration, loopMode)`.
  - `LoopMode::RESET` wraps relative time with modulo.
  - `duration == 0` emits `0`.

### Value Constraints
- **Normalization:** `SQ0_16Signal` public outputs are clamped to `[-1, 1]`.
- **Range Coverage:** Periodic samplers (`sine`, `noise`) must span the full signed range by default.
- **Directionality:** Internal phase accumulation must preserve signed speed for reverse motion.

### Signal Responsibilities
- `SQ0_16Signal` wraps waveform evaluation and time routing (`PERIODIC` vs `APERIODIC`).
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

## Implementation Standards
- **Error Handling:** Lean on predictable, documented overflow behavior (wrap or saturation). Use deterministic math rules to ensure consistent results across platforms.
- **Documentation:** Focus on high-quality `README.md` files for each major module that explain the architecture, math, and usage examples. Inline comments should be used sparingly and only to explain "why" complex logic is necessary.
- **Performance:** While clarity is key, ensure that inner loops are free of virtual dispatch and heap churn. Keep the memory footprint predictable.

## Verification and Safety
- **Build Verification:** Before finishing any track or committing significant changes, **YOU MUST** verify that the firmware builds and uploads successfully to the `teensy41_matrix` environment.
- **Hardware Targets:** Always verify compilation for both `native` (for unit tests) and `teensy41_matrix` (for hardware deployment).
- **Unit Tests:** Write and run unit tests for all new logic (patterns, math, transforms) in the `native` environment.
- **No Floating Point:** Do not use `float` or `double` in the rendering pipeline. Use fixed-point math exclusively.
