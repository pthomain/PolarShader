# Implementation Plan: Scene Architecture and Interpolators

This plan outlines the steps to refactor the current pipeline into a layer-based scene architecture and introduce a polymorphic interpolation system for animations.

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
- [ ] Task: Conductor - User Manual Verification 'Core Renaming and Infrastructure' (Protocol in workflow.md)

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
