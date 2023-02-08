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
#include <algorithm>

export module PNGParser:ChunkData;
import :ChunkParser;
import :PlatformDetection;
import :Image;
import :ColorTypeDescription;

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


enum class InterlaceMethod : std::uint8_t
{
    None = 0,
    Adam7 = 1
};

template<>
struct ChunkTraits<"IHDR">
{
    static constexpr ChunkType identifier = "IHDR";
    static constexpr std::string_view name = "Image Header";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::int32_t width;
        std::int32_t height;
        std::int8_t bitDepth;
        ColorType colorType;
        std::int8_t compressionMethod;
        std::int8_t filterMethod;
        InterlaceMethod interlaceMethod;

        std::int8_t SubpixelCount() const
        {
            for(const ColorFormatView& format : standardColorFormats)
            {
                if(colorType == format.type)
                    return format.subpixelCount;
            }

            throw std::exception("Unexpected pixel type");
        }

        ImageInfo ToImageInfo() const
        {
            return { PixelInfo(bitDepth, SubpixelCount()), width, height };
        }

        std::int8_t BitsPerPixel() const
        {
            return ToImageInfo().pixelInfo.BitsPerPixel();
        }

        std::int8_t BytesPerPixel() const 
        {
            return ToImageInfo().pixelInfo.BytesPerPixel();
        }

        std::int8_t PixelsPerByte() const 
        {
            return ToImageInfo().pixelInfo.PixelsPerByte();
        }

        std::size_t ScanlineSize() const
        {
            return ToImageInfo().ScanlineSize();
        }

        std::size_t ImageSize() const
        {
            return ToImageInfo().ImageSize();
        }
    };

    static constexpr size_t maxSize = 13;

    static void VerifyParsedData(const Data& data)
    {
        auto it = std::find_if(standardColorFormats.begin(), standardColorFormats.end(), [data](const ColorFormatView& format) { return format.type == data.colorType; });
        
        if(it == standardColorFormats.end())
            throw std::runtime_error("Unexpected color type: " + std::to_string(static_cast<int>(data.colorType)));
        
        auto matchBitdepth = [data](int bitDepth) { return data.bitDepth == bitDepth; };

        if(std::none_of(it->allowBitDepths.begin(), it->allowBitDepths.end(), matchBitdepth))
        {
            throw std::runtime_error("Unsupported bit depth color type: " + std::string(it->name) + " bitDepth: " + std::to_string(static_cast<int>(data.colorType)));
        }
    }

    static Data Parse(ChunkDataInputStream& stream)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.width = stream.ReadNative<std::uint32_t>();
        data.height = stream.ReadNative<std::uint32_t>();
        data.bitDepth = stream.ReadNative<std::int8_t>();
        data.colorType = stream.ReadNative<ColorType>();
        data.compressionMethod = stream.ReadNative<std::int8_t>();
        data.filterMethod = stream.ReadNative<std::int8_t>();
        data.interlaceMethod = stream.ReadNative<InterlaceMethod>();

        VerifyParsedData(data);

        return data;
    }
};

template<>
struct ChunkTraits<"PLTE">
{
    static constexpr ChunkType identifier = "PLTE";
    static constexpr std::string_view name = "Palette";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = false;

    static constexpr std::size_t maxEntries = 256;

    struct Data
    {
        using ColorEntry = std::array<std::uint8_t, 3>;
        std::array<ColorEntry, maxEntries> colorPalette;
    };

    static Data Parse(ChunkDataInputStream& stream)
    {
        if(stream.ChunkSize() % 3 > 0)
            throw std::runtime_error("Palette chunk has unexpected size");

        Data data;
        for(size_t i = 0; i < maxEntries && stream.HasUnreadData(); i++)
        {
            data.colorPalette[i] = stream.Read<3>();
        }
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

    static constexpr size_t maxSize = std::numeric_limits<int>::max() - 1;

    static constexpr size_t maxSlidingWindowSize = 32768;

    static Data Parse(ChunkDataInputStream& stream)
    {
        if(stream.ChunkSize() > maxSize)
            throw std::runtime_error("Data chunk max size exceeds the standard supported max size");

        Data data;
        data.bytes.reserve(stream.ChunkSize());
        while(stream.HasUnreadData())
        {
            data.bytes.push_back(stream.ReadNative<1>()[0]);
        }
        return data;
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

        data.gamma = stream.ReadNative<std::uint32_t>();

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

        data.renderingIntent = stream.ReadNative<Byte >();

        return data;
    }
};