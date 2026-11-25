// The minimap of the HUD (MenuMiniMap, isometric): the collision map drawn
// pixel by pixel in a surface, once per map (prerenderIso), and the pixels
// of the hero, the enemies, the NPCs, and the teleports in another, per
// frame (renderEntitiesIso); both blitted into the area of the map.

static SDL_Surface* FL_MM = NULL;           // the tiles, at the zoom
static SDL_Surface* FL_MM_ENT = NULL;       // the entities, of the area
static int          FL_MM_ZOOM = 1;
static int          FL_MM_MAX = 0;          // max(map w, map h)

// Image::drawPixel: an opaque pixel, if inside the surface
static void fl_mm_pixel (SDL_Surface* s, int x, int y, Uint32 c) {
    if (s == NULL || x < 0 || y < 0 || x >= s->w || y >= s->h) {
        return;
    }
    ((Uint32*)((Uint8*)s->pixels + y * s->pitch))[x] = c;
}

static Uint32 fl_mm_color (int64_t rgb) {
    return SDL_MapRGBA(FL_SCREEN->format, (rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF, 255);
}

// MenuMiniMap::prerenderIso and updateIso, for the whole map: walls (1, 5)
// and obstacles (2, 6) of the collision map, two pixels wide per tile
static void fl_mm_prerender (int zoom, int pw, int ph, int64_t wall, int64_t obst) {
    if (FL_MM != NULL) SDL_FreeSurface(FL_MM);
    if (FL_MM_ENT != NULL) SDL_FreeSurface(FL_MM_ENT);
    FL_MM_ZOOM = zoom;
    FL_MM_MAX = (FL_MAP_W > FL_MAP_H) ? FL_MAP_W : FL_MAP_H;
    int size = ((FL_MAP_W + zoom > FL_MAP_H + zoom) ? FL_MAP_W + zoom : FL_MAP_H + zoom) * 2 * zoom;
    FL_MM = SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, SDL_PIXELFORMAT_ARGB8888);
    FL_MM_ENT = SDL_CreateRGBSurfaceWithFormat(0, pw, ph, 32, SDL_PIXELFORMAT_ARGB8888);
    Uint32 cw = fl_mm_color(wall), co = fl_mm_color(obst);
    for (int i=0; i<FL_MAP_W; i++) {
        for (int j=0; j<FL_MAP_H; j++) {
            int t = fl_col_get(i, j);
            Uint32 c;
            if (t == 1 || t == 5) c = cw;
            else if (t == 2 || t == 6) c = co;
            else continue;
            int ex = zoom * (i - j + FL_MM_MAX);
            int ey = zoom * (i + j) - 1;
            for (int l=0; l<zoom; l++) {
                for (int k=0; k<zoom; k++) {
                    fl_mm_pixel(FL_MM, ex + k, ey + l, c);
                    fl_mm_pixel(FL_MM, ex + k - zoom, ey + l, c);
                }
            }
        }
    }
}

// renderEntitiesIso: the surface of the entities cleared
static void fl_mm_clear (void) {
    if (FL_MM_ENT != NULL) {
        SDL_FillRect(FL_MM_ENT, NULL, 0);
    }
}

// a pixel of an entity at a tile, with the offset of the hero
static void fl_mm_entity (int x, int y, int ox, int oy, int64_t rgb) {
    int z = FL_MM_ZOOM;
    int ex = z * (x - y + FL_MM_MAX) - ox;
    int ey = z * (x + y) - oy - 1;
    Uint32 c = fl_mm_color(rgb);
    for (int l=0; l<z; l++) {
        for (int k=0; k<z; k++) {
            fl_mm_pixel(FL_MM_ENT, ex + k, ey + l, c);
            fl_mm_pixel(FL_MM_ENT, ex + k - z, ey + l, c);
        }
    }
}

// renderMapSurface: the part of the tiles around the hero, then the
// entities, into the area at ax,ay
static void fl_mm_draw (int cx, int cy, int w, int h, int ax, int ay) {
    if (FL_MM == NULL) {
        return;
    }
    SDL_Rect src = { cx, cy, w, h };
    SDL_Rect dst = { ax, ay, w, h };
    SDL_SetSurfaceAlphaMod(FL_MM, 255);
    SDL_BlitSurface(FL_MM, &src, FL_SCREEN, &dst);
    SDL_Rect src2 = { 0, 0, w, h };
    SDL_Rect dst2 = { ax, ay, w, h };
    SDL_BlitSurface(FL_MM_ENT, &src2, FL_SCREEN, &dst2);
}
