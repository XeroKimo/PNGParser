module;

#include <array>
#include <span>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <stdexcept>

export module PNGParser:Adam7;
import :PlatformDetection;
import :Image;

namespace Adam7
{
    inline constexpr size_t passCount = 7;
    inline constexpr std::array<std::int32_t, passCount> startingRow = { 0, 0, 4, 0, 2, 0, 1 };
    inline constexpr std::array<std::int32_t, passCount> startingCol = { 0, 4, 0, 2, 0, 1, 0 };
    inline constexpr std::array<std::int32_t, passCount> rowIncrement = { 8, 8, 8, 4, 4, 2, 2 };
    inline constexpr std::array<std::int32_t, passCount> columnIncrement = { 8, 8, 4, 4, 2, 2, 1 };
}

namespace Adam7::Internal
{
    bool Exists(std::int32_t width, std::int32_t height, size_t passIndex) noexcept
    {
        return width > startingCol[passIndex] && height > startingRow[passIndex];
    }

    constexpr std::int32_t Width(std::int32_t width, size_t passIndex) noexcept
    {
        std::int32_t wrapToNextBlock = columnIncrement[passIndex] - 1;

        return (width + wrapToNextBlock - startingCol[passIndex]) / columnIncrement[passIndex];
    }

    constexpr std::int32_t Height(std::int32_t height, size_t passIndex) noexcept
    {
        std::int32_t wrapToNextBlock = rowIncrement[passIndex] - 1;
        return (height + wrapToNextBlock - startingRow[passIndex]) / rowIncrement[passIndex];
    }
}

namespace Adam7
{
    size_t NoFilter(ImageInfo info)
    {
        return info.ImageSize();
    }

    struct ImageInfos
    {
        std::array<std::int32_t, passCount> widths;
        std::array<std::int32_t, passCount> heights;
        std::array<std::int32_t, passCount> scanlineSizes;
        PixelInfo pixelInfo;

        ImageInfos() = default;
        ImageInfos(::ImageInfo info) :
            pixelInfo(info.pixelInfo)
        {
            if(pixelInfo.bitDepth < 8)
            {
                for(size_t i = 0; i < passCount; i++)
                {
                    if(Internal::Exists(info.width, info.height, i))
                    {
                        widths[i] = Internal::Width(info.width, i);
                        heights[i] = Internal::Height(info.width, i);
                        scanlineSizes[i] = widths[i] / pixelInfo.PixelsPerByte() + (widths[i] % pixelInfo.PixelsPerByte() > 0);
                    }
                    else
                    {
                        widths[i] = 0;
                        heights[i] = 0;
                        scanlineSizes[i] = 0;
                    }
                }
            }
            else
            {
                for(size_t i = 0; i < passCount; i++)
                {
                    if(Internal::Exists(info.width, info.height, i))
                    {
                        widths[i] = Internal::Width(info.width, i);
                        heights[i] = Internal::Height(info.width, i);
                        scanlineSizes[i] = widths[i] * pixelInfo.BytesPerPixel();
                    }
                    else
                    {
                        widths[i] = 0;
                        heights[i] = 0;
                        scanlineSizes[i] = 0;
                    }
                }
            }
        }

        AnyError<ImageInfo> ToImageInfo(size_t passIndex) const
        {
            if(passIndex > heights.size())
                return tl::unexpected(std::make_shared<std::out_of_range>("Out of range access"));

            return ImageInfo{ pixelInfo, widths[passIndex], heights[passIndex] };
        }

        AnyError<std::size_t> ImageSize(size_t passIndex) const
        {
            if(passIndex > heights.size())
                return tl::unexpected(std::make_shared<std::out_of_range>("Out of range access"));
            return scanlineSizes[passIndex] * heights[passIndex];
        }

        template<std::invocable<ImageInfo> FilterFn>
        std::size_t TotalImageSize(FilterFn func = NoFilter) const noexcept
        {
            size_t size = 0;
            for(size_t i = 0; i < passCount; i++)
            {
                size += func(std::move(ToImageInfo(i)).value());
            }
            return size;
        }
    };
}