#!/bin/sh
# Builds and runs a program of the port with the Atmos compiler.
#
#   ./run.sh [file.atm] [args]      (default: main.atm)
#
# Environment:
#   ATMOS        the compiler, ia-atmos/lang (default: ../lang/atmos,
#                ../ceu-atmos/atmos, its former name, or `atmos`)
#   ATMOS_ASAN=1 builds with the address sanitizer
#   FLARE_DATA   the directory with the folder `mods` of the original
#                (default: ../flare-orig/data, made by tools/bench/setup.sh)
#   ENV_SDL      the checkout of ia-atmos/env-sdl, at the commit of
#                env-sdl.txt (default: ../env-sdl, or ../ia-atmos/env-sdl),
#                linked as ./env
#
# The test variables of the patched original are converted to those of
# env-sdl: FLARE_FRAMES, FLARE_SHOT, FLARE_INPUT (tools/env-input.sh), and
# FLARE_MUTE. FLARE_SEED and FLARE_TRACE are the port's. FLARE_FAST is the
# test mode (no sleep between frames), on with FLARE_FRAMES.

ROOT=$(cd "$(dirname "$0")" && pwd)

if [ -z "$ATMOS" ]; then
	if [ -x "$ROOT/../lang/atmos" ]; then
		ATMOS="$ROOT/../lang/atmos"
	elif [ -x "$ROOT/../ceu-atmos/atmos" ]; then
		ATMOS="$ROOT/../ceu-atmos/atmos"
	else
		ATMOS=atmos
	fi
fi

export FLARE_DATA="${FLARE_DATA:-$ROOT/../flare-orig/data}"

if [ -z "$ENV_SDL" ]; then
	if [ -f "$ROOT/../env-sdl/sdl.atm" ]; then
		ENV_SDL="$ROOT/../env-sdl"
	else
		ENV_SDL="$ROOT/../ia-atmos/env-sdl"
	fi
fi
ENV_SDL=$(cd "$ENV_SDL" && pwd) || exit 1
WANT=$(sed -n '1p' "$ROOT/env-sdl.txt")
HAVE=$(git -C "$ENV_SDL" rev-parse HEAD 2>/dev/null)
case "$HAVE" in
	"$WANT"*) ;;
	*) echo "run.sh : env-sdl at ${HAVE:-?}, not at $WANT (env-sdl.txt)" >&2 ;;
esac
[ "$(readlink "$ROOT/env" 2>/dev/null)" = "$ENV_SDL" ] || ln -sfn "$ENV_SDL" "$ROOT/env"

TMPIN=""
if [ -n "$FLARE_FRAMES" ]; then
	export SDL_ATM_FRAMES="$FLARE_FRAMES"
fi
if [ -n "$FLARE_SHOT" ]; then
	export SDL_ATM_SHOT="$FLARE_SHOT"
fi
if [ -n "$FLARE_MUTE" ]; then
	export SDL_ATM_MUTE=1
fi
if [ -n "$FLARE_INPUT" ]; then
	TMPIN=$(mktemp)
	"$ROOT/tools/env-input.sh" "$FLARE_INPUT" "$TMPIN"
	export SDL_ATM_KEYS="@$TMPIN.keys" SDL_ATM_MOUSE="@$TMPIN.mouse"
fi

FILE=${1:-main.atm}
[ $# -gt 0 ] && shift

export ATMOS_CFLAGS="${ATMOS_CFLAGS:--O2} -I$ROOT"
export ATMOS_LDFLAGS="-lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lm"

cd "$ROOT" && "$ATMOS" "$FILE" "$@"
RET=$?
[ -n "$TMPIN" ] && rm -f "$TMPIN" "$TMPIN.keys" "$TMPIN.mouse"
exit $RET
