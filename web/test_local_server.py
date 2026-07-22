#!/usr/bin/env python3

from __future__ import annotations

import http.client
import json
from pathlib import Path
import sys
import tempfile
import threading
import time
import unittest

WEB_DIR = Path(__file__).resolve().parent
REPO_ROOT = WEB_DIR.parent
sys.path.insert(0, str(WEB_DIR))
sys.path.insert(0, str(REPO_ROOT / "scripts"))
import local_server  # noqa: E402
import psc_v1  # noqa: E402


VALID_PSC = bytes([
    0x50, 0x53, 0x43, 0x00,
    0x01,
    0x00,
    0x00, 0x05, 0x00,
    0x00, 0x02, 0x00, 0xF4, 0x01,
    0x00,
])
VALID_PSC_ALT = bytes([
    0x50, 0x53, 0x43, 0x00,
    0x01,
    0x00,
    0x00, 0x05, 0x00,
    0x00, 0x02, 0x00, 0xF5, 0x01,
    0x00,
])
LEGACY_PALETTE_TRANSFORM_PSC = bytes([
    0x50, 0x53, 0x43, 0x00,
    0x01,
    0x00,
    0x00, 0x05, 0x00,
    0x00, 0x02, 0x00, 0xF4, 0x01,
    0x01,
    0x06, 0x05, 0x00,
    0x00, 0x02, 0x00, 0x64, 0x00,
])
JS_XOR_FIXTURE = bytes([
    0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x22, 0x03, 0x00, 0x10, 0x28, 0x00,
    0x00,
])
JS_CONWAY_PALETTE_FIXTURE = bytes([
    0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x2B, 0x06, 0x00, 0xFA, 0x00, 0x01,
    0x00, 0x5E, 0x01, 0x01, 0x09, 0x0D, 0x00, 0x00, 0x80, 0x02, 0x00, 0x02,
    0x00, 0xF4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
])
JS_NESTED_SIGNALS_PALETTE_CLIP_FIXTURE = bytes([
    0x50, 0x53, 0x43, 0x00, 0x01, 0x02, 0x00, 0x19, 0x00, 0x1F, 0x16, 0x00,
    0x10, 0x09, 0x00, 0x00, 0x02, 0x00, 0x32, 0x00, 0xD2, 0x04, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x02, 0x00, 0x84, 0x03, 0x02, 0x09,
    0x21, 0x00, 0x00, 0x40, 0x01, 0x10, 0x09, 0x00, 0x00, 0x02, 0x00, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x0F, 0x00, 0x00, 0x02, 0x00, 0xD9,
    0x00, 0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x02, 0x00, 0x20, 0x03, 0x02,
    0x1B, 0x00, 0x20, 0x18, 0x00, 0x1B, 0x13, 0x00, 0x00, 0x02, 0x00, 0x4D,
    0x01, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x96, 0x00, 0x00, 0x02,
    0x00, 0x52, 0x03, 0x00, 0x80,
])


class PscValidationTest(unittest.TestCase):
    def test_validator_accepts_current_cross_impl_fixtures(self) -> None:
        fixtures = [
            ("simple", VALID_PSC, 0x00),
            ("xor", JS_XOR_FIXTURE, 0x22),
            ("conway", JS_CONWAY_PALETTE_FIXTURE, 0x2B),
            ("nested paletteClip", JS_NESTED_SIGNALS_PALETTE_CLIP_FIXTURE, 0x00),
        ]

        for name, payload, expected_tag in fixtures:
            with self.subTest(name=name):
                info = psc_v1.validate_psc_scene(payload)
                self.assertEqual(info.pattern_tag, expected_tag)
                tag, error = local_server._psc_pattern_tag(payload)
                self.assertEqual(tag, expected_tag)
                self.assertIsNone(error)

    def test_validator_accepts_all_raster_pattern_tags(self) -> None:
        def scene_for_pattern(tag: int, body: list[int]) -> bytes:
            return bytes([
                0x50, 0x53, 0x43, 0x00,
                0x01,
                0x00,
                tag, len(body) & 0xFF, (len(body) >> 8) & 0xFF,
                *body,
                0x00,
            ])

        fixtures = [
            (0x2B, [0xFA, 0x00, 0x01, 0x00, 0x5E, 0x01]),
            (0x2C, [0x78, 0x00, 0x01, 0x00, 0x08, 0x01]),
            (0x2D, [0x5A, 0x00, 0x01, 0x00, 0x2C, 0x01]),
            (0x2E, [0xC8, 0x00, 0x01, 0x00, 0x5E, 0x01, 0x00]),
            (0x2F, [0x5A, 0x00, 0x01, 0x00, 0x1E]),
            (0x30, [0x3C, 0x00, 0x01, 0x00, 0x28]),
            (0x31, [0x28, 0x00, 0x01, 0x00, 0x06]),
            (0x32, [0x78, 0x00, 0x01, 0x00, 0x28, 0x00, 0x02, 0x00]),
            (0x33, [0x64, 0x00, 0x01, 0x00, 0xF4, 0x01]),
            (0x34, [0x1E, 0x00, 0x01, 0x00, 0x01, 0x00]),
            (0x35, [0x00, 0x21, 0x00, 0x01, 0x00, 0x04, 0x00]),
        ]

        for tag, body in fixtures:
            with self.subTest(tag=f"0x{tag:02x}"):
                payload = scene_for_pattern(tag, body)
                info = psc_v1.validate_psc_scene(payload)
                self.assertEqual(info.pattern_tag, tag)

                detected_tag, error = local_server._psc_pattern_tag(payload)
                self.assertEqual(detected_tag, tag)
                self.assertIsNone(error)

    def test_validator_rejects_bad_raster_reaction_diffusion_preset(self) -> None:
        payload = bytes([
            0x50, 0x53, 0x43, 0x00,
            0x01,
            0x00,
            0x35, 0x07, 0x00,
            0x04, 0x21, 0x00, 0x01, 0x00, 0x04, 0x00,
            0x00,
        ])

        with self.assertRaisesRegex(psc_v1.PscValidationError, "bad enum value 4"):
            psc_v1.validate_psc_scene(payload)

    def test_validator_accepts_noise_basic_loop(self) -> None:
        payload = bytes([
            0x50, 0x53, 0x43, 0x00,
            0x01,
            0x00,
            0x3C, 0x07, 0x00,
            0x10, 0x27,                        # loopPeriodMs = 10000
            0x00, 0x02, 0x00, 0x26, 0x02,      # depthSpeed constant 550
            0x00,
        ])
        info = psc_v1.validate_psc_scene(payload)
        self.assertEqual(info.pattern_tag, 0x3C)

    def test_validator_rejects_zero_period_noise_basic_loop(self) -> None:
        payload = bytes([
            0x50, 0x53, 0x43, 0x00,
            0x01,
            0x00,
            0x3C, 0x07, 0x00,
            0x00, 0x00,                        # loopPeriodMs = 0 (rejected)
            0x00, 0x02, 0x00, 0x26, 0x02,
            0x00,
        ])
        with self.assertRaisesRegex(psc_v1.PscValidationError, "below min 1"):
            psc_v1.validate_psc_scene(payload)

    def test_validator_rejects_legacy_palette_transform(self) -> None:
        tag, error = local_server._psc_pattern_tag(LEGACY_PALETTE_TRANSFORM_PSC)
        self.assertEqual(tag, 0)
        self.assertEqual(error, "unknown transform tag 0x06")

    def test_playlist_metadata_reads_full_psc_body(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "legacy.psc"
            path.write_bytes(LEGACY_PALETTE_TRANSFORM_PSC)

            metadata = local_server._psc_file_metadata(path)

        self.assertFalse(metadata["supported"])
        self.assertEqual(metadata["patternTag"], 0)
        self.assertEqual(metadata["error"], "unknown transform tag 0x06")

    def test_deploy_start_releases_lock_when_job_creation_fails(self) -> None:
        original_job_class = local_server.DeployJob

        class BrokenDeployJob:
            def __init__(self, *_args, **_kwargs):
                raise RuntimeError("job creation failed")

        with tempfile.TemporaryDirectory() as tmp:
            manager = local_server.DeployManager(
                Path(tmp),
                sys.executable,
                platformio_base_override=[sys.executable],
            )
            local_server.DeployJob = BrokenDeployJob
            try:
                with self.assertRaises(local_server.ApiError) as caught:
                    manager.start_deploy("teensy41_matrix")
            finally:
                local_server.DeployJob = original_job_class

        self.assertEqual(caught.exception.status, 500)
        self.assertFalse(manager.is_deploy_active())


class LocalServerTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.repo_root = self.root / "repo"
        self.dist = self.root / "dist"
        self.repo_root.mkdir()
        self.dist.mkdir()
        (self.dist / "index.html").write_text("<!doctype html><title>test</title>", encoding="utf-8")
        self.fake_pio = self.root / "fake_pio.py"
        self.fake_pio.write_text(
            "\n".join([
                "import json, os, sys, time",
                "args = sys.argv[1:]",
                "if args == ['device', 'list', '--json-output']:",
                "    time.sleep(float(os.environ.get('FAKE_PIO_DEVICE_SLEEP', '0')))",
                "    print(json.dumps([{'port': '/dev/tty.fake', 'description': 'Teensy 4.1'}]))",
                "    raise SystemExit(0)",
                "if args[:1] == ['run']:",
                "    print('fake upload start', flush=True)",
                "    if os.environ.get('FAKE_PIO_PARTIAL'):",
                "        sys.stdout.write('partial progress')",
                "        sys.stdout.flush()",
                "    time.sleep(float(os.environ.get('FAKE_PIO_SLEEP', '0.1')))",
                "    print('fake upload done', flush=True)",
                "    raise SystemExit(int(os.environ.get('FAKE_PIO_EXIT', '0')))",
                "print('unexpected args: ' + repr(args), flush=True)",
                "raise SystemExit(2)",
            ]),
            encoding="utf-8",
        )
        self.manager = local_server.DeployManager(
            self.repo_root,
            sys.executable,
            deploy_timeout=2,
            platformio_base_override=[sys.executable, str(self.fake_pio)],
        )
        local_server.LocalComposerServer.repo_root = self.repo_root
        local_server.LocalComposerServer.playlist_dir = (self.repo_root / "build" / "psc").resolve()
        local_server.LocalComposerServer.display_config_path = (self.repo_root / "build" / "display_config.json").resolve()
        local_server.LocalComposerServer.deploy_manager = self.manager
        self.server = local_server.ThreadedLocalServer(
            ("127.0.0.1", 0),
            lambda *a: local_server.LocalComposerServer(*a, directory=self.dist),
        )
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.port = self.server.server_address[1]
        self.host = f"127.0.0.1:{self.port}"
        self._old_env = {}

    def tearDown(self) -> None:
        for key, value in self._old_env.items():
            if value is None:
                local_server.os.environ.pop(key, None)
            else:
                local_server.os.environ[key] = value
        self.manager.shutdown()
        self.server.shutdown()
        self.server.server_close()
        self.tmp.cleanup()

    def set_env(self, key: str, value: str) -> None:
        if key not in self._old_env:
            self._old_env[key] = local_server.os.environ.get(key)
        local_server.os.environ[key] = value

    def request(self, method: str, path: str, body: bytes = b"", headers: dict | None = None):
        all_headers = {"Host": self.host}
        all_headers.update(headers or {})
        conn = http.client.HTTPConnection("127.0.0.1", self.port, timeout=5)
        conn.request(method, path, body=body, headers=all_headers)
        response = conn.getresponse()
        data = response.read()
        conn.close()
        parsed = json.loads(data.decode("utf-8")) if data else {}
        return response.status, parsed

    def wait_job(self, job_id: str, timeout: float = 4) -> dict:
        deadline = time.time() + timeout
        last = {}
        while time.time() < deadline:
            status, body = self.request("GET", f"/api/deploy/status?id={job_id}")
            self.assertEqual(status, 200)
            last = body["job"]
            if last["status"] in {"succeeded", "failed", "timed_out"}:
                return last
            time.sleep(0.05)
        self.fail(f"job {job_id} did not finish; last={last}")

    def test_origin_absent_same_origin_save_passes(self) -> None:
        status, body = self.request(
            "POST",
            "/api/playlist/save?name=ok.psc",
            VALID_PSC,
            {"Content-Type": "application/octet-stream"},
        )
        self.assertEqual(status, 200, body)
        self.assertTrue((self.repo_root / "build" / "psc" / "ok.psc").is_file())

    def test_playlist_save_collision_gets_incremented_name(self) -> None:
        first_status, first_body = self.request(
            "POST",
            "/api/playlist/save?name=original.psc",
            VALID_PSC,
            {"Content-Type": "application/octet-stream"},
        )
        second_status, second_body = self.request(
            "POST",
            "/api/playlist/save?name=original.psc",
            VALID_PSC,
            {"Content-Type": "application/octet-stream"},
        )
        third_status, third_body = self.request(
            "POST",
            "/api/playlist/save?name=original.psc",
            VALID_PSC,
            {"Content-Type": "application/octet-stream"},
        )
        self.assertEqual(first_status, 200, first_body)
        self.assertEqual(second_status, 200, second_body)
        self.assertEqual(third_status, 200, third_body)
        self.assertEqual(first_body["file"]["name"], "original.psc")
        self.assertEqual(second_body["file"]["name"], "original-1.psc")
        self.assertEqual(third_body["file"]["name"], "original-2.psc")
        self.assertTrue((self.repo_root / "build" / "psc" / "original.psc").is_file())
        self.assertTrue((self.repo_root / "build" / "psc" / "original-1.psc").is_file())
        self.assertTrue((self.repo_root / "build" / "psc" / "original-2.psc").is_file())

    def test_playlist_save_overwrite_updates_existing_file(self) -> None:
        path = self.repo_root / "build" / "psc" / "existing.psc"
        path.parent.mkdir(parents=True)
        path.write_bytes(b"PSC\0\0old")

        status, body = self.request(
            "POST",
            "/api/playlist/save?name=existing.psc&overwrite=1",
            VALID_PSC_ALT,
            {"Content-Type": "application/octet-stream"},
        )

        self.assertEqual(status, 200, body)
        self.assertEqual(body["file"]["name"], "existing.psc")
        self.assertEqual(path.read_bytes(), VALID_PSC_ALT)
        self.assertFalse((self.repo_root / "build" / "psc" / "existing-1.psc").exists())

    def test_playlist_save_overwrite_missing_file_is_404(self) -> None:
        status, body = self.request(
            "POST",
            "/api/playlist/save?name=missing.psc&overwrite=1",
            VALID_PSC,
            {"Content-Type": "application/octet-stream"},
        )

        self.assertEqual(status, 404, body)
        self.assertFalse((self.repo_root / "build" / "psc" / "missing.psc").exists())

    def test_playlist_delete_removes_file(self) -> None:
        path = self.repo_root / "build" / "psc" / "delete-me.psc"
        path.parent.mkdir(parents=True)
        path.write_bytes(VALID_PSC)

        status, body = self.request(
            "POST",
            "/api/playlist/delete?name=delete-me.psc",
            headers={"Content-Type": "text/plain"},
        )

        self.assertEqual(status, 200, body)
        self.assertEqual(body["file"]["name"], "delete-me.psc")
        self.assertFalse(path.exists())

    def test_display_config_default_and_save(self) -> None:
        status, body = self.request("GET", "/api/display/config")
        self.assertEqual(status, 200, body)
        self.assertEqual(body["brightness"], 255)
        self.assertFalse(body["saved"])

        save_status, save_body = self.request(
            "POST",
            "/api/display/config",
            json.dumps({"brightness": 42}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )

        self.assertEqual(save_status, 200, save_body)
        self.assertEqual(save_body["brightness"], 42)
        self.assertTrue((self.repo_root / "build" / "display_config.json").is_file())

        next_status, next_body = self.request("GET", "/api/display/config")
        self.assertEqual(next_status, 200, next_body)
        self.assertEqual(next_body["brightness"], 42)
        self.assertTrue(next_body["saved"])

    def test_display_config_rejects_invalid_brightness(self) -> None:
        status, body = self.request(
            "POST",
            "/api/display/config",
            json.dumps({"brightness": 300}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(status, 400, body)

    def test_display_config_invalid_file_returns_default_with_warning(self) -> None:
        config = self.repo_root / "build" / "display_config.json"
        config.parent.mkdir(parents=True)
        config.write_text(json.dumps({"brightness": 300}), encoding="utf-8")

        status, body = self.request("GET", "/api/display/config")

        self.assertEqual(status, 200, body)
        self.assertEqual(body["brightness"], 255)
        self.assertFalse(body["saved"])
        self.assertIn("brightness must be between 0 and 255", body["warning"])

    def test_cross_origin_text_plain_save_is_rejected(self) -> None:
        status, body = self.request(
            "POST",
            "/api/playlist/save?name=evil.psc",
            VALID_PSC,
            {"Content-Type": "text/plain", "Origin": "http://evil.example"},
        )
        self.assertEqual(status, 403, body)
        self.assertFalse((self.repo_root / "build" / "psc" / "evil.psc").exists())

    def test_cross_origin_text_plain_delete_is_rejected(self) -> None:
        path = self.repo_root / "build" / "psc" / "evil.psc"
        path.parent.mkdir(parents=True)
        path.write_bytes(VALID_PSC)

        status, body = self.request(
            "POST",
            "/api/playlist/delete?name=evil.psc",
            b"ignored",
            {"Content-Type": "text/plain", "Origin": "http://evil.example"},
        )

        self.assertEqual(status, 403, body)
        self.assertTrue(path.exists())

    def test_cross_origin_text_plain_display_config_is_rejected(self) -> None:
        status, body = self.request(
            "POST",
            "/api/display/config",
            json.dumps({"brightness": 42}).encode("utf-8"),
            {"Content-Type": "text/plain", "Origin": "http://evil.example"},
        )
        self.assertEqual(status, 403, body)
        self.assertFalse((self.repo_root / "build" / "display_config.json").exists())

    def test_cross_origin_text_plain_deploy_is_rejected(self) -> None:
        status, body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "text/plain", "Origin": "http://evil.example"},
        )
        self.assertEqual(status, 403, body)

    def test_hostile_host_is_rejected(self) -> None:
        status, body = self.request(
            "POST",
            "/api/playlist/save?name=evil.psc",
            VALID_PSC,
            {"Host": "evil.example"},
        )
        self.assertEqual(status, 403, body)

    def test_device_detection_requires_post(self) -> None:
        status, body = self.request("GET", "/api/deploy/devices")
        self.assertEqual(status, 404, body)

    def test_deploy_rejects_non_hardware_env(self) -> None:
        status, body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "native_composer"}).encode("utf-8"),
            {"Content-Type": "text/plain"},
        )
        self.assertEqual(status, 400, body)

    def test_concurrent_deploy_is_rejected_without_queueing(self) -> None:
        self.set_env("FAKE_PIO_SLEEP", "0.5")
        first_status, first_body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(first_status, 202, first_body)

        second_status, second_body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(second_status, 409, second_body)

        job = self.wait_job(first_body["job"]["id"])
        self.assertEqual(job["status"], "succeeded", job)
        self.assertIn("fake upload start", job["logs"])
        self.assertIn("fake upload done", job["logs"])

    def test_deploy_captures_partial_output_without_newline(self) -> None:
        self.set_env("FAKE_PIO_PARTIAL", "1")
        status, body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(status, 202, body)
        job = self.wait_job(body["job"]["id"])
        self.assertEqual(job["status"], "succeeded", job)
        self.assertIn("partial progress", job["logs"])

    def test_device_detection_is_blocked_during_deploy(self) -> None:
        self.set_env("FAKE_PIO_SLEEP", "0.5")
        status, body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(status, 202, body)
        device_status, device_body = self.request("POST", "/api/deploy/devices")
        self.assertEqual(device_status, 409, device_body)
        self.assertEqual(device_body["error"], "deploy already in progress")
        self.wait_job(body["job"]["id"])

    def test_build_input_mutations_are_blocked_during_deploy(self) -> None:
        keep_path = self.repo_root / "build" / "psc" / "keep.psc"
        keep_path.parent.mkdir(parents=True)
        keep_path.write_bytes(VALID_PSC)

        self.set_env("FAKE_PIO_SLEEP", "1")
        status, body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(status, 202, body)
        self.assertTrue(self.manager.is_deploy_active())

        config_status, config_body = self.request(
            "POST",
            "/api/display/config",
            json.dumps({"brightness": 42}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(config_status, 409, config_body)
        self.assertFalse((self.repo_root / "build" / "display_config.json").exists())

        save_status, save_body = self.request(
            "POST",
            "/api/playlist/save?name=blocked.psc",
            VALID_PSC,
            {"Content-Type": "application/octet-stream"},
        )
        self.assertEqual(save_status, 409, save_body)
        self.assertFalse((self.repo_root / "build" / "psc" / "blocked.psc").exists())

        delete_status, delete_body = self.request(
            "POST",
            "/api/playlist/delete?name=keep.psc",
            headers={"Content-Type": "text/plain"},
        )
        self.assertEqual(delete_status, 409, delete_body)
        self.assertTrue(keep_path.exists())

        self.wait_job(body["job"]["id"])

    def test_deploy_is_blocked_during_device_detection(self) -> None:
        self.set_env("FAKE_PIO_DEVICE_SLEEP", "0.5")
        result = {}

        def run_detection() -> None:
            result["device"] = self.request("POST", "/api/deploy/devices")

        thread = threading.Thread(target=run_detection)
        thread.start()
        deadline = time.time() + 2
        while not self.manager.is_deploy_active() and time.time() < deadline:
            time.sleep(0.01)
        self.assertTrue(self.manager.is_deploy_active())

        deploy_status, deploy_body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(deploy_status, 409, deploy_body)
        self.assertEqual(deploy_body["error"], "device detection in progress")

        thread.join(timeout=3)
        self.assertFalse(thread.is_alive())
        device_status, device_body = result["device"]
        self.assertEqual(device_status, 200, device_body)
        self.assertIn("devices", device_body)

    def test_concurrent_device_detection_is_rejected(self) -> None:
        self.set_env("FAKE_PIO_DEVICE_SLEEP", "0.5")
        result = {}

        def run_detection() -> None:
            result["device"] = self.request("POST", "/api/deploy/devices")

        thread = threading.Thread(target=run_detection)
        thread.start()
        deadline = time.time() + 2
        while not self.manager.is_deploy_active() and time.time() < deadline:
            time.sleep(0.01)
        self.assertTrue(self.manager.is_deploy_active())

        second_status, second_body = self.request("POST", "/api/deploy/devices")
        self.assertEqual(second_status, 409, second_body)
        self.assertEqual(second_body["error"], "device detection in progress")

        thread.join(timeout=3)
        self.assertFalse(thread.is_alive())
        first_status, first_body = result["device"]
        self.assertEqual(first_status, 200, first_body)
        self.assertIn("devices", first_body)

    def test_deploy_timeout_reports_timeout_and_releases_lock(self) -> None:
        self.manager.deploy_timeout = 0.2
        self.set_env("FAKE_PIO_SLEEP", "5")
        status, body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(status, 202, body)
        job = self.wait_job(body["job"]["id"])
        self.assertEqual(job["status"], "timed_out", job)
        self.assertIn("timed out", job["error"])
        self.assertIsNotNone(job["exitCode"])

        self.manager.deploy_timeout = 2
        self.set_env("FAKE_PIO_SLEEP", "0")
        next_status, next_body = self.request(
            "POST",
            "/api/deploy",
            json.dumps({"env": "teensy41_matrix"}).encode("utf-8"),
            {"Content-Type": "application/json"},
        )
        self.assertEqual(next_status, 202, next_body)
        next_job = self.wait_job(next_body["job"]["id"])
        self.assertEqual(next_job["status"], "succeeded", next_job)


if __name__ == "__main__":
    unittest.main()
