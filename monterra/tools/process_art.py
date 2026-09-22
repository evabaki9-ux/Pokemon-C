"""Process AI-generated creature art into 64x64 RGBA .tex battle sprites.

Chroma-keys the magenta backdrop, crops to content, fits into the frame.
"""
import os

from PIL import Image, ImageOps

import tex

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "art_src")
DST = os.path.join(ROOT, "assets", "creatures")


def key_magenta(img: Image.Image) -> Image.Image:
    img = img.convert("RGBA")
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            # magenta-ish: red and blue high, green far below both
            if r > 150 and b > 150 and g < min(r, b) - 60 and abs(r - b) < 110:
                px[x, y] = (0, 0, 0, 0)
            elif r > 120 and b > 120 and g < 90:  # dimmer halo
                px[x, y] = (0, 0, 0, 0)
    return img


def process(name: str) -> None:
    path = os.path.join(SRC, name + ".png")
    img = Image.open(path)
    img = key_magenta(img)
    bbox = img.getbbox()
    if bbox is None:
        raise SystemExit(f"{name}: fully transparent after keying?!")
    img = img.crop(bbox)
    # Fit into 56x56 (leave 4px margin inside 64x64 frame), keep aspect.
    target = 56
    scale = min(target / img.width, target / img.height)
    nw, nh = max(1, round(img.width * scale)), max(1, round(img.height * scale))
    img = img.resize((nw, nh), Image.LANCZOS)
    # Quantize for a cleaner sprite look (FASTOCTREE keeps alpha).
    img = img.quantize(colors=24, method=Image.FASTOCTREE, dither=Image.Dither.NONE).convert("RGBA")
    canvas = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    canvas.paste(img, ((64 - nw) // 2, (64 - nh) // 2), img)
    tex.write_tex(os.path.join(DST, name + ".tex"), canvas)
    canvas.save(os.path.join(DST, name + ".png"))


def main() -> None:
    os.makedirs(DST, exist_ok=True)
    for f in sorted(os.listdir(SRC)):
        if f.endswith(".png"):
            process(os.path.splitext(f)[0])


if __name__ == "__main__":
    main()
