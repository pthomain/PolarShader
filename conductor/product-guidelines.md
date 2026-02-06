# Product Guidelines

## Development Philosophy
- **Clarity & Composability:** Prioritize modular, readable code that reflects the "shader-like" philosophy. Components should be easy to understand in isolation and simple to combine into complex pipelines.
- **Deterministic Fixed-Point:** Every operation must be deterministic. Avoid floating-point arithmetic and dynamic memory allocation in the hot path.

## API Design
- **Fluent & Declarative:** Provide a builder-style API that allows users to declare the rendering pipeline in a readable, sequential manner.
- **Strong Typing:** Use strong types (e.g., `AngleUnitsQ0_16`, `FracQ16_16`) to prevent unit confusion and ensure math correctness at compile-time.
- **Balanced Configuration:** Support runtime modulation of parameters and signal sources, while maintaining a stable or semi-static pipeline structure to ensure performance.

## Implementation Standards
- **Error Handling:** Lean on predictable, documented overflow behavior (wrap or saturation). Use deterministic math rules to ensure consistent results across platforms.
- **Documentation:** Focus on high-quality `README.md` files for each major module that explain the architecture, math, and usage examples. Inline comments should be used sparingly and only to explain "why" complex logic is necessary.
- **Performance:** While clarity is key, ensure that inner loops are free of virtual dispatch and heap churn. Keep the memory footprint predictable.
