# Implementation Plan: Legacy Cleanup and UV Unification

## Phase 1: Full Migration of Components
- [x] Task: Audit and migrate any remaining patterns (e.g., `WorleyPatterns`, `PolarGradient`) to `UVPattern`. [06b6d7e]
- [x] Task: Audit and migrate any remaining transforms (e.g., `KaleidoscopeTransform`, `DomainWarpTransform`) to `UVTransform`. [06b6d7e]
- [x] Task: Verify that all components implement the UV interface. [06b6d7e]

## Phase 2: Pipeline Core Refactoring
- [x] Task: Refactor `PipelineStep.h` to support only `UV` kind (and `Palette`). [857bf61]
- [x] Task: Update `PolarPipelineBuilder` to only accept `UVTransform` and `UVPattern`. [857bf61]
- [x] Task: Rewrite `PolarPipeline::build()` to create a single `UVLayer` chain without domain switching. [857bf61]

## Phase 3: Deletion & Cleanup
- [x] Task: Remove `CartesianPattern`, `PolarPattern` from `BasePattern.h`. [87e194e]
- [x] Task: Remove `CartesianTransform`, `PolarTransform` from `Transforms.h`. [87e194e]
- [x] Task: Remove `CartesianLayer`, `PolarLayer` from `Layers.h`. [87e194e]
- [x] Task: Remove unused legacy conversion functions from `PolarMaths` and `CartesianMaths`. [87e194e]

## Phase 4: Verification [checkpoint: e4b5672]
- [x] Task: Run all tests (`test_pipeline`, `test_units`) to ensure the unified pipeline works correctly. [87e194e]
- [x] Task: Update documentation to remove references to legacy domains. [87e194e]
- [x] Task: Conductor - User Manual Verification 'Phase 4: Verification' (Protocol in workflow.md)
