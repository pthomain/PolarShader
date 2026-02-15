# Implementation Plan: Parameter Unification & Constant Signal Helpers

This plan outlines the steps to introduce `bi*` and `mag*` signal helpers and migrate the codebase to use them consistently.

## Phase 1: Core Implementation & Renaming
Rename existing helpers and implement the new Magnitude-based helpers.

- [ ] Task: Update `Signals.h` and `Signals.cpp`
    - [ ] Task: Rename `floor`, `midPoint`, `ceiling` to `biFloor`, `biMid`, `biCeil`.
    - [ ] Task: Implement `magFloor`, `magMid`, `magCeil` with unipolar-to-signed remapping.
    - [ ] Task: Update `Sf16Signal` factory defaults to use `bi*` equivalents.
- [ ] Task: Update Signal Terminology
    - [ ] Task: Ensure internal `createPeriodicSignal` and other helpers use `threshold` instead of `offset`.
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Core Implementation' (Protocol in workflow.md)

## Phase 2: Migration & Refactoring
Update all call sites in the repository to use the new names while preserving exact current behavior.

- [ ] Task: Migrate Presets and Transforms
    - [ ] Task: Audit and replace `floor`, `midPoint`, `ceiling` with `biFloor`, `biMid`, `biCeil` in `Presets.cpp`.
    - [ ] Task: Update `DomainWarpPresets.cpp` call sites.
- [ ] Task: Update Test Suites
    - [ ] Task: Refactor `test_units.cpp` and `test_modulation.cpp` to use `bi*` helpers.
    - [ ] Task: Add new tests for `mag*` helpers verifying the unipolar-to-signed remapping.
- [ ] Task: Verify Hardware Compilation
    - [ ] Task: Run `pio run -e seeed_xiao` and `pio run -e teensy41_matrix` to ensure no broken signatures.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Migration' (Protocol in workflow.md)

## Phase 3: Documentation & Finalization
Ensure all documentation reflects the new helper sets.

- [ ] Task: Update Documentation
    - [ ] Task: Update `README.md` examples.
    - [ ] Task: Update `src/renderer/pipeline/transforms/README.md` signal model section.
- [ ] Task: Final Quality Pass
    - [ ] Task: Run full `pio test -e native` suite and ensure 100% pass rate.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Documentation' (Protocol in workflow.md)
