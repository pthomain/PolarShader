# Specification: Multi-Shape Cartesian Tiling

## Overview
This track involves extending the `CartesianTilingTransform` to support non-square tile shapes, specifically regular triangles and regular hexagons. The goal is to provide a unified tiling mechanism that allows patterns to be repeated across different geometric grids while maintaining consistent spatial proportions.

## Functional Requirements
- **Shape Selection:** Introduce a `TileShape` enum (e.g., `SQUARE`, `TRIANGLE`, `HEXAGON`) to determine the tiling geometry.
- **Unified Scale:** For non-square shapes, a single `scale` parameter will define the size, maintaining the regular geometric proportions (equilateral triangles, regular hexagons).
- **Seamless Tiling:** The implementation must ensure that triangles and hexagons tile perfectly without gaps or overlaps.
- **Consistent Coordinate Mapping:** Internal UV coordinates will use a consistent scale across shapes. A pattern (like a circle) will maintain its aspect ratio and relative size regardless of the tile shape it is rendered within.
- **Defaults:** Use standard "Pointy-top" orientation for hexagons and alternating "Up/Down" orientation for triangles to ensure a seamless grid.

## Non-Functional Requirements
- **Performance:** Tiling calculations must use fixed-point arithmetic (Q16.16) and remain efficient for real-time MCU rendering.
- **API Consistency:** The extension should integrate naturally into the existing `CartesianTilingTransform` class without breaking existing square tiling functionality.

## Acceptance Criteria
- [ ] `CartesianTilingTransform` supports `SQUARE`, `TRIANGLE`, and `HEXAGON` modes.
- [ ] Triangle tiling forms a continuous, gapless grid.
- [ ] Hexagon tiling forms a continuous, gapless grid.
- [ ] UV coordinates within non-square tiles are spatially consistent with the global scale.
- [ ] Existing square tiling remains unaffected.

## Out of Scope
- Configurable orientations (e.g., flat-top hexagons).
- Stretched/Non-regular polygons.
- Custom tiling offsets beyond the standard geometric grids.
