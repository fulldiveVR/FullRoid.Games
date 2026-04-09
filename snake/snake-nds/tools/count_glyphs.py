#!/usr/bin/env python3
"""Count unique Unicode codepoints needed from i18n string files."""
import re, sys

chars = set()

for path in sys.argv[1:]:
    with open(path, "rb") as f:
        raw = f.read()
    # Extract bytes between C string quotes, decode hex escapes
    in_str = False
    buf = bytearray()
    i = 0
    while i < len(raw):
        b = raw[i]
        if b == ord('"') and (i == 0 or raw[i-1] != ord('\\')):
            if in_str:
                try:
                    for c in buf.decode("utf-8", errors="ignore"):
                        cp = ord(c)
                        if cp >= 0x20:
                            chars.add(cp)
                except:
                    pass
                buf = bytearray()
            in_str = not in_str
        elif in_str:
            if b == ord('\\') and i+1 < len(raw):
                nb = raw[i+1]
                if nb == ord('x') and i+3 < len(raw):
                    try:
                        buf.append(int(raw[i+2:i+4].decode("ascii"), 16))
                    except:
                        pass
                    i += 3
                elif nb == ord('n'):
                    i += 1
                elif nb == ord('"'):
                    buf.append(ord('"'))
                    i += 1
                else:
                    i += 1
            else:
                buf.append(b)
        i += 1

# Full printable ASCII
for c in range(0x20, 0x7F):
    chars.add(c)

chars = sorted(chars)
print(f"Total: {len(chars)}")

blocks = {}
for cp in chars:
    if cp < 0x80: b = "ASCII"
    elif cp < 0x250: b = "Latin Ext"
    elif 0x370 <= cp < 0x400: b = "Greek"
    elif 0x400 <= cp < 0x500: b = "Cyrillic"
    elif 0x1E00 <= cp < 0x1F00: b = "Vietnamese"
    else: b = f"Other(U+{cp:04X})"
    blocks[b] = blocks.get(b, 0) + 1

for k, v in blocks.items():
    print(f"  {k}: {v}")

# Print non-ASCII
for cp in chars:
    if cp >= 0x80:
        print(f"  U+{cp:04X} {chr(cp)}")
