#!/usr/bin/env python3
"""Approximate preview of the DuperDurio top screen (400x240) with real sprites
at their in-game sizes. Tiles are drawn roughly like the procedural renderer.
NOT pixel-exact to the 3DS output — a composition/scale sanity check."""
from PIL import Image, ImageDraw
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
PROC = REPO / "duperdurio" / "duperdurio-common" / "sprites" / "processed"
SCALE = 3                      # upscale the whole 400x240 for visibility
W, H, TS = 400, 240, 24

img = Image.new("RGBA", (W, H), (8, 6, 28, 255))
d = ImageDraw.Draw(img)
# sky gradient bottom
for y in range(H*5//8, H):
    t = (y - H*5//8) / (H - H*5//8)
    d.line([(0, y), (W, y)], fill=(int(8+22*t), int(6+12*t), int(28+32*t), 255))
# a few stars
for (x, y) in [(30,12),(82,7),(140,22),(195,5),(308,35),(370,10),(255,18),(115,40)]:
    d.point((x, y), fill=(210,215,255,255))

def rock(x, y):
    d.rectangle([x, y, x+TS-1, y+TS-1], fill=(55,70,85,255))      # body
    d.rectangle([x, y, x+TS-1, y+5], fill=(90,115,135,255))       # lit top
    d.line([(x, y+TS//2), (x+TS-1, y+TS//2)], fill=(35,45,55,255))# crack
def qblock(x, y):
    d.rectangle([x, y, x+TS-1, y+TS-1], fill=(30,42,58,255))
    d.rectangle([x, y, x+TS-1, y+TS-1], outline=(80,200,175,255), width=2)
    d.ellipse([x+7, y+7, x+TS-7, y+TS-7], fill=(180,180,180,255))
    d.ellipse([x+10, y+10, x+TS-10, y+TS-10], fill=(110,110,110,255))

GY = 192                       # ground top on screen (two rock rows)
for tx in range(0, W//TS + 1):
    rock(tx*TS, GY); rock(tx*TS, GY+TS)
qblock(7*TS, GY-3*TS)
qblock(11*TS, GY-3*TS)

def place(name, cx_tile, scale_mul, flip=False, feet=GY):
    p = PROC / f"{name}.png"
    if not p.exists():
        return
    s = Image.open(p).convert("RGBA")
    box_h = TS
    base = (box_h / s.height) * scale_mul
    dw, dh = max(1, round(s.width*base)), max(1, round(s.height*base))
    s = s.resize((dw, dh), Image.NEAREST)
    if flip:
        s = s.transpose(Image.FLIP_LEFT_RIGHT)
    cx = cx_tile * TS + TS//2
    img.alpha_composite(s, (cx - dw//2, feet - dh))

place("durio_stand", 3, 1.40)                 # hero on ground
place("crab_walk0", 9, 1.50, flip=True)
place("snail_walk0", 15, 1.50, flip=True)
place("nut_0", 13, 0.85, feet=GY-3*TS+TS)     # nut sitting in a cell
place("nut_0", 5, 0.85)

out = img.resize((W*SCALE, H*SCALE), Image.NEAREST)
dst = REPO / "tools" / "sprite_cache" / "scene-preview.png"
out.save(dst)
print("wrote", dst.relative_to(REPO))
