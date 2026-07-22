#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>

#define HEIGHT 500
#define WIDTH 500

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("GAME",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       HEIGHT, WIDTH, 0);

    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, render_flags);

    // Section 2: SDL image loader
    SDL_Surface* surface = IMG_Load("cat_boss.png");
    if (!surface) {
        printf("Failed to load image: %s\n", IMG_GetError());
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, surface);
    SDL_FreeSurface(surface);

    SDL_Rect dest;
    SDL_QueryTexture(tex, NULL, NULL, &dest.w, &dest.h);

    dest.w /= 6;
    dest.h /= 6;

    // Centering based on window dimensions (HEIGHT/WIDTH are 500)
    dest.x = (WIDTH - dest.w) / 2;
    dest.y = (HEIGHT - dest.h) / 2;

    // Game loop control flag
    bool close = false;
    SDL_Event event;

    while (!close) {
        // --- 1. Event Handling ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                close = true; // Set flag to break out of loop when X is pressed!
            }
        }

        // --- 2. Render ---
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255); // Set background to black
        SDL_RenderClear(rend);
        SDL_RenderCopy(rend, tex, NULL, &dest);
        SDL_RenderPresent(rend);

        // Cap to ~60 FPS so CPU usage doesn't spike to 100%
        SDL_Delay(16);
    }

    // --- Section 5: Freeing resources ---
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}