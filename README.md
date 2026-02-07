# PolarShader

**PolarShader** is a fixed-point, shader-like rendering library for Arduino.

It provides a **polar-centric transform pipeline**, **deterministic fixed-point math**, and a **composable API** inspired by GPU shaders—designed specifically for **resource-constrained MCUs** driving LED displays and spatial light sculptures.

---

## What PolarShader Is

PolarShader is a **domain-specific rendering engine** for LED effects where:

- The **polar domain** (angle + radius) is first-class
- All math is **fixed-point** (no floats, no heap churn)
- Effects are built by **composing transforms**, not by writing monolithic loops
- Motion and modulation are **explicit, typed, and deterministic**

It targets platforms such as:
- RP2040
- SAMD21 / SAMD51
- ESP32 (when deterministic timing matters)

---

## Core Design Goals

### 1. Deterministic Fixed-Point Arithmetic

- No `float`, no `double`
- Explicit Q-formats (`Q0.16`, `Q16.16`, `Q1.15`)
- Fully deterministic across frames, compilers, and MCUs
- Predictable overflow, wrap, and saturation behavior

This makes PolarShader suitable for:
- Tight real-time loops
- Long-running installations
- Visual continuity under variable frame timing

---

### 2. Unified Coordinate System (UV)

PolarShader uses a **unified spatial representation (UV)**:
- **Normalized UV Space:** All spatial operations are performed in a normalized [0.0, 1.0] domain mapped to `FracQ16_16`.
- **Domain Agnostic:** Transforms like rotation or zoom apply seamlessly to any pattern.
- **High Precision:** `Q16.16` provides sufficient geometric headroom for large-scale translations and zooms.

---

### 3. Shader-Like Composition API

Effects are built by stacking **small, pure transforms**, similar to GPU shaders:

```cpp
pipeline
  .addTransform(RotationTransform(noise(cPerMil(120))))
  .addTransform(KaleidoscopeTransform(6, true))
  .addTransform(ZoomTransform(animate(cPerMil(30), SinusoidalInterpolatorInOut())))
````

Each transform:

* Has a **single responsibility**
* Is stateless (or uses an explicit integrator)
* Can be reordered, removed, or reused

---

## Key Concepts

### Modulators (Signals)

A **Modulator** is a time-based signal source:

```cpp
using ScalarModulator = fl::function<FracQ16_16(TimeMillis)>;
```

Modulators are used for:

* Speed
* Amplitude
* Phase velocity
* Offsets
* Control-rate parameters

Examples:

```cpp
auto speed = ConstantPhaseVelocity(FracQ16_16(60));
auto wobble = Sine(speed, Constant(FracQ16_16(200)));
```

They are:

* Cheap
* Deterministic
* Explicit about units

---

### Phase vs Angle

PolarShader clearly distinguishes:

* **AngleUnitsQ0_16**
  A sampled angle (0…65535 → 0…1 turn)

* **AngleTurnsUQ16_16**
  A high-resolution phase accumulator for smooth motion

This avoids:

* Quantization jitter
* Hidden precision loss
* Accidental misuse of angles as phases

---

### Motion as Integration

Motion is not implicit.
All motion goes through **explicit integrators**:

* Phase accumulators for rotation
* Scalar accumulators for scale / offset
* Cartesian integrators for translation

This ensures:

* Smooth motion under variable `dt`
* Correct wrap semantics
* Centralized timing logic

---

## Why Not FastLED Effects Directly?

FastLED is excellent at **pixel IO and color math**.
PolarShader focuses on **spatial structure and motion**.

Use them together:

* FastLED handles LEDs and palettes
* PolarShader handles geometry and time

---

## What PolarShader Is *Not*

* Not a general-purpose math library
* Not a floating-point abstraction
* Not a scene graph
* Not a GPU replacement

It is intentionally narrow—and fast.

---

## Performance Characteristics

* No heap allocation in hot paths
* No virtual dispatch in inner loops
* Small, predictable codegen
* Suitable for 48–133 MHz MCUs

---

## Philosophy Summary

> **Make geometry explicit.**
> **Make time explicit.**
> **Make units explicit.**
> **Compose everything.**

PolarShader treats LED effects as **deterministic programs over space and time**, not ad-hoc animations.

---

## License

GPL-3.0-or-later

