# Transforms

This folder contains pipeline transforms that modify sampling coordinates (UV space) or
palette mapping state (`PipelineContext`).

## Domains and units

- Spatial transforms consume and emit `UV` (`sr16` x/y).
- Internal transform math can use mapped units (`f16`, `sf16`, `sr8`), but
  public transform constructors must accept base signal types.
- `sr16` is the high-precision transform space; `sr8` is for grid/noise-style Cartesian internals where lower fractional precision is acceptable.

## Frame lifecycle

`SceneManager` calls `advanceFrame(progress, elapsedMs)` each frame:

- `progress` (`f16`) is scene-normalized progress for scene/layer orchestration.
- `elapsedMs` (`TimeMillis`) is scene-relative elapsed time and is the canonical input for
  scalar signal factories (`Sf16Signal`).

## Signal model

`Sf16Signal` is a scalar signal wrapper with two kinds:

- `SignalKind::PERIODIC`
  - Waveform receives scene `elapsedMs` directly.
  - Used by `sine(...)` and `noise(...)`.
- `SignalKind::APERIODIC`
  - Waveform receives relative time derived from `(duration, loopMode)`.
  - `LoopMode::RESET` wraps with `elapsedMs % duration`.
  - `duration == 0` emits `0`.

Sampling contract:

- Scalar waveform output is saturated to signed sf16 (Q0.16) `[-1, 1]`.
- Sampling always requires an explicit range: `signal.sample(range, elapsedMs)`.
- There is no range-less scalar sampling path.

Factory signatures:

- Periodic factories:
  - `sine(speed, amplitude, threshold, phaseOffset)`
  - `noise(speed, amplitude, threshold, phaseOffset)`
  - `speed` is signed turns-per-second and independent of scene duration.
  - `phaseOffset` is normalized turns (`0..1`).
- Aperiodic factories:
  - `linear(duration, loopMode)`
  - `quadraticIn(duration, loopMode)`
  - `quadraticOut(duration, loopMode)`
  - `quadraticInOut(duration, loopMode)`

Constant signal helpers:
- `constant(permille)`: returns a constant signal remapping unipolar `[0, 1000]` to signed `[-1, 1]`.
- `sPerMil(int16_t)` maps signed permille `[-1000, 1000]` to scalar `sf16 [-1, 1]`.
- `perMil(uint16_t)` maps unsigned permille `[0, 1000]` to scalar `f16 [0, 1]`.

Periodic shaping:

- Sine/noise factories emit signed waveform values, then saturate to `[-1, 1]`.

## Mapping and accumulation

- `Sf16Signal::sample(range, elapsedMs)` maps signed scalar signals into typed domains.
- `BipolarRange<T>` maps signed signal output directly to `[min, max]`.
- `MagnitudeRange<T>` first remaps `[-1, 1] -> [0, 1]`, then interpolates to `[min, max]`.
- `AngleRange` maps via unsigned remapping with wrapping for angular values.
- `UVSignal` no longer carries an absolute/relative flag.
- UV delta accumulation is handled explicitly by transforms that need it.
- Scalar `Sf16Signal` values are treated as absolute by contract (no scalar `absolute` flag).
- Shared mapping ranges in `Signals`:
  - `magnitudeRange()` for unsigned normalized scalar use (`[0, 1]` domain).
  - `bipolarRange()` for signed scalar use (`[-1, 1]` domain).

## Transform details

### RotationTransform

- Input: scalar angle or speed signal, optional `isAngleTurn` flag.
- `isAngleTurn = true` (Absolute): Internally maps with `AngleRange` to absolute turn offsets.
- `isAngleTurn = false` (Accumulation - default): Treats input as angular velocity (speed), maps with `bipolarRange` and integrates over time.

### VortexTransform

- Input: scalar strength signal.
- Maps to signed angular strength (`[-1, 1]` in f16/sf16 domain) and twists by radius.

### KaleidoscopeTransform

- Inputs: `nbFacets`, `isMirrored`.
- Folds angle into facets; optional mirroring for symmetric sectors.

### TranslationTransform

- Inputs: either a `UVSignal` or `(direction, speed)` scalar signals.
- `(direction, speed)` is converted to a per-frame UV delta signal and accumulated internally.
- Applies smoothing influenced by `PipelineContext::zoomScale`.

### ZoomTransform

- Input: scalar scale signal (`[-1, 1]` by convention).
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
- Samples signed speed directly with `bipolarRange()`.

### PaletteTransform

- Input: scalar threshold signal, optional clip signal/feather/power.
- Maps threshold to palette index shift and writes to `PipelineContext`.

## Usage note

- `defaultPreset` currently applies `ZoomTransform(sine(constant(550), constant(1000), constant(200)))`, so zoom should oscillate
  periodically by default.
