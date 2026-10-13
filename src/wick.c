/*
 * wick: screensaver of the Wick, from Calca 3 of "Anathem".
 *
 * Worlds joined by threads along which knowledge flows one way only. The diagrams
 * of the Calca are built up in turn, from Simple Protism to the Wick itself, and
 * the Daban Urnud climbs the Wick to Arbre.
 *
 *   wick            full screen on every display; any key or mouse movement quits
 *   wick --window   in a window; Esc or Q quits
 *   Wick.scr /s     Windows screensaver conventions
 */
#include <SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* The pictures (drawn with Gemini, prepared by tools/make_sprites.py) are built
 * into the program as PNG and JPEG files and decoded at start-up. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS /* else Windows builds need libwinpthread-1.dll */
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__APPLE__)
/* Mach-O: its own section names, and C names carry a leading underscore */
#define ART_SECTION ".section __DATA,__const\n"
#define ART_SYM "_art_"
#elif defined(_WIN32)
#define ART_SECTION ".section .rdata,\"dr\"\n"
#define ART_SYM "art_"
#else
#define ART_SECTION ".section .rodata\n"
#define ART_SYM "art_"
#endif
#define ART(name, file) \
    __asm__(ART_SECTION ".globl " ART_SYM #name "\n" ART_SYM #name ":\n.incbin \"assets/art/" file "\"\n" \
            ".globl " ART_SYM #name "_end\n" ART_SYM #name "_end:\n.text\n"); \
    extern const unsigned char art_##name[], art_##name##_end[];
ART(sky, "sky.jpg")
ART(banner, "banner.png")
ART(glow, "glow.jpg")
ART(w0, "w0.png")
ART(w1, "w1.png")
ART(w2, "w2.png")
ART(w3, "w3.png")
ART(w4, "w4.png")
ART(w5, "w5.png")
ART(u0, "u0.png")
ART(u1, "u1.png")
ART(u2, "u2.png")
ART(u3, "u3.png")
ART(u4, "u4.png")
ART(u5, "u5.png")
ART(u6, "u6.png")
ART(u7, "u7.png")
ART(u8, "u8.png")
ART(u9, "u9.png")
ART(u10, "u10.png")
ART(u11, "u11.png")
ART(u12, "u12.png")
ART(u13, "u13.png")
ART(u14, "u14.png")
ART(u15, "u15.png")

enum { SP_SKY, SP_BANNER, SP_GLOW, SP_W0, SP_W1, SP_W2, SP_W3, SP_W4, SP_W5, SP_U0, SP_U1, SP_U2, SP_U3, SP_U4, SP_U5, SP_U6, SP_U7, SP_U8, SP_U9, SP_U10, SP_U11, SP_U12, SP_U13, SP_U14, SP_U15, NSPRITE };
static struct { const unsigned char *file, *end; int w, h; unsigned char *px; } sprite[NSPRITE] = {
    {art_sky, art_sky_end},
    {art_banner, art_banner_end},
    {art_glow, art_glow_end},
    {art_w0, art_w0_end},
    {art_w1, art_w1_end},
    {art_w2, art_w2_end},
    {art_w3, art_w3_end},
    {art_w4, art_w4_end},
    {art_w5, art_w5_end},
    {art_u0, art_u0_end},
    {art_u1, art_u1_end},
    {art_u2, art_u2_end},
    {art_u3, art_u3_end},
    {art_u4, art_u4_end},
    {art_u5, art_u5_end},
    {art_u6, art_u6_end},
    {art_u7, art_u7_end},
    {art_u8, art_u8_end},
    {art_u9, art_u9_end},
    {art_u10, art_u10_end},
    {art_u11, art_u11_end},
    {art_u12, art_u12_end},
    {art_u13, art_u13_end},
    {art_u14, art_u14_end},
    {art_u15, art_u15_end},
};
static SDL_Texture **cur_tex; /* the pictures, for the renderer being drawn to */

static void load_sprites(void)
{
    for (int i = 0; i < NSPRITE; i++) {
        int n;
        sprite[i].px = stbi_load_from_memory(sprite[i].file, (int)(sprite[i].end - sprite[i].file), &sprite[i].w, &sprite[i].h, &n, 4);
        if (!sprite[i].px) { sprite[i].w = sprite[i].h = 1; }
    }
}
static void make_textures(SDL_Renderer *r, SDL_Texture **tex)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    for (int i = 0; i < NSPRITE; i++) {
        tex[i] = NULL;
        if (!sprite[i].px) continue;
        tex[i] = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, sprite[i].w, sprite[i].h);
        if (!tex[i]) continue;
        SDL_UpdateTexture(tex[i], NULL, sprite[i].px, sprite[i].w * 4);
        /* the glow is light painted on black, and is added to what is behind it */
        SDL_SetTextureBlendMode(tex[i], i == SP_GLOW ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    }
}

/* ---- options ---- */
static int opt_window = 0;
static double opt_speed = 1.0;
static uint64_t opt_seed = 0;
static const char *opt_snapshot = NULL;
static double opt_time = 30;
static int opt_w = 1920, opt_h = 1200;

/* ---- colours ---- */
typedef struct { Uint8 r, g, b; } Rgb;
static const Rgb BG = {8, 8, 20}, INK = {46, 32, 22}, CHORD = {150, 44, 30}, PAPER = {253, 246, 226};
static const Rgb THREAD = {170, 180, 230}, GOLD = {250, 214, 120};

/* ---- geometry buffer ---- */
static SDL_Vertex *vb;
static int *ib;
static int nv, ni, vcap, icap;

static void vb_need(int v, int i)
{
    if (nv + v > vcap) { vcap = (nv + v) * 2; vb = realloc(vb, sizeof *vb * vcap); }
    if (ni + i > icap) { icap = (ni + i) * 2; ib = realloc(ib, sizeof *ib * icap); }
}
static inline SDL_Color col(Rgb c, double a)
{
    SDL_Color o = {c.r, c.g, c.b, (Uint8)(a < 0 ? 0 : a > 1 ? 255 : a * 255)};
    return o;
}
static inline void vert(double x, double y, SDL_Color c)
{
    vb[nv].position.x = (float)x; vb[nv].position.y = (float)y;
    vb[nv].color = c; vb[nv].tex_coord.x = vb[nv].tex_coord.y = 0;
    nv++;
}
static void quad4(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3,
                  SDL_Color c0, SDL_Color c1, SDL_Color c2, SDL_Color c3)
{
    vb_need(4, 6);
    int b = nv;
    vert(x0, y0, c0); vert(x1, y1, c1); vert(x2, y2, c2); vert(x3, y3, c3);
    ib[ni++] = b; ib[ni++] = b + 1; ib[ni++] = b + 2; ib[ni++] = b; ib[ni++] = b + 2; ib[ni++] = b + 3;
}
static void quad(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, SDL_Color c)
{
    quad4(x0, y0, x1, y1, x2, y2, x3, y3, c, c, c, c);
}
static void rect(double x, double y, double w, double h, SDL_Color c) { quad(x, y, x + w, y, x + w, y + h, x, y + h, c); }
static void disc(double cx, double cy, double r, SDL_Color centre, SDL_Color rim)
{
    int n = r < 4 ? 10 : r < 30 ? 28 : 56;
    vb_need(n + 1, 3 * n);
    int b = nv;
    vert(cx, cy, centre);
    for (int i = 0; i < n; i++) vert(cx + r * cos(i * 2 * M_PI / n), cy + r * sin(i * 2 * M_PI / n), rim);
    for (int i = 0; i < n; i++) { ib[ni++] = b; ib[ni++] = b + 1 + i; ib[ni++] = b + 1 + (i + 1) % n; }
}
/* a stroke with round ends */
static void line(double x0, double y0, double x1, double y1, double w, SDL_Color c)
{
    double dx = x1 - x0, dy = y1 - y0, l = hypot(dx, dy);
    if (l > 1e-9) {
        double nx = -dy / l * w / 2, ny = dx / l * w / 2;
        quad(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, c);
    }
    if (w > 2.5) { disc(x0, y0, w / 2, c, c); disc(x1, y1, w / 2, c, c); }
}
__attribute__((unused)) static void ring(double cx, double cy, double r, double w, SDL_Color c)
{
    for (int i = 0; i < 64; i++) {
        double a = i * M_PI / 32, b = (i + 1) * M_PI / 32;
        line(cx + cos(a) * r, cy + sin(a) * r, cx + cos(b) * r, cy + sin(b) * r, w, c);
    }
}
static void flush(SDL_Renderer *r)
{
    if (ni) SDL_RenderGeometry(r, NULL, vb, nv, ib, ni);
    nv = ni = 0;
}

static SDL_Renderer *ren; /* where the current frame is being drawn */

/* a picture stretched over a rectangle; sx0..sx1 picks a slice of it across (0..1) */
static void pic_part(int which, double x, double y, double w, double h, double sx0, double sx1, double tilt, int flip)
{
    flush(ren);
    if (!cur_tex || !cur_tex[which]) return;
    SDL_Rect src = {(int)(sx0 * sprite[which].w), 0, (int)((sx1 - sx0) * sprite[which].w), sprite[which].h};
    SDL_FRect dst = {(float)x, (float)y, (float)w, (float)h};
    SDL_FPoint pivot = {(float)(w / 2), (float)h};
    SDL_RenderCopyExF(ren, cur_tex[which], &src, &dst, tilt, &pivot, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}
/* a picture `tall` high, standing with the point (ax, ay) of it (0..1 across and down) at (x, y) */
__attribute__((unused)) static void pic(int which, double x, double y, double tall, double ax, double ay)
{
    double w = tall * sprite[which].w / sprite[which].h;
    pic_part(which, x - w * ax, y - tall * ay, w, tall, 0, 1, 0, 0);
}

/* ---- lettering: pen strokes on a 4 x 6 grid, drawn with a round nib ---- */
static const char *GLYPH[] = {
    "A06 02 20 42 46|04 44", "B00 06 36 45 44 33 03|33 42 41 30 00", "C41 30 10 01 05 16 36 45",
    "D00 06 26 44 42 20 00", "E40 00 06 46|03 33", "F40 00 06|03 33", "G41 30 10 01 05 16 36 45 43 23",
    "H00 06|40 46|03 43", "I20 26|10 30|16 36", "J40 45 36 16 05", "K00 06|40 03 46", "L00 06 46",
    "M06 00 23 40 46", "N06 00 46 40", "O10 30 41 45 36 16 05 01 10", "P06 00 30 41 42 33 03",
    "Q10 30 41 45 36 16 05 01 10|24 46", "R06 00 30 41 42 33 03|23 46", "S41 30 10 01 02 13 33 44 45 36 16 05",
    "T00 40|20 26", "U00 05 16 36 45 40", "V00 26 40", "W00 16 23 36 40", "X00 46|40 06", "Y00 23 40|23 26",
    "Z00 40 06 46", "010 30 41 45 36 16 05 01 10", "111 20 26|16 36", "201 10 30 41 42 06 46",
    "301 10 30 41 42 33 23|33 44 45 36 16 05", "436 30 04 44", "540 00 03 33 44 45 36 16 05",
    "630 10 01 05 16 36 45 44 33 03", "700 40 16", "813 02 01 10 30 41 42 33 13 04 05 16 36 45 44 33",
    "943 13 02 01 10 30 41 45 36 16", ",25 26 17", ".26 26", ":22 22|25 25", "'20 21", "?01 10 30 41 42 24|26 26",
    "-13 33", "!20 24|26 26", "+03 43|21 25", "=02 42|04 44",
};
static const char *glyph(char ch)
{
    for (unsigned i = 0; i < sizeof GLYPH / sizeof *GLYPH; i++)
        if (GLYPH[i][0] == ch) return GLYPH[i] + 1;
    return NULL;
}
static double text_width(const char *str, double s) { return strlen(str) * 6.2 * s - 2.2 * s; }
/* text centred on cx, top at y; s is one grid step, so letters are 6s tall */
static void text(const char *str, double cx, double y, double s, double weight, SDL_Color c)
{
    double x = cx - text_width(str, s) / 2;
    for (; *str; str++, x += 6.2 * s) {
        const char *g = glyph(*str);
        if (!g) continue;
        int have = 0; double lx = 0, ly = 0;
        for (; *g; g++) {
            if (*g == '|') { have = 0; continue; }
            if (*g == ' ') continue;
            double gx = x + (g[0] - '0') * s, gy = y + (g[1] - '0') * s;
            g++;
            if (have) line(lx, ly, gx, gy, s * weight, c);
            disc(gx, gy, s * weight * (have && lx == gx && ly == gy ? 0.62 : 0.5), c, c); /* the nib is round */
            lx = gx; ly = gy; have = 1;
        }
    }
}
/* ---- the diagrams ----
 *
 * Calca 3: Complex versus Simplex Protism. Worlds are joined by arrows along which
 * knowledge of pure forms flows, always from a more Hylaean world to a less, and
 * never back round to where it began. The Calca builds up from one arrow to the
 * general case, the Wick; so does this.
 *
 * A diagram is worked out in a square from 0 to 1 each way, the flow running from
 * left to right, and fitted to the screen when drawn. */
#define MAXN 40
#define MAXE 120
typedef struct { double x, y; int kind; const char *label; double born; int layer, pic; } Node;
typedef struct { int a, b; double born, bend; } Edge;
static Node node[MAXN];
static Edge edge[MAXE];
static int nn, ne;
static int climb[5], nclimb;            /* in the Wick: the worlds the ship visits, the first one first */
enum { HTW, ARBRE, WORLD };             /* a world of pure forms, Arbre, any other world */

static double T, anim_t;
static uint64_t seed_state;
static int scene = -1;
static const double LENGTH[5] = {12, 13, 13, 13, 46};
#define NSCENE 5
#define ROUND (12 + 13 + 13 + 13 + 46.0)

static uint64_t next_rand(void)
{
    seed_state ^= seed_state << 13; seed_state ^= seed_state >> 7; seed_state ^= seed_state << 17;
    return seed_state;
}
static double frand(void) { return (next_rand() >> 11) * (1.0 / 9007199254740992.0); }
static double ramp(double t, double a, double b) { return t <= a ? 0 : t >= b ? 1 : (t - a) / (b - a); }
static double ease(double x) { return x * x * (3 - 2 * x); }

static int add(double x, double y, int kind, const char *label, double born)
{
    /* the picture: 0 pure forms, 1 Arbre; the rest are given theirs when the diagram is done */
    node[nn] = (Node){x, y, kind, label, born, 0, kind == HTW ? 0 : kind == ARBRE ? 1 : -1};
    return nn++;
}
static void join(int a, int b, double born)
{
    edge[ne++] = (Edge){a, b, born, (frand() - 0.5) * 0.12};
}

/* The general case: worlds in ranks from the most Hylaean on the left, every arrow
 * pointing to a later rank, so the flow can never come round again. */
static void build_wick(void)
{
    enum { RANKS = 6 };
    int first[RANKS + 1], count[RANKS];
    for (int r = 0; r < RANKS; r++) {
        count[r] = r == 0 ? 3 : 3 + (int)(frand() * 2); /* never more nameless worlds than there are pictures */
        first[r] = nn;
        for (int k = 0; k < count[r]; k++) {
            double x = 0.04 + 0.92 * r / (RANKS - 1) + (frand() - 0.5) * 0.05;
            double y = (k + 0.5) / count[r] + (frand() - 0.5) * 0.3 / count[r];
            node[add(x, y, r == 0 ? HTW : WORLD, NULL, 0.4 + r * 0.9 + frand() * 0.6)].layer = r;
        }
    }
    first[RANKS] = nn;
    for (int r = 1; r < RANKS; r++)
        for (int k = first[r]; k < first[r + 1]; k++) {
            /* every world has at least one source in the rank before, usually two, and now and then one further back */
            int nearest = first[r - 1];
            for (int j = first[r - 1]; j < first[r]; j++) if (fabs(node[j].y - node[k].y) < fabs(node[nearest].y - node[k].y)) nearest = j;
            join(nearest, k, node[k].born + 0.3);
            int other = first[r - 1] + (int)(frand() * count[r - 1]);
            if (other != nearest && frand() < 0.7) join(other, k, node[k].born + 0.6);
            if (r >= 2 && frand() < 0.25) join(first[r - 2] + (int)(frand() * count[r - 2]), k, node[k].born + 0.9);
        }
    /* The ship's road: from a world far down the Wick, up against the flow, a rank at
     * a time, to Arbre. */
    static const char *NAMES[5] = {"URNUD", "TRO", "LATERRE", "FTHOS", "ARBRE"};
    int at = first[RANKS - 1] + (int)(frand() * count[RANKS - 1]);
    nclimb = 0;
    for (int step = 0; step < 5; step++) {
        climb[nclimb++] = at;
        node[at].label = NAMES[step];
        /* Urnud with its one continent, Tro all islands, Laterre with its polar
         * continent, Fthos with its belt, and Arbre */
        node[at].pic = step == 4 ? 1 : step + 2;
        if (step == 4) { node[at].kind = ARBRE; break; }
        int up = -1;
        for (int e = 0; e < ne; e++) if (edge[e].b == at && node[edge[e].a].layer == node[at].layer - 1) { up = edge[e].a; break; }
        if (up < 0) break;
        at = up;
    }
}

static void build(int which)
{
    nn = ne = nclimb = 0;
    scene = which;
    switch (which) {
    case 0: { /* Simple Protism */
        int h = add(0.22, 0.5, HTW, "THE HYLAEAN THEORIC WORLD", 0.6), a = add(0.78, 0.5, ARBRE, "ARBRE", 1.6);
        join(h, a, 2.6);
        break; }
    case 1: { /* the Freight Train: a line of worlds, each the source of the next */
        int prev = add(0.06, 0.5, HTW, "THE HYLAEAN THEORIC WORLD", 0.5);
        for (int k = 1; k < 5; k++) {
            int n = add(0.06 + 0.22 * k, 0.5 + (k % 2 ? -0.06 : 0.06), k == 4 ? ARBRE : WORLD, k == 4 ? "ARBRE" : NULL, 0.5 + k * 1.1);
            join(prev, n, 1.0 + k * 1.1);
            prev = n;
        }
        break; }
    case 2: { /* the Firing Squad: many sources, one world */
        int a = add(0.8, 0.5, ARBRE, "ARBRE", 0.5);
        for (int k = 0; k < 4; k++) join(add(0.16 + 0.05 * fabs(k - 1.5), 0.08 + 0.28 * k, HTW, NULL, 1.2 + k * 0.8), a, 1.8 + k * 0.8);
        break; }
    case 3: { /* the Strider: Arbre takes from some worlds and gives to others */
        int a = add(0.5, 0.5, ARBRE, "ARBRE", 0.5);
        for (int k = 0; k < 2; k++) join(add(0.14, 0.26 + 0.48 * k, HTW, NULL, 1.4 + k * 0.8), a, 2.0 + k * 0.8);
        for (int k = 0; k < 2; k++) join(a, add(0.86, 0.26 + 0.48 * k, WORLD, NULL, 4.4 + k * 0.8), 5.0 + k * 0.8);
        break; }
    default: build_wick(); break;
    }
    /* every world without a name gets a picture of its own: sixteen, dealt out at random */
    int deck[16];
    for (int i = 0; i < 16; i++) deck[i] = i;
    for (int i = 15; i > 0; i--) { int j = (int)(frand() * (i + 1)) % (i + 1), t = deck[i]; deck[i] = deck[j]; deck[j] = t; }
    for (int i = 0, dealt = 0; i < nn; i++) if (node[i].pic < 0) node[i].pic = 6 + deck[dealt++ % 16];
}

static void new_round(void) { T = 0; scene = -1; }

static void update(double dt)
{
    anim_t += dt;
    T += dt * opt_speed;
    if (T > ROUND) new_round();
}

/* ---- drawing ---- */
static double unit;
static int sw, sh;
static double px(double x) { return sw * (0.1 + 0.8 * x); }
static double py(double y) { return sh * (0.24 + 0.68 * y); }

static void glow(double x, double y, double d, double alpha)
{
    flush(ren);
    if (!cur_tex || !cur_tex[SP_GLOW] || alpha <= 0) return;
    SDL_FRect dst = {(float)(x - d / 2), (float)(y - d / 2), (float)d, (float)d};
    SDL_SetTextureAlphaMod(cur_tex[SP_GLOW], (Uint8)(fmin(alpha, 1) * 255));
    SDL_RenderCopyF(ren, cur_tex[SP_GLOW], NULL, &dst);
    SDL_SetTextureAlphaMod(cur_tex[SP_GLOW], 255);
}
/* a point so far along the thread from one world to another, which sags a little */
static void along(const Edge *e, double q, double *x, double *y)
{
    double x0 = px(node[e->a].x), y0 = py(node[e->a].y), x1 = px(node[e->b].x), y1 = py(node[e->b].y);
    double mx = (x0 + x1) / 2 - (y1 - y0) * e->bend, my = (y0 + y1) / 2 + (x1 - x0) * e->bend;
    *x = (1 - q) * (1 - q) * x0 + 2 * q * (1 - q) * mx + q * q * x1;
    *y = (1 - q) * (1 - q) * y0 + 2 * q * (1 - q) * my + q * q * y1;
}
static double size_of(const Node *n) { return sh * (nn > 12 ? 0.085 : 0.17) * (n->kind == HTW ? 1.08 : 1); }

static const char *say1, *say2;

static void draw_caption(int w, int h, const char *l1, const char *l2)
{
    double s1 = unit * 0.46, s2 = unit * 0.3;
    double widest = fmax(text_width(l1, s1), text_width(l2, s2)), room = w * 0.62;
    if (widest > room) { s1 *= room / widest; s2 *= room / widest; }
    double bw = fmax(text_width(l1, s1), text_width(l2, s2)) + unit * 13, bh = unit * 11.5;
    double bx = w / 2.0 - bw / 2, by = unit * 1.2;
    double cap = 0.135, cw = bh * 1.15 * sprite[SP_BANNER].w * cap / sprite[SP_BANNER].h;
    pic_part(SP_BANNER, bx + cw - 1, by, bw - 2 * cw + 2, bh, cap, 1 - cap, 0, 0);
    pic_part(SP_BANNER, bx, by, cw, bh, 0, cap, 0, 0);
    pic_part(SP_BANNER, bx + bw - cw, by, cw, bh, 1 - cap, 1, 0, 0);
    text(l1, w / 2.0, by + bh * 0.25, s1, 0.95, col(INK, 1));
    text(l2, w / 2.0, by + bh * 0.25 + s1 * 6 + unit * 1.2, s2, 0.9, col(CHORD, 1));
}

static void draw_scene(SDL_Renderer *r, int w, int h)
{
    ren = r; sw = w; sh = h;
    unit = (w < h ? w : h) * 0.0125;
    SDL_SetRenderDrawColor(r, BG.r, BG.g, BG.b, 255);
    SDL_RenderClear(r);
    pic_part(SP_SKY, 0, 0, w, h, 0, 1, 0, 0);

    int k = 0;
    double t = T;
    while (k < NSCENE - 1 && t >= LENGTH[k]) t -= LENGTH[k++];
    if (k != scene) build(k);
    int wick = k == NSCENE - 1;
    double speed = wick ? 0.5 : 0.36; /* how much of a thread a spark covers in a second */

    /* the threads, drawn out from their sources as they come into being */
    for (int e = 0; e < ne; e++) {
        double grown = ease(ramp(t, edge[e].born, edge[e].born + 0.9));
        if (grown <= 0) continue;
        double x0, y0, x1, y1;
        int on_road = 0; /* the ship's road is picked out */
        for (int c = 0; c + 1 < nclimb; c++) on_road |= edge[e].b == climb[c] && edge[e].a == climb[c + 1];
        for (int s = 0; s < 24; s++) {
            double q0 = s / 24.0 * grown, q1 = (s + 1) / 24.0 * grown;
            along(&edge[e], q0, &x0, &y0); along(&edge[e], q1, &x1, &y1);
            line(x0, y0, x1, y1, unit * (on_road ? 0.42 : 0.26), col(on_road ? GOLD : THREAD, on_road ? 0.95 : 0.6));
        }
        if (grown >= 1) { /* the arrowhead, two thirds of the way along */
            double ax, ay, bx, by;
            along(&edge[e], 0.62, &ax, &ay); along(&edge[e], 0.66, &bx, &by);
            double dx = bx - ax, dy = by - ay, len = hypot(dx, dy), ux = dx / len, uy = dy / len, a = unit * 0.9;
            line(bx, by, bx - ux * a - uy * a * 0.5, by - uy * a + ux * a * 0.5, unit * 0.2, col(on_road ? GOLD : THREAD, 0.85));
            line(bx, by, bx - ux * a + uy * a * 0.5, by - uy * a - ux * a * 0.5, unit * 0.2, col(on_road ? GOLD : THREAD, 0.85));
        }
    }
    flush(r);
    /* the flow: sparks running down every thread, one after another */
    double lit[MAXN] = {0};
    for (int e = 0; e < ne; e++) {
        if (t < edge[e].born + 0.9) continue;
        double since = t - edge[e].born - 0.9, phase = fmod(since * speed + e * 0.37, 1.0), x, y;
        for (int s = 0; s < 2; s++) {
            double q = fmod(phase + s * 0.5, 1.0);
            along(&edge[e], q, &x, &y);
            glow(x, y, unit * 2.6, 0.8 * sin(q * M_PI));
            if (q > 0.88) lit[edge[e].b] = fmax(lit[edge[e].b], (q - 0.88) / 0.12);
        }
    }
    /* the worlds */
    for (int i = 0; i < nn; i++) {
        double grown = ramp(t, node[i].born, node[i].born + 0.6);
        if (grown <= 0) continue;
        double pop = grown < 1 ? 1 - cos(grown * M_PI * 2.2) * exp(-grown * 3.5) * (1 - grown) : 1;
        double d = size_of(&node[i]) * pop, x = px(node[i].x), y = py(node[i].y) + sin(anim_t * 0.7 + i * 1.7) * unit * 0.25;
        glow(x, y, d * (node[i].kind == HTW ? 1.9 : 1.7), (node[i].kind == HTW ? 0.2 : 0.06) + 0.3 * lit[i]);
        int which = SP_W0 + node[i].pic;
        flush(r);
        if (cur_tex && cur_tex[which]) {
            SDL_FRect dst = {(float)(x - d / 2), (float)(y - d / 2), (float)d, (float)d};
            SDL_RenderCopyF(r, cur_tex[which], NULL, &dst);
        }
        if (node[i].label && grown >= 1) {
            double s = unit * (wick ? 0.2 : 0.26);
            text(node[i].label, x, y + d * 0.56, s, 1, col(PAPER, ramp(t, node[i].born + 0.6, node[i].born + 1.2)));
        }
    }
    flush(r);
    /* the ship climbing the Wick, a world at a time, against the flow */
    int reached = -1;
    if (wick && nclimb == 5) {
        double start = 16, hop = 5.2, u = (t - start) / hop;
        if (u >= 0) {
            int leg = (int)u;
            double f = u - leg, x, y;
            if (leg >= 4) { leg = 3; f = 1; }
            reached = f >= 1 ? leg + 1 : leg;
            f = ease(ramp(f, 0.35, 1)); /* it waits a while at each world before going on */
            double x0 = px(node[climb[leg]].x), y0 = py(node[climb[leg]].y), x1 = px(node[climb[leg + 1]].x), y1 = py(node[climb[leg + 1]].y);
            /* it rides just above the worlds, so that they can still be seen */
            x = x0 + (x1 - x0) * f; y = y0 + (y1 - y0) * f - sin(f * M_PI) * unit * 2.5 - size_of(&node[climb[leg]]) * 0.62;
            glow(x, y, unit * 4.5, 0.75);
            disc(x, y, unit * 0.45, col(PAPER, 1), col(GOLD, 1));
            flush(r);
        }
    }

    static const char *S1[5] = {"SIMPLE PROTISM", "THE FREIGHT TRAIN", "THE FIRING SQUAD", "THE STRIDER", "THE WICK"};
    static const char *S2[5] = {"ONE WORLD OF PURE FORMS, AND WHAT IT KNOWS FLOWS TO ARBRE",
                                "WORLD AFTER WORLD IN A LINE, EACH THE SOURCE OF THE NEXT",
                                "MANY SOURCES, ALL AIMED AT THE ONE WORLD",
                                "ARBRE DRINKS FROM SOME WORLDS, AND OTHERS DRINK FROM ARBRE",
                                "ANY NUMBER OF WORLDS, AND THE FLOW NEVER COMES ROUND AGAIN"};
    say1 = S1[k]; say2 = S2[k];
    if (wick && reached >= 0) {
        static const char *AT[5] = {"THE DABAN URNUD SETS OUT FROM URNUD", "UP THE WICK TO TRO", "ON TO LATERRE", "ON TO FTHOS", "AND SO TO ARBRE"};
        say2 = AT[reached];
        if (reached == 4) say2 = "AND SO TO ARBRE: EACH WORLD MORE HYLAEAN THAN THE LAST";
    }
    draw_caption(w, h, say1, say2);
    flush(r);
    double fade = fmin(ramp(t, 0, 0.8), 1 - ramp(t, LENGTH[k] - 0.8, LENGTH[k]));
    if (fade < 1) { rect(0, 0, w, h, col(BG, 1 - fade)); flush(r); }
}

typedef struct { SDL_Window *win; SDL_Renderer *ren; SDL_Texture *scene, *tex[NSPRITE]; int w, h; } View;

static void size_view(View *v)
{
    int ow, oh;
    SDL_RendererInfo info;
    SDL_GetRendererOutputSize(v->ren, &ow, &oh);
    if (ow < 1 || oh < 1) return;
    int ss = 2; /* drawn at twice the size and scaled down, for smooth edges */
    if (SDL_GetRendererInfo(v->ren, &info) == 0) {
        if (!(info.flags & SDL_RENDERER_TARGETTEXTURE) || (info.flags & SDL_RENDERER_SOFTWARE)) ss = 0;
        else if (info.max_texture_width && (ow * 2 > info.max_texture_width || oh * 2 > info.max_texture_height)) ss = 0;
    }
    if (v->w == ow * (ss ? ss : 1) && v->h == oh * (ss ? ss : 1) && (v->scene || !ss)) return;
    if (v->scene) SDL_DestroyTexture(v->scene);
    v->scene = NULL; v->w = ow; v->h = oh;
    if (!ss) return;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    v->scene = SDL_CreateTexture(v->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, ow * ss, oh * ss);
    if (v->scene) { v->w = ow * ss; v->h = oh * ss; }
}
static void render(View *v)
{
    if (v->scene) SDL_SetRenderTarget(v->ren, v->scene);
    cur_tex = v->tex;
    draw_scene(v->ren, v->w, v->h);
    if (v->scene) { SDL_SetRenderTarget(v->ren, NULL); SDL_RenderCopy(v->ren, v->scene, NULL, NULL); }
    SDL_RenderPresent(v->ren);
}
static int make_view(View *v, int display)
{
    Uint32 flags = SDL_WINDOW_ALLOW_HIGHDPI;
    int x = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
    if (opt_window) flags |= SDL_WINDOW_RESIZABLE;
    else flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
    memset(v, 0, sizeof *v);
    v->win = SDL_CreateWindow("The Wick", x, x, 1200, 750, flags);
    if (!v->win) return 0;
    v->ren = SDL_CreateRenderer(v->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!v->ren) v->ren = SDL_CreateRenderer(v->win, -1, SDL_RENDERER_SOFTWARE);
    if (!v->ren) return 0;
    SDL_SetRenderDrawBlendMode(v->ren, SDL_BLENDMODE_BLEND);
    make_textures(v->ren, v->tex);
    return 1;
}

static void start_show(void)
{
    seed_state = opt_seed ? opt_seed * 0x9E3779B97F4A7C15ULL : ((uint64_t)time(NULL) << 20) ^ SDL_GetPerformanceCounter();
    if (!seed_state) seed_state = 1;
    for (int i = 0; i < 6; i++) next_rand();
    anim_t = 0;
    new_round();
}

static int snapshot(void)
{
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, opt_w, opt_h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *r = surf ? SDL_CreateSoftwareRenderer(surf) : NULL;
    if (!r) { fprintf(stderr, "wick: %s\n", SDL_GetError()); return 1; }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Texture *tex[NSPRITE];
    make_textures(r, tex);
    cur_tex = tex;
    start_show();
    for (double t = 0; t < opt_time; t += 1.0 / 60) update(1.0 / 60);
    draw_scene(r, opt_w, opt_h);
    SDL_RenderPresent(r);
    if (SDL_SaveBMP(surf, opt_snapshot) != 0) { fprintf(stderr, "wick: %s\n", SDL_GetError()); return 1; }
    return 0;
}

static void usage(void)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "The Wick",
        "The Wick screensaver\n\n"
        "  --window        run in a window (Esc or Q quits)\n"
        "  --speed X       animation speed, 1 = normal\n"
        "  --seed N        repeatable sequence\n"
        "  --snapshot FILE.bmp [--time SECONDS] [--size WxH]\n"
        "                  write a picture of the screensaver after SECONDS and exit\n", NULL);
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if ((a[0] == '/' || a[0] == '-') && (a[1] == 's' || a[1] == 'S') && !a[2]) continue;
        else if ((a[0] == '/' || a[0] == '-') && (a[1] == 'p' || a[1] == 'P') && (!a[2] || a[2] == ':')) return 0;
        else if ((a[0] == '/' || a[0] == '-') && (a[1] == 'c' || a[1] == 'C') && (!a[2] || a[2] == ':')) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "The Wick", "The Wick screensaver has no settings.", NULL);
            return 0;
        }
        else if (!strcmp(a, "--window")) opt_window = 1;
        else if (!strcmp(a, "--speed") && i + 1 < argc) opt_speed = SDL_atof(argv[++i]);
        else if (!strcmp(a, "--seed") && i + 1 < argc) opt_seed = SDL_strtoull(argv[++i], NULL, 10);
        else if (!strcmp(a, "--snapshot") && i + 1 < argc) opt_snapshot = argv[++i];
        else if (!strcmp(a, "--time") && i + 1 < argc) opt_time = SDL_atof(argv[++i]);
        else if (!strcmp(a, "--size") && i + 1 < argc) {
            const char *x = SDL_strchr(argv[++i], 'x');
            opt_w = SDL_atoi(argv[i]); opt_h = x ? SDL_atoi(x + 1) : 0;
            if (opt_w < 64 || opt_h < 64) { usage(); return 1; }
        }
        else { usage(); return 1; }
    }
    if (opt_speed < 0.05) opt_speed = 0.05;
    load_sprites();

    if (opt_snapshot) return snapshot();
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "wick: %s\n", SDL_GetError()); return 1; }

    View views[8];
    int nviews = 0, ndisp = opt_window ? 1 : SDL_GetNumVideoDisplays();
    if (ndisp < 1) ndisp = 1;
    for (int d = 0; d < ndisp && nviews < 8; d++)
        if (make_view(&views[nviews], d)) nviews++;
    if (!nviews) { fprintf(stderr, "wick: %s\n", SDL_GetError()); return 1; }
    if (!opt_window) SDL_ShowCursor(SDL_DISABLE);
    start_show();

    Uint64 freqc = SDL_GetPerformanceFrequency(), last = SDL_GetPerformanceCounter(), start = last;
    double moved = 0;
    int running = 1;
    while (running) {
        SDL_Event e;
        double since = (double)(SDL_GetPerformanceCounter() - start) / freqc;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) running = 0;
            else if (e.type == SDL_KEYDOWN) {
                if (!opt_window || e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q) running = 0;
            } else if (!opt_window && since > 1.0) {
                if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN) running = 0;
                else if (e.type == SDL_MOUSEMOTION) {
                    moved += abs(e.motion.xrel) + abs(e.motion.yrel);
                    if (moved > 24) running = 0;
                }
            }
        }
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last) / freqc;
        last = now;
        if (dt > 0.1) dt = 0.1;
        update(dt);
        for (int i = 0; i < nviews; i++) { size_view(&views[i]); render(&views[i]); }
        SDL_Delay(views[0].scene ? 1 : 12);
    }
    SDL_Quit();
    return 0;
}




