#!/usr/bin/env python3
"""Compose a SQUARE repository cover from the real DuperDurio game assets:
hero + ?-block + totem + planet + ground, no text (scene like the in-game look)."""
from PIL import Image, ImageDraw
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
PROC = REPO / "duperdurio" / "duperdurio-common" / "sprites" / "processed"
OUT = REPO / "duperdurio" / "cover.png"

S = 1024
GROUND = 740
img = Image.new("RGBA", (S, S), (8, 6, 28, 255))
d = ImageDraw.Draw(img)

# ── deep-space gradient ──
for y in range(S):
    t = y / S
    d.line([(0, y), (S, y)], fill=(int(8 + 16 * t), int(6 + 9 * t), int(28 + 30 * t), 255))

# ── stars ──
r = 987654321
for _ in range(150):
    r = (r * 1103515245 + 12345) & 0x7fffffff
    x = r % S
    r = (r * 1103515245 + 12345) & 0x7fffffff
    y = r % (GROUND - 30)
    s = 3 if (r & 7) == 0 else 2
    c = 235 if s == 3 else 160
    d.rectangle([x, y, x + s, y + s], fill=(c, c, 255, 255))


def paste(name, w, x, y, anchor="bottomcenter"):
    p = PROC / f"{name}.png"
    if not p.exists():
        return
    sp = Image.open(p).convert("RGBA")
    sp = sp.resize((w, max(1, round(sp.height * w / sp.width))), Image.NEAREST)
    if anchor == "bottomcenter":
        x -= sp.width // 2; y -= sp.height
    elif anchor == "center":
        x -= sp.width // 2; y -= sp.height // 2
    img.alpha_composite(sp, (x, y))


# ── celestial bodies ──
paste("bg_nebula", 360, 820, 60, "center")     # teal ringed planet, top-right
paste("bg_moon", 120, 150, 150, "center")       # small moon, top-left

# ── distant mountains ──
for px, h, hw in [(150, 150, 190), (430, 210, 220), (760, 160, 200), (1010, 180, 200)]:
    d.polygon([(px - hw, GROUND), (px, GROUND - h), (px + hw, GROUND)], fill=(30, 26, 54, 255))

# ── ground ──
d.rectangle([0, GROUND, S, S], fill=(55, 70, 85, 255))
d.rectangle([0, GROUND, S, GROUND + 14], fill=(90, 115, 135, 255))
d.rectangle([0, GROUND, S, GROUND + 6], fill=(125, 152, 172, 255))
g = 13579
for _ in range(160):
    g = (g * 1103515245 + 12345) & 0x7fffffff
    x = g % S; y = GROUND + 24 + (g >> 8) % (S - GROUND - 28)
    d.rectangle([x, y, x + 3, y + 3], fill=(40, 50, 60, 255))


def draw_qblock(cx, cy, s):
    x, y = cx - s // 2, cy - s // 2
    d.rectangle([x, y, x + s, y + s], fill=(30, 42, 58, 255))
    bw = max(4, s // 12)
    d.rectangle([x, y, x + s, y + s], outline=(80, 200, 175, 255), width=bw)
    paste("nut_0", int(s * 0.6), cx, cy, "center")


def draw_totem(cx, base_y, seg, w):
    """Basalt totem: body segments with orange rune stripes + eye-rune cap."""
    body, edge, glyph = (50, 44, 62, 255), (78, 70, 94, 255), (220, 100, 20, 255)
    h = w  # square segments
    for i in range(seg):
        y = base_y - (i + 1) * h
        x = cx - w // 2
        d.rectangle([x, y, x + w, y + h], fill=body)
        d.rectangle([x, y, x + max(3, w // 10), y + h], fill=edge)               # left edge
        d.rectangle([x + w - max(3, w // 10), y, x + w, y + h], fill=edge)        # right edge
        d.rectangle([x + w // 6, y + h // 2 - 4, x + w - w // 6, y + h // 2 + 4], fill=glyph)
    # cap (top)
    ty = base_y - seg * h - h
    cx0 = cx - (w // 2 + 8)
    d.rectangle([cx0, ty, cx0 + w + 16, ty + h], fill=body)
    d.rectangle([cx0, ty, cx0 + w + 16, ty + 6], fill=edge)
    # orange eye rune + dark pupil
    ew, eh = int(w * 0.5), int(h * 0.42)
    ex, ey = cx - ew // 2, ty + h // 4
    d.rectangle([ex, ey, ex + ew, ey + eh], fill=glyph)
    d.rectangle([ex + ew // 4, ey + eh // 4, ex + ew * 3 // 4, ey + eh * 3 // 4], fill=body)


# ── scene props (like the reference: block + totem) ──
draw_qblock(440, 470, 110)
paste("nut_0", 64, 600, 380, "center")          # a floating nut
draw_totem(800, GROUND, 3, 96)                  # totem on the right

# ── hero ──
paste("durio_stand", 230, 300, GROUND + 18, "bottomcenter")

img.convert("RGB").save(OUT)
print("wrote", OUT.relative_to(REPO), img.size)
