# Implementation Plan: Range Unification and UV Optimization

## Phase 1: Core Template Implementation [checkpoint: 803f33a]
- [x] Task: Create `LinearRange.h` implementing a generic mapping template. [b1b21cb]
- [x] Task: Create `UVRange.h` for mapping signals to 2D spatial areas. [b1b21cb]
- [x] Task: Update `Range.h` to support these new types if necessary. [b1b21cb]

## Phase 2: Component Refactoring [checkpoint: d97f186]
- [x] Task: Refactor `ZoomTransform` to use `LinearRange<sf16>`. [e3db100]
- [x] Task: Refactor `DomainWarpTransform` to use generic linear ranges for its many parameters. [e3db100]
- [x] Task: Refactor `TranslationTransform` to use `LinearRange<int32_t>` internally if needed. [e3db100]

## Phase 3: Deletion & Cleanup [checkpoint: 7e3c82e]
- [x] Task: Delete all redundant linear range classes (`CartRange`, `ZoomRange`, `SFracRange`, `DepthRange`, `ScalarRange`, `PatternRange`, `PaletteRange`, `TimeRange`). [cabc9f8]
- [x] Task: Ensure all factory methods in `Signals.h` are compatible with the new range types. [cabc9f8]

## Phase 4: Verification [checkpoint: 8539237]
- [x] Task: Run all native tests to ensure no mapping regressions. [cabc9f8]
- [x] Task: Update documentation to reflect the simplified range system. [cabc9f8]
- [x] Task: Conductor - User Manual Verification 'Phase 4: Verification' (Protocol in workflow.md)