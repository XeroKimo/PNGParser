#include <span>
#include <iostream>
#include <fstream>
#include <array>
#include <bit>
#include <chrono>
#include <filesystem>
#include <numeric>
#include "tl/expected.hpp"

import PNGParser;

#undef main

static constexpr int benchmarkAttempts = 100;

void OutputTest(std::string file)
{
    std::unique_ptr<std::array<std::chrono::nanoseconds, benchmarkAttempts>> attempts = std::make_unique<std::array<std::chrono::nanoseconds, benchmarkAttempts>>();
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
            (*attempts)[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - timePoint);
            std::cout << "Time taken to parse: " << (*attempts)[i] << "\n";
        }
    }

    std::cout << "Min: " << *std::min_element(attempts->begin(), attempts->end()) << "\n";
    std::cout << "Max: " << *std::max_element(attempts->begin(), attempts->end()) << "\n";
    std::cout << "Average Time to parse: " << std::accumulate(attempts->begin(), attempts->end(), std::chrono::nanoseconds{}) / benchmarkAttempts << "\n";
}

int main()
{
    OutputTest("Soccer Chess 3.png");
    return 0;
}
