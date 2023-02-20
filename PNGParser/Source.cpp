#include <span>
#include <iostream>
#include <fstream>
#include <array>
#include <bit>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <filesystem>

import PNGParser;

#undef main

void TestImageParser()
{
    for(auto dir_entry : std::filesystem::directory_iterator("Test Images"))
    {
        std::cout << dir_entry.path() << "\n";
        try
        {
             std::fstream image{ dir_entry.path(), std::ios::binary | std::ios::in };
             ParsePNG(image);
             std::cout << "\n";
        }
        catch(const std::exception& e)
        {
            std::cout << "Failed to parse image: " << dir_entry.path() << "\nError: " << e.what() << "\n\n";
        }
    }
}

void OutputTest(std::string file)
{

    std::fstream image{ file, std::ios::binary | std::ios::in };

    Image2 im;
    if(image.is_open())
    {
        auto timePoint = std::chrono::steady_clock::now();
        im = ParsePNG(image);
        auto end = std::chrono::steady_clock::now();
        std::cout << "Time taken to parse: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - timePoint);

    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer(800, 600, SDL_WindowFlags::SDL_WINDOW_OPENGL, &window, &renderer);

    auto er = SDL_GetError();
    SDL_Event e;

    
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(im.imageBytes.data(), im.width, im.height, im.bitDepth, im.pitch, 0x00'00'00'ff, 0x00'00'ff'00, 0x00'ff'00'00, 0xff'00'00'00);
    er = SDL_GetError();
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

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
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }
    }
}

int main()
{
    TestImageParser();
    //OutputTest("Test Images/ps1n0g08.png");
    return 0;
}
