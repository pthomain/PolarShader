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
    LED_RENDERING_SNIPPET = """class GraphicsManagerThreeJS {
  constructor() {
    this.LED_SCALE = 1;
    this.SCREEN_WIDTH = 0;
  }
  createGrid(frameData) {
    let isDenseScreenMap = true;
    for (const screenMap of Object.values(this.screenMaps)) {
      if (!isDenseGrid(screenMap)) {
        isDenseScreenMap = false;
        break;
      }
    }
    const { defaultDotSize, normalizedScale } = this._calculateDotSizes(
      frameData,
      ledPositions,
      width,
      height,
      calcXPosition,
      isDenseScreenMap
    );
    let maxLedVisualRadius = 0;
    for (const strip of Object.values(screenMap.strips)) {
        const visualRadius = isDenseScreenMap ? diameter * this.LED_SCALE / 2 : diameter * this.LED_SCALE;
        maxLedVisualRadius = Math.max(maxLedVisualRadius, visualRadius);
    }
    return { isDenseScreenMap, maxLedVisualRadius };
  }
  /**
   * Creates the LED material. Sparse screen maps use a radial alpha falloff
   * so neighbouring pixels blend instead of drawing as hard overlapping discs.
   * @private
   */
  _createLedMaterial(isDenseScreenMap, color, useVertexColors = false) {
    const { THREE } = this.threeJsModules;
    if (isDenseScreenMap) {
      return new THREE.MeshBasicMaterial({ color, vertexColors: useVertexColors });
    }

    const material = new THREE.ShaderMaterial({
      vertexColors: useVertexColors
    });
    return material;
  }
  /**
   * Creates LED objects for each pixel in the frame data
   * @private
   */
  _createLedObjects(frameData, calcXPosition, calcYPosition, isDenseScreenMap, defaultDotSize, normalizedScale) {
    const canMergeGeometries = this.useMergedGeometry && BufferGeometryUtils && !DISABLE_MERGE_GEOMETRIES;
    if (!canMergeGeometries) {
      let ignored = stripDiameter * this.LED_SCALE;
          const material = new THREE.MeshBasicMaterial({ color: 0 });
    }
        const material = new THREE.MeshBasicMaterial({
          color: 16777215,
          vertexColors: true
        });
  }
  _collectLedColorData(frameData) {
    const data = frameData[0].pixel_data;
    const r = (data[0] & 255) / 255;
    const g = (data[1] & 255) / 255;
    const b = (data[2] & 255) / 255;
    const brightness = (r + g + b) / 3;
    return brightness;
  }
  /**
   * Updates LED visuals based on position map data
   * @private
   */
  _updateLedVisuals(positionMap) {
    const mergedMeshUpdates = new Map();
    for (const [, ledData] of positionMap) {
      const led = this.leds[0];
      const z = this._calculateDepthEffect();
      if (led._isMerged) {
        led.material.color.setRGB(ledData.r, ledData.g, ledData.b);
        if (led._parentMesh) {
          mergedMeshUpdates.get(led._parentMesh).push({
            index: led._mergedIndex,
            color: new THREE.Color(ledData.r, ledData.g, ledData.b)
          });
        }
      } else {
        led.position.set(0, 0, z);
        led.material.color.setRGB(ledData.r, ledData.g, ledData.b);
      }
    }
  }
}
"""

    def test_bloom_strength_patch_uses_reduced_glow_constants(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            assets = dist / "assets"
            assets.mkdir()
            graphics = assets / "graphics_manager_threejs-test.js"
            graphics.write_text(self.BLOOM_STRENGTH_SNIPPET, encoding="utf-8")

            build_site.patch_threejs_bloom(dist)

            patched = graphics.read_text(encoding="utf-8")
            self.assertIn("this.bloom_stength = 0.04375;", patched)
            self.assertIn("this.base_bloom_strength = 0.7;", patched)
            self.assertIn("this.max_bloom_strength = 0.875;", patched)
            self.assertIn("this.min_bloom_strength = 0.021875;", patched)
            self.assertIn("this.bloom_stength = 0.7;", patched)

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

    def test_led_gradient_patch_uses_sparse_rendering_for_all_displays(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            assets = dist / "assets"
            assets.mkdir()
            graphics = assets / "graphics_manager_threejs-test.js"
            graphics.write_text(self.LED_RENDERING_SNIPPET, encoding="utf-8")

            build_site.patch_threejs_led_gradient(dist)

            patched = graphics.read_text(encoding="utf-8")
            self.assertIn("isDenseScreenMap = false;", patched)
            self.assertIn("this.LED_BRIGHTNESS_MULTIPLIER = 2;", patched)
            self.assertIn("_scalePreviewLedChannel(value)", patched)
            self.assertIn("const ledScale = this.LED_SCALE * this.SPARSE_LED_SCALE;", patched)
            self.assertIn(
                "const visualRadius = diameter * this.LED_SCALE * this.SPARSE_LED_SCALE;",
                patched,
            )
            self.assertIn("new THREE.ShaderMaterial", patched)
            self.assertNotIn("return new THREE.MeshBasicMaterial({ color, vertexColors: useVertexColors });", patched)
            self.assertIn("All screen maps use the sparse radial alpha", patched)
            self.assertIn("const material = this._createLedMaterial(isDenseScreenMap, 16777215, true);", patched)
            self.assertIn("const displayR = this._scalePreviewLedChannel(ledData.r);", patched)
            self.assertIn("led.material.color.setRGB(displayR, displayG, displayB);", patched)
            self.assertIn("color: new THREE.Color(displayR, displayG, displayB)", patched)
            self.assertIn("const brightness = (r + g + b) / 3;", patched)


if __name__ == "__main__":
    unittest.main()
