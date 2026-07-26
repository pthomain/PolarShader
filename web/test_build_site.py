#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from web import build_site


class BuildSitePatchTest(unittest.TestCase):
    BLOOM_STRENGTH_SNIPPET = """class GraphicsManagerThreeJS {
  constructor() {
    this.bloom_stength = 1;
    this.bloom_radius = 16;
    this.base_bloom_strength = 16;
    this.max_bloom_strength = 20;
    this.min_bloom_strength = 0.5;
  }
  setup() {
      this.bloom_stength = 16;
      this.bloom_radius = 1;
  }
}
"""
    GRAPHICS_MANAGER_SNIPPET = """class GraphicsManagerThreeJS {
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
"""
    WORKER_GRAPHICS_MANAGER_SNIPPET = """      class GraphicsManagerThreeJS {
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
"""

    def test_bloom_strength_patch_reduces_current_glow_by_75_percent(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            assets = dist / "assets"
            assets.mkdir()
            graphics = assets / "graphics_manager_threejs-test.js"
            graphics.write_text(self.BLOOM_STRENGTH_SNIPPET, encoding="utf-8")

            build_site.patch_threejs_bloom(dist)

            patched = graphics.read_text(encoding="utf-8")
            self.assertIn("this.bloom_stength = 0.0625;", patched)
            self.assertIn("this.base_bloom_strength = 1;", patched)
            self.assertIn("this.max_bloom_strength = 1.25;", patched)
            self.assertIn("this.min_bloom_strength = 0.03125;", patched)
            self.assertIn("this.bloom_stength = 1;", patched)

    def test_bloom_toggle_patch_keeps_direct_pass_reference(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            assets = dist / "assets"
            assets.mkdir()
            graphics = assets / "graphics_manager_threejs-test.js"
            graphics.write_text(self.GRAPHICS_MANAGER_SNIPPET, encoding="utf-8")

            build_site.patch_threejs_bloom_toggle(dist)

            patched = graphics.read_text(encoding="utf-8")
            self.assertIn("this.bloom_pass = null;", patched)
            self.assertIn("this.bloom_pass = bloomPass;", patched)
            self.assertIn("this.bloom_pass.enabled = this.bloom_enabled;", patched)
            self.assertIn("pass === this.bloom_pass", patched)

    def test_bloom_toggle_patch_does_not_confuse_worker_handler_for_method(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            worker = dist / "fastled_background_worker.js"
            worker.write_text(
                self.WORKER_GRAPHICS_MANAGER_SNIPPET
                + """
function handleComposerSetBloom(enabled) {
  gm.setBloomEnabled(enabled);
}
""",
                encoding="utf-8",
            )

            build_site.patch_threejs_bloom_toggle(dist)

            patched = worker.read_text(encoding="utf-8")
            self.assertIn("gm.setBloomEnabled(enabled);", patched)
            self.assertIn("this.bloom_pass = null;", patched)
            self.assertIn("this.bloom_pass = bloomPass;", patched)


if __name__ == "__main__":
    unittest.main()
