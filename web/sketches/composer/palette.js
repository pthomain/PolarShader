// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// Pure, dependency-free palette helpers shared by the composer UI and the
// Node codec tests. No window/document access so it imports cleanly under
// `node --test`.
//
// Cosine palettes use Inigo Quilez's formula, evaluated per RGB channel:
//   channel(t) = a + b * cos(2π * (c * t + d))
// Coefficients are normalized floats: a, b, d in ~0..1; c is a small frequency
// multiplier (number of colour cycles across the strip, not wrapped).

export const PALETTE_ENTRY_COUNT = 16;

const TAU = Math.PI * 2;

function clamp01(x) {
    if (x < 0) return 0;
    if (x > 1) return 1;
    return x;
}

// Evaluate one channel of the cosine palette at position t (0..1) and quantize
// to an integer 0..255. This exact quantization is the single contract shared
// by the slider preview, the saved entries, and the Node unit test.
function cosineChannel(t, a, b, c, d) {
    const value = a + b * Math.cos(TAU * (c * t + d));
    return Math.round(255 * clamp01(value));
}

// coeffs = { a:[r,g,b], b:[r,g,b], c:[r,g,b], d:[r,g,b] } (each channel a float).
// Returns 16 [r,g,b] integer triples sampled at t = i / 16.
export function cosinePaletteEntries(coeffs) {
    const { a, b, c, d } = coeffs;
    const entries = [];
    for (let i = 0; i < PALETTE_ENTRY_COUNT; ++i) {
        const t = i / PALETTE_ENTRY_COUNT;
        entries.push([
            cosineChannel(t, a[0], b[0], c[0], d[0]),
            cosineChannel(t, a[1], b[1], c[1], d[1]),
            cosineChannel(t, a[2], b[2], c[2], d[2]),
        ]);
    }
    return entries;
}

// A classic rainbow coefficient set (IQ's canonical example).
export function defaultCosineCoeffs() {
    return {
        a: [0.5, 0.5, 0.5],
        b: [0.5, 0.5, 0.5],
        c: [1.0, 1.0, 1.0],
        d: [0.0, 0.33, 0.67],
    };
}

// Build a CSS linear-gradient matching FastLED's 16-entry LINEARBLEND indexing
// over 0..255: entry i sits at stop i/16, and entry 0 repeats at 100% so the
// gradient spans the same index mapping the engine renders.
export function paletteGradientCss(entries) {
    const stops = [];
    for (let i = 0; i < entries.length; ++i) {
        const [r, g, b] = entries[i];
        const pct = (100 * i / entries.length).toFixed(4);
        stops.push(`rgb(${r},${g},${b}) ${pct}%`);
    }
    const [r0, g0, b0] = entries[0];
    stops.push(`rgb(${r0},${g0},${b0}) 100%`);
    return `linear-gradient(90deg, ${stops.join(', ')})`;
}

// Derive a stable, safe filename slug from a display name. Shared by the local
// server, the static localStorage library, bundle import, and tests so the same
// name always maps to the same key. Does NOT append a dedup suffix — callers
// dedup on collision.
export function paletteSlug(name) {
    const base = String(name ?? '')
        .toLowerCase()
        .trim()
        .replace(/[^a-z0-9]+/g, '-')
        .replace(/^-+|-+$/g, '')
        .slice(0, 64)
        .replace(/-+$/g, '');
    return `${base || 'palette'}.json`;
}
