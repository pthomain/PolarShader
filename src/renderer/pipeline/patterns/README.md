# PolarShader Pattern Architecture

This document outlines the architectural principles for creating patterns within the PolarShader engine. Adhering to these guidelines is crucial for ensuring the rendering pipeline is modular, performant, and predictable, especially on resource-constrained microcontrollers like the SAMD21.

## Core Principles

There are six fundamental principles that all pattern implementations **must** follow.

---

### 1. Follow the Class Hierarchy

All patterns must represent a specific visual structure in the unified UV coordinate space.

- **UV Patterns:** Must inherit from `PolarShader::UVPattern`.

This inheritance is mandatory as it allows the rendering pipeline to handle patterns polymorphically.

---

### 2. Use Lightweight, POD-Style Functors (Preferred)

The `layer()` method should return an instance of a **lightweight functor** (a struct with an `operator()`). This practice is critical for keeping compiled sampler chains small and stable when scenes are cached for single-core or dual-core rendering.

Lambdas are allowed when they are non-capturing or only capture lightweight, read-only state known to fit within the `fl::function` small-buffer storage. Avoid mutable captures and avoid capturing objects that perform work during copy or move.

---

### 3. Separate Frame State from Sampling

A pattern's sampling path must remain pure during render, but the pattern may cache frame-derived values ahead of time.

- **Use `advanceFrame()` for mutable work:** Signal sampling, cached radius/scale calculation, or any other frame-varying state update must happen in `advanceFrame(progress, elapsedMs)`.
- **Keep `layer()` pure:** `layer()` is part of scene compilation. It must not sample signals, read time, advance accumulators, or mutate pattern state.
- **Read-only functors during render:** The returned functor may read immutable configuration, `PipelineContext` values that are stable for the current frame, and values prepared in `advanceFrame()`.
- **NO `static` Variables:** Pattern code **must not** use `static` variables for render state.
- **NO Time-Based Logic in sampling:** Pattern functors must not call `millis()` or any other live time source.

This split is required for RP2040 dual-core rendering, where both cores sample compiled pattern chains concurrently.

---

### 4. Adhere to the Normalization Contract

A pattern's final output **must** be a `PatternNormU16` value that spans the full `0..65535` dynamic range. This ensures consistent brightness and palette mapping across all patterns.

- **Use the Central Utility:** If a pattern's native output is not already in the full `0..65535` range, it must be normalized using the canonical helper:
  ```cpp
  #include "renderer/pipeline/maths/PatternMaths.h"
  return patternNormalize(raw_value, min_range, max_range);
  ```

---

### 5. Use Explicit Coordinate Contracts (Cartesian)

Fixed-point arithmetic with `fl::s24x8` is powerful but requires care. For patterns involving grids, cells, or integer-based steps, you **must** use the explicit helpers to avoid scaling errors and precision loss.

- **Use the Central Utility:** The `CartesianMaths.h` header provides safe, explicit functions for fixed-point operations. Note that `CartesianMaths` now only exposes `from_uv()` and `to_uv()` for coordinate conversion between the `fl::s16x16` UV domain and the `fl::s24x8` Cartesian domain.
  ```cpp
  #include "renderer/pipeline/maths/CartesianMaths.h"

  // Convert from UV (fl::s16x16) to Cartesian (fl::s24x8)
  fl::s24x8 cart = CartesianMaths::from_uv(uv_coord);

  // Convert back from Cartesian (fl::s24x8) to UV (fl::s16x16)
  fl::s16x16 uv = CartesianMaths::to_uv(cart_coord);
  ```

---

### 6. Handle Polar Coordinates Safely (Polar)

Polar patterns must treat the `angle` coordinate as a wraparound domain to prevent visual seams at the 0/360-degree boundary.

- **Use the Central Utility:** When calculating distances, differences, or neighbors in the angle domain, you **must** use the wrap-safe helpers provided in `PolarMaths.h`.
  ```cpp
  #include "renderer/pipeline/maths/PolarMaths.h"

  // Calculate the shortest distance between the current angle and a target spoke angle
  f16 dist_to_spoke = PolarMaths::shortest_angle_dist(current_angle, spoke_angle);
  ```
