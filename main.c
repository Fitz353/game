#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

/* ── Window & maze constants ─────────────────────────────────────────────── */
#define WIN_W       500
#define WIN_H       600
#define MAZE_COLS   15
#define MAZE_ROWS   15
#define CELL_SIZE   32
#define WALL_T      3   /* wall thickness in pixels */

#define MAZE_OFF_X  ((WIN_W - MAZE_COLS * CELL_SIZE) / 2)
#define MAZE_OFF_Y  ((WIN_H - MAZE_ROWS * CELL_SIZE) / 2)

/* Movement animation duration in milliseconds */
#define MOVE_DURATION 120

/* ── Wall bit-flags ──────────────────────────────────────────────────────── */
#define WALL_N  (1 << 0)
#define WALL_S  (1 << 1)
#define WALL_E  (1 << 2)
#define WALL_W  (1 << 3)
#define VISITED (1 << 4)

/* ── Maze state ──────────────────────────────────────────────────────────── */
static int maze[MAZE_ROWS][MAZE_COLS];

/* Direction arrays: N  S  E  W */
static const int DX[]        = { 0,  0,  1, -1 };
static const int DY[]        = {-1,  1,  0,  0 };
static const int WFROM[]     = { WALL_N, WALL_S, WALL_E, WALL_W };
static const int WTO[]       = { WALL_S, WALL_N, WALL_W, WALL_E };

/* ── Cat animation state ─────────────────────────────────────────────────── */
typedef struct {
    int   grid_x, grid_y;     /* current logical cell */
    float draw_x, draw_y;     /* pixel position for rendering */
    float from_x, from_y;     /* animation start pixel position */
    float to_x,   to_y;       /* animation target pixel position */
    bool  moving;             /* currently animating? */
    Uint32 move_start;        /* tick when move animation began */
} Cat;

static float cell_px(int cx) { return (float)(MAZE_OFF_X + cx * CELL_SIZE); }
static float cell_py(int cy) { return (float)(MAZE_OFF_Y + cy * CELL_SIZE); }

/* Smooth ease-out curve: fast start, gentle arrival */
static float ease_out(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/* ── Maze generation (recursive backtracker) ─────────────────────────────── */
static void shuffle4(int *a) {
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

static void carve(int x, int y) {
    maze[y][x] |= VISITED;
    int dirs[] = {0, 1, 2, 3};
    shuffle4(dirs);
    for (int i = 0; i < 4; i++) {
        int d  = dirs[i];
        int nx = x + DX[d], ny = y + DY[d];
        if (nx < 0 || nx >= MAZE_COLS || ny < 0 || ny >= MAZE_ROWS) continue;
        if (maze[ny][nx] & VISITED) continue;
        maze[y][x]   &= ~WFROM[d];
        maze[ny][nx] &= ~WTO[d];
        carve(nx, ny);
    }
}

static void gen_maze(void) {
    for (int y = 0; y < MAZE_ROWS; y++)
        for (int x = 0; x < MAZE_COLS; x++)
            maze[y][x] = WALL_N | WALL_S | WALL_E | WALL_W;
    carve(0, 0);
    for (int y = 0; y < MAZE_ROWS; y++)
        for (int x = 0; x < MAZE_COLS; x++)
            maze[y][x] &= ~VISITED;
}

/* ── Rendering helpers ───────────────────────────────────────────────────── */
static void fill_cell(SDL_Renderer *r, int x, int y, int pad,
                       Uint8 R, Uint8 G, Uint8 B, Uint8 A) {
    SDL_SetRenderDrawColor(r, R, G, B, A);
    SDL_Rect rect = {
        MAZE_OFF_X + x * CELL_SIZE + pad,
        MAZE_OFF_Y + y * CELL_SIZE + pad,
        CELL_SIZE - pad * 2,
        CELL_SIZE - pad * 2
    };
    SDL_RenderFillRect(r, &rect);
}

static void draw_maze(SDL_Renderer *rend) {
    /* Floor */
    for (int y = 0; y < MAZE_ROWS; y++)
        for (int x = 0; x < MAZE_COLS; x++)
            fill_cell(rend, x, y, 0, 18, 10, 35, 255);

    /* Walls */
    SDL_SetRenderDrawColor(rend, 100, 60, 200, 255);
    for (int y = 0; y < MAZE_ROWS; y++) {
        for (int x = 0; x < MAZE_COLS; x++) {
            int px = MAZE_OFF_X + x * CELL_SIZE;
            int py = MAZE_OFF_Y + y * CELL_SIZE;
            SDL_Rect w;
            if (maze[y][x] & WALL_N) {
                w = (SDL_Rect){px, py, CELL_SIZE, WALL_T};
                SDL_RenderFillRect(rend, &w);
            }
            if (maze[y][x] & WALL_S) {
                w = (SDL_Rect){px, py + CELL_SIZE - WALL_T, CELL_SIZE, WALL_T};
                SDL_RenderFillRect(rend, &w);
            }
            if (maze[y][x] & WALL_E) {
                w = (SDL_Rect){px + CELL_SIZE - WALL_T, py, WALL_T, CELL_SIZE};
                SDL_RenderFillRect(rend, &w);
            }
            if (maze[y][x] & WALL_W) {
                w = (SDL_Rect){px, py, WALL_T, CELL_SIZE};
                SDL_RenderFillRect(rend, &w);
            }
        }
    }
}

/* ── Entry point ─────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    srand((unsigned)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("IMG_Init: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window   *win  = SDL_CreateWindow("Cat Maze",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             WIN_W, WIN_H, 0);
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

    SDL_Surface *surf = IMG_Load("cat_boss.png");
    if (!surf) { printf("IMG_Load: %s\n", IMG_GetError()); return 1; }
    SDL_Texture *cat_tex = SDL_CreateTextureFromSurface(rend, surf);
    SDL_FreeSurface(surf);

    gen_maze();

    Cat cat = {
        .grid_x = 0, .grid_y = 0,
        .draw_x = cell_px(0), .draw_y = cell_py(0),
        .from_x = cell_px(0), .from_y = cell_py(0),
        .to_x   = cell_px(0), .to_y   = cell_py(0),
        .moving = false, .move_start = 0
    };

    bool   game_won = false;
    bool   running  = true;
    SDL_Event event;
    Uint32 win_tick = 0;

    while (running) {
        Uint32 now = SDL_GetTicks();

        /* ── Update animation ── */
        if (cat.moving) {
            float elapsed = (float)(now - cat.move_start);
            float t = elapsed / (float)MOVE_DURATION;
            if (t >= 1.0f) {
                t = 1.0f;
                cat.moving = false;
                cat.draw_x = cat.to_x;
                cat.draw_y = cat.to_y;

                /* Check win after arriving */
                if (cat.grid_x == MAZE_COLS - 1 && cat.grid_y == MAZE_ROWS - 1) {
                    game_won = true;
                    win_tick = now;
                }
            } else {
                float e = ease_out(t);
                cat.draw_x = lerp(cat.from_x, cat.to_x, e);
                cat.draw_y = lerp(cat.from_y, cat.to_y, e);
            }
        }

        /* ── Events ── */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { running = false; break; }

            if (event.type == SDL_KEYDOWN) {
                SDL_Keycode k = event.key.keysym.sym;

                if (k == SDLK_ESCAPE) { running = false; break; }

                /* R = new maze */
                if (k == SDLK_r) {
                    gen_maze();
                    cat.grid_x = 0; cat.grid_y = 0;
                    cat.draw_x = cell_px(0); cat.draw_y = cell_py(0);
                    cat.from_x = cell_px(0); cat.from_y = cell_py(0);
                    cat.to_x   = cell_px(0); cat.to_y   = cell_py(0);
                    cat.moving = false;
                    game_won = false;
                    continue;
                }

                /* Only accept movement if not mid-animation and not won */
                if (!game_won && !cat.moving) {
                    int nx = cat.grid_x, ny = cat.grid_y, d = -1;
                    switch (k) {
                        case SDLK_UP:    case SDLK_w: ny--; d = 0; break;
                        case SDLK_DOWN:  case SDLK_s: ny++; d = 1; break;
                        case SDLK_RIGHT: case SDLK_d: nx++; d = 2; break;
                        case SDLK_LEFT:  case SDLK_a: nx--; d = 3; break;
                        default: break;
                    }
                    if (d >= 0 &&
                        nx >= 0 && nx < MAZE_COLS &&
                        ny >= 0 && ny < MAZE_ROWS &&
                        !(maze[cat.grid_y][cat.grid_x] & WFROM[d]))
                    {
                        /* Start slide animation */
                        cat.from_x = cat.draw_x;
                        cat.from_y = cat.draw_y;
                        cat.grid_x = nx;
                        cat.grid_y = ny;
                        cat.to_x   = cell_px(nx);
                        cat.to_y   = cell_py(ny);
                        cat.moving = true;
                        cat.move_start = now;
                    }
                }
            }
        }

        /* ── Draw ── */
        /* Background */
        SDL_SetRenderDrawColor(rend, 8, 4, 18, 255);
        SDL_RenderClear(rend);

        draw_maze(rend);

        /* Start cell — green glow (after maze so floor doesn't cover it) */
        fill_cell(rend, 0, 0, 1, 15, 130, 50, 255);
        fill_cell(rend, 0, 0, 4, 30, 180, 70, 180);

        /* Exit cell — gold glow */
        fill_cell(rend, MAZE_COLS - 1, MAZE_ROWS - 1, 1, 200, 150, 10, 255);
        fill_cell(rend, MAZE_COLS - 1, MAZE_ROWS - 1, 4, 255, 210, 40, 180);

        /* Cat sprite — rendered at animated pixel position */
        {
            int pad = 2;
            /* Subtle bounce: slight scale pulse during movement */
            int bounce = 0;
            if (cat.moving) {
                float elapsed = (float)(now - cat.move_start);
                float t = elapsed / (float)MOVE_DURATION;
                bounce = (int)(3.0f * sinf(t * 3.14159f));
            }
            SDL_Rect cat_dst = {
                (int)cat.draw_x + pad - bounce,
                (int)cat.draw_y + pad - bounce,
                CELL_SIZE - pad * 2 + bounce * 2,
                CELL_SIZE - pad * 2 + bounce * 2
            };
            SDL_RenderCopy(rend, cat_tex, NULL, &cat_dst);
        }

        /* Win overlay */
        if (game_won) {
            float t = (float)(now - win_tick) / 1000.0f;
            /* Pulsing dark overlay */
            Uint8 alpha = (Uint8)(140 + 40 * sinf(t * 3.0f));
            SDL_SetRenderDrawColor(rend, 0, 0, 0, alpha);
            SDL_Rect overlay = {0, 0, WIN_W, WIN_H};
            SDL_RenderFillRect(rend, &overlay);

            /* Big cat in center */
            SDL_Rect big = {WIN_W/2 - 72, WIN_H/2 - 72, 144, 144};
            SDL_RenderCopy(rend, cat_tex, NULL, &big);

            /* Golden ring around cat */
            Uint8 pulse = (Uint8)(200 + 55 * sinf(t * 4.0f));
            SDL_SetRenderDrawColor(rend, pulse, 180, 20, 200);
            for (int b = 0; b < 4; b++) {
                SDL_Rect ring = {big.x - b, big.y - b,
                                 big.w + b*2, big.h + b*2};
                SDL_RenderDrawRect(rend, &ring);
            }

            /* Hint bar: press R */
            SDL_SetRenderDrawColor(rend, 100, 60, 200, 255);
            SDL_Rect hint = {WIN_W/2 - 60, WIN_H/2 + 90, 120, 6};
            SDL_RenderFillRect(rend, &hint);
        }

        SDL_RenderPresent(rend);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(cat_tex);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    IMG_Quit();
    SDL_Quit();
    return 0;
}