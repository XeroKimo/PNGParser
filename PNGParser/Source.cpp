#include <span>
#include <iostream>
#include <fstream>
#include <array>
#include <bit>
#include <chrono>

import PNGParser;

int main()
{
    std::fstream image{ "Korone_NotLikeThis.png", std::ios::binary | std::ios::in };

    if(image.is_open())
    {
        auto timePoint = std::chrono::steady_clock::now();
        ParsePNG(image);
        auto end = std::chrono::steady_clock::now();
        std::cout << "Time taken to parse: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - timePoint);

    }

    return 0;
}
