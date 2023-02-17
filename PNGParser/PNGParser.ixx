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
#include <cassert>

export module PNGParser;
import :PlatformDetection;
export import :ChunkParser;
export import :ChunkData;
import :PNGFilter0;
import :Image;
import :Adam7;

export struct Image2
{
    int width;
    int height;
    std::vector<Byte> imageBytes;
    int pitch;
    int bitDepth;
};

export Image2 ParsePNG(std::istream& stream);

std::size_t DecompressedImageSize(const ChunkData<"IHDR">& header)
{
    auto filter0 = [&header]()
    {
        switch(header.interlaceMethod)
        {
        case InterlaceMethod::None:
            return Filter0::ImageSize(header.ToImageInfo());
        case InterlaceMethod::Adam7:
        {
            Adam7::ImageInfos infos = Adam7::ImageInfos(header.ToImageInfo());
            size_t size = 0;
            for(size_t i = 0; i < Adam7::passCount; i++)
            {
                size += Filter0::ImageSize(infos.ToImageInfo(i));
            }
            return size;
        }
        }

        throw std::exception("Unknown interlace method");
    };

    switch(header.filterMethod)
    {
    case 0:
        return filter0();
    }

    throw std::exception("Unknown filter type");
}
