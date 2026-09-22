"""Bake the pixel font atlas: ASCII 32..126 rendered from Press Start 2P (OFL).
Each glyph occupies one 8x8 cell, monospace advance 8px. White pixels; the
engine tints via SDL_SetTextureColorMod.
"""
import os
from PIL import Image, ImageDraw, ImageFont

import tex

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

FIRST, LAST = 32, 126
CW, CH = 8, 8


def main() -> None:
    font = ImageFont.truetype(os.path.join(HERE, "press-start-2p.ttf"), 8)
    cols = 16
    count = LAST - FIRST + 1
    rows = (count + cols - 1) // cols
    atlas = Image.new("RGBA", (cols * CW, rows * CH), (0, 0, 0, 0))
    d = ImageDraw.Draw(atlas)
    for i, code in enumerate(range(FIRST, LAST + 1)):
        cx, cy = (i % cols) * CW, (i // cols) * CH
        ch = chr(code)
        if ch == " ":
            continue
        # Render each glyph into its own tiny image so nothing bleeds over cells.
        cell = Image.new("RGBA", (CW * 2, CH * 2), (0, 0, 0, 0))
        cd = ImageDraw.Draw(cell)
        cd.text((0, 0), ch, font=font, fill=(255, 255, 255, 255))
        bbox = cell.getbbox()
        if bbox is None:
            continue
        # Centre the ink inside the 8x8 cell.
        w = min(bbox[2], CW)
        h = min(bbox[3], CH)
        crop = cell.crop((0, 0, w, h))
        ox = (CW - w) // 2
        oy = (CH - h) // 2
        atlas.paste(crop, (cx + ox, cy + oy), crop)
    tex.write_tex(os.path.join(ROOT, "assets", "font.tex"), atlas)


if __name__ == "__main__":
    main()
