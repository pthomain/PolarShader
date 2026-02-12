# Specification: Coordinate System Unification

## Goal
Establish a unified, normalized coordinate system (UV) for both Cartesian and Polar domains to ensure seamless composability of all transforms and patterns.

## Context
Currently, the codebase contains separate implementations for Cartesian and Polar logic. To achieve the "shader-like" goal, transforms must operate on a consistent spatial scale regardless of the underlying display resolution or coordinate type.

## Requirements
- **Normalized UV Space:** All spatial coordinates must be represented as normalized fixed-point values (typically 0.0 to 1.0 mapped to `SQ16_16`).
- **Origin Consistency:** The origin (0,0) must be clearly defined across all systems (e.g., center-aligned for Polar, top-left or center-aligned for Cartesian).
- **Unit Safety:** Use strong types to distinguish between normalized units and raw pixel/hardware units.
- **Composable Conversion:** Provide zero-cost or low-cost conversion mechanisms between Cartesian and Polar UV spaces within the pipeline.

## Success Criteria
- A unified coordinate type is used across all `PipelineStep` implementations.
- A "UV" coordinate can be passed through a Polar rotation followed by a Cartesian tiling transform without manual rescaling.
- Unit tests verify that coordinate transformations are deterministic and maintain precision.
