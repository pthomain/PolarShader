#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import textwrap
import unittest

WEB_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(WEB_DIR))

from build_site import patch_threejs_bloom_toggle


def graphics_manager_fixture(indent: str) -> str:
    return textwrap.dedent(f"""\
        class GraphicsManagerThreeJS {{
        {indent}constructor() {{
        {indent}  this.auto_bloom_enabled = true;
        {indent}}}
        {indent}_setupRenderPasses() {{
        {indent}  const bloomPass = {{}};
        {indent}  this.composer.addPass(bloomPass);
        {indent}}}
        {indent}/**
        {indent} * Updates the bloom pass strength dynamically
        {indent} * @private
        {indent} * @param {{number}} strength - New bloom strength value
        {indent} */
        {indent}_updateBloomStrength(strength) {{
        {indent}  if (!this.composer || !this.composer.passes) {{
        {indent}    return;
        {indent}  }}
        {indent}  for (const pass of this.composer.passes) {{
        {indent}    if (pass.constructor.name === "UnrealBloomPass") {{
        {indent}      pass.strength = strength;
        {indent}      this.bloom_stength = strength;
        {indent}      break;
        {indent}    }}
        {indent}  }}
        {indent}}}
        }}
        """)


class BuildSitePatchTest(unittest.TestCase):
    def test_bloom_toggle_patch_reaches_worker_with_handler_reference(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            dist = Path(temp)
            app_path = dist / "app.js"
            worker_path = dist / "fastled_background_worker.js"
            app_path.write_text(graphics_manager_fixture("  "), encoding="utf-8")
            worker_path.write_text(
                graphics_manager_fixture("      ")
                + "\nfunction handleComposerSetBloom(gm, enabled) {\n"
                + "  gm.setBloomEnabled(enabled);\n"
                + "}\n",
                encoding="utf-8",
            )

            patch_threejs_bloom_toggle(dist)
            patch_threejs_bloom_toggle(dist)

            for path in (app_path, worker_path):
                with self.subTest(path=path.name):
                    text = path.read_text(encoding="utf-8")
                    self.assertEqual(text.count("setBloomEnabled(enabled) {\n"), 1)
                    self.assertEqual(text.count("setBloomStrengthMultiplier(multiplier) {\n"), 1)
                    self.assertIn("this.bloom_enabled = true;", text)
                    self.assertIn("bloomPass.__polarComposerBloomPass = true;", text)
                    self.assertIn("bloomPass.enabled = this.bloom_enabled;", text)
                    self.assertIn("pass.__polarComposerBloomPass === true", text)
                    self.assertIn("this.bloom_strength_base = strength;", text)
                    self.assertIn("pass.strength = scaledStrength;", text)

            self.assertIn("gm.setBloomEnabled(enabled);", worker_path.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
