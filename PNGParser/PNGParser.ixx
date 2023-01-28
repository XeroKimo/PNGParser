module;

#include <cstdint>
#include <string_view>
#include <memory>
#include <bit>
#include <array>
#include <algorithm>
#include <span>
#include <string>
#include <vector>
#include <istream>

export module PNGParser;
import :PlatformDetection;
export import :ChunkParser;
export import :ChunkData;

export struct Image
{
    int width;
    int height;
    std::vector<Byte> imageBytes;
    int pitch;
    int bitDepth;
};

export Image ParsePNG(std::istream& stream);

