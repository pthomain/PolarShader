# Specification: Unit Consolidation and Type Documentation

## Goal
Simplify the fixed-point type system by merging redundant types and adding comprehensive documentation to the unit library.

## Context
The project uses many specialized fixed-point units (`TrigQ0_16`, `sf16`, etc.) that are structurally identical but used in different contexts. Merging these reduces complexity and improves developer ergonomics.

## Requirements
- **Merge TrigQ0_16:** Replace all usages of `TrigQ0_16` with `sf16`.
- **Cleanup Dead Code:** Remove `CartesianMotionAccumulator` as it is superseded by `UVSignal`.
- **Unit Documentation:** Add Definition, Usage, and Analysis comments to every unit header.
- **Guideline Integration:** Add a "Strong Type System" section to the project guidelines.

## Success Criteria
- Redundant types are removed.
- All unit tests pass.
- Guidelines are updated.
