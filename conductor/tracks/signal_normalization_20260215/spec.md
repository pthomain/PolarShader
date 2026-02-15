# Specification: Signal and Range Normalization

## Overview
This track aims to standardize the behavior of signals and ranges across the PolarShader library. It focuses on ensuring predictable modulation scaling, consistent boundary handling for ranges, and maintaining high precision using Q16.16 fixed-point arithmetic. The goal is a robust, fully tested foundation for all procedural animations.

## Functional Requirements
- **Standardized Modulation:**
    - Normalize frequency and phase modulation behavior across all signal types (Periodic, Noise, Aperiodic, Composite).
    - Ensure modulation scales predictably regardless of the source signal's sampling rate or type.
- **Unified Range Behavior:**
    - Implement consistent overflow/boundary policies for all `Range` types.
    - `AngleRange`: Mandatory wrapping behavior for values outside the unit circle.
    - `BipolarRange` / `MagnitudeRange`: Predictable clamping or scaling policies.
- **Precision Integrity:**
    - Enforce Q16.16 (r16/sr16) fixed-point representation across all signal calculations and range mappings to prevent precision loss.
- **Comprehensive Sampler Support:**
    - Apply normalization to `sine`, `triangle`, `square`, `sawtooth`, `linear`, `quadratic`, `exponential`, and all Noise (Perlin/Worley) samplers.

## Non-Functional Requirements
- **Performance:** Maintain fixed-point performance targets without introducing floating-point overhead.
- **Determinism:** Ensure signal output is identical across different MCU architectures for the same input time and parameters.

## Acceptance Criteria
- [ ] All signals produce outputs within their expected mathematical ranges when sampled.
- [ ] Frequency modulation of a periodic signal by another signal (e.g., Noise) results in a stable output within defined bounds.
- [ ] `Range` types correctly handle edge-case inputs (0.0, 1.0, and out-of-bounds values) according to their specific policy (wrap vs. clamp).
- [ ] Unit tests achieve >80% coverage for the `signals` and `ranges` directories.

## Testing Strategy
- **Edge Case Suite:** Test 0 frequency, negative time, and INT32_MIN/MAX values.
- **Modulation Sweeps:** Verify that signal-on-signal modulation does not cause overflows or unexpected discontinuities.
- **Range Invariance:** Confirm that `Range::map()` operations always return valid Q16.16 values.

## Out of Scope
- Adding new complex pattern samplers (e.g., Voronoi).
- Changing the high-level `Scene` or `Layer` composition logic, except where it interacts with the underlying signal API.