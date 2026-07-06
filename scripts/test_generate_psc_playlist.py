#!/usr/bin/env python3

from __future__ import annotations

from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_psc_playlist  # noqa: E402


class GeneratePscPlaylistTest(unittest.TestCase):
    def test_invalid_display_brightness_is_ignored_with_warning(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            project_dir = Path(tmp)
            display_config = project_dir / "build" / "display_config.json"
            display_config.parent.mkdir(parents=True)
            display_config.write_text(json.dumps({"brightness": 300}), encoding="utf-8")
            output_header = project_dir / "out" / "PscPlaylistAssets.h"

            stdout = io.StringIO()
            with redirect_stdout(stdout):
                generate_psc_playlist.generate(project_dir, output_header)

            config = output_header.with_name("PscPlaylistConfig.h").read_text(encoding="utf-8")
            self.assertIn("brightness must be between 0 and 255", stdout.getvalue())
            self.assertNotIn("POLAR_SHADER_DISPLAY_BRIGHTNESS", config)

    def test_valid_display_brightness_is_embedded(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            project_dir = Path(tmp)
            display_config = project_dir / "build" / "display_config.json"
            display_config.parent.mkdir(parents=True)
            display_config.write_text(json.dumps({"brightness": 42}), encoding="utf-8")
            output_header = project_dir / "out" / "PscPlaylistAssets.h"

            generate_psc_playlist.generate(project_dir, output_header)

            config = output_header.with_name("PscPlaylistConfig.h").read_text(encoding="utf-8")
            self.assertIn("#define POLAR_SHADER_DISPLAY_BRIGHTNESS 42", config)


if __name__ == "__main__":
    unittest.main()
