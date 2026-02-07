# Specification: Math Inlining and Unit Consolidation Cleanup

## Goal
Improve rendering performance by inlining math functions and consolidate unit definitions into a single header.

## Context
Fixed-point math functions are called millions of times per second. Moving them to headers as `inline` or `constexpr` allows the compiler to optimize the hot paths effectively.

## Requirements
- **Consolidate Units:** Move all strong types and constants into `Units.h`.
- **Inline Maths:** Move implementations from `ScalarMaths`, `AngleMaths`, `PatternMaths`, and `ZoomMaths` into their respective headers.
- **Maintain Scoping:** Ensure `TimeMillis` remains in the global namespace for Arduino compatibility.
- **Verification:** Ensure all native and hardware builds continue to compile and pass tests.

## Success Criteria
- [x] Unit headers consolidated.
- [x] Math implementations inlined in headers.
- [x] Native tests pass.
- [x] Teensy hardware build succeeds.
