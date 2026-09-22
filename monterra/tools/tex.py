"""Common .tex writer: trivial raw RGBA texture format used by the C engine.

Layout: b"TEX1" | u16 width (LE) | u16 height (LE) | width*height*4 bytes RGBA
"""
import struct
from PIL import Image


def write_tex(path: str, img: Image.Image) -> None:
    img = img.convert("RGBA")
    with open(path, "wb") as f:
        f.write(b"TEX1")
        f.write(struct.pack("<HH", img.width, img.height))
        f.write(img.tobytes())
    print(f"wrote {path} ({img.width}x{img.height})")
