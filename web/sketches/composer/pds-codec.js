// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// .PDS v1 ("Polar Display Spec") encoder/decoder. Mirrors
// src/display/DisplaySpecCodec.h and scripts/pds_v1.py. The sectioned byte
// layout is the contract — drift is caught by the tri-language golden fixtures.

import { ByteReader, ByteWriter } from './codec.js';

export const PDS_MAGIC = [0x50, 0x44, 0x53, 0x00]; // "PDS\0"
export const PDS_VERSION = 0x01;
export const MAX_RASTER_CELLS = 4096;

export const SECTION_DISPLAY_INFO = 0x01;
export const SECTION_GEOMETRY_POLAR = 0x02;
export const SECTION_RASTER_GRID = 0x03;
export const SECTION_OUTPUT_CONFIG = 0x04;
const KNOWN_SECTIONS = new Set([
    SECTION_DISPLAY_INFO, SECTION_GEOMETRY_POLAR, SECTION_RASTER_GRID, SECTION_OUTPUT_CONFIG,
]);
export const SECTION_FLAG_CRITICAL = 0x01;

export const BACKEND_NONE = 0;
export const BACKEND_FASTLED = 1;
export const BACKEND_SMARTMATRIX = 2;

const PDS_CHIPSET_MAX = 3;
const PDS_RGB_ORDER_MAX = 5;
const PDS_PANEL_TYPE_MAX = 2;

function chipsetIsClockBased(chipset) {
    return chipset === 3; // APA102
}

const utf8Decoder = new TextDecoder();
const utf8Encoder = new TextEncoder();

function readStringU8(r) {
    const n = r.u8();
    return utf8Decoder.decode(r.bytes(n));
}

function readStringU16(r) {
    const n = r.u16();
    return utf8Decoder.decode(r.bytes(n));
}

function writeStringU8(w, s) {
    const bytes = utf8Encoder.encode(s ?? '');
    w.u8(bytes.length);
    w.appendBytes(bytes);
}

function writeStringU16(w, s) {
    const bytes = utf8Encoder.encode(s ?? '');
    w.u16(bytes.length);
    w.appendBytes(bytes);
}

// ─────────────────────────────────────────────────────────────────────
// Decode
// ─────────────────────────────────────────────────────────────────────

export function decodePds(bytes) {
    const r = new ByteReader(bytes, 'PDS');
    for (let i = 0; i < PDS_MAGIC.length; ++i) {
        if (r.u8() !== PDS_MAGIC[i]) throw new Error('bad PDS magic');
    }
    const version = r.u8();
    if (version !== PDS_VERSION) throw new Error(`unsupported PDS version ${version}`);
    const sectionCount = r.u8();

    const out = {
        hasInfo: false,
        sourceKind: 0,
        name: '',
        ledCount: 0,
        leds: [],
        raster: null,
        output: { backendKind: BACKEND_NONE },
    };
    const seen = new Set();
    let lastKnown = 0;
    let seenGeometry = false;

    for (let s = 0; s < sectionCount; ++s) {
        const type = r.u8();
        const flags = r.u8();
        const bodyLen = r.u32();
        if (flags & ~SECTION_FLAG_CRITICAL) throw new Error('PDS reserved section flag bits set');
        const critical = (flags & SECTION_FLAG_CRITICAL) !== 0;
        if (r.remaining < bodyLen) throw new Error('PDS blob truncated');

        if (!KNOWN_SECTIONS.has(type)) {
            if (critical) throw new Error('PDS unknown critical section');
            r.bytes(bodyLen); // skip whole
            continue;
        }
        if (seen.has(type)) throw new Error('PDS duplicate section');
        if (type <= lastKnown) throw new Error('PDS sections out of order');
        seen.add(type);
        lastKnown = type;

        const body = r.subReader(bodyLen);
        if (type === SECTION_DISPLAY_INFO) {
            out.hasInfo = true;
            out.sourceKind = body.u8();
            out.name = readStringU8(body);
            body.expectEnd('PDS section');
        } else if (type === SECTION_GEOMETRY_POLAR) {
            seenGeometry = true;
            const ledCount = body.u16();
            if (ledCount === 0) throw new Error('PDS empty geometry');
            if (ledCount > MAX_RASTER_CELLS) throw new Error('PDS geometry too large');
            for (let i = 0; i < ledCount; ++i) {
                out.leds.push({ angle: body.u16(), radius: body.u16() });
            }
            out.ledCount = ledCount;
            body.expectEnd('PDS section');
        } else if (type === SECTION_RASTER_GRID) {
            if (!seenGeometry) throw new Error('PDS raster before geometry');
            const width = body.u16();
            const height = body.u16();
            if (width === 0 || height === 0 || width * height > MAX_RASTER_CELLS) {
                throw new Error('PDS bad raster dimensions');
            }
            const cells = [];
            for (let i = 0; i < out.ledCount; ++i) {
                const x = body.u16();
                const y = body.u16();
                if (x >= width || y >= height) throw new Error('PDS raster coord out of range');
                cells.push({ x, y });
            }
            out.raster = { width, height, cells };
            body.expectEnd('PDS section');
        } else if (type === SECTION_OUTPUT_CONFIG) {
            const backendKind = body.u8();
            out.output = { backendKind };
            if (backendKind === BACKEND_FASTLED) {
                const f = {
                    targetId: readStringU16(body),
                    chipset: body.u8(),
                    dataPin: body.u16(),
                    clockPin: body.u16(),
                    rgbOrder: body.u8(),
                    brightness: body.u8(),
                    refreshMs: body.u8(),
                    colorCorrection: body.u32(),
                };
                if (f.chipset > PDS_CHIPSET_MAX || f.rgbOrder > PDS_RGB_ORDER_MAX) {
                    throw new Error('PDS output enum out of range');
                }
                if (f.dataPin === 0xFFFF) throw new Error('PDS FASTLED requires a data pin');
                if (chipsetIsClockBased(f.chipset)) {
                    if (f.clockPin === 0xFFFF) throw new Error('PDS clock chipset requires clock pin');
                } else if (f.clockPin !== 0xFFFF) {
                    throw new Error('PDS clockless chipset forbids clock pin');
                }
                out.output.fastled = f;
                body.expectEnd('PDS section');
            } else if (backendKind === BACKEND_SMARTMATRIX) {
                const m = {
                    targetId: readStringU16(body),
                    panelWidth: body.u16(),
                    panelHeight: body.u16(),
                    chainWidth: body.u16(),
                    chainHeight: body.u16(),
                    subsample: body.u8(),
                    refreshDepth: body.u8(),
                    dmaBufferRows: body.u8(),
                    panelType: body.u8(),
                    matrixOptions: body.u32(),
                    backgroundOptions: body.u32(),
                    brightness: body.u8(),
                    refreshMs: body.u8(),
                };
                if (m.panelType > PDS_PANEL_TYPE_MAX) throw new Error('PDS output enum out of range');
                if (m.panelWidth === 0 || m.panelHeight === 0 || m.chainWidth === 0
                    || m.chainHeight === 0 || m.subsample < 1 || m.refreshDepth === 0
                    || m.dmaBufferRows === 0) {
                    throw new Error('PDS bad SmartMatrix config');
                }
                if (out.raster) {
                    const physW = m.panelWidth * m.chainWidth;
                    const physH = m.panelHeight * m.chainHeight;
                    if (physW % m.subsample || physH % m.subsample) {
                        throw new Error('PDS SmartMatrix size not divisible');
                    }
                    if (out.raster.width !== physW / m.subsample
                        || out.raster.height !== physH / m.subsample) {
                        throw new Error('PDS raster != physical/subsample');
                    }
                }
                out.output.smartmatrix = m;
                body.expectEnd('PDS section');
            } else if (backendKind === BACKEND_NONE) {
                body.expectEnd('PDS section');
            } else {
                // Unknown backend: preserve remaining body verbatim.
                out.output.rawBody = Array.from(body.bytes(body.remaining));
            }
        }
    }

    if (!seenGeometry) throw new Error('PDS missing geometry');
    if (r.remaining) throw new Error('PDS trailing bytes');
    return out;
}

// ─────────────────────────────────────────────────────────────────────
// Encode
// ─────────────────────────────────────────────────────────────────────

function frameSection(w, type, bodyBytes) {
    w.u8(type);
    w.u8(SECTION_FLAG_CRITICAL);
    w.u32(bodyBytes.length);
    w.appendBytes(bodyBytes);
}

export function encodePds(spec) {
    const sections = [];

    if (spec.hasInfo) {
        const b = new ByteWriter();
        b.u8(spec.sourceKind ?? 0);
        writeStringU8(b, spec.name ?? '');
        sections.push([SECTION_DISPLAY_INFO, b.toUint8Array()]);
    }

    {
        const b = new ByteWriter();
        b.u16(spec.leds.length);
        for (const led of spec.leds) {
            b.u16(led.angle & 0xFFFF);
            b.u16(led.radius & 0xFFFF);
        }
        sections.push([SECTION_GEOMETRY_POLAR, b.toUint8Array()]);
    }

    if (spec.raster) {
        const b = new ByteWriter();
        b.u16(spec.raster.width);
        b.u16(spec.raster.height);
        for (const cell of spec.raster.cells) {
            b.u16(cell.x);
            b.u16(cell.y);
        }
        sections.push([SECTION_RASTER_GRID, b.toUint8Array()]);
    }

    const output = spec.output ?? { backendKind: BACKEND_NONE };
    if (output.backendKind !== BACKEND_NONE) {
        const b = new ByteWriter();
        b.u8(output.backendKind);
        if (output.backendKind === BACKEND_FASTLED) {
            const f = output.fastled;
            writeStringU16(b, f.targetId);
            b.u8(f.chipset);
            b.u16(f.dataPin);
            b.u16(f.clockPin);
            b.u8(f.rgbOrder);
            b.u8(f.brightness);
            b.u8(f.refreshMs);
            b.u32(f.colorCorrection >>> 0);
        } else if (output.backendKind === BACKEND_SMARTMATRIX) {
            const m = output.smartmatrix;
            writeStringU16(b, m.targetId);
            b.u16(m.panelWidth);
            b.u16(m.panelHeight);
            b.u16(m.chainWidth);
            b.u16(m.chainHeight);
            b.u8(m.subsample);
            b.u8(m.refreshDepth);
            b.u8(m.dmaBufferRows);
            b.u8(m.panelType);
            b.u32(m.matrixOptions >>> 0);
            b.u32(m.backgroundOptions >>> 0);
            b.u8(m.brightness);
            b.u8(m.refreshMs);
        } else {
            b.appendBytes(output.rawBody ?? []);
        }
        sections.push([SECTION_OUTPUT_CONFIG, b.toUint8Array()]);
    }

    const w = new ByteWriter();
    for (const byte of PDS_MAGIC) w.u8(byte);
    w.u8(PDS_VERSION);
    w.u8(sections.length);
    for (const [type, body] of sections) frameSection(w, type, body);
    return w.toUint8Array();
}

// ─────────────────────────────────────────────────────────────────────
// Deployability (separate layer)
// ─────────────────────────────────────────────────────────────────────

export function validateDeployable(spec) {
    const backend = spec.output?.backendKind ?? BACKEND_NONE;
    if (backend === BACKEND_NONE) throw new Error('PDS not deployable: no backend');
    if (backend !== BACKEND_FASTLED && backend !== BACKEND_SMARTMATRIX) {
        throw new Error('PDS not deployable: unsupported backend');
    }
    if (backend === BACKEND_SMARTMATRIX) {
        const raster = spec.raster;
        if (!raster || spec.leds.length !== raster.width * raster.height) {
            throw new Error('PDS not deployable: SmartMatrix needs dense raster');
        }
        const seen = new Set();
        for (const { x, y } of raster.cells) {
            const idx = y * raster.width + x;
            if (seen.has(idx)) throw new Error('PDS not deployable: duplicate raster cell');
            seen.add(idx);
        }
    }
    return true;
}
