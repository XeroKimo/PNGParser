module;

#include <cstdint>
#include <bit>
#include <string>
#include <string_view>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>
#include <array>
#include <istream>
#include <cassert>

export module PNGParser:ChunkData;
import :ChunkParser;
import :PlatformDetection;

constexpr bool IsUppercase(Byte b)
{
    return b >= 'A' && b <= 'Z';
}

constexpr bool IsLowercase(Byte b)
{
    return b >= 'a' && b <= 'z';
}

template<size_t N>
struct ConstString
{
    using c_string_ref = const char(&)[N + 1];

    std::array<char, N> string;

    constexpr ConstString() = default;
    constexpr ConstString(std::array<char, N> string) :
        string(string)
    {
    }
    constexpr ConstString(std::array<unsigned char, N> string) :
        string(std::bit_cast<std::array<char, N>>(string))
    {
    }
    constexpr ConstString(const char(&string)[N + 1])
    {
        std::copy(std::begin(string), std::end(string) - 1, std::begin(this->string));
    }

    constexpr char& At(size_t index) { return string.at(index); }
    constexpr const char& At(size_t index) const { return string.at(index); }

    constexpr auto begin() noexcept { return string.begin(); }
    constexpr auto end() noexcept { return string.end(); }

    constexpr auto begin() const noexcept { return string.begin(); }
    constexpr auto end() const noexcept { return string.end(); }

    constexpr char& operator[](size_t index) noexcept { return string[index]; }
    constexpr const char& operator[](size_t index) const noexcept { return string[index]; }

    constexpr bool operator==(const ConstString& rh) const noexcept { return string == rh.string; }
    constexpr bool operator!=(const ConstString& rh) const noexcept { return string != rh.string; }
};

template<size_t N>
ConstString(const char (&string)[N]) -> ConstString<N - 1>;

export struct ChunkType
{
    ConstString<4> identifier;

    constexpr ChunkType() = default;
    constexpr ChunkType(std::array<char, 4> string) :
        identifier(string)
    {
        VerifyString();
    }
    constexpr ChunkType(std::array<unsigned char, 4> string) :
        identifier(string)
    {
        VerifyString();
    }
    constexpr ChunkType(ConstString<4> string) :
        identifier(string)
    {
        VerifyString();
    }

    constexpr ChunkType(ConstString<4>::c_string_ref string) :
        identifier(string)
    {
        VerifyString();
    }

    constexpr std::string_view ToString() const noexcept { return { &identifier[0], identifier.string.size() }; }

    constexpr bool IsCritical() const noexcept { return IsUppercase(identifier[0]); }
    constexpr bool IsPrivate() const noexcept { return IsUppercase(identifier[1]); }
    constexpr bool Copyable() const noexcept { return !IsUppercase(identifier[3]); }

    constexpr operator std::uint32_t() noexcept { return std::bit_cast<std::uint32_t>(identifier.string); }
    constexpr operator const std::uint32_t() const noexcept { return std::bit_cast<std::uint32_t>(identifier.string); }

    constexpr bool operator==(const ChunkType& rh) const noexcept { return identifier == rh.identifier; }
    constexpr bool operator!=(const ChunkType& rh) const noexcept { return identifier != rh.identifier; }

private:
    constexpr void VerifyString()
    {
        for(auto byte : identifier.string)
        {
            if(!(IsUppercase(byte) || IsLowercase(byte)))
                throw std::exception("String must only contain Alphabetical characters");
        }
    }
};

constexpr ChunkType operator""_ct(const char* string, size_t n)
{
    if(n > 4)
        throw std::exception("Expected string size to be 4");

    ConstString<4> constString;
    std::copy_n(string, 4, constString.begin());
    return ChunkType(constString);
}

export template<ChunkType Ty>
struct ChunkTraits;

export template<ChunkType Ty>
struct ChunkTraits;

export template<ChunkType Ty>
using ChunkData = ChunkTraits<Ty>::Data;

enum class ColorType : std::uint8_t
{
    GreyScale = 0,
    TrueColor = 2,
    IndexedColor = 3,
    GreyscaleWithAlpha = 4,
    TruecolorWithAlpha = 6
};

enum class InterlaceMethod : std::uint8_t
{
    None = 0,
    Adam7 = 1
};

namespace Adam7
{
    inline constexpr size_t passCount = 7;
    inline constexpr std::array<std::int32_t, passCount> startingRow = { 0, 0, 4, 0, 2, 0, 1 };
    inline constexpr std::array<std::int32_t, passCount> startingCol = { 0, 4, 0, 2, 0, 1, 0 };
    inline constexpr std::array<std::int32_t, passCount> rowIncrement = { 8, 8, 8, 4, 4, 2, 2 };
    inline constexpr std::array<std::int32_t, passCount> columnIncrement = { 8, 8, 4, 4, 2, 2, 1 };
}

template<>
struct ChunkTraits<"IHDR">
{
    static constexpr ChunkType identifier = "IHDR";
    static constexpr std::string_view name = "Image Header";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        static constexpr std::uint32_t filterByteCount = 1;

        std::uint32_t width;
        std::uint32_t height;
        std::int8_t bitDepth;
        ColorType colorType;
        std::int8_t compressionMethod;
        std::int8_t filterMethod;
        InterlaceMethod interlaceMethod;

        std::uint32_t SubpixelPerPixel() const
        {
            using enum ColorType;
            switch(colorType)
            {
            case GreyScale:
                return 1;
            case TrueColor:
                return 3;
            case IndexedColor:
                return 1;
            case GreyscaleWithAlpha:
                return 2;
            case TruecolorWithAlpha:
                return 4;
            }

            throw std::exception("Unexpected pixel type");
        }

        std::uint32_t BitsPerPixel() const
        {
            return bitDepth * SubpixelPerPixel();
        }

        std::uint32_t BytesPerPixel() const 
        {
            return std::max(bitDepth / 8, 1) * SubpixelPerPixel();
        }

        std::uint32_t FilteredScanlineSize() const
        {
            return filterByteCount + ScanlineSize();
        }

        std::uint32_t FilteredImageSize() const
        {
            if(interlaceMethod == InterlaceMethod::None)
                return FilteredScanlineSize() * height;
            else
            {
                std::uint32_t size = 0;
                for(size_t i = 0; i < Adam7::columnIncrement.size(); i++)
                {
                    size += Adam7FilteredImageSize(i);
                }
                return size;
            }
        }

        std::uint32_t FilterByte(std::size_t scanline) const
        {
            return FilteredScanlineSize() * scanline;
        }

        std::span<Byte> ScanlineSubspanNoFilterByte(std::span<Byte> decompressedImageData, std::size_t scanline) const
        {
            return decompressedImageData.subspan(FilteredScanlineSize() * scanline + 1, ScanlineSize());
        }

        std::uint32_t ScanlineSize() const
        {
            return (width * BytesPerPixel());
        }

        std::uint32_t ImageSize() const
        {
            return (ScanlineSize() * height);
        }

        std::uint32_t Adam7ReducedWidth(size_t index) const
        {
            assert(index < Adam7::passCount);
            return width / Adam7::columnIncrement[index] * static_cast<std::uint32_t>(Adam7ReducedImageExists(index));
        }

        std::uint32_t Adam7ReducedHeight(size_t index) const
        {
            assert(index < Adam7::passCount);
            return height / Adam7::rowIncrement[index] * static_cast<std::uint32_t>(Adam7ReducedImageExists(index));
        }

        std::uint32_t Adam7FilteredScanlineSize(size_t index) const
        {
            assert(index < Adam7::passCount);
            return (Adam7ReducedWidth(index) * BytesPerPixel()) + filterByteCount;
        }

        std::uint32_t Adam7FilteredImageSize(size_t index) const
        {
            assert(index < Adam7::passCount);
            return (Adam7FilteredScanlineSize(index) * Adam7ReducedHeight(index));
        }

        std::uint32_t Adam7ScanlineSize(size_t index) const
        {
            assert(index < Adam7::passCount);
            return (Adam7ReducedWidth(index) * BytesPerPixel());
        }

        std::uint32_t Adam7ImageSize(size_t index) const
        {
            assert(index < Adam7::passCount);
            return (Adam7ScanlineSize(index) * Adam7ReducedHeight(index));
        }

        bool Adam7ReducedImageExists(size_t index) const
        {
            assert(index < Adam7::passCount);
            return width > Adam7::startingCol[index] && height > Adam7::startingRow[index];
        }

        std::span<Byte> Adam7ReducedImageSpan(std::span<Byte> decompressedImageData, size_t index) const
        {
            assert(index < Adam7::passCount);
            size_t offset = 0;
            for(size_t i = 1; i <= index; i++)
                offset += Adam7FilteredImageSize(i - 1);
            return decompressedImageData.subspan(offset, Adam7FilteredImageSize(index));
        }

        std::uint32_t Adam7DeinterlacedPixelIndex(std::uint32_t x, std::uint32_t y, std::uint32_t reducedWidth, size_t pass) const
        {
            std::uint32_t yStart = (Adam7::startingRow[pass] * width * y + Adam7::rowIncrement[pass] * Adam7::columnIncrement[pass] * y) * BytesPerPixel();
            std::uint32_t xStart = (Adam7::startingCol[pass] + Adam7::columnIncrement[pass] * x) * BytesPerPixel();
            return xStart + yStart;
        }
    };

    static constexpr size_t maxSize = 13;

    static Data Parse(ChunkDataInputStream& stream)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.width = stream.Read<std::uint32_t>();
        data.height = stream.Read<std::uint32_t>();
        data.bitDepth = stream.Read<std::int8_t>();
        data.colorType = stream.Read<ColorType>();
        data.compressionMethod = stream.Read<std::int8_t>();
        data.filterMethod = stream.Read<std::int8_t>();
        data.interlaceMethod = stream.Read<InterlaceMethod>();

        return data;
    }
};

template<std::unsigned_integral ByteTy>
    requires (sizeof(ByteTy) == 1)
class FilteredScanline
{
private:
    std::uint8_t m_bitDepth;
    std::uint8_t m_subpixelCount;
    std::span<ByteTy> m_bytes;

public:
    FilteredScanline(std::uint8_t bitDepth, std::uint8_t subpixelCount, std::span<ByteTy> bytes) noexcept :
        m_bitDepth(bitDepth),
        m_subpixelCount(subpixelCount),
        m_bytes(bytes)
    {

    }

    static FilteredScanline FromFilteredImage(std::span<ByteTy> imageBytes, std::uint32_t width, std::uint8_t bitDepth, std::uint8_t subpixelCount, size_t scanlineIndex)
    {
        std::uint32_t scanlineWidth = width * bitDepth * subpixelCount / 8;
        std::uint32_t filteredScanlineWidth = scanlineWidth + ChunkData<"IHDR">::filterByteCount;
        return FilteredScanline(bitDepth, subpixelCount, imageBytes.subspan(scanlineIndex * filteredScanlineWidth + ChunkData<"IHDR">::filterByteCount, scanlineWidth));
    }

public:
    std::uint8_t GetFilterByte() const noexcept
    {
        return m_bytes[0];
    }

    std::uint8_t GetBytesPerPixel() const noexcept
    {
        return std::max(m_bitDepth / 8, 1) * m_subpixelCount;
    }

    std::span<ByteTy> GetPixel(size_t i) noexcept
    {
        assert(m_bitDepth >= 8);
        return m_bytes.subspan(i * GetBytesPerPixel() + ChunkData<"IHDR">::filterByteCount, GetBytesPerPixel());
    }

    std::span<const ByteTy> GetPixel(size_t i) const noexcept
    {
        assert(m_bitDepth >= 8);
        return m_bytes.subspan(i * GetBytesPerPixel() + ChunkData<"IHDR">::filterByteCount, GetBytesPerPixel());
    }

    std::uint32_t GetWidth() const noexcept
    {
        return (m_bitDepth < 8) ? m_bytes.size() : m_bytes.size() / GetBytesPerPixel();
    }

    std::span<ByteTy> GetData() noexcept
    {
        return m_bytes;
    }

    std::span<const ByteTy> GetData() const noexcept
    {
        return m_bytes;
    }
};

template<std::unsigned_integral ByteTy>
    requires (sizeof(ByteTy) == 1)
class Scanline
{
private:
    std::uint8_t m_bitDepth;
    std::uint8_t m_subpixelCount;
    std::span<ByteTy> m_bytes;

public:
    Scanline(std::uint8_t bitDepth, std::uint8_t subpixelCount, std::span<ByteTy> bytes) noexcept :
        m_bitDepth(bitDepth),
        m_subpixelCount(subpixelCount),
        m_bytes(bytes)
    {

    }

    static Scanline FromImage(std::span<ByteTy> imageBytes, std::uint32_t width, std::uint8_t bitDepth, std::uint8_t subpixelCount, size_t scanlineIndex)
    {
        std::uint32_t scanlineWidth = width * bitDepth * subpixelCount / 8;
        return Scanline(bitDepth, subpixelCount, imageBytes.subspan(scanlineIndex * scanlineWidth, scanlineWidth));
    }

    static Scanline FromFilteredImage(std::span<ByteTy> imageBytes, std::uint32_t width, std::uint8_t bitDepth, std::uint8_t subpixelCount, size_t scanlineIndex)
    {
        std::uint32_t scanlineWidth = width * bitDepth * subpixelCount / 8;
        std::uint32_t filteredScanlineWidth = scanlineWidth + ChunkData<"IHDR">::filterByteCount;
        return Scanline(bitDepth, subpixelCount, imageBytes.subspan(scanlineIndex * filteredScanlineWidth + ChunkData<"IHDR">::filterByteCount, scanlineWidth));
    }

public:
    std::uint8_t GetBytesPerPixel() const noexcept
    {
        return std::max(m_bitDepth / 8, 1) * m_subpixelCount;
    }

    std::span<ByteTy> GetPixel(size_t i) noexcept
    {
        assert(m_bitDepth >= 8);
        return m_bytes.subspan(i * GetBytesPerPixel(), GetBytesPerPixel());
    }

    std::span<const ByteTy> GetPixel(size_t i) const noexcept
    {
        assert(m_bitDepth >= 8);
        return m_bytes.subspan(i * GetBytesPerPixel(), GetBytesPerPixel());
    }

    std::uint32_t GetWidth() const noexcept
    {
        return (m_bitDepth < 8) ? m_bytes.size() : m_bytes.size() / GetBytesPerPixel();
    }

    std::span<ByteTy> GetData() noexcept
    {
        return m_bytes;
    }

    std::span<const ByteTy> GetData() const noexcept
    {
        return m_bytes;
    }
};

struct FilteredReducedImageView
{
    const ChunkData<"IHDR">* header;
    std::uint32_t width;
    std::uint32_t height;
    std::span<Byte> bytes;

    std::uint32_t FilteredScanlineSize() const
    {
        return ChunkData<"IHDR">::filterByteCount + ScanlineSize();
    }

    std::uint32_t FilteredImageSize() const
    {
        return FilteredScanlineSize() * height;
    }

    Byte FilterByte(std::size_t scanline) const
    {
        auto index = FilteredScanlineSize() * scanline;
        return bytes[FilteredScanlineSize() * scanline];
    }

    Scanline<Byte> GetScanline(std::size_t scanline)
    {
        return Scanline<Byte>::FromFilteredImage(bytes, width, header->bitDepth, header->SubpixelPerPixel(), scanline);
    }

    Scanline<const Byte> GetScanline(std::size_t scanline) const
    {
        return Scanline<const Byte>::FromFilteredImage(bytes, width, header->bitDepth, header->SubpixelPerPixel(), scanline);
    }

    FilteredScanline<Byte> GetFilteredScanline(std::size_t scanline)
    {
        return FilteredScanline<Byte>::FromFilteredImage(bytes, width, header->bitDepth, header->SubpixelPerPixel(), scanline);
    }

    FilteredScanline<const Byte> GetFilteredScanline(std::size_t scanline) const
    {
        return FilteredScanline<const Byte>::FromFilteredImage(bytes, width, header->bitDepth, header->SubpixelPerPixel(), scanline);
    }

    std::uint32_t ScanlineSize() const
    {
        return (width * header->BytesPerPixel());
    }

    std::uint32_t ImageSize() const
    {
        return (ScanlineSize() * height);
    }
};

struct ReducedImage
{
    const ChunkData<"IHDR">* header;
    std::uint32_t width;
    std::uint32_t height;
    std::vector<Byte> bytes;

    ::Scanline<Byte> Scanline(std::size_t scanline)
    {
        return ::Scanline<Byte>::FromImage(bytes, width, header->bitDepth, header->SubpixelPerPixel(), scanline);
    }

    ::Scanline<const Byte> Scanline(std::size_t scanline) const
    {
        return ::Scanline<const Byte>::FromImage(bytes, width, header->bitDepth, header->SubpixelPerPixel(), scanline);
    }

    std::uint32_t ScanlineSize() const
    {
        return (width * header->BytesPerPixel());
    }

    std::uint32_t ImageSize() const
    {
        return (ScanlineSize() * height);
    }

    std::span<Byte> GetPixel(std::uint32_t i)
    {
        return std::span(bytes).subspan(i * header->BytesPerPixel(), header->BytesPerPixel());
    }
};

template<>
struct ChunkTraits<"gAMA">
{
    static constexpr ChunkType identifier = "gAMA";
    static constexpr std::string_view name = "Image Gamma";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;


    struct Data
    {
        std::uint32_t gamma;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static Data Parse(ChunkDataInputStream& stream)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.gamma = stream.Read<std::uint32_t>();

        return data;
    }
};

template<>
struct ChunkTraits<"sRGB">
{
    static constexpr ChunkType identifier = "sRGB";
    static constexpr std::string_view name = "Standard RGB Color Space";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        Byte renderingIntent;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static Data Parse(ChunkDataInputStream& stream)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.renderingIntent = stream.Read<Byte >();

        return data;
    }
};

template<>
struct ChunkTraits<"IDAT">
{
    static constexpr ChunkType identifier = "IDAT";
    static constexpr std::string_view name = "Image Data";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = true;

    struct Data
    {
        std::vector<Byte> bytes;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static constexpr size_t maxSlidingWindowSize = 32768;

    static Data Parse(ChunkDataInputStream& stream)
    {
        Data data;
        data.bytes.reserve(stream.ChunkSize());
        while(stream.HasUnreadData())
        {
            data.bytes.push_back(stream.Read<1>()[0]);
        }
        return data;
    }
};