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
- Explicit Q-formats (`f16/sf16 (Q0.16)`, `r16/sr16 (Q16.16)`, `Q1.15`)
- Fully deterministic across frames, compilers, and MCUs
- Predictable overflow, wrap, and saturation behavior

This makes PolarShader suitable for:
- Tight real-time loops
- Long-running installations
- Visual continuity under variable frame timing

---

### 2. Unified Coordinate System (UV)

PolarShader uses a **unified spatial representation (UV)**:
- **Normalized UV Space:** All spatial operations are performed in a normalized [0.0, 1.0] domain mapped to `sr16`.
- **Domain Agnostic:** Transforms like rotation or zoom apply seamlessly to any pattern.
- **High Precision:** `r16/sr16 (Q16.16)` provides sufficient geometric headroom for large-scale translations and zooms.
- **Specialized Cartesian Domain:** `sr8/r8` is used for lattice/noise-style Cartesian internals where 8 fractional bits are enough and integer-grid behavior is the priority.

`r16` and `r8` are both ratio/range formats, but they serve different purposes:
- `r16/sr16`: high-precision transform space (UV, composition, smooth motion).
- `r8/sr8`: lower-fractional precision Cartesian/noise space (grid cells, lattice sampling, fast domain math).

---

### 3. Shader-Like Composition API

Effects are built by stacking **small, pure transforms**, similar to GPU shaders:

```cpp
pipeline
  .addTransform(RotationTransform(noise(cPerMil(120))))
  .addTransform(KaleidoscopeTransform(6, true))
  .addTransform(ZoomTransform(sine(cPerMil(100))))
```

Each transform:

* Has a **single responsibility**
* Is stateless (or uses an explicit integrator)
* Can be reordered, removed, or reused

---

## Key Concepts

### Signals

PolarShader uses `Sf16Signal` factories for time-varying scalar control values.

Signal kinds:

* `PERIODIC`: waveform receives scene `elapsedMs` directly.
* `APERIODIC`: waveform receives a relative time derived from `duration` and `LoopMode`.
  - `LoopMode::RESET` wraps by `elapsedMs % duration`.
  - `duration == 0` emits 0.

Factory families:

* Periodic factories (`sine`, `noise`) share one signature:
  - `(speed, amplitude, offset, phaseOffset)`.
  - `speed` is signed turns/second and independent of scene duration.
* Aperiodic factories (`linear`, `quadraticIn`, `quadraticOut`, `quadraticInOut`) share:
  - `(duration, loopMode)`.

Output contract:

* `Sf16Signal` emits values in `[-1, 1]`.
* Periodic factories span `[-1, 1]` by default.
* Internal phase integration uses signed speed for direction reversal.

---

### Phase vs Angle

PolarShader clearly distinguishes:

* **`f16` angle samples**
  A wrapped turn-domain angle (0..65535 -> 0..1 turn)

* **`PhaseAccumulator` internal phase state**
  A higher-precision integrated phase used for smooth periodic motion

This avoids:

* Quantization jitter
* Hidden precision loss
* Accidental misuse of angles as phases

---

### Motion as Integration

Motion is not implicit.
All motion goes through **explicit integrators**:

* Phase accumulators for rotation
* Mapped-signal accumulators for relative mapped/UV domains
* Cartesian integrators for translation and domain warp drift

This ensures:

* Smooth motion under variable `dt`
* Correct wrap semantics
* Centralized timing logic

---

### Unit Naming Cheat Sheet

| Type | Signed | Format | Typical Use |
| :--- | :--- | :--- | :--- |
| `f16` | No | Q0.16 | Unit fractions and wrapped angle-like values in `[0, 1)` domains |
| `sf16` | Yes | Q0.16 | Signed scalar signals/modulation in `[-1, 1]` |
| `r16` | No | Q16.16 | Unsigned high-precision ratio/range values (not implicitly `[0, 1]`) |
| `sr16` | Yes | Q16.16 | UV transform/composition space with high fractional precision |
| `r8` | No | Q24.8 | Unsigned Cartesian/noise-domain coordinates and depth sampling |
| `sr8` | Yes | Q24.8 | Signed lattice/grid Cartesian internals for pattern math |

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
