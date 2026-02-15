# Implementation Plan: Rotation Accumulation Support

This plan outlines the steps to update `RotationTransform` to support integrated angular velocity.

## Phase 1: Core Implementation [checkpoint: b395f30]
Update the transform logic to support the new `isAngleTurn` parameter and accumulation mode.

- [x] Task: Update `RotationTransform.h` b395f30
    - [x] Task: Add `bool isAngleTurn` to constructor.
- [x] Task: Update `RotationTransform.cpp` b395f30
    - [x] Task: Update `MappedInputs` and `State` structs to include `isAngleTurn` and optional `PhaseAccumulator`.
    - [x] Task: Implement accumulation logic in `advanceFrame` using `PhaseAccumulator` when `isAngleTurn` is false.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Core Implementation' (Protocol in workflow.md) b395f30

## Phase 2: Migration & Testing [checkpoint: e384c7d]
Ensure all call sites are updated and the new functionality is verified with tests.

- [x] Task: Update Existing Call Sites e384c7d
    - [x] Task: Audit and update all `RotationTransform` usages in `Presets.cpp` and examples.
- [x] Task: Write Tests for Rotation Modes e384c7d
    - [x] Task: Create a new unit test in `test_units.cpp` verifying both absolute and accumulation modes.
    - [x] Task: Verify bi-directional rotation in accumulation mode.
- [x] Task: Final Build Pass e384c7d
    - [x] Task: Ensure firmware builds for all hardware targets.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Migration & Testing' (Protocol in workflow.md) e384c7d
