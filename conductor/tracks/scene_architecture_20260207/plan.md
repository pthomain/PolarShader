# Implementation Plan: Scene Architecture and Interpolators

This plan outlines the steps to refactor the current pipeline into a layer-based scene architecture and introduce a polymorphic interpolation system for animations.

## Status Update (2026-02-07)

This file is kept as historical execution detail. Current signal behavior differs from older
entries in this plan:

- Scalar signals are elapsed-time sampled (`elapsedMs`) with `SignalKind::{PERIODIC, APERIODIC}`.
- Periodic factories use `(speed, amplitude, offset, phaseOffset)`.
- Aperiodic factories use `(duration, loopMode)` with `LoopMode::RESET` modulo behavior.
- Signed phase speed is preserved internally for phase accumulation.

## Phase 1: Core Renaming and Infrastructure
Rename existing pipeline components to reflect the new "Layer" terminology and update basic type aliases.

- [x] Task: Rename `PolarPipeline` to `Layer` and `PolarPipelineBuilder` to `LayerBuilder`. [cecceae]
    - Update `src/renderer/pipeline/PolarPipeline.h/cpp` to `src/renderer/pipeline/Layer.h/cpp`.
    - Update `src/renderer/pipeline/PolarPipelineBuilder.h/cpp` to `src/renderer/pipeline/LayerBuilder.h/cpp`.
    - Update all include guards and class names.
- [x] Task: Update Type Aliases in `src/renderer/pipeline/transforms/base/Layers.h`. [cecceae]
    - Rename `UVLayer` to `UVMap`.
    - Rename `ColourLayer` to `ColourMap`.
- [x] Task: Propagate name changes across the codebase. [cecceae]
    - Update `PolarRenderer`, `Presets`, and `test_pipeline.cpp`.
- [x] Task: Conductor - User Manual Verification 'Core Renaming and Infrastructure' (Protocol in workflow.md)

## Phase 2: Interpolator Hierarchy
Introduce the `Interpolator` base class and its various easing implementations.

- [x] Task: Create `src/renderer/pipeline/signals/interpolators/Interpolator.h`.
    - Define the base `Interpolator` interface with a `calculate(FracQ0_16 progress)` method.
- [x] Task: Implement `LinearInterpolator` and `QuadraticInterpolator` (In/Out/InOut).
- [x] Task: Implement `SinusoidalInterpolator` (In/Out/InOut).
- [x] Task: Implement `ElasticInterpolator` and `BounceInterpolator` (In/Out/InOut).
- [x] Task: Conductor - User Manual Verification 'Interpolator Hierarchy' (Protocol in workflow.md)

## Phase 3: Animation Signals API
Replace legacy linear/sine/pulse signals with the new `animate` system.

- [x] Task: Implement `animate(TimeMillis durationMs, const Interpolator& interpolator)` in `Signals.cpp/h`.
- [x] Task: Remove `sine()` and `pulse()` signal functions from `Signals.h/cpp` and `SignalSamplers.h/cpp`.
- [x] Task: Update all presets and existing code to use `animate` with appropriate interpolators to replicate previous behavior.
- [x] Task: Conductor - User Manual Verification 'Animation Signals API' (Protocol in workflow.md)

## Phase 4: Layer and Scene Architecture
Enhance `Layer` with composition properties and implement the `Scene` and `SceneManager` classes.

- [x] Task: Add `alpha` and `BlendMode` to `Layer`.
    - Update `Layer` class and `LayerBuilder` to support these properties.
- [x] Task: Implement `Scene` class in `src/renderer/pipeline/Scene.h/cpp`.
    - Manage a list of `Layer`s.
    - Handle relative timing (translation from absolute time).
    - Implement composition logic using `BlendMode`.
- [x] Task: Implement `SceneProvider` interface and `DefaultSceneProvider`.
- [x] Task: Implement `SceneManager` in `src/renderer/pipeline/SceneManager.h/cpp`.
    - Handle scene lifecycle and duration-based transitions.
- [x] Task: Update `PolarRenderer` to use `SceneManager`.
- [x] Task: Conductor - User Manual Verification 'Layer and Scene Architecture' (Protocol in workflow.md)

## Phase 5: Unified Signal & Progress-based Timing
Refactor the signal system to use normalized progress (0..1) instead of absolute time, aligning with the new Scene-based architecture.

- [x] Task: Refactor `SignalTypes.h` and `Signals.h` to use `FracQ0_16 progress` instead of `TimeMillis`. [3651167]
- [x] Task: Update `SceneManager` to calculate progress and pass it to `Scene`. [b2f91a5]
- [x] Task: Update `Scene` and `Layer` to use progress for sampling signals. [573a4b9]
- [x] Task: Remove `Interpolator` hierarchy and replace with unified `SFracQ0_16Signal` functions (easing functions). [919d7a2]
- [x] Task: Remove `animate()` function from `Signals.h/cpp`. [cecceae]
- [x] Task: Update all existing signals, transforms, and patterns to the new progress-based API. [f8e4b3c]
- [x] Task: Conductor - User Manual Verification 'Unified Signal & Progress-based Timing' (Protocol in workflow.md)

## Phase 6: Signal Refinement and Corrections
Address API regressions and enforce strict timing/phase logic.

- [x] Task: Restore Signed Phase Speed for Periodic Signals.
    - Update `sine` and `noise` to accept `SFracQ0_16Signal speed`.
    - Restore `PhaseAccumulator` usage in periodic signals to support varying speed and direction.
    - Ensure easing functions retain `Period` (TimeMillis) for looping.
- [x] Task: Update Transforms and Presets for Speed Signals.
    - Update `DomainWarpTransform` to use speed signal.
    - Fix all `Presets.cpp` calls to use `cPerMil` or similar for speed.
- [x] Task: Enforce Transform API Constraints. [fed1b49]
- [x] Task: Verify Negative Phase Support and Timing Independence. [fed1b49]
- [x] Task: Conductor - User Manual Verification 'Signal Refinement and Corrections' (Protocol in workflow.md)
