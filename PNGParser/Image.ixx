module;

#include <cstdint>
#include <span>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <cmath>

export module PNGParser:Image;
import :PlatformDetection;


struct PixelInfo
{
    std::int8_t bitDepth;
    std::int8_t subpixelCount;

    std::int8_t BytesPerPixel() const noexcept
    {
        return bitDepth / 8 * subpixelCount;
    }

    std::int8_t BitsPerPixel() const noexcept
    {
        return bitDepth * subpixelCount;
    }

    std::int8_t PixelsPerByte() const noexcept
    {
        return SubpixelPerByte() * subpixelCount;
    }

    std::int8_t SubpixelPerByte() const noexcept
    {
        return (8 / bitDepth);
    }

    PixelInfo ExplodedPixelFormat() const noexcept
    {
        return { (bitDepth < 8) ? std::int8_t{ 8 } : bitDepth, subpixelCount };
    }
};

struct ImageInfo
{
    PixelInfo pixelInfo;
    std::int32_t width;
    std::int32_t height;

    std::size_t ScanlineSize() const noexcept
    {
        if(pixelInfo.bitDepth >= 8)
        {
            return width * pixelInfo.BytesPerPixel();
        }
        else
        {
            return width / pixelInfo.PixelsPerByte() + (width % pixelInfo.PixelsPerByte() > 0);
        }
    }

    std::size_t ImageSize() const noexcept
    {
        return ScanlineSize() * height;
    }
};