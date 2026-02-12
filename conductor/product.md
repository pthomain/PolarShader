# Initial Concept
A fixed-point, shader-like rendering library for microcontrollers, utilizing a unified UV coordinate system for highly composable LED effects.

# Product Definition

## Target Users
PolarShader is designed for hobbyists who want to create complex, procedural animations for small to medium-scale LED projects. These users seek a balance between creative flexibility and the performance constraints of low-power microcontrollers.

## Core Value Proposition
The primary goal of PolarShader is to enable smooth, deterministic animations on hardware that typically struggles with floating-point math. By providing a robust fixed-point infrastructure, it ensures that visual effects remain consistent and fluid across various MCU architectures.

## Key Features
- **Unified UV Pipeline:** A normalized spatial domain ([0, 1] in r16/sr16 (Q16.16)) that unifies all spatial operations into a single, stackable transform chain.
- **Multi-Layer Composition:** Support for stacking multiple layers with per-layer alpha and standard blend modes (Normal, Add, Multiply, Screen).
- **Deterministic Fixed-Point Math:** Utilizes explicit Q-formats for float-free, predictable performance.
- **Procedural Pattern Samplers:** Built-in support for Noise (Perlin/Worley), Tiling, and Gradients, optimized for MCU performance.
- **Functional Animation System:** Unified signal-based system split into periodic (`sine`, `noise`) and aperiodic (`linear`, `quadratic*`) factories, all sampled from scene elapsed time.
- **Scene Management:** High-level `Scene` and `SceneManager` constructs for orchestrating complex multi-layered effects over time with relative timing.
- **Library Integration:** Seamless integration with popular LED IO libraries, including FastLED and SmartMatrix.
