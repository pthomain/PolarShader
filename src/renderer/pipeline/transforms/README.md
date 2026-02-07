# Transforms

This folder contains the pipeline transforms that modify pattern sampling coordinates in either
Cartesian (Q24.8) or Polar (Q0.16 turns) space, plus palette transforms that influence final
palette lookup. Transforms are evaluated per-frame to update internal state, then applied per-sample
by wrapping a layer function (or by updating shared context).

## Domains and units

- **UV domain:** Uses `UV` coordinates (two `FracQ16_16` values) representing a normalized [0, 1] spatial domain. This is the standard for all spatial transformations.
- **Internal Units:** Patterns may use internal units like `CartQ24_8` or `FracQ0_16` for calculation, but the transformation interface is strictly `UV`.

## Signals and ranges

All transform inputs are time-indexed signals driven by scene progress:

- `SceneManager` computes `progress` as 0..1 across the current scene duration.
- Easing signals (`linear`, `quadratic*`, `sinusoidal*`) emit 0 at scene start and 1 at scene end.
- Periodic/open-ended signals (`noise`, `sine`) use `phaseSpeed` in **turns-per-second** (Q16.16),
  integrated with elapsed milliseconds.

Signal types:

- `SFracQ0_16Signal` is a function `FracQ0_16 -> SFracQ0_16` (typically 0..1 in Q0.16).
- `UVSignal` is a `MappedSignal<UV>` used for spatial modulation (e.g., translation offsets).
- `MappedValue<T>` is the output of a range mapping at a specific progress value.
- `MappedSignal<T>` is the output of `Range::mapSignal(...)`. Transforms should store `MappedSignal`
  values internally, not raw `SFracQ0_16Signal` inputs.
- If a signal is relative (`absolute=false`), call `resolveMappedSignal(...)` after mapping so deltas
  accumulate into absolute values.

Avoid pre-mapping or scaling signals manually; pass normalized signals and let the transform range
do the conversion.

Ranges used by transforms:

- `PolarRange` maps `SFracQ0_16` to angular turns with proper wrapping.
- `LinearRange<T>` generic linear interpolation from a 0..1 signal to any type `T`.
- `UVRange` maps a 0..1 signal into a 2D line segment in UV space.

## Pipeline usage

Transforms are added via `LayerBuilder`. `SceneManager` handles progress and elapsed time and calls
`advanceFrame(progress, elapsedMs)` each frame before sampling layers.

Example:

```cpp
auto manager = SceneManager(std::make_unique<DefaultSceneProvider>([] {
    fl::vector<std::shared_ptr<Layer>> layers;
    layers.push_back(std::make_shared<Layer>(
        LayerBuilder(noisePattern(), palette, "demo")
            .addTransform(ZoomTransform(sine()))
            .addTransform(RotationTransform(noise(phasePerMil(200))))
            .build()
    ));
    return std::make_unique<Scene>(std::move(layers), 30000);
}));

manager.advanceFrame(millis());
auto layer = manager.build();
```

## Transform details

### RotationTransform (polar)

- Input: `angle` signal (turns, Q0.16).
- Uses `PolarRange` to map the resolved signal into an angular offset (0..1 turns).
- `advanceFrame` caches the mapped angle; `operator()` samples the wrapped layer at that offset so
  the transform follows whatever value was resolved from the signal (absolute values pass through,
  relative deltas are accumulated before mapping).

### VortexTransform (polar)

- Input: `strength` signal (turns, Q0.16).
- Maps via `LinearRange<SFracQ0_16>(Q0_16_MIN, Q0_16_MAX)` to a signed angular strength.
- `operator()` adds `radius * strength` to the incoming angle, producing a radial twist.

### KaleidoscopeTransform (polar)

- Inputs: `nbFacets` (number of radial wedges), `isMirrored` (mirror every other wedge).
- Folds the angle into `nbFacets` equal sectors.
- If `isMirrored` is true, odd sectors are reflected for a symmetric kaleidoscope effect.

### TranslationTransform (cartesian)

- Inputs: `direction` (turns, Q0.16) and `speed` (0..1, Q0.16).
- Uses `PolarRange` for direction and `LinearRange<int32_t>` for speed (Q16.16 units per scene).
- Builds a **relative** UV velocity signal and resolves it into an accumulated offset.
- Applies smoothing based on `PipelineContext::zoomScale` (higher zoom -> less smoothing).
- Result is an offset added to every `(x, y)` sample.

### ZoomTransform (cartesian)

- Input: `scale` signal (0..1, Q0.16).
- Uses `LinearRange<SFracQ0_16>` to map into `[MIN_SCALE, MAX_SCALE]` and smooths changes based on current frequency.
- Zoom treats its mapped signal as absolute (relative accumulation is intentionally disabled).
- Updates `PipelineContext::zoomScale` each frame.

### DomainWarpTransform (cartesian)

- Inputs:
  - `phaseSpeed`: turns-per-second for the time axis (mapped via `LinearRange` into Q16.16 turns/s).
  - `amplitude`: 0..1 signal mapped to `[0, maxOffset]` (Q24.8). Controls warp displacement strength.
  - `warpScale`: 0..1 signal mapped via `CartRange`, applied to input coords before sampling warp noise (Q24.8).
    - Smaller than 1.0 => lower-frequency, broad bends.
    - Larger than 1.0 => higher-frequency, tighter warps.
    - Does not change displacement magnitude; only the spatial frequency of the warp field.
  - `maxOffset`: 0..1 signal mapped via `CartRange`, maximum warp displacement (Q24.8).
    - This is the dominant control for “how much” warp you see.
    - Displacement is roughly `amplitude * maxOffset * noise`, where noise is centered around 0.
    - If `maxOffset` is too small relative to your coordinate scale (Q0.16 ≈ 0..65535),
      the effect can appear invisible.
  - For `Directional` warp:
    - `flowDirection`: angular direction of the bias vector (mapped with `PolarRange`).
    - `flowStrength`: 0..1 signal mapped to `[0, maxOffset]` (Q24.8) for directional drift strength.

Example values (Q24.8 constants shown as `CartQ24_8(n << CARTESIAN_FRAC_BITS)`):

- WarpScale (frequency control):
  - Broad/slow field: `CartQ24_8(1 << CARTESIAN_FRAC_BITS)` (1.0)
  - Medium detail: `CartQ24_8(2 << CARTESIAN_FRAC_BITS)` (2.0)
  - Tight/busy: `CartQ24_8(4 << CARTESIAN_FRAC_BITS)` (4.0)

- MaxOffset (displacement strength):
  - Subtle: `CartQ24_8(256 << CARTESIAN_FRAC_BITS)`
  - Medium: `CartQ24_8(1024 << CARTESIAN_FRAC_BITS)`
  - Strong: `CartQ24_8(4096 << CARTESIAN_FRAC_BITS)`

- Typical presets:
  - `phaseSpeed`: `noise(phasePerMil(800))` for gentle drift, `noise(phasePerMil(2000))` for faster motion.
  - `amplitude`: `cPerMil(200)` (20%) subtle, `cPerMil(500)` (50%) obvious, `ceiling()` (100%) max.
- Warp types:
  - `Basic`, `FBM`, `Nested`, `Curl`, `Polar`, `Directional`.
- `advanceFrame` updates phase (time offset), amplitude, and directional flow offset.
- `operator()` warps coordinates and samples the wrapped layer.

Presets are provided in `DomainWarpPresets.*` for common parameter sets.

### PaletteTransform (palette)

- Input: `offset` signal (0..1, Q0.16).
- Uses `LinearRange<uint8_t>` to map the signal into a palette index offset.
- `advanceFrame` writes the offset into `PipelineContext::paletteOffset` for use during palette lookup.

## Context expectations

Transforms rely on a shared, non-null `PipelineContext` supplied by the pipeline. If you bypass
the builder, call `setContext` on transforms and patterns yourself, and ensure a single shared
instance is used everywhere.
