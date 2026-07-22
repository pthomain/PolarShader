// SPDX-License-Identifier: GPL-3.0-or-later
//
// Authors the default hero composition (tools/gif/hero.psc) via the web codec,
// so the seamless-loop GIF pipeline is testable before a user supplies their
// own .psc. Run from the repo root:
//
//   node tools/gif/build_hero_psc.mjs
//
// The composition is a kaleidoscopic, palette-clipped basic-Perlin-noise field
// that loops seamlessly over 10 s (noiseBasicLoop, tag 0x3C). Swap in a
// user-supplied .psc — using noiseBasicLoop with loopPeriodMs=10000 — and
// re-render to change the look.

import { writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import { encodeScene } from '../../web/sketches/composer/codec.js';

const HERO_SCENE = {
    paletteId: 0, // Rainbow
    pattern: {
        id: 'noiseBasicLoop',
        config: { loopPeriodMs: 10000 },
        // Slow depth drift so the 10 s loop evolves gently.
        signals: { depthSpeed: { id: 'constant', params: { permille: 150 } } },
    },
    transforms: [
        {
            // Palette clip: crisp clipped edges (small feather) → the sparse
            // "glowing dots" look. Hue-remap tint (mode 0) through the palette.
            id: 'paletteClip',
            config: { maxFeather: 16384, tintMode: 0 },
            signals: {
                offset: { id: 'constant', params: { permille: 0 } },
                clip: { id: 'constant', params: { permille: 300 } },
            },
        },
        {
            // Kaleidoscope warps the input UV (spatial only) → seam preserved.
            id: 'kaleidoscope',
            config: { nbFacets: 6, isMirrored: 1 },
            signals: {},
        },
    ],
};

const bytes = encodeScene(HERO_SCENE);
const outPath = join(dirname(fileURLToPath(import.meta.url)), 'hero.psc');
writeFileSync(outPath, Buffer.from(bytes));
console.log(`wrote ${outPath} (${bytes.length} bytes)`);
