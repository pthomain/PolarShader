# Initial Concept
A fixed-point, shader-like rendering library for microcontrollers, focused on polar LED effects with deterministic math and a composable API.

# Product Definition

## Target Users
PolarShader is designed for hobbyists who want to create complex, procedural animations for small to medium-scale LED projects. These users seek a balance between creative flexibility and the performance constraints of low-power microcontrollers.

## Core Value Proposition
The primary goal of PolarShader is to enable smooth, deterministic animations on hardware that typically struggles with floating-point math. By providing a robust fixed-point infrastructure, it ensures that visual effects remain consistent and fluid across various MCU architectures.

## Key Features
- **Polar-Centric Transforms:** First-class support for radial effects like rotation, kaleidoscopes, and vortices.
- **Cartesian Support:** Full support for Cartesian transforms and coordinate mapping.
- **Unified UV Coordinate System:** A normalized spatial domain ([0, 1] mapped to Q16.16) allowing seamless composability of both Cartesian and Polar transforms.
- **Deterministic Fixed-Point Math:** Utilizes explicit Q-formats for float-free, predictable performance.
- **Composable Pipeline:** A stackable API for chaining effects, transforms, and modulators in a shader-like fashion.
- **Pattern & Noise Generation:** Built-in support for Perlin noise and other procedural patterns.
- **Library Integration:** Seamless integration with popular LED IO libraries, including FastLED and SmartMatrix.
- **Explicit Modulation:** Time-based signals (e.g., sine waves, phase velocity) for controlling motion and parameters.
