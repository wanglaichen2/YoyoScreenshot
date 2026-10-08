# -*- coding: utf-8 -*-
"""Unify app icons from Store/Logos; make outer rounded border fully transparent."""
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
LOGOS = Path(__file__).resolve().parent / "Logos"
ASSETS = ROOT / "Pack" / "msix" / "Assets"
RESOURCES = ROOT / "resources"
SRC = LOGOS / "StoreLogo_1080x1080.png"


def make_transparent_border(img: Image.Image, radius_ratio: float = 0.22) -> Image.Image:
    """Keep content inside a rounded square; make outer border fully transparent."""
    img = img.convert("RGBA")
    w, h = img.size
    r = max(1, int(min(w, h) * radius_ratio))
    mask = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle((0, 0, w - 1, h - 1), radius=r, fill=255)

    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    out.paste(img, (0, 0))
    # force outside rounded rect to alpha 0
    alpha = out.split()[-1]
    alpha = Image.composite(alpha, Image.new("L", (w, h), 0), mask)
    out.putalpha(alpha)

    # also clear residual near-white fringe outside soft edge
    px = out.load()
    for y in range(h):
        for x in range(w):
            if mask.getpixel((x, y)) == 0:
                px[x, y] = (0, 0, 0, 0)
                continue
            r_, g_, b_, a_ = px[x, y]
            # soft fringe near corners that is almost white with low content
            if a_ < 250 and r_ > 245 and g_ > 245 and b_ > 245:
                # distance to nearest corner
                d = min(
                    math.hypot(x, y),
                    math.hypot(w - 1 - x, y),
                    math.hypot(x, h - 1 - y),
                    math.hypot(w - 1 - x, h - 1 - y),
                )
                if d < r * 0.85:
                    px[x, y] = (0, 0, 0, 0)
    return out


def resize_cover(img: Image.Image, size: tuple[int, int]) -> Image.Image:
    return img.resize(size, Image.Resampling.LANCZOS)


def save_ico(img: Image.Image, path: Path, sizes: list[int]) -> None:
    """Write multi-size ICO with PNG-compressed entries (Vista+)."""
    import io
    import struct

    frames = [resize_cover(img, (s, s)) for s in sizes]
    entries = []
    blobs = []
    for im in frames:
        bio = io.BytesIO()
        im.save(bio, format="PNG")
        data = bio.getvalue()
        w, h = im.size
        entries.append((0 if w >= 256 else w, 0 if h >= 256 else h, data))
        blobs.append(data)

    offset = 6 + 16 * len(entries)
    out = bytearray()
    out += struct.pack("<HHH", 0, 1, len(entries))
    for w, h, data in entries:
        out += struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    for data in blobs:
        out += data
    path.write_bytes(bytes(out))


def main() -> None:
    if not SRC.exists():
        raise SystemExit(f"missing source: {SRC}")

    master = make_transparent_border(Image.open(SRC))
    master_path = LOGOS / "StoreLogo_master_transparent.png"
    master.save(master_path, "PNG")
    print("wrote", master_path)

    # Store listing / package logos
    targets = {
        LOGOS / "StoreLogo_1080x1080.png": (1080, 1080),
        LOGOS / "StoreLogo_300x300.png": (300, 300),
        LOGOS / "StoreLogo_71x71.png": (71, 71),
        LOGOS / "StoreLogo_50x50.png": (50, 50),
        LOGOS / "Square44x44Logo.png": (44, 44),
        LOGOS / "Square150x150Logo.png": (150, 150),
        LOGOS / "Square310x310Logo.png": (310, 310),
        LOGOS / "Wide310x150Logo.png": (310, 150),
        LOGOS / "SplashScreen_620x300.png": (620, 300),
        ASSETS / "StoreLogo.png": (50, 50),
        ASSETS / "Square44x44Logo.png": (44, 44),
        ASSETS / "Square150x150Logo.png": (150, 150),
        ASSETS / "Square310x310Logo.png": (310, 310),
        ASSETS / "Wide310x150Logo.png": (310, 150),
        ASSETS / "SplashScreen.png": (620, 300),
    }

    def center_on_canvas(canvas_size: tuple[int, int], logo_side: int) -> Image.Image:
        canvas = Image.new("RGBA", canvas_size, (0, 0, 0, 0))
        logo = resize_cover(master, (logo_side, logo_side))
        x = (canvas_size[0] - logo_side) // 2
        y = (canvas_size[1] - logo_side) // 2
        canvas.paste(logo, (x, y), logo)
        return canvas

    for path, size in targets.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        if size[0] != size[1]:
            # wide / splash: square logo centered, outer area transparent
            side = min(size)
            im = center_on_canvas(size, side)
        else:
            im = resize_cover(master, size)
        im.save(path, "PNG")
        print("wrote", path.name, size)

    # 720x1080 portrait: place square logo centered on transparent canvas
    portrait = center_on_canvas((720, 1080), 560)
    portrait_path = LOGOS / "StoreLogo_720x1080.png"
    portrait.save(portrait_path, "PNG")
    print("wrote", portrait_path.name)

    # App / tray / exe icon
    RESOURCES.mkdir(parents=True, exist_ok=True)
    ico_path = RESOURCES / "icon.ico"
    save_ico(master, ico_path, [16, 24, 32, 48, 64, 128, 256])
    print("wrote", ico_path)

    # also drop next to Pack for Inno convenience
    pack_ico = ROOT / "Pack" / "icon.ico"
    pack_ico.write_bytes(ico_path.read_bytes())
    print("wrote", pack_ico)


if __name__ == "__main__":
    main()
