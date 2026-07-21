#!/usr/bin/env python3
"""Structural validator for PolarShader .PDS v1 display-spec files.

Two validation layers, kept deliberately separate:

  * ``validate_pds``        — general decode. Enforces the byte-format rules
    (sections, raster all-or-nothing, enum ranges, structural output config).
    Absent ``OUTPUT_CONFIG`` decodes as ``backend_kind = NONE``.
  * ``validate_deployable`` — deployability. Layered on top; rejects ``NONE``,
    unknown backends, and SmartMatrix without a dense/complete raster.

Mirrors src/display/DisplaySpecCodec.h and web/sketches/composer/pds-codec.js.
"""

from __future__ import annotations

import sys
from dataclasses import dataclass

MAGIC = b"PDS\0"
VERSION = 1
MAX_RASTER_CELLS = 4096

# Section types (known singletons; must appear in ascending order).
SECTION_DISPLAY_INFO = 0x01
SECTION_GEOMETRY_POLAR = 0x02
SECTION_RASTER_GRID = 0x03
SECTION_OUTPUT_CONFIG = 0x04
KNOWN_SECTIONS = frozenset(
    {SECTION_DISPLAY_INFO, SECTION_GEOMETRY_POLAR, SECTION_RASTER_GRID, SECTION_OUTPUT_CONFIG}
)

SECTION_FLAG_CRITICAL = 0x01

# Backend kinds.
BACKEND_NONE = 0
BACKEND_FASTLED = 1
BACKEND_SMARTMATRIX = 2

# PDS enum ceilings (append-only; never renumber).
PDS_CHIPSET_MAX = 3      # 0=WS2812,1=WS2812B,2=SK6812,3=APA102
PDS_RGB_ORDER_MAX = 5    # 0=RGB..5=BGR
PDS_PANEL_TYPE_MAX = 2   # 0=HUB75_32x32,1=HUB75_64x32,2=HUB75_64x64


def _chipset_is_clock_based(chipset: int) -> bool:
    return chipset == 3  # APA102


class PdsValidationError(ValueError):
    pass


@dataclass(frozen=True)
class PdsInfo:
    source_kind: int
    name: str
    led_count: int
    has_raster: bool
    backend_kind: int


class _Reader:
    def __init__(self, data: bytes, offset: int = 0, end: int | None = None) -> None:
        self._data = data
        self._pos = offset
        self._end = len(data) if end is None else end

    @property
    def pos(self) -> int:
        return self._pos

    @property
    def remaining(self) -> int:
        return self._end - self._pos

    def _need(self, count: int) -> None:
        if self._pos + count > self._end:
            raise PdsValidationError("PDS blob truncated")

    def u8(self) -> int:
        self._need(1)
        v = self._data[self._pos]
        self._pos += 1
        return v

    def u16(self) -> int:
        self._need(2)
        v = self._data[self._pos] | (self._data[self._pos + 1] << 8)
        self._pos += 2
        return v

    def u32(self) -> int:
        self._need(4)
        v = (
            self._data[self._pos]
            | (self._data[self._pos + 1] << 8)
            | (self._data[self._pos + 2] << 16)
            | (self._data[self._pos + 3] << 24)
        )
        self._pos += 4
        return v

    def take(self, count: int) -> bytes:
        self._need(count)
        start = self._pos
        self._pos += count
        return self._data[start:self._pos]

    def string_u8(self) -> str:
        n = self.u8()
        return self.take(n).decode("utf-8")

    def string_u16(self) -> str:
        n = self.u16()
        return self.take(n).decode("utf-8")


@dataclass
class _Decoded:
    source_kind: int = 0
    name: str = ""
    led_count: int = 0
    leds: list = None
    has_raster: bool = False
    raster_width: int = 0
    raster_height: int = 0
    raster_cells: list = None
    backend_kind: int = BACKEND_NONE
    fastled: dict = None
    smartmatrix: dict = None


def _decode(data: bytes) -> _Decoded:
    if data[:4] != MAGIC:
        raise PdsValidationError("PDS bad magic")
    r = _Reader(data, 4)
    version = r.u8()
    if version != VERSION:
        raise PdsValidationError(f"PDS unsupported version {version}")
    section_count = r.u8()

    out = _Decoded(leds=[], raster_cells=[])
    seen: set[int] = set()
    last_known = 0
    seen_geometry = False

    for _ in range(section_count):
        section_type = r.u8()
        flags = r.u8()
        body_len = r.u32()
        if flags & ~SECTION_FLAG_CRITICAL:
            raise PdsValidationError("PDS reserved section flag bits set")
        critical = bool(flags & SECTION_FLAG_CRITICAL)
        body_start = r.pos
        if r.remaining < body_len:
            raise PdsValidationError("PDS blob truncated")
        body_end = body_start + body_len

        if section_type not in KNOWN_SECTIONS:
            if critical:
                raise PdsValidationError("PDS unknown critical section")
            r.take(body_len)  # skip whole
            continue

        if section_type in seen:
            raise PdsValidationError("PDS duplicate section")
        if section_type <= last_known:
            raise PdsValidationError("PDS sections out of order")
        seen.add(section_type)
        last_known = section_type

        sub = _Reader(data, body_start, body_end)
        if section_type == SECTION_DISPLAY_INFO:
            out.source_kind = sub.u8()
            out.name = sub.string_u8()
        elif section_type == SECTION_GEOMETRY_POLAR:
            seen_geometry = True
            led_count = sub.u16()
            if led_count == 0:
                raise PdsValidationError("PDS empty geometry")
            if led_count > MAX_RASTER_CELLS:
                raise PdsValidationError("PDS geometry too large")
            for _i in range(led_count):
                out.leds.append((sub.u16(), sub.u16()))
            out.led_count = led_count
        elif section_type == SECTION_RASTER_GRID:
            if not seen_geometry:
                raise PdsValidationError("PDS raster before geometry")
            rw = sub.u16()
            rh = sub.u16()
            if rw == 0 or rh == 0 or rw * rh > MAX_RASTER_CELLS:
                raise PdsValidationError("PDS bad raster dimensions")
            out.has_raster = True
            out.raster_width = rw
            out.raster_height = rh
            for _i in range(out.led_count):
                x = sub.u16()
                y = sub.u16()
                if x >= rw or y >= rh:
                    raise PdsValidationError("PDS raster coord out of range")
                out.raster_cells.append((x, y))
        elif section_type == SECTION_OUTPUT_CONFIG:
            backend = sub.u8()
            out.backend_kind = backend
            if backend == BACKEND_FASTLED:
                f = {
                    "target_id": sub.string_u16(),
                    "chipset": sub.u8(),
                    "data_pin": sub.u16(),
                    "clock_pin": sub.u16(),
                    "rgb_order": sub.u8(),
                    "brightness": sub.u8(),
                    "refresh_ms": sub.u8(),
                    "color_correction": sub.u32(),
                }
                if f["chipset"] > PDS_CHIPSET_MAX or f["rgb_order"] > PDS_RGB_ORDER_MAX:
                    raise PdsValidationError("PDS output enum out of range")
                if f["data_pin"] == 0xFFFF:
                    raise PdsValidationError("PDS FASTLED requires a data pin")
                if _chipset_is_clock_based(f["chipset"]):
                    if f["clock_pin"] == 0xFFFF:
                        raise PdsValidationError("PDS clock chipset requires clock pin")
                elif f["clock_pin"] != 0xFFFF:
                    raise PdsValidationError("PDS clockless chipset forbids clock pin")
                out.fastled = f
            elif backend == BACKEND_SMARTMATRIX:
                m = {
                    "target_id": sub.string_u16(),
                    "panel_width": sub.u16(),
                    "panel_height": sub.u16(),
                    "chain_width": sub.u16(),
                    "chain_height": sub.u16(),
                    "subsample": sub.u8(),
                    "refresh_depth": sub.u8(),
                    "dma_buffer_rows": sub.u8(),
                    "panel_type": sub.u8(),
                    "matrix_options": sub.u32(),
                    "background_options": sub.u32(),
                    "brightness": sub.u8(),
                    "refresh_ms": sub.u8(),
                }
                if m["panel_type"] > PDS_PANEL_TYPE_MAX:
                    raise PdsValidationError("PDS output enum out of range")
                if (m["panel_width"] == 0 or m["panel_height"] == 0
                        or m["chain_width"] == 0 or m["chain_height"] == 0
                        or m["subsample"] < 1 or m["refresh_depth"] == 0
                        or m["dma_buffer_rows"] == 0):
                    raise PdsValidationError("PDS bad SmartMatrix config")
                if out.has_raster:
                    phys_w = m["panel_width"] * m["chain_width"]
                    phys_h = m["panel_height"] * m["chain_height"]
                    if phys_w % m["subsample"] or phys_h % m["subsample"]:
                        raise PdsValidationError("PDS SmartMatrix size not divisible")
                    if (out.raster_width != phys_w // m["subsample"]
                            or out.raster_height != phys_h // m["subsample"]):
                        raise PdsValidationError("PDS raster != physical/subsample")
                out.smartmatrix = m
            elif backend == BACKEND_NONE:
                pass  # explicit NONE, empty body
            else:
                sub.take(body_end - sub.pos)  # unknown backend: preserve verbatim

        if section_type in KNOWN_SECTIONS and section_type != SECTION_OUTPUT_CONFIG:
            if sub.pos != body_end:
                raise PdsValidationError("PDS section has trailing bytes")
        elif section_type == SECTION_OUTPUT_CONFIG and out.backend_kind in (
            BACKEND_NONE, BACKEND_FASTLED, BACKEND_SMARTMATRIX
        ):
            if sub.pos != body_end:
                raise PdsValidationError("PDS section has trailing bytes")
        r.take(body_len)

    if not seen_geometry:
        raise PdsValidationError("PDS missing geometry")
    if r.remaining:
        raise PdsValidationError("PDS trailing bytes")
    return out


def validate_pds(data: bytes) -> PdsInfo:
    """General decode. Raises PdsValidationError on any format violation."""
    d = _decode(data)
    return PdsInfo(
        source_kind=d.source_kind,
        name=d.name,
        led_count=d.led_count,
        has_raster=d.has_raster,
        backend_kind=d.backend_kind,
    )


def validate_deployable(data: bytes) -> PdsInfo:
    """Deployability layer. Runs the general decode first, then rejects
    non-deployable files (NONE, unknown backend, sparse SmartMatrix raster)."""
    d = _decode(data)
    if d.backend_kind == BACKEND_NONE:
        raise PdsValidationError("PDS not deployable: no backend")
    if d.backend_kind not in (BACKEND_FASTLED, BACKEND_SMARTMATRIX):
        raise PdsValidationError("PDS not deployable: unsupported backend")
    if d.backend_kind == BACKEND_SMARTMATRIX:
        cells = d.raster_width * d.raster_height
        if not d.has_raster or d.led_count != cells:
            raise PdsValidationError("PDS not deployable: SmartMatrix needs dense raster")
        seen = set()
        for x, y in d.raster_cells:
            idx = y * d.raster_width + x
            if idx in seen:
                raise PdsValidationError("PDS not deployable: duplicate raster cell")
            seen.add(idx)
    return validate_pds(data)


def _main(argv: list[str]) -> int:
    deploy = "--deploy" in argv
    paths = [a for a in argv[1:] if not a.startswith("--")]
    if not paths:
        print("usage: pds_v1.py [--deploy] <file.pds> ...", file=sys.stderr)
        return 2
    rc = 0
    for path in paths:
        with open(path, "rb") as fh:
            data = fh.read()
        try:
            info = (validate_deployable if deploy else validate_pds)(data)
            print(f"OK   {path}  {info.name!r} leds={info.led_count} "
                  f"raster={info.has_raster} backend={info.backend_kind}")
        except PdsValidationError as exc:
            print(f"FAIL {path}  {exc}", file=sys.stderr)
            rc = 1
    return rc


if __name__ == "__main__":
    raise SystemExit(_main(sys.argv))
