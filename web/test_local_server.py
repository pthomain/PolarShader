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

sys.path.insert(0, str(Path(__file__).resolve().parent))
import local_server  # noqa: E402


VALID_PSC = b"PSC\0\0\0"


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

    def test_cross_origin_text_plain_save_is_rejected(self) -> None:
        status, body = self.request(
            "POST",
            "/api/playlist/save?name=evil.psc",
            VALID_PSC,
            {"Content-Type": "text/plain", "Origin": "http://evil.example"},
        )
        self.assertEqual(status, 403, body)
        self.assertFalse((self.repo_root / "build" / "psc" / "evil.psc").exists())

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
        self.wait_job(body["job"]["id"])

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
