// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// PSC v0 schema — mirrors src/composer/SceneCodec.cpp byte-for-byte.
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
    noise:              { tag: 0x1C, label: 'noise',           kind: 'modulator', params: periodicBaseParams },
    noiseBounded:       { tag: 0x1D, label: 'noise (b)',       kind: 'modulator', params: periodicBoundedParams },
    noiseBoundedPhase:  { tag: 0x1E, label: 'noise (b+ph)',    kind: 'modulator', params: periodicBoundedPhaseParams },
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
};

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
    palette: {
        tag: 0x06, label: 'Palette (offset)',
        config: [],
        signals: [{ name: 'offset' }],
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

export const TRANSFORM_BY_TAG = (() => {
    const m = {};
    for (const [id, def] of Object.entries(TRANSFORMS)) m[def.tag] = id;
    return m;
})();

// Initial scene model the composer mounts with.
export const DEFAULT_SCENE = () => ({
    paletteId: 0,
    pattern: {
        id: 'noiseBasic',
        config: {},
        signals: { depthSpeed: DEFAULT_SIGNAL() },
    },
    transforms: [],
});
