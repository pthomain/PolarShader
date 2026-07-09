// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// PSC v1 schema — mirrors src/composer/SceneCodec.cpp byte-for-byte.
// Drift between this file and the C++ tag tables is caught by the
// cross-implementation golden fixture in test/test_composer.

// ─────────────────────────────────────────────────────────────────────
// Param kinds (used in pattern.config / pattern.signals / signal.params)
//
//   'u8'         single byte 0..255             → number input
//   'u16'        little-endian uint16 0..65535  → number input
//   'u32'        little-endian uint32           → number input
//   'i32'        little-endian int32 (sf16 raw) → number input
//   'bool'       single byte 0/1                → checkbox
//   'enum'       single byte index into options → dropdown
//   'permille'   uint16 with UI label "permille" (1000 = 1.0)   → slider 0..1000
//   'f16'        uint16 raw f16 fraction (65535 ≈ 1.0)          → slider 0..65535
//   'signal'     recursive Signal blob          → SignalEditor
// ─────────────────────────────────────────────────────────────────────

export const PALETTES = [
    { id: 0, name: 'Rainbow' },
    { id: 1, name: 'Cloud' },
    { id: 2, name: 'Party' },
    { id: 3, name: 'Forest' },
];

export const LOOP_MODES        = ['RESET', 'SATURATE'];
export const NOISE_TYPES       = ['Basic', 'FBM', 'Turbulence', 'Ridged'];
export const TRANSPORT_MODES   = [
    'SpiralInward', 'SpiralOutward', 'RadialSink', 'RadialSource',
    'RotatingSwirl', 'PolarPulse', 'Shockwave', 'FractalSpiral',
    'SquareSpiral', 'AttractorField',
];
export const FLURRY_SHAPES     = ['Line', 'Ball'];
export const FLOWFIELD_MODES   = ['Lissajous', 'Dots', 'Both'];
export const RD_PRESETS        = ['Spots', 'Stripes', 'Coral', 'Worms'];
export const TILE_SHAPES       = ['SQUARE', 'TRIANGLE', 'HEXAGON'];
export const WORLEY_ALIASING   = ['None', 'Fast', 'Precise'];
export const LIFE_RULES        = ['HighLife', 'Day & Night', 'Seeds'];

// ─────────────────────────────────────────────────────────────────────
// Signal definitions. Tag values are SIG_* in SceneCodec.cpp.
// ─────────────────────────────────────────────────────────────────────

const aperiodicParams = [
    { name: 'duration',  kind: 'u32',  default: 1000, label: 'duration (ms)' },
    { name: 'loopMode',  kind: 'enum', options: LOOP_MODES, default: 0 },
];

const periodicBaseParams = [
    { name: 'phaseVelocity', kind: 'signal' },
    { name: 'phaseOffset',   kind: 'i32',    default: 0, label: 'phase (raw sf16)' },
];

const noiseBaseParams = [
    { name: 'phaseVelocity', kind: 'signal' },
];

const periodicBoundedParams = [
    { name: 'phaseVelocity', kind: 'signal' },
    { name: 'floor',         kind: 'signal' },
    { name: 'ceiling',       kind: 'signal' },
];

const periodicBoundedPhaseParams = [
    { name: 'phaseVelocity', kind: 'signal' },
    { name: 'phaseOffset',   kind: 'i32',    default: 0, label: 'phase (raw sf16)' },
    { name: 'floor',         kind: 'signal' },
    { name: 'ceiling',       kind: 'signal' },
];

export const SIGNALS = {
    constant:           { tag: 0x00, label: 'constant',        kind: 'leaf',
                          params: [{ name: 'permille', kind: 'permille', default: 500 }] },
    cRandom:            { tag: 0x01, label: 'cRandom',         kind: 'leaf', params: [] },
    linear:             { tag: 0x02, label: 'linear',          kind: 'leaf', params: aperiodicParams },
    quadraticIn:        { tag: 0x03, label: 'quadraticIn',     kind: 'leaf', params: aperiodicParams },
    quadraticOut:       { tag: 0x04, label: 'quadraticOut',    kind: 'leaf', params: aperiodicParams },
    quadraticInOut:     { tag: 0x05, label: 'quadraticInOut',  kind: 'leaf', params: aperiodicParams },

    sine:               { tag: 0x10, label: 'sine',            kind: 'modulator', params: periodicBaseParams },
    sineBounded:        { tag: 0x11, label: 'sine (bounded)',  kind: 'modulator', params: periodicBoundedParams },
    sineBoundedPhase:   { tag: 0x12, label: 'sine (b+phase)',  kind: 'modulator', params: periodicBoundedPhaseParams },
    triangle:           { tag: 0x13, label: 'triangle',        kind: 'modulator', params: periodicBaseParams },
    triangleBounded:    { tag: 0x14, label: 'triangle (b)',    kind: 'modulator', params: periodicBoundedParams },
    triangleBoundedPh:  { tag: 0x15, label: 'triangle (b+ph)', kind: 'modulator', params: periodicBoundedPhaseParams },
    square:             { tag: 0x16, label: 'square',          kind: 'modulator', params: periodicBaseParams },
    squareBounded:      { tag: 0x17, label: 'square (b)',      kind: 'modulator', params: periodicBoundedParams },
    squareBoundedPh:    { tag: 0x18, label: 'square (b+ph)',   kind: 'modulator', params: periodicBoundedPhaseParams },
    sawtooth:           { tag: 0x19, label: 'sawtooth',        kind: 'modulator', params: periodicBaseParams },
    sawtoothBounded:    { tag: 0x1A, label: 'sawtooth (b)',    kind: 'modulator', params: periodicBoundedParams },
    sawtoothBoundedPh:  { tag: 0x1B, label: 'sawtooth (b+ph)', kind: 'modulator', params: periodicBoundedPhaseParams },
    noise:              { tag: 0x1C, label: 'noise',           kind: 'modulator',
                          params: noiseBaseParams, wireParams: periodicBaseParams },
    noiseBounded:       { tag: 0x1D, label: 'noise (b)',       kind: 'modulator', params: periodicBoundedParams },
    noiseBoundedPhase:  { tag: 0x1E, label: 'noise (b+ph)',    kind: 'modulator',
                          params: periodicBoundedPhaseParams, hidden: true },
    smap:               { tag: 0x1F, label: 'smap',            kind: 'modulator',
                          params: [
                              { name: 'signal',  kind: 'signal' },
                              { name: 'floor',   kind: 'signal' },
                              { name: 'ceiling', kind: 'signal' },
                          ] },
    scale:              { tag: 0x20, label: 'scale',           kind: 'modulator',
                          params: [
                              { name: 'signal', kind: 'signal' },
                              { name: 'factor', kind: 'f16', default: 32768, label: 'factor (raw f16)' },
                          ] },
};

// Map tag → signal-id (for decoding).
export const SIGNAL_BY_TAG = (() => {
    const m = {};
    for (const [id, def] of Object.entries(SIGNALS)) m[def.tag] = id;
    return m;
})();

// Default signal model used when a fresh slot is created.
export const DEFAULT_SIGNAL = () => ({ id: 'constant', params: { permille: 500 } });

// ─────────────────────────────────────────────────────────────────────
// Pattern definitions. Tag values are PAT_* in SceneCodec.cpp.
// `config` = static-config bytes encoded BEFORE the signals.
// `signals` = ordered Sf16Signal slots encoded AFTER the config.
// ─────────────────────────────────────────────────────────────────────

export const PATTERNS = {
    noiseBasic: {
        tag: 0x00, label: 'Noise — Basic',
        config: [],
        signals: [{ name: 'depthSpeed' }],
    },
    noiseFbm: {
        tag: 0x01, label: 'Noise — FBM',
        config: [{ name: 'octaves', kind: 'u8', default: 4 }],
        signals: [],
    },
    noiseTurbulence: {
        tag: 0x02, label: 'Noise — Turbulence',
        config: [], signals: [],
    },
    noiseRidged: {
        tag: 0x03, label: 'Noise — Ridged',
        config: [], signals: [],
    },
    tiling: {
        tag: 0x04, label: 'Tiling',
        config: [
            { name: 'cellSize',   kind: 'u16', default: 1 },
            { name: 'colorCount', kind: 'u8',  default: 4 },
            { name: 'shape',      kind: 'enum', options: TILE_SHAPES, default: 2 },  // HEXAGON
        ],
        signals: [],
    },
    reactionDiffusion: {
        tag: 0x05, label: 'Reaction-Diffusion',
        config: [
            { name: 'preset',        kind: 'enum', options: RD_PRESETS, default: 2 },  // Coral
            { name: 'width',         kind: 'u8',   default: 20 },
            { name: 'height',        kind: 'u8',   default: 20 },
            { name: 'stepsPerFrame', kind: 'u8',   default: 4 },
        ],
        signals: [],
    },
    flowField: {
        tag: 0x06, label: 'Flow Field',
        config: [
            { name: 'gridSize', kind: 'u8',   default: 32 },
            { name: 'dotCount', kind: 'u8',   default: 3 },
            { name: 'mode',     kind: 'enum', options: FLOWFIELD_MODES, default: 2 },  // Both
        ],
        signals: [
            { name: 'xDrift' }, { name: 'yDrift' }, { name: 'amplitude' },
            { name: 'frequency' }, { name: 'endpointSpeed' }, { name: 'halfLife' },
            { name: 'orbitSpeed' }, { name: 'orbitRadius' },
        ],
    },
    transport: {
        tag: 0x07, label: 'Transport',
        config: [
            { name: 'gridSize',     kind: 'u8',   default: 32 },
            { name: 'mode',         kind: 'enum', options: TRANSPORT_MODES, default: 0 },
            { name: 'velocityGlow', kind: 'bool', default: 0 },
        ],
        signals: [
            { name: 'radialSpeed' }, { name: 'angularSpeed' },
            { name: 'halfLife' }, { name: 'emitterSpeed' },
        ],
    },
    spiral: {
        tag: 0x08, label: 'Spiral',
        config: [
            { name: 'armCount',  kind: 'u8',   default: 2 },
            { name: 'clockwise', kind: 'bool', default: 1 },
        ],
        signals: [
            { name: 'tightness' }, { name: 'armThickness' }, { name: 'rotationSpeed' },
        ],
    },
    annuli: {
        tag: 0x09, label: 'Annuli',
        config: [
            { name: 'ringCount',      kind: 'u8',  default: 8 },
            { name: 'slicesPerRing',  kind: 'u8',  default: 32 },
            { name: 'reverse',        kind: 'bool', default: 0 },
            { name: 'stepIntervalMs', kind: 'u16', default: 80 },
            { name: 'holdMs',         kind: 'u16', default: 800 },
        ],
        signals: [],
    },
    flurry: {
        tag: 0x0A, label: 'Flurry',
        config: [
            { name: 'gridSize',  kind: 'u8',   default: 32 },
            { name: 'lineCount', kind: 'u8',   default: 1 },
            { name: 'shape',     kind: 'enum', options: FLURRY_SHAPES, default: 0 },  // Line
        ],
        signals: [
            { name: 'xDrift' }, { name: 'yDrift' }, { name: 'amplitude' },
            { name: 'frequency' }, { name: 'endpointSpeed' }, { name: 'fade' },
        ],
    },
    worley: {
        tag: 0x0B, label: 'Worley',
        config: [
            { name: 'cellSizeRaw', kind: 'i32',  default: 256, label: 'cellSize (raw s24x8)' },
            { name: 'aliasing',    kind: 'enum', options: WORLEY_ALIASING, default: 1 },  // Fast
        ],
        signals: [],
    },
    voronoi: {
        tag: 0x0C, label: 'Voronoi',
        config: [
            { name: 'cellSizeRaw', kind: 'i32',  default: 256, label: 'cellSize (raw s24x8)' },
            { name: 'aliasing',    kind: 'enum', options: WORLEY_ALIASING, default: 1 },
        ],
        signals: [],
    },

    // ── PatternFlow bare field patterns (one entry per Variant) ──────────
    // Adapted from engmung/Patternflow (CC-BY-SA-4.0). Global scale/rotation/
    // fold/palette are NOT pattern params — add them as transforms, or load a
    // ready-made recipe from PF_PRESETS below.
    pfDualAxis: {
        tag: 0x0D, label: 'PF — Dual Axis (0510)', output: 'rgb',
        config: [],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfCounterRibbons: {
        tag: 0x0E, label: 'PF — Counter Ribbons (0515)', output: 'rgb',
        config: [],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfQuadDirectional: {
        tag: 0x0F, label: 'PF — Quad Directional (0515-3)', output: 'rgb',
        config: [],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfPosterized: {
        tag: 0x10, label: 'PF — Posterized (0531)', output: 'rgb',
        config: [{ name: 'posterizeLevels', kind: 'u8', default: 5 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfCross: {
        tag: 0x11, label: 'PF — Cross (0614-2)', output: 'rgb',
        config: [],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfPetals: {
        tag: 0x12, label: 'PF — Petals (0512)', output: 'rgb',
        config: [{ name: 'petalCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'fold' }, { name: 'thickness' }],
    },
    pfRipple: {
        tag: 0x13, label: 'PF — Ripple (0524-2)', output: 'rgb',
        config: [{ name: 'waveCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfOrganic: {
        tag: 0x14, label: 'PF — Organic (0519-1)',
        config: [
            { name: 'contourLevels', kind: 'u8',   default: 10 },
            { name: 'hardEdges',     kind: 'bool', default: 0 },
        ],
        signals: [{ name: 'phaseSpeed' }, { name: 'tension' }],
    },
    pfTopographic: {
        tag: 0x15, label: 'PF — Topographic (0529)',
        config: [
            { name: 'contourLevels', kind: 'u8',   default: 8 },
            { name: 'hardEdges',     kind: 'bool', default: 0 },
        ],
        signals: [{ name: 'phaseSpeed' }, { name: 'tension' }],
    },
    pfPlasma: {
        tag: 0x16, label: 'PF — Plasma', output: 'rgb',
        config: [],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfTendrils: {
        tag: 0x17, label: 'PF — Tendrils (0520)', output: 'rgb',
        config: [],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfLiquidGate: {
        tag: 0x18, label: 'PF — Liquid Gate (0530)', output: 'rgb',
        config: [],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfConcentricGrid: {
        tag: 0x19, label: 'PF — Concentric Grid (Origin)',
        config: [{ name: 'cellCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfRowSegments: {
        tag: 0x1A, label: 'PF — Row Segments (0511)',
        config: [{ name: 'cellCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfShapes: {
        tag: 0x1B, label: 'PF — Shapes (0514)',
        config: [{ name: 'cellCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfDots: {
        tag: 0x1C, label: 'PF — Dots (0528)',
        config: [{ name: 'cellCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfWaveMatrix: {
        tag: 0x1D, label: 'PF — Wave Matrix (0601)',
        config: [{ name: 'cellCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    pfRadialPulse: {
        tag: 0x1E, label: 'PF — Radial Pulse (0624)',
        config: [{ name: 'cellCount', kind: 'u8', default: 6 }],
        signals: [{ name: 'phaseSpeed' }, { name: 'warp' }, { name: 'thickness' }],
    },
    paletteGlow: {
        tag: 0x1F, label: 'ShaderToy — Palette Glow', output: 'rgb',
        config: [],
        signals: [
            { name: 'speed', default: { id: 'constant', params: { permille: 1000 } } },
            { name: 'tileScale', default: { id: 'constant', params: { permille: 500 } } },
        ],
    },
    rocaille: {
        tag: 0x20, label: 'ShaderToy — Rocaille', output: 'rgb',
        config: [],
        signals: [
            { name: 'scale', default: { id: 'constant', params: { permille: 333 } } },
            { name: 'length', default: { id: 'constant', params: { permille: 429 } } },
            { name: 'detail', default: { id: 'constant', params: { permille: 222 } } },
            { name: 'turbulence', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'frequency', default: { id: 'constant', params: { permille: 333 } } },
            { name: 'speed', default: { id: 'constant', params: { permille: 333 } } },
            { name: 'layers', default: { id: 'constant', params: { permille: 727 } } },
            { name: 'hue', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'glow', default: { id: 'constant', params: { permille: 429 } } },
        ],
    },
    proteanClouds: {
        tag: 0x21, label: 'ShaderToy — Protean Clouds', output: 'rgb',
        config: [],
        signals: [
            { name: 'speed', default: { id: 'constant', params: { permille: 1000 } } },
            { name: 'warp', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'frequency', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'brightness', default: { id: 'constant', params: { permille: 500 } } },
        ],
    },
    xor: {
        tag: 0x22, label: 'XOR — Munching Squares',
        config: [
            { name: 'gridSize', kind: 'u8', default: 16 },
            { name: 'speed', kind: 'u16', default: 40 },
        ],
        signals: [],
    },
    octgrams: {
        tag: 0x23, label: 'ShaderToy — Octgrams', output: 'rgb',
        config: [],
        signals: [
            { name: 'speed', default: { id: 'constant', params: { permille: 1000 } } },
            { name: 'travel', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'pulse', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'density', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'glow', default: { id: 'constant', params: { permille: 500 } } },
        ],
    },
    rotatingSquares: {
        tag: 0x24, label: 'ShaderToy — Rotating Squares', output: 'rgb',
        config: [],
        signals: [
            { name: 'speed', default: { id: 'constant', params: { permille: 1000 } } },
            { name: 'thickness', default: { id: 'constant', params: { permille: 375 } } },
            { name: 'pulse', default: { id: 'constant', params: { permille: 333 } } },
            { name: 'brightness', default: { id: 'constant', params: { permille: 500 } } },
        ],
    },
    starryPlanes: {
        tag: 0x25, label: 'ShaderToy — Starry Planes', output: 'rgb',
        config: [],
        signals: [
            { name: 'speed', default: { id: 'constant', params: { permille: 1000 } } },
            { name: 'planeSpacing', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'starSize', default: { id: 'constant', params: { permille: 400 } } },
            { name: 'path', default: { id: 'constant', params: { permille: 500 } } },
            { name: 'brightness', default: { id: 'constant', params: { permille: 500 } } },
        ],
    },
    rasterConway: {
        tag: 0x2B, label: 'Pixel Grid — Conway',
        domain: 'raster',
        output: 'rgb',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 250, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'densityPermille', kind: 'permille', default: 350, label: 'density' },
        ],
        signals: [],
    },
    rasterCyclicCA: {
        tag: 0x2C, label: 'Pixel Grid — Cyclic CA',
        domain: 'raster',
        output: 'rgb',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 120, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'numStates', kind: 'u8', default: 8, label: 'states' },
            { name: 'threshold', kind: 'u8', default: 1 },
        ],
        signals: [],
    },
    rasterBriansBrain: {
        tag: 0x2D, label: "Pixel Grid — Brian's Brain",
        domain: 'raster',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 90, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'densityPermille', kind: 'permille', default: 300, label: 'density' },
        ],
        signals: [],
    },
    rasterLifeVariant: {
        tag: 0x2E, label: 'Pixel Grid — Life Variants',
        domain: 'raster',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 200, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'densityPermille', kind: 'permille', default: 350, label: 'density' },
            { name: 'rule', kind: 'enum', options: LIFE_RULES, default: 0 },
        ],
        signals: [],
    },
    rasterElementary: {
        tag: 0x2F, label: 'Pixel Grid — Elementary CA',
        domain: 'raster',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 90, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'rule', kind: 'u8', default: 30 },
        ],
        signals: [],
    },
    rasterMatrixRain: {
        tag: 0x30, label: 'Pixel Grid — Matrix Rain',
        domain: 'raster',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 60, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'fadeAmount', kind: 'u8', default: 40, label: 'fade' },
        ],
        signals: [],
    },
    rasterRipple: {
        tag: 0x31, label: 'Pixel Grid — Ripple',
        domain: 'raster',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 40, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'damping', kind: 'u8', default: 6 },
        ],
        signals: [],
    },
    rasterForestFire: {
        tag: 0x32, label: 'Pixel Grid — Forest Fire',
        domain: 'raster',
        output: 'rgb',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 120, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'growthPermille', kind: 'permille', default: 40, label: 'growth' },
            { name: 'lightningPermille', kind: 'permille', default: 2, label: 'lightning' },
        ],
        signals: [],
    },
    rasterWireWorld: {
        tag: 0x33, label: 'Pixel Grid — Wireworld',
        domain: 'raster',
        output: 'rgb',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 100, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'densityPermille', kind: 'permille', default: 500, label: 'conductor density' },
        ],
        signals: [],
    },
    rasterLangtonAnt: {
        tag: 0x34, label: "Pixel Grid — Langton's Ant",
        domain: 'raster',
        config: [
            { name: 'stepIntervalMs', kind: 'u16', default: 30, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'antCount', kind: 'u16', default: 1, label: 'ants' },
        ],
        signals: [],
    },
    rasterReactionDiffusion: {
        tag: 0x35, label: 'Pixel Grid — Reaction-Diffusion',
        domain: 'raster',
        config: [
            { name: 'preset', kind: 'enum', options: RD_PRESETS, default: 0 },
            { name: 'stepIntervalMs', kind: 'u16', default: 33, label: 'step interval (ms)' },
            { name: 'seed', kind: 'u16', default: 0 },
            { name: 'iterationsPerStep', kind: 'u16', default: 4, label: 'iterations/step' },
        ],
        signals: [],
    },
};

for (const def of Object.values(PATTERNS)) {
    if (!def.domain) def.domain = 'uv';
    if (!def.output) def.output = 'greyscale';
}

export const PATTERN_BY_TAG = (() => {
    const m = {};
    for (const [id, def] of Object.entries(PATTERNS)) m[def.tag] = id;
    return m;
})();

// ─────────────────────────────────────────────────────────────────────
// Transform definitions. Tag values are TFM_* in SceneCodec.cpp.
// ─────────────────────────────────────────────────────────────────────

export const TRANSFORMS = {
    rotation: {
        tag: 0x00, label: 'Rotation',
        config: [{ name: 'isAngleTurn', kind: 'bool', default: 0 }],
        signals: [{ name: 'angle' }],
    },
    translation: {
        tag: 0x01, label: 'Translation',
        config: [],
        signals: [{ name: 'direction' }, { name: 'speed' }],
    },
    zoom: {
        tag: 0x02, label: 'Zoom',
        config: [],
        signals: [{ name: 'scale' }],
    },
    vortex: {
        tag: 0x03, label: 'Vortex',
        config: [],
        signals: [{ name: 'strength' }],
    },
    kaleidoscope: {
        tag: 0x04, label: 'Kaleidoscope',
        config: [
            { name: 'nbFacets',   kind: 'u8',   default: 4 },
            { name: 'isMirrored', kind: 'bool', default: 1 },
        ],
        signals: [],
    },
    radialKaleidoscope: {
        tag: 0x05, label: 'Radial Kaleidoscope',
        config: [
            { name: 'radialDivisions', kind: 'u16',  default: 8 },
            { name: 'isMirrored',      kind: 'bool', default: 1 },
        ],
        signals: [],
    },
    paletteClip: {
        tag: 0x09, label: 'Palette', transformDomain: 'palette',
        config: [
            { name: 'maxFeather', kind: 'f16',  default: 32768, label: 'max feather (raw f16)' },
            // Single wire byte as a 3-way tint mode:
            //   0 = hue-remap (tint through palette, offset = phase)
            //   1 = colour-mask (single colour at offset, value = alpha)
            //   2 = native (effect's own hue via CHSV, palette bypassed)
            // Rendered in the panel as two linked checkboxes.
            { name: 'tintMode',   kind: 'tintMode', default: 0, label: 'tint' },
        ],
        signals: [{ name: 'offset' }, { name: 'clip' }],
    },
    tiling: {
        tag: 0x07, label: 'Tiling',
        config: [
            { name: 'mirrored', kind: 'bool', default: 0 },
            { name: 'shape',    kind: 'enum', options: TILE_SHAPES, default: 0 },  // SQUARE
        ],
        signals: [{ name: 'cellSize' }],
    },
    flowField: {
        tag: 0x08, label: 'Flow Field',
        config: [],
        signals: [
            { name: 'phaseSpeed' }, { name: 'flowStrength' },
            { name: 'fieldScale' }, { name: 'maxOffset' },
        ],
    },
};

for (const def of Object.values(TRANSFORMS)) {
    if (!def.transformDomain) def.transformDomain = 'uv';
}

export const TRANSFORM_BY_TAG = (() => {
    const m = {};
    for (const [id, def] of Object.entries(TRANSFORMS)) m[def.tag] = id;
    return m;
})();

export const DEFAULT_PALETTE_TRANSFORM = () => ({
    id: 'paletteClip',
    config: { maxFeather: 32768, tintMode: 0 },
    signals: {
        offset: DEFAULT_SIGNAL(),
        clip: { id: 'constant', params: { permille: 0 } },
    },
    enabled: true,
});

// ─────────────────────────────────────────────────────────────────────
// PatternFlow ready-made presets.
//
// Each entry mirrors a pf*Preset recipe in
// src/renderer/pipeline/patterns/patternflow/PatternFlowPresets.cpp — a
// bare pf field pattern plus its transform stack (palette-offset, zoom,
// rotation, vortex, kaleidoscope, tiling). The bare patterns expose only
// intrinsic field controls; the "look" of an effect lives in these
// recipes. scene() returns a FRESH model so loading one never mutates the
// library. Palette defaults to Rainbow (id 0); the C++ presets take the
// palette as an argument, so it is free to change after loading.
// ─────────────────────────────────────────────────────────────────────

const pfK    = (permille) => ({ id: 'constant', params: { permille } });
const pfSine = (permille) => ({
    id: 'sine', params: { phaseVelocity: pfK(permille), phaseOffset: 0 },
});
const signalSlotDefault = (slot) => (
    slot.default ? JSON.parse(JSON.stringify(slot.default)) : DEFAULT_SIGNAL()
);

// Bare pf pattern model with schema defaults (matches the C++ factory
// defaults the presets rely on).
function pfPatternModel(id) {
    const def = PATTERNS[id];
    const config = {};
    for (const c of def.config) config[c.name] = c.default ?? 0;
    const signals = {};
    for (const s of def.signals) signals[s.name] = signalSlotDefault(s);
    return { id, config, signals };
}

const pfPalette       = (n)   => ({
    id: 'paletteClip',
    config: { maxFeather: 32768, tintMode: 0 },
    signals: { offset: pfSine(n), clip: pfK(0) },
});
const pfZoom          = (n)   => ({ id: 'zoom', config: {}, signals: { scale: pfK(n) } });
const pfRotate        = (n)   => ({ id: 'rotation', config: { isAngleTurn: 1 }, signals: { angle: pfK(n) } });
const pfVortex        = (n)   => ({ id: 'vortex', config: {}, signals: { strength: pfK(n) } });
const pfRadialKaleido = (div) => ({ id: 'radialKaleidoscope', config: { radialDivisions: div, isMirrored: 1 }, signals: {} });
const pfTiling        = (n)   => ({ id: 'tiling', config: { mirrored: 0, shape: 0 }, signals: { cellSize: pfK(n) } });

export const PF_PRESETS = [
    { id: 'pfConcentricGrid', label: 'Concentric Grid (Origin)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfConcentricGrid'),
                      transforms: [pfPalette(120), pfZoom(400), pfRotate(60)] }) },
    { id: 'pfDualAxis', label: 'Dual Axis (0510)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfDualAxis'),
                      transforms: [pfPalette(150), pfZoom(450)] }) },
    { id: 'pfRowSegments', label: 'Row Segments (0511)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfRowSegments'),
                      transforms: [pfPalette(100), pfZoom(400)] }) },
    { id: 'pfPetals', label: 'Petals (0512)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfPetals'),
                      transforms: [pfPalette(140), pfRotate(80)] }) },
    { id: 'pfShapes', label: 'Shapes (0514)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfShapes'),
                      transforms: [pfPalette(130), pfZoom(450)] }) },
    { id: 'pfCounterRibbons', label: 'Counter Ribbons (0515)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfCounterRibbons'),
                      transforms: [pfPalette(160), pfZoom(450)] }) },
    { id: 'pfQuadDirectional', label: 'Quad Directional (0515-3)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfQuadDirectional'),
                      transforms: [pfPalette(150), pfZoom(450)] }) },
    { id: 'pfOrganic', label: 'Organic (0519-1)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfOrganic'),
                      transforms: [pfPalette(120), pfZoom(500)] }) },
    { id: 'pfTendrils', label: 'Tendrils (0520)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfTendrils'),
                      transforms: [pfPalette(140), pfZoom(500), pfRotate(50)] }) },
    { id: 'pfRipple', label: 'Ripple (0524-2)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfRipple'),
                      transforms: [pfPalette(150), pfZoom(450), pfRotate(40)] }) },
    { id: 'pfKaleidoVortex', label: 'Kaleido Vortex (0526)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfRipple'),
                      transforms: [pfPalette(160), pfZoom(450), pfVortex(500), pfRadialKaleido(6)] }) },
    { id: 'pfDots', label: 'Dots (0528)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfDots'),
                      transforms: [pfPalette(150), pfZoom(400)] }) },
    { id: 'pfTopographic', label: 'Topographic (0529)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfTopographic'),
                      transforms: [pfPalette(120), pfZoom(500)] }) },
    { id: 'pfLiquidGate', label: 'Liquid Gate (0530)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfLiquidGate'),
                      transforms: [pfPalette(170), pfZoom(500), pfRotate(45)] }) },
    { id: 'pfPosterized', label: 'Posterized (0531)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfPosterized'),
                      transforms: [pfPalette(160), pfZoom(450)] }) },
    { id: 'pfWaveMatrix', label: 'Wave Matrix (0601)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfWaveMatrix'),
                      transforms: [pfPalette(150), pfZoom(400)] }) },
    { id: 'pfPlasma', label: 'Plasma',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfPlasma'),
                      transforms: [pfPalette(140), pfZoom(500), pfRotate(40)] }) },
    { id: 'pfCross', label: 'Cross (0614-2)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfCross'),
                      transforms: [pfPalette(150), pfTiling(1000), pfZoom(355), pfRotate(30)] }) },
    { id: 'pfRadialPulse', label: 'Radial Pulse (0624)',
      scene: () => ({ paletteId: 0, pattern: pfPatternModel('pfRadialPulse'),
                      transforms: [pfPalette(160), pfZoom(450)] }) },
];

// Initial scene model the composer mounts with.
export const DEFAULT_SCENE = () => ({
    paletteId: 0,
    pattern: {
        id: 'noiseBasic',
        config: {},
        signals: { depthSpeed: DEFAULT_SIGNAL() },
    },
    transforms: [DEFAULT_PALETTE_TRANSFORM()],
});
