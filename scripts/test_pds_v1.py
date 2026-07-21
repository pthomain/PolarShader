#!/usr/bin/env python3

from __future__ import annotations

import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pds_v1  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
DISPLAYS_DIR = REPO_ROOT / "displays"


def _geometry_body(led_count: int) -> bytes:
    body = struct.pack("<H", led_count)
    for i in range(led_count):
        body += struct.pack("<HH", (i * 7) & 0xFFFF, (i * 11) & 0xFFFF)
    return body


def _fastled_body() -> bytes:
    body = bytes([pds_v1.BACKEND_FASTLED])
    body += struct.pack("<H", 4) + b"xiao"
    body += bytes([0])                 # chipset WS2812
    body += struct.pack("<H", 5)       # data_pin
    body += struct.pack("<H", 0xFFFF)  # clock_pin none
    body += bytes([2, 200, 16])        # rgb_order, brightness, refresh_ms
    body += struct.pack("<I", 0)       # color_correction
    return body


def _frame(sections: list[tuple[int, int, bytes]], version: int = pds_v1.VERSION) -> bytes:
    out = pds_v1.MAGIC + bytes([version, len(sections)])
    for section_type, flags, body in sections:
        out += bytes([section_type, flags]) + struct.pack("<I", len(body)) + body
    return out


CRIT = pds_v1.SECTION_FLAG_CRITICAL


class PdsFixtureTest(unittest.TestCase):
    EXPECTED = {
        "round": (241, False, pds_v1.BACKEND_FASTLED),
        "fibonacci": (324, False, pds_v1.BACKEND_FASTLED),
        "fabric": (400, True, pds_v1.BACKEND_FASTLED),
        "fabric32x8": (256, True, pds_v1.BACKEND_FASTLED),
        "matrix128": (4096, True, pds_v1.BACKEND_SMARTMATRIX),
    }

    def test_builtin_fixtures_validate(self) -> None:
        for name, (leds, raster, backend) in self.EXPECTED.items():
            data = (DISPLAYS_DIR / f"{name}.pds").read_bytes()
            info = pds_v1.validate_pds(data)
            self.assertEqual(info.led_count, leds, name)
            self.assertEqual(info.has_raster, raster, name)
            self.assertEqual(info.backend_kind, backend, name)
            # Every built-in is deployable.
            pds_v1.validate_deployable(data)


class PdsRejectionTest(unittest.TestCase):
    def _reject(self, data: bytes) -> None:
        with self.assertRaises(pds_v1.PdsValidationError):
            pds_v1.validate_pds(data)

    def test_bad_magic(self) -> None:
        self._reject(b"XDS\0" + bytes([1, 0]))

    def test_bad_version(self) -> None:
        self._reject(_frame([(0x02, CRIT, _geometry_body(2))], version=2))

    def test_trailing_bytes(self) -> None:
        self._reject(_frame([(0x02, CRIT, _geometry_body(2))]) + b"\x00")

    def test_missing_geometry(self) -> None:
        self._reject(_frame([(0x01, CRIT, bytes([0, 0]))]))

    def test_empty_geometry(self) -> None:
        self._reject(_frame([(0x02, CRIT, _geometry_body(0))]))

    def test_dup_section(self) -> None:
        self._reject(_frame([(0x02, CRIT, _geometry_body(2)),
                             (0x02, CRIT, _geometry_body(2))]))

    def test_bad_section_order(self) -> None:
        self._reject(_frame([(0x04, CRIT, _fastled_body()),
                             (0x02, CRIT, _geometry_body(2))]))

    def test_unknown_critical_section(self) -> None:
        self._reject(_frame([(0x02, CRIT, _geometry_body(2)),
                             (0x7F, CRIT, b"\x01\x02")]))

    def test_bad_section_flags(self) -> None:
        self._reject(_frame([(0x02, 0x02, _geometry_body(2))]))

    def test_section_trailing_bytes(self) -> None:
        self._reject(_frame([(0x02, CRIT, _geometry_body(2) + b"\x99")]))

    def test_bad_output_enum(self) -> None:
        body = bytearray(_fastled_body())
        body[7] = 99  # chipset
        self._reject(_frame([(0x02, CRIT, _geometry_body(2)),
                             (0x04, CRIT, bytes(body))]))

    def test_fastled_no_data_pin(self) -> None:
        body = bytearray(_fastled_body())
        body[8] = 0xFF
        body[9] = 0xFF
        self._reject(_frame([(0x02, CRIT, _geometry_body(2)),
                             (0x04, CRIT, bytes(body))]))


class PdsAcceptTest(unittest.TestCase):
    def test_unknown_noncritical_positions(self) -> None:
        unk = (0x7F, 0x00, b"\x09\x09\x09")
        geo = (0x02, CRIT, _geometry_body(3))
        out = (0x04, CRIT, _fastled_body())
        for order in ([unk, geo, out], [geo, unk, out], [geo, out, unk]):
            info = pds_v1.validate_pds(_frame(order))
            self.assertEqual(info.led_count, 3)

    def test_absent_output_is_none(self) -> None:
        info = pds_v1.validate_pds(_frame([(0x02, CRIT, _geometry_body(2))]))
        self.assertEqual(info.backend_kind, pds_v1.BACKEND_NONE)


class PdsDeployTest(unittest.TestCase):
    def test_none_backend_not_deployable(self) -> None:
        data = _frame([(0x02, CRIT, _geometry_body(2))])
        pds_v1.validate_pds(data)  # decodes fine
        with self.assertRaises(pds_v1.PdsValidationError):
            pds_v1.validate_deployable(data)

    def test_sparse_smartmatrix_not_deployable(self) -> None:
        # Build a 2x2 SmartMatrix with a duplicated raster cell.
        geo = _geometry_body(4)
        raster = struct.pack("<HH", 2, 2)
        for x, y in [(0, 0), (0, 0), (1, 0), (1, 1)]:  # duplicate (0,0)
            raster += struct.pack("<HH", x, y)
        sm = bytes([pds_v1.BACKEND_SMARTMATRIX])
        sm += struct.pack("<H", 2) + b"tt"
        sm += struct.pack("<HHHH", 2, 2, 1, 1)  # panel 2x2, chain 1x1
        sm += bytes([1, 24, 8, 0])              # subsample, refresh_depth, dma, panel_type
        sm += struct.pack("<II", 0, 0)          # options
        sm += bytes([20, 30])
        data = _frame([(0x02, CRIT, geo), (0x03, CRIT, raster), (0x04, CRIT, sm)])
        pds_v1.validate_pds(data)  # decodes fine (sparse ok for preview)
        with self.assertRaises(pds_v1.PdsValidationError):
            pds_v1.validate_deployable(data)


if __name__ == "__main__":
    unittest.main()
