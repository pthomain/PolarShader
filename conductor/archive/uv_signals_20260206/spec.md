# Specification: UV Signal Integration

## Goal
Integrate the unified `UV` coordinate system into the signal and modulation engine to enable complex, relative spatial animations.

## Context
The spatial pipeline now uses `UV` (two `FracQ16_16` values) for sampling. To animate these coordinates effectively (e.g., for translation or custom warp offsets), the signal engine needs to understand how to sample and accumulate `UV` values over time.

## Requirements
- **UVSignal:** Define a new signal type `UVSignal` that maps `TimeMillis` to `UV`.
- **FracQ16_16 Accumulation:** Add a `SignalAccumulator` specialization for `FracQ16_16` to support relative 16.16 signals.
- **UV Accumulation:** Add a `SignalAccumulator` specialization for `UV` to support relative spatial signals.
- **Factory Methods:** Provide basic helpers for creating `UVSignal` (e.g., `constantUV`, `uvSignal`).

## Success Criteria
- A `MappedSignal<UV>` can be resolved as a relative signal (accumulating deltas each frame).
- `TranslationTransform` can be refactored (in a future track) to use `UVSignal` for its offset.
- Unit tests verify that `UV` signals correctly accumulate over time.
