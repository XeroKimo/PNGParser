#include <span>
#include <iostream>
#include <fstream>
#include <array>
#include <bit>
#include <chrono>
#include <filesystem>
#include <tl/expected.hpp>

import PNGParser;

#undef main

static constexpr int benchmarkAttempts = 100;

void OutputTest(std::string file)
{
    for(int i = 0; i < benchmarkAttempts; i++)
    {
        std::fstream image{ file, std::ios::binary | std::ios::in };
        bool suceeded = true;
        if(image.is_open())
        {
            auto timePoint = std::chrono::steady_clock::now();
            ParsePNG(image).map_error([&suceeded](auto&) { suceeded = false; });
            auto end = std::chrono::steady_clock::now();

            if(!suceeded)
                std::cout << "Error occured. ";
            std::cout << "Time taken to parse: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - timePoint) << "\n";
        }
    }
}

int main()
{
    OutputTest("Soccer Chess 3.png");
    return 0;
}
