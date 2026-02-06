# Implementation Plan: Range Unification and UV Optimization

## Phase 1: Core Template Implementation
- [~] Task: Create `LinearRange.h` implementing a generic mapping template.
- [ ] Task: Create `UVRange.h` for mapping signals to 2D spatial areas.
- [ ] Task: Update `Range.h` to support these new types if necessary.

## Phase 2: Component Refactoring
- [~] Task: Refactor `ZoomTransform` to use `LinearRange<SFracQ0_16>`.
- [~] Task: Refactor `DomainWarpTransform` to use generic linear ranges for its many parameters.
- [~] Task: Refactor `TranslationTransform` to use `UVRange` if appropriate.

## Phase 3: Deletion & Cleanup
- [ ] Task: Delete `CartRange.h/cpp`, `ZoomRange.h/cpp`, and `SFracRange.h/cpp`.
- [ ] Task: Ensure all factory methods in `Signals.h` are compatible with the new range types.

## Phase 4: Verification
- [ ] Task: Run all native tests to ensure no mapping regressions.
- [ ] Task: Update documentation to reflect the simplified range system.
