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
#include <optional>
#include <variant>

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

public:
    constexpr ChunkType() = default;
    friend constexpr ChunkType operator""_ct(const char* string, size_t n);

public:
    static AnyError<ChunkType> Create(std::array<char, 4> string)
    {
        ChunkType type;
        type.identifier = string;
        if(auto value = type.VerifyString(); !value)
            return tl::unexpected(std::move(value).error());

        return type;
    }
    static AnyError<ChunkType> Create(std::array<unsigned char, 4> string)
    {
        ChunkType type;
        type.identifier = string;
        if(auto value = type.VerifyString(); !value)
            return tl::unexpected(std::move(value).error());

        return type;
    }
    static AnyError<ChunkType> Create(ConstString<4> string)
    {
        ChunkType type;
        type.identifier = string;
        if(auto value = type.VerifyString(); !value)
            return tl::unexpected(std::move(value).error());

        return type;
    }
                               
    static AnyError<ChunkType> Create(ConstString<4>::c_string_ref string)
    {
        ChunkType type;
        type.identifier = ConstString<4>{ string };
        if(auto value = type.VerifyString(); !value)
            return tl::unexpected(std::move(value).error());

        return type;
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
    AnyError<void> VerifyString()
    {
        for(auto byte : identifier.string)
        {
            if(!(IsUppercase(byte) || IsLowercase(byte)))
                tl::unexpected(std::make_unique<std::exception>("String must only contain Alphabetical characters"));
        }
        return {};
    }
};

constexpr ChunkType operator""_ct(const char* string, size_t n)
{
    if(n > 4)
        throw std::exception("Expected string size to be 4");

    ConstString<4> constString;
    std::copy_n(string, 4, constString.begin());

    ChunkType type;
    type.identifier = constString;
    return type;
}

namespace ChunkIdentifiers
{
    constexpr ChunkType header = "IHDR"_ct;
    constexpr ChunkType palette = "PLTE"_ct;
    constexpr ChunkType imageData = "IDAT"_ct;
    constexpr ChunkType imageTrailer = "IEND"_ct;
    constexpr ChunkType chromaticities = "cHRM"_ct;
    constexpr ChunkType imageGamma = "gAMA"_ct;
    constexpr ChunkType iccProfile = "iCCP"_ct;
    constexpr ChunkType significantBits = "sBIT"_ct;
    constexpr ChunkType rgbColorSpace = "sRGB"_ct;
    constexpr ChunkType backgroundColor = "bKGD"_ct;
    constexpr ChunkType imageHistogram = "hIST"_ct;
    constexpr ChunkType transparency = "tRNS"_ct;
    constexpr ChunkType physicalPixelDimensions = "pHYs"_ct;
    constexpr ChunkType suggestedPalette = "sPLT"_ct;
    constexpr ChunkType lastModificationTime = "tIME"_ct;
    constexpr ChunkType internationalTextualData = "iTXt"_ct;
    constexpr ChunkType texturalData = "tEXt"_ct;
    constexpr ChunkType compressedTextualData = "zTXt"_ct;
}

static constexpr auto textDataStrings = std::to_array<std::string_view>({
    "Title",
    "Author",
    "Description",
    "Copyright",
    "Creation Time",
    "Software",
    "Disclaimer",
    "Warning",
    "Source",
    "Comment" });

/// <summary>
/// Reads a c-string from chunk data input stream
/// </summary>
/// <param name="stream"></param>
/// <param name="maxSize">Maximum size that will be read, null terminator included</param>
/// <returns></returns>
AnyError<std::string> ReadCString(ChunkDataInputStream& stream, std::size_t maxSize = std::numeric_limits<std::size_t>::max())
{
    std::string string;
    AnyError<char> letter = stream.Read<char>();
    for(size_t i = 0; i < maxSize && (letter.has_value()); i++, letter = stream.Read<char>())
    {
        string.push_back(std::move(letter).value());
    }

    if(!letter)
        return tl::unexpected(std::move(letter).error());

    if(std::move(letter).value() != 0)
        return tl::unexpected(PNGError::String_Not_Null_Terminated);

    return string;
}


AnyError<std::string> ReadString(ChunkDataInputStream& stream)
{
    std::string string;
    for(AnyError<char> letter = stream.Read<char>(); stream.HasUnreadData(); letter = stream.Read<char>())
    {
        if(letter.has_value())
            string.push_back(std::move(letter).value());
        else
            return tl::unexpected(std::move(letter).error());
    }

    return string;
}

export template<ChunkType Ty>
struct ChunkTraits;

export template<ChunkType Ty>
struct ChunkTraits;

export template<ChunkType Ty>
using ChunkData = ChunkTraits<Ty>::Data;

template<ChunkType Ty, bool Optional, bool Multiple>
struct ChunkContainerImpl;

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, false, false>
{
    using type = ChunkTraits<Ty>::Data;
};

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, false, true>
{
    using type = std::vector<typename ChunkTraits<Ty>::Data>;
};

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, true, false>
{
    using type = std::optional<typename ChunkTraits<Ty>::Data>;
};

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, true, true>
{
    using type = std::vector<typename ChunkTraits<Ty>::Data>;
};

struct DecodedChunks;

enum class InterlaceMethod : std::uint8_t
{
    None = 0,
    Adam7 = 1
};

template<>
struct ChunkTraits<"IHDR"_ct>
{
    static constexpr ChunkType identifier = "IHDR"_ct;
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

        AnyError<std::int8_t> SubpixelCount() const
        {
            for(const ColorFormatView& format : standardColorFormats)
            {
                if(colorType == format.type)
                    return format.subpixelCount;
            }

            return tl::unexpected(PNGError::Unsupported_Color_Format);
        }

        AnyError<ImageInfo> ToImageInfo() const
        {
            if(auto subpixelValue = SubpixelCount(); subpixelValue)
                return ImageInfo{ PixelInfo(bitDepth, std::move(subpixelValue).value()), width, height };
            else
                return tl::unexpected(std::move(subpixelValue).error());
        }

        AnyError<std::int8_t> BitsPerPixel() const
        {
            if(auto imageInfo = ToImageInfo(); imageInfo)
                return std::move(imageInfo).value().pixelInfo.BitsPerPixel();
            else
                return tl::unexpected(std::move(imageInfo).error());
        }

        AnyError<std::int8_t> BytesPerPixel() const
        {
            if(auto imageInfo = ToImageInfo(); imageInfo)
                return std::move(imageInfo).value().pixelInfo.BytesPerPixel();
            else
                return tl::unexpected(std::move(imageInfo).error());
        }

        AnyError<std::int8_t> PixelsPerByte() const
        {
            if(auto imageInfo = ToImageInfo(); imageInfo)
                return std::move(imageInfo).value().pixelInfo.PixelsPerByte();
            else
                return tl::unexpected(std::move(imageInfo).error());
        }

        AnyError<std::size_t> ScanlineSize() const
        {
            if(auto imageInfo = ToImageInfo(); imageInfo)
                return std::move(imageInfo).value().ScanlineSize();
            else
                return tl::unexpected(std::move(imageInfo).error());
        }

        AnyError<std::size_t> ImageSize() const
        {
            if(auto imageInfo = ToImageInfo(); imageInfo)
                return std::move(imageInfo).value().ImageSize();
            else
                return tl::unexpected(std::move(imageInfo).error());
        }
    };

    static constexpr size_t maxSize = 13;

    static AnyError<void> VerifyParsedData(const Data& data)
    {
        auto it = std::find_if(standardColorFormats.begin(), standardColorFormats.end(), [data](const ColorFormatView& format) { return format.type == data.colorType; });
        
        if(it == standardColorFormats.end())
            return tl::unexpected(PNGError::Unsupported_Color_Format);
        
        auto matchBitdepth = [data](int bitDepth) { return data.bitDepth == bitDepth; };

        if(std::none_of(it->allowBitDepths.begin(), it->allowBitDepths.end(), matchBitdepth))
        {
            return tl::unexpected(PNGError::Unsupported_Color_Format);
        }

        return {};
    }

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.width = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.height = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::int8_t>(); value)
            data.bitDepth = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<ColorType>(); value)
            data.colorType = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::int8_t>(); value)
            data.compressionMethod = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::int8_t>(); value)
            data.filterMethod = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<InterlaceMethod>(); value)
            data.interlaceMethod = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = VerifyParsedData(data); !value)
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"PLTE"_ct>
{
    static constexpr ChunkType identifier = "PLTE"_ct;
    static constexpr std::string_view name = "Palette";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = false;

    static constexpr std::size_t maxEntries = 256;

    struct Data
    {
        using ColorEntry = std::array<std::uint8_t, 3>;
        std::array<ColorEntry, maxEntries> colorPalette;
    };

    static constexpr size_t maxSize = Data{}.colorPalette.size() * sizeof(Data::ColorEntry::value_type);

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() % 3 > 0)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;
        for(size_t i = 0; i < maxEntries && stream.HasUnreadData(); i++)
        {
            if(auto value = stream.Read<3>(); value)
                data.colorPalette[i] = std::move(value).value();
            else
                return tl::unexpected(std::move(value).error());
        }

        return data;
    }
};

template<>
struct ChunkTraits<"IDAT"_ct>
{
    static constexpr ChunkType identifier = "IDAT"_ct;
    static constexpr std::string_view name = "Image Data";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = true;

    struct Data
    {
        std::vector<Byte> bytes;
    };

    static constexpr size_t maxSize = std::numeric_limits<int>::max() - 1;

    static constexpr size_t maxSlidingWindowSize = 32768;

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;
        data.bytes.reserve(stream.ChunkSize());
        while(stream.HasUnreadData())
        {
            if(auto value = stream.Read<Byte>(); value)
                data.bytes.push_back(std::move(value).value());
            else
                return tl::unexpected(std::move(value).error());
        }
        return data;
    }
};

template<>
struct ChunkTraits<"IEND"_ct>
{
    static constexpr ChunkType identifier = "IEND"_ct;
    static constexpr std::string_view name = "Image Trailer";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
    };

    static constexpr size_t maxSize = 0;

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        return {};
    }
};

template<>
struct ChunkTraits<"cHRM"_ct>
{
    static constexpr ChunkType identifier = "cHRM"_ct;
    static constexpr std::string_view name = "Chromaticities";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::uint32_t whitePointX;
        std::uint32_t whitePointY;
        std::uint32_t redPointX;
        std::uint32_t redPointY;
        std::uint32_t greenPointX;
        std::uint32_t greenPointY;
        std::uint32_t bluePointX;
        std::uint32_t bluePointY;
    };

    static constexpr size_t maxSize = sizeof(std::uint32_t) * 8;

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.whitePointX = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.whitePointY = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.redPointX = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.redPointY = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.greenPointX = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.greenPointY = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.bluePointX = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.bluePointY = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"gAMA"_ct>
{
    static constexpr ChunkType identifier = "gAMA"_ct;
    static constexpr std::string_view name = "Image Gamma";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::uint32_t gamma;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.gamma = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"iCCP"_ct>
{
    static constexpr ChunkType identifier = "iCCP"_ct;
    static constexpr std::string_view name = "ICC Profile";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::string profileName;
        Byte compressionMethod;
        std::vector<Byte> compressedProfile;
    };

    static constexpr size_t maxSize = std::numeric_limits<std::int32_t>::max();

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        if(auto value = ReadCString(stream, 80); value)
            data.profileName = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<Byte>(); value)
            data.compressionMethod = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        data.compressedProfile.reserve(stream.UnreadSize());

        while(stream.HasUnreadData())
        {
            if(auto value = stream.ReadNative<Byte>(); value)
                data.compressedProfile.push_back(std::move(value).value());
            else
                return tl::unexpected(std::move(value).error());
        }

        return data;
    }
};

template<>
struct ChunkTraits<"sBIT"_ct>
{
    static constexpr ChunkType identifier = "sBIT"_ct;
    static constexpr std::string_view name = "Significant Bits";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        Bytes<4> bits;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        for(size_t i = 0; i < stream.ChunkSize(); i++)
        {
            if(auto value = stream.ReadNative<Byte>(); value)
                data.bits[i] = std::move(value).value();
            else
                return tl::unexpected(std::move(value).error());
        }

        return data;
    }
};

template<>
struct ChunkTraits<"sRGB"_ct>
{
    static constexpr ChunkType identifier = "sRGB"_ct;
    static constexpr std::string_view name = "Standard RGB Color Space";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        Byte renderingIntent;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        if(auto value = stream.ReadNative<Byte>(); value)
            data.renderingIntent = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"bKGD"_ct>
{
    static constexpr ChunkType identifier = "bKGD"_ct;
    static constexpr std::string_view name = "Background Color";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        using GreyscaleBackground = std::uint16_t;
        using ColorBackground = std::array<std::uint16_t, 3>;
        using PaletteBackground = Byte;
        std::variant<GreyscaleBackground, ColorBackground, PaletteBackground> color;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        if(stream.ChunkSize() == sizeof(Data::GreyscaleBackground))
        {
            if(auto value = stream.ReadNative<Data::GreyscaleBackground>(); value)
                data.color = std::move(value).value();
            else
                return tl::unexpected(std::move(value).error());
        }
        else if(stream.ChunkSize() == Data::ColorBackground{}.size() * sizeof(Data::ColorBackground::value_type))
        {
            Data::ColorBackground color;
            for(size_t i = 0; i < color.size(); i++)
            {
                if(auto value = stream.ReadNative<Data::ColorBackground::value_type>(); value)
                    data.color = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());
            }
            data.color = color;
        }
        else if(stream.ChunkSize() == sizeof(Data::PaletteBackground))
        {
            if(auto value = stream.ReadNative<Data::PaletteBackground>(); value)
                data.color = std::move(value).value();
            else
                return tl::unexpected(std::move(value).error());
        }
        else
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        return data;
    }
};

template<>
struct ChunkTraits<"hIST"_ct>
{
    static constexpr ChunkType identifier = "hIST"_ct;
    static constexpr std::string_view name = "Image Histogram";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::array<std::uint16_t, ChunkTraits<"PLTE"_ct>::maxEntries> frequency;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize || (stream.ChunkSize() % sizeof(std::uint16_t) > 0))
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        for(size_t i = 0; stream.HasUnreadData(); i++)
        {
            if(auto value = stream.ReadNative<std::uint16_t>(); value)
                data.frequency[i] = std::move(value).value();
            else
                return tl::unexpected(std::move(value).error());
        }

        return data;
    }
};

template<>
struct ChunkTraits<"tRNS"_ct>
{
    static constexpr ChunkType identifier = "tRNS"_ct;
    static constexpr std::string_view name = "Transparency";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::vector<Byte> alphaValues;
    };

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;
        data.alphaValues.reserve(stream.ChunkSize());
        while(stream.HasUnreadData())
        {
            if(auto value = stream.Read<Byte>(); value)
                data.alphaValues.push_back(std::move(value).value());
            else
                return tl::unexpected(std::move(value).error());
        }
        return {};
    }
};

template<>
struct ChunkTraits<"pHYs"_ct>
{
    static constexpr ChunkType identifier = "pHYs"_ct;
    static constexpr std::string_view name = "Physical Pixel Dimensions";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::uint32_t pixelsPerUnitX;
        std::uint32_t pixelsPerUnitY;
        std::uint8_t unit;
    };

    static constexpr size_t maxSize = 9;

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.pixelsPerUnitX = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint32_t>(); value)
            data.pixelsPerUnitY = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint8_t>(); value)
            data.unit = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"sPLT"_ct>
{
    static constexpr ChunkType identifier = "sPLT"_ct;
    static constexpr std::string_view name = "Suggested Palette";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = true;

    struct Data
    {
        struct Sample
        {
            std::uint16_t red;
            std::uint16_t green;
            std::uint16_t blue;
            std::uint16_t alpha;
            std::uint16_t frequency;
        };

        std::string name;
        std::uint8_t sampleDepth;
        std::vector<Sample> palette;
    };


    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        if(auto value = ReadCString(stream, 80); value)
            data.name = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.Read<std::uint8_t>(); value)
            data.sampleDepth = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(data.sampleDepth == 8 && stream.UnreadSize() % 6 == 0)
        {
            while(stream.HasUnreadData())
            {
                Data::Sample sample;

                if(auto value = stream.ReadNative<std::uint8_t>(); value)
                    sample.red = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint8_t>(); value)
                    sample.green = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint8_t>(); value)
                    sample.blue = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint8_t>(); value)
                    sample.alpha = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint16_t>(); value)
                    sample.frequency = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                data.palette.push_back(sample);
            }
        }
        else if(data.sampleDepth == 16 && stream.UnreadSize() % 10 == 0)
        {
            while(stream.HasUnreadData())
            {
                Data::Sample sample;

                if(auto value = stream.ReadNative<std::uint16_t>(); value)
                    sample.red = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint16_t>(); value)
                    sample.green = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint16_t>(); value)
                    sample.blue = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint16_t>(); value)
                    sample.alpha = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                if(auto value = stream.ReadNative<std::uint16_t>(); value)
                    sample.frequency = std::move(value).value();
                else
                    return tl::unexpected(std::move(value).error());

                data.palette.push_back(sample);
            }
        }
        else
            return tl::unexpected(PNGError::Unsupported_Color_Format);

        return data;
    }
};

template<>
struct ChunkTraits<"tIME"_ct>
{
    static constexpr ChunkType identifier = "tIME"_ct;
    static constexpr std::string_view name = "Last Modification Time";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::uint16_t year;
        std::uint8_t month;
        std::uint8_t day;
        std::uint8_t hour;
        std::uint8_t minute;
        std::uint8_t second;
    };

    static constexpr size_t maxSize = 7;

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            return tl::unexpected(PNGError::Chunk_Expected_Size_Exceeded);

        Data data;

        if(auto value = stream.ReadNative<std::uint16_t>(); value)
            data.year = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint8_t>(); value)
            data.month = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint8_t>(); value)
            data.day = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint8_t>(); value)
            data.hour = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint8_t>(); value)
            data.minute = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.ReadNative<std::uint8_t>(); value)
            data.second = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"iTXt"_ct>
{
    static constexpr ChunkType identifier = "iTXt"_ct;
    static constexpr std::string_view name = "International Text Data";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::string keyword;
        std::uint8_t compressionFlag;
        std::uint8_t compressionMethod;
        std::string languageTag;
        std::string translatedKeyword;
        std::string text;
    };

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        //TODO: Add keyword error handling here? or when we want to actually construct the text data

        if(auto value = ReadCString(stream, 80); value)
            data.keyword = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.Read<std::uint8_t>(); value)
            data.compressionFlag = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.Read<std::uint8_t>(); value)
            data.compressionMethod = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = ReadCString(stream); value)
            data.languageTag = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = ReadCString(stream); value)
            data.translatedKeyword = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = ReadString(stream); value)
            data.text = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"tEXt"_ct>
{
    static constexpr ChunkType identifier = "tEXt"_ct;
    static constexpr std::string_view name = "Text Data";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::string keyword;
        std::string text;
    };

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        //TODO: Add keyword error handling here? or when we want to actually construct the text data

        if(auto value = ReadCString(stream, 80); value)
            data.keyword = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = ReadString(stream); value)
            data.text = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<>
struct ChunkTraits<"zTXt"_ct>
{
    static constexpr ChunkType identifier = "zTXt"_ct;
    static constexpr std::string_view name = "Compressed Text Data";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::string keyword;
        std::uint8_t compressionMethod;
        std::string text;
    };

    static AnyError<Data> Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        //TODO: Add keyword error handling here? or when we want to actually construct the text data
        if(auto value = ReadCString(stream, 80); value)
            data.keyword = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = stream.Read<std::uint8_t>(); value)
            data.compressionMethod = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        if(auto value = ReadString(stream); value)
            data.text = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());

        return data;
    }
};

template<ChunkType Ty>
using ChunkContainer = ChunkContainerImpl<Ty, ChunkTraits<Ty>::is_optional, ChunkTraits<Ty>::multiple_allowed>::type;

using StandardChunks = std::tuple<
    ChunkContainer<"IHDR"_ct>,
    ChunkContainer<"PLTE"_ct>,
    ChunkContainer<"IDAT"_ct>,
    ChunkContainer<"cHRM"_ct>,
    ChunkContainer<"gAMA"_ct>,
    ChunkContainer<"iCCP"_ct>,
    ChunkContainer<"sBIT"_ct>,
    ChunkContainer<"sRGB"_ct>,
    ChunkContainer<"bKGD"_ct>,
    ChunkContainer<"hIST"_ct>,
    ChunkContainer<"tRNS"_ct>,
    ChunkContainer<"pHYs"_ct>,
    ChunkContainer<"sPLT"_ct>,
    ChunkContainer<"tIME"_ct>,
    ChunkContainer<"iTXt"_ct>,
    ChunkContainer<"tEXt"_ct>,
    ChunkContainer<"zTXt"_ct>>;

struct DecodedChunks
{
    StandardChunks standardChunks;

    template<ChunkType Ty>
    ChunkContainer<Ty>& Get() noexcept
    {
        return std::get<ChunkContainer<Ty>>(standardChunks);
    }

    template<ChunkType Ty>
    const ChunkContainer<Ty>& Get() const noexcept
    {
        return std::get<ChunkContainer<Ty>>(standardChunks);
    }
};