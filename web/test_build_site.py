#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from web import build_site


class BuildSitePatchTest(unittest.TestCase):
    WORKER_SCRIPT_SNIPPET = """function handleWorkerMessage(type, payload) {
  let response;
  switch (type) {
      default:
        throw new Error(`Unknown message type: ${type}`);
  }
  return response;
}
function startWorker(workerState) {
    workerState.externFunctions.externSetup();
    workerLog("LOG", "BACKGROUND_WORKER", "FastLED setup completed");
}
function createGraphicsManager() {
    workerLog("LOG", "BACKGROUND_WORKER", "Graphics manager initialized", {});
}
"""
    LED_RENDERING_SNIPPET = """class GraphicsManagerThreeJS {
  constructor() {
    this.LED_SCALE = 1;
    this.SCREEN_WIDTH = 0;
  }
  _setupRenderPasses() {
    if (this.bloom_stength > 0 || this.bloom_radius > 0) {
      const bloomPass = {};
      this.composer.addPass(bloomPass);
    }
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

    const uniforms = { diffuse: { value: new THREE.Color(color) } };
    const material = new THREE.ShaderMaterial({
      uniforms,
      vertexColors: useVertexColors,
      fragmentShader: `
        uniform vec3 diffuse;
        varying vec2 vUv;
        void main() {
          float radius = length((vUv - vec2(0.5)) * 2.0);
          float alpha = clamp(1.0 - radius, 0.0, 1.0);
          gl_FragColor = vec4(diffuse, alpha);
        }
      `
    });
    material.color = uniforms.diffuse.value;
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
            self.assertIn("this.LED_HALO_MAX_SCALE = 2.25;", patched)
            self.assertIn("this.LED_HALO_LIMIT_SCALE = this.LED_HALO_MAX_SCALE * 10;", patched)
            self.assertIn("this.LED_CORE_SCALE = 0.5;", patched)
            self.assertIn("this.LED_DIFFUSION_SCALE = this.LED_HALO_MAX_SCALE;", patched)
            self.assertIn("this.LED_BRIGHTNESS_MULTIPLIER = 8;", patched)
            self.assertIn("setLedDiffusionScale(scale, maxScale = this.LED_HALO_MAX_SCALE)", patched)
            self.assertIn("const nextMaxScale =", patched)
            self.assertIn("this.needsRebuild = true;", patched)
            self.assertIn("this._updateLedDiffusionUniforms();", patched)
            self.assertIn("uniform float coreScale;", patched)
            self.assertIn("uniform float haloScale;", patched)
            self.assertIn("uniform float maxHaloScale;", patched)
            self.assertIn("extensions: { derivatives: true }", patched)
            self.assertIn("float physicalRadius = radius * maxHaloScale;", patched)
            self.assertIn("float diffusionRange = max(maxHaloScale - coreScale, 0.0001);", patched)
            self.assertIn(
                "float diffusionAmount = clamp((haloScale - coreScale) / diffusionRange, 0.0, 1.0);",
                patched,
            )
            self.assertIn("float antialiasWidth = fwidth(physicalRadius);", patched)
            self.assertIn("float diffusionBlurWidth = coreScale * 0.45 * diffusionAmount;", patched)
            self.assertIn("float coreEdgeWidth = max(antialiasWidth, diffusionBlurWidth);", patched)
            self.assertIn(
                "float coreAlpha = 1.0 - smoothstep(coreScale - coreEdgeWidth, coreScale + coreEdgeWidth, physicalRadius);",
                patched,
            )
            self.assertIn("float haloUnit = haloScale > 0.0 ? physicalRadius / haloScale : 1.0;", patched)
            self.assertIn("float haloAlpha = clamp(1.0 - haloUnit, 0.0, 1.0);", patched)
            self.assertIn("float alpha = max(coreAlpha, haloAlpha);", patched)
            self.assertIn("material.uniforms.haloScale.value = this.LED_DIFFUSION_SCALE;", patched)
            self.assertNotIn("smoothstep(coreRadius, diffusionRadius, radius)", patched)
            self.assertNotIn("smoothstep(coreScale, coreScale + 0.001, physicalRadius)", patched)
            self.assertIn("_scalePreviewLedChannel(value)", patched)
            self.assertIn("const ledScale = this.LED_SCALE * this.LED_HALO_MAX_SCALE;", patched)
            self.assertIn(
                "const visualRadius = diameter * this.LED_SCALE * this.LED_HALO_MAX_SCALE;",
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
            self.assertNotIn("const bloomPass", patched)
            self.assertNotIn("this.composer.addPass(bloomPass);", patched)

    def test_led_halo_scale_patch_updates_existing_core_scale(self) -> None:
        text = (
            "    this.LED_HALO_MAX_SCALE = 2.25;\n"
            "    this.LED_CORE_SCALE = 1;\n"
            "    this.LED_DIFFUSION_SCALE = this.LED_HALO_MAX_SCALE;\n"
        )

        patched = build_site.insert_led_halo_scales(text, Path("graphics.js"))

        self.assertIn("    this.LED_HALO_LIMIT_SCALE = this.LED_HALO_MAX_SCALE * 10;\n", patched)
        self.assertIn("    this.LED_CORE_SCALE = 0.5;\n", patched)
        self.assertNotIn("this.LED_CORE_SCALE = 1;", patched)

    def test_led_diffusion_setter_patch_updates_existing_setter(self) -> None:
        text = """
  /**
   * Updates LED diffusion/halo spread without changing LED geometry size.
   * @param {number} scale - Absolute halo diameter scale
   */
  setLedDiffusionScale(scale) {
    const rawScale = Number.isFinite(scale) && scale > 0 ? scale : this.LED_HALO_MAX_SCALE;
    const nextScale = Math.max(this.LED_CORE_SCALE, Math.min(this.LED_HALO_MAX_SCALE, rawScale));
    if (Math.abs(this.LED_DIFFUSION_SCALE - nextScale) < 0.0001) {
      return;
    }
    this.LED_DIFFUSION_SCALE = nextScale;
    this._updateLedDiffusionUniforms();
  }
  _updateLedDiffusionUniforms() {
    const updateMaterial = (material) => {
      if (!material || !material.uniforms || !material.uniforms.haloScale) return;
      material.uniforms.coreScale.value = this.LED_CORE_SCALE;
      material.uniforms.haloScale.value = this.LED_DIFFUSION_SCALE;
      material.uniforms.maxHaloScale.value = this.LED_HALO_MAX_SCALE;
    };
  }
  /**
   * Scales a web-preview LED color channel.
   * @private
   */
  _scalePreviewLedChannel(value) {
    return value;
  }
"""

        patched = build_site.replace_led_diffusion_setter(text, Path("graphics.js"))

        self.assertIn("setLedDiffusionScale(scale, maxScale = this.LED_HALO_MAX_SCALE)", patched)
        self.assertIn("const nextMaxScale =", patched)
        self.assertIn("this.needsRebuild = true;", patched)
        self.assertNotIn("setLedDiffusionScale(scale) {", patched)

    def test_led_core_alpha_patch_updates_existing_shader(self) -> None:
        text = """
  _createLedMaterial(isDenseScreenMap, color, useVertexColors = false) {
    const material = new THREE.ShaderMaterial({
      uniforms,
      transparent: true,
      depthWrite: false,
      vertexShader: `
        void main() {
          float radius = length((vUv - vec2(0.5)) * 2.0);
          float physicalRadius = radius * maxHaloScale;
          float coreAlpha = 1.0 - smoothstep(coreScale, coreScale + 0.001, physicalRadius);
          float haloUnit = haloScale > 0.0 ? physicalRadius / haloScale : 1.0;
          float haloAlpha = clamp(1.0 - haloUnit, 0.0, 1.0);
          float alpha = max(coreAlpha, haloAlpha);
        }
      `
    });
  }
"""

        patched = build_site.replace_led_core_alpha_blur(text, Path("graphics.js"))
        patched = build_site.ensure_led_material_derivatives_extension(patched, Path("graphics.js"))

        self.assertIn("extensions: { derivatives: true }", patched)
        self.assertIn("float antialiasWidth = fwidth(physicalRadius);", patched)
        self.assertIn("float diffusionBlurWidth = coreScale * 0.45 * diffusionAmount;", patched)
        self.assertIn(
            "float coreAlpha = 1.0 - smoothstep(coreScale - coreEdgeWidth, coreScale + coreEdgeWidth, physicalRadius);",
            patched,
        )
        self.assertIn("float haloUnit = haloScale > 0.0 ? physicalRadius / haloScale : 1.0;", patched)
        self.assertIn("float haloAlpha = clamp(1.0 - haloUnit, 0.0, 1.0);", patched)
        self.assertNotIn("smoothstep(coreScale, coreScale + 0.001, physicalRadius)", patched)

    def test_generated_bloom_controls_are_removed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            (dist / "index.html").write_text(
                """<div class="menu-dropdown-item" data-value="gfx-bloom">bloom</div>
                <div class="menu-dropdown-item" data-value="toggle-auto-bloom">
                    <input type="checkbox" id="auto-bloom-toggle" checked>
                    <label for="auto-bloom-toggle">Auto Bloom</label>
                </div>
""",
                encoding="utf-8",
            )
            (dist / "app.js").write_text(
                """function handleGraphicsOptionChange(option) {
  if (option === "toggle-auto-bloom") {
    return;
  }
  let gfxParam = "";
  if (option === "gfx-simple") {
    gfxParam = "0";
  } else if (option === "gfx-bloom") {
    gfxParam = "1";
  }
}
var autoBloomToggle = document.getElementById("auto-bloom-toggle");
if (autoBloomToggle) {
  autoBloomToggle.addEventListener("change", function(e) {
    if (window.graphicsManager) {
      window.graphicsManager.auto_bloom_enabled = this.checked;
    }
  });
}
""",
                encoding="utf-8",
            )

            build_site.remove_generated_bloom_controls(dist)

            index = (dist / "index.html").read_text(encoding="utf-8")
            script = (dist / "app.js").read_text(encoding="utf-8")
            self.assertNotIn("gfx-bloom", index)
            self.assertNotIn("auto-bloom-toggle", index)
            self.assertNotIn("gfx-bloom", script)
            self.assertNotIn("auto-bloom-toggle", script)

    def test_render_worker_patch_adds_diffusion_message(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dist = Path(tmp)
            worker = dist / "fastled_background_worker.js"
            worker.write_text(self.WORKER_SCRIPT_SNIPPET, encoding="utf-8")

            build_site.patch_render_worker(dist)

            patched = worker.read_text(encoding="utf-8")
            self.assertIn('case "composer_set_diffusion":', patched)
            self.assertIn("function handleComposerSetDiffusion(payload)", patched)
            self.assertIn('const rawMaxScale = payload && typeof payload.maxScale === "number" ? payload.maxScale : 2.25;', patched)
            self.assertIn("workerState.composerDiffusionScale = scale;", patched)
            self.assertIn("workerState.composerDiffusionMaxScale = maxScale;", patched)
            self.assertIn("workerState.graphicsManager.setLedDiffusionScale(workerState.composerDiffusionScale, workerState.composerDiffusionMaxScale);", patched)
            self.assertIn("gm.LED_DIFFUSION_SCALE = scale;", patched)
            self.assertIn("return { ok: true, scale, maxScale };", patched)
            self.assertNotIn("gm.SPARSE_LED_SCALE = scale;", patched)


if __name__ == "__main__":
    unittest.main()
