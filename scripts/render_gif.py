#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Assemble a looping animation from psc_render output (geometry.csv +
# frames.bin + meta.json). Each LED is drawn as an additive glowing dot, giving
# the authentic sparse-LED look. The animation loops seamlessly because the
# field itself loops (see NoisePattern's two-path cross-dissolve) — no
# post-crossfade.
#
# Output formats (chosen by -o extension or --format):
#   gif  : 256-colour palette on an opaque black background (legacy default).
#   webp : 24-bit colour + 8-bit alpha; background fully transparent, LED dots
#          opaque. Far smaller and higher quality than gif; renders on GitHub.
#   apng : lossless RGBA (.png extension); same transparency, wider fallback.
#
# GIF/animation timing is quantised: GIF delays are stored in centiseconds
# (10 ms), so durations are exact only when meta.json's frame_ms is a whole
# multiple of 10 ms (e.g. 10000 ms / 25 fps => 40 ms). Frame count / fps is a
# psc_render knob (its --fps sets how many frames are sampled); this assembler
# never resamples, so there is deliberately no --fps here.
#
# Usage:
#   python3 scripts/render_gif.py build/gif/ -o media/polarshader.webp
#
# Deps: pillow, numpy  (python3 -m pip install pillow numpy)

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont


def load_geometry(path: Path) -> np.ndarray:
    xs, ys = [], []
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
    return np.stack([np.asarray(xs), np.asarray(ys)], axis=1)  # (led, 2) in [-1,1]


def make_sprite(radius: int, glow: float) -> np.ndarray:
    # Radial gradient in [0,1], 1 at centre falling to 0 at the edge. `glow`
    # sharpens (>1) or softens (<1) the falloff.
    size = radius * 2 + 1
    yy, xx = np.mgrid[0:size, 0:size]
    cx = cy = radius
    dist = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / max(radius, 1)
    sprite = np.clip(1.0 - dist, 0.0, 1.0) ** glow
    return sprite.astype(np.float32)


def make_glow_sprite(radius: int) -> np.ndarray:
    # Soft Gaussian glow: a tight bright core plus a wide, low halo. Because the
    # halo reaches well beyond the LED, neighbouring LEDs' sprites overlap and
    # their colours blend additively — mimicking the WASM renderer's bloom pass.
    size = radius * 2 + 1
    yy, xx = np.mgrid[0:size, 0:size]
    d = np.sqrt((xx - radius) ** 2 + (yy - radius) ** 2) / max(radius, 1)
    core = np.exp(-(d / 0.22) ** 2)
    halo = np.exp(-(d / 0.62) ** 2)
    sprite = core + 0.45 * halo
    # The Gaussian halo is still non-zero at the square array edge, which would
    # leave a faint hard-edged square around each LED (visible over light
    # backgrounds). Taper smoothly to exactly zero at the circular radius so the
    # footprint is a clean disc with no square boundary.
    w = np.clip((1.0 - d) / 0.3, 0.0, 1.0)  # 1 for d<0.7, ramps to 0 at d=1
    window = w * w * (3.0 - 2.0 * w)        # smoothstep
    sprite *= window
    return (sprite / sprite.max()).astype(np.float32)


def make_disc_alpha(radius: int, core_frac: float) -> np.ndarray:
    # Straight-alpha profile for one LED: a solid opaque disc in the centre
    # (alpha 1 out to `core_frac` of the radius) then a radial fade of the alpha
    # down to 0 at the edge. The LED's colour is kept constant across the whole
    # sprite; only the alpha fades. Composited source-over this reads as a glow
    # on a dark background and a soft colour halo on a light one, without ever
    # greying the background the way a semi-transparent HDR halo does.
    size = radius * 2 + 1
    yy, xx = np.mgrid[0:size, 0:size]
    d = np.sqrt((xx - radius) ** 2 + (yy - radius) ** 2) / max(radius, 1)
    t = np.clip((d - core_frac) / max(1.0 - core_frac, 1e-6), 0.0, 1.0)
    alpha = 1.0 - t * t * (3.0 - 2.0 * t)  # smoothstep 1 -> 0 over the fade band
    alpha[d >= 1.0] = 0.0
    return alpha.astype(np.float32)


def _hex_rgb(s: str) -> tuple[int, int, int]:
    s = s.lstrip("#")
    return (int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16))


def compose_banner(images: list[Image.Image], args: argparse.Namespace) -> list[Image.Image]:
    # Turn the square LED frames into a self-contained hero banner: a dark
    # rounded card with the animation inset on the left and the baked title on
    # the right. Baking the title means the card can be dropped straight into the
    # README (no markdown heading) and reads on any page background — the card's
    # own dark fill keeps the light title legible in GitHub light and dark modes.
    disc = images[0].size[1]                 # square frame edge == pill height
    W, H = args.banner_width, disc
    margin = args.banner_margin
    radius = H // 2                           # full pill: ends are semicircles
    bg = _hex_rgb(args.banner_bg)
    fg = _hex_rgb(args.title_color)

    # Size the title to ~42% of the banner height, then shrink to fit the right
    # region if it would overflow.
    text_x0 = disc                            # right region starts after the disc
    region_w = W - disc
    pad = int(H * 0.06)
    px = int(H * 0.42)
    title = args.banner_title

    def load_font(size: int) -> ImageFont.FreeTypeFont:
        f = ImageFont.truetype(args.title_font, size)
        try:
            f.set_variation_by_name("Bold")
        except Exception:
            pass
        return f

    font = load_font(px)
    while font.getbbox(title)[2] > region_w - 2 * pad and px > 8:
        px -= 2
        font = load_font(px)

    tb = font.getbbox(title)
    tw, th = tb[2] - tb[0], tb[3] - tb[1]
    tx = text_x0 + (region_w - tw) // 2 - tb[0]
    ty = (H - th) // 2 - tb[1]

    # Antialiased pill mask: draw at 4x then downsample so the rounded ends have
    # smooth, sub-pixel edges instead of a jagged 1-bit outline.
    ss = 4
    big = Image.new("L", (W * ss, H * ss), 0)
    ImageDraw.Draw(big).rounded_rectangle(
        [0, 0, W * ss - 1, H * ss - 1], radius=radius * ss, fill=255)
    mask = big.resize((W, H), Image.LANCZOS)

    cw, ch = W + 2 * margin, H + 2 * margin  # transparent margin around the pill
    out: list[Image.Image] = []
    for im in images:
        pill = Image.new("RGBA", (W, H), bg + (255,))
        pill.alpha_composite(im.convert("RGBA"), (0, 0))
        ImageDraw.Draw(pill).text((tx, ty), title, font=font, fill=fg + (255,))
        pill.putalpha(mask)  # AA rounded alpha (content is opaque, so alpha == mask)
        card = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
        card.alpha_composite(pill, (margin, margin))
        out.append(card)
    return out


def resolve_format(out: str, explicit: str | None) -> str:
    if explicit:
        return explicit
    ext = Path(out).suffix.lower()
    return {".webp": "webp", ".png": "apng", ".apng": "apng"}.get(ext, "gif")


def render(args: argparse.Namespace) -> None:
    fmt = resolve_format(args.out, args.format)
    in_dir = Path(args.in_dir)
    meta = json.loads((in_dir / "meta.json").read_text())
    frames_n = int(meta["frames"])
    led_count = int(meta["ledCount"])
    frame_ms = float(meta["frame_ms"])

    coords = load_geometry(in_dir / "geometry.csv")
    assert coords.shape[0] == led_count, "geometry/led count mismatch"

    raw = np.fromfile(in_dir / "frames.bin", dtype=np.uint8)
    assert raw.size == frames_n * led_count * 3, "frames.bin size mismatch"
    frames = raw.reshape(frames_n, led_count, 3).astype(np.float32)

    size = args.size
    # Map normalised [-1,1] coords into the canvas with a margin for the glow.
    margin = args.dot_radius + 2
    span = size - 2 * margin
    px = ((coords[:, 0] * 0.5 + 0.5) * span + margin).astype(np.int32)
    py = ((coords[:, 1] * 0.5 + 0.5) * span + margin).astype(np.int32)

    if args.style == "glow":
        sprite = make_glow_sprite(args.dot_radius)
    elif args.style == "disc":
        sprite = make_disc_alpha(args.dot_radius, args.core_frac)
    else:
        sprite = make_sprite(args.dot_radius, args.glow)
    r = args.dot_radius
    inv_gamma = 1.0 / args.gamma
    transparent = fmt in ("webp", "apng")

    # Optional black round background: a filled circle (opaque black outside the
    # glow) that fills the canvas, transparent beyond its rim. The glow always
    # reads because it sits on black; off LEDs simply show through as black.
    round_mask = None
    if args.round_bg:
        yy, xx = np.mgrid[0:size, 0:size]
        d = np.sqrt((xx - (size - 1) / 2.0) ** 2 + (yy - (size - 1) / 2.0) ** 2)
        rim = size / 2.0
        round_mask = np.clip((rim - d) / 1.5, 0.0, 1.0).astype(np.float32)  # AA edge

    images: list[Image.Image] = []
    for f in range(frames_n):
        colors = frames[f].copy()  # (led, 3), 0..255
        if args.full_value:
            peak = colors.max(axis=1, keepdims=True)
            lit = peak[:, 0] > args.value_floor
            colors[lit] *= 255.0 / peak[lit]
        colors *= args.brightness

        if args.style == "disc":
            # Composite constant-colour, alpha-faded discs with premultiplied
            # "over". Colour never changes across a sprite, so the result
            # composites correctly over any background: pre = colour*alpha.
            c01 = np.clip(colors / 255.0, 0.0, 1.0)  # (led, 3)
            # Per-LED alpha scales with brightness: an off LED still shows at
            # `off_alpha`, a fully lit one is opaque, lerping in between.
            bright = np.clip(frames[f].max(axis=1) / 255.0, 0.0, 1.0)  # (led,) raw
            afac = args.off_alpha + (1.0 - args.off_alpha) * bright     # (led,)
            pre = np.zeros((size, size, 3), dtype=np.float32)  # premultiplied RGB
            acc = np.zeros((size, size), dtype=np.float32)     # accumulated alpha
            for i in range(led_count):
                x0, y0 = px[i] - r, py[i] - r
                x1, y1 = x0 + sprite.shape[1], y0 + sprite.shape[0]
                sx0 = max(0, -x0); sy0 = max(0, -y0)
                cx0 = max(0, x0); cy0 = max(0, y0)
                cx1 = min(size, x1); cy1 = min(size, y1)
                if cx1 <= cx0 or cy1 <= cy0:
                    continue
                a = (sprite[sy0:sy0 + (cy1 - cy0), sx0:sx0 + (cx1 - cx0)]
                     * afac[i])[..., None]
                pre[cy0:cy1, cx0:cx1] = c01[i] * a + pre[cy0:cy1, cx0:cx1] * (1.0 - a)
                acc[cy0:cy1, cx0:cx1] = a[..., 0] + acc[cy0:cy1, cx0:cx1] * (1.0 - a[..., 0])
            alpha = np.clip(acc, 0.0, 1.0)
            if transparent:
                with np.errstate(divide="ignore", invalid="ignore"):
                    straight = np.where(alpha[..., None] > 0, pre / alpha[..., None], 0.0)
                rgb = (np.clip(straight, 0.0, 1.0) * 255.0).astype(np.uint8)
                a8 = (alpha * 255.0).astype(np.uint8)
                images.append(Image.fromarray(np.dstack([rgb, a8])))
            else:
                # Opaque output: the premultiplied buffer is already composited
                # over black.
                rgb = (np.clip(pre, 0.0, 1.0) * 255.0).astype(np.uint8)
                images.append(Image.fromarray(rgb))
            continue

        canvas = np.zeros((size, size, 3), dtype=np.float32)
        # `cover` accumulates the raw sprite footprint (colour-independent) so we
        # can build an alpha channel: transparent where no LED reaches, opaque at
        # the dot cores.
        cover = np.zeros((size, size), dtype=np.float32)
        for i in range(led_count):
            x0, y0 = px[i] - r, py[i] - r
            x1, y1 = x0 + sprite.shape[1], y0 + sprite.shape[0]
            # Clip sprite to canvas bounds.
            sx0 = max(0, -x0); sy0 = max(0, -y0)
            cx0 = max(0, x0); cy0 = max(0, y0)
            cx1 = min(size, x1); cy1 = min(size, y1)
            if cx1 <= cx0 or cy1 <= cy0:
                continue
            sub = sprite[sy0:sy0 + (cy1 - cy0), sx0:sx0 + (cx1 - cx0)]
            canvas[cy0:cy1, cx0:cx1] += sub[..., None] * colors[i]
            cover[cy0:cy1, cx0:cx1] += sub
        # Additive HDR -> tone-map by clamping, then gamma for glow bloom.
        out = np.clip(canvas, 0.0, 255.0) / 255.0
        out = np.power(out, inv_gamma)
        rgb = (out * 255.0).astype(np.uint8)
        if transparent:
            if round_mask is not None:
                # Black round background: the whole disc is opaque (black + glow);
                # only the area outside the rim is transparent.
                alpha = round_mask
            elif args.style == "glow":
                # Alpha follows the blended brightness (peak channel of the
                # tone-mapped colour): bright cores are opaque for any hue, the
                # glow halo tapers to transparent, background stays clear.
                alpha = np.clip(out.max(axis=2) * args.alpha_boost, 0.0, 1.0)
            else:
                # `alpha_boost` saturates the dot bodies to fully opaque (LEDs
                # are not see-through) while the outer glow tapers to transparent.
                alpha = np.clip(cover * args.alpha_boost, 0.0, 1.0)
            a = (alpha * 255.0).astype(np.uint8)
            images.append(Image.fromarray(np.dstack([rgb, a])))
        else:
            images.append(Image.fromarray(rgb))

    if args.banner_title:
        images = compose_banner(images, args)

    # Timing: GIF/WebP/APNG delays are stored in centiseconds; snap to that grid.
    delay_cs = max(1, round(frame_ms / 10.0))
    duration_ms = delay_cs * 10
    total_ms = duration_ms * frames_n

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    if fmt == "webp":
        images[0].save(
            out_path, save_all=True, append_images=images[1:], loop=0,
            duration=duration_ms, format="WEBP",
            lossless=args.lossless, quality=args.quality, method=4, exact=True,
        )
    elif fmt == "apng":
        images[0].save(
            out_path, save_all=True, append_images=images[1:], loop=0,
            duration=duration_ms, format="PNG", disposal=1,
        )
    else:
        quantized = [
            im.quantize(colors=args.colors, method=Image.Quantize.MEDIANCUT,
                        dither=Image.Dither.FLOYDSTEINBERG)
            for im in images
        ]
        quantized[0].save(
            out_path, save_all=True, append_images=quantized[1:], loop=0,
            duration=duration_ms, optimize=True, disposal=2,
        )
    size_kb = out_path.stat().st_size / 1024.0
    print(f"wrote {out_path} ({fmt}, {size_kb:.1f} KiB, {frames_n} frames, "
          f"{total_ms} ms quantized total, {duration_ms} ms/frame)")
    if abs(total_ms - meta["period_ms"]) > 0.5:
        print(f"  note: total {total_ms} ms differs from sampled period "
              f"{meta['period_ms']} ms (frame_ms={frame_ms} is not a whole 10 ms multiple)")


def main() -> None:
    p = argparse.ArgumentParser(
        description="Assemble a looping glowing-dots GIF from psc_render output.",
        epilog="Exact GIF timing requires meta.json frame_ms (= period_ms/N) to be "
               "a whole multiple of 10 ms (one centisecond); otherwise the GIF "
               "duration is rounded per frame. Frame count is a psc_render --fps knob.",
    )
    p.add_argument("in_dir", help="directory with geometry.csv, frames.bin, meta.json")
    p.add_argument("-o", "--out", default="media/polarshader.webp", help="output path")
    p.add_argument("--format", choices=["gif", "webp", "apng"], default=None,
                   help="output format (default: inferred from -o extension)")
    p.add_argument("--style", choices=["dots", "glow", "disc"], default="dots",
                   help="dots: hard opaque LEDs; glow: soft additive bloom; "
                        "disc: solid colour core + radial alpha fade (works on any bg)")
    p.add_argument("--size", type=int, default=640, help="canvas edge in pixels")
    p.add_argument("--dot-radius", type=int, default=14, help="glow sprite radius in pixels")
    p.add_argument("--core-frac", type=float, default=0.35,
                   help="disc style: fraction of the radius that is a solid opaque core")
    p.add_argument("--off-alpha", type=float, default=0.30,
                   help="disc style: alpha of an unlit LED; lit LEDs lerp up to 1.0 by brightness")
    p.add_argument("--round-bg", action="store_true",
                   help="fill a black round background disc; alpha = the circle (glow always on black)")
    p.add_argument("--glow", type=float, default=2.2, help="falloff exponent (>1 sharper)")
    p.add_argument("--gamma", type=float, default=1.8, help="output gamma (bloom)")
    p.add_argument("--brightness", type=float, default=1.0, help="linear gain before tone-map")
    p.add_argument("--full-value", action="store_true",
                   help="normalise every lit LED to full brightness (peak channel -> 255)")
    p.add_argument("--value-floor", type=float, default=8.0,
                   help="LEDs whose peak channel is <= this stay off (with --full-value)")
    p.add_argument("--alpha-boost", type=float, default=3.0,
                   help="alpha gain: >1 makes dot bodies fully opaque (webp/apng)")
    p.add_argument("--banner-title", default=None,
                   help="bake this title to the right of the animation as a hero banner")
    p.add_argument("--banner-width", type=int, default=720, help="banner total width in pixels")
    p.add_argument("--banner-bg", default="#000000", help="banner card background colour")
    p.add_argument("--title-color", default="#f5f5f7", help="banner title text colour")
    p.add_argument("--title-font", default="/System/Library/Fonts/SFNS.ttf",
                   help="TrueType font path for the banner title")
    p.add_argument("--banner-margin", type=int, default=8,
                   help="transparent margin in pixels around the pill banner")
    p.add_argument("--quality", type=int, default=80, help="WebP quality 0-100 (lossy)")
    p.add_argument("--lossless", action="store_true", help="WebP lossless mode")
    p.add_argument("--colors", type=int, default=128, help="GIF palette depth (<=256)")
    args = p.parse_args()
    render(args)


if __name__ == "__main__":
    main()
