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
    - **Speed:** Periodic signals (`sine`, `noise`) use a **Signed Phase Speed Signal** (`SFracQ0_16Signal`).
        - Units: Turns per second (1.0 = 1 full cycle per second).
        - Directional: Positive values move forward, negative values move backward.
        - Dynamic: As a signal, speed can vary over time.
    - **Period (Duration):** Non-periodic easing functions (Linear, Quadratic, etc.) use `Period` (TimeMillis).
        - Represents the duration after which the easing function resets and repeats.
        - **Default:** If `period` is 0, it defaults to the Scene's duration.
        - **Looping:** If `0 < period < scene_duration`, the signal loops.
- **Signal Logic:**
    - **Independent Phasing:** Signals using `Speed` are independent of Scene duration. They utilize a `PhaseAccumulator` to integrate speed over `elapsedTime`.
    - **Signed Phase Speed:** `PhaseAccumulator` MUST use signed speed (`MappedSignal<SFracQ0_16>`) to support bidirectional movement.
    - **Range:** `sine` and `noise` signals must span the full output range of `[0, 1]`.
- **API Constraints:**
    - `cPerMil` logic: `cPerMil` stands for "Constant per-mil". It creates a constant signed signal.
    - `cPerMil16_16` (unsigned) is removed.
    - **Transforms:** Must **not** accept `MappedSignal` in their public constructors. They should accept `Signal` types and/or `Ranges`, and internally construct/store a `MappedSignal`.

## Scene & Layering
- **Layer Attributes:**
    - Each `Layer` instance has an `alpha` value (FracQ0_16) and a `BlendMode`.
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
