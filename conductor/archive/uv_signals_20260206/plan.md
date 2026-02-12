# Implementation Plan: UV Signal Integration

## Phase 1: Type & Engine Updates
- [x] Task: Add `SignalAccumulator<sr16>` and `SignalAccumulator<UV>` to `SignalTypes.h`.
- [x] Task: Define `UVSignal` type alias in `SignalTypes.h`.
- [x] Task: Write unit tests for `UV` signal accumulation.

## Phase 2: Factory Methods & Helpers
- [x] Task: Add `constantUV` factory to `Signals.h`.
- [x] Task: Implement `uvSignal` helper to combine two scalar signals into a `UVSignal`.
- [x] Task: Port `TranslationTransform` to use `UVSignal` instead of internal `v32` integration.

## Phase 3: Verification [checkpoint: 22238ee]
- [x] Task: Run all tests to ensure spatial modulation is working as expected. [86e7b81]
- [x] Task: Update signal documentation. [86e7b81]
- [x] Task: Conductor - User Manual Verification 'Phase 3: Verification' (Protocol in workflow.md)
