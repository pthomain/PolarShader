# Transforms

This folder contains pipeline transforms that modify sampling coordinates (UV space) or
palette mapping state (`PipelineContext`).

## Domains and units

- Spatial transforms consume and emit `UV` (`SQ16_16` x/y).
- Internal transform math can use mapped units (`UQ0_16`, `SQ0_16`, `SQ24_8`), but
  public transform constructors must accept base signal types.

## Frame lifecycle

`SceneManager` calls `advanceFrame(progress, elapsedMs)` each frame:

- `progress` (`UQ0_16`) is scene-normalized progress for scene/layer orchestration.
- `elapsedMs` (`TimeMillis`) is scene-relative elapsed time and is the canonical input for
  scalar signal factories (`SQ0_16Signal`).

## Signal model

`SQ0_16Signal` is a scalar signal wrapper with two kinds:

- `SignalKind::PERIODIC`
  - Waveform receives scene `elapsedMs` directly.
  - Used by `sine(...)` and `noise(...)`.
- `SignalKind::APERIODIC`
  - Waveform receives relative time derived from `(duration, loopMode)`.
  - `LoopMode::RESET` wraps with `elapsedMs % duration`.
  - `duration == 0` emits `0`.

Sampling contract:

- Scalar waveform output is saturated to signed Q0.16 `[-1, 1]`.
- Sampling always requires an explicit range: `signal.sample(range, elapsedMs)`.
- There is no range-less scalar sampling path.

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

- Sine/noise factories emit signed waveform values, then saturate to `[-1, 1]`.

## Mapping and accumulation

- `SQ0_16Signal::sample(range, elapsedMs)` maps signed scalar signals into typed domains.
- Signed ranges map directly from signed signal output.
- Unsigned ranges first remap `[-1, 1] -> [0, 1]`, then interpolate to `[min, max]`.
- `UVSignal` no longer carries an absolute/relative flag.
- UV delta accumulation is handled explicitly by transforms that need it.
- Scalar `SQ0_16Signal` values are treated as absolute by contract (no scalar `absolute` flag).
- Shared mapping ranges in `Signals`:
  - `unitRange()` for unsigned normalized scalar use (`[0, 1]` domain).
  - `signedUnitRange()` for signed scalar use (`[-1, 1]` domain).

## Transform details

### RotationTransform

- Input: scalar angle signal.
- Internally maps with `PolarRange` to turn offsets.

### VortexTransform

- Input: scalar strength signal.
- Maps to signed angular strength (`[-1, 1]` in Q0.16 domain) and twists by radius.

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
- Samples signed speed directly with `signedUnitRange()`.

### PaletteTransform

- Input: scalar offset signal, optional clip signal/feather/power.
- Maps offset to palette index shift and writes to `PipelineContext`.

## Usage note

- `defaultPreset` currently applies `ZoomTransform(sine(cPerMil(100)))`, so zoom should oscillate
  periodically by default.
