#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from web import build_site


class BuildSitePatchTest(unittest.TestCase):
    def test_bloom_toggle_patch_keeps_direct_pass_reference(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            assets = dist / "assets"
            assets.mkdir()
            graphics = assets / "graphics_manager_threejs-test.js"
            graphics.write_text(
                """class GraphicsManagerThreeJS {
  constructor() {
    this.auto_bloom_enabled = true;
  }
  setup() {
    const bloomPass = {};
      this.composer.addPass(bloomPass);
  }
  /**
   * Updates the bloom pass strength dynamically
   */
  _updateBloomStrength(strength) {
    if (!this.composer || !this.composer.passes) {
      return;
    }
    for (const pass of this.composer.passes) {
        pass.strength = strength;
        this.bloom_stength = strength;
    }
  }
}
""",
                encoding="utf-8",
            )

            build_site.patch_threejs_bloom_toggle(dist)

            patched = graphics.read_text(encoding="utf-8")
            self.assertIn("this.bloom_pass = null;", patched)
            self.assertIn("this.bloom_pass = bloomPass;", patched)
            self.assertIn("this.bloom_pass.enabled = this.bloom_enabled;", patched)
            self.assertIn("pass === this.bloom_pass", patched)


if __name__ == "__main__":
    unittest.main()
