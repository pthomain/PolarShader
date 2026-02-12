# Specification: Legacy Cleanup and UV Unification

## Goal
Complete the transition to the Unified UV Coordinate System by removing all legacy Cartesian/Polar pipeline logic, base classes, and bridging steps.

## Context
The project recently introduced `UVPattern` and `UVTransform` to unify spatial operations. However, the codebase is currently in a hybrid state where legacy `Cartesian` and `Polar` types coexist, and the `PolarPipeline` explicitly manages domain switching.

## Requirements
- **Single Pipeline Domain:** The `PolarPipeline` must operate exclusively on `UVLayer` and `UVTransform`.
- **Removal of Legacy Bases:** `CartesianPattern`, `PolarPattern`, `CartesianTransform`, and `PolarTransform` must be deleted.
- **Internal Units Only:** Specialized units like `SQ24_8` must be encapsulated as implementation details within specific patterns and not exposed in the pipeline interfaces.
- **Pipeline Simplification:** Remove explicit `ToCartesian` and `ToPolar` steps from the pipeline builder and processing loop.

## Success Criteria
- The project compiles without `CartesianPattern` or `PolarPattern` classes.
- `PolarPipeline` builds a single function chain without domain switching logic.
- All unit tests pass, verifying that effects render correctly through the UV pipeline.
