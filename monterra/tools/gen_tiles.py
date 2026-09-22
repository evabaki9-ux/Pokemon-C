"""Bake the overworld tileset (16x16 tiles) and the title background.

Tile ids are fixed and mirrored by TILE_* enums in src/game.h. Animated tiles
occupy 2 adjacent columns (frame A / frame B); the renderer picks the column
from the global tick.
Grid: 8 columns. Column pairs for animated tiles.
"""
import os
import random

from PIL import Image, ImageDraw

import tex

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
T = 16

# --- palette ---------------------------------------------------------------
GRASS_L = (120, 192, 88)
GRASS_D = (96, 168, 72)
GRASS_DK = (72, 144, 56)
TALL_G = (56, 136, 56)
TALL_D = (40, 112, 44)
PATH_C = (232, 192, 128)
PATH_D = (208, 168, 104)
PATH_EDGE = (184, 144, 88)
WATER_C = (72, 136, 216)
WATER_D = (48, 112, 192)
WATER_L = (128, 184, 240)
TREE_G = (48, 128, 64)
TREE_D = (32, 96, 48)
TREE_TRUNK = (136, 96, 56)
CANOPY_D = (24, 72, 40)
FENCE_W = (184, 136, 80)
FENCE_D = (128, 88, 48)
ROCK_C = (168, 168, 176)
ROCK_D = (120, 120, 132)
ROCK_L = (208, 208, 216)
SAND_C = (240, 216, 152)
SAND_D = (216, 192, 128)
LEDGE_C = (184, 144, 96)
LEDGE_D = (144, 104, 64)
ROOF_R = (216, 72, 56)
ROOF_D = (168, 48, 40)
WALL_C = (244, 232, 200)
WALL_D = (208, 192, 160)
DOOR_C = (72, 48, 32)
WIN_C = (136, 200, 232)
FLOOR_C = (208, 168, 120)
FLOOR_D = (184, 144, 96)
FLOOR_LINE = (160, 120, 80)
BED_C = (224, 88, 88)
BED_D = (184, 64, 72)
BLANKET_C = (240, 208, 120)
TABLE_C = (200, 148, 92)
TABLE_D = (156, 108, 64)
BOOK_1 = (196, 108, 92)
BOOK_2 = (108, 140, 196)
BOOK_3 = (140, 180, 108)
MAT_C = (168, 72, 56)
WHITE = (250, 250, 250)
BLACKISH = (36, 36, 36)
PINK_N = (248, 168, 184)

rng = random.Random(1337)


def blank():
    return Image.new("RGBA", (T, T), (0, 0, 0, 0))


def px(d, x, y, c):
    d.point((x, y), c)


def fill(img, c):
    img.paste(c, (0, 0, T, T))


def speckle(d, c, n, ymax=T):
    for _ in range(n):
        x, y = rng.randrange(T), rng.randrange(ymax)
        px(d, x, y, c)


def outline_round(d, c, x0, y0, x1, y1, r=3):
    """Rounded rect outline helper."""
    d.rounded_rectangle((x0, y0, x1, y1), radius=r, outline=c)


# --- tiles -----------------------------------------------------------------
# Each entry: list of functions returning frame images (1 = static, 2 = anim).
def t_grass():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, GRASS_L)
        speckle(d, GRASS_D, 14)
        speckle(d, GRASS_DK, 5)
        return img
    return [frame]


def t_tall():
    def make(shift):
        def frame():
            img = blank(); d = ImageDraw.Draw(img)
            fill(img, GRASS_L)
            speckle(d, GRASS_D, 10)
            for bx in (2, 7, 12):
                for i in range(4):
                    x = bx + i + shift
                    if 0 <= x < T:
                        h = 6 + (i % 3) * 2
                        for y in range(h):
                            px(d, x, T - 1 - y, TALL_G if y < h - 2 else TALL_D)
            for x in range(T):
                px(d, x, T - 1, TALL_D)
            return img
        return frame
    return [make(0), make(1)]


def t_path():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, PATH_C)
        speckle(d, PATH_D, 12)
        for x in range(T):
            px(d, x, 0, PATH_EDGE); px(d, x, T - 1, PATH_EDGE)
            px(d, 0, x, PATH_EDGE); px(d, T - 1, x, PATH_EDGE)
        return img
    return [frame]


def t_flower():
    def make(col):
        def frame():
            img = t_grass()[0]()
            d = ImageDraw.Draw(img)
            d.ellipse((3, 4, 6, 7), fill=col, outline=TALL_D)
            px(d, 4, 5, WHITE); px(d, 5, 6, WHITE)
            d.ellipse((10, 10, 13, 13), fill=col, outline=TALL_D)
            px(d, 11, 11, WHITE); px(d, 12, 12, WHITE)
            return img
        return frame
    return [make((232, 88, 88)), make((240, 208, 96))]


def t_tree():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, GRASS_L)
        speckle(d, GRASS_D, 8)
        d.rounded_rectangle((1, 0, 14, 11), radius=4, fill=TREE_G, outline=CANOPY_D)
        d.rounded_rectangle((3, 2, 12, 9), radius=3, outline=TREE_D)
        px(d, 5, 3, TREE_D); px(d, 9, 5, TREE_D); px(d, 6, 7, TREE_D); px(d, 10, 8, TREE_D)
        d.rectangle((6, 12, 9, 15), fill=TREE_TRUNK, outline=BLACKISH)
        return img
    return [frame]


def t_water():
    def make(off):
        def frame():
            img = blank(); d = ImageDraw.Draw(img)
            fill(img, WATER_C)
            speckle(d, WATER_D, 10)
            for y in (3, 11):
                for i in range(6):
                    x = (off + i * 5 + y) % T
                    px(d, x, y, WATER_L); px(d, x + 1 if x + 1 < T else 0, y, WATER_L)
            return img
        return frame
    return [make(0), make(2)]


def t_fence():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, GRASS_L); speckle(d, GRASS_D, 10)
        d.rectangle((0, 6, 15, 8), fill=FENCE_W, outline=FENCE_D)
        d.rectangle((2, 3, 4, 13), fill=FENCE_W, outline=FENCE_D)
        d.rectangle((11, 3, 13, 13), fill=FENCE_W, outline=FENCE_D)
        return img
    return [frame]


def t_rock():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, GRASS_L); speckle(d, GRASS_D, 8)
        d.polygon([(3, 13), (2, 8), (6, 3), (11, 2), (14, 7), (13, 13)], fill=ROCK_C, outline=BLACKISH)
        d.line((5, 6) and (5, 6, 8, 5), fill=ROCK_D)
        d.line((8, 9, 11, 8), fill=ROCK_D)
        px(d, 6, 4, ROCK_L); px(d, 7, 4, ROCK_L); px(d, 5, 5, ROCK_L)
        return img
    return [frame]


def t_sand():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, SAND_C)
        speckle(d, SAND_D, 12)
        return img
    return [frame]


def t_bridge():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, WATER_C); speckle(d, WATER_D, 6)
        for y in (0, 5, 10, 15):
            d.line((0, y, 15, y), fill=FENCE_D)
        for y in (1, 6, 11):
            d.line((0, y, 15, y), fill=FENCE_W)
        d.line((0, 0, 0, 15), fill=FENCE_D); d.line((15, 0, 15, 15), fill=FENCE_D)
        return img
    return [frame]


def t_ledge():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, GRASS_L); speckle(d, GRASS_D, 6)
        d.rectangle((0, 4, 15, 15), fill=LEDGE_C, outline=BLACKISH)
        for y in range(6, 15, 3):
            d.line((1, y, 14, y), fill=LEDGE_D)
        px(d, 3, 6, LEDGE_D); px(d, 9, 9, LEDGE_D); px(d, 5, 12, LEDGE_D)
        return img
    return [frame]


def t_sign():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, GRASS_L); speckle(d, GRASS_D, 8)
        d.rectangle((5, 11, 10, 14), fill=TREE_TRUNK, outline=BLACKISH)
        d.rectangle((1, 2, 14, 11), fill=FENCE_W, outline=BLACKISH)
        d.line((3, 5, 12, 5), fill=FENCE_D); d.line((3, 8, 12, 8), fill=FENCE_D)
        return img
    return [frame]


def t_roof():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, ROOF_R)
        for y in range(0, T, 4):
            d.line((0, y, 15, y), fill=ROOF_D)
        d.line((0, 0, 15, 0), fill=BLACKISH)
        return img
    return [frame]


def t_roof_edge_l():
    def frame():
        img = t_roof()[0]()
        d = ImageDraw.Draw(img)
        d.line((0, 0, 0, 15), fill=BLACKISH)
        for y in range(0, T, 4):
            px(d, 1, y, ROOF_D)
        return img
    return [frame]


def t_roof_edge_r():
    def frame():
        img = t_roof()[0]()
        d = ImageDraw.Draw(img)
        d.line((15, 0, 15, 15), fill=BLACKISH)
        for y in range(3, T, 4):
            px(d, 14, y, ROOF_D)
        return img
    return [frame]


def t_wall():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, WALL_C)
        d.line((0, 0, 15, 0), fill=BLACKISH)
        d.line((0, 15, 15, 15), fill=WALL_D)
        px(d, 4, 5, WALL_D); px(d, 10, 9, WALL_D)
        return img
    return [frame]


def t_window():
    def frame():
        img = t_wall()[0]()
        d = ImageDraw.Draw(img)
        d.rectangle((3, 4, 12, 11), fill=WIN_C, outline=BLACKISH)
        d.line((3, 7, 12, 7), fill=BLACKISH); d.line((7, 4, 7, 11), fill=BLACKISH)
        px(d, 4, 5, WHITE)
        return img
    return [frame]


def t_door():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, WALL_C)
        d.line((0, 0, 15, 0), fill=BLACKISH)
        d.rectangle((2, 2, 13, 15), fill=DOOR_C, outline=BLACKISH)
        d.rectangle((4, 4, 11, 15), fill=(96, 68, 44), outline=BLACKISH)
        d.ellipse((10, 9, 11, 10), fill=(240, 208, 120))
        return img
    return [frame]


def t_door_heal():
    def frame():
        img = t_door()[0]()
        d = ImageDraw.Draw(img)
        d.rectangle((6, 6, 9, 12), fill=WHITE)
        d.rectangle((5, 8, 10, 10), fill=WHITE)
        return img
    return [frame]


def t_floor():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, FLOOR_C)
        d.line((0, 15, 15, 15), fill=FLOOR_LINE)
        d.line((15, 0, 15, 15), fill=FLOOR_LINE)
        px(d, 5, 5, FLOOR_D); px(d, 11, 11, FLOOR_D)
        return img
    return [frame]


def t_wall_in():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, WALL_D)
        d.line((0, 12, 15, 12), fill=FLOOR_LINE)
        d.line((0, 13, 15, 13), fill=BLACKISH)
        return img
    return [frame]


def t_bed():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, FLOOR_C)
        d.rectangle((2, 0, 13, 15), fill=BED_C, outline=BLACKISH)
        d.rectangle((2, 0, 13, 4), fill=BLANKET_C, outline=BLACKISH)
        d.rectangle((3, 1, 12, 3), fill=WHITE, outline=BED_D)
        d.rectangle((2, 5, 13, 15), fill=BLANKET_C, outline=BLACKISH)
        return img
    return [frame]


def t_table():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, FLOOR_C)
        d.rectangle((1, 2, 14, 13), fill=TABLE_C, outline=BLACKISH)
        d.rectangle((1, 2, 14, 4), fill=TABLE_D)
        return img
    return [frame]


def t_shelf():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, WALL_D)
        d.rectangle((1, 1, 14, 14), fill=TABLE_D, outline=BLACKISH)
        d.line((1, 7, 14, 7), fill=BLACKISH)
        for x, c in ((3, BOOK_1), (5, BOOK_2), (8, BOOK_3), (10, BOOK_1), (12, BOOK_2)):
            d.rectangle((x, 2, x + 1, 6), fill=c, outline=BLACKISH)
            d.rectangle((x, 8, x + 1, 13), fill=c, outline=BLACKISH)
        return img
    return [frame]


def t_mat():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, FLOOR_C)
        d.rectangle((2, 2, 13, 13), fill=MAT_C, outline=BLACKISH)
        d.rectangle((4, 4, 11, 11), outline=(220, 140, 120))
        return img
    return [frame]


def t_pc():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, FLOOR_C)
        d.rectangle((2, 1, 13, 14), fill=(150, 160, 176), outline=BLACKISH)
        d.rectangle((3, 2, 12, 8), fill=(56, 72, 104), outline=BLACKISH)
        d.ellipse((6, 4, 9, 6), fill=(120, 200, 240))
        d.rectangle((4, 10, 11, 12), fill=(210, 210, 218), outline=BLACKISH)
        return img
    return [frame]


def t_counter():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, FLOOR_C)
        d.rectangle((0, 2, 15, 13), fill=TABLE_C, outline=BLACKISH)
        d.rectangle((0, 2, 15, 4), fill=TABLE_D)
        d.rectangle((6, 7, 9, 9), fill=WIN_C, outline=BLACKISH)
        return img
    return [frame]


def t_plant():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, FLOOR_C)
        d.rectangle((5, 11, 10, 14), fill=(200, 120, 72), outline=BLACKISH)
        d.ellipse((3, 3, 12, 11), fill=TREE_G, outline=CANOPY_D)
        px(d, 6, 5, TREE_D); px(d, 9, 7, TREE_D); px(d, 5, 8, TREE_D)
        return img
    return [frame]


def t_carpet():
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, (200, 120, 120))
        d.rectangle((1, 1, 14, 14), outline=(232, 168, 88))
        px(d, 3, 3, (232, 168, 88)); px(d, 12, 3, (232, 168, 88))
        px(d, 3, 12, (232, 168, 88)); px(d, 12, 12, (232, 168, 88))
        return img
    return [frame]


def t_grass_town():  # softer town grass, no speckle clutter
    def frame():
        img = blank(); d = ImageDraw.Draw(img)
        fill(img, GRASS_L)
        speckle(d, GRASS_D, 6)
        return img
    return [frame]


# Order matters! Mirrors enum TileId in src/game.h.
TILES = [
    t_grass,          # 0  TILE_GRASS (anim A/B)
    t_tall,           # 1  TILE_TALL
    t_path,           # 2  TILE_PATH
    t_flower,         # 3  TILE_FLOWER
    t_tree,           # 4  TILE_TREE
    t_water,          # 5  TILE_WATER
    t_fence,          # 6  TILE_FENCE
    t_rock,           # 7  TILE_ROCK
    t_sand,           # 8  TILE_SAND
    t_bridge,         # 9  TILE_BRIDGE
    t_ledge,          # 10 TILE_LEDGE
    t_sign,           # 11 TILE_SIGN
    t_roof,           # 12 TILE_ROOF
    t_roof_edge_l,    # 13 TILE_ROOF_L
    t_roof_edge_r,    # 14 TILE_ROOF_R
    t_wall,           # 15 TILE_WALL
    t_window,         # 16 TILE_WINDOW
    t_door,           # 17 TILE_DOOR
    t_door_heal,      # 18 TILE_DOOR_HEAL
    t_floor,          # 19 TILE_FLOOR
    t_wall_in,        # 20 TILE_WALL_IN
    t_bed,            # 21 TILE_BED
    t_table,          # 22 TILE_TABLE
    t_shelf,          # 23 TILE_SHELF
    t_mat,            # 24 TILE_MAT
    t_pc,             # 25 TILE_PC
    t_counter,        # 26 TILE_COUNTER
    t_plant,          # 27 TILE_PLANT
    t_carpet,         # 28 TILE_CARPET
    t_grass_town,     # 29 TILE_GRASS_TOWN
]


def build_tileset():
    frames = [fn() for fn in TILES]
    ncols = sum(len(f) for f in frames)
    sheet = Image.new("RGBA", (ncols * T, T), (0, 0, 0, 0))
    x = 0
    for f in frames:
        for framefn in f:
            sheet.paste(framefn(), (x * T, 0))
            x += 1
    return sheet


def build_title():
    """240x160 title background: dusk gradient, hills, route silhouette."""
    W, H = 240, 160
    img = Image.new("RGBA", (W, H))
    d = ImageDraw.Draw(img)
    top, mid, bot = (24, 20, 72), (120, 72, 136), (232, 128, 104)
    for y in range(H):
        if y < H // 2:
            t = y / (H // 2)
            c = tuple(int(a + (b - a) * t) for a, b in zip(top, mid))
        else:
            t = (y - H // 2) / (H - H // 2)
            c = tuple(int(a + (b - a) * t) for a, b in zip(mid, bot))
        d.line((0, y, W, y), fill=c)
    # stars
    trng = random.Random(7)
    for _ in range(26):
        x, y = trng.randrange(W), trng.randrange(56)
        px(d, x, y, (250, 250, 220))
    # far hills
    d.polygon([(0, 96), (30, 74), (64, 92), (98, 68), (140, 94), (176, 72), (210, 90), (240, 78), (240, 130), (0, 130)], fill=(48, 40, 88))
    # near hills
    d.polygon([(0, 112), (40, 96), (90, 112), (130, 94), (190, 110), (240, 98), (240, 160), (0, 160)], fill=(34, 30, 64))
    # ground strip
    d.rectangle((0, 128, W, H), fill=(40, 72, 48))
    for _ in range(80):
        x, y = trng.randrange(W), trng.randrange(128, H)
        px(d, x, y, (56, 96, 60) if trng.random() < 0.7 else (88, 136, 80))
    return img


def build_platforms():
    """Two battle platforms (80x20 each): enemy tan, player grey."""
    img = Image.new("RGBA", (160, 20), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse((4, 2, 76, 18), fill=(224, 200, 160, 255), outline=(168, 144, 104, 255))
    d.ellipse((84, 2, 156, 18), fill=(176, 176, 188, 255), outline=(120, 120, 136, 255))
    return img


def main():
    out = os.path.join(ROOT, "assets")
    os.makedirs(out, exist_ok=True)
    sheet = build_tileset()
    tex.write_tex(os.path.join(out, "tiles.tex"), sheet)
    tex.write_tex(os.path.join(out, "title.tex"), build_title())
    tex.write_tex(os.path.join(out, "platforms.tex"), build_platforms())
    sheet.save(os.path.join(ROOT, "tools", "tiles_preview.png"))


if __name__ == "__main__":
    main()
