# -*- coding: utf-8 -*-
"""
Generate taskbar unplated variants from ORIGINAL logos.
Do NOT shrink / add padding — keep artwork full-size; only resize.
Unplated files tell Windows not to draw a color plate on the taskbar.
"""
from PIL import Image
from pathlib import Path

ROOT = Path(r"e:\BB_pro\gitHub\lailaichen\YoyoScreenshot")
ASSETS = ROOT / "Pack" / "msix" / "Assets"


def scrub_fringe(im: Image.Image, a_cut: int = 8) -> Image.Image:
	im = im.copy()
	px = im.load()
	w, h = im.size
	for y in range(h):
		for x in range(w):
			r, g, b, a = px[x, y]
			if a <= a_cut:
				px[x, y] = (0, 0, 0, 0)
	return im


def resize_hq(im: Image.Image, size: int) -> Image.Image:
	return scrub_fringe(im.resize((size, size), Image.Resampling.LANCZOS))


def save(im: Image.Image, path: Path):
	im.save(path, "PNG")
	print(f"wrote {path.name} {im.size} bbox={im.getbbox()}")


def main():
	master = Image.open(ASSETS / "Square310x310Logo.png").convert("RGBA")
	master = scrub_fringe(master)
	print(f"master bbox={master.getbbox()} corner={master.getpixel((0,0))}")

	# Keep Square44 as restored original; only emit qualifier variants
	target_sizes = [16, 20, 24, 30, 32, 36, 40, 48, 60, 64, 72, 80, 96, 256]
	for s in target_sizes:
		img = resize_hq(master, s)
		save(img, ASSETS / f"Square44x44Logo.targetsize-{s}.png")
		save(img, ASSETS / f"Square44x44Logo.targetsize-{s}_altform-unplated.png")
		save(img, ASSETS / f"Square44x44Logo.targetsize-{s}_altform-lightunplated.png")

	for scale, px in ((100, 44), (125, 55), (150, 66), (200, 88), (400, 176)):
		img = resize_hq(master, px)
		save(img, ASSETS / f"Square44x44Logo.scale-{scale}_altform-unplated.png")
		save(img, ASSETS / f"Square44x44Logo.scale-{scale}_altform-lightunplated.png")

	print("DONE (base logos untouched)")


if __name__ == "__main__":
	main()
