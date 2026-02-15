# Implementation Plan: Multi-Shape Cartesian Tiling

## Phase 1: Infrastructure & Setup [checkpoint: e02cdbf]
- [x] Task: Define `TileShape` enum in `CartesianTilingTransform.h`. 3ce32fa
- [x] Task: Update `CartesianTilingTransform` constructor and member variables to support `TileShape`. 3ce32fa
- [x] Task: Create a base test suite in `test/test_tiling/test_tiling.cpp` verifying existing square tiling functionality. 3ce32fa
- [x] Task: Conductor - User Manual Verification 'Phase 1: Infrastructure & Setup' (Protocol in workflow.md)

## Phase 2: Triangle Tiling Implementation
- [x] Task: Write failing tests for triangle grid UV mapping and cell ID generation. 91100f6
- [x] Task: Implement triangle tiling logic in `CartesianTilingTransform.cpp`. 91100f6
    - [x] Calculate triangle grid cell IDs based on UV input.
    - [x] Calculate local UV coordinates within the equilateral triangle.
    - [x] Ensure seamless "Up/Down" alternating orientation.
- [x] Task: Verify triangle tiling tests pass and coverage is >80%.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Triangle Tiling Implementation' (Protocol in workflow.md)

## Phase 3: Hexagon Tiling Implementation
- [x] Task: Write failing tests for hexagon grid UV mapping and cell ID generation. 1e4c447
- [x] Task: Implement hexagon tiling logic in `CartesianTilingTransform.cpp`. 1e4c447
    - [x] Implement axial or cube coordinate conversion for hex grids.
    - [x] Calculate hexagon cell IDs.
    - [x] Calculate local UV coordinates within the pointy-top hexagon.
- [x] Task: Verify hexagon tiling tests pass and coverage is >80%.
- [x] Task: Conductor - User Manual Verification 'Phase 3: Hexagon Tiling Implementation' (Protocol in workflow.md)

## Phase 4: Integration & Refinement [checkpoint: b364c55]
- [x] Task: Ensure consistent coordinate scaling across all three shapes (Square, Triangle, Hexagon).
- [x] Task: Run full test suite including `test_pipeline` to ensure no regressions in existing presets.
- [x] Task: Conductor - User Manual Verification 'Phase 4: Integration & Refinement' (Protocol in workflow.md)
