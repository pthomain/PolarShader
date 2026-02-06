# Implementation Plan: Coordinate System Unification

## Phase 1: Analysis & Type Definition [checkpoint: c75bbbf]
- [x] Task: Audit existing coordinate types in `src/renderer/pipeline/units/` and `src/renderer/pipeline/maths/`.
- [x] Task: Define a unified `UV` coordinate structure using `FracQ16_16`. [8194f1f]
- [x] Task: Conductor - User Manual Verification 'Phase 1: Analysis & Type Definition' (Protocol in workflow.md)

## Phase 2: Math & Conversion Refactoring
- [ ] Task: Update `CartesianMaths.h` and `PolarMaths.h` to operate on normalized UV types.
- [ ] Task: Implement/Refactor conversion functions between Cartesian UV and Polar UV.
- [ ] Task: Write unit tests for coordinate conversions to ensure precision and wrap behavior.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Math & Conversion Refactoring' (Protocol in workflow.md)

## Phase 3: Transform Implementation Update
- [ ] Task: Update base `Transform` and `PipelineStep` interfaces to use the unified UV type.
- [ ] Task: Refactor `RotationTransform`, `ZoomTransform`, and `TranslationTransform` to the new system.
- [ ] Task: Update `NoisePattern` and `HexTilingPattern` to sample using normalized UV coordinates.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Transform Implementation Update' (Protocol in workflow.md)

## Phase 4: Verification & Cleanup
- [ ] Task: Run all pipeline tests to ensure no regressions in effect rendering.
- [ ] Task: Update `README.md` and module documentation to reflect the unified coordinate system.
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Verification & Cleanup' (Protocol in workflow.md)
