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
#include <memory>
#include "tl/expected.hpp"

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

export AnyError<Image2> ParsePNG(std::istream& stream);

AnyError<std::size_t> DecompressedImageSize(const ChunkData<"IHDR"_ct>& header)
{
    auto filter0 = [&header]() -> AnyError<std::size_t>
    {
        switch(header.interlaceMethod)
        {
        case InterlaceMethod::None:
        {
            if(auto imageInfo = header.ToImageInfo(); imageInfo)
                return Filter0::ImageSize(std::move(imageInfo).value());
            else
                return tl::unexpected(std::move(imageInfo).error());
        }
        case InterlaceMethod::Adam7:
        {
            if(auto imageInfo = header.ToImageInfo(); imageInfo)
            {
                Adam7::ImageInfos infos = Adam7::ImageInfos(std::move(imageInfo).value());
                return infos.TotalImageSize(Filter0::ImageSize);
            }
            else
                return tl::unexpected(std::move(imageInfo).error());
        }
        }

        return tl::unexpected(PNGError::Unknown_Interlace_Method);
    };

    switch(header.filterMethod)
    {
    case 0:
        return filter0();
    }

    return tl::unexpected(PNGError::Unknown_Filter_Type);
}
