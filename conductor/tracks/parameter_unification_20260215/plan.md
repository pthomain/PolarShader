# Implementation Plan: Parameter Unification & Constant Signal Helpers

This plan outlines the steps to introduce `bi*` and `mag*` signal helpers and migrate the codebase to use them consistently.

## Phase 1: Core Implementation & Renaming [checkpoint: e354438]
Rename existing helpers and implement the new Magnitude-based helpers.

- [x] Task: Update `Signals.h` and `Signals.cpp` e354438
    - [x] Task: Rename `floor`, `midPoint`, `ceiling` to `biFloor`, `biMid`, `biCeil`.
    - [x] Task: Implement `magFloor`, `magMid`, `magCeil` with unipolar-to-signed remapping.
    - [x] Task: Update `Sf16Signal` factory defaults to use `bi*` equivalents.
- [x] Task: Update Signal Terminology e354438
    - [x] Task: Ensure internal `createPeriodicSignal` and other helpers use `threshold` instead of `offset`.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Core Implementation' (Protocol in workflow.md) e354438

## Phase 2: Migration & Refactoring [checkpoint: e354438]
Update all call sites in the repository to use the new names while preserving exact current behavior.

- [x] Task: Migrate Presets and Transforms e354438
    - [x] Task: Audit and replace `floor`, `midPoint`, `ceiling` with `biFloor`, `biMid`, `biCeil` in `Presets.cpp`.
    - [x] Task: Update `DomainWarpPresets.cpp` call sites.
- [x] Task: Update Test Suites e354438
    - [x] Task: Refactor `test_units.cpp` and `test_modulation.cpp` to use `bi*` helpers.
    - [x] Task: Add new tests for `mag*` helpers verifying the unipolar-to-signed remapping.
- [x] Task: Verify Hardware Compilation e354438
    - [x] Task: Run `pio run -e seeed_xiao` and `pio run -e teensy41_matrix` to ensure no broken signatures.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Migration' (Protocol in workflow.md) e354438

## Phase 3: Documentation & Finalization [checkpoint: f6fb728]
Ensure all documentation reflects the new helper sets.

- [x] Task: Update Documentation f6fb728
    - [x] Task: Update `README.md` examples.
    - [x] Task: Update `src/renderer/pipeline/transforms/README.md` signal model section.
- [x] Task: Final Quality Pass f6fb728
    - [x] Task: Run full `pio test -e native` suite and ensure 100% pass rate.
- [x] Task: Conductor - User Manual Verification 'Phase 3: Documentation' (Protocol in workflow.md) f6fb728
