# Transforms

This folder contains pipeline transforms that modify sampling coordinates (UV space) or
palette mapping state (`PipelineContext`).

## Domains and units

- Spatial transforms consume and emit `UV` (`FracQ16_16` x/y).
- Internal transform math can use mapped units (`FracQ0_16`, `SFracQ0_16`, `CartQ24_8`), but
  public transform constructors must accept base signal types.

## Frame lifecycle

`SceneManager` calls `advanceFrame(progress, elapsedMs)` each frame:

- `progress` (`FracQ0_16`) is scene-normalized progress and is still used by mapped signal APIs.
- `elapsedMs` (`TimeMillis`) is scene-relative elapsed time and is the canonical input for
  scalar signal factories (`SFracQ0_16Signal`).

## Signal model

`SFracQ0_16Signal` is a scalar signal wrapper with two kinds:

- `SignalKind::PERIODIC`
  - Waveform receives scene `elapsedMs` directly.
  - Used by `sine(...)` and `noise(...)`.
- `SignalKind::APERIODIC`
  - Waveform receives relative time derived from `(duration, loopMode)`.
  - `LoopMode::RESET` wraps with `elapsedMs % duration`.
  - `duration == 0` emits `0`.

Output behavior:

- Public sampling (`operator()` / `sample`) clamps to `[0, 1]`.
- Internal signed-use sampling is available via `sampleUnclamped(...)` for phase speed paths.

Factory signatures:

- Periodic factories:
  - `sine(speed, amplitude, offset, phaseOffset)`
  - `noise(speed, amplitude, offset, phaseOffset)`
  - `speed` is signed turns-per-second and independent of scene duration.
  - `phaseOffset` is normalized turns (`0..1`).
- Aperiodic factories:
  - `linear(duration, loopMode)`
  - `quadraticIn(duration, loopMode)`
  - `quadraticOut(duration, loopMode)`
  - `quadraticInOut(duration, loopMode)`

Periodic shaping:

- Sine/noise samplers provide full-range `0..1`.
- Factory output follows: `0.5 + (waveCentered * amplitude)/2 + offset/2`, then clamped to `[0, 1]`.

## Mapping and accumulation

- `Range::mapSignal(...)` maps normalized scalar signals into typed domains (`MappedSignal<T>`).
- `MappedSignal<T>` no longer carries an absolute/relative flag.
- UV relative accumulation is handled via `resolveMappedSignal(UVSignal)` in
  `signals/SignalAccumulators.h`.
- Scalar `SFracQ0_16Signal` values are treated as absolute by contract (no scalar `absolute` flag).

## Transform details

### RotationTransform

- Input: scalar angle signal.
- Internally maps with `PolarRange` to turn offsets and stores a mapped signal.

### VortexTransform

- Input: scalar strength signal.
- Maps to signed angular strength (`[-1, 1]` in Q0.16 domain) and twists by radius.

### KaleidoscopeTransform

- Inputs: `nbFacets`, `isMirrored`.
- Folds angle into facets; optional mirroring for symmetric sectors.

### TranslationTransform

- Inputs: either a `UVSignal` or `(direction, speed)` scalar signals.
- `(direction, speed)` is converted to a relative UV vector signal and then accumulated via
  `resolveMappedSignal(...)`.
- Applies smoothing influenced by `PipelineContext::zoomScale`.

### ZoomTransform

- Input: scalar scale signal (`0..1`).
- Maps to internal zoom range `[MIN_SCALE_RAW, MAX_SCALE_RAW]`.
- Zoom mapping is absolute by design (no mapped relative mode).
- Writes current zoom scale to `PipelineContext::zoomScale`.

### DomainWarpTransform

- Inputs:
  - `speed`: signed turns-per-second for time-axis phase integration.
  - `amplitude`: mapped displacement strength.
  - `warpScale`: mapped spatial frequency control.
  - `maxOffset`: mapped displacement cap.
  - Optional directional flow controls.
- Uses `PhaseAccumulator` for time evolution.
- Uses unclamped scalar speed sampling internally to preserve signed direction.

### PaletteTransform

- Input: scalar offset signal, optional clip signal/feather/power.
- Maps offset to palette index shift and writes to `PipelineContext`.

## Usage note

- `defaultPreset` currently applies `ZoomTransform(sine(cPerMil(100)))`, so zoom should oscillate
  periodically by default.
