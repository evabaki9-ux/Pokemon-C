"""Bake character sprites: 16x16 walking sprites, 4 directions x 2 frames.

Directions authored: DOWN, UP, SIDE (side is flipped at draw time for RIGHT).
Palette letters:
  . transparent | o outline | h hat/hair | H hat brim/shade | s skin
  e eye | w white | b shirt | p pants | d shoe
Characters are palette swaps of one base sprite set.
"""
import os

from PIL import Image

import tex

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

DOWN_A = [
    "................",
    "................",
    "................",
    ".....oooooo.....",
    "....ohhhhhho....",
    "...ohhhhhhhho...",
    "...oHHHHHHHHo...",
    "...osssssssso...",
    "...oseseseso....",
    "....ssssssss....",
    "....obbbbbbo....",
    "...obwbbbbwbo...",
    "...osobbbbso....",
    "....opppppo.....",
    "....odo.odo.....",
    "....od..od......",
]
DOWN_B = [
    "................",
    "................",
    "................",
    ".....oooooo.....",
    "....ohhhhhho....",
    "...ohhhhhhhho...",
    "...oHHHHHHHHo...",
    "...osssssssso...",
    "...oseseseso....",
    "....ssssssss....",
    "....obbbbbbo....",
    "...obwbbbbwbo...",
    "...osobbbbso....",
    "....opppppo.....",
    ".....dpppod.....",
    "....odo..od.....",
]
UP_A = [
    "................",
    "................",
    "................",
    ".....oooooo.....",
    "....ohhhhhho....",
    "...ohhhhhhhho...",
    "...ohhhhhhhho...",
    "...ohhhhhhhho...",
    "...oohhhhhhoo...",
    "....ohhhhhho....",
    "....obbbbbbo....",
    "...obbbbbbbbo...",
    "...osobbbbso....",
    "....opppppo.....",
    "....odo.odo.....",
    "....od..od......",
]
UP_B = [
    "................",
    "................",
    "................",
    ".....oooooo.....",
    "....ohhhhhho....",
    "...ohhhhhhhho...",
    "...ohhhhhhhho...",
    "...ohhhhhhhho...",
    "...oohhhhhhoo...",
    "....ohhhhhho....",
    "....obbbbbbo....",
    "...obbbbbbbbo...",
    "...osobbbbso....",
    "....opppppo.....",
    ".....dpppod.....",
    "....odo..od.....",
]
SIDE_A = [
    "................",
    "................",
    "................",
    ".....oooooo.....",
    "....ohhhhhho....",
    "....ohhhhhhho...",
    "....oHHHHHHHo...",
    "....osssssso....",
    "....osses.so....",
    "....osssssso....",
    ".....obbbbo.....",
    "....obbbbbbo....",
    "....osobbbso....",
    ".....opppo......",
    ".....oppod......",
    ".....od.od......",
]
SIDE_B = [
    "................",
    "................",
    "................",
    ".....oooooo.....",
    "....ohhhhhho....",
    "....ohhhhhhho...",
    "....oHHHHHHHo...",
    "....osssssso....",
    "....osses.so....",
    "....osssssso....",
    ".....obbbbo.....",
    "....obbbbbbo....",
    "....osobbbso....",
    ".....opppo......",
    "......dppo......",
    ".....oddod......",
]

GRIDS = {"down": [DOWN_A, DOWN_B], "up": [UP_A, UP_B], "side": [SIDE_A, SIDE_B]}

# Character palettes: id -> {letter: rgba}
CHARACTERS = [
    {  # 0 PLAYER: red cap, blue shirt
        "o": (32, 32, 40, 255), "h": (216, 64, 56, 255), "H": (160, 40, 40, 255),
        "s": (248, 200, 144, 255), "e": (32, 40, 56, 255), "w": (250, 250, 250, 255),
        "b": (56, 104, 200, 255), "p": (56, 64, 88, 255), "d": (40, 32, 32, 255),
    },
    {  # 1 PROF: grey hair, white coat
        "o": (32, 32, 40, 255), "h": (184, 184, 192, 255), "H": (144, 144, 152, 255),
        "s": (240, 192, 144, 255), "e": (32, 40, 56, 255), "w": (250, 250, 250, 255),
        "b": (242, 242, 246, 255), "p": (88, 88, 104, 255), "d": (48, 40, 40, 255),
    },
    {  # 2 MOM: brown hair, rose top
        "o": (32, 32, 40, 255), "h": (146, 92, 48, 255), "H": (112, 68, 36, 255),
        "s": (248, 204, 152, 255), "e": (32, 40, 56, 255), "w": (250, 250, 250, 255),
        "b": (216, 96, 128, 255), "p": (120, 72, 120, 255), "d": (48, 40, 40, 255),
    },
    {  # 3 NURSE: pink hair, white uniform
        "o": (32, 32, 40, 255), "h": (244, 160, 184, 255), "H": (208, 120, 152, 255),
        "s": (248, 208, 160, 255), "e": (32, 40, 56, 255), "w": (250, 250, 250, 255),
        "b": (250, 250, 252, 255), "p": (232, 200, 208, 255), "d": (180, 120, 136, 255),
    },
    {  # 4 KID: yellow hat, green shirt
        "o": (32, 32, 40, 255), "h": (248, 208, 72, 255), "H": (200, 160, 48, 255),
        "s": (240, 196, 148, 255), "e": (32, 40, 56, 255), "w": (250, 250, 250, 255),
        "b": (88, 168, 96, 255), "p": (64, 96, 168, 255), "d": (40, 32, 32, 255),
    },
]


def render_grid(grid, pal):
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    p = img.load()
    for y, row in enumerate(grid):
        assert len(row) == 16, f"row {y} len {len(row)}"
        for x, ch in enumerate(row):
            if ch == ".":
                continue
            p[x, y] = pal[ch]
    return img


def main():
    nchars = len(CHARACTERS)
    # Layout: row per character; 6 frames per row (down A/B, up A/B, side A/B)
    frames_per_char = 6
    sheet = Image.new("RGBA", (16 * frames_per_char, 16 * nchars), (0, 0, 0, 0))
    for ci, pal in enumerate(CHARACTERS):
        i = 0
        for d in ("down", "up", "side"):
            for grid in GRIDS[d]:
                sheet.paste(render_grid(grid, pal), (16 * i, 16 * ci))
                i += 1
    out = os.path.join(ROOT, "assets")
    os.makedirs(out, exist_ok=True)
    tex.write_tex(os.path.join(out, "chars.tex"), sheet)
    sheet.save(os.path.join(ROOT, "tools", "chars_preview.png"))


if __name__ == "__main__":
    main()
