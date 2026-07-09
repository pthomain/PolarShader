// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// PSC v1 encoder/decoder. Mirrors src/composer/SceneCodec.cpp's wire
// format. The byte layout is the contract — drift is caught by the
// cross-implementation golden fixture in test/test_composer.

import {
    PALETTES, PATTERNS, PATTERN_BY_TAG,
    TRANSFORMS, TRANSFORM_BY_TAG,
    SIGNALS, SIGNAL_BY_TAG,
    DEFAULT_SIGNAL,
} from './schema.js';

const MAGIC = [0x50, 0x53, 0x43, 0x00];   // "PSC\0"
const VERSION = 0x01;
const MAX_SIGNAL_RECURSION_DEPTH = 64;

// ─────────────────────────────────────────────────────────────────────
// ByteWriter — grows a Uint8Array and writes LE primitives.
// ─────────────────────────────────────────────────────────────────────

class ByteWriter {
    constructor() {
        this.bytes = [];
    }

    u8(v)  { this.bytes.push(v & 0xFF); }
    u16(v) { this.bytes.push(v & 0xFF, (v >>> 8) & 0xFF); }
    u32(v) {
        this.bytes.push(v & 0xFF, (v >>> 8) & 0xFF, (v >>> 16) & 0xFF, (v >>> 24) & 0xFF);
    }
    i32(v) {
        const u = v >>> 0;   // bit-cast int32 → uint32 via unsigned-shift trick
        this.u32(u);
    }

    appendBytes(values) {
        for (const value of values) this.u8(value);
    }

    get length() {
        return this.bytes.length;
    }

    toUint8Array() {
        return new Uint8Array(this.bytes);
    }
}

// ─────────────────────────────────────────────────────────────────────
// ByteReader — bounds-checked reads. Throws on truncation.
// ─────────────────────────────────────────────────────────────────────

class ByteReader {
    constructor(bytes) {
        this.view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        this.pos = 0;
    }

    _need(n) {
        if (this.pos + n > this.view.byteLength) {
            throw new Error('PSC blob truncated');
        }
    }

    u8()  { this._need(1); const v = this.view.getUint8(this.pos);          this.pos += 1; return v; }
    u16() { this._need(2); const v = this.view.getUint16(this.pos, true);   this.pos += 2; return v; }
    u32() { this._need(4); const v = this.view.getUint32(this.pos, true);   this.pos += 4; return v; }
    i32() { this._need(4); const v = this.view.getInt32(this.pos, true);    this.pos += 4; return v; }

    bytes(n) {
        this._need(n);
        const out = new Uint8Array(this.view.buffer, this.view.byteOffset + this.pos, n);
        this.pos += n;
        return out;
    }

    subReader(n) {
        return new ByteReader(this.bytes(n));
    }

    get remaining() {
        return this.view.byteLength - this.pos;
    }

    expectEnd(kind) {
        if (this.pos !== this.view.byteLength) {
            throw new Error(`${kind} record has ${this.view.byteLength - this.pos} trailing byte(s)`);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────
// Param encoders/decoders by kind
// ─────────────────────────────────────────────────────────────────────

function writeParam(w, paramSchema, value, version) {
    switch (paramSchema.kind) {
        case 'u8':       w.u8(value | 0); break;
        case 'u16':      w.u16(value | 0); break;
        case 'u32':      w.u32(value >>> 0); break;
        case 'i32':      w.i32(value | 0); break;
        case 'bool':     w.u8(value ? 1 : 0); break;
        case 'enum':     w.u8(value | 0); break;
        case 'tintMode': w.u8(value | 0); break;
        case 'permille': w.u16(value | 0); break;
        case 'f16':      w.u16(value | 0); break;
        case 'signal':   writeSignal(w, value, version); break;
        default: throw new Error(`unknown param kind ${paramSchema.kind}`);
    }
}

function readParam(r, paramSchema, version, depth = 0) {
    switch (paramSchema.kind) {
        case 'u8':       return r.u8();
        case 'u16':      return r.u16();
        case 'u32':      return r.u32();
        case 'i32':      return r.i32();
        case 'bool':     return r.u8() !== 0 ? 1 : 0;
        case 'enum':     return r.u8();
        case 'tintMode': return r.u8();
        case 'permille': return r.u16();
        case 'f16':      return r.u16();
        case 'signal':   return readSignal(r, version, depth + 1);
        default: throw new Error(`unknown param kind ${paramSchema.kind}`);
    }
}

// ─────────────────────────────────────────────────────────────────────
// Signal (recursive)
// ─────────────────────────────────────────────────────────────────────

function wireParams(def) {
    return def.wireParams ?? def.params;
}

function normalizeSignalForComposer(signal) {
    if (!signal || typeof signal !== 'object') return DEFAULT_SIGNAL();
    if (signal.id === 'noise') {
        // Noise phase remains hidden in the editor, but a non-zero wire phase
        // still has meaning and must survive decode/re-encode.
        return {
            id: 'noise',
            params: {
                phaseVelocity: signal.params?.phaseVelocity ?? DEFAULT_SIGNAL(),
                phaseOffset: signal.params?.phaseOffset ?? 0,
            },
        };
    }
    if (signal.id === 'noiseBoundedPhase') {
        return {
            id: 'noiseBounded',
            wireId: 'noiseBoundedPhase',
            params: {
                phaseVelocity: signal.params?.phaseVelocity ?? DEFAULT_SIGNAL(),
                phaseOffset: signal.params?.phaseOffset ?? 0,
                floor: signal.params?.floor ?? DEFAULT_SIGNAL(),
                ceiling: signal.params?.ceiling ?? DEFAULT_SIGNAL(),
            },
        };
    }
    return signal;
}

function writeSignalBody(w, signal, version) {
    for (const p of wireParams(signal.def)) {
        const value = signal.params?.[p.name];
        if (value === undefined && p.kind !== 'signal') {
            writeParam(w, p, p.default ?? 0, version);
        } else if (value === undefined && p.kind === 'signal') {
            writeParam(w, p, DEFAULT_SIGNAL(), version);
        } else {
            writeParam(w, p, value, version);
        }
    }
}

export function writeSignal(w, signal, version = VERSION) {
    signal = normalizeSignalForComposer(signal);
    const writeId = signal.wireId ?? signal.id;
    const def = SIGNALS[writeId];
    if (!def) throw new Error(`unknown signal id ${signal.id}`);
    signal = { ...signal, def };

    w.u8(def.tag);
    const body = new ByteWriter();
    writeSignalBody(body, signal, version);
    w.u16(body.length);
    w.appendBytes(body.toUint8Array());
}

function defaultSignalForSlot(slot) {
    return slot.default ? JSON.parse(JSON.stringify(slot.default)) : DEFAULT_SIGNAL();
}

function readSignalBody(r, tag, version, depth) {
    const id = SIGNAL_BY_TAG[tag];
    if (id === undefined) throw new Error(`unknown signal tag 0x${tag.toString(16)}`);
    const def = SIGNALS[id];
    const params = {};
    for (const p of wireParams(def)) params[p.name] = readParam(r, p, version, depth);
    return normalizeSignalForComposer({ id, params });
}

export function readSignal(r, version = VERSION, depth = 0) {
    if (depth > MAX_SIGNAL_RECURSION_DEPTH) {
        throw new Error('signal nesting is too deep');
    }
    const tag = r.u8();
    const body = r.subReader(r.u16());
    const signal = readSignalBody(body, tag, version, depth);
    body.expectEnd('signal');
    return signal;
}

// ─────────────────────────────────────────────────────────────────────
// Pattern + Transform body
// ─────────────────────────────────────────────────────────────────────

function writePatternBody(w, pattern, def, version) {
    for (const c of def.config) {
        const value = pattern.config?.[c.name] ?? c.default;
        writeParam(w, c, value, version);
    }
    for (const s of def.signals) {
        const sig = pattern.signals?.[s.name] ?? defaultSignalForSlot(s);
        writeSignal(w, sig, version);
    }
}

function writePattern(w, pattern, version) {
    const def = PATTERNS[pattern.id];
    if (!def) throw new Error(`unknown pattern id ${pattern.id}`);
    w.u8(def.tag);
    const body = new ByteWriter();
    writePatternBody(body, pattern, def, version);
    w.u16(body.length);
    w.appendBytes(body.toUint8Array());
}

function readPatternBody(r, tag, version) {
    const id = PATTERN_BY_TAG[tag];
    if (id === undefined) throw new Error(`unknown pattern tag 0x${tag.toString(16)}`);
    const def = PATTERNS[id];
    const config = {};
    for (const c of def.config) config[c.name] = readParam(r, c, version, 0);
    const signals = {};
    for (const s of def.signals) {
        signals[s.name] = (r.remaining === 0 && s.default)
            ? defaultSignalForSlot(s)
            : readSignal(r, version, 0);
    }
    return { id, config, signals };
}

function readPattern(r, version) {
    const tag = r.u8();
    const body = r.subReader(r.u16());
    const pattern = readPatternBody(body, tag, version);
    body.expectEnd('pattern');
    return pattern;
}

function writeTransformBody(w, transform, def, version) {
    for (const c of def.config) {
        const value = transform.config?.[c.name] ?? c.default;
        writeParam(w, c, value, version);
    }
    for (const s of def.signals) {
        const sig = transform.signals?.[s.name] ?? DEFAULT_SIGNAL();
        writeSignal(w, sig, version);
    }
}

function writeTransform(w, transform, version) {
    const def = TRANSFORMS[transform.id];
    if (!def) throw new Error(`unknown transform id ${transform.id}`);
    w.u8(def.tag);
    const body = new ByteWriter();
    writeTransformBody(body, transform, def, version);
    w.u16(body.length);
    w.appendBytes(body.toUint8Array());
}

function assertTransformCompatible(pattern, transforms) {
    const patternDef = PATTERNS[pattern?.id];
    if ((patternDef?.domain ?? 'uv') !== 'raster') return;

    for (const transform of transforms ?? []) {
        const transformDef = TRANSFORMS[transform.id];
        if ((transformDef?.transformDomain ?? 'uv') !== 'palette') {
            throw new Error(`transform ${transform.id} is not compatible with raster pattern ${pattern.id}`);
        }
    }
}

function readTransformBody(r, tag, version) {
    const id = TRANSFORM_BY_TAG[tag];
    if (id === undefined) throw new Error(`unknown transform tag 0x${tag.toString(16)}`);
    const def = TRANSFORMS[id];
    const config = {};
    for (const c of def.config) config[c.name] = readParam(r, c, version, 0);
    const signals = {};
    for (const s of def.signals) signals[s.name] = readSignal(r, version, 0);
    return { id, config, signals };
}

function readTransform(r, version) {
    const tag = r.u8();
    const body = r.subReader(r.u16());
    const transform = readTransformBody(body, tag, version);
    body.expectEnd('transform');
    return transform;
}

// ─────────────────────────────────────────────────────────────────────
// Top-level
// ─────────────────────────────────────────────────────────────────────

export function encodeScene(scene, { version = VERSION } = {}) {
    if (version !== VERSION) {
        throw new Error(`unsupported PSC encode version ${version}`);
    }
    const w = new ByteWriter();
    for (const b of MAGIC) w.u8(b);
    w.u8(version);
    if (!PALETTES.find(p => p.id === scene.paletteId)) {
        throw new Error(`unknown palette id ${scene.paletteId}`);
    }
    w.u8(scene.paletteId);
    writePattern(w, scene.pattern, version);
    const tfms = scene.transforms ?? [];
    assertTransformCompatible(scene.pattern, tfms);
    if (tfms.length > 255) throw new Error('too many transforms (max 255)');
    w.u8(tfms.length);
    for (const t of tfms) writeTransform(w, t, version);
    return w.toUint8Array();
}

export function decodeScene(bytes) {
    const r = new ByteReader(bytes);
    for (let i = 0; i < MAGIC.length; ++i) {
        if (r.u8() !== MAGIC[i]) throw new Error('bad PSC magic');
    }
    const version = r.u8();
    if (version !== VERSION) throw new Error(`unsupported PSC version ${version}`);
    const paletteId = r.u8();
    if (!PALETTES.find(p => p.id === paletteId)) {
        throw new Error(`unknown palette id ${paletteId}`);
    }
    const pattern = readPattern(r, version);
    const transformCount = r.u8();
    const transforms = [];
    for (let i = 0; i < transformCount; ++i) transforms.push(readTransform(r, version));
    r.expectEnd('scene');
    assertTransformCompatible(pattern, transforms);
    return { paletteId, pattern, transforms };
}
