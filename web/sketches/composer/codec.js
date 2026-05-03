// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// PSC v0 encoder/decoder. Mirrors src/composer/SceneCodec.cpp's wire
// format. The byte layout is the contract — drift is caught by the
// cross-implementation golden fixture in test/test_composer.

import {
    PALETTES, PATTERNS, PATTERN_BY_TAG,
    TRANSFORMS, TRANSFORM_BY_TAG,
    SIGNALS, SIGNAL_BY_TAG,
    DEFAULT_SIGNAL,
} from './schema.js';

const MAGIC = [0x50, 0x53, 0x43, 0x00];   // "PSC\0"
const VERSION = 0x00;

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
}

// ─────────────────────────────────────────────────────────────────────
// Param encoders/decoders by kind
// ─────────────────────────────────────────────────────────────────────

function writeParam(w, paramSchema, value) {
    switch (paramSchema.kind) {
        case 'u8':       w.u8(value | 0); break;
        case 'u16':      w.u16(value | 0); break;
        case 'u32':      w.u32(value >>> 0); break;
        case 'i32':      w.i32(value | 0); break;
        case 'bool':     w.u8(value ? 1 : 0); break;
        case 'enum':     w.u8(value | 0); break;
        case 'permille': w.u16(value | 0); break;
        case 'f16':      w.u16(value | 0); break;
        case 'signal':   writeSignal(w, value); break;
        default: throw new Error(`unknown param kind ${paramSchema.kind}`);
    }
}

function readParam(r, paramSchema) {
    switch (paramSchema.kind) {
        case 'u8':       return r.u8();
        case 'u16':      return r.u16();
        case 'u32':      return r.u32();
        case 'i32':      return r.i32();
        case 'bool':     return r.u8() !== 0 ? 1 : 0;
        case 'enum':     return r.u8();
        case 'permille': return r.u16();
        case 'f16':      return r.u16();
        case 'signal':   return readSignal(r);
        default: throw new Error(`unknown param kind ${paramSchema.kind}`);
    }
}

// ─────────────────────────────────────────────────────────────────────
// Signal (recursive)
// ─────────────────────────────────────────────────────────────────────

export function writeSignal(w, signal) {
    const def = SIGNALS[signal.id];
    if (!def) throw new Error(`unknown signal id ${signal.id}`);
    w.u8(def.tag);
    for (const p of def.params) {
        const value = signal.params[p.name];
        if (value === undefined && p.kind !== 'signal') {
            // Use the schema default when the model omits a leaf field.
            writeParam(w, p, p.default ?? 0);
        } else if (value === undefined && p.kind === 'signal') {
            // Inner signal slot defaulted to constant(500).
            writeParam(w, p, DEFAULT_SIGNAL());
        } else {
            writeParam(w, p, value);
        }
    }
}

export function readSignal(r) {
    const tag = r.u8();
    const id = SIGNAL_BY_TAG[tag];
    if (id === undefined) throw new Error(`unknown signal tag 0x${tag.toString(16)}`);
    const def = SIGNALS[id];
    const params = {};
    for (const p of def.params) {
        params[p.name] = readParam(r, p);
    }
    return { id, params };
}

// ─────────────────────────────────────────────────────────────────────
// Pattern + Transform body
// ─────────────────────────────────────────────────────────────────────

function writePattern(w, pattern) {
    const def = PATTERNS[pattern.id];
    if (!def) throw new Error(`unknown pattern id ${pattern.id}`);
    w.u8(def.tag);
    for (const c of def.config) {
        const value = pattern.config?.[c.name] ?? c.default;
        writeParam(w, c, value);
    }
    for (const s of def.signals) {
        const sig = pattern.signals?.[s.name] ?? DEFAULT_SIGNAL();
        writeSignal(w, sig);
    }
}

function readPattern(r) {
    const tag = r.u8();
    const id = PATTERN_BY_TAG[tag];
    if (id === undefined) throw new Error(`unknown pattern tag 0x${tag.toString(16)}`);
    const def = PATTERNS[id];
    const config = {};
    for (const c of def.config) config[c.name] = readParam(r, c);
    const signals = {};
    for (const s of def.signals) signals[s.name] = readSignal(r);
    return { id, config, signals };
}

function writeTransform(w, transform) {
    const def = TRANSFORMS[transform.id];
    if (!def) throw new Error(`unknown transform id ${transform.id}`);
    w.u8(def.tag);
    for (const c of def.config) {
        const value = transform.config?.[c.name] ?? c.default;
        writeParam(w, c, value);
    }
    for (const s of def.signals) {
        const sig = transform.signals?.[s.name] ?? DEFAULT_SIGNAL();
        writeSignal(w, sig);
    }
}

function readTransform(r) {
    const tag = r.u8();
    const id = TRANSFORM_BY_TAG[tag];
    if (id === undefined) throw new Error(`unknown transform tag 0x${tag.toString(16)}`);
    const def = TRANSFORMS[id];
    const config = {};
    for (const c of def.config) config[c.name] = readParam(r, c);
    const signals = {};
    for (const s of def.signals) signals[s.name] = readSignal(r);
    return { id, config, signals };
}

// ─────────────────────────────────────────────────────────────────────
// Top-level
// ─────────────────────────────────────────────────────────────────────

export function encodeScene(scene) {
    const w = new ByteWriter();
    for (const b of MAGIC) w.u8(b);
    w.u8(VERSION);
    if (!PALETTES.find(p => p.id === scene.paletteId)) {
        throw new Error(`unknown palette id ${scene.paletteId}`);
    }
    w.u8(scene.paletteId);
    writePattern(w, scene.pattern);
    const tfms = scene.transforms ?? [];
    if (tfms.length > 255) throw new Error('too many transforms (max 255)');
    w.u8(tfms.length);
    for (const t of tfms) writeTransform(w, t);
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
    const pattern = readPattern(r);
    const transformCount = r.u8();
    const transforms = [];
    for (let i = 0; i < transformCount; ++i) transforms.push(readTransform(r));
    return { paletteId, pattern, transforms };
}
