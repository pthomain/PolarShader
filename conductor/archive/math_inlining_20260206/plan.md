# Implementation Plan: Math Inlining and Consolidation

## Phase 1: Unit Consolidation
- [x] Task: Create unified `Units.h`. [e45ee58]
- [x] Task: Update all includes to use `Units.h`. [e45ee58]
- [x] Task: Remove individual unit headers. [e45ee58]

## Phase 2: Math Inlining
- [x] Task: Move `ScalarMaths` implementation to header. [1941174]
- [x] Task: Move `AngleMaths` implementation to header. [1941174]
- [x] Task: Move `PatternMaths` implementation to header. [1941174]
- [x] Task: Move `ZoomMaths` implementation to header. [1941174]

## Phase 3: Verification
- [x] Task: Re-calibrate `native` test suite to handle inlined maths. [1941174]
- [x] Task: Verify hardware build on Teensy 4.1. [1941174]
