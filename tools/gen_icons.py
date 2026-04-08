#!/usr/bin/env python3
"""Generate 256x256 snake game icons for each platform."""
from PIL import Image, ImageDraw, ImageFont
import os

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "repository")

# ── Palette ────────────────────────────────────────────────────────────────
BG        = (15,  15,  26)       # dark navy
GRID      = (22,  22,  38)       # subtle grid lines
HEAD      = (34, 197,  94)       # green head
BODY_COLS = [                    # body gradient (head→tail)
    ( 74, 222, 128),
    ( 96, 165, 250),
    (139,  92, 246),
    (248, 113, 113),
    (251, 191,  36),
]
FOOD_R    = (221,  34,  34)      # red food
FOOD_H    = (255, 153, 153)      # highlight
EYE       = (15,  15,  26)       # eye dot

# ── Snake path (pixel coords in a 16×16 grid, cell=14px, offset 16px) ─────
CELL  = 14
OFF   = 17   # left/top margin so snake is centered

def cell(cx, cy):
    """Top-left pixel of a grid cell."""
    return OFF + cx * CELL, OFF + cy * CELL

def draw_segment(d, cx, cy, color, cell_size=CELL, shrink=2):
    """Rounded rectangle for one body segment."""
    x, y = cell(cx, cy)
    r = cell_size // 4
    s = shrink
    d.rounded_rectangle([x+s, y+s, x+cell_size-s, y+cell_size-s],
                         radius=r, fill=color)

def draw_head(d, cx, cy, color, highlight):
    """Head with eye."""
    x, y = cell(cx, cy)
    r = CELL // 3
    d.rounded_rectangle([x+1, y+1, x+CELL-1, y+CELL-1], radius=r, fill=color)
    # highlight
    d.ellipse([x+3, y+2, x+6, y+5], fill=highlight)
    # eye
    d.ellipse([x+CELL-5, y+3, x+CELL-3, y+5], fill=EYE)

def draw_food(d, cx, cy):
    """3-shade volumetric food dot."""
    x, y = cell(cx, cy)
    c = CELL // 2
    r = CELL // 2 - 1
    d.ellipse([x+1, y+1, x+CELL-2, y+CELL-2], fill=FOOD_R)
    d.ellipse([x+2, y+2, x+5, y+5], fill=FOOD_H)

# Snake body path: (col, row) list, index 0 = head
SNAKE = [
    (10,  3),
    ( 9,  3), ( 8,  3), ( 7,  3), ( 6,  3),
    ( 6,  4), ( 6,  5), ( 6,  6),
    ( 7,  6), ( 8,  6), ( 9,  6), (10,  6), (11, 6),
    (11,  7), (11,  8), (11,  9),
    (10,  9), ( 9,  9), ( 8,  9), ( 7,  9), ( 6, 9),
    ( 5,  9), ( 4,  9),
    ( 4, 10), ( 4, 11),
    ( 5, 11), ( 6, 11),
]
FOOD = [(13, 5), (3, 7)]

def lerp_color(a, b, t):
    return tuple(int(a[i] + (b[i]-a[i])*t) for i in range(3))

def body_color(idx, total):
    """Gradient across BODY_COLS stops."""
    if total <= 1:
        return BODY_COLS[0]
    t = idx / (total - 1) * (len(BODY_COLS) - 1)
    lo = int(t)
    hi = min(lo + 1, len(BODY_COLS) - 1)
    return lerp_color(BODY_COLS[lo], BODY_COLS[hi], t - lo)

def make_icon(platform_label, out_path):
    SIZE = 256
    img  = Image.new("RGB", (SIZE, SIZE), BG)
    d    = ImageDraw.Draw(img)

    # Subtle grid
    for i in range(0, SIZE, CELL):
        d.line([(i, 0), (i, SIZE)], fill=GRID, width=1)
        d.line([(0, i), (SIZE, i)], fill=GRID, width=1)

    # Food
    for fx, fy in FOOD:
        draw_food(d, fx, fy)

    # Body (tail → head-1)
    total = len(SNAKE)
    for idx in range(total - 1, 0, -1):
        cx, cy = SNAKE[idx]
        col = body_color(idx, total)
        draw_segment(d, cx, cy, col)

    # Head
    hx, hy = SNAKE[0]
    hl = tuple(min(255, c + 60) for c in HEAD)
    draw_head(d, hx, hy, HEAD, hl)

    # Platform label (bottom-right corner badge)
    badge_w, badge_h = 72, 32
    bx = SIZE - badge_w - 10
    by = SIZE - badge_h - 10
    d.rounded_rectangle([bx, by, bx+badge_w, by+badge_h],
                         radius=8, fill=(30, 30, 52))
    d.rounded_rectangle([bx+1, by+1, bx+badge_w-1, by+badge_h-1],
                         radius=7, outline=(96, 165, 250), width=2)

    # Try system fonts, fall back to default
    font = None
    for name in [
        "/System/Library/Fonts/Helvetica.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Bold.ttf",
    ]:
        if os.path.exists(name):
            try:
                font = ImageFont.truetype(name, 18)
                break
            except Exception:
                pass
    if font is None:
        font = ImageFont.load_default()

    bbox = d.textbbox((0, 0), platform_label, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    tx = bx + (badge_w - tw) // 2
    ty = by + (badge_h - th) // 2 - bbox[1]
    d.text((tx, ty), platform_label, font=font, fill=(220, 220, 255))

    img.save(out_path, "PNG")
    print(f"  {out_path}")

if __name__ == "__main__":
    print("Generating icons...")
    make_icon("3DS", os.path.join(OUT_DIR, "3ds", "snake.png"))
    make_icon("NDS", os.path.join(OUT_DIR, "nds", "snake.png"))
    print("Done.")
