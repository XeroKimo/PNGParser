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

namespace ChunkIdentifiers
{
    constexpr ChunkType header = "IHDR";
    constexpr ChunkType palette = "PLTE";
    constexpr ChunkType imageData = "IDAT";
    constexpr ChunkType imageTrailer = "IEND";
    constexpr ChunkType chromaticities = "cHRM";
    constexpr ChunkType imageGamma = "gAMA";
    constexpr ChunkType iccProfile = "iCCP";
    constexpr ChunkType significantBits = "sBIT";
    constexpr ChunkType rgbColorSpace = "sRGB";
    constexpr ChunkType backgroundColor = "bKGD";
    constexpr ChunkType imageHistogram = "hIST";
    constexpr ChunkType transparency = "tRNS";
    constexpr ChunkType physicalPixelDimensions = "pHYs";
    constexpr ChunkType suggestedPalette = "sPLT";
    constexpr ChunkType lastModificationTime = "tIME";
    constexpr ChunkType internationalTextualData = "iTXt";
    constexpr ChunkType texturalData = "tEXt";
    constexpr ChunkType compressedTextualData = "zTXt";
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
std::string ReadCString(ChunkDataInputStream& stream, std::size_t maxSize = std::numeric_limits<std::size_t>::max())
{
    std::string string;
    char letter = stream.Read<char>();
    for(size_t i = 0; i < maxSize && letter != 0; i++, letter = stream.Read<char>())
    {
        string.push_back(letter);
    }

    if(letter != 0)
        throw std::runtime_error("Null terminator not found when parsing suggested palette name");

    return string;
}


std::string ReadString(ChunkDataInputStream& stream)
{
    std::string string;

    for(char letter = stream.Read<char>(); stream.HasUnreadData(); letter = stream.Read<char>())
    {
        string.push_back(letter);
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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
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

    static constexpr size_t maxSize = Data{}.colorPalette.size() * sizeof(Data::ColorEntry::value_type);

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() % 3 > 0)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

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
struct ChunkTraits<"IEND">
{
    static constexpr ChunkType identifier = "IEND";
    static constexpr std::string_view name = "Image Trailer";
    static constexpr bool is_optional = false;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
    };

    static constexpr size_t maxSize = 0;

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        return {};
    }
};

template<>
struct ChunkTraits<"cHRM">
{
    static constexpr ChunkType identifier = "cHRM";
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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;
        data.whitePointX = stream.Read<std::uint32_t>();
        data.whitePointY = stream.Read<std::uint32_t>();
        data.redPointX = stream.Read<std::uint32_t>();
        data.redPointY = stream.Read<std::uint32_t>();
        data.greenPointX = stream.Read<std::uint32_t>();
        data.greenPointY = stream.Read<std::uint32_t>();
        data.bluePointX = stream.Read<std::uint32_t>();
        data.bluePointY = stream.Read<std::uint32_t>();

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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.gamma = stream.ReadNative<std::uint32_t>();

        return data;
    }
};

template<>
struct ChunkTraits<"iCCP">
{
    static constexpr ChunkType identifier = "iCCP";
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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        for(char character = stream.ReadNative<char>(); character != '\n'; character = stream.ReadNative<char>())
        {
            data.profileName.push_back(character);
        }

        data.compressionMethod = stream.ReadNative<Byte>();
        data.compressedProfile.reserve(stream.UnreadSize());

        while(stream.HasUnreadData())
        {
            data.compressedProfile.push_back(stream.ReadNative<Byte>());
        }

        return data;
    }
};

template<>
struct ChunkTraits<"sBIT">
{
    static constexpr ChunkType identifier = "sBIT";
    static constexpr std::string_view name = "Significant Bits";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        Bytes<4> bits;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        for(size_t i = 0; i < stream.ChunkSize(); i++)
        {
            data.bits[i] = stream.ReadNative<std::uint8_t>();
        }

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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.renderingIntent = stream.ReadNative<Byte>();

        return data;
    }
};

template<>
struct ChunkTraits<"bKGD">
{
    static constexpr ChunkType identifier = "bKGD";
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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {

        Data data;

        if(stream.ChunkSize() == sizeof(Data::GreyscaleBackground))
            data.color = stream.Read<Data::GreyscaleBackground>();
        else if(stream.ChunkSize() == Data::ColorBackground{}.size() * sizeof(Data::ColorBackground::value_type))
        {
            Data::ColorBackground color;
            for(size_t i = 0; i < color.size(); i++)
            {
                color[i] = stream.Read<Data::ColorBackground::value_type>();
            }
            data.color = color;
        }
        else if(stream.ChunkSize() == sizeof(Data::PaletteBackground))
            data.color = stream.Read<Data::PaletteBackground>();
        else
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\n");

        return data;
    }
};

template<>
struct ChunkTraits<"hIST">
{
    static constexpr ChunkType identifier = "hIST";
    static constexpr std::string_view name = "Image Histogram";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::array<std::uint16_t, ChunkTraits<"PLTE">::maxEntries> frequency;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() > maxSize || (stream.ChunkSize() % sizeof(std::uint16_t) > 0))
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        for(size_t i = 0; stream.HasUnreadData(); i++)
        {
            data.frequency[i] = stream.ReadNative<std::uint16_t>();
        }

        return data;
    }
};

template<>
struct ChunkTraits<"tRNS">
{
    static constexpr ChunkType identifier = "tRNS";
    static constexpr std::string_view name = "Transparency";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::vector<Byte> alphaValues;
    };

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;
        data.alphaValues.reserve(stream.ChunkSize());
        while(stream.HasUnreadData())
        {
            data.alphaValues.push_back(stream.Read<1>()[0]);
        }
        return {};
    }
};

template<>
struct ChunkTraits<"pHYs">
{
    static constexpr ChunkType identifier = "pHYs";
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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.pixelsPerUnitX = stream.ReadNative<std::uint32_t>();
        data.pixelsPerUnitY = stream.ReadNative<std::uint32_t>();
        data.unit = stream.ReadNative<std::uint8_t>();

        return data;
    }
};

template<>
struct ChunkTraits<"sPLT">
{
    static constexpr ChunkType identifier = "sPLT";
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


    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        data.name = ReadCString(stream, 80);
        data.sampleDepth = stream.Read<std::uint8_t>();

        if(data.sampleDepth == 8 && stream.UnreadSize() % 6 == 0)
        {
            while(stream.HasUnreadData())
            {
                data.palette.push_back(Data::Sample(
                    stream.Read<std::uint8_t>(),
                    stream.Read<std::uint8_t>(),
                    stream.Read<std::uint8_t>(),
                    stream.Read<std::uint8_t>(),
                    stream.Read<std::uint16_t>()));
            }
        }
        else if(data.sampleDepth == 16 && stream.UnreadSize() % 10 == 0)
        {
            while(stream.HasUnreadData())
            {
                data.palette.push_back(Data::Sample(
                    stream.Read<std::uint16_t>(),
                    stream.Read<std::uint16_t>(),
                    stream.Read<std::uint16_t>(),
                    stream.Read<std::uint16_t>(),
                    stream.Read<std::uint16_t>()));
            }
        }
        else
            throw std::runtime_error(std::string(identifier.ToString()) + " unexpected sample depth to remaining stream chunk size when parsing suggested palette.\nSample Depth: " + std::to_string(data.sampleDepth) + "\nRemaining Chunk Size: " + std::to_string(stream.UnreadSize()));

        return data;
    }
};

template<>
struct ChunkTraits<"tIME">
{
    static constexpr ChunkType identifier = "tIME";
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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;
        data.year = stream.ReadNative<std::uint16_t>();
        data.month = stream.ReadNative<std::uint8_t>();
        data.day = stream.ReadNative<std::uint8_t>();
        data.hour = stream.ReadNative<std::uint8_t>();
        data.minute = stream.ReadNative<std::uint8_t>();
        data.second = stream.ReadNative<std::uint8_t>();

        return data;
    }
};

template<>
struct ChunkTraits<"iTXt">
{
    static constexpr ChunkType identifier = "iTXt";
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

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        //TODO: Add keyword error handling here? or when we want to actually construct the text data
        data.keyword = ReadCString(stream, 80);
        data.compressionFlag = stream.Read<std::uint8_t>();
        data.compressionMethod = stream.Read<std::uint8_t>();
        data.languageTag = ReadCString(stream);
        data.translatedKeyword = ReadCString(stream);
        data.text = ReadString(stream);

        return data;
    }
};

template<>
struct ChunkTraits<"tEXt">
{
    static constexpr ChunkType identifier = "tEXt";
    static constexpr std::string_view name = "Text Data";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::string keyword;
        std::string text;
    };

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        //TODO: Add keyword error handling here? or when we want to actually construct the text data
        data.keyword = ReadCString(stream, 80);
        data.text = ReadString(stream);

        return data;
    }
};

template<>
struct ChunkTraits<"zTXt">
{
    static constexpr ChunkType identifier = "zTXt";
    static constexpr std::string_view name = "Compressed Text Data";
    static constexpr bool is_optional = true;
    static constexpr bool multiple_allowed = false;

    struct Data
    {
        std::string keyword;
        std::uint8_t compressionMethod;
        std::string text;
    };

    static Data Parse(ChunkDataInputStream& stream, DecodedChunks& chunks)
    {
        Data data;

        //TODO: Add keyword error handling here? or when we want to actually construct the text data
        data.keyword = ReadCString(stream, 80);
        data.compressionMethod = stream.Read<std::uint8_t>();
        data.text = ReadString(stream);

        return data;
    }
};

template<ChunkType Ty>
using ChunkContainer = ChunkContainerImpl<Ty, ChunkTraits<Ty>::is_optional, ChunkTraits<Ty>::multiple_allowed>::type;

using StandardChunks = std::tuple<
    ChunkContainer<"IHDR">,
    ChunkContainer<"PLTE">,
    ChunkContainer<"IDAT">,
    ChunkContainer<"cHRM">,
    ChunkContainer<"gAMA">,
    ChunkContainer<"iCCP">,
    ChunkContainer<"sBIT">,
    ChunkContainer<"sRGB">,
    ChunkContainer<"bKGD">,
    ChunkContainer<"hIST">,
    ChunkContainer<"tRNS">,
    ChunkContainer<"pHYs">,
    ChunkContainer<"sPLT">,
    ChunkContainer<"tIME">,
    ChunkContainer<"iTXt">,
    ChunkContainer<"tEXt">,
    ChunkContainer<"zTXt">>;

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