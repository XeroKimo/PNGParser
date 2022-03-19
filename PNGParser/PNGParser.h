#pragma once
#include <istream>
#include <array>
#include <cstdint>
#include <concepts>
#include <tuple>
#include <string_view>

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

template<ColorType color>
struct ColorDataTable;

template<>
struct ColorDataTable<ColorType::Grey_Scale>
{
    static constexpr ColorType value = ColorType::Grey_Scale;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 1, 2, 4, 8, 16 });
};

template<>
struct ColorDataTable<ColorType::True_Color>
{
    static constexpr ColorType value = ColorType::True_Color;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 8, 16 });
};

template<>
struct ColorDataTable<ColorType::Indexed_Color>
{
    static constexpr ColorType value = ColorType::Indexed_Color;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 1, 2, 4, 8 });
};

template<>
struct ColorDataTable<ColorType::Grey_Scale_With_Alpha>
{
    static constexpr ColorType value = ColorType::Grey_Scale_With_Alpha;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 8, 16 });
};

template<>
struct ColorDataTable<ColorType::True_Color_With_Alpha>
{
    static constexpr ColorType value = ColorType::True_Color_With_Alpha;
    static constexpr std::uint8_t intValue = static_cast<std::uint8_t>(value);
    static constexpr auto bitDepths = std::to_array<std::uint8_t>({ 8, 16 });
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

    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

template<>
struct ChunkData<ChunkType::Image_Data>
{
    static constexpr ChunkType value = ChunkType::Image_Data;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(73, 68, 65, 84));

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
    //compressedProfile?
};

template<>
struct ChunkData<ChunkType::ICC_Profile>
{
    static constexpr ChunkType value = ChunkType::ICC_Profile;
    static constexpr std::uint32_t intValue = static_cast<std::uint32_t>(value);

    static_assert(intValue == BytesToIntegral<std::uint32_t>(105, 67, 67, 80));

    std::array<std::uint8_t, 80> profileName;
    std::uint8_t compressionMethod;
    //compressedProfile?
};