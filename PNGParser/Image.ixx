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

    constexpr std::int8_t BytesPerPixel() const noexcept
    {
        return bitDepth / 8 * subpixelCount;
    }

    constexpr std::int8_t BitsPerPixel() const noexcept
    {
        return bitDepth * subpixelCount;
    }

    constexpr std::int8_t PixelsPerByte() const noexcept
    {
        return SubpixelPerByte() * subpixelCount;
    }

    constexpr std::int8_t SubpixelPerByte() const noexcept
    {
        return (8 / bitDepth);
    }

    constexpr PixelInfo ExplodedPixelFormat() const noexcept
    {
        return { (bitDepth < 8) ? std::int8_t{ 8 } : bitDepth, subpixelCount };
    }
};

struct ImageInfo
{
    PixelInfo pixelInfo;
    std::int32_t width;
    std::int32_t height;

    constexpr std::size_t ScanlineSize() const noexcept
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

    constexpr std::size_t ImageSize() const noexcept
    {
        return ScanlineSize() * height;
    }
};



template<std::unsigned_integral ByteTy>
    requires (sizeof(ByteTy) == 1)
struct Scanline
{
    PixelInfo pixelInfo;
    std::uint32_t width;
    std::span<ByteTy> bytes;

    Scanline(PixelInfo pixelInfo, std::uint32_t width, std::span<ByteTy> bytes) :
        pixelInfo(pixelInfo),
        width(width),
        bytes(bytes)
    {

    }

    static Scanline FromImage(std::span<ByteTy> imageBytes, ImageInfo info, size_t scanlineIndex)
    {
        std::span scanline = imageBytes.subspan(scanlineIndex * info.ScanlineSize(), info.ScanlineSize());
        return Scanline(info.pixelInfo, info.width, scanline);
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
        if (pixelInfo.bitDepth < 8)
        {
            const size_t subpixelPerByte = pixelInfo.SubpixelPerByte();
            const size_t index = i / subpixelPerByte;
            const Byte shift = static_cast<Byte>(i % subpixelPerByte) * pixelInfo.BitsPerPixel();
            const Byte bitMask = [this](std::uint8_t shift)
            {
                Byte bitMask = 0;
                if (pixelInfo.bitDepth == 1)
                    bitMask |= 0b1000'0000;
                else if (pixelInfo.bitDepth == 2)
                    bitMask |= 0b1100'0000;
                else
                    bitMask |= 0b1111'0000;

                return bitMask >> shift;
            }(shift);

            return (bytes[index] & bitMask) >> (8 - pixelInfo.BitsPerPixel() - shift);
        }
        else
        {
            return bytes[i];
        }
    }

    void SetByte(Byte byte, size_t i) const
    {
        if (pixelInfo.bitDepth < 8)
        {
            const size_t subpixelPerByte = pixelInfo.SubpixelPerByte();
            const size_t index = i / subpixelPerByte;
            const Byte shift = static_cast<Byte>(i % subpixelPerByte) * pixelInfo.BitsPerPixel();
            const Byte bitMask = [this](std::uint8_t shift)
            {
                Byte bitMask = 0;
                if (pixelInfo.bitDepth == 1)
                    bitMask |= 0b1000'0000;
                else if (pixelInfo.bitDepth == 2)
                    bitMask |= 0b1100'0000;
                else
                    bitMask |= 0b1111'0000;

                return bitMask >> shift;
            }(shift);

            return (bytes[index] & ~bitMask) | ((byte << (8 - pixelInfo.BitsPerPixel() - shift)) & bitMask);
        }
        else
        {
            bytes[i] = byte;
        }
    }

    std::span<ByteTy> GetPixel(size_t i) noexcept
    {
        assert(pixelInfo.bitDepth >= 8);
        return bytes.subspan(i * BytesPerPixel(), BytesPerPixel());
    }

    std::span<const ByteTy> GetPixel(size_t i) const noexcept
    {
        assert(pixelInfo.bitDepth >= 8);
        return bytes.subspan(i * BytesPerPixel(), BytesPerPixel());
    }

public:
    std::uint8_t BitsPerPixel() const noexcept
    {
        return pixelInfo.BitsPerPixel();
    }

    std::uint8_t BytesPerPixel() const noexcept
    {
        return pixelInfo.BytesPerPixel();
    }

    std::uint8_t PixelsPerByte() const noexcept
    {
        return pixelInfo.PixelsPerByte();
    }

    operator Scanline<const ByteTy>() const noexcept { return Scanline<const ByteTy>(pixelInfo, width, bytes); }
};

void ExplodeScanline(Scanline<const Byte> bitScanline, Scanline<Byte> byteScanline)
{
    assert(bitScanline.pixelInfo.ExplodedPixelFormat().BytesPerPixel() != byteScanline.BytesPerPixel());
    assert(bitScanline.width < byteScanline.width);

    for (std::uint32_t i = 0; i < byteScanline.width; i++)
    {
        byteScanline.bytes[i] = bitScanline.GetByte(i);
    }
}

struct Image
{
    ImageInfo imageInfo;
    std::vector<Byte> bytes;

public:
    Image(ImageInfo info) :
        imageInfo(info)
    {
        bytes.resize(info.ImageSize());
    }
    Image(ImageInfo info, std::vector<Byte> bytes) :
        imageInfo(info),
        bytes(std::move(bytes))
    {
        assert(bytes.size() == info.ImageSize());
    }

    std::uint32_t BitDepth() const noexcept
    {
        return imageInfo.pixelInfo.bitDepth;
    }

    std::uint32_t BitsPerPixel() const noexcept
    {
        return imageInfo.pixelInfo.BitsPerPixel();
    }

    std::uint32_t Height() const noexcept
    {
        return imageInfo.height;
    }

    std::uint32_t Width() const noexcept
    {
        return imageInfo.width;
    }

    std::size_t ImageSize() const noexcept
    {
        return imageInfo.ImageSize();
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
        return Scanline<Byte>::FromImage(bytes, imageInfo, scanline);
    }

    Scanline<const Byte> GetScanline(size_t scanline) const
    {
        return Scanline<const Byte>::FromImage(bytes, imageInfo, scanline);
    }

    std::span<Byte> GetPixel(std::uint32_t i)
    {
        return std::span(bytes).subspan(i * BytesPerPixel(), BytesPerPixel());
    }
};