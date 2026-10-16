"""Find the Xbox 360 D3D swap-queue function in ReXGlue-generated code.

The function that queues a swap decodes a "swap word" near its start: bits 8-11 are the
presentation interval (vblanks per frame, 2 = 30 FPS), the low byte is a late-frame threshold.
That shows up as `rlwinm rX,rY,24,28,31` and `clrlwi rZ,rY,24` within the first instructions.
In Swap Force this matches exactly one function, sub_82F5C258; a midasm hook there can override the
interval (see docs/high-framerate.md). Usage:

    python tools/find_swap_interval.py [generated/default]
"""
import pathlib
import re
import sys

FUNC_RE = re.compile(r"^DEFINE_REX_FUNC\((sub_[0-9A-F]{8})\)")
INTERVAL_RE = re.compile(r"// rlwinm r\d+,r\d+,24,28,31$")
THRESHOLD_RE = re.compile(r"// clrlwi r\d+,r\d+,24$")
WINDOW = 40  # instructions from the function start


def scan(gen_dir: pathlib.Path):
    hits = []
    for path in sorted(gen_dir.glob("*.cpp")):
        func, count, has_interval, has_threshold = None, 0, False, False
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            m = FUNC_RE.match(line)
            if m:
                if func and has_interval and has_threshold:
                    hits.append(func)
                func, count, has_interval, has_threshold = m.group(1), 0, False, False
                continue
            if line.startswith("\t// "):
                count += 1
                if count <= WINDOW:
                    has_interval |= bool(INTERVAL_RE.search(line))
                    has_threshold |= bool(THRESHOLD_RE.search(line))
        if func and has_interval and has_threshold:
            hits.append(func)
    return hits


def main():
    gen_dir = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "generated/default")
    hits = scan(gen_dir)
    if not hits:
        print("no candidate found")
        return 1
    for h in hits:
        print(h, "(hook its start address with a midasm hook on r3 to override bits 8-11)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
