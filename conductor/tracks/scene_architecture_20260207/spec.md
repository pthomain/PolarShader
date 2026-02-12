# Specification: Scene-Based Architecture and Enhanced Animation System

## Overview
This track introduces a higher-level "Scene" architecture to PolarShader, allowing for multi-layer composition with blend modes and duration-based transitions. It also overhauls the animation system by replacing basic linear/sine/pulse signals with a unified functional signal system.

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
- **Timing Terminology:**
    - **Elapsed Time:** Scalar signals are sampled with scene-relative `elapsedMs`.
    - **Speed:** Periodic signals (`sine`, `noise`) use a signed speed signal in turns/second.
    - **Duration:** Aperiodic signals (`linear`, `quadratic*`) use explicit `duration` and `LoopMode`.
        - `LoopMode::RESET` wraps by modulo.
        - `duration == 0` emits `0`.
- **Signal Logic:**
    - **Independent Phasing:** Speed-based periodic signals are independent of scene duration.
    - **Signed Phase Speed:** Internal phase accumulation preserves signed speed for bidirectional movement.
    - **Range:** `sine` and `noise` samplers span full `[0, 1]`.
    - **Output Contract:** Public scalar signal outputs clamp to `[0, 1]`.
- **API Constraints:**
    - `cPerMil` creates constant signed scalar values in f16/sf16.
    - `cPerMil16_16` (unsigned helper) is removed.
    - Scalar signals are absolute by contract (no scalar relative/absolute flag).
    - Relative accumulation behavior is isolated in mapped/UV signal accumulators.
    - **Transforms:** Must **not** accept `MappedSignal` in their public constructors. They should accept `Signal` types and/or `Ranges`, and internally construct/store a `MappedSignal`.

## Scene & Layering
- **Layer Attributes:**
    - Each `Layer` instance has an `alpha` value (sf16) and a `BlendMode`.
    - Supported `BlendMode`s: `Normal`, `Add`, `Multiply`, `Screen`.
- **Scene Class:**
    - Manages a collection of `Layer` objects.
    - Has a `durationMs`.
    - Translates absolute time to **relative time** (absolute - startTime) for its layers.
- **Scene Management:**
    - `SceneProvider` / `SceneManager`: Orchestrates scene transitions based on duration.

## Functional Requirements
- **Relative Timing:** All layers within a scene render based on the scene's elapsed time.
- **Composition:** The `Scene` composites layers from bottom to top using blend modes and alpha.
- **Fixed-Point Consistency:** All operations use the project's established Q-formats.
