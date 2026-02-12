# Specification: Range Unification and UV Optimization

## Goal
Simplify the range system by replacing redundant linear mapping classes with a generic `LinearRange<T>` and introducing specialized `UVRange` support.

## Context
Currently, the codebase has many range types (`CartRange`, `ZoomRange`, `SFracRange`, etc.) that perform identical linear interpolation but return different units. With the unification of spatial operations under `SQ16_16` and `UV`, these can be simplified.

## Requirements
- **Generic LinearRange:** Implement a `LinearRange<T>` template that can replace most existing range types.
- **UVRange:** Implement a `UVRange` class that can map a 0..1 signal into a 2D `UV` area (Min UV to Max UV).
- **Deprecation:** Remove or alias `CartRange`, `ZoomRange`, and `SFracRange` once `LinearRange` is in place.
- **Maintain Specialized Logic:** Keep `PolarRange` for angle-wrapping logic and `PaletteRange` for palette-specific constraints.

## Success Criteria
- The number of range classes is significantly reduced.
- `TranslationTransform` and `ZoomTransform` use unified range types.
- All unit tests pass, verifying that signal mapping remains accurate.
