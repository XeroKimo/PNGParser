module;

#include <cstdint>
#include <span>
#include <vector>
#include <stdexcept>
#include <cassert>

export module PNGParser:Image;
import :PlatformDetection;

enum class PixelRepresentation
{
    Bit,
    Byte
};

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

template<PixelRepresentation Pixel, std::unsigned_integral ByteTy>
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
        std::uint32_t scanlineWidth = width * bitDepth * subpixelCount / 8;
        std::span scanline = imageBytes.subspan(scanlineIndex * scanlineWidth, scanlineWidth);
        return Scanline(bitDepth, subpixelCount, width, scanline);
    }

    static std::pair<Scanline, Byte> FromFilteredImage(std::span<ByteTy> imageBytes, std::uint32_t width, std::uint32_t pitch, std::uint8_t bitDepth, std::uint8_t subpixelCount, size_t scanlineIndex)
    {
        std::uint32_t scanlineWidth = width * bitDepth * subpixelCount / 8;
        std::uint32_t filteredScanlineWidth = scanlineWidth + ChunkData<"IHDR">::filterByteCount;
        std::span scanline = imageBytes.subspan(scanlineIndex * filteredScanlineWidth, filteredScanlineWidth);

        return { Scanline(bitDepth, subpixelCount, width, scanline.subspan(1, scanlineWidth)), scanline[0] };
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
        if constexpr(Pixel == PixelRepresentation::Bit)
        {
            const Byte index = i / bitDepth;
            const Byte shift = i % bitDepth;
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

            return (bytes[i] & bitMask) >> (8 - shift);
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
};

template<FilterState Filter>
struct FilterInfo {};

template<>
struct FilterInfo<FilterState::Filtered>
{
    std::vector<Byte> filterBytes;
};

template<PixelRepresentation Pixel, FilterState Filter, InterlaceState Interlace>
struct Image : FilterInfo<Filter>
{
    std::uint8_t bitDepth;
    std::uint8_t subpixelCount;
    std::uint32_t width;
    std::uint32_t height;
    std::vector<Byte> bytes;

    std::uint32_t ImageSize() const noexcept
    {
        return bytes.size();
    }

    std::uint8_t BytesPerPixel() const noexcept
    {
        return std::max(1, bitDepth / 8) * subpixelCount;
    }

    std::uint32_t ScanlineSize() const noexcept
    {
        return width * BytesPerPixel();
    }

    Scanline<Pixel, Byte> GetScanline(size_t scanline)
    {
        return Scanline<Pixel, Byte>::FromImage(bytes, width, width, bitDepth, subpixelCount, scanline);
    }

    Scanline<Pixel, const Byte> GetScanline(size_t scanline) const noexcept
    {
        return Scanline<Pixel, Byte>::FromImage(bytes, width, width, bitDepth, subpixelCount, scanline);
    }

    std::span<Byte> GetPixel(std::uint32_t i)
    {
        return std::span(bytes).subspan(i * BytesPerPixel(), BytesPerPixel());
    }
};

void ExplodeScanline(Scanline<PixelRepresentation::Bit, const Byte> bitScanline, Scanline<PixelRepresentation::Bit, Byte> byteScanline)
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