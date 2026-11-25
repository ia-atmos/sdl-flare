// The C side of the engine of the port of Flare.
//
// It is included by `engine/engine.atm`, in a prelude, after the
// environment `env/sdl.atm` (ia-atmos/env-sdl), which holds the window,
// the loop of frames, the events, the sounds, and the test mode. This
// file holds what the game needs beyond it, and what works per pixel:
//
//   - the generator of random numbers of the patched original;
//   - the trace;
//   - the table of images drawn from C (the map, the renders), over the
//     images of the environment, blitted onto its screen surface (the
//     software path: ARGB8888 surfaces, as the software renderer of the
//     original, SDLSoftwareRenderDevice, so that the pixels are the same);
//   - fonts (TrueType) and texts, as SDLFontEngine;
//   - the state of the input of the original (InputState).
//
// Values cross to Atmos as numbers: an image or a font is an index in a
// table of this file, which lives until the program ends.
//
// Coordinates are those of the render, 1706x960 for a window of
// 1280x720, as the original computes them (`windowResizeInternal`).
//
// The test mode is that of the environment; `run.sh` converts the
// variables of the patched original to it (see `doc/fidelity.md`), but
// FLARE_SEED and FLARE_TRACE, which are read here.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#define FL_SCREEN sdl_scr()              // the render, at FL_W x FL_H
static int           FL_W   = 1706;
static int           FL_H   = 960;
static unsigned long FL_FRAME = 0;      // logic frames done

static int fl_int (AVal v) {
    return a_isnum(v) ? (int)a_dbl(v) : 0;
}

static const char* fl_cstr (AVal v) {
    return (v.type == A_STR) ? A_STR_OF(v)->s : "";
}

static const char* fl_env (const char* name) {
    const char* v = getenv(name);
    return (v != NULL && *v != '\0') ? v : NULL;
}

///////////////////////////////////////////////////////////////////////////////
// RANDOM: the generator of the patched original (`Bench.h`)
///////////////////////////////////////////////////////////////////////////////

static uint64_t FL_RND = 0;
static unsigned long FL_RND_CALLS = 0;      // since the last count (FLARE_RNG)

// from 0 to 2^31-1, as rand() of glibc
static int fl_rand (void) {
    FL_RND_CALLS++;
    FL_RND = FL_RND * 6364136223846793005ULL + 1442695040888963407ULL;
    return (int)(FL_RND >> 33);
}

///////////////////////////////////////////////////////////////////////////////
// FILES
///////////////////////////////////////////////////////////////////////////////

// The names of the files of a directory that end in `ext`, in the order
// of readdir, as Filesystem::getFileList: a vector, or nil.
static AVal fl_dir (const char* path, const char* ext) {
    DIR* dp = opendir(path);
    if (dp == NULL) {
        return a_nil();
    }
    AVal ret = a_tab();
    size_t n = strlen(ext);
    struct dirent* d;
    while ((d = readdir(dp)) != NULL) {
        size_t m = strlen(d->d_name);
        if (m > n && strcmp(d->d_name + m - n, ext) == 0) {
            AVal s = a_strz(d->d_name);
            a_setindex_next(ret, s);
            a_dec(s);
        }
    }
    closedir(dp);
    return ret;
}

///////////////////////////////////////////////////////////////////////////////
// TRACE
///////////////////////////////////////////////////////////////////////////////

static FILE* FL_TRACE = NULL;

// one line: the logic frame, then the text
static void fl_trace (const char* s) {
    if (FL_TRACE != NULL) {
        fprintf(FL_TRACE, "%lu %s\n", FL_FRAME, s);
        fflush(FL_TRACE);
    }
}

///////////////////////////////////////////////////////////////////////////////
// IMAGES
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    char*        path;
    int          raw;
    SDL_Surface* tex;
    int          w, h;
} FlImg;

#define FL_IMGS_MAX 4096
static FlImg FL_IMGS[FL_IMGS_MAX];
static int   FL_IMGS_N = 0;

// The index of an image of a path, or -1 if it failed to load, or -2 if
// it was not loaded. A raw image keeps the format of IMG_Load, as the
// images of animations of the original (RenderDevice::loadQueuedImages),
// which SDL blends differently from the ARGB8888 of the others
// (SDLSoftwareRenderDevice::loadImage).
static int fl_img_find (const char* path, int raw) {
    for (int i=0; i<FL_IMGS_N; i++) {
        if (strcmp(FL_IMGS[i].path, path) == 0 && FL_IMGS[i].raw == raw) {
            return FL_IMGS[i].tex ? i : -1;
        }
    }
    return -2;
}

// Adds an image of the environment (or nil, a failed load), which the
// caller keeps alive: its index, or -1. sdl_sfc gives it converted to
// ARGB8888, unless raw.
static int fl_img_add (AVal v, const char* path, int raw) {
    if (FL_IMGS_N == FL_IMGS_MAX) {
        return -1;
    }
    FlImg* img = &FL_IMGS[FL_IMGS_N];
    img->path = strdup(path);
    img->raw = raw;
    img->tex = NULL;
    img->w = img->h = 0;
    if (v.type == A_NIL) {
        fprintf(stderr, "flare : image : %s : %s\n", path, IMG_GetError());
    } else {
        img->tex = sdl_sfc(v);
        img->w = img->tex->w;
        img->h = img->tex->h;
    }
    return (img->tex != NULL) ? FL_IMGS_N++ : (FL_IMGS_N++, -1);
}

static FlImg* fl_img (int i) {
    return (i >= 0 && i < FL_IMGS_N && FL_IMGS[i].tex) ? &FL_IMGS[i] : NULL;
}

// Draws the part `sx,sy,sw,sh` of an image at `dx,dy`, with the same
// size (or the whole image if `sw` is 0), with an alpha, a color mod
// (0xRRGGBB), and a blend mode (0 normal, 1 add).
static void fl_img_draw (int i, int sx, int sy, int sw, int sh, int dx, int dy,
                         int alpha, int color, int blend) {
    FlImg* img = fl_img(i);
    if (img == NULL) {
        return;
    }
    if (sw == 0) {
        sw = img->w;
        sh = img->h;
    }
    SDL_Rect src = { sx, sy, sw, sh };
    SDL_Rect dst = { dx, dy, sw, sh };
    SDL_SetSurfaceBlendMode(img->tex, blend ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceColorMod(img->tex, (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF);
    SDL_SetSurfaceAlphaMod(img->tex, (Uint8)alpha);
    SDL_BlitSurface(img->tex, &src, FL_SCREEN, &dst);
}

///////////////////////////////////////////////////////////////////////////////
// SHAPES
///////////////////////////////////////////////////////////////////////////////

// fills a rectangle with a color 0xRRGGBBAA, with no blending, as SDL_FillRect
static void fl_fill (int64_t rgba, int x, int y, int w, int h) {
    SDL_Rect r = { x, y, w, h };
    SDL_FillRect(FL_SCREEN, &r, SDL_MapRGBA(FL_SCREEN->format,
        (rgba>>24)&0xFF, (rgba>>16)&0xFF, (rgba>>8)&0xFF, rgba&0xFF));
}

///////////////////////////////////////////////////////////////////////////////
// FONTS
///////////////////////////////////////////////////////////////////////////////

#define FL_FONTS_MAX 64
static TTF_Font* FL_FONTS[FL_FONTS_MAX];
static int       FL_FONTS_N = 0;

// A font of a path, a size, and a style (TTF_STYLE_*), or -1.
static int fl_font_load (const char* path, int size, int style) {
    if (FL_FONTS_N == FL_FONTS_MAX) {
        return -1;
    }
    TTF_Font* f = TTF_OpenFont(path, size);
    if (f == NULL) {
        fprintf(stderr, "flare : font : %s : %s\n", path, TTF_GetError());
        return -1;
    }
    TTF_SetFontStyle(f, style);
    FL_FONTS[FL_FONTS_N] = f;
    return FL_FONTS_N++;
}

static int fl_font_width (int fi, const char* s) {
    int w = 0, h = 0;
    if (fi >= 0 && fi < FL_FONTS_N && *s) {
        TTF_SizeUTF8(FL_FONTS[fi], s, &w, &h);
    }
    return w;
}

static int fl_font_height (int fi) {
    return (fi >= 0 && fi < FL_FONTS_N) ? TTF_FontLineSkip(FL_FONTS[fi]) : 0;
}

// Draws a text at x,y, justified (0 left, 1 right, 2 center: x is the
// left, the right, or the center), with a color 0xRRGGBB, blended or
// solid, as SDLFontEngine::renderInternal: twice, since blending the text
// with itself makes it clearer.
static void fl_font_draw (int fi, const char* s, int x, int y, int justify, int color, int blend) {
    if (fi < 0 || fi >= FL_FONTS_N || *s == '\0') {
        return;
    }
    if (justify == 1) {
        x -= fl_font_width(fi, s);
    } else if (justify == 2) {
        x -= fl_font_width(fi, s) / 2;
    }
    SDL_Color c = { (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF, 255 };
    SDL_Surface* sfc = blend ? TTF_RenderUTF8_Blended(FL_FONTS[fi], s, c)
                             : TTF_RenderUTF8_Solid(FL_FONTS[fi], s, c);
    if (sfc == NULL) {
        return;
    }
    SDL_Rect dst = { x, y, sfc->w, sfc->h };
    SDL_BlitSurface(sfc, NULL, FL_SCREEN, &dst);
    dst.x = x; dst.y = y; dst.w = sfc->w; dst.h = sfc->h;
    SDL_BlitSurface(sfc, NULL, FL_SCREEN, &dst);
    SDL_FreeSurface(sfc);
}

// A label, as WidgetLabel::recacheTextSprite and render: the text in an
// image of its size (the width of the text, the height of the text or of
// the line, if more), cleared (SDLSoftwareRenderDevice::createImage),
// with its shadow in black at an offset and then the text, each blitted
// twice (FontEngine::renderShadowed, SDLFontEngine::renderInternal); then
// the image at x,y, with an alpha (WidgetLabel::setAlpha).
static void fl_label (int fi, const char* s, int x, int y, int color, int sx, int sy, int blend, int alpha) {
    if (fi < 0 || fi >= FL_FONTS_N || *s == '\0') {
        return;
    }
    int w = 0, h = 0;
    TTF_SizeUTF8(FL_FONTS[fi], s, &w, &h);
    int lh = TTF_FontLineSkip(FL_FONTS[fi]);
    if (lh > h) {
        h = lh;
    }
    SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (img == NULL) {
        return;
    }
    for (int k=0; k<2; k++) {
        SDL_Color c = (k == 0) ? (SDL_Color){ 0, 0, 0, 255 }
                               : (SDL_Color){ (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF, 255 };
        SDL_Surface* t = blend ? TTF_RenderUTF8_Blended(FL_FONTS[fi], s, c)
                               : TTF_RenderUTF8_Solid(FL_FONTS[fi], s, c);
        if (t != NULL) {
            for (int j=0; j<2; j++) {
                SDL_Rect d = { (k == 0) ? sx : 0, (k == 0) ? sy : 0, t->w, t->h };
                SDL_BlitSurface(t, NULL, img, &d);
            }
            SDL_FreeSurface(t);
        }
    }
    SDL_Rect dst = { x, y, w, h };
    SDL_SetSurfaceBlendMode(img, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceAlphaMod(img, (Uint8)alpha);
    SDL_BlitSurface(img, NULL, FL_SCREEN, &dst);
    SDL_FreeSurface(img);
}

// The lines of a text wrapped to a width, as FontEngine::render renders
// them (with their trailing spaces), and its size as calcSizeWrapped gives
// it (no newlines in the text).
#define FL_WRAP_MAX 64
#define FL_WRAP_LEN 1024
static char FL_WRAP[FL_WRAP_MAX][FL_WRAP_LEN];
static int  FL_WRAP_N = 0, FL_WRAP_W = 0, FL_WRAP_H = 0;

static int fl_wrap_width (TTF_Font* f, const char* s) {
    int w = 0, h = 0;
    if (*s) TTF_SizeUTF8(f, s, &w, &h);
    return w;
}

static void fl_wrap_line (const char* s) {
    if (FL_WRAP_N < FL_WRAP_MAX) {
        snprintf(FL_WRAP[FL_WRAP_N++], FL_WRAP_LEN, "%s", s);
    }
}

// FontEngine::popTokenByWidth: `word` keeps what fits, `rest` the rest
// (or `word` itself if nothing fits)
static void fl_wrap_pop (TTF_Font* f, char* word, char* rest, int width) {
    size_t n = strlen(word), keep = 0;
    char tmp[FL_WRAP_LEN];
    for (size_t i=0; i<=n; i++) {
        if (i < n && (word[i] & 0xc0) == 0x80) continue;
        memcpy(tmp, word, i); tmp[i] = '\0';
        if (fl_wrap_width(f, tmp) > width) break;
        keep = i;
    }
    if (keep > 0) {
        strcpy(rest, word + keep);
        word[keep] = '\0';
    } else {
        strcpy(rest, word);
    }
}

static void fl_wrap (int fi, const char* text, int width) {
    FL_WRAP_N = FL_WRAP_W = FL_WRAP_H = 0;
    if (fi < 0 || fi >= FL_FONTS_N) return;
    TTF_Font* f = FL_FONTS[fi];
    int lh = TTF_FontLineSkip(f);
    char full[FL_WRAP_LEN], word[FL_WRAP_LEN], rest[FL_WRAP_LEN];
    char builder[FL_WRAP_LEN] = "", prev[FL_WRAP_LEN] = "";
    snprintf(full, sizeof(full), "%s ", text);
    size_t cur = 0, len = strlen(full);
    while (1) {
        char* sp = strchr(full + cur, ' ');
        if (sp == NULL) break;                      // cursor = npos
        size_t old = cur;
        size_t wl = (size_t)(sp - (full + cur));
        memcpy(word, full + cur, wl); word[wl] = '\0';
        cur = (size_t)(sp - full) + 1;
        strncat(builder, word, FL_WRAP_LEN - strlen(builder) - 1);
        if (fl_wrap_width(f, builder) > width) {
            if (prev[0] != '\0') {
                fl_wrap_line(prev);
                FL_WRAP_H += lh;
                int pw = fl_wrap_width(f, prev);
                if (pw > FL_WRAP_W) FL_WRAP_W = pw;
            }
            builder[0] = '\0';
            fl_wrap_pop(f, word, rest, width);
            if (rest[0] != '\0') {
                while (rest[0] != '\0') {
                    fl_wrap_line(word);
                    int ww = fl_wrap_width(f, word);
                    if (ww > FL_WRAP_W) FL_WRAP_W = ww;
                    FL_WRAP_H += lh;
                    strcpy(word, rest);
                    fl_wrap_pop(f, word, rest, width);
                    if (strcmp(rest, word) == 0) break;
                }
            }
            snprintf(builder, FL_WRAP_LEN, "%s ", word);
            strcpy(prev, builder);
        } else {
            strncat(builder, " ", FL_WRAP_LEN - strlen(builder) - 1);
            strcpy(prev, builder);
        }
        if (cur >= len) break;                      // no more separators
        if (cur == old) break;
    }
    fl_wrap_line(builder);
    // Parse::trim of the last line, for the size
    char trim[FL_WRAP_LEN];
    const char* a = builder;
    while (*a == ' ' || *a == '\t') a++;
    snprintf(trim, sizeof(trim), "%s", a);
    size_t tl = strlen(trim);
    while (tl > 0 && (trim[tl-1] == ' ' || trim[tl-1] == '\t')) trim[--tl] = '\0';
    if (trim[0] != '\0') FL_WRAP_H += lh;
    int tw = fl_wrap_width(f, trim);
    if (tw > FL_WRAP_W) FL_WRAP_W = tw;
    if (strcmp(text, " ") == 0) FL_WRAP_H += lh;
}

// A wrapped label: as fl_label, the lines in an image of the size of
// calcSizeWrapped, a line height apart (MenuHUDLog::add).
static void fl_label_wrap (int fi, const char* s, int x, int y, int color, int sx, int sy, int blend, int width) {
    fl_wrap(fi, s, width);
    if (FL_WRAP_W <= 0 || FL_WRAP_H <= 0) return;
    int lh = TTF_FontLineSkip(FL_FONTS[fi]);
    SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, FL_WRAP_W, FL_WRAP_H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (img == NULL) return;
    for (int k=0; k<2; k++) {
        SDL_Color c = (k == 0) ? (SDL_Color){ 0, 0, 0, 255 }
                               : (SDL_Color){ (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF, 255 };
        for (int i=0; i<FL_WRAP_N; i++) {
            if (FL_WRAP[i][0] == '\0') continue;
            SDL_Surface* t = blend ? TTF_RenderUTF8_Blended(FL_FONTS[fi], FL_WRAP[i], c)
                                   : TTF_RenderUTF8_Solid(FL_FONTS[fi], FL_WRAP[i], c);
            if (t == NULL) continue;
            for (int j=0; j<2; j++) {
                SDL_Rect d = { (k == 0) ? sx : 0, i * lh + ((k == 0) ? sy : 0), t->w, t->h };
                SDL_BlitSurface(t, NULL, img, &d);
            }
            SDL_FreeSurface(t);
        }
    }
    SDL_Rect dst = { x, y, FL_WRAP_W, FL_WRAP_H };
    SDL_SetSurfaceBlendMode(img, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(img, NULL, FL_SCREEN, &dst);
    SDL_FreeSurface(img);
}

#include "engine/map.h"
#include "engine/collision.h"
#include "engine/minimap.h"
#include "engine/geom.h"
#include "engine/astar.h"

///////////////////////////////////////////////////////////////////////////////
// INPUT, as InputState and SDLInputState of the original
///////////////////////////////////////////////////////////////////////////////

// the actions of Input:: of the original, in its order
enum {
    FL_CANCEL = 0, FL_ACCEPT, FL_UP, FL_DOWN, FL_LEFT, FL_RIGHT,
    FL_BAR_1, FL_BAR_2, FL_BAR_3, FL_BAR_4, FL_BAR_5, FL_BAR_6, FL_BAR_7, FL_BAR_8, FL_BAR_9, FL_BAR_0,
    FL_CHARACTER, FL_INVENTORY, FL_POWERS, FL_LOG, FL_MAIN1, FL_MAIN2,
    FL_EQUIPMENT_SWAP, FL_EQUIPMENT_SWAP_PREV, FL_MINIMAP_MODE, FL_LOOT_TOOLTIP_MODE, FL_ACTIONBAR,
    FL_MENU_PAGE_NEXT, FL_MENU_PAGE_PREV, FL_MENU_ACTIVATE, FL_PAUSE, FL_CYCLE_MENUS,
    FL_AIM_UP, FL_AIM_DOWN, FL_AIM_LEFT, FL_AIM_RIGHT,
    FL_DEVELOPER_MENU, FL_DEVELOPER_CMD_1, FL_DEVELOPER_CMD_2, FL_DEVELOPER_CMD_3,
    FL_CTRL, FL_SHIFT, FL_ALT, FL_DEL, FL_TEXTEDIT_UP, FL_TEXTEDIT_DOWN,
    FL_KEY_COUNT
};

static int FL_PRESSING[FL_KEY_COUNT];
static int FL_LOCK[FL_KEY_COUNT];
static int FL_UNPRESS[FL_KEY_COUNT];
static int FL_MOUSE_X = 0, FL_MOUSE_Y = 0;

// the default bindings of SDLInputState (QWERTY), by scancode
typedef struct { int action; int scancode; } FlBind;
static const FlBind FL_KEYS[] = {
    { FL_CANCEL, SDL_SCANCODE_ESCAPE }, { FL_ACCEPT, SDL_SCANCODE_RETURN }, { FL_ACCEPT, SDL_SCANCODE_SPACE },
    { FL_UP, SDL_SCANCODE_W }, { FL_UP, SDL_SCANCODE_UP },
    { FL_DOWN, SDL_SCANCODE_S }, { FL_DOWN, SDL_SCANCODE_DOWN },
    { FL_LEFT, SDL_SCANCODE_A }, { FL_LEFT, SDL_SCANCODE_LEFT },
    { FL_RIGHT, SDL_SCANCODE_D }, { FL_RIGHT, SDL_SCANCODE_RIGHT },
    { FL_BAR_1, SDL_SCANCODE_Q }, { FL_BAR_2, SDL_SCANCODE_E }, { FL_BAR_3, SDL_SCANCODE_R },
    { FL_BAR_4, SDL_SCANCODE_F }, { FL_BAR_5, SDL_SCANCODE_1 }, { FL_BAR_6, SDL_SCANCODE_2 },
    { FL_BAR_7, SDL_SCANCODE_3 }, { FL_BAR_8, SDL_SCANCODE_4 }, { FL_BAR_9, SDL_SCANCODE_5 },
    { FL_BAR_0, SDL_SCANCODE_6 },
    { FL_CHARACTER, SDL_SCANCODE_C }, { FL_INVENTORY, SDL_SCANCODE_I }, { FL_POWERS, SDL_SCANCODE_P },
    { FL_LOG, SDL_SCANCODE_L },
    { FL_MENU_PAGE_NEXT, SDL_SCANCODE_PAGEDOWN }, { FL_MENU_PAGE_PREV, SDL_SCANCODE_PAGEUP },
    { FL_MENU_ACTIVATE, SDL_SCANCODE_V },
    { FL_CTRL, SDL_SCANCODE_LCTRL }, { FL_CTRL, SDL_SCANCODE_RCTRL },
    { FL_SHIFT, SDL_SCANCODE_LSHIFT }, { FL_SHIFT, SDL_SCANCODE_RSHIFT },
    { FL_ALT, SDL_SCANCODE_LALT }, { FL_ALT, SDL_SCANCODE_RALT },
    { FL_DEL, SDL_SCANCODE_DELETE }, { FL_DEL, SDL_SCANCODE_BACKSPACE },
};

// InputState::handle: what was released in the frame before is no longer
// pressed nor locked
static void fl_input_frame (void) {
    for (int k=0; k<FL_KEY_COUNT; k++) {
        if (FL_UNPRESS[k]) {
            FL_PRESSING[k] = 0;
            FL_UNPRESS[k] = 0;
            FL_LOCK[k] = 0;
        }
    }
}

static void fl_input_key (int scancode, int down) {
    for (size_t i=0; i<sizeof(FL_KEYS)/sizeof(FL_KEYS[0]); i++) {
        if (FL_KEYS[i].scancode == scancode) {
            int k = FL_KEYS[i].action;
            if (down) {
                FL_PRESSING[k] = 1;
                FL_UNPRESS[k] = 0;
            } else {
                FL_UNPRESS[k] = 1;
            }
        }
    }
}

static void fl_input_mouse (int x, int y, int button, int down) {
    FL_MOUSE_X = x;
    FL_MOUSE_Y = y;
    int k = (button == SDL_BUTTON_LEFT) ? FL_MAIN1 : (button == SDL_BUTTON_RIGHT) ? FL_MAIN2 : -1;
    if (k >= 0) {
        if (down) {
            FL_PRESSING[k] = 1;
            FL_UNPRESS[k] = 0;
        } else {
            FL_UNPRESS[k] = 1;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// START
///////////////////////////////////////////////////////////////////////////////

// InputState::handle, after the frame: what was released in it is no
// longer pressed nor locked (the original does it before the events of
// the next frame, which is the same)
static void fl_flush (int render) {
    (void) render;
    fl_input_frame();
}

// After the window: the size of the render, the seed (FLARE_SEED, or the
// time), the trace (FLARE_TRACE: a file, or "-" for stdout).
static void fl_start (void) {
    sdl_size(&FL_W, &FL_H);
    FL_RND = fl_env("FLARE_SEED") ? strtoull(fl_env("FLARE_SEED"), NULL, 10)
                                  : (uint64_t)time(NULL);
    if (fl_env("FLARE_TRACE")) {
        FL_TRACE = strcmp(fl_env("FLARE_TRACE"), "-") ? fopen(fl_env("FLARE_TRACE"), "w") : stdout;
    }
    sdl_on_flush(fl_flush);
}
