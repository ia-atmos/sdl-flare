#!/bin/sh
# Converts a scripted input of the patched original (FLARE_INPUT: lines
# "<frame> key <name> | keyup <name> | move <x> <y> | down <b> | up <b>",
# frames from 0) to the test mode of env-sdl, whose frames count from 1:
# writes <out>.keys (SDL_ATM_KEYS) and <out>.mouse (SDL_ATM_MOUSE), one
# entry per line, in the order of the script.
#
#   tools/env-input.sh <input> <out>

awk -v K="$2.keys" -v M="$2.mouse" '
BEGIN { x = 0; y = 0; printf "" > K; printf "" > M }
/^#/ || NF < 2 { next }
{
    f = $1 + 1
    if ($2 == "key")        print f ":dn:" $3 > K
    else if ($2 == "keyup") print f ":up:" $3 > K
    else if ($2 == "move")  { x = $3; y = $4; print f ":mv:" x ":" y > M }
    else if ($2 == "down")  print f ":dn:" x ":" y ":" $3 > M
    else if ($2 == "up")    print f ":up:" x ":" y ":" $3 > M
}' "$1"
