# Technology Stack

## Core Language & Standards
- **Language:** C++17
- **Standards:** Arduino Framework

## Build & Development
- **Build System:** PlatformIO
- **Target Platforms:**
  - SAMD21 (e.g., Seeeduino XIAO)
  - Teensy 4.x (using Teensyduino)
  - ESP32
  - RP2040

## Primary Libraries
- **FastLED:** Primary library for LED IO and color management.
- **SmartMatrix:** Used for high-performance matrix display driving on supported platforms (e.g., Teensy).

## Hardware & Architecture
- **Focus:** 32-bit MCUs with sufficient clock speed for real-time rendering.
- **Constraint:** Fixed-point arithmetic only; no reliance on hardware floating-point units (FPU).
