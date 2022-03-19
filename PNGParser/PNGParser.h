#pragma once
#include <istream>
#include <array>
#include <cstdint>
#include <concepts>
#include <tuple>
#include <string_view>
#include <variant>

template<size_t Size>
std::array<std::uint8_t, Size> ReadBytes(std::istream& stream)
{
    std::array<std::uint8_t, Size> bytes;

    for(std::uint8_t& b : bytes)
        b = static_cast<std::uint8_t>(stream.get());

    return bytes;
}

enum class Endian
{
    Little,
    Big,
};

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 1)
constexpr Ty BytesToIntegral(std::uint8_t b0)
{
    return b0;
}

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 1)
constexpr Ty BytesToIntegral(std::array<std::uint8_t, 1> bytes)
{
    return BytesToIntegral<Ty, ed>(bytes[0]);
}

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 2)
constexpr Ty BytesToIntegral(std::uint8_t b1, std::uint8_t b0)
{
    if constexpr(ed == Endian::Little)
    {
        return (b1 << 8) | (b0 << 0);
    }
    else
    {
        return (b0 << 8) | (b1 << 0);
    }
}

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 2)
constexpr Ty BytesToIntegral(std::array<std::uint8_t, 2> bytes)
{
    return BytesToIntegral<Ty, ed>(bytes[0], bytes[1]);
}

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 4)
constexpr Ty BytesToIntegral(std::uint8_t b3, std::uint8_t b2, std::uint8_t b1, std::uint8_t b0)
{
    if constexpr(ed == Endian::Little)
    {
        return (b3 << 24) | (b2 << 16) | (b1 << 8) | (b0 << 0);
    }
    else
    {
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | (b3 << 0);
    }
}

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 4)
constexpr Ty BytesToIntegral(std::array<std::uint8_t, 4> bytes)
{
    return BytesToIntegral<Ty, ed>(bytes[0], bytes[1], bytes[2], bytes[3]);
}

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 8)
constexpr Ty BytesToIntegral(std::uint8_t b7, std::uint8_t b6, std::uint8_t b5, std::uint8_t b4, std::uint8_t b3, std::uint8_t b2, std::uint8_t b1, std::uint8_t b0)
{
    if constexpr(ed == Endian::Little)
    {
        return (b7 << 56) | (b6 << 48) | (b5 << 40) | (b4 << 32) | (b3 << 24) | (b2 << 16) | (b1 << 8) | (b0 << 0);
    }
    else
    {
        return (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) | (b4 << 24) | (b5 << 16) | (b6 << 8) | (b7 << 0);
    }
}

template<std::integral Ty, Endian ed = Endian::Little>
    requires(sizeof(Ty) == 8)
constexpr Ty BytesToIntegral(std::array<std::uint8_t, 8> bytes)
{
    return BytesToIntegral<Ty, ed>(bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
}

template<std::integral Ty, Endian ed = Endian::Little>
Ty Parse(std::istream& stream)
{
    return BytesToIntegral<Ty, ed>(ReadBytes<sizeof(Ty)>(stream));
}


constexpr std::uint32_t StringLiteralToInt(std::string_view string)
{
    return BytesToIntegral<uint32_t>(string[0], string[1], string[2], string[3]);
}

enum class ChunkType : std::uint32_t
{
    Image_Header = StringLiteralToInt("IHDR"),
    Palette = StringLiteralToInt("PLTE"),
    Image_Data = StringLiteralToInt("IDAT"),
    Image_Trailer = StringLiteralToInt("IEND"),
    Chroma = StringLiteralToInt("cHRM"),
    Image_Gamma = StringLiteralToInt("gAMA"),
    ICC_Profile = StringLiteralToInt("iCCP"),
    Significant_Bits = StringLiteralToInt("sBIT"),
    RGB_Color_Space = StringLiteralToInt("sRGB"),
    Background_Color = StringLiteralToInt("bKGD"),
    Image_Histogram = StringLiteralToInt("hIST"),
    Transparency = StringLiteralToInt("tRNS"),
    Physical_Pixel_Dimensions = StringLiteralToInt("pHYs"),
    Suggested_Palette = StringLiteralToInt("sPLT"),
    Time = StringLiteralToInt("tIME"),
    International_Text_Data = StringLiteralToInt("iTXt"),
    Text_Data = StringLiteralToInt("tEXt"),
    Compressed_Text_Data = StringLiteralToInt("zTXt"),
};


enum class ColorType : std::uint8_t
{
    Grey_Scale = 0,
    True_Color = 2,
    Indexed_Color = 3,
    Grey_Scale_With_Alpha = 4,
    True_Color_With_Alpha = 6
};

template<ChunkType Type>
struct ChunkData;

template<>
struct ChunkData<ChunkType::Image_Header>
{
    static constexpr ChunkType value = ChunkType::Image_Header;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(73, 72, 68, 82));

    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t bitDepth;
    ColorType type;
    std::uint8_t compressionMethod;
    std::uint8_t filterMethod;
    std::uint8_t interlaceMethod;
};

template<>
struct ChunkData<ChunkType::Palette>
{
    static constexpr ChunkType value = ChunkType::Palette;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(80, 76, 84, 69));
    static constexpr size_t maxPaletteEntries = 256;

    struct Color
    {
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
    };

    //There are paletteEntries + 1 entries
    std::uint8_t paletteEntries;
    std::array<Color, maxPaletteEntries> palette;
};

template<ColorType color>
struct ColorTypeTable;

template<>
struct ColorTypeTable<ColorType::Grey_Scale>
{
    static constexpr ColorType value = ColorType::Grey_Scale;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 1, 2, 4, 8, 16 });

    struct SignificantBits
    {
        std::uint8_t grayScaleBits;
    };

    struct BackgroundColor
    {
        std::uint16_t grayScale;
    };

    struct Transparency
    {
        std::uint16_t transparancy;
    };
};

template<>
struct ColorTypeTable<ColorType::True_Color>
{
    static constexpr ColorType value = ColorType::True_Color;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 8, 16 });

    struct SignificantBits
    {
        std::uint8_t redBits;
        std::uint8_t greenBits;
        std::uint8_t blueBits;
    };

    struct BackgroundColor
    {
        std::uint16_t red;
        std::uint16_t green;
        std::uint16_t blue;
    };

    struct Transparency
    {
        std::uint16_t redTransparency;
        std::uint16_t greenTransparency;
        std::uint16_t blueTransparency;
    };
};

template<>
struct ColorTypeTable<ColorType::Indexed_Color>
{
    static constexpr ColorType value = ColorType::Indexed_Color;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 1, 2, 4, 8 });

    struct SignificantBits
    {
        std::uint8_t redBits;
        std::uint8_t greenBits;
        std::uint8_t blueBits;
    };

    struct BackgroundColor
    {
        std::uint8_t paletteIndex;
    };

    struct Transparency
    {
        std::array<std::uint8_t, ChunkData<ChunkType::Palette>::maxPaletteEntries> transparency;
    };
};

template<>
struct ColorTypeTable<ColorType::Grey_Scale_With_Alpha>
{
    static constexpr ColorType value = ColorType::Grey_Scale_With_Alpha;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 8, 16 });

    struct SignificantBits
    {
        std::uint8_t grayScaleBits;
        std::uint8_t alphaBits;
    };

    struct BackgroundColor
    {
        std::uint16_t grayScale;
    };
};

template<>
struct ColorTypeTable<ColorType::True_Color_With_Alpha>
{
    static constexpr ColorType value = ColorType::True_Color_With_Alpha;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 8, 16 });

    struct SignificantBits
    {
        std::uint8_t redBits;
        std::uint8_t greenBits;
        std::uint8_t blueBits;
        std::uint8_t alphaBits;
    };

    struct BackgroundColor
    {
        std::uint16_t red;
        std::uint16_t green;
        std::uint16_t blue;
    };
};

template<>
struct ChunkData<ChunkType::Image_Data>
{
    static constexpr ChunkType value = ChunkType::Image_Data;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(73, 68, 65, 84));

    //TODO: Image data
};

template<>
struct ChunkData<ChunkType::Image_Trailer>
{
    static constexpr ChunkType value = ChunkType::Image_Trailer;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(73, 69, 78, 68));
};

template<>
struct ChunkData<ChunkType::Chroma>
{
    static constexpr ChunkType value = ChunkType::Chroma;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(99, 72, 82, 77));

    std::uint32_t whitePointX;
    std::uint32_t whitePointY;

    std::uint32_t redX;
    std::uint32_t redY;

    std::uint32_t greenX;
    std::uint32_t greenY;

    std::uint32_t blueX;
    std::uint32_t blueY;
};

template<>
struct ChunkData<ChunkType::Image_Gamma>
{
    static constexpr ChunkType value = ChunkType::Image_Gamma;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(103, 65, 77, 65));

    std::uint32_t gamma;
};

template<>
struct ChunkData<ChunkType::ICC_Profile>
{
    static constexpr ChunkType value = ChunkType::ICC_Profile;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(105, 67, 67, 80));

    std::array<std::uint8_t, 80> profileName;
    std::uint8_t compressionMethod;
    //TODO: compressedProfile
};

template<>
struct ChunkData<ChunkType::Significant_Bits>
{
    static constexpr ChunkType value = ChunkType::Significant_Bits;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(115, 66, 73, 84));
    
    using SignificantBitVairent = std::variant<
        ColorTypeTable<ColorType::Grey_Scale>::SignificantBits,
        ColorTypeTable<ColorType::True_Color>::SignificantBits,
        ColorTypeTable<ColorType::Indexed_Color>::SignificantBits,
        ColorTypeTable<ColorType::Grey_Scale_With_Alpha>::SignificantBits,
        ColorTypeTable<ColorType::True_Color_With_Alpha>::SignificantBits>;

    SignificantBitVairent bits;
};

enum class RenderingIntent : std::uint8_t
{
    Perceptual = 0,
    Relative_Colorimetric = 1,
    Saturation = 2,
    Absolute_Colorimetric = 3
};

template<>
struct ChunkData<ChunkType::RGB_Color_Space>
{
    static constexpr ChunkType value = ChunkType::RGB_Color_Space;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(115, 82, 71, 66));

    static constexpr ChunkData<ChunkType::Image_Gamma> compatabilityGamma
    {
        .gamma = 45455
    };
    static constexpr ChunkData<ChunkType::Chroma> compatabilityChroma
    {
        .whitePointX = 31'270,
        .whitePointY = 32'900,
        .redX = 64'000,
        .redY = 33'000,
        .greenX = 30'000,
        .greenY = 60'000,
        .blueX = 15'000,
        .blueY = 6'000,
    };

    RenderingIntent intent;
};

template<>
struct ChunkData<ChunkType::Image_Histogram>
{
    static constexpr ChunkType value = ChunkType::Image_Histogram;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(104, 73, 83, 84));

    std::array<std::uint16_t, ChunkData<ChunkType::Palette>::maxPaletteEntries> histogram;
};

template<>
struct ChunkData<ChunkType::Transparency>
{
    static constexpr ChunkType value = ChunkType::Transparency;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(116, 82, 78, 83));

    using TransparencyVariant = std::variant<
        ColorTypeTable<ColorType::Grey_Scale>::Transparency,
        ColorTypeTable<ColorType::True_Color>::Transparency,
        ColorTypeTable<ColorType::Indexed_Color>::Transparency>;

    TransparencyVariant transparency;
};

enum class UnitType : std::uint8_t
{
    Unknown,
    Metre
};

template<>
struct ChunkData<ChunkType::Physical_Pixel_Dimensions>
{
    static constexpr ChunkType value = ChunkType::Physical_Pixel_Dimensions;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(112, 72, 89, 115));
    
    std::uint32_t pixelsPerUnitX;
    std::uint32_t pixelsPerUnitY;
    UnitType type;
};

template<>
struct ChunkData<ChunkType::Suggested_Palette>
{
    static constexpr ChunkType value = ChunkType::Suggested_Palette;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(115, 80, 76, 84));
    
    template<std::uint8_t sampleDepth>
    static constexpr std::uint8_t chunkDivisibility = 0;
    
    template<>
    static constexpr std::uint8_t chunkDivisibility<8> = 6;
    
    template<>
    static constexpr std::uint8_t chunkDivisibility<16> = 10;

    template<std::uint8_t sampleDepth>
    struct PaletteEntry;

    template<>
    struct PaletteEntry<8>
    {
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
        std::uint8_t alpha;
        std::uint16_t frequency;
    };

    template<>
    struct PaletteEntry<16>
    {
        std::uint16_t red;
        std::uint16_t green;
        std::uint16_t blue;
        std::uint16_t alpha;
        std::uint16_t frequency;
    };

    template<std::uint8_t sampleDepth>
    using Palette = std::array<PaletteEntry<sampleDepth>, ChunkData<ChunkType::Palette>::maxPaletteEntries>;

    using PaletteVariant = std::variant<Palette<8>, Palette<16>>;

    std::array<std::uint8_t, 80> profileName;
    std::uint8_t sampleDepth;
    PaletteVariant palette;
};

template<>
struct ChunkData<ChunkType::Time>
{
    static constexpr ChunkType value = ChunkType::Time;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(116, 73, 77, 69));

    std::uint16_t year;
    std::uint8_t month;
    std::uint8_t day;
    std::uint8_t hour;
    std::uint8_t minute;
    std::uint8_t second;
};

template<>
struct ChunkData<ChunkType::International_Text_Data>
{
    static constexpr ChunkType value = ChunkType::International_Text_Data;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(105, 84, 88, 116));

    std::array<std::uint8_t, 80> keyword;
    std::uint8_t compressionFlag;
    std::uint8_t compressionMethod;
    //TODO: Language tag
    //TODO: Trasnlated Keyword
    //TODO: Text
};

template<>
struct ChunkData<ChunkType::Text_Data>
{
    static constexpr ChunkType value = ChunkType::Text_Data;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(116, 69, 88, 116));

    std::array<std::uint8_t, 80> keyword;
    //TODO: Text
};

template<>
struct ChunkData<ChunkType::Compressed_Text_Data>
{
    static constexpr ChunkType value = ChunkType::Compressed_Text_Data;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(122, 84, 88, 116));

    std::array<std::uint8_t, 80> keyword;
    std::uint8_t compressionMethod;
    //TODO: Text
};

template<ChunkType Type>
struct Chunk
{
    std::uint32_t length;
    static constexpr ChunkType type = Type;
    ChunkData<Type> data;
    std::uint32_t CRC;
};