module;

#include <cstdint>
#include <string_view>
#include <memory>
#include <bit>
#include <array>
#include <algorithm>
#include <span>
#include <string>

export module PNGParser;
import :PlatformDetection;
export import :ChunkParser;
export import :ChunkData;
export import <istream>;

export void ParsePNG(std::istream& stream);

