# PolarShader Wiki

PolarShader is a deterministic, fixed-point LED shader engine for microcontrollers (Seeeduino XIAO
SAMD21, XIAO RP2040, Teensy 4.1). You compose animated effects from stackable transforms and signals in
a browser-based composer, then flash them to your board — with no floating-point math anywhere in the
pipeline.

New here? Start with **[Getting Started](Getting-Started.md)**, or
**[try the composer live](https://pthomain.github.io/PolarShader/composer/)** in your browser.

## Contents

### Guides
- **[Getting Started](Getting-Started.md)** — install prerequisites, run the local composer, build and save your first scene.
- **[Web Composer Guide](Web-Composer-Guide.md)** — the composer UI in depth: displays, scenes, playlists, embed mode, and hosted-vs-local differences.
- **[Deploying to Hardware](Deploying-to-Hardware.md)** — PlatformIO targets, the deploy flow, and how playlists are embedded into firmware.
- **[Exporting GIFs and WebP](Exporting-GIFs-and-WebP.md)** — render any composition into a seamlessly looping GIF, WebP, or animated PNG.

### Concepts & reference
- **[Core Concepts](Core-Concepts.md)** — the Scene / Layer / Pattern / Transform / Signal model and the key design choices (fixed-point, unified UV space, dual-core rendering).
- **[Patterns](Patterns.md)** — catalog of built-in procedural patterns.
- **[Transforms](Transforms.md)** — catalog of composable transforms.
- **[Signals](Signals.md)** — the signal system that drives all animation.
- **[Displays](Displays.md)** — supported display geometries and how layout is selected.
- **[PSC Format](https://github.com/pthomain/PolarShader/blob/main/docs/psc-format.md)** — the binary scene format shared by the composer, WASM renderer, and firmware.
