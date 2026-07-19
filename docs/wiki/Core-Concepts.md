# Core Concepts

PolarShader treats LED effects as **deterministic programs over space and time**: explicit geometry,
explicit motion, explicit units, composed from small parts. This page covers the entities you compose
with and the design choices behind them.

## The rendering model

A frame flows through a fixed hierarchy:

```
PolarRenderer → SceneManager → Scene → Layer(s)
                                          ↓
                       Pattern → UV Transforms → Palette → CRGB
```

### Entities

- **Scene** — a temporal container of one or more layers with a duration. It converts global time into
  relative elapsed time and normalized progress `[0.0, 1.0]`, which drives every signal deterministically.
- **Layer** — the core rendering unit. It chains a pattern → UV transforms → palette lookup into a single
  composited `CRGB` per pixel, with its own palette, blend mode (Normal / Add / Multiply / Screen), and
  alpha. Multiple layers stack to build up an effect.
- **Pattern** — a stateless spatial sampler. Given a coordinate it emits an intensity, a hue+brightness
  pair, or a full RGB value. See [Patterns](Patterns.md).
- **Transform** — a pure warp applied in sequence: geometric (rotate, zoom, translate, vortex,
  kaleidoscope, tiling, flow field) or palette (hue remap, clipping, tinting). See [Transforms](Transforms.md).
- **Signal** — a time-varying scalar in `[-1, 1]` that drives parameters (a transform's rotation speed, a
  pattern's scale, …). Signals are what make a scene move. See [Signals](Signals.md).
- **PipelineContext** — per-frame mutable state (time, zoom scale, palette offset, clipping ranges, raster
  geometry) written once per frame and read-only while sampling.

### Frame lifecycle

Rendering is split into distinct phases — this split is what makes dual-core rendering safe:

1. **`compile()`** — runs when a scene is created or replaced; builds stable sampler chains (one per core
   on RP2040).
2. **`advanceFrame(progress, elapsedMs)`** — updates all mutable frame state: signals, phase
   accumulators, cached pattern parameters.
3. **`sample()`** — for each pixel, threads the coordinate through pattern → transforms → palette to yield
   a `CRGB`. It reads cached state only and never mutates, so it's safe to run concurrently.

Anything that samples signals, advances accumulators, or mutates state must happen in `advanceFrame()`,
before sampling starts.

## Design choices

### Fixed-point arithmetic (no floats)

All math uses strongly-typed fixed-point formats — no `float`, no `double`. Strong typing prevents
accidentally mixing an angle with a scalar, or a coordinate with a ratio, and keeps output identical
across compilers, MCUs, and reboots (essential for long-running installations and smooth motion under
variable frame timing).

The type system uses FastLED's `fl` fixed-point types plus two angle/signal-domain types:

| Type | Signed | Format | Typical use |
|------|--------|--------|-------------|
| `f16` | No | Q0.16 | Unit fractions and wrapped angle-like values in `[0, 1)` |
| `sf16` | Yes | Q0.16 | Signed scalar signals / modulation in `[-1, 1]` |
| `fl::u16x16` | No | Q16.16 | Unsigned high-precision ratio / range values |
| `fl::s16x16` | Yes | Q16.16 | UV transform / composition space (high fractional precision) |
| `fl::u24x8` | No | Q24.8 | Unsigned Cartesian / noise-domain coordinates, depth sampling |
| `fl::s24x8` | Yes | Q24.8 | Signed lattice / grid Cartesian internals for pattern math |

`fl::s16x16`/`u16x16` (Q16.16) gives the fractional headroom for smooth motion and large zooms;
`fl::s24x8`/`u24x8` (Q24.8) is used where integer-grid behaviour matters more than sub-pixel smoothness
(noise lattices, cell sampling). Definitions live in `src/renderer/pipeline/maths/units/Units.h`.

### Unified UV / polar coordinate model

Every spatial transform operates on one **normalized UV space** mapped to `fl::s16x16`, independent of
whether the underlying pattern is polar (rings) or Cartesian (matrix). The same rotation, zoom, or
kaleidoscope therefore applies to any display, and the layer performs polar↔Cartesian conversions only at
the boundaries that need them. This separates *geometric intent* from *coordinate representation*, so the
transform library works across round LED rings and matrix grids without duplication.

### 16-bit angle domain (modular turns)

Angles are `uint16_t` where `65536` = one full turn (so `32768` = half a turn). Wrapping is automatic via
unsigned modular arithmetic — no `% 360` or `% 2π` — and trig uses FastLED's `sin16`/`cos16` lookups.
Angles are a distinct type from `f16` fractions, so the compiler stops you from using one as the other.

### Phase vs angle, and motion as integration

Motion is never implicit. It always goes through explicit integrators:

- **`f16` angle samples** are wrapped turn-domain angles (`0..65535 → 0..1` turn).
- **`PhaseAccumulator`** holds a higher-precision integrated phase for smooth periodic motion (rotation),
  avoiding quantization jitter and hidden precision loss.
- Mapped-signal and Cartesian integrators handle translation and domain-warp drift.

Integrating phase (rather than recomputing an angle each frame) yields smooth motion under variable `dt`
and correct wrap semantics, with timing logic centralized instead of scattered through patterns.

### No RTTI, no exceptions

The firmware builds with `-fno-rtti -fno-exceptions`. There's no `dynamic_cast`/`typeid` and no
`try`/`catch`; the engine uses static polymorphism (templates, functors, discriminated variants) and
compile-time strong typing instead. RTTI bloats binary size and exceptions add hidden control-flow cost —
both are luxuries an MCU's tight memory and timing budgets can't spare.

### Dual-core-safe rendering

On RP2040 dual-core builds, Core 0 owns frame timing, scene transitions, and `FastLED.show()`. Scene
compilation happens only on scene change and builds one compiled sampler chain **per core** from the same
logical scene. During rendering each core samples only its own chain — there's no per-frame handoff or
copying between cores — which is why the read-only `sample()` phase must never mutate shared state.

### Determinism & performance

- No heap allocation in the per-frame hot path (scene/layer compilation may allocate only on scene change).
- No virtual dispatch in inner loops; small, predictable codegen.
- Suitable for 48–133 MHz MCUs and long-running installations where visual continuity matters.

## Where PolarShader fits

PolarShader handles **geometry and time**; FastLED handles **LED I/O and color**. They're used together —
FastLED drives the pixels and palettes, PolarShader decides what each pixel should be. It is intentionally
narrow: not a general math library, not a floating-point abstraction, not a scene graph, not a GPU
replacement.
