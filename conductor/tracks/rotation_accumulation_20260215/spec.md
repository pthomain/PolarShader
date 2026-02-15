# Specification: Rotation Accumulation Support

## Overview
This track updates `RotationTransform` to support both absolute angular turns and integrated angular velocity (accumulation). By default, rotations will now accumulate over time, enabling smooth continuous motion from standard signals like constants or low-frequency oscillators.

## Functional Requirements
- **Parameter `isAngleTurn`:**
    - Add an optional `bool isAngleTurn` parameter to the `RotationTransform` constructor.
    - Default value: `false` (Accumulation mode).
- **Absolute Mode (`isAngleTurn = true`):**
    - Treat the input signal as an absolute angular offset.
    - Sample the signal using `AngleRange` (0..1 turns).
- **Accumulation Mode (`isAngleTurn = false`):**
    - Treat the input signal as angular velocity (speed).
    - Sample the signal using `bipolarRange` (-1..1 turns per second).
    - Use `PhaseAccumulator` to integrate the speed into a continuous angular offset.
- **State Management:**
    - Update `RotationTransform::State` to conditionally manage either an absolute offset or a `PhaseAccumulator`.

## Migration Strategy
- Update all existing `RotationTransform` call sites to align with the new signature.
- Allow existing effects to switch to accumulation mode by default.

## Acceptance Criteria
- [ ] `RotationTransform` correctly rotates patterns in both modes.
- [ ] In accumulation mode, a constant signal results in a constant rotation speed.
- [ ] In absolute mode, a constant signal results in a static rotation offset.
- [ ] Automated tests verify both modes of operation.
