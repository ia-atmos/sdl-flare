#!/usr/bin/env python3
# The md5 of the pixels (RGB, row by row) of a screenshot, whatever its
# format (the BMP of the original, the PNG of env-sdl).
#
#   tools/shot-md5.py <image>
import hashlib, sys
from PIL import Image
print(hashlib.md5(Image.open(sys.argv[1]).convert('RGB').tobytes()).hexdigest())
