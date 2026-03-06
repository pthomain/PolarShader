# Architectural Principles for Pattern Implementation

This document defines the "Definition of Done" for any new pattern in the PolarShader project. To ensure a modular, performant, and predictable rendering pipeline, all pattern implementations MUST adhere to the following principles.

---

## API Contract & Structure

1.  **HIERARCHY**: Inherit from `UVPattern`.

2.  **FUNCTORS**: The `layer()` method must return a lightweight, read-only sampler. Explicit functor structs are preferred. Lambdas are acceptable only when their captures are lightweight, immutable, and proven safe for storage inside `fl::function`.

3.  **PARAMETERIZATION**: Patterns must accept configuration via constructor parameters (e.g., cell size, softness). Hardcoded values are discouraged.

4.  **FRAME LIFECYCLE**: Any frame-varying state must be updated in `advanceFrame(progress, elapsedMs)`.
    - `advanceFrame()` may sample signals and cache derived values.
    - `layer()` is part of scene compilation and must not do frame-varying work.

---

## Behavior & Correctness

5.  **DETERMINISM & SAMPLE-TIME PURITY**: The compiled sampling path must be a pure function of position plus stable per-frame state.
    - No `static` variables, no `random()` calls, no time reads in the sampler.
    - Patterns may keep mutable cached frame state, but that state must be fully prepared before render starts.
    - Stochastic patterns (e.g., Voronoi) MUST accept a `uint32_t seed` in their constructor to ensure they are deterministic.

6.  **NO TIME-BASED LOGIC IN `layer()` OR THE SAMPLER**: All animation is handled by `advanceFrame()`, transforms, and/or a depth signal updated in the pipeline context. Patterns must not use `millis()` or any other time source during compilation or sampling.

7.  **DUAL-CORE SAFETY**: RP2040 renders by sampling compiled pattern chains from two cores.
    - The sampler returned by `layer()` must be read-only during render.
    - Do not sample signals, mutate shared state, or depend on copy/move side effects in the compiled sampler.
    - `PipelineContext` may be read during sampling only for values that are stable for the current frame, such as depth.

8.  **NORMALIZATION**: The final output MUST be a `PatternNormU16` (0-65535).
    - Normalize continuous fields to the full range.
    - Categorical IDs should not be normalized.
    - Use `patternNormalize(value, min, max)` for consistent remapping.

9.  **ALIASING**: Patterns with sharp edges must offer a `softness` parameter or use `patternSmoothstepU16()` to provide anti-aliasing.

10. **COORDINATE & SEAM SAFETY**:
    - Cartesian: Use `CartesianMaths.h` helpers for fixed-point arithmetic. Define sizes in domain units and clamp minimums to avoid collapse.
    - Polar: Use `PolarMaths::shortest_angle_dist()` to handle angle wrapping and prevent visual seams.

---

## Performance & Embedded Constraints (SAMD21)

11. **NO HEAVY FEATURES**: No exceptions (`throw`), RTTI (`dynamic_cast`), iostream, or heap-allocating STL containers like `std::vector` or `std::map`.

12. **BOUNDED COST**: The computational cost must be predictable and bounded. Avoid algorithms with unbounded `while` loops or unpredictable recursion.
