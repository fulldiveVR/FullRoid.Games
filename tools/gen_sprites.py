#!/usr/bin/env python3
"""
DuperDurio sprite generator.

Pipeline:  manifest.json  ->  Replicate (recraft-v3 / pixel_art)  ->  raw PNG cache
           ->  background removal (rembg) + trim + downscale  ->  processed PNGs
           ->  gfx/sprites.t3s  (atlas description for tex3ds, in SPR_* index order)
           ->  contact-sheet.png (review grid)

Generation is cached per sprite (tools/sprite_cache/raw/<name>.png); re-runs are free
unless you pass --force or --only to regenerate specific sprites.

Env:   REPLICATE_API_TOKEN must be set.
Usage: python3 tools/gen_sprites.py [--force] [--only name1,name2] [--no-generate]
       --no-generate : skip the API, only (re)post-process whatever is already cached.
"""
import argparse
import io
import json
import os
import sys
import time
from pathlib import Path

import requests
from PIL import Image

REPO = Path(__file__).resolve().parent.parent
SPRITES_DIR = REPO / "duperdurio" / "duperdurio-common" / "sprites"
MANIFEST = SPRITES_DIR / "manifest.json"
CACHE_RAW = REPO / "tools" / "sprite_cache" / "raw"
PROCESSED = SPRITES_DIR / "processed"
GFX_DIR = REPO / "duperdurio" / "duperdurio-3ds" / "gfx"
T3S_OUT = GFX_DIR / "sprites.t3s"
ATLAS_H = SPRITES_DIR / "sprite_atlas.h"
CONTACT = REPO / "tools" / "sprite_cache" / "contact-sheet.png"

API = "https://api.replicate.com/v1"


def token():
    t = os.environ.get("REPLICATE_API_TOKEN")
    if not t:
        sys.exit("REPLICATE_API_TOKEN not set")
    return t


def generate(model, inputs, prompt):
    """Run a Replicate prediction, return raw image bytes."""
    owner, name = model.split("/")
    headers = {
        "Authorization": f"Bearer {token()}",
        "Content-Type": "application/json",
        "Prefer": "wait",  # block up to ~60s for sync completion
    }
    body = {"input": {**inputs, "prompt": prompt}}
    r = requests.post(f"{API}/models/{owner}/{name}/predictions",
                      headers=headers, json=body, timeout=120)
    r.raise_for_status()
    pred = r.json()

    # Poll if not finished synchronously.
    poll_url = pred.get("urls", {}).get("get")
    while pred.get("status") not in ("succeeded", "failed", "canceled"):
        time.sleep(2)
        pred = requests.get(poll_url, headers=headers, timeout=60).json()
    if pred.get("status") != "succeeded":
        raise RuntimeError(f"prediction {pred.get('status')}: {pred.get('error')}")

    out = pred["output"]
    url = out[0] if isinstance(out, list) else out
    img = requests.get(url, timeout=120)
    img.raise_for_status()
    return img.content


# ── post-processing ─────────────────────────────────────────────

_rembg_session = None


def cutout(img: Image.Image) -> Image.Image:
    """Remove background -> RGBA with alpha."""
    global _rembg_session
    from rembg import remove, new_session
    if _rembg_session is None:
        _rembg_session = new_session("u2netp")  # light, fast model
    return remove(img.convert("RGBA"), session=_rembg_session)


def harden_alpha(img: Image.Image) -> Image.Image:
    """Threshold alpha to fully on/off. Done BEFORE measuring the bbox so the
    faint semi-transparent halo rembg leaves doesn't inflate the crop (which,
    after downscale, would become a transparent margin and float the sprite)."""
    if img.mode != "RGBA":
        return img
    r, g, b, a = img.split()
    a = a.point(lambda v: 255 if v >= 128 else 0)
    return Image.merge("RGBA", (r, g, b, a))


def trim(img: Image.Image) -> Image.Image:
    """Crop tight to the alpha bounding box (NO square padding — otherwise the
    transparent bottom margin sits on the ground and the sprite appears to
    float). A 1px transparent border keeps NEAREST scaling edges clean."""
    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)
    canvas = Image.new("RGBA", (img.width + 2, img.height + 2), (0, 0, 0, 0))
    canvas.paste(img, (1, 1))
    return canvas


def pixelize(img: Image.Image, store_px: int) -> Image.Image:
    """Downscale to fit store_px on the longest side, PRESERVING aspect ratio
    so the renderer's bottom-align puts the creature's feet on the ground."""
    w, h = img.size
    scale = store_px / float(max(w, h))
    tw, th = max(1, round(w * scale)), max(1, round(h * scale))
    # Two-step: area-average to ~2x target (anti-alias), then nearest (crisp).
    if max(w, h) > max(tw, th) * 2:
        img = img.resize((tw * 2, th * 2), Image.LANCZOS)
    img = img.resize((tw, th), Image.NEAREST)
    # Harden alpha: anything semi-transparent from cutout becomes fully on/off.
    if img.mode == "RGBA":
        r, g, b, a = img.split()
        a = a.point(lambda v: 255 if v >= 128 else 0)
        img = Image.merge("RGBA", (r, g, b, a))
    return img


def process(raw_bytes: bytes, store_px: int, is_tile: bool, is_bg: bool = False) -> Image.Image:
    img = Image.open(io.BytesIO(raw_bytes)).convert("RGBA")
    if is_bg:
        # Background props: cut out the white, but KEEP soft alpha (glow/edges)
        # — no hard threshold, downscale smoothly.
        img = cutout(img)
        a = img.split()[3]
        bbox = a.point(lambda v: 255 if v > 20 else 0).getbbox()
        if bbox:
            img = img.crop(bbox)
        scale = store_px / float(max(img.size))
        tw, th = max(1, round(img.width * scale)), max(1, round(img.height * scale))
        return img.resize((tw, th), Image.LANCZOS)
    if is_tile:
        # Tiles must be a seamless edge-to-edge fill. Models frame the subject as a
        # centered object on a margin, so crop the central region (where the texture
        # is dense and uniform) before downscaling — avoids rounded/iso framing edges.
        w, h = img.size
        side = int(min(w, h) * 0.62)
        l = (w - side) // 2
        t = (h - side) // 2
        img = img.crop((l, t, l + side, t + side))
        img = img.resize((store_px * 2, store_px * 2), Image.LANCZOS)
        return img.resize((store_px, store_px), Image.NEAREST).convert("RGBA")
    img = cutout(img)
    img = harden_alpha(img)   # kill the rembg halo before measuring the bbox
    img = trim(img)
    return pixelize(img, store_px)


# ── atlas description (.t3s) ────────────────────────────────────

def write_t3s(names):
    """Emit a tex3ds atlas script listing processed PNGs in atlas-index order."""
    GFX_DIR.mkdir(parents=True, exist_ok=True)
    lines = ["-f rgba -z auto", "--atlas"]
    for n in names:
        # paths relative to the gfx/ dir, where the Makefile invokes tex3ds
        rel = os.path.relpath(PROCESSED / f"{n}.png", GFX_DIR)
        lines.append(rel)
    T3S_OUT.write_text("\n".join(lines) + "\n")
    print(f"  wrote {T3S_OUT.relative_to(REPO)}  ({len(names)} sub-images)")


def write_atlas_header(sprites):
    """Emit ATLAS_<NAME> -> sub-image index defines for the generated sprites,
    in the same order they appear in the .t3s (== C2D_SpriteSheetGetImage index)."""
    lines = [
        "#pragma once",
        "/* AUTO-GENERATED by tools/gen_sprites.py — do not edit by hand.",
        " * Sub-image indices into the citro2d sprite sheet (gfx/sprites.t3s).",
        " * Only AI-generated entity sprites live in the atlas; tiles stay procedural. */",
        "",
    ]
    for i, sp in enumerate(sprites):
        macro = "ATLAS_" + sp["name"].upper()
        lines.append(f"#define {macro:<24} {i}")
    lines.append("")
    lines.append(f"#define ATLAS_COUNT {len(sprites)}")
    lines.append("")
    ATLAS_H.write_text("\n".join(lines))
    print(f"  wrote {ATLAS_H.relative_to(REPO)}  ({len(sprites)} indices)")


def derive_durio_anim():
    """Build Durio's walk/jump frames procedurally from the canonical stand sprite.
    They are byte-identical to the stand except the leg band shifts, so the walk
    cycle is perfectly consistent (no AI frame-to-frame drift in color/proportion)."""
    stand_p = PROCESSED / "durio_stand.png"
    if not stand_p.exists():
        return
    st = Image.open(stand_p).convert("RGBA")
    w, h = st.size
    hip   = int(h * 0.56)                 # leg/boot band below this line
    upper = st.crop((0, 0, w, hip))
    band  = st.crop((0, hip, w, h))

    def legs(dx, dy):
        f = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        f.alpha_composite(upper, (0, 0))
        f.alpha_composite(band, (dx, hip + dy))
        return f

    legs(2, 0).save(PROCESSED / "durio_walk0.png")    # boots stepped right
    bob = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    bob.alpha_composite(st, (0, -1))                  # neutral, body bobbed up 1px
    bob.save(PROCESSED / "durio_walk1.png")
    legs(-2, 0).save(PROCESSED / "durio_walk2.png")   # boots stepped left
    legs(0, -3).save(PROCESSED / "durio_jump.png")    # boots tucked up
    print("  derived durio_walk0/1/2 + jump from stand (consistent)")


def write_contact(names):
    cols = 6
    cell = 72
    rows = (len(names) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cell, rows * cell), (40, 40, 60, 255))
    for i, n in enumerate(names):
        p = PROCESSED / f"{n}.png"
        if not p.exists():
            continue
        s = Image.open(p).convert("RGBA")
        s.thumbnail((cell - 8, cell - 8), Image.NEAREST)
        x = (i % cols) * cell + (cell - s.width) // 2
        y = (i // cols) * cell + (cell - s.height) // 2
        sheet.alpha_composite(s, (x, y))
    sheet.save(CONTACT)
    print(f"  wrote {CONTACT.relative_to(REPO)}")


# ── main ────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--force", action="store_true", help="regenerate even if cached")
    ap.add_argument("--only", default="", help="comma-separated sprite names to (re)generate")
    ap.add_argument("--no-generate", action="store_true", help="skip API; reprocess cache only")
    args = ap.parse_args()
    only = set(s.strip() for s in args.only.split(",") if s.strip())

    man = json.loads(MANIFEST.read_text())
    model = man["model"]
    base_inputs = man.get("model_inputs", {})
    preamble = man["style_preamble"]
    sprites = man["sprites"]

    CACHE_RAW.mkdir(parents=True, exist_ok=True)
    PROCESSED.mkdir(parents=True, exist_ok=True)

    # Tiles stay procedural (draw_tile in render_3ds.c): AI can't make seamless
    # edge-to-edge block fills reliably. Only entity sprites go into the atlas.
    gen_sprites = [sp for sp in sprites if not sp.get("tile")]

    for sp in gen_sprites:
        name = sp["name"]
        if only and name not in only:
            continue
        raw_path = CACHE_RAW / f"{name}.png"
        store_px = sp.get("store_px", man.get("defaults", {}).get("store_px", 64))
        is_tile = bool(sp.get("tile"))
        is_bg   = bool(sp.get("bg"))

        if not args.no_generate and (args.force or not raw_path.exists()):
            full_prompt = f"{sp['prompt']}. {preamble}"
            print(f"[gen] {name} ({sp['id']})")
            try:
                raw = generate(model, base_inputs, full_prompt)
                raw_path.write_bytes(raw)
            except Exception as e:
                print(f"  !! generation failed: {e}")
                continue
        if not raw_path.exists():
            print(f"  -- {name}: no cached raw, skipping")
            continue

        print(f"[proc] {name}  store={store_px}px")
        out = process(raw_path.read_bytes(), store_px, is_tile, is_bg)
        out.save(PROCESSED / f"{name}.png")

    derive_durio_anim()   # override Durio walk/jump with consistent procedural frames

    names = [sp["name"] for sp in gen_sprites]
    write_t3s(names)
    write_atlas_header(gen_sprites)
    write_contact(names)
    print(f"done. {len(names)} entity sprites in atlas; tiles stay procedural.")


if __name__ == "__main__":
    main()
