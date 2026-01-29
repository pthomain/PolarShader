# Architectural Principles for Pattern Implementation

This document defines the "Definition of Done" for any new pattern in the PolarShader project. To ensure a modular, performant, and predictable rendering pipeline, all pattern implementations MUST adhere to the following principles.

---

## API Contract & Structure

1.  **HIERARCHY**: Inherit from `CartesianPattern` or `PolarPattern`.

2.  **FUNCTORS**: The `layer()` method must return a lightweight, POD-style functor. Lambdas are only acceptable if they are non-capturing or capture only POD data proven to fit within the `fl::function` small buffer optimization. Returning an explicit functor struct is strongly preferred for clarity.

3.  **PARAMETERIZATION**: Patterns must accept configuration via constructor parameters (e.g., cell size, softness). Hardcoded values are discouraged.

---

## Behavior & Correctness

4.  **DETERMINISM & STATELESSNESS**: Patterns must be pure functions of position (and optional depth from the pipeline context).
    - No `static` variables, no mutable state, no `random()` calls.
    - Stochastic patterns (e.g., Voronoi) MUST accept a `uint32_t seed` in their constructor to ensure they are deterministic.

5.  **NO TIME-BASED LOGIC**: All animation is handled by `Transforms` and/or a depth signal updated in the pipeline context. Patterns must not use `millis()` or any other time source.

6.  **NORMALIZATION**: The final output MUST be a `PatternNormU16` (0-65535).
    - Normalize continuous fields to the full range.
    - Categorical IDs should not be normalized.
    - Use `PatternRange(min, max).normalize(value)` for consistent remapping.

7.  **ALIASING**: Patterns with sharp edges must offer a `softness` parameter or use `PatternRange::smoothstep_u16()` to provide anti-aliasing.

8.  **COORDINATE & SEAM SAFETY**:
    - Cartesian: Use `CartesianMaths.h` helpers for fixed-point arithmetic. Define sizes in domain units and clamp minimums to avoid collapse.
    - Polar: Use `PolarMaths::shortest_angle_dist()` to handle angle wrapping and prevent visual seams.

---

## Performance & Embedded Constraints (SAMD21)

9.  **NO HEAVY FEATURES**: No exceptions (`throw`), RTTI (`dynamic_cast`), iostream, or heap-allocating STL containers like `std::vector` or `std::map`.

10. **BOUNDED COST**: The computational cost must be predictable and bounded. Avoid algorithms with unbounded `while` loops or unpredictable recursion.
