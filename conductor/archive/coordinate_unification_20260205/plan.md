# Implementation Plan: Coordinate System Unification

## Phase 1: Analysis & Type Definition [checkpoint: c75bbbf]
- [x] Task: Audit existing coordinate types in `src/renderer/pipeline/units/` and `src/renderer/pipeline/maths/`.
- [x] Task: Define a unified `UV` coordinate structure using `sr16`. [8194f1f]
- [x] Task: Conductor - User Manual Verification 'Phase 1: Analysis & Type Definition' (Protocol in workflow.md)

## Phase 2: Math & Conversion Refactoring [checkpoint: 3e393d5]
- [x] Task: Update `CartesianMaths.h` and `PolarMaths.h` to operate on normalized UV types. [90bafe8]
- [x] Task: Implement/Refactor conversion functions between Cartesian UV and Polar UV. [55a7d98]
- [x] Task: Write unit tests for coordinate conversions to ensure precision and wrap behavior. [55a7d98]
- [x] Task: Conductor - User Manual Verification 'Phase 2: Math & Conversion Refactoring' (Protocol in workflow.md)

## Phase 3: Transform Implementation Update [checkpoint: 0f342ab]
- [x] Task: Update base `Transform` and `PipelineStep` interfaces to use the unified UV type. [2141098]
- [x] Task: Refactor `RotationTransform`, `ZoomTransform`, and `TranslationTransform` to the new system. [2141098]
- [x] Task: Update `NoisePattern` and `HexTilingPattern` to sample using normalized UV coordinates. [2141098]
- [x] Task: Conductor - User Manual Verification 'Phase 3: Transform Implementation Update' (Protocol in workflow.md)

## Phase 4: Verification & Cleanup [checkpoint: 18846a5]
- [x] Task: Run all pipeline tests to ensure no regressions in effect rendering. [249fd80]
- [x] Task: Update `README.md` and module documentation to reflect the unified coordinate system. [249fd80]
- [x] Task: Conductor - User Manual Verification 'Phase 4: Verification & Cleanup' (Protocol in workflow.md)
