#!/usr/bin/env python3
"""Turn the drawings in assets/src into the pictures the screensaver builds in,
under assets/art.

    tools/make_sprites.py

worlds.jpeg is a sheet: six worlds in glass on one flat magenta background, not
touching, read off in rows, left to right. sky.jpeg is the background. Needs
ImageMagick and numpy.
"""
import subprocess
import numpy as np

SRC, OUT = "assets/src", "assets/art"
def load(path):
    w, h = map(int, subprocess.check_output(["magick", "identify", "-format", "%w %h", path]).split())
    raw = subprocess.check_output(["magick", path, "-depth", "8", "RGB:-"])
    return np.frombuffer(raw, np.uint8).reshape(h, w, 3).astype(np.float32)

def key(img):
    """Magenta background to alpha, and take its colour cast off the edges."""
    r, g, b = img[..., 0].copy(), img[..., 1].copy(), img[..., 2].copy()
    spill = np.minimum(r, b) - g
    cut = np.clip(spill, 0, None) * 0.92
    r, b = r - cut, b - cut
    alpha = np.clip(1.0 - (spill - 24) / 70.0, 0, 1)
    return np.dstack([r, g, b, alpha * 255]).clip(0, 255).astype(np.uint8)

def runs(on, gap):
    """Stretches where `on` is true, joining ones separated by less than `gap`."""
    out, start, last = [], None, None
    for i, v in enumerate(on):
        if not v: continue
        if start is None: start = i
        elif i - last > gap: out.append((start, last + 1)); start = i
        last = i
    if start is not None: out.append((start, last + 1))
    return [r for r in out if r[1] - r[0] > 30]

def pieces(a, rows):
    """The separate drawings on a sheet, in reading order: split into rows by the
    empty bands between them, then each row into drawings by the empty gaps."""
    solid = a[..., 3] > 128
    bands = runs(solid.sum(axis=1) > 2, 40)
    if len(bands) != rows:
        raise SystemExit(f"found {len(bands)} rows of drawings, expected {rows}")
    out = []
    for y0, y1 in bands:
        for x0, x1 in runs(solid[y0:y1].sum(axis=0) > 2, 40):
            out.append(a[y0:y1, x0:x1])
    return out

def trim(a):
    solid = a[..., 3] > 128
    rows = np.where(solid.sum(axis=1) > 2)[0]; cols = np.where(solid.sum(axis=0) > 2)[0]
    return a[rows.min():rows.max() + 1, cols.min():cols.max() + 1]

def save(rgba, w, h, path):
    subprocess.run(["magick", "-size", f"{rgba.shape[1]}x{rgba.shape[0]}", "-depth", "8", "RGBA:-",
                    "-filter", "Lanczos", "-resize", f"{w}x{h}!", "-define", "png:compression-level=9", path],
                   input=np.ascontiguousarray(rgba).tobytes(), check=True)

def main():
    subprocess.run(["mkdir", "-p", OUT], check=True)
    got = [trim(p) for p in pieces(key(load(f"{SRC}/worlds.jpeg")), 2)]
    if len(got) != 6:
        raise SystemExit(f"worlds: found {len(got)} drawings, expected 6")
    for i, p in enumerate(got):  # 0 the world of pure forms, 1 Arbre, 2 Urnud, 3 Tro, 4 Laterre, 5 Fthos
        save(p, 360, max(1, round(p.shape[0] * 360 / p.shape[1])), f"{OUT}/w{i}.png")
    print("worlds")
    subprocess.run(["magick", f"{SRC}/sky.jpeg", "-resize", "1920x1080!", "-quality", "90", f"{OUT}/sky.jpg"], check=True)
    print("sky")

if __name__ == "__main__":
    main()
