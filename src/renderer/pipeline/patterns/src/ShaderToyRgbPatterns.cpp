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

#include "renderer/pipeline/patterns/ShaderToyRgbPatterns.h"
#include "renderer/pipeline/maths/ShaderMaths.h"
#include <limits.h>
#include <utility>

namespace PolarShader {
    namespace {
        constexpr int32_t ST_Q16_HALF = 0x00008000;
        constexpr int32_t Q16_PI = 205887;
        constexpr int32_t Q16_TAU = 411775;
        constexpr int32_t Q16_HALF_PI = 102944;
        constexpr int32_t ST_Q16_INV_TAU = 10430;

        constexpr int32_t Q16_0_005 = 328;
        constexpr int32_t Q16_0_01 = 655;
        constexpr int32_t Q16_0_02 = 1311;
        constexpr int32_t Q16_0_03 = 1966;
        constexpr int32_t Q16_0_035 = 2294;
        constexpr int32_t Q16_0_04 = 2621;
        constexpr int32_t Q16_0_042 = 2753;
        constexpr int32_t Q16_0_05 = 3277;
        constexpr int32_t Q16_0_064 = 4194;
        constexpr int32_t Q16_0_07 = 4588;
        constexpr int32_t Q16_0_10 = 6554;
        constexpr int32_t Q16_0_12 = 7864;
        constexpr int32_t Q16_0_20 = 13107;
        constexpr int32_t Q16_0_25 = 16384;
        constexpr int32_t Q16_0_30 = 19661;
        constexpr int32_t Q16_0_40 = 26214;
        constexpr int32_t Q16_0_50 = 32768;
        constexpr int32_t Q16_0_55 = 36045;
        constexpr int32_t Q16_0_60 = 39322;
        constexpr int32_t Q16_0_70 = 45875;
        constexpr int32_t Q16_0_75 = 49152;
        constexpr int32_t Q16_0_80 = 52429;
        constexpr int32_t Q16_0_90 = 58982;
        constexpr int32_t Q16_1_30 = 85197;
        constexpr int32_t Q16_1_34 = 87818;
        constexpr int32_t Q16_1_50 = 98304;
        constexpr int32_t Q16_1_525 = 99942;
        constexpr int32_t Q16_1_60 = 104858;
        constexpr int32_t Q16_1_75 = 114688;
        constexpr int32_t Q16_1_80 = 117965;
        constexpr int32_t Q16_2_00 = 131072;
        constexpr int32_t Q16_2_43 = 159253;
        constexpr int32_t Q16_2_51 = 164495;
        constexpr int32_t Q16_2_50 = 163840;
        constexpr int32_t Q16_3_00 = 196608;
        constexpr int32_t Q16_4_00 = 262144;
        constexpr int32_t Q16_5_00 = 327680;
        constexpr int32_t Q16_6_00 = 393216;
        constexpr int32_t Q16_8_00 = 524288;
        constexpr int32_t Q16_9_00 = 589824;
        constexpr int32_t Q16_12_00 = 786432;
        constexpr int32_t Q16_15_00 = 983040;
        constexpr int32_t Q16_23_00 = 1507328;
        constexpr int32_t Q16_46_00 = 3014656;
        constexpr int32_t Q16_100_PI = 20588742;
        constexpr int32_t Q16_OFFSET_1_8M_MOD_TAU = 369595;
        constexpr int32_t Q16_OFFSET_2_35M_MOD_TAU = 47875;

        struct Vec3Q16 {
            fl::s16x16 x;
            fl::s16x16 y;
            fl::s16x16 z;
        };

        int32_t absRaw(int32_t value) {
            if (value == INT32_MIN) return INT32_MAX;
            return value < 0 ? -value : value;
        }

        int32_t clampRaw(int64_t value, int32_t minValue, int32_t maxValue) {
            if (value < minValue) return minValue;
            if (value > maxValue) return maxValue;
            return static_cast<int32_t>(value);
        }

        int32_t wrapRadiansRaw(int64_t value) {
            int64_t mod = value % Q16_TAU;
            if (mod < 0) mod += Q16_TAU;
            return static_cast<int32_t>(mod);
        }

        int32_t floorToIntRaw(int32_t valueRaw) {
            int64_t value = valueRaw;
            if (value >= 0) return static_cast<int32_t>(value >> 16);
            return -static_cast<int32_t>((-value + SF16_ONE - 1) >> 16);
        }

        int32_t shaderToyDisplayAspectRaw(const std::shared_ptr<PipelineContext> &context) {
            if (!(context && context->rasterDisplay.valid && context->rasterDisplay.height > 0)) {
                return SF16_ONE;
            }
            uint64_t scaled = static_cast<uint64_t>(context->rasterDisplay.width) << 16;
            return static_cast<int32_t>(scaled / context->rasterDisplay.height);
        }

        uint16_t displayWidth(const std::shared_ptr<PipelineContext> &context) {
            if (!(context && context->rasterDisplay.valid && context->rasterDisplay.width > 0)) {
                return 128;
            }
            return context->rasterDisplay.width;
        }

        fl::s16x16 shaderToyAxis(fl::s16x16 axis) {
            int64_t centred = static_cast<int64_t>(raw(axis)) * 2 - SF16_ONE;
            return fl::s16x16::from_raw(clampRaw(centred, INT32_MIN, INT32_MAX));
        }

        int32_t divSignedRaw(int32_t numRaw, int32_t denRaw, int32_t maxAbsRaw = INT32_MAX) {
            if (denRaw == 0) return numRaw < 0 ? -maxAbsRaw : maxAbsRaw;
            int64_t value = (static_cast<int64_t>(numRaw) << 16) / denRaw;
            return clampRaw(value, -maxAbsRaw, maxAbsRaw);
        }

        uint32_t divUnsignedRaw(uint32_t numRaw, uint32_t denRaw, uint32_t maxRaw) {
            if (denRaw == 0u) return maxRaw;
            uint64_t value = (static_cast<uint64_t>(numRaw) << 16) / denRaw;
            return value > maxRaw ? maxRaw : static_cast<uint32_t>(value);
        }

        uint32_t mulUnsignedRaw(uint32_t valueRaw, uint32_t scaleRaw) {
            uint64_t value = (static_cast<uint64_t>(valueRaw) * scaleRaw) >> 16;
            return value > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(value);
        }

        uint32_t squareRaw(uint32_t valueRaw, uint32_t maxRaw) {
            uint64_t value = (static_cast<uint64_t>(valueRaw) * valueRaw) >> 16;
            return value > maxRaw ? maxRaw : static_cast<uint32_t>(value);
        }

        int32_t dot2Raw(Vec2Q16 a, Vec2Q16 b) {
            return clampRaw(
                static_cast<int64_t>(mulQ16Raw(raw(a.x), raw(b.x))) +
                static_cast<int64_t>(mulQ16Raw(raw(a.y), raw(b.y))),
                INT32_MIN,
                INT32_MAX
            );
        }

        int32_t dot3Raw(Vec3Q16 a, Vec3Q16 b) {
            return clampRaw(
                static_cast<int64_t>(mulQ16Raw(raw(a.x), raw(b.x))) +
                static_cast<int64_t>(mulQ16Raw(raw(a.y), raw(b.y))) +
                static_cast<int64_t>(mulQ16Raw(raw(a.z), raw(b.z))),
                INT32_MIN,
                INT32_MAX
            );
        }

        fl::s16x16 length3Q16(Vec3Q16 v) {
            int64_t x = absRaw(raw(v.x));
            int64_t y = absRaw(raw(v.y));
            int64_t z = absRaw(raw(v.z));
            uint64_t sum =
                static_cast<uint64_t>(x) * static_cast<uint64_t>(x) +
                static_cast<uint64_t>(y) * static_cast<uint64_t>(y) +
                static_cast<uint64_t>(z) * static_cast<uint64_t>(z);
            uint64_t root = sqrtU64Raw(sum);
            if (root > static_cast<uint64_t>(INT32_MAX)) root = static_cast<uint64_t>(INT32_MAX);
            return fl::s16x16::from_raw(static_cast<int32_t>(root));
        }

        uint16_t angleFromRadiansRaw(int32_t radiansRaw) {
            int64_t turns = (static_cast<int64_t>(radiansRaw) * ST_Q16_INV_TAU) >> 16;
            return static_cast<uint16_t>(turns);
        }

        int32_t sinRadiansRaw(int32_t radiansRaw) {
            return raw(angleSinF16(f16(angleFromRadiansRaw(radiansRaw))));
        }

        int32_t cosRadiansRaw(int32_t radiansRaw) {
            return raw(angleCosF16(f16(angleFromRadiansRaw(radiansRaw))));
        }

        uint16_t unsignedCosRadiansRaw(int32_t radiansRaw) {
            return raw(toUnsignedClamped(angleCosF16(f16(angleFromRadiansRaw(radiansRaw)))));
        }

        Vec2Q16 rotate2(Vec2Q16 v, int32_t radiansRaw) {
            const int32_t c = cosRadiansRaw(radiansRaw);
            const int32_t s = sinRadiansRaw(radiansRaw);
            return Vec2Q16{
                fl::s16x16::from_raw(clampRaw(
                    static_cast<int64_t>(mulQ16Raw(raw(v.x), c)) -
                    static_cast<int64_t>(mulQ16Raw(raw(v.y), s)),
                    INT32_MIN,
                    INT32_MAX
                )),
                fl::s16x16::from_raw(clampRaw(
                    static_cast<int64_t>(mulQ16Raw(raw(v.x), s)) +
                    static_cast<int64_t>(mulQ16Raw(raw(v.y), c)),
                    INT32_MIN,
                    INT32_MAX
                ))
            };
        }

        Vec2Q16 shaderUvHeight(UV uv, int32_t aspectRaw) {
            return Vec2Q16{
                fl::s16x16::from_raw(mulQ16Raw(raw(shaderToyAxis(uv.u)), aspectRaw)),
                shaderToyAxis(uv.v)
            };
        }

        Vec2Q16 shaderUvMin(UV uv, int32_t aspectRaw) {
            fl::s16x16 x = shaderToyAxis(uv.u);
            fl::s16x16 y = shaderToyAxis(uv.v);
            if (aspectRaw >= SF16_ONE) {
                x = fl::s16x16::from_raw(mulQ16Raw(raw(x), aspectRaw));
            } else if (aspectRaw > 0) {
                y = fl::s16x16::from_raw(divSignedRaw(raw(y), aspectRaw, 32 * SF16_ONE));
            }
            return Vec2Q16{x, y};
        }

        Vec2Q16 shaderUvWidth(UV uv, int32_t aspectRaw) {
            fl::s16x16 y = shaderToyAxis(uv.v);
            if (aspectRaw > 0) {
                y = fl::s16x16::from_raw(divSignedRaw(raw(y), aspectRaw, 32 * SF16_ONE));
            }
            return Vec2Q16{shaderToyAxis(uv.u), y};
        }

        uint32_t defaultUnitRaw(uint16_t permille) {
            if (permille >= 1000u) return SF16_ONE;
            return (static_cast<uint32_t>(permille) * 0xFFFFu) / 1000u + 1u;
        }

        uint32_t signalUnitRaw(const Sf16Signal &signal, TimeMillis elapsedMs, uint16_t fallbackPermille) {
            if (!signal) return defaultUnitRaw(fallbackPermille);
            int32_t sampled = raw(signal.sample(magnitudeRange(), elapsedMs));
            if (sampled <= 0) return 0u;
            if (sampled >= SF16_MAX - 1) return SF16_ONE;
            return static_cast<uint32_t>(sampled) + 1u;
        }

        int32_t lerpByUnitRaw(int32_t aRaw, int32_t bRaw, uint32_t unitRaw) {
            if (unitRaw >= SF16_ONE) return bRaw;
            int64_t value =
                static_cast<int64_t>(aRaw) +
                ((static_cast<int64_t>(bRaw - aRaw) * unitRaw) >> 16);
            return clampRaw(value, INT32_MIN, INT32_MAX);
        }

        uint8_t lerpIntByUnit(uint8_t a, uint8_t b, uint32_t unitRaw) {
            int32_t value = lerpByUnitRaw(
                static_cast<int32_t>(a) << 16,
                static_cast<int32_t>(b) << 16,
                unitRaw
            );
            return static_cast<uint8_t>((value + ST_Q16_HALF) >> 16);
        }

        uint16_t lerpPermilleByUnit(uint16_t a, uint16_t b, uint32_t unitRaw) {
            int32_t value = lerpByUnitRaw(
                static_cast<int32_t>(a) << 16,
                static_cast<int32_t>(b) << 16,
                unitRaw
            );
            return static_cast<uint16_t>((value + ST_Q16_HALF) >> 16);
        }

        uint32_t secondsRaw(TimeMillis elapsedMs) {
            return static_cast<uint32_t>((static_cast<uint64_t>(elapsedMs) << 16) / 1000u);
        }

        void shaderToySaturatingAdd(uint32_t &target, uint32_t value) {
            uint32_t previous = target;
            target += value;
            if (target < previous) target = UINT32_MAX;
        }

        uint16_t shaderToyClampAccumToU16(uint32_t value) {
            return value > F16_MAX ? F16_MAX : static_cast<uint16_t>(value);
        }

        PatternNormU16 shaderToyUnpremultiplyChannel(uint32_t channel, uint32_t maxChannel) {
            if (maxChannel == 0u) return PatternNormU16(0);
            if (maxChannel > F16_MAX) return PatternNormU16(shaderToyClampAccumToU16(channel));
            uint64_t scaled = (static_cast<uint64_t>(channel) << 16) / maxChannel;
            if (scaled > F16_MAX) scaled = F16_MAX;
            return PatternNormU16(static_cast<uint16_t>(scaled));
        }

        RgbSample rgbFromPremultiplied(uint32_t r, uint32_t g, uint32_t b) {
            uint32_t maxChannel = r;
            if (g > maxChannel) maxChannel = g;
            if (b > maxChannel) maxChannel = b;
            if (maxChannel == 0u) return RgbSample();
            return RgbSample(
                shaderToyUnpremultiplyChannel(r, maxChannel),
                shaderToyUnpremultiplyChannel(g, maxChannel),
                shaderToyUnpremultiplyChannel(b, maxChannel),
                PatternNormU16(shaderToyClampAccumToU16(maxChannel))
            );
        }

        uint32_t tanhQ16(uint32_t xRaw) {
            if (xRaw >= static_cast<uint32_t>(8 * SF16_ONE)) return F16_MAX;
            uint32_t twoX = xRaw >= static_cast<uint32_t>(4 * SF16_ONE)
                ? static_cast<uint32_t>(8 * SF16_ONE)
                : xRaw << 1;
            uint32_t e = raw(expNegQ16(fl::u16x16::from_raw(twoX)));
            if (e >= static_cast<uint32_t>(SF16_ONE)) return 0u;
            uint32_t numerator = static_cast<uint32_t>(SF16_ONE) - e;
            uint32_t denominator = static_cast<uint32_t>(SF16_ONE) + e;
            uint32_t value = divUnsignedRaw(numerator, denominator, SF16_ONE);
            return value > F16_MAX ? F16_MAX : value;
        }

        uint32_t toneMapRocaille(uint32_t accumulatedRaw, uint32_t glowRaw) {
            uint32_t glowing = mulUnsignedRaw(accumulatedRaw, glowRaw);
            if (glowing >= static_cast<uint32_t>(8 * SF16_ONE)) return F16_MAX;
            return tanhQ16(squareRaw(glowing, 64u * SF16_ONE));
        }

        int32_t floorModRaw(int32_t valueRaw, int32_t periodRaw) {
            int32_t mod = valueRaw % periodRaw;
            if (mod < 0) mod += periodRaw;
            return mod;
        }

        uint32_t linstepRaw(int32_t aRaw, int32_t bRaw, int32_t xRaw) {
            if (xRaw <= aRaw) return 0u;
            if (xRaw >= bRaw) return SF16_ONE;
            return divUnsignedRaw(
                static_cast<uint32_t>(xRaw - aRaw),
                static_cast<uint32_t>(bRaw - aRaw),
                SF16_ONE
            );
        }

        uint32_t smoothstepSignedRaw(int32_t edge0Raw, int32_t edge1Raw, int32_t xRaw) {
            if (edge0Raw == edge1Raw) return xRaw < edge0Raw ? 0u : SF16_ONE;
            bool reversed = edge1Raw < edge0Raw;
            int32_t lo = reversed ? edge1Raw : edge0Raw;
            int32_t hi = reversed ? edge0Raw : edge1Raw;
            uint32_t t = linstepRaw(lo, hi, xRaw);
            uint32_t t2 = mulUnsignedRaw(t, t);
            uint32_t shaped = mulUnsignedRaw(t2, static_cast<uint32_t>(3 * SF16_ONE) - (t << 1));
            return reversed ? static_cast<uint32_t>(SF16_ONE) - shaped : shaped;
        }

        uint32_t starFieldTravelBirthFadeRaw(int32_t cycleRaw) {
            constexpr int32_t fadeWindowRaw = Q16_0_12;
            int32_t distanceFromBirthRaw = SF16_ONE - cycleRaw;
            if (distanceFromBirthRaw <= 0) return 0u;
            if (distanceFromBirthRaw >= fadeWindowRaw) return SF16_ONE;
            return smoothstepSignedRaw(0, fadeWindowRaw, distanceFromBirthRaw);
        }

        int32_t mixSignedRaw(int32_t aRaw, int32_t bRaw, uint32_t tRaw) {
            return lerpByUnitRaw(aRaw, bRaw, tRaw);
        }

        uint32_t mixUnsignedRaw(uint32_t aRaw, uint32_t bRaw, uint32_t tRaw) {
            if (tRaw >= SF16_ONE) return bRaw;
            int64_t value =
                static_cast<int64_t>(aRaw) +
                ((static_cast<int64_t>(bRaw) - static_cast<int64_t>(aRaw)) * tRaw >> 16);
            if (value < 0) return 0u;
            if (value > UINT32_MAX) return UINT32_MAX;
            return static_cast<uint32_t>(value);
        }

        int32_t sdBox(Vec3Q16 p, Vec3Q16 halfSize) {
            int32_t qx = absRaw(raw(p.x)) - raw(halfSize.x);
            int32_t qy = absRaw(raw(p.y)) - raw(halfSize.y);
            int32_t qz = absRaw(raw(p.z)) - raw(halfSize.z);
            Vec3Q16 positive{
                fl::s16x16::from_raw(qx > 0 ? qx : 0),
                fl::s16x16::from_raw(qy > 0 ? qy : 0),
                fl::s16x16::from_raw(qz > 0 ? qz : 0)
            };
            int32_t outside = raw(length3Q16(positive));
            int32_t inside = qx;
            if (qy > inside) inside = qy;
            if (qz > inside) inside = qz;
            if (inside > 0) inside = 0;
            return clampRaw(static_cast<int64_t>(outside) + inside, INT32_MIN, INT32_MAX);
        }

        int32_t octBox(Vec3Q16 pos, int32_t scaleRaw) {
            Vec3Q16 scaled{
                fl::s16x16::from_raw(mulQ16Raw(raw(pos.x), scaleRaw)),
                fl::s16x16::from_raw(mulQ16Raw(raw(pos.y), scaleRaw)),
                fl::s16x16::from_raw(mulQ16Raw(raw(pos.z), scaleRaw))
            };
            int32_t base = sdBox(
                scaled,
                Vec3Q16{
                    fl::s16x16::from_raw(Q16_0_40),
                    fl::s16x16::from_raw(Q16_0_40),
                    fl::s16x16::from_raw(Q16_0_10)
                }
            );
            return -divSignedRaw(base, Q16_1_50, 64 * SF16_ONE);
        }

        Vec3Q16 rotateXy(Vec3Q16 pos, int32_t angleRaw) {
            Vec2Q16 xy = rotate2(Vec2Q16{pos.x, pos.y}, angleRaw);
            return Vec3Q16{xy.x, xy.y, pos.z};
        }

        int32_t octBoxSet(Vec3Q16 pos, int32_t gTimeRaw, uint32_t pulseRaw) {
            int32_t wave = mulQ16Raw(sinRadiansRaw(mulQ16Raw(gTimeRaw, Q16_0_40)), static_cast<int32_t>(pulseRaw));
            int32_t shift = mulQ16Raw(wave, Q16_2_50);
            int32_t scale = Q16_2_00 - mulQ16Raw(absRaw(wave), Q16_1_50);
            if (scale < Q16_0_25) scale = Q16_0_25;

            Vec3Q16 p1 = rotateXy(Vec3Q16{pos.x, fl::s16x16::from_raw(raw(pos.y) + shift), pos.z}, Q16_0_80);
            Vec3Q16 p2 = rotateXy(Vec3Q16{pos.x, fl::s16x16::from_raw(raw(pos.y) - shift), pos.z}, Q16_0_80);
            Vec3Q16 p3 = rotateXy(Vec3Q16{fl::s16x16::from_raw(raw(pos.x) + shift), pos.y, pos.z}, Q16_0_80);
            Vec3Q16 p4 = rotateXy(Vec3Q16{fl::s16x16::from_raw(raw(pos.x) - shift), pos.y, pos.z}, Q16_0_80);
            int32_t result = octBox(p1, scale);
            int32_t candidate = octBox(p2, scale);
            if (candidate > result) result = candidate;
            candidate = octBox(p3, scale);
            if (candidate > result) result = candidate;
            candidate = octBox(p4, scale);
            if (candidate > result) result = candidate;
            candidate = mulQ16Raw(octBox(rotateXy(pos, Q16_0_80), Q16_0_50), Q16_6_00);
            if (candidate > result) result = candidate;
            candidate = mulQ16Raw(octBox(pos, Q16_0_50), Q16_6_00);
            if (candidate > result) result = candidate;
            return result;
        }

        Vec3Q16 normalize3(Vec3Q16 v) {
            int32_t len = raw(length3Q16(v));
            if (len < 1) len = 1;
            return Vec3Q16{
                fl::s16x16::from_raw(divSignedRaw(raw(v.x), len, 8 * SF16_ONE)),
                fl::s16x16::from_raw(divSignedRaw(raw(v.y), len, 8 * SF16_ONE)),
                fl::s16x16::from_raw(divSignedRaw(raw(v.z), len, 8 * SF16_ONE))
            };
        }

        Vec3Q16 cross3(Vec3Q16 a, Vec3Q16 b) {
            return Vec3Q16{
                fl::s16x16::from_raw(clampRaw(
                    static_cast<int64_t>(mulQ16Raw(raw(a.y), raw(b.z))) -
                    static_cast<int64_t>(mulQ16Raw(raw(a.z), raw(b.y))),
                    INT32_MIN,
                    INT32_MAX
                )),
                fl::s16x16::from_raw(clampRaw(
                    static_cast<int64_t>(mulQ16Raw(raw(a.z), raw(b.x))) -
                    static_cast<int64_t>(mulQ16Raw(raw(a.x), raw(b.z))),
                    INT32_MIN,
                    INT32_MAX
                )),
                fl::s16x16::from_raw(clampRaw(
                    static_cast<int64_t>(mulQ16Raw(raw(a.x), raw(b.y))) -
                    static_cast<int64_t>(mulQ16Raw(raw(a.y), raw(b.x))),
                    INT32_MIN,
                    INT32_MAX
                ))
            };
        }

        uint32_t sqrtUnitRaw(uint32_t valueRaw) {
            uint64_t root = sqrtU64Raw(static_cast<uint64_t>(valueRaw) << 16);
            return root > F16_MAX ? F16_MAX : static_cast<uint32_t>(root);
        }

        uint32_t acesChannelRaw(uint32_t valueRaw) {
            uint32_t v = mulUnsignedRaw(valueRaw, 39322u); // 0.6
            uint32_t numerator = mulUnsignedRaw(v, mulUnsignedRaw(Q16_2_51, v) + Q16_0_03);
            uint32_t denominator = mulUnsignedRaw(v, mulUnsignedRaw(Q16_2_43, v) + 38666u) + 9175u;
            return divUnsignedRaw(numerator, denominator, SF16_ONE);
        }

        uint32_t trigFieldChannel(int32_t phaseRaw, int32_t brightnessRaw) {
            int32_t value = Q16_0_50 + mulQ16Raw(sinRadiansRaw(phaseRaw), Q16_0_80);
            if (value <= 0) return 0u;
            return mulUnsignedRaw(static_cast<uint32_t>(value), static_cast<uint32_t>(brightnessRaw));
        }

        uint32_t starFieldHash32(uint32_t value) {
            value ^= value >> 16;
            value *= 0x7feb352du;
            value ^= value >> 15;
            value *= 0x846ca68bu;
            value ^= value >> 16;
            return value;
        }

        uint32_t starFieldHashCell(int32_t x, int32_t y, uint8_t layer, uint8_t salt) {
            uint32_t h = static_cast<uint32_t>(x) * 0x9e3779b9u;
            h ^= static_cast<uint32_t>(y) * 0x85ebca6bu;
            h ^= static_cast<uint32_t>(layer) * 0xc2b2ae35u;
            h ^= static_cast<uint32_t>(salt) * 0x27d4eb2fu;
            return starFieldHash32(h);
        }

        void starFieldColour(uint32_t hash, uint32_t &r, uint32_t &g, uint32_t &b) {
            uint32_t h1 = starFieldHash32(hash ^ 0x5bd1e995u);
            uint32_t h2 = starFieldHash32(hash ^ 0x68bc21ebu);
            uint32_t h3 = starFieldHash32(hash ^ 0x02e5be93u);
            r = Q16_0_25 + mulUnsignedRaw(h1 & 0xFFFFu, Q16_0_75);
            g = Q16_0_25 + mulUnsignedRaw(h2 & 0xFFFFu, Q16_0_75);
            b = Q16_0_25 + mulUnsignedRaw(h3 & 0xFFFFu, Q16_0_75);

            switch ((hash >> 29) % 3u) {
                case 0: r = SF16_ONE; break;
                case 1: g = SF16_ONE; break;
                default: b = SF16_ONE; break;
            }
        }

        int32_t pminRaw(int32_t aRaw, int32_t bRaw, int32_t kRaw) {
            uint32_t h = linstepRaw(-kRaw, kRaw, bRaw - aRaw);
            int32_t mixed = mixSignedRaw(bRaw, aRaw, h);
            int32_t soft = mulQ16Raw(kRaw, mulQ16Raw(static_cast<int32_t>(h), SF16_ONE - static_cast<int32_t>(h)));
            return mixed - soft;
        }

        int32_t pabsRaw(int32_t aRaw, int32_t kRaw) {
            return -pminRaw(aRaw, -aRaw, kRaw);
        }

        int32_t star5Raw(Vec2Q16 point, int32_t radiusRaw, int32_t radiusFactorRaw, int32_t smoothRaw) {
            Vec2Q16 p{
                fl::s16x16::from_raw(-raw(point.x)),
                fl::s16x16::from_raw(-raw(point.y))
            };
            constexpr int32_t k1x = 53020;  // 0.809016994375
            constexpr int32_t k1y = -38521; // -0.587785252292
            Vec2Q16 k1{fl::s16x16::from_raw(k1x), fl::s16x16::from_raw(k1y)};
            Vec2Q16 k2{fl::s16x16::from_raw(-k1x), fl::s16x16::from_raw(k1y)};

            p.x = fl::s16x16::from_raw(absRaw(raw(p.x)));
            int32_t h = dot2Raw(k1, p);
            if (h > 0) {
                p.x = fl::s16x16::from_raw(raw(p.x) - mulQ16Raw(2 * h, raw(k1.x)));
                p.y = fl::s16x16::from_raw(raw(p.y) - mulQ16Raw(2 * h, raw(k1.y)));
            }
            h = dot2Raw(k2, p);
            if (h > 0) {
                p.x = fl::s16x16::from_raw(raw(p.x) - mulQ16Raw(2 * h, raw(k2.x)));
                p.y = fl::s16x16::from_raw(raw(p.y) - mulQ16Raw(2 * h, raw(k2.y)));
            }

            p.x = fl::s16x16::from_raw(pabsRaw(raw(p.x), smoothRaw));
            p.y = fl::s16x16::from_raw(raw(p.y) - radiusRaw);

            Vec2Q16 ba{
                fl::s16x16::from_raw(mulQ16Raw(radiusFactorRaw, -raw(k1.y))),
                fl::s16x16::from_raw(mulQ16Raw(radiusFactorRaw, raw(k1.x)) - SF16_ONE)
            };
            int32_t baLen2 = dot2Raw(ba, ba);
            int32_t projection = baLen2 == 0 ? 0 : divSignedRaw(dot2Raw(p, ba), baLen2, 8 * SF16_ONE);
            if (projection < 0) projection = 0;
            if (projection > radiusRaw) projection = radiusRaw;

            Vec2Q16 d{
                fl::s16x16::from_raw(raw(p.x) - mulQ16Raw(raw(ba.x), projection)),
                fl::s16x16::from_raw(raw(p.y) - mulQ16Raw(raw(ba.y), projection))
            };
            int32_t len = raw(lengthQ16(d));
            int64_t side = static_cast<int64_t>(raw(p.y)) * raw(ba.x) - static_cast<int64_t>(raw(p.x)) * raw(ba.y);
            return side < 0 ? -len : len;
        }

        Vec3Q16 starryOffset(int32_t zRaw, uint32_t pathRaw) {
            int32_t x = mulQ16Raw(sinRadiansRaw(mulQ16Raw(20316, zRaw)), static_cast<int32_t>(pathRaw));
            int32_t y = mulQ16Raw(sinRadiansRaw(mulQ16Raw(26870, zRaw)), mulQ16Raw(46341, static_cast<int32_t>(pathRaw)));
            return Vec3Q16{
                fl::s16x16::from_raw(x),
                fl::s16x16::from_raw(y),
                fl::s16x16::from_raw(zRaw)
            };
        }

        Vec3Q16 starryDOffset(int32_t zRaw, uint32_t pathRaw) {
            int32_t x = mulQ16Raw(mulQ16Raw(20316, cosRadiansRaw(mulQ16Raw(20316, zRaw))), static_cast<int32_t>(pathRaw));
            int32_t y = mulQ16Raw(mulQ16Raw(26870, cosRadiansRaw(mulQ16Raw(26870, zRaw))), mulQ16Raw(46341, static_cast<int32_t>(pathRaw)));
            return Vec3Q16{
                fl::s16x16::from_raw(x),
                fl::s16x16::from_raw(y),
                fl::s16x16::from_raw(SF16_ONE)
            };
        }

        Vec3Q16 starryDDOffset(int32_t zRaw, uint32_t pathRaw) {
            int32_t x = -mulQ16Raw(mulQ16Raw(6298, sinRadiansRaw(mulQ16Raw(20316, zRaw))), static_cast<int32_t>(pathRaw));
            int32_t y = -mulQ16Raw(mulQ16Raw(11017, sinRadiansRaw(mulQ16Raw(26870, zRaw))), mulQ16Raw(46341, static_cast<int32_t>(pathRaw)));
            return Vec3Q16{
                fl::s16x16::from_raw(x),
                fl::s16x16::from_raw(y),
                fl::s16x16::from_raw(0)
            };
        }
    }

    struct RocaillePattern::State {
        Sf16Signal scaleSignal;
        Sf16Signal lengthSignal;
        Sf16Signal detailSignal;
        Sf16Signal turbulenceSignal;
        Sf16Signal frequencySignal;
        Sf16Signal speedSignal;
        Sf16Signal layersSignal;
        Sf16Signal hueSignal;
        Sf16Signal glowSignal;

        uint32_t timeRaw{0};
        int32_t spatialScaleRaw{19661};
        uint16_t lengthExponentPermille{1000};
        uint8_t detail{9};
        uint32_t turbulenceRaw{SF16_ONE};
        uint32_t frequencyRaw{SF16_ONE};
        uint8_t layers{9};
        int32_t hueRaw{0};
        uint32_t glowRaw{SF16_ONE};

        State(
            Sf16Signal scale,
            Sf16Signal length,
            Sf16Signal detail,
            Sf16Signal turbulence,
            Sf16Signal frequency,
            Sf16Signal speed,
            Sf16Signal layers,
            Sf16Signal hue,
            Sf16Signal glow
        ) :
            scaleSignal(std::move(scale)),
            lengthSignal(std::move(length)),
            detailSignal(std::move(detail)),
            turbulenceSignal(std::move(turbulence)),
            frequencySignal(std::move(frequency)),
            speedSignal(std::move(speed)),
            layersSignal(std::move(layers)),
            hueSignal(std::move(hue)),
            glowSignal(std::move(glow)) {}
    };

    struct RocaillePattern::Functor {
        const State *state;
        int32_t aspectRaw;

        RgbSample sample(UV uv) const {
            if (!state) return RgbSample();
            Vec2Q16 p = shaderUvHeight(uv, aspectRaw);
            p.x = fl::s16x16::from_raw(divSignedRaw(raw(p.x), state->spatialScaleRaw, 64 * SF16_ONE));
            p.y = fl::s16x16::from_raw(divSignedRaw(raw(p.y), state->spatialScaleRaw, 64 * SF16_ONE));

            uint32_t accR = 0;
            uint32_t accG = 0;
            uint32_t accB = 0;

            for (uint8_t layerIndex = 1; layerIndex <= state->layers; ++layerIndex) {
                Vec2Q16 turbulent = p;

                for (uint8_t iterationIndex = 1; iterationIndex <= state->detail; ++iterationIndex) {
                    int32_t iterationFrequency = mulQ16Raw(
                        static_cast<int32_t>(state->frequencyRaw),
                        static_cast<int32_t>(iterationIndex) << 16
                    );
                    int32_t common = static_cast<int32_t>(layerIndex) * SF16_ONE + static_cast<int32_t>(state->timeRaw);
                    int32_t sx = sinRadiansRaw(mulQ16Raw(raw(turbulent.y), iterationFrequency) + common);
                    int32_t sy = sinRadiansRaw(mulQ16Raw(raw(turbulent.x), iterationFrequency) + common);
                    int32_t stepX = mulQ16Raw(sx, static_cast<int32_t>(state->turbulenceRaw)) / iterationIndex;
                    int32_t stepY = mulQ16Raw(sy, static_cast<int32_t>(state->turbulenceRaw)) / iterationIndex;
                    turbulent.x = fl::s16x16::from_raw(clampRaw(static_cast<int64_t>(raw(turbulent.x)) + stepX, -64 * SF16_ONE, 64 * SF16_ONE));
                    turbulent.y = fl::s16x16::from_raw(clampRaw(static_cast<int64_t>(raw(turbulent.y)) + stepY, -64 * SF16_ONE, 64 * SF16_ONE));
                }

                uint32_t lenRaw = static_cast<uint32_t>(absRaw(raw(lengthQ16(turbulent))));
                if (lenRaw < 7u) lenRaw = 7u;
                uint32_t attenuationRaw = raw(powQ16(fl::u16x16::from_raw(lenRaw), state->lengthExponentPermille));
                if (attenuationRaw < 16u) attenuationRaw = 16u;

                int32_t baseAngle = static_cast<int32_t>(layerIndex) * SF16_ONE + state->hueRaw;
                uint32_t layerR = static_cast<uint32_t>(unsignedCosRadiansRaw(baseAngle)) / 3u;
                uint32_t layerG = static_cast<uint32_t>(unsignedCosRadiansRaw(baseAngle + SF16_ONE)) / 3u;
                uint32_t layerB = static_cast<uint32_t>(unsignedCosRadiansRaw(baseAngle + 2 * SF16_ONE)) / 3u;

                shaderToySaturatingAdd(accR, divUnsignedRaw(layerR, attenuationRaw, 128u * SF16_ONE));
                shaderToySaturatingAdd(accG, divUnsignedRaw(layerG, attenuationRaw, 128u * SF16_ONE));
                shaderToySaturatingAdd(accB, divUnsignedRaw(layerB, attenuationRaw, 128u * SF16_ONE));
            }

            return rgbFromPremultiplied(
                toneMapRocaille(accR, state->glowRaw),
                toneMapRocaille(accG, state->glowRaw),
                toneMapRocaille(accB, state->glowRaw)
            );
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    RocaillePattern::RocaillePattern(
        Sf16Signal scale,
        Sf16Signal length,
        Sf16Signal detail,
        Sf16Signal turbulence,
        Sf16Signal frequency,
        Sf16Signal speed,
        Sf16Signal layers,
        Sf16Signal hue,
        Sf16Signal glow
    ) : state(std::make_shared<State>(
        std::move(scale),
        std::move(length),
        std::move(detail),
        std::move(turbulence),
        std::move(frequency),
        std::move(speed),
        std::move(layers),
        std::move(hue),
        std::move(glow)
    )) {}

    void RocaillePattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->spatialScaleRaw = lerpByUnitRaw(Q16_0_10, Q16_0_70, signalUnitRaw(state->scaleSignal, elapsedMs, 333));
        state->lengthExponentPermille = lerpPermilleByUnit(250, 2000, signalUnitRaw(state->lengthSignal, elapsedMs, 429));
        state->detail = lerpIntByUnit(3, 30, signalUnitRaw(state->detailSignal, elapsedMs, 222));
        state->turbulenceRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_2_00, signalUnitRaw(state->turbulenceSignal, elapsedMs, 500)));
        state->frequencyRaw = static_cast<uint32_t>(lerpByUnitRaw(Q16_0_25, Q16_2_50, signalUnitRaw(state->frequencySignal, elapsedMs, 333)));
        uint32_t speedRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_3_00, signalUnitRaw(state->speedSignal, elapsedMs, 333)));
        state->timeRaw = mulUnsignedRaw(secondsRaw(elapsedMs), speedRaw);
        state->layers = lerpIntByUnit(1, 12, signalUnitRaw(state->layersSignal, elapsedMs, 727));
        state->hueRaw = lerpByUnitRaw(-Q16_PI, Q16_PI, signalUnitRaw(state->hueSignal, elapsedMs, 500));
        state->glowRaw = static_cast<uint32_t>(lerpByUnitRaw(Q16_0_25, Q16_2_00, signalUnitRaw(state->glowSignal, elapsedMs, 429)));
    }

    UVMap RocaillePattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{state.get(), shaderToyDisplayAspectRaw(context)};
    }

    UVLayer RocaillePattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        Functor f{state.get(), shaderToyDisplayAspectRaw(context)};
        return UVLayer::fromRgb([f](UV uv) {
            return f.sample(uv);
        });
    }

    struct ProteanCloudsPattern::State {
        Sf16Signal speedSignal;
        Sf16Signal warpSignal;
        Sf16Signal frequencySignal;
        Sf16Signal brightnessSignal;
        uint32_t timeRaw{0};
        uint32_t warpRaw{SF16_ONE};
        uint32_t frequencyRaw{Q16_9_00};
        uint32_t brightnessRaw{Q16_0_01};

        State(Sf16Signal speed, Sf16Signal warp, Sf16Signal frequency, Sf16Signal brightness)
            : speedSignal(std::move(speed)),
              warpSignal(std::move(warp)),
              frequencySignal(std::move(frequency)),
              brightnessSignal(std::move(brightness)) {}
    };

    struct ProteanCloudsPattern::Functor {
        const State *state;
        int32_t aspectRaw;

        RgbSample sample(UV uv) const {
            if (!state) return RgbSample();
            Vec2Q16 p{
                fl::s16x16::from_raw(mulQ16Raw(raw(shaderToyAxis(uv.u)) / 2, aspectRaw)),
                fl::s16x16::from_raw(raw(shaderToyAxis(uv.v)) / 2)
            };
            uint32_t lRaw = static_cast<uint32_t>(absRaw(raw(lengthQ16(p))));
            if (lRaw < 64u) lRaw = 64u;

            uint32_t channels[3] = {0, 0, 0};
            int32_t zRaw = static_cast<int32_t>(state->timeRaw);

            for (uint8_t i = 0; i < 3; ++i) {
                zRaw += Q16_0_07;

                uint32_t sinPlusOne = static_cast<uint32_t>(sinRadiansRaw(zRaw) + SF16_ONE);
                uint32_t warp = mulUnsignedRaw(mulUnsignedRaw(sinPlusOne, state->warpRaw), static_cast<uint32_t>(absRaw(sinRadiansRaw(mulQ16Raw(static_cast<int32_t>(lRaw), static_cast<int32_t>(state->frequencyRaw)) - zRaw - zRaw))));

                int32_t unitX = divSignedRaw(raw(p.x), static_cast<int32_t>(lRaw), 8 * SF16_ONE);
                int32_t unitY = divSignedRaw(raw(p.y), static_cast<int32_t>(lRaw), 8 * SF16_ONE);
                int32_t uvx = raw(uv.u) + mulQ16Raw(unitX, static_cast<int32_t>(warp));
                int32_t uvy = raw(uv.v) + mulQ16Raw(unitY, static_cast<int32_t>(warp));

                Vec2Q16 tiled{
                    fl::s16x16::from_raw(raw(fractQ16(fl::s16x16::from_raw(uvx))) - ST_Q16_HALF),
                    fl::s16x16::from_raw(raw(fractQ16(fl::s16x16::from_raw(uvy))) - ST_Q16_HALF)
                };
                uint32_t tileLength = static_cast<uint32_t>(absRaw(raw(lengthQ16(tiled))));
                if (tileLength < 64u) tileLength = 64u;
                uint32_t channel = divUnsignedRaw(state->brightnessRaw, tileLength, 64u * SF16_ONE);
                channels[i] = divUnsignedRaw(channel, lRaw, 64u * SF16_ONE);
            }

            return rgbFromPremultiplied(channels[0], channels[1], channels[2]);
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    ProteanCloudsPattern::ProteanCloudsPattern(
        Sf16Signal speed,
        Sf16Signal warp,
        Sf16Signal frequency,
        Sf16Signal brightness
    ) : state(std::make_shared<State>(
        std::move(speed),
        std::move(warp),
        std::move(frequency),
        std::move(brightness)
    )) {}

    void ProteanCloudsPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->timeRaw = mulUnsignedRaw(secondsRaw(elapsedMs), signalUnitRaw(state->speedSignal, elapsedMs, 1000));
        state->warpRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_2_00, signalUnitRaw(state->warpSignal, elapsedMs, 500)));
        state->frequencyRaw = static_cast<uint32_t>(lerpByUnitRaw(Q16_3_00, Q16_15_00, signalUnitRaw(state->frequencySignal, elapsedMs, 500)));
        state->brightnessRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_0_02, signalUnitRaw(state->brightnessSignal, elapsedMs, 500)));
    }

    UVMap ProteanCloudsPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{state.get(), shaderToyDisplayAspectRaw(context)};
    }

    UVLayer ProteanCloudsPattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        Functor f{state.get(), shaderToyDisplayAspectRaw(context)};
        return UVLayer::fromRgb([f](UV uv) {
            return f.sample(uv);
        });
    }

    struct OctgramsPattern::State {
        Sf16Signal speedSignal;
        Sf16Signal travelSignal;
        Sf16Signal pulseSignal;
        Sf16Signal densitySignal;
        Sf16Signal glowSignal;
        uint32_t timeRaw{0};
        uint32_t travelRaw{Q16_4_00};
        uint32_t pulseRaw{SF16_ONE};
        uint32_t densityRaw{Q16_23_00};
        uint32_t glowRaw{Q16_0_02};

        State(Sf16Signal speed, Sf16Signal travel, Sf16Signal pulse, Sf16Signal density, Sf16Signal glow)
            : speedSignal(std::move(speed)),
              travelSignal(std::move(travel)),
              pulseSignal(std::move(pulse)),
              densitySignal(std::move(density)),
              glowSignal(std::move(glow)) {}
    };

    struct OctgramsPattern::Functor {
        const State *state;
        int32_t aspectRaw;

        RgbSample sample(UV uv) const {
            if (!state) return RgbSample();
            Vec2Q16 p = shaderUvMin(uv, aspectRaw);
            Vec3Q16 ray = normalize3(Vec3Q16{p.x, p.y, fl::s16x16::from_raw(Q16_1_50)});
            uint32_t timeRaw = state->timeRaw;

            Vec2Q16 xy = rotate2(Vec2Q16{ray.x, ray.y}, mulQ16Raw(sinRadiansRaw(mulQ16Raw(static_cast<int32_t>(timeRaw), Q16_0_03)), Q16_5_00));
            ray.x = xy.x;
            ray.y = xy.y;
            Vec2Q16 yz = rotate2(Vec2Q16{ray.y, ray.z}, mulQ16Raw(sinRadiansRaw(mulQ16Raw(static_cast<int32_t>(timeRaw), Q16_0_05)), Q16_0_20));
            ray.y = yz.x;
            ray.z = yz.y;

            Vec3Q16 ro{
                fl::s16x16::from_raw(0),
                fl::s16x16::from_raw(-Q16_0_20),
                fl::s16x16::from_raw(mulQ16Raw(static_cast<int32_t>(timeRaw), static_cast<int32_t>(state->travelRaw)))
            };

            uint32_t tRaw = Q16_0_10;
            uint32_t acRaw = 0;

            for (uint8_t i = 0; i < 99; ++i) {
                Vec3Q16 pos{
                    fl::s16x16::from_raw(raw(ro.x) + mulQ16Raw(raw(ray.x), static_cast<int32_t>(tRaw))),
                    fl::s16x16::from_raw(raw(ro.y) + mulQ16Raw(raw(ray.y), static_cast<int32_t>(tRaw))),
                    fl::s16x16::from_raw(raw(ro.z) + mulQ16Raw(raw(ray.z), static_cast<int32_t>(tRaw)))
                };
                pos.x = fl::s16x16::from_raw(floorModRaw(raw(pos.x) - Q16_2_00, Q16_4_00) - Q16_2_00);
                pos.y = fl::s16x16::from_raw(floorModRaw(raw(pos.y) - Q16_2_00, Q16_4_00) - Q16_2_00);
                pos.z = fl::s16x16::from_raw(floorModRaw(raw(pos.z) - Q16_2_00, Q16_4_00) - Q16_2_00);

                int32_t gTime = static_cast<int32_t>(timeRaw) - static_cast<int32_t>(i) * Q16_0_01;
                uint32_t dRaw = static_cast<uint32_t>(absRaw(octBoxSet(pos, gTime, state->pulseRaw)));
                if (dRaw < static_cast<uint32_t>(Q16_0_01)) dRaw = Q16_0_01;

                shaderToySaturatingAdd(acRaw, raw(expNegQ16(fl::u16x16::from_raw(mulUnsignedRaw(dRaw, state->densityRaw)))));
                tRaw += mulUnsignedRaw(dRaw, Q16_0_55);
            }

            uint32_t base = mulUnsignedRaw(acRaw, state->glowRaw);
            uint32_t greenPulse = mulUnsignedRaw(static_cast<uint32_t>(absRaw(sinRadiansRaw(static_cast<int32_t>(timeRaw)))), Q16_0_20);
            int32_t bluePulseSigned = mulQ16Raw(sinRadiansRaw(static_cast<int32_t>(timeRaw)), Q16_0_20);
            int32_t blue = Q16_0_50 + bluePulseSigned;
            if (blue < 0) blue = 0;

            return rgbFromPremultiplied(
                base,
                base + greenPulse,
                base + static_cast<uint32_t>(blue)
            );
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    OctgramsPattern::OctgramsPattern(
        Sf16Signal speed,
        Sf16Signal travel,
        Sf16Signal pulse,
        Sf16Signal density,
        Sf16Signal glow
    ) : state(std::make_shared<State>(
        std::move(speed),
        std::move(travel),
        std::move(pulse),
        std::move(density),
        std::move(glow)
    )) {}

    void OctgramsPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->timeRaw = mulUnsignedRaw(secondsRaw(elapsedMs), signalUnitRaw(state->speedSignal, elapsedMs, 1000));
        state->travelRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_8_00, signalUnitRaw(state->travelSignal, elapsedMs, 500)));
        state->pulseRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_2_00, signalUnitRaw(state->pulseSignal, elapsedMs, 500)));
        state->densityRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_46_00, signalUnitRaw(state->densitySignal, elapsedMs, 500)));
        state->glowRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_0_04, signalUnitRaw(state->glowSignal, elapsedMs, 500)));
    }

    UVMap OctgramsPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{state.get(), shaderToyDisplayAspectRaw(context)};
    }

    UVLayer OctgramsPattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        Functor f{state.get(), shaderToyDisplayAspectRaw(context)};
        return UVLayer::fromRgb([f](UV uv) {
            return f.sample(uv);
        });
    }

    struct RotatingSquaresPattern::State {
        Sf16Signal speedSignal;
        Sf16Signal thicknessSignal;
        Sf16Signal pulseSignal;
        Sf16Signal brightnessSignal;
        uint32_t timeRaw{0};
        int32_t thicknessRaw{Q16_0_05};
        int32_t pulseRaw{Q16_0_01};
        uint32_t brightnessRaw{SF16_ONE};

        State(Sf16Signal speed, Sf16Signal thickness, Sf16Signal pulse, Sf16Signal brightness)
            : speedSignal(std::move(speed)),
              thicknessSignal(std::move(thickness)),
              pulseSignal(std::move(pulse)),
              brightnessSignal(std::move(brightness)) {}
    };

    struct RotatingSquaresPattern::Functor {
        const State *state;
        int32_t aspectRaw;
        uint16_t width;

        RgbSample sample(UV uv) const {
            if (!state) return RgbSample();
            Vec2Q16 coord = shaderUvWidth(uv, aspectRaw);
            uint32_t channels[3] = {0, 0, 0};
            uint32_t pixelBlur = (static_cast<uint32_t>(2) << 16) / (width == 0 ? 128u : width);

            for (uint8_t j = 0; j < 3; ++j) {
                for (uint8_t i = 0; i < 50; ++i) {
                    uint32_t diRaw = (static_cast<uint32_t>(i) << 16) / 49u;
                    uint32_t phaseOffset = static_cast<uint32_t>(j) * 3277u;
                    uint16_t exponentPermille = static_cast<uint16_t>(2000u + (static_cast<uint32_t>(i) * 1000u) / 49u);
                    uint32_t delay = raw(powQ16(fl::u16x16::from_raw(Q16_2_00), exponentPermille));
                    int32_t tRaw = static_cast<int32_t>(state->timeRaw + phaseOffset) - static_cast<int32_t>(delay);

                    int32_t phaseAngle = mulQ16Raw(raw(fractQ16(fl::s16x16::from_raw(tRaw / 2))), Q16_PI);
                    uint32_t fold = static_cast<uint32_t>(SF16_ONE) - unsignedCosRadiansRaw(phaseAngle);
                    uint32_t turnAmount = mulUnsignedRaw(raw(powQ16(fl::u16x16::from_raw(fold), 6000)), Q16_HALF_PI);
                    Vec2Q16 tc = rotate2(coord, static_cast<int32_t>(turnAmount));

                    uint32_t dRaw = static_cast<uint32_t>(absRaw(raw(tc.x)));
                    uint32_t ay = static_cast<uint32_t>(absRaw(raw(tc.y)));
                    if (ay > dRaw) dRaw = ay;

                    int32_t thRaw = state->thicknessRaw + mulQ16Raw(sinRadiansRaw(mulQ16Raw(tRaw, Q16_PI)), state->pulseRaw);
                    if (thRaw < Q16_0_005) thRaw = Q16_0_005;
                    if (thRaw > Q16_0_20) thRaw = Q16_0_20;

                    int32_t pBase = SF16_ONE - thRaw * 2;
                    if (pBase < 0) pBase = 0;
                    uint16_t pExponent = static_cast<uint16_t>(static_cast<uint32_t>(i) * 1000u);
                    uint32_t pRaw = raw(powQ16(fl::u16x16::from_raw(static_cast<uint32_t>(pBase)), pExponent));
                    uint32_t bRaw = pixelBlur + mulUnsignedRaw(Q16_0_01, diRaw);

                    int32_t outer = static_cast<int32_t>(dRaw) - mulQ16Raw(SF16_ONE - thRaw, static_cast<int32_t>(pRaw));
                    int32_t inner = static_cast<int32_t>(dRaw) - static_cast<int32_t>(pRaw);
                    uint32_t v1 = linstepRaw(-static_cast<int32_t>(bRaw), static_cast<int32_t>(bRaw), outer);
                    uint32_t v2 = linstepRaw(-static_cast<int32_t>(bRaw), static_cast<int32_t>(bRaw), inner);
                    if (v1 <= v2) continue;
                    uint32_t vRaw = v1 - v2;
                    uint32_t value = mulUnsignedRaw(vRaw, dRaw > static_cast<uint32_t>(4 * SF16_ONE) ? static_cast<uint32_t>(8 * SF16_ONE) : dRaw * 2u);
                    if (value > channels[j]) channels[j] = value;
                }
            }

            return rgbFromPremultiplied(
                mulUnsignedRaw(channels[0], state->brightnessRaw),
                mulUnsignedRaw(channels[1], state->brightnessRaw),
                mulUnsignedRaw(channels[2], state->brightnessRaw)
            );
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    RotatingSquaresPattern::RotatingSquaresPattern(
        Sf16Signal speed,
        Sf16Signal thickness,
        Sf16Signal pulse,
        Sf16Signal brightness
    ) : state(std::make_shared<State>(
        std::move(speed),
        std::move(thickness),
        std::move(pulse),
        std::move(brightness)
    )) {}

    void RotatingSquaresPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->timeRaw = mulUnsignedRaw(secondsRaw(elapsedMs), signalUnitRaw(state->speedSignal, elapsedMs, 1000));
        state->thicknessRaw = lerpByUnitRaw(Q16_0_02, Q16_0_10, signalUnitRaw(state->thicknessSignal, elapsedMs, 375));
        state->pulseRaw = lerpByUnitRaw(0, Q16_0_03, signalUnitRaw(state->pulseSignal, elapsedMs, 333));
        state->brightnessRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_2_00, signalUnitRaw(state->brightnessSignal, elapsedMs, 500)));
    }

    UVMap RotatingSquaresPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{state.get(), shaderToyDisplayAspectRaw(context), displayWidth(context)};
    }

    UVLayer RotatingSquaresPattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        Functor f{state.get(), shaderToyDisplayAspectRaw(context), displayWidth(context)};
        return UVLayer::fromRgb([f](UV uv) {
            return f.sample(uv);
        });
    }

    struct StarryPlanesPattern::State {
        Sf16Signal speedSignal;
        Sf16Signal planeSpacingSignal;
        Sf16Signal starSizeSignal;
        Sf16Signal pathSignal;
        Sf16Signal brightnessSignal;
        uint32_t timeRaw{0};
        int32_t planeSpacingRaw{Q16_0_50};
        int32_t starRadiusRaw{29491};
        uint32_t pathRaw{SF16_ONE};
        uint32_t brightnessRaw{SF16_ONE};

        State(
            Sf16Signal speed,
            Sf16Signal planeSpacing,
            Sf16Signal starSize,
            Sf16Signal path,
            Sf16Signal brightness
        ) :
            speedSignal(std::move(speed)),
            planeSpacingSignal(std::move(planeSpacing)),
            starSizeSignal(std::move(starSize)),
            pathSignal(std::move(path)),
            brightnessSignal(std::move(brightness)) {}
    };

    struct StarryPlanesPattern::Functor {
        const State *state;
        int32_t aspectRaw;
        uint16_t width;

        RgbSample sample(UV uv) const {
            if (!state) return RgbSample();

            Vec2Q16 p = shaderUvHeight(uv, aspectRaw);
            int32_t tmRaw = mulQ16Raw(static_cast<int32_t>(state->timeRaw), state->planeSpacingRaw);
            Vec3Q16 ro = starryOffset(tmRaw, state->pathRaw);
            Vec3Q16 dro = starryDOffset(tmRaw, state->pathRaw);
            Vec3Q16 ddro = starryDDOffset(tmRaw, state->pathRaw);

            Vec3Q16 ww = normalize3(dro);
            Vec3Q16 up{
                ddro.x,
                fl::s16x16::from_raw(raw(ddro.y) + SF16_ONE),
                ddro.z
            };
            Vec3Q16 uu = normalize3(cross3(up, ww));
            Vec3Q16 vv = cross3(ww, uu);

            Vec3Q16 ray = normalize3(Vec3Q16{
                fl::s16x16::from_raw(
                    mulQ16Raw(raw(p.x), raw(uu.x)) +
                    mulQ16Raw(raw(p.y), raw(vv.x)) +
                    mulQ16Raw(Q16_1_75, raw(ww.x))
                ),
                fl::s16x16::from_raw(
                    mulQ16Raw(raw(p.x), raw(uu.y)) +
                    mulQ16Raw(raw(p.y), raw(vv.y)) +
                    mulQ16Raw(Q16_1_75, raw(ww.y))
                ),
                fl::s16x16::from_raw(
                    mulQ16Raw(raw(p.x), raw(uu.z)) +
                    mulQ16Raw(raw(p.y), raw(vv.z)) +
                    mulQ16Raw(Q16_1_75, raw(ww.z))
                )
            });

            int32_t spacingRaw = state->planeSpacingRaw;
            if (spacingRaw < Q16_0_10) spacingRaw = Q16_0_10;
            int32_t nz = raw(ro.z) / spacingRaw;

            uint32_t accR = 0;
            uint32_t accG = 0;
            uint32_t accB = 0;
            uint32_t accA = 0;
            int32_t accumulatedDistance = 0;

            for (uint8_t i = 1; i <= 16; ++i) {
                if (accA > 62259u) break; // 0.95

                int32_t pzRaw = spacingRaw * (nz + static_cast<int32_t>(i));
                int32_t rayZ = raw(ray.z);
                if (rayZ < Q16_0_05) rayZ = Q16_0_05;
                int32_t lpdRaw = divSignedRaw(pzRaw - raw(ro.z), rayZ, 64 * SF16_ONE);
                if (lpdRaw <= 0) continue;

                Vec3Q16 pp{
                    fl::s16x16::from_raw(raw(ro.x) + mulQ16Raw(raw(ray.x), lpdRaw)),
                    fl::s16x16::from_raw(raw(ro.y) + mulQ16Raw(raw(ray.y), lpdRaw)),
                    fl::s16x16::from_raw(pzRaw)
                };
                accumulatedDistance = clampRaw(
                    static_cast<int64_t>(accumulatedDistance) + lpdRaw,
                    0,
                    64 * SF16_ONE
                );

                Vec3Q16 off = starryOffset(raw(pp.z), state->pathRaw);
                Vec2Q16 p2{
                    fl::s16x16::from_raw(raw(pp.x) - raw(off.x)),
                    fl::s16x16::from_raw(raw(pp.y) - raw(off.y))
                };

                Vec3Q16 doff = starryDDOffset(raw(pp.z), state->pathRaw);
                Vec3Q16 ddoff = starryDOffset(raw(pp.z), state->pathRaw);
                int32_t dd = mulQ16Raw(dot2Raw(Vec2Q16{doff.x, doff.z}, Vec2Q16{ddoff.x, ddoff.z}), Q16_PI * 5);
                p2 = rotate2(p2, dd);

                uint32_t aaRaw = static_cast<uint32_t>(accumulatedDistance / (width == 0 ? 128u : width));
                if (aaRaw < static_cast<uint32_t>(Q16_0_005)) aaRaw = Q16_0_005;
                if (aaRaw > static_cast<uint32_t>(Q16_0_10)) aaRaw = Q16_0_10;

                int32_t d0Raw = star5Raw(p2, state->starRadiusRaw, Q16_1_60, Q16_0_20) - Q16_0_02;
                int32_t d1Raw = d0Raw - Q16_0_01;
                uint32_t d2Raw = static_cast<uint32_t>(absRaw(raw(lengthQ16(p2))));
                if (d2Raw < static_cast<uint32_t>(Q16_0_01)) d2Raw = Q16_0_01;
                uint32_t d2Sq = squareRaw(d2Raw, 32u * SF16_ONE);
                if (d2Sq < static_cast<uint32_t>(Q16_0_005)) d2Sq = Q16_0_005;

                int32_t ringSin = sinRadiansRaw(mulQ16Raw(static_cast<int32_t>(d2Raw), Q16_100_PI));
                uint32_t ring = static_cast<uint32_t>(ringSin + SF16_ONE) >> 1;
                uint32_t invD2 = divUnsignedRaw(Q16_0_50, d2Sq, 32u * SF16_ONE);
                uint32_t sparkle = mixUnsignedRaw(invD2, SF16_ONE, ring);
                uint32_t denominator = d2Sq < static_cast<uint32_t>(2185u) ? 6554u : d2Sq * 3u;
                uint32_t intensity = divUnsignedRaw(sparkle, denominator, 32u * SF16_ONE);

                int32_t paletteArg = (static_cast<int32_t>(nz + i) * Q16_0_50) + mulQ16Raw(2 * SF16_ONE, static_cast<int32_t>(d2Raw));
                uint32_t r = mulUnsignedRaw(unsignedCosRadiansRaw(paletteArg - Q16_HALF_PI), intensity);
                uint32_t g = mulUnsignedRaw(unsignedCosRadiansRaw(paletteArg + SF16_ONE - Q16_HALF_PI), intensity);
                uint32_t b = mulUnsignedRaw(unsignedCosRadiansRaw(paletteArg + 2 * SF16_ONE - Q16_HALF_PI), intensity);

                uint32_t whiteMask = smoothstepSignedRaw(static_cast<int32_t>(aaRaw), -static_cast<int32_t>(aaRaw), d1Raw);
                r = mixUnsignedRaw(r, Q16_2_00, whiteMask);
                g = mixUnsignedRaw(g, Q16_2_00, whiteMask);
                b = mixUnsignedRaw(b, Q16_2_00, whiteMask);

                uint32_t alpha = smoothstepSignedRaw(static_cast<int32_t>(aaRaw), -static_cast<int32_t>(aaRaw), -d0Raw);
                int32_t dzRaw = raw(pp.z) - raw(ro.z);
                uint32_t fadeIn = smoothstepSignedRaw(spacingRaw * 16, spacingRaw * 8, dzRaw);
                uint32_t fadeOut = smoothstepSignedRaw(0, mulQ16Raw(spacingRaw, Q16_0_10), dzRaw);
                alpha = mulUnsignedRaw(mulUnsignedRaw(alpha, fadeIn), fadeOut);
                if (alpha == 0u) continue;

                uint32_t remaining = static_cast<uint32_t>(SF16_ONE) - accA;
                shaderToySaturatingAdd(accR, mulUnsignedRaw(mulUnsignedRaw(r, alpha), remaining));
                shaderToySaturatingAdd(accG, mulUnsignedRaw(mulUnsignedRaw(g, alpha), remaining));
                shaderToySaturatingAdd(accB, mulUnsignedRaw(mulUnsignedRaw(b, alpha), remaining));
                accA += mulUnsignedRaw(alpha, remaining);
                if (accA > static_cast<uint32_t>(SF16_ONE)) accA = SF16_ONE;
            }

            accR = mulUnsignedRaw(accR, state->brightnessRaw);
            accG = mulUnsignedRaw(accG, state->brightnessRaw);
            accB = mulUnsignedRaw(accB, state->brightnessRaw);

            return rgbFromPremultiplied(
                sqrtUnitRaw(acesChannelRaw(accR)),
                sqrtUnitRaw(acesChannelRaw(accG)),
                sqrtUnitRaw(acesChannelRaw(accB))
            );
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    StarryPlanesPattern::StarryPlanesPattern(
        Sf16Signal speed,
        Sf16Signal planeSpacing,
        Sf16Signal starSize,
        Sf16Signal path,
        Sf16Signal brightness
    ) : state(std::make_shared<State>(
        std::move(speed),
        std::move(planeSpacing),
        std::move(starSize),
        std::move(path),
        std::move(brightness)
    )) {}

    void StarryPlanesPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->timeRaw = mulUnsignedRaw(secondsRaw(elapsedMs), signalUnitRaw(state->speedSignal, elapsedMs, 1000));
        state->planeSpacingRaw = lerpByUnitRaw(Q16_0_25, Q16_0_70, signalUnitRaw(state->planeSpacingSignal, elapsedMs, 500));
        state->starRadiusRaw = lerpByUnitRaw(Q16_0_25, Q16_0_75, signalUnitRaw(state->starSizeSignal, elapsedMs, 400));
        state->pathRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_2_00, signalUnitRaw(state->pathSignal, elapsedMs, 500)));
        state->brightnessRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_2_00, signalUnitRaw(state->brightnessSignal, elapsedMs, 500)));
    }

    UVMap StarryPlanesPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{state.get(), shaderToyDisplayAspectRaw(context), displayWidth(context)};
    }

    UVLayer StarryPlanesPattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        Functor f{state.get(), shaderToyDisplayAspectRaw(context), displayWidth(context)};
        return UVLayer::fromRgb([f](UV uv) {
            return f.sample(uv);
        });
    }

    struct TrigFieldPattern::State {
        Sf16Signal zoomSignal;
        Sf16Signal yOffsetSignal;
        Sf16Signal waveScaleSignal;
        Sf16Signal speedSignal;
        Sf16Signal colorSpreadSignal;
        Sf16Signal brightnessSignal;
        uint32_t timeRaw{0};
        uint32_t zoomRaw{Q16_0_064};
        int32_t yOffsetRaw{Q16_OFFSET_1_8M_MOD_TAU};
        int32_t waveScaleRaw{Q16_0_035};
        int32_t colorSpreadRaw{Q16_1_30};
        int32_t brightnessRaw{SF16_ONE};

        State(
            Sf16Signal zoom,
            Sf16Signal yOffset,
            Sf16Signal waveScale,
            Sf16Signal speed,
            Sf16Signal colorSpread,
            Sf16Signal brightness
        ) :
            zoomSignal(std::move(zoom)),
            yOffsetSignal(std::move(yOffset)),
            waveScaleSignal(std::move(waveScale)),
            speedSignal(std::move(speed)),
            colorSpreadSignal(std::move(colorSpread)),
            brightnessSignal(std::move(brightness)) {}
    };

    struct TrigFieldPattern::Functor {
        const State *state;

        RgbSample sample(UV uv) const {
            if (!state) return RgbSample();

            int32_t uvX = wrapRadiansRaw(divUnsignedRaw(
                static_cast<uint32_t>(raw(uv.u)),
                state->zoomRaw,
                64u * SF16_ONE
            ));
            int32_t uvY = wrapRadiansRaw(static_cast<int64_t>(divUnsignedRaw(
                static_cast<uint32_t>(raw(uv.v)),
                state->zoomRaw,
                64u * SF16_ONE
            )) + state->yOffsetRaw);

            int32_t piRaw = Q16_PI;
            int32_t product = mulQ16Raw(piRaw, mulQ16Raw(uvY, piRaw));
            piRaw = wrapRadiansRaw(mulQ16Raw(product, state->waveScaleRaw));
            piRaw = wrapRadiansRaw(static_cast<int64_t>(piRaw) + uvX - uvY + piRaw);
            uvY = wrapRadiansRaw(static_cast<int64_t>(uvY) + sinRadiansRaw(divSignedRaw(piRaw, Q16_12_00, 64 * SF16_ONE) + uvY));
            uvY = wrapRadiansRaw(static_cast<int64_t>(uvY) + cosRadiansRaw(mulQ16Raw(Q16_1_525, piRaw)));

            const int32_t colorBase = wrapRadiansRaw(static_cast<int64_t>(state->timeRaw) + uvY);
            const int32_t rgSpread = state->colorSpreadRaw + (Q16_1_34 - Q16_1_30);

            uint32_t r = trigFieldChannel(
                wrapRadiansRaw(static_cast<int64_t>(colorBase) + mulQ16Raw(absRaw(piRaw + uvY), state->colorSpreadRaw)),
                state->brightnessRaw
            );
            uint32_t g = trigFieldChannel(
                wrapRadiansRaw(static_cast<int64_t>(colorBase) + mulQ16Raw(absRaw(piRaw + uvY + Q16_0_20), rgSpread)),
                state->brightnessRaw
            );
            uint32_t b = trigFieldChannel(
                wrapRadiansRaw(static_cast<int64_t>(colorBase) + mulQ16Raw(absRaw(piRaw + uvY + Q16_0_10), rgSpread)),
                state->brightnessRaw
            );

            return rgbFromPremultiplied(r, g, b);
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    TrigFieldPattern::TrigFieldPattern(
        Sf16Signal zoom,
        Sf16Signal yOffset,
        Sf16Signal waveScale,
        Sf16Signal speed,
        Sf16Signal colorSpread,
        Sf16Signal brightness
    ) : state(std::make_shared<State>(
        std::move(zoom),
        std::move(yOffset),
        std::move(waveScale),
        std::move(speed),
        std::move(colorSpread),
        std::move(brightness)
    )) {}

    void TrigFieldPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->zoomRaw = static_cast<uint32_t>(lerpByUnitRaw(Q16_0_042, Q16_0_10, signalUnitRaw(state->zoomSignal, elapsedMs, 379)));
        state->yOffsetRaw = wrapRadiansRaw(lerpByUnitRaw(
            Q16_OFFSET_1_8M_MOD_TAU,
            Q16_OFFSET_2_35M_MOD_TAU,
            signalUnitRaw(state->yOffsetSignal, elapsedMs, 0)
        ));
        state->waveScaleRaw = lerpByUnitRaw(Q16_0_01, Q16_0_07, signalUnitRaw(state->waveScaleSignal, elapsedMs, 364));
        uint32_t speedRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_4_00, signalUnitRaw(state->speedSignal, elapsedMs, 500)));
        state->timeRaw = mulUnsignedRaw(secondsRaw(elapsedMs), speedRaw);
        state->colorSpreadRaw = lerpByUnitRaw(Q16_0_80, Q16_1_80, signalUnitRaw(state->colorSpreadSignal, elapsedMs, 500));
        state->brightnessRaw = lerpByUnitRaw(0, Q16_2_00, signalUnitRaw(state->brightnessSignal, elapsedMs, 500));
    }

    UVMap TrigFieldPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return Functor{state.get()};
    }

    UVLayer TrigFieldPattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        Functor f{state.get()};
        return UVLayer::fromRgb([f](UV uv) {
            return f.sample(uv);
        });
    }

    struct StarFieldTravelPattern::State {
        Sf16Signal speedSignal;
        Sf16Signal densitySignal;
        Sf16Signal trailSignal;
        Sf16Signal starSizeSignal;
        Sf16Signal brightnessSignal;
        uint32_t timeRaw{0};
        uint32_t densityRaw{Q16_0_20};
        uint32_t trailLengthRaw{Q16_0_30};
        uint32_t trailAlphaRaw{Q16_0_50};
        int32_t starSizeRaw{Q16_0_07};
        uint32_t brightnessRaw{SF16_ONE};

        State(
            Sf16Signal speed,
            Sf16Signal density,
            Sf16Signal trail,
            Sf16Signal starSize,
            Sf16Signal brightness
        ) :
            speedSignal(std::move(speed)),
            densitySignal(std::move(density)),
            trailSignal(std::move(trail)),
            starSizeSignal(std::move(starSize)),
            brightnessSignal(std::move(brightness)) {}
    };

    struct StarFieldTravelPattern::Functor {
        const State *state;
        int32_t aspectRaw;

        void accumulatePlane(
            Vec2Q16 uv,
            int32_t zRaw,
            uint8_t layer,
            uint32_t tapWeightRaw,
            uint32_t &accR,
            uint32_t &accG,
            uint32_t &accB
        ) const {
            constexpr int32_t gridScaleRaw = Q16_8_00;
            int32_t scaledZ = mulQ16Raw(zRaw, gridScaleRaw);
            int32_t pxRaw = mulQ16Raw(raw(uv.x), scaledZ);
            int32_t pyRaw = mulQ16Raw(raw(uv.y), scaledZ);
            int32_t cellX = floorToIntRaw(pxRaw);
            int32_t cellY = floorToIntRaw(pyRaw);
            int32_t fracX = pxRaw - cellX * SF16_ONE;
            int32_t fracY = pyRaw - cellY * SF16_ONE;
            int32_t haloRadiusRaw = state->starSizeRaw * 6;
            uint32_t depthRaw = divUnsignedRaw(SF16_ONE, static_cast<uint32_t>(zRaw), Q16_3_00);
            uint32_t planeWeightRaw = mulUnsignedRaw(tapWeightRaw, depthRaw);
            planeWeightRaw = mulUnsignedRaw(planeWeightRaw, state->brightnessRaw);

            for (int8_t oy = -1; oy <= 1; ++oy) {
                for (int8_t ox = -1; ox <= 1; ++ox) {
                    int32_t sx = cellX + ox;
                    int32_t sy = cellY + oy;
                    uint32_t hash = starFieldHashCell(sx, sy, layer, 0);
                    if ((hash & 0xFFFFu) > state->densityRaw) continue;

                    uint32_t hx = starFieldHashCell(sx, sy, layer, 1);
                    uint32_t hy = starFieldHashCell(sx, sy, layer, 2);
                    int32_t starXRaw = static_cast<int32_t>(ox) * SF16_ONE + static_cast<int32_t>(hx & 0xFFFFu);
                    int32_t starYRaw = static_cast<int32_t>(oy) * SF16_ONE + static_cast<int32_t>(hy & 0xFFFFu);
                    int32_t dxRaw = fracX - starXRaw;
                    int32_t dyRaw = fracY - starYRaw;
                    if (absRaw(dxRaw) > haloRadiusRaw || absRaw(dyRaw) > haloRadiusRaw) continue;

                    Vec2Q16 delta{
                        fl::s16x16::from_raw(dxRaw),
                        fl::s16x16::from_raw(dyRaw)
                    };
                    int32_t distRaw = raw(lengthQ16(delta));
                    uint32_t core = smoothstepSignedRaw(state->starSizeRaw, 0, distRaw);
                    uint32_t halo = smoothstepSignedRaw(haloRadiusRaw, 0, distRaw);
                    uint32_t alphaRaw = core + mulUnsignedRaw(halo, Q16_0_20);
                    if (alphaRaw > static_cast<uint32_t>(SF16_ONE)) alphaRaw = SF16_ONE;
                    if (alphaRaw == 0u) continue;

                    uint32_t intensityRaw = mulUnsignedRaw(alphaRaw, planeWeightRaw);
                    if (intensityRaw == 0u) continue;

                    uint32_t r;
                    uint32_t g;
                    uint32_t b;
                    starFieldColour(hash, r, g, b);
                    shaderToySaturatingAdd(accR, mulUnsignedRaw(r, intensityRaw));
                    shaderToySaturatingAdd(accG, mulUnsignedRaw(g, intensityRaw));
                    shaderToySaturatingAdd(accB, mulUnsignedRaw(b, intensityRaw));
                }
            }
        }

        RgbSample sample(UV uv) const {
            if (!state) return RgbSample();

            Vec2Q16 p = shaderUvHeight(uv, aspectRaw);
            uint32_t accR = 0;
            uint32_t accG = 0;
            uint32_t accB = 0;
            constexpr uint8_t layerCount = 6;
            constexpr uint8_t trailTaps = 3;
            int32_t timePhaseRaw = static_cast<int32_t>(state->timeRaw & 0xFFFFu);

            for (uint8_t layer = 0; layer < layerCount; ++layer) {
                int32_t layerPhaseRaw = static_cast<int32_t>((static_cast<uint32_t>(layer) * SF16_ONE) / layerCount);
                int32_t cycleRaw = floorModRaw(layerPhaseRaw - timePhaseRaw, SF16_ONE);
                int32_t zRaw = lerpByUnitRaw(Q16_0_20, Q16_1_50, static_cast<uint32_t>(cycleRaw));
                uint32_t birthFadeRaw = starFieldTravelBirthFadeRaw(cycleRaw);

                for (uint8_t tap = 0; tap < trailTaps; ++tap) {
                    int32_t tapZRaw = zRaw;
                    uint32_t tapWeightRaw = SF16_ONE;
                    if (tap > 0) {
                        int32_t tapUnitRaw = static_cast<int32_t>((static_cast<uint32_t>(tap) * SF16_ONE) / (trailTaps - 1));
                        tapZRaw += mulQ16Raw(static_cast<int32_t>(state->trailLengthRaw), tapUnitRaw);
                        if (tapZRaw > Q16_1_50) continue;
                        uint32_t fadeRaw = (static_cast<uint32_t>(trailTaps - tap) * SF16_ONE) / trailTaps;
                        tapWeightRaw = mulUnsignedRaw(state->trailAlphaRaw, fadeRaw);
                    }
                    tapWeightRaw = mulUnsignedRaw(tapWeightRaw, birthFadeRaw);
                    if (tapWeightRaw == 0u) continue;
                    accumulatePlane(p, tapZRaw, layer, tapWeightRaw, accR, accG, accB);
                }
            }

            return rgbFromPremultiplied(accR, accG, accB);
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    StarFieldTravelPattern::StarFieldTravelPattern(
        Sf16Signal speed,
        Sf16Signal density,
        Sf16Signal trail,
        Sf16Signal starSize,
        Sf16Signal brightness
    ) : state(std::make_shared<State>(
        std::move(speed),
        std::move(density),
        std::move(trail),
        std::move(starSize),
        std::move(brightness)
    )) {}

    void StarFieldTravelPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->timeRaw = mulUnsignedRaw(secondsRaw(elapsedMs), static_cast<uint32_t>(
            lerpByUnitRaw(0, Q16_0_80, signalUnitRaw(state->speedSignal, elapsedMs, 250))
        ));
        state->densityRaw = static_cast<uint32_t>(
            lerpByUnitRaw(Q16_0_03, Q16_0_30, signalUnitRaw(state->densitySignal, elapsedMs, 500))
        );
        uint32_t trailUnitRaw = signalUnitRaw(state->trailSignal, elapsedMs, 500);
        state->trailLengthRaw = static_cast<uint32_t>(lerpByUnitRaw(0, Q16_0_60, trailUnitRaw));
        state->trailAlphaRaw = trailUnitRaw;
        state->starSizeRaw = lerpByUnitRaw(Q16_0_03, Q16_0_25, signalUnitRaw(state->starSizeSignal, elapsedMs, 400));
        state->brightnessRaw = static_cast<uint32_t>(
            lerpByUnitRaw(Q16_0_50, Q16_2_00, signalUnitRaw(state->brightnessSignal, elapsedMs, 600))
        );
    }

    UVMap StarFieldTravelPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{state.get(), shaderToyDisplayAspectRaw(context)};
    }

    UVLayer StarFieldTravelPattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        Functor f{state.get(), shaderToyDisplayAspectRaw(context)};
        return UVLayer::fromRgb([f](UV uv) {
            return f.sample(uv);
        });
    }
}
