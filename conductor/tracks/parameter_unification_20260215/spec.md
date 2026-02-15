# Specification: Parameter Unification & Constant Signal Helpers

## Overview
This track introduces two distinct sets of constant signal helpers to support different mental models for parameter definition: **Bipolar** (additive offsets in signed space) and **Magnitude** (absolute positions in remapped unipolar space). It focuses on providing clear tools for defining fixed-point constants while maintaining the existing `amplitude` terminology.

## Functional Requirements
- **Rename Bipolar Helpers:**
    - Rename `floor` to `biFloor`.
    - Rename `midPoint` to `biMid`.
    - Rename `ceiling` to `biCeil`.
- **Implement Magnitude Helpers:**
    - `magFloor(uint16_t offsetPerMil)`: Clamped to `[0, 1000]`. Remaps unipolar `[0, 1]` to signed `[-1, 1]`.
    - `magMid(int16_t offsetPerMil)`: Clamped to `[-500, 500]`. Remaps unipolar `0.5 + offset` to signed `[-1, 1]`.
    - `magCeil(int16_t offsetPerMil)`: Clamped to `[-1000, 0]`. Remaps unipolar `1.0 + offset` to signed `[-1, 1]`.
- **Terminology Normalization:**
    - Ensure all signal parameters consistently use `threshold` (instead of offset) where appropriate.
    - **Note:** `amplitude` terminology remains unchanged.
- **Factory Updates:**
    - Update defaults for `sine`, `noise`, `triangle`, `square`, `sawtooth` to use the new `bi*` or `mag*` helpers.

## Migration Strategy
- **Preserve Behavior:** Perform a literal swap of existing `floor`, `midPoint`, `ceiling` calls to their `bi*` equivalents in all Presets, Transforms, and Tests.
- **Update Documentation:** Update `README.md` and module documentations to explain the two helper sets.

## Acceptance Criteria
- [ ] All existing tests pass after the renaming migration.
- [ ] `magFloor(0)` produces a signal returning `-1.0`.
- [ ] `magMid(0)` produces a signal returning `0.0`.
- [ ] `magCeil(0)` produces a signal returning `1.0`.
- [ ] No occurrences of old helper names (non-prefixed) remain in public APIs.
