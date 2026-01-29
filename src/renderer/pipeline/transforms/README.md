# Transforms

This folder contains the pipeline transforms that modify pattern sampling coordinates in either
Cartesian (Q24.8) or Polar (Q0.16 turns) space. Transforms are evaluated per-frame to update
internal state, then applied per-sample by wrapping a layer function.

## Domains and units

- Polar domain uses `FracQ0_16` turns for angle and `FracQ0_16` for radius.
- Cartesian domain uses `CartQ24_8` fixed-point coordinates (Q0.16 lattice units with extra precision).
- Domain changes only happen through pipeline steps (`toPolar`, `toCartesian`).

## Signals and ranges

All transform inputs are time-indexed signals:

- `SFracQ0_16Signal` is a function `TimeMillis -> SFracQ0_16` (usually 0..1 in Q0.16).
- Every transform maps its signal(s) to its own internal range before use.
- Avoid pre-mapping or scaling signals manually; pass normalized signals and let the transform range
  do the conversion.

Ranges used by transforms:

- `PolarRange` maps `SFracQ0_16` to angular turns with proper wrapping.
- `CartesianRange` maps direction + velocity signals to a cartesian vector.
  Negative velocity flips direction by 180 degrees.
- `ScalarRange` maps a 0..1 signal into an integer range `[min, max]`.

## Pipeline usage

Transforms are intended to be added via `PolarPipelineBuilder`, which also sets a shared
`PipelineContext` on all transforms and patterns. `advanceFrame(time)` must be called each frame
to update transform state before sampling layers.

Example (simplified):

```cpp
auto pipeline = PolarPipelineBuilder(std::move(pattern), palette, "demo")
    .addCartesianTransform(ZoomTransform(noise(cPerMil(120))))
    .addCartesianTransform(DomainWarpTransform(noise(cPerMil(120)), cPerMil(250),
                                               CartQ24_8(1 << CARTESIAN_FRAC_BITS),
                                               CartQ24_8(4 << CARTESIAN_FRAC_BITS)))
    .addPolarTransform(RotationTransform(noise(cPerMil(80))))
    .build();

pipeline.advanceFrame(timeInMillis);
auto layer = pipeline.build();
```

## Transform details

### RotationTransform (polar)

- Input: `angle` signal (turns, Q0.16).
- Uses `PolarRange` to map the signal into an angular offset.
- `advanceFrame` caches the mapped angle; `operator()` adds it to incoming angles.

### TranslationTransform (cartesian)

- Inputs: `direction` (turns, Q0.16) and `velocity` (0..1, Q0.16).
- Uses `CartesianMotionAccumulator` with a `CartesianRange` radius to integrate motion over time.
- Applies smoothing based on `PipelineContext::zoomNormalized` (higher zoom -> less smoothing).
- Result is an offset added to every `(x, y)` sample.

### ZoomTransform (cartesian)

- Input: `scale` signal (0..1, Q0.16).
- Maps into `[MIN_SCALE, MAX_SCALE]` and smooths changes based on current frequency.
- Anchors:
  - `Floor`: 0 maps to MIN, 1 maps to MAX.
  - `Ceiling`: 0 maps to MAX, 1 maps to MIN.
  - `MidPoint`: centered around the midpoint of the range.
- Updates `PipelineContext::zoomScale` and `zoomNormalized` each frame.

### DomainWarpTransform (cartesian)

- Inputs:
  - `phaseVelocity`: turns-per-second for the time axis (mapped via `ScalarRange`).
  - `amplitude`: 0..1 signal mapped to `[0, maxOffset]` (Q24.8).
  - `warpScale`: scale applied to input coords before sampling warp noise (Q24.8).
  - `maxOffset`: maximum warp displacement (Q24.8).
  - For `Directional` warp: `flowDirection` + `flowStrength` signals.
- Warp types:
  - `Basic`, `FBM`, `Nested`, `Curl`, `Polar`, `Directional`.
- `advanceFrame` updates phase (time offset), amplitude, and directional flow offset.
- `operator()` warps coordinates and samples the wrapped layer.

Presets are provided in `DomainWarpPresets.*` for common parameter sets.

## Context expectations

Transforms rely on a shared, non-null `PipelineContext` supplied by the pipeline. If you bypass
the builder, call `setContext` on transforms and patterns yourself, and ensure a single shared
instance is used everywhere.
