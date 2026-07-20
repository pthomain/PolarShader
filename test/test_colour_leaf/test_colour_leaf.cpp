//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Exact colour-leaf tests for the (hue, value) palette-tint path.
 *
 * A test-only UVPattern emits a KNOWN PaletteSample{hue, value} (no field
 * maths), so Layer::compile()'s colour branch and Layer::tintPalette can be
 * asserted against a known palette exactly:
 *   - normal mode: ColorFromPalette selects index map16_to_8(hue)+paletteOffset
 *     (the native stub ignores brightness, so value->bright is proven on
 *     WASM/hardware only);
 *   - clip enabled: computeClipMask gates the emitted value channel;
 *   - colour-mask mode: single paletteOffset tint with value-driven alpha.
 */

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif
#include <unity.h>
#include <memory>

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/PipelineContext.h"
#include "renderer/layer/Layer.h"
#include "renderer/layer/LayerBuilder.h"

#ifndef ARDUINO
#include "renderer/pipeline/maths/src/PolarMaths.cpp"
#include "renderer/pipeline/maths/src/PatternMaths.cpp"
#include "renderer/pipeline/patterns/src/base/UVPattern.cpp"
#include "renderer/layer/src/Layer.cpp"
#include "renderer/layer/src/LayerBuilder.cpp"
#endif

using namespace PolarShader;

// A pattern that emits a fixed (hue, value) pair for every UV, and captures the
// live PipelineContext that compile() hands to colourLayer(), so tests can
// toggle clip / colour-mask / offset on the exact context tintPalette reads.
class TestColourPattern : public UVPattern {
public:
    PatternNormU16 hueValue;
    PatternNormU16 valValue;
    mutable std::shared_ptr<PipelineContext> seenCtx;

    TestColourPattern(uint16_t h, uint16_t v)
        : hueValue(h), valValue(v) {}

    bool emitsColour() const override { return true; }

    UVColourMap colourLayer(const std::shared_ptr<PipelineContext> &context) const override {
        seenCtx = context; // same object compile() captures for tintPalette
        PaletteSample s{hueValue, valValue};
        return [s](UV) { return s; };
    }
};

// A scalar pattern (no emitted hue) that returns a fixed intensity for every
// UV. Used to assert the scalar mapPalette path: greyscale by default and a
// brightness-preserving hue-remap when a palette transform opts in.
class TestScalarPattern : public UVPattern {
public:
    PatternNormU16 valValue;
    mutable std::shared_ptr<PipelineContext> seenCtx;

    explicit TestScalarPattern(uint16_t v) : valValue(v) {}

    UVMap layer(const std::shared_ptr<PipelineContext> &context) const override {
        seenCtx = context;
        PatternNormU16 v = valValue;
        return [v](UV) { return v; };
    }
};

class TestRgbPattern : public UVPattern {
public:
    RgbSample sampleValue;
    mutable std::shared_ptr<PipelineContext> seenCtx;

    TestRgbPattern(uint16_t r, uint16_t g, uint16_t b, uint16_t value)
        : sampleValue(PatternNormU16(r), PatternNormU16(g), PatternNormU16(b), PatternNormU16(value)) {}

    UVLayer uvLayer(const std::shared_ptr<PipelineContext> &context) const override {
        seenCtx = context;
        RgbSample sample = sampleValue;
        return UVLayer::fromRgb([sample](UV) { return sample; });
    }
};

static CRGBPalette16 distinctPalette() {
    CRGBPalette16 p;
    for (int i = 0; i < 16; ++i) {
        p.entries[i] = CRGB(static_cast<uint8_t>(16 + i * 15),
                            static_cast<uint8_t>(240 - i * 14),
                            static_cast<uint8_t>(i * 17));
    }
    return p;
}

static void assertEqualCRGB(const CRGB &a, const CRGB &b, const char *msg) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(a.r, b.r, msg);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(a.g, b.g, msg);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(a.b, b.b, msg);
}

// hue -> index math is exact; a fixed sample point suffices (field is constant).
static constexpr uint16_t kHue = 0x3A00;   // map16_to_8 -> 0x3A (58); %16 -> 10
static constexpr uint16_t kVal = 0xC000;

// Build a layer over the test pattern and return both the compiled map and a
// live handle to the shared context (owned by the Layer via the pattern).
struct Harness {
    Layer layer;
    TestColourPattern *pattern;
};

static Harness makeHarness(const CRGBPalette16 &pal) {
    auto up = std::make_unique<TestColourPattern>(kHue, kVal);
    TestColourPattern *raw = up.get();
    Layer l = LayerBuilder(std::move(up), pal, "colour-leaf-test").build();
    return Harness{std::move(l), raw};
}

static RenderPoint testPoint() {
    return RenderPoint{u0x16(0), u0x16(0x8000), RasterPoint{}};
}

struct ScalarHarness {
    Layer layer;
    TestScalarPattern *pattern;
};

static ScalarHarness makeScalarHarness(const CRGBPalette16 &pal, uint16_t value) {
    auto up = std::make_unique<TestScalarPattern>(value);
    TestScalarPattern *raw = up.get();
    Layer l = LayerBuilder(std::move(up), pal, "scalar-leaf-test").build();
    return ScalarHarness{std::move(l), raw};
}

struct RgbHarness {
    Layer layer;
    TestRgbPattern *pattern;
};

static RgbHarness makeRgbHarness(
    const CRGBPalette16 &pal,
    uint16_t r,
    uint16_t g,
    uint16_t b,
    uint16_t value
) {
    auto up = std::make_unique<TestRgbPattern>(r, g, b, value);
    TestRgbPattern *raw = up.get();
    Layer l = LayerBuilder(std::move(up), pal, "rgb-leaf-test").build();
    return RgbHarness{std::move(l), raw};
}

// Default (Native) mode: a scalar pattern renders greyscale, so its intensity
// maps straight to an (v,v,v) RGB and value 0 stays black. The palette is not
// applied unless a palette transform opts in.
void test_scalar_default_is_greyscale() {
    CRGBPalette16 pal = distinctPalette();
    ScalarHarness h = makeScalarHarness(pal, kVal);
    auto cm = h.layer.compile();
    TEST_ASSERT_NOT_NULL(cm.get());

    CRGB got = (*cm)(testPoint());
    uint8_t v = fl::map16_to_8(kVal);
    assertEqualCRGB(got, CRGB(v, v, v), "scalar default should be greyscale");
}

void test_scalar_zero_value_stays_black() {
    CRGBPalette16 pal = distinctPalette();
    ScalarHarness h = makeScalarHarness(pal, 0);
    auto cm = h.layer.compile();

    // Native (default): greyscale black.
    assertEqualCRGB((*cm)(testPoint()), CRGB::Black, "scalar zero should be black (native)");

    // Colour-mask: single tint with value-driven alpha; alpha 0 -> black.
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::ColourMask;
    assertEqualCRGB((*cm)(testPoint()), CRGB::Black, "scalar zero should be black (colour-mask)");
}

// HueRemap: intensity selects the palette index (offset = phase). The native
// ColorFromPalette stub ignores brightness, so index selection is asserted
// here; value->brightness is proven on WASM/hardware.
void test_scalar_hue_remap_indexes_palette() {
    CRGBPalette16 pal = distinctPalette();
    ScalarHarness h = makeScalarHarness(pal, kVal);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    h.pattern->seenCtx->paletteOffset = 6;

    CRGB got = (*cm)(testPoint());
    uint8_t index = static_cast<uint8_t>(fl::map16_to_8(kVal) + 6);
    assertEqualCRGB(got, pal.entries[index % 16], "scalar hue-remap index mismatch");
}

void test_rgb_native_scales_unpremultiplied_colour_by_value() {
    CRGBPalette16 pal = distinctPalette();
    RgbHarness h = makeRgbHarness(pal, U0X16_MAX, 0x8000u, 0x4000u, 0x8000u);
    auto cm = h.layer.compile();
    TEST_ASSERT_NOT_NULL(cm.get());

    CRGB got = (*cm)(testPoint());
    CRGB expected(
        fl::map16_to_8(scale16(U0X16_MAX, 0x8000u)),
        fl::map16_to_8(scale16(0x8000u, 0x8000u)),
        fl::map16_to_8(scale16(0x4000u, 0x8000u))
    );
    assertEqualCRGB(got, expected, "rgb native value scaling mismatch");
}

void test_rgb_native_clip_gates_by_value_channel() {
    CRGBPalette16 pal = distinctPalette();
    RgbHarness h = makeRgbHarness(pal, U0X16_MAX, 0, 0, 0x4000u);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteClipEnabled = true;
    h.pattern->seenCtx->paletteClip = PatternNormU16(0x4001u);
    h.pattern->seenCtx->paletteClipFeather = u0x16(0);

    assertEqualCRGB((*cm)(testPoint()), CRGB::Black, "rgb native clip should gate by value");
}

void test_rgb_colour_mask_ignores_rgb_and_uses_value_alpha() {
    CRGBPalette16 pal = distinctPalette();
    RgbHarness h = makeRgbHarness(pal, U0X16_MAX, 0, 0, kVal);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::ColourMask;
    h.pattern->seenCtx->paletteOffset = 9;

    CRGB got = (*cm)(testPoint());
    uint16_t alpha = scale16(kVal, U0X16_MAX);
    CRGB expected = pal.entries[9 % 16];
    expected.nscale8_video(static_cast<uint8_t>(alpha >> 8));
    assertEqualCRGB(got, expected, "rgb colour-mask tint/alpha mismatch");
}

void test_rgb_hue_remap_indexes_palette_from_rgb_hue() {
    CRGBPalette16 pal = distinctPalette();
    RgbHarness h = makeRgbHarness(pal, U0X16_MAX, 0, 0, kVal);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    h.pattern->seenCtx->paletteOffset = 5;

    CRGB got = (*cm)(testPoint());
    assertEqualCRGB(got, pal.entries[5 % 16], "rgb hue-remap red hue mismatch");
}

void test_rgb_hue_remap_grey_and_near_black_pin_hue_to_zero() {
    CRGBPalette16 pal = distinctPalette();
    RgbHarness grey = makeRgbHarness(pal, 0x7000u, 0x7000u, 0x7001u, kVal);
    auto greyMap = grey.layer.compile();
    grey.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    grey.pattern->seenCtx->paletteOffset = 2;
    assertEqualCRGB((*greyMap)(testPoint()), pal.entries[2 % 16], "grey rgb hue should pin to zero");

    RgbHarness nearBlack = makeRgbHarness(pal, U0X16_MAX, 0, 0, 0);
    auto nearBlackMap = nearBlack.layer.compile();
    nearBlack.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    assertEqualCRGB((*nearBlackMap)(testPoint()), CRGB::Black, "zero-value rgb should stay black");

    RgbHarness blackRgb = makeRgbHarness(pal, 0, 0, 0, kVal);
    auto blackRgbMap = blackRgb.layer.compile();
    blackRgb.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    assertEqualCRGB((*blackRgbMap)(testPoint()), CRGB::Black, "black rgb should stay black");
}

void test_rgb_hue_remap_blue_hue_is_lossy_but_stable() {
    CRGBPalette16 pal = distinctPalette();
    RgbHarness h = makeRgbHarness(pal, 0, 0, U0X16_MAX, kVal);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;

    CRGB got = (*cm)(testPoint());
    uint8_t blueHue = fl::map16_to_8(static_cast<uint16_t>(4 * 10923));
    assertEqualCRGB(got, pal.entries[blueHue % 16], "rgb hue-remap blue hue mismatch");
}

void test_normal_mode_exact_index() {
    CRGBPalette16 pal = distinctPalette();
    Harness h = makeHarness(pal);
    // HueRemap: offset 0, clip disabled, colour-mask off.
    auto cm = h.layer.compile();
    TEST_ASSERT_NOT_NULL(cm.get());
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;

    CRGB got = (*cm)(testPoint());
    uint8_t index = fl::map16_to_8(kHue); // + offset 0
    CRGB expected = pal.entries[index % 16];
    assertEqualCRGB(got, expected, "normal-mode index mismatch");
}

void test_normal_mode_offset_shifts_index() {
    CRGBPalette16 pal = distinctPalette();
    Harness h = makeHarness(pal);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    h.pattern->seenCtx->paletteOffset = 5;

    CRGB got = (*cm)(testPoint());
    uint8_t index = static_cast<uint8_t>(fl::map16_to_8(kHue) + 5);
    CRGB expected = pal.entries[index % 16];
    assertEqualCRGB(got, expected, "offset did not shift index");
}

void test_clip_below_value_passes_full_colour() {
    CRGBPalette16 pal = distinctPalette();
    Harness h = makeHarness(pal);
    auto cm = h.layer.compile();
    // clip strictly below the value channel, feather 0 -> mask == U0X16_MAX,
    // so the colour is the un-scaled palette entry.
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    h.pattern->seenCtx->paletteClipEnabled = true;
    h.pattern->seenCtx->paletteClip = PatternNormU16(kVal - 1);
    h.pattern->seenCtx->paletteClipFeather = u0x16(0);

    CRGB got = (*cm)(testPoint());
    CRGB expected = pal.entries[fl::map16_to_8(kHue) % 16];
    assertEqualCRGB(got, expected, "value above clip should pass full colour");
}

void test_clip_above_value_gates_to_black() {
    CRGBPalette16 pal = distinctPalette();
    Harness h = makeHarness(pal);
    auto cm = h.layer.compile();
    // clip above the value channel, feather 0 -> mask == 0 -> black.
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    h.pattern->seenCtx->paletteClipEnabled = true;
    h.pattern->seenCtx->paletteClip = PatternNormU16(kVal + 1);
    h.pattern->seenCtx->paletteClipFeather = u0x16(0);

    CRGB got = (*cm)(testPoint());
    assertEqualCRGB(got, CRGB::Black, "value below clip should gate to black");
}

void test_colour_mask_single_tint_with_value_alpha() {
    CRGBPalette16 pal = distinctPalette();
    Harness h = makeHarness(pal);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::ColourMask;
    h.pattern->seenCtx->paletteOffset = 7;

    CRGB got = (*cm)(testPoint());
    // Mirror tintPalette's colour-mask branch: single tint at offset, alpha =
    // scale16(value, mask), mask == U0X16_MAX (clip disabled).
    uint16_t alpha = scale16(kVal, U0X16_MAX);
    CRGB expected = pal.entries[7 % 16];
    expected.nscale8_video(static_cast<uint8_t>(alpha >> 8));
    assertEqualCRGB(got, expected, "colour-mask tint/alpha mismatch");
}

void test_native_mode_bypasses_palette_via_chsv() {
    CRGBPalette16 pal = distinctPalette();
    Harness h = makeHarness(pal);
    auto cm = h.layer.compile();
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::Native;
    h.pattern->seenCtx->paletteOffset = 4;

    CRGB got = (*cm)(testPoint());
    // Native renders the emitted hue directly with offset as a hue phase; the
    // palette is bypassed. The native stub honours the value->brightness map.
    uint8_t hue8 = static_cast<uint8_t>(fl::map16_to_8(kHue) + 4);
    uint8_t bright = fl::map16_to_8(kVal);
    CRGB expected = CHSV(hue8, 255, bright);
    assertEqualCRGB(got, expected, "native-mode CHSV mismatch");
}

void test_rainbow_hue_remap_renders_native() {
    CRGBPalette16 pal = distinctPalette();
    Harness h = makeHarness(pal);
    auto cm = h.layer.compile();
    // With a Rainbow palette a HueRemap is redundant and must render natively
    // (offset applied as a hue phase), NOT index the provided palette.
    h.pattern->seenCtx->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
    h.pattern->seenCtx->paletteIsRainbow = true;
    h.pattern->seenCtx->paletteOffset = 3;

    CRGB got = (*cm)(testPoint());
    uint8_t hue8 = static_cast<uint8_t>(fl::map16_to_8(kHue) + 3);
    uint8_t bright = fl::map16_to_8(kVal);
    CRGB expected = CHSV(hue8, 255, bright);
    assertEqualCRGB(got, expected, "rainbow hue-remap should render native");
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_normal_mode_exact_index);
    RUN_TEST(test_normal_mode_offset_shifts_index);
    RUN_TEST(test_clip_below_value_passes_full_colour);
    RUN_TEST(test_clip_above_value_gates_to_black);
    RUN_TEST(test_colour_mask_single_tint_with_value_alpha);
    RUN_TEST(test_native_mode_bypasses_palette_via_chsv);
    RUN_TEST(test_rainbow_hue_remap_renders_native);
    RUN_TEST(test_scalar_default_is_greyscale);
    RUN_TEST(test_scalar_zero_value_stays_black);
    RUN_TEST(test_scalar_hue_remap_indexes_palette);
    RUN_TEST(test_rgb_native_scales_unpremultiplied_colour_by_value);
    RUN_TEST(test_rgb_native_clip_gates_by_value_channel);
    RUN_TEST(test_rgb_colour_mask_ignores_rgb_and_uses_value_alpha);
    RUN_TEST(test_rgb_hue_remap_indexes_palette_from_rgb_hue);
    RUN_TEST(test_rgb_hue_remap_grey_and_near_black_pin_hue_to_zero);
    RUN_TEST(test_rgb_hue_remap_blue_hue_is_lossy_but_stable);
    return UNITY_END();
}
