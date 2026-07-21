// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict';
import test from 'node:test';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import {
    decodePds,
    encodePds,
    validateDeployable,
    BACKEND_NONE,
    BACKEND_FASTLED,
    BACKEND_SMARTMATRIX,
    SECTION_GEOMETRY_POLAR,
    SECTION_OUTPUT_CONFIG,
    SECTION_FLAG_CRITICAL,
    PDS_MAGIC,
    PDS_VERSION,
} from './sketches/composer/pds-codec.js';

const HERE = dirname(fileURLToPath(import.meta.url));
const DISPLAYS = join(HERE, '..', 'displays');

const EXPECTED = {
    round: { leds: 241, raster: false, backend: BACKEND_FASTLED },
    fibonacci: { leds: 324, raster: false, backend: BACKEND_FASTLED },
    fabric: { leds: 400, raster: true, backend: BACKEND_FASTLED },
    fabric32x8: { leds: 256, raster: true, backend: BACKEND_FASTLED },
    matrix128: { leds: 4096, raster: true, backend: BACKEND_SMARTMATRIX },
};

// ─────────────────────────────────────────────────────────────────────
// Raw-byte helpers for crafting inline .PDS blobs.
// ─────────────────────────────────────────────────────────────────────

function u16(v) { return [v & 0xFF, (v >>> 8) & 0xFF]; }
function u32(v) { return [v & 0xFF, (v >>> 8) & 0xFF, (v >>> 16) & 0xFF, (v >>> 24) & 0xFF]; }

function geometryBody(ledCount) {
    const b = [...u16(ledCount)];
    for (let i = 0; i < ledCount; ++i) b.push(...u16((i * 7) & 0xFFFF), ...u16((i * 11) & 0xFFFF));
    return b;
}

function fastledBody() {
    return [
        BACKEND_FASTLED,
        ...u16(4), 0x78, 0x69, 0x61, 0x6F, // "xiao"
        0,             // chipset WS2812
        ...u16(5),     // data_pin
        ...u16(0xFFFF),// clock_pin none
        2, 200, 16,    // rgb_order, brightness, refresh_ms
        ...u32(0),     // color_correction
    ];
}

function frame(sections, version = PDS_VERSION) {
    const out = [...PDS_MAGIC, version, sections.length];
    for (const { type, flags, body } of sections) {
        out.push(type, flags, ...u32(body.length), ...body);
    }
    return new Uint8Array(out);
}

const CRIT = SECTION_FLAG_CRITICAL;

function assertRejects(bytes) {
    assert.throws(() => decodePds(bytes));
}

// ─────────────────────────────────────────────────────────────────────
// Fixtures
// ─────────────────────────────────────────────────────────────────────

test('built-in .pds fixtures decode with expected shape', () => {
    for (const [name, want] of Object.entries(EXPECTED)) {
        const bytes = new Uint8Array(readFileSync(join(DISPLAYS, `${name}.pds`)));
        const spec = decodePds(bytes);
        assert.equal(spec.ledCount, want.leds, name);
        assert.equal(spec.leds.length, want.leds, name);
        assert.equal(Boolean(spec.raster), want.raster, name);
        assert.equal(spec.output.backendKind, want.backend, name);
        validateDeployable(spec); // every built-in is deployable
    }
});

test('fixture re-encode is byte-identical', () => {
    for (const name of Object.keys(EXPECTED)) {
        const bytes = new Uint8Array(readFileSync(join(DISPLAYS, `${name}.pds`)));
        const spec = decodePds(bytes);
        const re = encodePds(spec);
        assert.deepEqual(Array.from(re), Array.from(bytes), name);
    }
});

// ─────────────────────────────────────────────────────────────────────
// Rejection cases
// ─────────────────────────────────────────────────────────────────────

test('rejects bad magic', () => assertRejects(new Uint8Array([0x58, 0x44, 0x53, 0x00, 1, 0])));

test('rejects bad version', () =>
    assertRejects(frame([{ type: 0x02, flags: CRIT, body: geometryBody(2) }], 2)));

test('rejects trailing bytes', () => {
    const bytes = frame([{ type: 0x02, flags: CRIT, body: geometryBody(2) }]);
    assertRejects(new Uint8Array([...bytes, 0]));
});

test('rejects missing geometry', () =>
    assertRejects(frame([{ type: 0x01, flags: CRIT, body: [0, 0] }])));

test('rejects empty geometry', () =>
    assertRejects(frame([{ type: 0x02, flags: CRIT, body: geometryBody(0) }])));

test('rejects duplicate section', () =>
    assertRejects(frame([
        { type: 0x02, flags: CRIT, body: geometryBody(2) },
        { type: 0x02, flags: CRIT, body: geometryBody(2) },
    ])));

test('rejects out-of-order sections', () =>
    assertRejects(frame([
        { type: 0x04, flags: CRIT, body: fastledBody() },
        { type: 0x02, flags: CRIT, body: geometryBody(2) },
    ])));

test('rejects unknown critical section', () =>
    assertRejects(frame([
        { type: 0x02, flags: CRIT, body: geometryBody(2) },
        { type: 0x7F, flags: CRIT, body: [1, 2] },
    ])));

test('rejects reserved section flag bits', () =>
    assertRejects(frame([{ type: 0x02, flags: 0x02, body: geometryBody(2) }])));

test('rejects extra bytes inside a known section', () =>
    assertRejects(frame([{ type: 0x02, flags: CRIT, body: [...geometryBody(2), 0x99] }])));

test('rejects FASTLED without a data pin', () => {
    const body = fastledBody();
    body[8] = 0xFF; body[9] = 0xFF; // data_pin bytes
    assertRejects(frame([
        { type: 0x02, flags: CRIT, body: geometryBody(2) },
        { type: 0x04, flags: CRIT, body },
    ]));
});

test('rejects out-of-range output enum', () => {
    const body = fastledBody();
    body[7] = 99; // chipset
    assertRejects(frame([
        { type: 0x02, flags: CRIT, body: geometryBody(2) },
        { type: 0x04, flags: CRIT, body },
    ]));
});

// ─────────────────────────────────────────────────────────────────────
// Acceptance / round-trip
// ─────────────────────────────────────────────────────────────────────

test('accepts unknown non-critical sections in any position', () => {
    const unk = { type: 0x7F, flags: 0x00, body: [9, 9, 9] };
    const geo = { type: 0x02, flags: CRIT, body: geometryBody(3) };
    const out = { type: 0x04, flags: CRIT, body: fastledBody() };
    for (const order of [[unk, geo, out], [geo, unk, out], [geo, out, unk]]) {
        const spec = decodePds(frame(order));
        assert.equal(spec.ledCount, 3);
    }
});

test('absent OUTPUT_CONFIG decodes as NONE', () => {
    const spec = decodePds(frame([{ type: 0x02, flags: CRIT, body: geometryBody(2) }]));
    assert.equal(spec.output.backendKind, BACKEND_NONE);
});

test('unknown backend body round-trips byte-identically', () => {
    const body = [7, 0xDE, 0xAD, 0xBE, 0xEF];
    const bytes = frame([
        { type: 0x02, flags: CRIT, body: geometryBody(2) },
        { type: 0x04, flags: CRIT, body },
    ]);
    const spec = decodePds(bytes);
    assert.equal(spec.output.backendKind, 7);
    assert.deepEqual(spec.output.rawBody, [0xDE, 0xAD, 0xBE, 0xEF]);
    assert.deepEqual(Array.from(encodePds(spec)), Array.from(bytes));
});

test('SmartMatrix without dense raster is not deployable but decodes', () => {
    const bytes = new Uint8Array(readFileSync(join(DISPLAYS, 'matrix128.pds')));
    const spec = decodePds(bytes);
    // Break density by duplicating a raster cell.
    spec.raster.cells[1] = { ...spec.raster.cells[0] };
    assert.throws(() => validateDeployable(spec));
});
