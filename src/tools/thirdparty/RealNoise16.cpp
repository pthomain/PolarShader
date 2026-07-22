// Vendored 16-bit Perlin noise from FastLED 3.10.4 (src/noise.cpp.hpp).
// See RealNoise16.h for the full MIT license notice.
//
// The entire translation unit is gated behind POLAR_SHADER_REAL_NOISE so that
// every native env can compile it harmlessly (empty TU) while only the
// exporter env (which sets the flag) emits the real symbols. This mirrors
// FASTLED_NOISE_FIXED=1 (EASE16 = ease16InOutQuad, FADE_16 => LERP =
// lerp15by16) and FASTLED_SCALE8_FIXED=1 (the +1 rounding term in scale16).

#ifdef POLAR_SHADER_REAL_NOISE

#include "RealNoise16.h"

namespace {

// FastLED permutation table (256 entries + 1 wrap guard).
static const uint8_t p[] = {
    151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
    140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
    247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
     57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
     74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
     60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
     65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
    200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
     52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
    207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
    119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
    129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
    218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
     81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
    184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
    222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180,
    151};

inline uint8_t noiseP(uint32_t x) { return p[x]; }

// scale16 with FASTLED_SCALE8_FIXED == 1 (the +1 rounding term).
inline uint16_t scale16(uint16_t i, uint16_t scale) {
    return static_cast<uint16_t>((static_cast<uint32_t>(i) * (1u + static_cast<uint32_t>(scale))) / 65536u);
}

// avg15: signed 16-bit average without overflow.
inline int16_t avg15(int16_t i, int16_t j) {
    return static_cast<int16_t>((i >> 1) + (j >> 1) + (i & 0x1));
}

// ease16InOutQuad: 16-bit quadratic ease-in/ease-out (EASE16 in FASTLED_NOISE_FIXED=1).
inline uint16_t ease16InOutQuad(uint16_t i) {
    uint16_t j = i;
    if (j & 0x8000) {
        j = static_cast<uint16_t>(65535 - j);
    }
    uint16_t jj = scale16(j, j);
    uint16_t jj2 = static_cast<uint16_t>(jj << 1);
    if (i & 0x8000) {
        jj2 = static_cast<uint16_t>(65535 - jj2);
    }
    return jj2;
}

// lerp15by16: signed 16-bit lerp by a 16-bit fraction (LERP in FADE_16).
inline int16_t lerp15by16(int16_t a, int16_t b, uint16_t frac) {
    int16_t result;
    if (b > a) {
        uint16_t delta = static_cast<uint16_t>(b - a);
        uint16_t scaled = scale16(delta, frac);
        result = static_cast<int16_t>(a + scaled);
    } else {
        uint16_t delta = static_cast<uint16_t>(a - b);
        uint16_t scaled = scale16(delta, frac);
        result = static_cast<int16_t>(a - scaled);
    }
    return result;
}

inline int16_t grad16(uint8_t hash, int16_t x, int16_t y, int16_t z) {
    hash = hash & 15;
    int16_t u = hash < 8 ? x : y;
    int16_t v = hash < 4 ? y : (hash == 12 || hash == 14 ? x : z);
    if (hash & 1) { u = static_cast<int16_t>(-u); }
    if (hash & 2) { v = static_cast<int16_t>(-v); }
    return avg15(u, v);
}

inline int16_t grad16(uint8_t hash, int16_t x, int16_t y) {
    hash = hash & 7;
    int16_t u, v;
    if (hash < 4) { u = x; v = y; } else { u = y; v = x; }
    if (hash & 1) { u = static_cast<int16_t>(-u); }
    if (hash & 2) { v = static_cast<int16_t>(-v); }
    return avg15(u, v);
}

inline int16_t grad16(uint8_t hash, int16_t x) {
    hash = hash & 15;
    int16_t u, v;
    if (hash > 8) { u = x; v = x; }
    else if (hash < 4) { u = x; v = 1; }
    else { u = 1; v = x; }
    if (hash & 1) { u = static_cast<int16_t>(-u); }
    if (hash & 2) { v = static_cast<int16_t>(-v); }
    return avg15(u, v);
}

int16_t inoise16_raw(uint32_t x, uint32_t y, uint32_t z) {
    uint8_t X = (x >> 16) & 0xFF;
    uint8_t Y = (y >> 16) & 0xFF;
    uint8_t Z = (z >> 16) & 0xFF;

    uint8_t A  = static_cast<uint8_t>(noiseP(X) + Y);
    uint8_t AA = static_cast<uint8_t>(noiseP(A) + Z);
    uint8_t AB = static_cast<uint8_t>(noiseP(static_cast<uint8_t>(A + 1)) + Z);
    uint8_t B  = static_cast<uint8_t>(noiseP(static_cast<uint8_t>(X + 1)) + Y);
    uint8_t BA = static_cast<uint8_t>(noiseP(B) + Z);
    uint8_t BB = static_cast<uint8_t>(noiseP(static_cast<uint8_t>(B + 1)) + Z);

    uint16_t u = x & 0xFFFF;
    uint16_t v = y & 0xFFFF;
    uint16_t w = z & 0xFFFF;

    int16_t xx = (u >> 1) & 0x7FFF;
    int16_t yy = (v >> 1) & 0x7FFF;
    int16_t zz = (w >> 1) & 0x7FFF;
    uint16_t N = 0x8000;

    u = ease16InOutQuad(u);
    v = ease16InOutQuad(v);
    w = ease16InOutQuad(w);

    int16_t X1 = lerp15by16(grad16(noiseP(AA), xx, yy, zz),                     grad16(noiseP(BA), static_cast<int16_t>(xx - N), yy, zz), u);
    int16_t X2 = lerp15by16(grad16(noiseP(AB), xx, static_cast<int16_t>(yy - N), zz), grad16(noiseP(BB), static_cast<int16_t>(xx - N), static_cast<int16_t>(yy - N), zz), u);
    int16_t X3 = lerp15by16(grad16(noiseP(static_cast<uint8_t>(AA + 1)), xx, yy, static_cast<int16_t>(zz - N)), grad16(noiseP(static_cast<uint8_t>(BA + 1)), static_cast<int16_t>(xx - N), yy, static_cast<int16_t>(zz - N)), u);
    int16_t X4 = lerp15by16(grad16(noiseP(static_cast<uint8_t>(AB + 1)), xx, static_cast<int16_t>(yy - N), static_cast<int16_t>(zz - N)), grad16(noiseP(static_cast<uint8_t>(BB + 1)), static_cast<int16_t>(xx - N), static_cast<int16_t>(yy - N), static_cast<int16_t>(zz - N)), u);

    int16_t Y1 = lerp15by16(X1, X2, v);
    int16_t Y2 = lerp15by16(X3, X4, v);

    return lerp15by16(Y1, Y2, w);
}

int16_t inoise16_raw(uint32_t x, uint32_t y) {
    uint8_t X = x >> 16;
    uint8_t Y = y >> 16;

    uint8_t A  = static_cast<uint8_t>(noiseP(X) + Y);
    uint8_t AA = noiseP(A);
    uint8_t AB = noiseP(static_cast<uint8_t>(A + 1));
    uint8_t B  = static_cast<uint8_t>(noiseP(static_cast<uint8_t>(X + 1)) + Y);
    uint8_t BA = noiseP(B);
    uint8_t BB = noiseP(static_cast<uint8_t>(B + 1));

    uint16_t u = x & 0xFFFF;
    uint16_t v = y & 0xFFFF;

    int16_t xx = (u >> 1) & 0x7FFF;
    int16_t yy = (v >> 1) & 0x7FFF;
    uint16_t N = 0x8000;

    u = ease16InOutQuad(u);
    v = ease16InOutQuad(v);

    int16_t X1 = lerp15by16(grad16(noiseP(AA), xx, yy),                     grad16(noiseP(BA), static_cast<int16_t>(xx - N), yy), u);
    int16_t X2 = lerp15by16(grad16(noiseP(AB), xx, static_cast<int16_t>(yy - N)), grad16(noiseP(BB), static_cast<int16_t>(xx - N), static_cast<int16_t>(yy - N)), u);

    return lerp15by16(X1, X2, v);
}

int16_t inoise16_raw(uint32_t x) {
    uint8_t X = x >> 16;

    uint8_t A  = noiseP(X);
    uint8_t AA = noiseP(A);
    uint8_t B  = noiseP(static_cast<uint8_t>(X + 1));
    uint8_t BA = noiseP(B);

    uint16_t u = x & 0xFFFF;

    int16_t xx = (u >> 1) & 0x7FFF;
    uint16_t N = 0x8000;

    u = ease16InOutQuad(u);

    return lerp15by16(grad16(noiseP(AA), xx), grad16(noiseP(BA), static_cast<int16_t>(xx - N)), u);
}

} // namespace

uint16_t inoise16(uint32_t x, uint32_t y, uint32_t z) {
    int32_t ans = inoise16_raw(x, y, z);
    ans = ans + 19052L;
    uint32_t pan = static_cast<uint32_t>(ans);
    pan *= 440L;
    return static_cast<uint16_t>(pan >> 8);
}

uint16_t inoise16(uint32_t x, uint32_t y) {
    int32_t ans = inoise16_raw(x, y);
    ans = ans + 17308L;
    uint32_t pan = static_cast<uint32_t>(ans);
    pan *= 484L;
    return static_cast<uint16_t>(pan >> 8);
}

uint16_t inoise16(uint32_t x) {
    return static_cast<uint16_t>((static_cast<uint32_t>(static_cast<int32_t>(inoise16_raw(x)) + 17308L)) << 1);
}

#endif // POLAR_SHADER_REAL_NOISE
