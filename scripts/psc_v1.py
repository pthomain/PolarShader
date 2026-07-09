#!/usr/bin/env python3
"""Small structural validator for PolarShader PSC v1 scene files."""

from __future__ import annotations

from dataclasses import dataclass


MAGIC = b"PSC\0"
VERSION = 1
MAX_RECURSION_DEPTH = 64
PALETTE_IDS = frozenset({0, 1, 2, 3})


class PscValidationError(ValueError):
    pass


@dataclass(frozen=True)
class PscInfo:
    pattern_tag: int


class _Reader:
    def __init__(self, data: bytes, offset: int = 0) -> None:
        self._data = data
        self._pos = offset

    @property
    def pos(self) -> int:
        return self._pos

    @property
    def remaining(self) -> int:
        return len(self._data) - self._pos

    def _need(self, count: int) -> None:
        if self._pos + count > len(self._data):
            raise PscValidationError("PSC blob truncated")

    def u8(self) -> int:
        self._need(1)
        value = self._data[self._pos]
        self._pos += 1
        return value

    def u16(self) -> int:
        self._need(2)
        value = self._data[self._pos] | (self._data[self._pos + 1] << 8)
        self._pos += 2
        return value

    def u32(self) -> int:
        self._need(4)
        value = (
            self._data[self._pos]
            | (self._data[self._pos + 1] << 8)
            | (self._data[self._pos + 2] << 16)
            | (self._data[self._pos + 3] << 24)
        )
        self._pos += 4
        return value

    def i32(self) -> int:
        value = self.u32()
        return value - (1 << 32) if value & 0x80000000 else value

    def subreader(self, count: int) -> "_Reader":
        self._need(count)
        start = self._pos
        self._pos += count
        return _Reader(self._data[start:self._pos])

    def expect_end(self, kind: str) -> None:
        if self.remaining:
            raise PscValidationError(f"{kind} record has {self.remaining} trailing byte(s)")


def _sig(*params: str | tuple[str, int]) -> tuple[str | tuple[str, int], ...]:
    return params


APERIODIC = _sig("u32", ("enum", 1))
PERIODIC_BASE = _sig("signal", "i32")
PERIODIC_BOUNDED = _sig("signal", "signal", "signal")
PERIODIC_BOUNDED_PHASE = _sig("signal", "i32", "signal", "signal")

SIGNAL_SCHEMAS: dict[int, tuple[str | tuple[str, int], ...]] = {
    0x00: _sig("u16"),
    0x01: _sig(),
    0x02: APERIODIC,
    0x03: APERIODIC,
    0x04: APERIODIC,
    0x05: APERIODIC,
    0x10: PERIODIC_BASE,
    0x11: PERIODIC_BOUNDED,
    0x12: PERIODIC_BOUNDED_PHASE,
    0x13: PERIODIC_BASE,
    0x14: PERIODIC_BOUNDED,
    0x15: PERIODIC_BOUNDED_PHASE,
    0x16: PERIODIC_BASE,
    0x17: PERIODIC_BOUNDED,
    0x18: PERIODIC_BOUNDED_PHASE,
    0x19: PERIODIC_BASE,
    0x1A: PERIODIC_BOUNDED,
    0x1B: PERIODIC_BOUNDED_PHASE,
    0x1C: PERIODIC_BASE,
    0x1D: PERIODIC_BOUNDED,
    0x1E: PERIODIC_BOUNDED_PHASE,
    0x1F: _sig("signal", "signal", "signal"),
    0x20: _sig("signal", "u16"),
}

PATTERN_SCHEMAS: dict[int, tuple[tuple[str | tuple[str, int], ...], str]] = {
    0x00: (_sig("signal"), "uv"),
    0x01: (_sig("u8"), "uv"),
    0x02: (_sig(), "uv"),
    0x03: (_sig(), "uv"),
    0x04: (_sig("u16", "u8", ("enum", 2)), "uv"),
    0x05: (_sig(("enum", 3), "u8", "u8", "u8"), "uv"),
    0x06: (_sig("u8", "u8", ("enum", 2), "signal", "signal", "signal", "signal", "signal", "signal", "signal", "signal"), "uv"),
    0x07: (_sig("u8", ("enum", 9), "bool", "signal", "signal", "signal", "signal"), "uv"),
    0x08: (_sig("u8", "bool", "signal", "signal", "signal"), "uv"),
    0x09: (_sig("u8", "u8", "bool", "u16", "u16"), "uv"),
    0x0A: (_sig("u8", "u8", ("enum", 1), "signal", "signal", "signal", "signal", "signal", "signal"), "uv"),
    0x0B: (_sig("i32", ("enum", 2)), "uv"),
    0x0C: (_sig("i32", ("enum", 2)), "uv"),
    0x0D: (_sig("signal", "signal", "signal"), "uv"),
    0x0E: (_sig("signal", "signal", "signal"), "uv"),
    0x0F: (_sig("signal", "signal", "signal"), "uv"),
    0x10: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x11: (_sig("signal", "signal", "signal"), "uv"),
    0x12: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x13: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x14: (_sig("u8", "bool", "signal", "signal"), "uv"),
    0x15: (_sig("u8", "bool", "signal", "signal"), "uv"),
    0x16: (_sig("signal", "signal", "signal"), "uv"),
    0x17: (_sig("signal", "signal", "signal"), "uv"),
    0x18: (_sig("signal", "signal", "signal"), "uv"),
    0x19: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x1A: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x1B: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x1C: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x1D: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x1E: (_sig("u8", "signal", "signal", "signal"), "uv"),
    0x1F: (_sig(), "uv"),
    0x22: (_sig("u8", "u16"), "uv"),
    0x2B: (_sig("u16", "u16", "u16"), "raster"),
    0x2C: (_sig("u16", "u16", "u8", "u8"), "raster"),
    0x2D: (_sig("u16", "u16", "u16"), "raster"),
    0x2E: (_sig("u16", "u16", "u16", ("enum", 2)), "raster"),
    0x2F: (_sig("u16", "u16", "u8"), "raster"),
    0x30: (_sig("u16", "u16", "u8"), "raster"),
    0x31: (_sig("u16", "u16", "u8"), "raster"),
    0x32: (_sig("u16", "u16", "u16", "u16"), "raster"),
    0x33: (_sig("u16", "u16", "u16"), "raster"),
    0x34: (_sig("u16", "u16", "u16"), "raster"),
    0x35: (_sig(("enum", 3), "u16", "u16", "u16"), "raster"),
}

TRANSFORM_SCHEMAS: dict[int, tuple[tuple[str | tuple[str, int], ...], str]] = {
    0x00: (_sig("bool", "signal"), "uv"),
    0x01: (_sig("signal", "signal"), "uv"),
    0x02: (_sig("signal"), "uv"),
    0x03: (_sig("signal"), "uv"),
    0x04: (_sig("u8", "bool"), "uv"),
    0x05: (_sig("u16", "bool"), "uv"),
    0x07: (_sig("bool", ("enum", 2), "signal"), "uv"),
    0x08: (_sig("signal", "signal", "signal", "signal"), "uv"),
    0x09: (_sig("u16", ("enum", 2), "signal", "signal"), "palette"),
}


def _read_params(
    reader: _Reader,
    params: tuple[str | tuple[str, int], ...],
    depth: int,
) -> None:
    for param in params:
        if param == "u8" or param == "bool":
            reader.u8()
        elif param == "u16":
            reader.u16()
        elif param == "u32":
            reader.u32()
        elif param == "i32":
            reader.i32()
        elif param == "signal":
            _read_signal(reader, depth + 1)
        elif isinstance(param, tuple) and param[0] == "enum":
            value = reader.u8()
            if value > param[1]:
                raise PscValidationError(f"bad enum value {value}")
        else:
            raise AssertionError(f"unknown PSC validator param {param!r}")


def _read_record(reader: _Reader, kind: str) -> tuple[int, _Reader]:
    tag = reader.u8()
    body_len = reader.u16()
    return tag, reader.subreader(body_len)


def _read_signal(reader: _Reader, depth: int = 0) -> None:
    if depth > MAX_RECURSION_DEPTH:
        raise PscValidationError("signal nesting is too deep")
    tag, body = _read_record(reader, "signal")
    schema = SIGNAL_SCHEMAS.get(tag)
    if schema is None:
        raise PscValidationError(f"unknown signal tag 0x{tag:02x}")
    _read_params(body, schema, depth)
    body.expect_end("signal")


def _read_pattern(reader: _Reader) -> tuple[int, str]:
    tag, body = _read_record(reader, "pattern")
    schema = PATTERN_SCHEMAS.get(tag)
    if schema is None:
        raise PscValidationError(f"unknown pattern tag 0x{tag:02x}")
    params, domain = schema
    _read_params(body, params, 0)
    body.expect_end("pattern")
    return tag, domain


def _read_transform(reader: _Reader) -> tuple[int, str]:
    tag, body = _read_record(reader, "transform")
    schema = TRANSFORM_SCHEMAS.get(tag)
    if schema is None:
        raise PscValidationError(f"unknown transform tag 0x{tag:02x}")
    params, domain = schema
    _read_params(body, params, 0)
    body.expect_end("transform")
    return tag, domain


def raw_pattern_tag(data: bytes) -> int | None:
    if len(data) >= 7 and data[:4] == MAGIC and data[4] == VERSION:
        return data[6]
    return None


def validate_psc_scene(data: bytes) -> PscInfo:
    reader = _Reader(data)
    if reader.remaining < len(MAGIC):
        raise PscValidationError(f".psc file is too short ({reader.remaining} bytes)")
    if bytes(reader.u8() for _ in range(4)) != MAGIC:
        raise PscValidationError("bad .psc magic")

    version = reader.u8()
    if version != VERSION:
        raise PscValidationError(f"unsupported .psc version {version}")

    palette_id = reader.u8()
    if palette_id not in PALETTE_IDS:
        raise PscValidationError(f"unknown palette id {palette_id}")

    pattern_tag, pattern_domain = _read_pattern(reader)
    transform_count = reader.u8()
    for _ in range(transform_count):
        transform_tag, transform_domain = _read_transform(reader)
        if pattern_domain == "raster" and transform_domain != "palette":
            raise PscValidationError(
                f"transform tag 0x{transform_tag:02x} is not compatible with raster pattern 0x{pattern_tag:02x}"
            )

    reader.expect_end("scene")
    return PscInfo(pattern_tag=pattern_tag)
