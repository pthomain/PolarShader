# Implementation Plan: Legacy Cleanup and UV Unification

## Phase 1: Full Migration of Components
- [x] Task: Audit and migrate any remaining patterns (e.g., `WorleyPatterns`, `PolarGradient`) to `UVPattern`. [06b6d7e]
- [x] Task: Audit and migrate any remaining transforms (e.g., `KaleidoscopeTransform`, `DomainWarpTransform`) to `UVTransform`. [06b6d7e]
- [x] Task: Verify that all components implement the UV interface. [06b6d7e]

## Phase 2: Pipeline Core Refactoring
- [ ] Task: Refactor `PipelineStep.h` to support only `UV` kind (and `Palette`).
- [ ] Task: Update `PolarPipelineBuilder` to only accept `UVTransform` and `UVPattern`.
- [ ] Task: Rewrite `PolarPipeline::build()` to create a single `UVLayer` chain without domain switching.

## Phase 3: Deletion & Cleanup
- [ ] Task: Remove `CartesianPattern`, `PolarPattern` from `BasePattern.h`.
- [ ] Task: Remove `CartesianTransform`, `PolarTransform` from `Transforms.h`.
- [ ] Task: Remove `CartesianLayer`, `PolarLayer` from `Layers.h`.
- [ ] Task: Remove unused legacy conversion functions from `PolarMaths` and `CartesianMaths`.

## Phase 4: Verification
- [ ] Task: Run all tests (`test_pipeline`, `test_units`) to ensure the unified pipeline works correctly.
- [ ] Task: Update documentation to remove references to legacy domains.
