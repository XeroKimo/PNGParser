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
            return Filter0::ImageSize(header.height, header.ScanlineSize());
        case InterlaceMethod::Adam7:
        {
            Adam7::ImageInfos infos = Adam7::ImageInfos(ImageInfo{ { header.bitDepth, header.SubpixelPerPixel() }, header.width, header.height });
            size_t size = 0;
            for(size_t i = 0; i < Adam7::passCount; i++)
            {
                size += Filter0::ImageSize(infos.heights[i], infos.scanlineSizes[i]);
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


enum class FilterState
{
    Defiltered,
    Filtered,
};

enum class InterlaceState
{
    Deinterlaced,
    Interlaced
};


template<std::unsigned_integral ByteTy>
    requires (sizeof(ByteTy) == 1)
struct Scanline
{
    std::uint8_t bitDepth;
    std::uint8_t subpixelCount;
    std::uint32_t width;
    std::span<ByteTy> bytes;

    Scanline(std::uint8_t bitDepth, std::uint8_t subpixelCount, std::uint32_t width, std::span<ByteTy> bytes) :
        bitDepth(bitDepth),
        subpixelCount(subpixelCount),
        width(width),
        bytes(bytes)
    {

    }

    static Scanline FromImage(std::span<ByteTy> imageBytes, std::uint32_t width, std::uint32_t pitch, std::uint8_t bitDepth, std::uint8_t subpixelCount, size_t scanlineIndex)
    {
        if(bitDepth < 8)
        {
            const std::uint32_t bitsPerPixel = bitDepth * subpixelCount;
            const std::uint32_t pixelsPerByte = 8 / bitsPerPixel;
            const size_t realScanlineIndex = scanlineIndex / pixelsPerByte;
            const std::uint32_t scanlinePitch = std::max<std::uint32_t>(pitch / pixelsPerByte, 1);
            const std::uint32_t scanlineWidth = std::max<std::uint32_t>(width / pixelsPerByte, 1);
            std::span scanline = imageBytes.subspan(realScanlineIndex * scanlinePitch, scanlineWidth);
            return Scanline(bitDepth, subpixelCount, width, scanline);
        }
        else
        {
            const std::uint32_t bytesPerPixel = std::max(bitDepth / 8, 1) * subpixelCount;
            const std::uint32_t scanlinePitch = pitch * bytesPerPixel;
            const std::uint32_t scanlineWidth = width * bytesPerPixel;
            std::span scanline = imageBytes.subspan(scanlineIndex * scanlinePitch, scanlineWidth);
            return Scanline(bitDepth, subpixelCount, width, scanline);
        }
    }

    static std::pair<Scanline, Byte> FromFilteredImage(std::span<ByteTy> imageBytes, std::uint32_t width, std::uint32_t pitch, std::uint8_t bitDepth, std::uint8_t subpixelCount, size_t scanlineIndex)
    {
        if(bitDepth < 8)
        {
            const std::uint32_t bitsPerPixel = bitDepth * subpixelCount;
            const std::uint32_t pixelsPerByte = 8 / bitsPerPixel;
            const size_t realScanlineIndex = scanlineIndex / pixelsPerByte;

            const std::uint32_t scanlinePitch = std::max<std::uint32_t>(pitch / pixelsPerByte, 1);
            const std::uint32_t filteredScanlinePitch = scanlinePitch + Filter0::filterByteCount;

            const std::uint32_t scanlineWidth = std::max<std::uint32_t>(width / pixelsPerByte, 1);
            const std::uint32_t filteredScanlineWidth = scanlineWidth + Filter0::filterByteCount;
            std::span scanline = imageBytes.subspan(realScanlineIndex * filteredScanlinePitch, filteredScanlineWidth);

            return { Scanline(bitDepth, subpixelCount, width, scanline.subspan(1, scanlineWidth)), scanline[0] };
        }
        else
        {
            const std::uint32_t bytesPerPixel = std::max(bitDepth / 8, 1) * subpixelCount;
            const std::uint32_t scanlinePitch = pitch * bytesPerPixel;
            const std::uint32_t filteredScanlinePitch = scanlinePitch + Filter0::filterByteCount;

            const std::uint32_t scanlineWidth = width * bytesPerPixel;
            const std::uint32_t filteredScanlineWidth = scanlineWidth + Filter0::filterByteCount;
            std::span scanline = imageBytes.subspan(scanlineIndex * filteredScanlinePitch, filteredScanlineWidth);

            return { Scanline(bitDepth, subpixelCount, width, scanline.subspan(1, scanlineWidth)), scanline[0] };
        }
    }

public:
    std::uint32_t TotalWidth() const noexcept
    {
        return width * PixelsPerByte();
    }

    std::uint32_t Pitch()
    {
        return bytes.size();
    }

    Byte GetByte(size_t i) const
    {
        if(bitDepth < 8)
        {
            const size_t subpixelPerByte = 8 / bitDepth;
            const size_t index = i / subpixelPerByte;
            const Byte shift = static_cast<Byte>(i % subpixelPerByte);
            const Byte bitMask = [this](std::uint8_t shift)
            {
                Byte bitMask = 0;
                if(bitDepth == 1)
                    bitMask |= 0b1000'0000;
                else if(bitDepth == 2)
                    bitMask |= 0b1100'0000;
                else
                    bitMask |= 0b1111'0000;

                return bitMask >> shift;
            }(shift);

            return (bytes[index] & bitMask) >> (7 - shift);
        }
        else
        {
            return bytes[i];
        }
    }

    std::span<ByteTy> GetPixel(size_t i) noexcept
    {
        assert(bitDepth >= 8);
        return bytes.subspan(i * BytesPerPixel(), BytesPerPixel());
    }

    std::span<const ByteTy> GetPixel(size_t i) const noexcept
    {
        assert(bitDepth >= 8);
        return bytes.subspan(i * BytesPerPixel(), BytesPerPixel());
    }

public:
    std::uint8_t BitsPerPixel() const noexcept
    {
        return bitDepth * subpixelCount;
    }

    std::uint8_t BytesPerPixel() const noexcept
    {
        return std::max(1, bitDepth / 8) * subpixelCount;
    }

    std::uint8_t PixelsPerByte() const noexcept
    {
        return 8 / BitsPerPixel();
    }

    operator Scanline<const ByteTy>() const noexcept { return Scanline<const ByteTy>(bitDepth, subpixelCount, width, bytes); }
};

template<FilterState Filter>
struct FilterInfo {};

template<>
struct FilterInfo<FilterState::Filtered>
{
    std::vector<Byte> filterBytes;
};

template<FilterState Filter, InterlaceState Interlace>
struct Image : FilterInfo<Filter>
{
    ImageInfo imageInfo;
    std::vector<Byte> bytes;

    std::size_t ImageSize() const noexcept
    {
        return bytes.size();
    }

    std::uint8_t BytesPerPixel() const noexcept
    {
        return imageInfo.pixelInfo.BytesPerPixel();
    }

    std::uint32_t ScanlineSize() const noexcept
    {
        return imageInfo.width * BytesPerPixel();
    }

    Scanline<Byte> GetScanline(size_t scanline)
    {
        return Scanline<Byte>::FromImage(bytes, imageInfo.width, imageInfo.width, imageInfo.pixelInfo.bitDepth, imageInfo.pixelInfo.subpixelCount, scanline);
    }

    Scanline<const Byte> GetScanline(size_t scanline) const
    {
        return Scanline<const Byte>::FromImage(bytes, imageInfo.width, imageInfo.width, imageInfo.pixelInfo.bitDepth, imageInfo.pixelInfo.subpixelCount, scanline);
    }

    std::span<Byte> GetPixel(std::uint32_t i)
    {
        return std::span(bytes).subspan(i * BytesPerPixel(), BytesPerPixel());
    }
};

void ExplodeScanline(Scanline<const Byte> bitScanline, Scanline<Byte> byteScanline)
{
    if(bitScanline.BytesPerPixel() != byteScanline.BytesPerPixel())
        throw std::exception("Format incompatible");

    if(bitScanline.width < byteScanline.width)
        throw std::exception("Not enough bytes");

    for(std::uint32_t i = 0; i < byteScanline.width; i++)
    {
        byteScanline.bytes[i] = bitScanline.GetByte(i);
    }
}