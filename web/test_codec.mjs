// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict';
import test from 'node:test';

import { decodeScene, encodeScene } from './sketches/composer/codec.js';
import {
    DEFAULT_SCENE,
    PATTERNS,
    PF_PRESETS,
    SIGNALS,
    TRANSFORMS,
} from './sketches/composer/schema.js';

const SIMPLE_SCENE = {
    paletteId: 0,
    pattern: {
        id: 'noiseBasic',
        config: {},
        signals: {
            depthSpeed: { id: 'constant', params: { permille: 550 } },
        },
    },
    transforms: [
        {
            id: 'zoom',
            config: {},
            signals: {
                scale: { id: 'constant', params: { permille: 200 } },
            },
        },
    ],
};

const SIMPLE_FIXTURE = [
    0x50, 0x53, 0x43, 0x00,
    0x01,
    0x00,
    0x00, 0x05, 0x00,
    0x00, 0x02, 0x00, 0x26, 0x02,
    0x01,
    0x02, 0x05, 0x00,
    0x00, 0x02, 0x00, 0xc8, 0x00,
];

const LEGACY_PALETTE_GLOW_FIXTURE = [
    0x50, 0x53, 0x43, 0x00,
    0x01,
    0x00,
    0x1F, 0x00, 0x00,
    0x00,
];

const SPEED_ONLY_PALETTE_GLOW_FIXTURE = [
    0x50, 0x53, 0x43, 0x00,
    0x01,
    0x00,
    0x1F, 0x05, 0x00,
    0x00, 0x02, 0x00, 0xE8, 0x03,
    0x00,
];

function bytes(values) {
    return new Uint8Array(values);
}

function assertThrowsMessage(fn, pattern) {
    assert.throws(fn, (error) => pattern.test(error.message));
}

function assertUniqueTags(group, name) {
    const seen = new Map();
    for (const [id, def] of Object.entries(group)) {
        const existing = seen.get(def.tag);
        assert.equal(existing, undefined, `${name} tag 0x${def.tag.toString(16)} reused by ${existing} and ${id}`);
        seen.set(def.tag, id);
    }
}

function defaultParamValue(param) {
    switch (param.kind) {
        case 'signal':
            return { id: 'constant', params: { permille: 500 } };
        case 'bool':
        case 'enum':
        case 'tintMode':
        case 'u8':
        case 'u16':
        case 'u32':
        case 'i32':
        case 'permille':
        case 'f16':
            return param.default ?? 0;
        default:
            throw new Error(`unhandled test param kind ${param.kind}`);
    }
}

function defaultConfig(def) {
    return Object.fromEntries((def.config ?? []).map((param) => [param.name, defaultParamValue(param)]));
}

function signalModel(id) {
    const def = SIGNALS[id];
    return {
        id,
        params: Object.fromEntries((def.params ?? []).map((param) => [param.name, defaultParamValue(param)])),
    };
}

function patternModel(id) {
    const def = PATTERNS[id];
    return {
        id,
        config: defaultConfig(def),
        signals: Object.fromEntries((def.signals ?? []).map((signal) => [
            signal.name,
            { id: 'constant', params: { permille: 500 } },
        ])),
    };
}

function transformModel(id) {
    const def = TRANSFORMS[id];
    return {
        id,
        config: defaultConfig(def),
        signals: Object.fromEntries((def.signals ?? []).map((signal) => [
            signal.name,
            { id: 'constant', params: { permille: 500 } },
        ])),
    };
}

function assertStableRoundTrip(scene, name) {
    const encoded = encodeScene(scene);
    const decoded = decodeScene(encoded);
    assert.deepEqual(Array.from(encodeScene(decoded)), Array.from(encoded), name);
}

function nestedSmapSignal(depth) {
    if (depth === 0) {
        return { id: 'constant', params: { permille: 500 } };
    }
    return {
        id: 'smap',
        params: {
            signal: nestedSmapSignal(depth - 1),
            floor: { id: 'constant', params: { permille: 0 } },
            ceiling: { id: 'constant', params: { permille: 1000 } },
        },
    };
}

test('encodeScene emits exact PSC v1 golden bytes', () => {
    assert.deepEqual(Array.from(encodeScene(SIMPLE_SCENE)), SIMPLE_FIXTURE);
});

test('decodeScene reads PSC v1 golden bytes', () => {
    const scene = decodeScene(bytes(SIMPLE_FIXTURE));
    assert.equal(scene.paletteId, 0);
    assert.equal(scene.pattern.id, 'noiseBasic');
    assert.equal(scene.pattern.signals.depthSpeed.id, 'constant');
    assert.equal(scene.pattern.signals.depthSpeed.params.permille, 550);
    assert.equal(scene.transforms.length, 1);
    assert.equal(scene.transforms[0].id, 'zoom');
    assert.equal(scene.transforms[0].signals.scale.params.permille, 200);
});

test('schema tags are unique', () => {
    assertUniqueTags(PATTERNS, 'pattern');
    assertUniqueTags(TRANSFORMS, 'transform');
    assertUniqueTags(SIGNALS, 'signal');
});

test('PatternFlow presets use the current palette transform', () => {
    assert.equal(TRANSFORMS.paletteClip.label, 'Palette');

    for (const preset of PF_PRESETS) {
        const scene = preset.scene();
        const decoded = decodeScene(encodeScene(scene));
        assert.ok(
            decoded.transforms.some((transform) => transform.id === 'paletteClip'),
            `${preset.id} does not include paletteClip`,
        );
        assert.equal(
            decoded.transforms.some((transform) => transform.id === 'palette'),
            false,
            `${preset.id} still uses legacy palette transform`,
        );
    }
});

test('default greyscale scene starts with an enabled Palette transform', () => {
    const scene = DEFAULT_SCENE();

    assert.equal(PATTERNS[scene.pattern.id].output, 'greyscale');
    assert.equal(scene.transforms[0].id, 'paletteClip');
    assert.equal(scene.transforms[0].enabled, true);
    assert.equal(scene.transforms[0].signals.clip.params.permille, 0);

    const decoded = decodeScene(encodeScene(scene));
    assert.equal(decoded.transforms[0].id, 'paletteClip');
});

test('paletteGlow speed defaults to full ShaderToy time', () => {
    const scene = {
        paletteId: 0,
        pattern: { id: 'paletteGlow', config: {}, signals: {} },
        transforms: [],
    };

    const decoded = decodeScene(encodeScene(scene));
    assert.equal(decoded.pattern.signals.speed.id, 'constant');
    assert.equal(decoded.pattern.signals.speed.params.permille, 1000);
    assert.equal(decoded.pattern.signals.tileScale.id, 'constant');
    assert.equal(decoded.pattern.signals.tileScale.params.permille, 500);
});

test('requested ShaderToy RGB patterns expose expected signal defaults', () => {
    const expected = {
        rocaille: {
            scale: 333,
            length: 429,
            detail: 222,
            turbulence: 500,
            frequency: 333,
            speed: 333,
            layers: 727,
            hue: 500,
            glow: 429,
        },
        proteanClouds: {
            speed: 1000,
            warp: 500,
            frequency: 500,
            brightness: 500,
        },
        octgrams: {
            speed: 1000,
            travel: 500,
            pulse: 500,
            density: 500,
            glow: 500,
        },
        rotatingSquares: {
            speed: 1000,
            thickness: 375,
            pulse: 333,
            brightness: 500,
        },
        starryPlanes: {
            speed: 1000,
            planeSpacing: 500,
            starSize: 400,
            path: 500,
            brightness: 500,
        },
        trigField: {
            zoom: 379,
            yOffset: 0,
            waveScale: 364,
            speed: 500,
            colorSpread: 500,
            brightness: 500,
        },
    };

    for (const [id, signals] of Object.entries(expected)) {
        const decoded = decodeScene(encodeScene({
            paletteId: 0,
            pattern: { id, config: {}, signals: {} },
            transforms: [],
        }));
        for (const [name, permille] of Object.entries(signals)) {
            assert.equal(decoded.pattern.signals[name].id, 'constant', `${id}.${name}`);
            assert.equal(decoded.pattern.signals[name].params.permille, permille, `${id}.${name}`);
        }
    }
});

test('decodeScene accepts legacy paletteGlow without speed signal', () => {
    const decoded = decodeScene(bytes(LEGACY_PALETTE_GLOW_FIXTURE));
    assert.equal(decoded.pattern.id, 'paletteGlow');
    assert.equal(decoded.pattern.signals.speed.id, 'constant');
    assert.equal(decoded.pattern.signals.speed.params.permille, 1000);
    assert.equal(decoded.pattern.signals.tileScale.id, 'constant');
    assert.equal(decoded.pattern.signals.tileScale.params.permille, 500);
});

test('decodeScene accepts paletteGlow without tileScale signal', () => {
    const decoded = decodeScene(bytes(SPEED_ONLY_PALETTE_GLOW_FIXTURE));
    assert.equal(decoded.pattern.id, 'paletteGlow');
    assert.equal(decoded.pattern.signals.speed.id, 'constant');
    assert.equal(decoded.pattern.signals.speed.params.permille, 1000);
    assert.equal(decoded.pattern.signals.tileScale.id, 'constant');
    assert.equal(decoded.pattern.signals.tileScale.params.permille, 500);
});

test('all current patterns round-trip through the web codec', () => {
    for (const id of Object.keys(PATTERNS)) {
        assertStableRoundTrip({
            paletteId: 0,
            pattern: patternModel(id),
            transforms: [],
        }, `pattern ${id}`);
    }
});

test('all current transforms round-trip through the web codec', () => {
    for (const id of Object.keys(TRANSFORMS)) {
        assertStableRoundTrip({
            paletteId: 0,
            pattern: patternModel('annuli'),
            transforms: [transformModel(id)],
        }, `transform ${id}`);
    }
});

test('all current signals round-trip through recursive signal slots', () => {
    for (const id of Object.keys(SIGNALS)) {
        assertStableRoundTrip({
            paletteId: 0,
            pattern: {
                id: 'noiseBasic',
                config: {},
                signals: { depthSpeed: signalModel(id) },
            },
            transforms: [],
        }, `signal ${id}`);
    }
});

test('nested modulated signal graph round-trips through the web codec', () => {
    const nestedScene = {
        paletteId: 2,
        pattern: {
            id: 'noiseBasic',
            config: {},
            signals: {
                depthSpeed: {
                    id: 'smap',
                    params: {
                        signal: {
                            id: 'sine',
                            params: {
                                phaseVelocity: { id: 'constant', params: { permille: 50 } },
                                phaseOffset: 1234,
                            },
                        },
                        floor: { id: 'constant', params: { permille: 100 } },
                        ceiling: { id: 'constant', params: { permille: 900 } },
                    },
                },
            },
        },
        transforms: [
            {
                id: 'paletteClip',
                config: { maxFeather: 16384, tintMode: 1 },
                signals: {
                    offset: {
                        id: 'sine',
                        params: {
                            phaseVelocity: { id: 'constant', params: { permille: 120 } },
                            phaseOffset: 0,
                        },
                    },
                    clip: {
                        id: 'noiseBounded',
                        params: {
                            phaseVelocity: { id: 'constant', params: { permille: 217 } },
                            floor: { id: 'constant', params: { permille: 100 } },
                            ceiling: { id: 'constant', params: { permille: 800 } },
                        },
                    },
                },
            },
            {
                id: 'zoom',
                config: {},
                signals: {
                    scale: {
                        id: 'scale',
                        params: {
                            signal: {
                                id: 'sawtoothBoundedPh',
                                params: {
                                    phaseVelocity: { id: 'constant', params: { permille: 333 } },
                                    phaseOffset: 77,
                                    floor: { id: 'constant', params: { permille: 150 } },
                                    ceiling: { id: 'constant', params: { permille: 850 } },
                                },
                            },
                            factor: 32768,
                        },
                    },
                },
            },
        ],
    };

    assertStableRoundTrip(nestedScene, 'nested signal graph');
});

test('noise phase offset survives decode and re-encode even though the UI hides it', () => {
    const scene = {
        paletteId: 0,
        pattern: {
            id: 'noiseBasic',
            config: {},
            signals: {
                depthSpeed: {
                    id: 'noise',
                    params: {
                        phaseVelocity: { id: 'constant', params: { permille: 333 } },
                        phaseOffset: 1234,
                    },
                },
            },
        },
        transforms: [],
    };

    const encoded = encodeScene(scene);
    const decoded = decodeScene(encoded);

    assert.equal(decoded.pattern.signals.depthSpeed.id, 'noise');
    assert.equal(decoded.pattern.signals.depthSpeed.params.phaseOffset, 1234);
    assert.deepEqual(Array.from(encodeScene(decoded)), Array.from(encoded));
});

test('decoder rejects excessive signal nesting', () => {
    const scene = {
        paletteId: 0,
        pattern: {
            id: 'noiseBasic',
            config: {},
            signals: {
                depthSpeed: nestedSmapSignal(65),
            },
        },
        transforms: [],
    };

    assertThrowsMessage(
        () => decodeScene(encodeScene(scene)),
        /signal nesting is too deep/,
    );
});

test('encoder and decoder reject unsupported versions', () => {
    assertThrowsMessage(() => encodeScene(SIMPLE_SCENE, { version: 0 }), /unsupported PSC encode version 0/);

    const legacy = bytes(SIMPLE_FIXTURE);
    legacy[4] = 0;
    assertThrowsMessage(() => decodeScene(legacy), /unsupported PSC version 0/);
});

test('decoder rejects unknown palette ids', () => {
    const bad = bytes(SIMPLE_FIXTURE);
    bad[5] = 0xfe;
    assertThrowsMessage(() => decodeScene(bad), /unknown palette id 254/);
});

test('decoder rejects trailing scene bytes', () => {
    assertThrowsMessage(
        () => decodeScene(bytes([...SIMPLE_FIXTURE, 0x99])),
        /scene record has 1 trailing byte/,
    );
});

test('decoder rejects trailing pattern body bytes', () => {
    const bad = bytes([
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x06, 0x00,
        0x00, 0x02, 0x00, 0x26, 0x02, 0x99,
        0x00,
    ]);
    assertThrowsMessage(() => decodeScene(bad), /pattern record has 1 trailing byte/);
});

test('decoder rejects trailing signal body bytes', () => {
    const bad = bytes([
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x06, 0x00,
        0x00, 0x03, 0x00, 0x26, 0x02, 0x99,
        0x00,
    ]);
    assertThrowsMessage(() => decodeScene(bad), /signal record has 1 trailing byte/);
});

test('decoder rejects trailing transform body bytes', () => {
    const bad = bytes([
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x05, 0x00,
        0x00, 0x02, 0x00, 0x26, 0x02,
        0x01,
        0x02, 0x06, 0x00,
        0x00, 0x02, 0x00, 0xc8, 0x00, 0x99,
    ]);
    assertThrowsMessage(() => decodeScene(bad), /transform record has 1 trailing byte/);
});

test('decoder rejects legacy palette transform tag', () => {
    const bad = bytes([
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x05, 0x00,
        0x00, 0x02, 0x00, 0x26, 0x02,
        0x01,
        0x06, 0x05, 0x00,
        0x00, 0x02, 0x00, 0x64, 0x00,
    ]);
    assertThrowsMessage(() => decodeScene(bad), /unknown transform tag 0x6/);
});

test('raster patterns reject UV transforms on encode and decode', () => {
    const rasterScene = {
        paletteId: 0,
        pattern: {
            id: 'rasterConway',
            config: { stepIntervalMs: 250, seed: 1, densityPermille: 350 },
            signals: {},
        },
        transforms: [
            {
                id: 'zoom',
                config: {},
                signals: { scale: { id: 'constant', params: { permille: 200 } } },
            },
        ],
    };

    assertThrowsMessage(
        () => encodeScene(rasterScene),
        /transform zoom is not compatible with raster pattern rasterConway/,
    );

    const bad = bytes([
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x2b, 0x06, 0x00,
        0xfa, 0x00, 0x01, 0x00, 0x5e, 0x01,
        0x01,
        0x02, 0x05, 0x00,
        0x00, 0x02, 0x00, 0xc8, 0x00,
    ]);
    assertThrowsMessage(
        () => decodeScene(bad),
        /transform zoom is not compatible with raster pattern rasterConway/,
    );
});
