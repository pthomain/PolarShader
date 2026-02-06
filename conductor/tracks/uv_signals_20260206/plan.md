# Implementation Plan: UV Signal Integration

## Phase 1: Type & Engine Updates
- [ ] Task: Add `SignalAccumulator<FracQ16_16>` and `SignalAccumulator<UV>` to `SignalTypes.h`.
- [ ] Task: Define `UVSignal` type alias in `SignalTypes.h`.
- [ ] Task: Write unit tests for `UV` signal accumulation.

## Phase 2: Factory Methods & Helpers
- [ ] Task: Add `constantUV` factory to `Signals.h`.
- [ ] Task: Implement `uvSignal` helper to combine two scalar signals into a `UVSignal`.
- [ ] Task: Port `TranslationTransform` to use `UVSignal` instead of internal `SPoint32` integration.

## Phase 3: Verification
- [ ] Task: Run all tests to ensure spatial modulation is working as expected.
- [ ] Task: Update signal documentation.
