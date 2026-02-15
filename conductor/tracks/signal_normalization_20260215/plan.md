# Implementation Plan: Signal and Range Normalization

This plan outlines the steps to normalize signal modulation and range handling, ensuring consistency and precision across the PolarShader library.

## Phase 1: Range Normalization & Boundary Enforcement [checkpoint: 3dd1f9a]
Focus on standardizing how `AngleRange`, `BipolarRange`, and `MagnitudeRange` handle inputs and overflows.

- [x] Task: Audit and Refactor Range Classes 52c9289
    - [x] Task: Standardize `Range.h` base interface for Q16.16 mapping.
    - [x] Task: Implement explicit wrapping in `AngleRange.h`.
    - [x] Task: Implement strict clamping/scaling in `BipolarRange.h` and `MagnitudeRange.h`.
- [x] Task: Write Tests for Range Boundaries 52c9289
    - [x] Task: Create `test/test_units/test_ranges.cpp`.
    - [x] Task: Test `AngleRange` with values < 0 and > 1 (verify wrapping).
    - [x] Task: Test `MagnitudeRange` and `BipolarRange` with extreme Q16.16 values (verify clamping).
- [x] Task: Conductor - User Manual Verification 'Phase 1: Range Normalization' (Protocol in workflow.md) 3dd1f9a

## Phase 2: Signal Modulation Standardization
Standardize the modulation API across periodic, noise, and aperiodic signals.

- [ ] Task: Standardize Signal Sampler Interface
    - [ ] Task: Update `SignalSamplers.h` to ensure consistent modulation scaling.
    - [ ] Task: Refactor `sine`, `triangle`, `square`, `sawtooth` to use unified modulation logic.
- [ ] Task: Normalize Noise and Aperiodic Modulation
    - [ ] Task: Update `NoiseMaths` and `NoisePattern` to adhere to standardized modulation scaling.
    - [ ] Task: Ensure `linear`, `quadratic`, and `exponential` ramps handle modulation parameters consistently.
- [ ] Task: Write Tests for Modulation Behavior
    - [ ] Task: Create `test/test_signals/test_modulation.cpp`.
    - [ ] Task: Test frequency modulation (FM) consistency across different carrier/modulator pairs.
    - [ ] Task: Verify modulation depth scales correctly in Q16.16.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Signal Modulation' (Protocol in workflow.md)

## Phase 3: Precision Audit & Comprehensive Testing
Final sweep to ensure no precision loss and maximum test coverage.

- [ ] Task: Q16.16 Precision Audit
    - [ ] Task: Review all intermediate calculations in `Signals.cpp` and `SignalSamplers.cpp` for overflow risk.
    - [ ] Task: Replace any remaining implicit casts with explicit Q-format math.
- [ ] Task: Final Coverage Push
    - [ ] Task: Ensure `signals/` and `ranges/` directories have >80% code coverage.
    - [ ] Task: Run full suite of "Edge Case" tests (0 frequency, negative time).
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Precision & Coverage' (Protocol in workflow.md)