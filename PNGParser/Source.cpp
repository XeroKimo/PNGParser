#include <span>
#include <iostream>
#include <fstream>
#include <array>
#include <bit>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
import PNGParser;
#undef main;
int main()
{
    std::fstream image{ "Korone_NotLikeThis2.png", std::ios::binary | std::ios::in };

    Image im;
    if(image.is_open())
    {
        auto timePoint = std::chrono::steady_clock::now();
        im = ParsePNG(image);
        auto end = std::chrono::steady_clock::now();
        std::cout << "Time taken to parse: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - timePoint);

    }


    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer(800, 600, SDL_WindowFlags::SDL_WINDOW_OPENGL, &window, &renderer);

    auto er = SDL_GetError();
    SDL_Event e;

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(im.imageBytes.data(), im.width, im.height, 24, im.pitch, 0x00'00'00'FF, 0x00'00'FF'00, 0x00'FF'00'00, 0);
    er = SDL_GetError();
    SDL_Surface* surf = IMG_Load("Korone_NotLikeThis2.png");
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Texture* texture2 = SDL_CreateTextureFromSurface(renderer, surface);

    while(true)
    {
        if(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
                break;
        }
        else
        {
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture2, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }
    }

    return 0;
}
