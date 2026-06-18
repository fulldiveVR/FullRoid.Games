#!/usr/bin/env python3
"""
Generate CONSISTENT animation frames by image-to-image editing a canonical frame
with flux-kontext-pro (identity-preserving). Each derived frame is edited FROM
the same source sprite, so colors/design stay matched across the walk cycle —
unlike independent text-to-image generations which drift.

Outputs overwrite the raw cache (tools/sprite_cache/raw/<name>.png); afterwards
run `python3 tools/gen_sprites.py --no-generate` to re-process + repack the atlas.
"""
import base64
import io
import os
import sys
import time
from pathlib import Path

import requests
from PIL import Image

REPO = Path(__file__).resolve().parent.parent
CACHE_RAW = REPO / "tools" / "sprite_cache" / "raw"
API = "https://api.replicate.com/v1"
MODEL = "black-forest-labs/flux-kontext-pro"

STYLE = ("Keep the EXACT same character, identical colors, shapes, proportions and "
         "pixel-art style. Only change the pose as described. Side view, centered, "
         "full body in frame, plain flat solid white background, no text.")

# name -> (source frame in cache, edit prompt)
FRAMES = {
    "durio_walk0": ("durio_stand",
        "Show this little alien space explorer WALKING: one orange rocket boot "
        "stepped forward and the other back, mid-stride, slight forward lean."),
    "durio_walk1": ("durio_stand",
        "Show this little alien space explorer WALKING: both legs together in the "
        "neutral contact pose, body bobbed slightly up."),
    "durio_walk2": ("durio_stand",
        "Show this little alien space explorer WALKING: the OPPOSITE mid-stride, the "
        "other orange rocket boot forward."),
    "durio_jump": ("durio_stand",
        "Show this little alien space explorer JUMPING upward: legs tucked up, arms "
        "slightly raised, small flame bursts from the orange rocket boots."),
    "durio_dead": ("durio_stand",
        "Show this little alien space explorer DEFEATED: tipped over backwards, dizzy "
        "X eyes on the visor, cracked dim helmet."),
    "crab_walk1": ("crab_walk0",
        "Show this alien crab enemy with its spiky legs in the OPPOSITE walking "
        "position and pincer claws lowered slightly."),
    "snail_walk1": ("snail_walk0",
        "Show this alien snail enemy with its body slightly more contracted toward "
        "the shell, eye-stalks shorter."),
}


def token():
    t = os.environ.get("REPLICATE_API_TOKEN")
    if not t:
        sys.exit("REPLICATE_API_TOKEN not set")
    return t


def data_uri(path: Path) -> str:
    b = path.read_bytes()
    return "data:image/png;base64," + base64.b64encode(b).decode()


def edit(src_uri, prompt):
    owner, name = MODEL.split("/")
    headers = {"Authorization": f"Bearer {token()}",
               "Content-Type": "application/json", "Prefer": "wait"}
    body = {"input": {"prompt": f"{prompt} {STYLE}",
                      "input_image": src_uri, "output_format": "png"}}
    r = requests.post(f"{API}/models/{owner}/{name}/predictions",
                      headers=headers, json=body, timeout=180)
    r.raise_for_status()
    pred = r.json()
    poll = pred.get("urls", {}).get("get")
    while pred.get("status") not in ("succeeded", "failed", "canceled"):
        time.sleep(2)
        pred = requests.get(poll, headers=headers, timeout=60).json()
    if pred.get("status") != "succeeded":
        raise RuntimeError(f"{pred.get('status')}: {pred.get('error')}")
    out = pred["output"]
    url = out[0] if isinstance(out, list) else out
    img = requests.get(url, timeout=180)
    img.raise_for_status()
    return img.content


def main():
    only = set(a for a in sys.argv[1:] if not a.startswith("-"))
    for name, (src, prompt) in FRAMES.items():
        if only and name not in only:
            continue
        src_path = CACHE_RAW / f"{src}.png"
        if not src_path.exists():
            print(f"  !! source {src} missing, skip {name}")
            continue
        print(f"[kontext] {name}  <- {src}")
        try:
            raw = edit(data_uri(src_path), prompt)
            # sanity: ensure it's a valid image
            Image.open(io.BytesIO(raw)).verify()
            (CACHE_RAW / f"{name}.png").write_bytes(raw)
        except Exception as e:
            print(f"  !! failed: {e}")
    print("done. now run: python3 tools/gen_sprites.py --no-generate")


if __name__ == "__main__":
    main()
