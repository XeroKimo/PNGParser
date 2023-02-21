#include <span>
#include <iostream>
#include <fstream>
#include <array>
#include <bit>
#include <chrono>
#include <filesystem>

import PNGParser;

#undef main

static constexpr int benchmarkAttempts = 100;

void OutputTest(std::string file)
{
    for(int i = 0; i < benchmarkAttempts; i++)
    {
        std::fstream image{ file, std::ios::binary | std::ios::in };

        if(image.is_open())
        {
            auto timePoint = std::chrono::steady_clock::now();
            ParsePNG(image);
            auto end = std::chrono::steady_clock::now();
            std::cout << "Time taken to parse: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - timePoint) << "\n";
        }
    }
}

int main()
{
    OutputTest("Soccer Chess 3.png");
    return 0;
}
