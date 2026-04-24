"""
frame_variance.py — compute per-pixel temporal variance across a sequence of
gpu_output_NNNN.png frames dumped by -dump_frames, and write a heatmap PNG
highlighting regions that flicker most. Usage:
    python frame_variance.py <dir> <out_png>
where <dir> contains gpu_output_NNNN.png files (any contiguous numbering).
"""
import os
import sys
from PIL import Image
import numpy as np

def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    src_dir = sys.argv[1]
    out_path = sys.argv[2]

    files = sorted(
        f for f in os.listdir(src_dir)
        if f.startswith("gpu_output_") and f.endswith(".png")
    )
    if len(files) < 2:
        print(f"need >=2 frames, found {len(files)} in {src_dir}", file=sys.stderr)
        sys.exit(1)

    stack = []
    for f in files:
        im = Image.open(os.path.join(src_dir, f)).convert("RGB")
        stack.append(np.asarray(im, dtype=np.float32))
    arr = np.stack(stack)  # [N, H, W, 3]
    N = arr.shape[0]

    # Per-pixel luma, then temporal std over frames
    luma = 0.2126 * arr[..., 0] + 0.7152 * arr[..., 1] + 0.0722 * arr[..., 2]
    std = luma.std(axis=0)  # [H, W], in 0..255 scale

    # Report top bucket of temporal std
    p50 = float(np.percentile(std, 50))
    p95 = float(np.percentile(std, 95))
    p99 = float(np.percentile(std, 99))
    sMax = float(std.max())
    print(f"frames: {N}  resolution: {arr.shape[2]}x{arr.shape[1]}")
    print(f"temporal-std percentiles (luma 0..255):")
    print(f"  p50 = {p50:.2f}   p95 = {p95:.2f}   p99 = {p99:.2f}   max = {sMax:.2f}")

    # Locate region of highest std — report centroid of top 1% cells
    thr = p99
    mask = std >= thr
    ys, xs = np.where(mask)
    if len(xs) > 0:
        cy = ys.mean() / std.shape[0]
        cx = xs.mean() / std.shape[1]
        print(f"top-1% flicker centroid (normalised): x={cx:.2f} y={cy:.2f}  "
              f"(x=0 left, y=0 top)")
        # Quadrant breakdown
        H, W = std.shape
        q_tl = int(((ys < H//2) & (xs < W//2)).sum())
        q_tr = int(((ys < H//2) & (xs >= W//2)).sum())
        q_bl = int(((ys >= H//2) & (xs < W//2)).sum())
        q_br = int(((ys >= H//2) & (xs >= W//2)).sum())
        tot = max(1, q_tl + q_tr + q_bl + q_br)
        print(f"top-1% flicker quadrants: TL={q_tl/tot:.0%} "
              f"TR={q_tr/tot:.0%} BL={q_bl/tot:.0%} BR={q_br/tot:.0%}")

    # Write heatmap — log-scale so medium-std pixels are still visible
    heat = np.log1p(std)
    heat = heat / max(heat.max(), 1e-6)
    heat_rgb = np.stack([heat, np.zeros_like(heat), 1.0 - heat], axis=-1)
    heat_u8 = (heat_rgb * 255).clip(0, 255).astype(np.uint8)
    Image.fromarray(heat_u8).save(out_path)
    print(f"heatmap -> {out_path}  (red = high temporal std, blue = stable)")


if __name__ == "__main__":
    main()
