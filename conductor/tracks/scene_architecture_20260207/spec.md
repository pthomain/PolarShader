# Specification: Scene-Based Architecture and Enhanced Animation System

## Overview
This track introduces a higher-level "Scene" architecture to PolarShader, allowing for multi-layer composition with blend modes and duration-based transitions. It also overhauls the animation system by replacing basic linear/sine/pulse signals with a unified `animate` function powered by a polymorphic `Interpolator` hierarchy.

## Renaming & Structural Changes
- **Core Classes:** 
    - `PolarPipeline` is renamed to `Layer`.
    - `PolarPipelineBuilder` is renamed to `LayerBuilder`.
- **Type Aliases:**
    - `UVLayer` is renamed to `UVMap`.
    - `ColourLayer` is renamed to `ColourMap`.
- **File Moves:**
    - `src/renderer/pipeline/PolarPipeline.h` -> `src/renderer/pipeline/Layer.h`
    - (And corresponding .cpp files and builder files).

## Animation System Overhaul
- **Interpolators:**
    - A new directory `src/renderer/pipeline/signals/interpolators/` will house the `Interpolator` base class and its implementations.
    - **Included Interpolators:**
        - `LinearInterpolator` (Default)
        - `QuadraticInterpolator` (In, Out, InOut)
        - `SinusoidalInterpolator` (In, Out, InOut)
        - `ElasticInterpolator` (In, Out, InOut)
        - `BounceInterpolator` (In, Out, InOut)
- **Signals API:**
    - `linear(TimeMillis durationMs)` is renamed to `animate(TimeMillis durationMs, const Interpolator& interpolator = LinearInterpolator())`.
    - `sine()` and `pulse()` are removed in favor of `animate` with appropriate interpolators.
    - Existing usage of `linear`, `sine`, and `pulse` across the codebase must be updated.

## Scene & Layering
- **Layer Attributes:**
    - Each `Layer` instance now has an `alpha` value (FracQ0_16) and a `BlendMode`.
    - Supported `BlendMode`s: `Normal`, `Add`, `Multiply`, `Screen`.
- **Scene Class:**
    - Manages a collection (list) of `Layer` objects.
    - Has a `durationMs`.
    - Receives absolute time and translates it to **relative time** (absolute - startTime) for its layers.
    - Resets its internal timer to 0 when `durationMs` is reached.
- **Scene Management:**
    - `SceneProvider`: An interface for providing the next `Scene`.
    - `SceneManager`: Orchestrates the current `Scene`, handles duration expiry, and requests the next scene from the `SceneProvider`.
    - `DefaultSceneProvider`: A simple implementation that always returns the same scene (loops).

## Functional Requirements
- **Relative Timing:** All layers within a scene must render based on the scene's elapsed time, ensuring deterministic behavior regardless of when the scene started.
- **Composition:** The `Scene` must composite its layers from bottom to top using the specified blend modes and per-layer alpha.
- **Fixed-Point Consistency:** All alpha blending and interpolation must continue to use the project's Q16.16 (or relevant Q-format) fixed-point math.

## Out of Scope
- Complex transition effects between scenes (e.g., cross-fades). Scenes swap instantly for now.
- Dynamic scene loading from external files/SD cards.
