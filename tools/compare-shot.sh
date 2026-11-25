#!/bin/sh
# Compares the drawing of a scripted run of tst/orig by the port with the
# patched original, at the given logic frames: the map and its objects,
# with no menus (FLARE_NOHUD) unless HUD=1, and no hero unless HERO=1
# (FLARE_NOHERO). REGION="x0 y0 x1 y1" compares only that rectangle (x1 and
# y1 excluded), for the HUD drawn piece by piece. Prints, per frame, "same"
# or the number of differing pixels and their bounds.
#
#   [HERO=1] [HUD=1] [REGION="x0 y0 x1 y1"] tools/compare-shot.sh <run> <frame> [frame ...]

ROOT=$(cd "$(dirname "$0")/.." && pwd)
N=$1
shift
ENV=$(sed -n '1s/^# env: //p' "$ROOT/tst/orig/$N.in" | tr ' ' '\n' | grep -v '^FLARE_FRAMES=')
NOHERO="FLARE_NOHERO=1"
[ -n "$HERO" ] && NOHERO=""
NOHUD="FLARE_NOHUD=1"
[ -n "$HUD" ] && NOHUD=""
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0
for f in "$@"; do
	env $ENV FLARE_INPUT="$ROOT/tst/orig/$N.in" $NOHUD $NOHERO \
		FLARE_FRAMES=$((f+1)) FLARE_SHOT=$f:"$TMP/orig.bmp" "$ROOT/tools/bench/run.sh" > /dev/null 2>&1
	env SDL_VIDEODRIVER=dummy FLARE_MUTE=1 FLARE_FAST=1 FLARE_SEED=1 $ENV FLARE_INPUT="$ROOT/tst/orig/$N.in" \
		$NOHUD $NOHERO FLARE_FRAMES=$((f+1)) FLARE_SHOT=$f:"$TMP/port.bmp" "$ROOT/run.sh" main.atm > /dev/null 2>&1
	if python3 -c "
from PIL import Image
import numpy as np, sys
a = np.asarray(Image.open('$TMP/orig.bmp').convert('RGB')); b = np.asarray(Image.open('$TMP/port.bmp').convert('RGB'))
r = '$REGION'.split()
if r: x0, y0, x1, y1 = map(int, r); a, b = a[y0:y1, x0:x1], b[y0:y1, x0:x1]
sys.exit(0 if (a == b).all() else 1)"; then
		echo "same  $N $f"
	else
		echo "DIFF  $N $f $(python3 -c "
from PIL import Image, ImageChops
import numpy as np
a = Image.open('$TMP/orig.bmp').convert('RGB'); b = Image.open('$TMP/port.bmp').convert('RGB')
x = np.asarray(ImageChops.difference(a, b)).sum(axis=2)
r = '$REGION'.split()
if r: x0, y0, x1, y1 = map(int, r); x = x[y0:y1, x0:x1]
ys, xs = np.nonzero(x)
print((x > 0).sum(), 'pixels in', xs.min(), ys.min(), xs.max(), ys.max())" 2>&1)"
		FAIL=1
	fi
done
exit $FAIL
