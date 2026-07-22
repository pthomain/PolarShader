# Displays

A **display spec** binds a logical LED layout to a physical wiring order. Geometry is defined in C++ at
compile time (`src/*DisplaySpec.h`); the composer selects which one to render at runtime.

## Supported displays

| Display | Layout | LEDs | Spec |
|---------|--------|------|------|
| Fabric | 20×20 matrix | 400 | `FabricDisplaySpec.h` |
| Round | radial | 241 | `RoundDisplaySpec.h` |
| Fabric 32×8 | 32×8 matrix | 256 | `Fabric32x8DisplaySpec.h` |
| SmartMatrix 128×128 | 128×128 HUB75 (four 64×64 panels) | 16 384 | `Matrix128x128DisplaySpec.h` |
| Fibonacci | 12 spiral arms × 27 | 324 | `FibonacciDisplaySpec.h` |

Base classes: **`PolarDisplaySpec`** for polar (angle/radius) layouts like Round and Fibonacci, and
**`MatrixDisplaySpec`** for Cartesian grids like Fabric and the matrices.

## Selecting a display in the composer

Choose the display from the dropdown, or set it with the `?display=` query parameter — see the id table
in the [Web Composer Guide](Web-Composer-Guide.md#selecting-a-display). Because geometry is compile-time,
switching displays reloads the composer with the new selection; `src/display/WebDisplayGeometry.h`
supplies the points/center/diameter the WASM canvas uses to render each layout.

## Polar vs matrix

The pipeline works on a [unified UV space](Core-Concepts.md#unified-uv--polar-coordinate-model), so the
same patterns and transforms run on both polar and matrix displays. Some raster (pixel-grid) patterns are
matrix-oriented and accept only palette-domain transforms — see [Patterns](Patterns.md).

## Firmware targets

Each display maps to a PlatformIO deploy env and firmware entry point; see
[Deploying to Hardware](Deploying-to-Hardware.md) for the full target table.
