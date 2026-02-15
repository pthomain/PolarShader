# Implementation Plan: Rotation Accumulation Support

This plan outlines the steps to update `RotationTransform` to support integrated angular velocity.

## Phase 1: Core Implementation
Update the transform logic to support the new `isAngleTurn` parameter and accumulation mode.

- [x] Task: Update `RotationTransform.h`
    - [x] Task: Add `bool isAngleTurn` to constructor.
- [x] Task: Update `RotationTransform.cpp`
    - [x] Task: Update `MappedInputs` and `State` structs to include `isAngleTurn` and optional `PhaseAccumulator`.
    - [x] Task: Implement accumulation logic in `advanceFrame` using `PhaseAccumulator` when `isAngleTurn` is false.
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Core Implementation' (Protocol in workflow.md)

## Phase 2: Migration & Testing
Ensure all call sites are updated and the new functionality is verified with tests.

- [ ] Task: Update Existing Call Sites
    - [ ] Task: Audit and update all `RotationTransform` usages in `Presets.cpp` and examples.
- [ ] Task: Write Tests for Rotation Modes
    - [ ] Task: Create a new unit test in `test_units.cpp` verifying both absolute and accumulation modes.
    - [ ] Task: Verify bi-directional rotation in accumulation mode.
- [ ] Task: Final Build Pass
    - [ ] Task: Ensure firmware builds for all hardware targets.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Migration & Testing' (Protocol in workflow.md)
