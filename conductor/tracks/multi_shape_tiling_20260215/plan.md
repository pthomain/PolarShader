# Implementation Plan: Multi-Shape Cartesian Tiling

## Phase 1: Infrastructure & Setup
- [ ] Task: Define `TileShape` enum in `CartesianTilingTransform.h`.
- [ ] Task: Update `CartesianTilingTransform` constructor and member variables to support `TileShape`.
- [ ] Task: Create a base test suite in `test/test_tiling/test_tiling.cpp` verifying existing square tiling functionality.
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Infrastructure & Setup' (Protocol in workflow.md)

## Phase 2: Triangle Tiling Implementation
- [ ] Task: Write failing tests for triangle grid UV mapping and cell ID generation.
- [ ] Task: Implement triangle tiling logic in `CartesianTilingTransform.cpp`.
    - [ ] Calculate triangle grid cell IDs based on UV input.
    - [ ] Calculate local UV coordinates within the equilateral triangle.
    - [ ] Ensure seamless "Up/Down" alternating orientation.
- [ ] Task: Verify triangle tiling tests pass and coverage is >80%.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Triangle Tiling Implementation' (Protocol in workflow.md)

## Phase 3: Hexagon Tiling Implementation
- [ ] Task: Write failing tests for hexagon grid UV mapping and cell ID generation.
- [ ] Task: Implement hexagon tiling logic in `CartesianTilingTransform.cpp`.
    - [ ] Implement axial or cube coordinate conversion for hex grids.
    - [ ] Calculate hexagon cell IDs.
    - [ ] Calculate local UV coordinates within the pointy-top hexagon.
- [ ] Task: Verify hexagon tiling tests pass and coverage is >80%.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Hexagon Tiling Implementation' (Protocol in workflow.md)

## Phase 4: Integration & Refinement
- [ ] Task: Ensure consistent coordinate scaling across all three shapes (Square, Triangle, Hexagon).
- [ ] Task: Run full test suite including `test_pipeline` to ensure no regressions in existing presets.
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Integration & Refinement' (Protocol in workflow.md)
