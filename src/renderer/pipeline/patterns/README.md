# PolarShader Pattern Architecture

This document outlines the architectural principles for creating patterns within the PolarShader engine. Adhering to these guidelines is crucial for ensuring the rendering pipeline is modular, performant, and predictable, especially on resource-constrained microcontrollers like the SAMD21.

## Core Principles

There are six fundamental principles that all pattern implementations **must** follow.

---

### 1. Follow the Class Hierarchy

All patterns must represent a specific visual structure in either a Cartesian, Polar, or unified UV coordinate space.

- **UV Patterns:** Must inherit from `PolarShader::UVPattern`. This is the recommended standard for new patterns.
- **Cartesian Patterns:** Must inherit from `PolarShader::CartesianPattern`.
- **Polar Patterns:** Must inherit from `PolarShader::PolarPattern`.

This inheritance is mandatory as it allows the rendering pipeline to handle patterns polymorphically.

---

### 2. Use Lightweight, POD-Style Functors (Preferred)

The `layer()` method should return an instance of a **lightweight functor** (a struct with an `operator()`). This practice is critical for avoiding heap allocations by allowing our `fl::function` layer types to use their small buffer optimization.

Lambdas are allowed when they are non-capturing or only capture POD data known to fit within the `fl::function` small-buffer storage. Keep them to a minimum and avoid them in hot paths.

---

### 3. Patterns MUST Be Stateless

A pattern's role is to define a static, visual structure as a pure function of position.

- **Pure Functions of Position (+ Depth):** A pattern's output must depend **only** on the input coordinates, the optional depth value from the `PipelineContext`, and any immutable configuration parameters passed to its constructor.
- **NO Internal State:** A pattern's functor **must not** have mutable member variables.
- **NO `static` Variables:** Pattern code **must not** use `static` variables.
- **NO Time-Based Logic:** Patterns **must not** use `millis()` or any other time source.

**All animation is handled *outside* the pattern** via transforms and/or a depth signal updated in the pipeline context. Patterns may consume the context depth but must remain stateless.

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

Fixed-point arithmetic with `CartQ24_8` is powerful but requires care. For patterns involving grids, cells, or integer-based steps, you **must** use the explicit helpers to avoid scaling errors and precision loss.

- **Use the Central Utility:** The `CartesianMaths.h` header provides safe, explicit functions for fixed-point operations.
  ```cpp
  #include "renderer/pipeline/maths/CartesianMaths.h"

  // Convert an integer to the Q24.8 format
  CartQ24_8 cell_size = CartesianMaths::from_int(32);

  // Get the integer part of a coordinate
  int32_t cell_x = CartesianMaths::floor_to_int(x);

  // Perform safe multiplication/division
  CartQ24_8 scaled_x = CartesianMaths::mul(x, scale_factor);
  ```

---

### 6. Handle Polar Coordinates Safely (Polar)

Polar patterns must treat the `angle` coordinate as a wraparound domain to prevent visual seams at the 0/360-degree boundary.

- **Use the Central Utility:** When calculating distances, differences, or neighbors in the angle domain, you **must** use the wrap-safe helpers provided in `PolarMaths.h`.
  ```cpp
  #include "renderer/pipeline/maths/PolarMaths.h"

  // Calculate the shortest distance between the current angle and a target spoke angle
  FracQ0_16 dist_to_spoke = PolarMaths::shortest_angle_dist(current_angle, spoke_angle);
  ```
