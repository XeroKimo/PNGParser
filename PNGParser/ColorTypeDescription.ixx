module;

#include <cstdint>
#include <array>
#include <string_view>
#include <span>

export module PNGParser:ColorTypeDescription;

enum class ColorType : std::uint8_t
{
    GreyScale = 0,
    TrueColor = 2,
    IndexedColor = 3,
    GreyscaleWithAlpha = 4,
    TruecolorWithAlpha = 6
};

struct ColorFormatView
{
    ColorType type;
    std::string_view name;
    std::span<const int> allowBitDepths;
    int subpixelCount;
};

template<ColorType Color>
struct StaticColorFormat;

template<>
struct StaticColorFormat<ColorType::GreyScale>
{
    static constexpr ColorType type = ColorType::GreyScale;
    static constexpr std::string_view name = "Grey Scale";
    static constexpr std::array allowedBitDepths = { 1, 2, 4, 8, 16 };
    static constexpr int subpixelCount = 1;

    static constexpr ColorFormatView ToRuntime()
    {
        return { type, name, allowedBitDepths, subpixelCount };
    }
};

template<>
struct StaticColorFormat<ColorType::TrueColor>
{
    static constexpr ColorType type = ColorType::TrueColor;
    static constexpr std::string_view name = "True Color";
    static constexpr std::array allowedBitDepths = { 8, 16 };
    static constexpr int subpixelCount = 3;

    static constexpr ColorFormatView ToRuntime()
    {
        return { type, name, allowedBitDepths, subpixelCount };
    }
};

template<>
struct StaticColorFormat<ColorType::IndexedColor>
{
    static constexpr ColorType type = ColorType::IndexedColor;
    static constexpr std::string_view name = "Indexed Color";
    static constexpr std::array allowedBitDepths = { 1, 2, 4, 8 };
    static constexpr int subpixelCount = 1;

    static constexpr ColorFormatView ToRuntime()
    {
        return { type, name, allowedBitDepths, subpixelCount };
    }
};

template<>
struct StaticColorFormat<ColorType::GreyscaleWithAlpha>
{
    static constexpr ColorType type = ColorType::GreyscaleWithAlpha;
    static constexpr std::string_view name = "Greyscale with Alpha";
    static constexpr std::array allowedBitDepths = { 8, 16 };
    static constexpr int subpixelCount = 2;

    static constexpr ColorFormatView ToRuntime()
    {
        return { type, name, allowedBitDepths, subpixelCount };
    }
};

template<>
struct StaticColorFormat<ColorType::TruecolorWithAlpha>
{
    static constexpr ColorType type = ColorType::TruecolorWithAlpha;
    static constexpr std::string_view name = "True Color with Alpha";
    static constexpr std::array allowedBitDepths = { 8, 16 };
    static constexpr int subpixelCount = 4;

    static constexpr ColorFormatView ToRuntime()
    {
        return { type, name, allowedBitDepths, subpixelCount };
    }
};

constexpr std::array standardColorFormats
{
    StaticColorFormat<ColorType::GreyScale>::ToRuntime(),
    StaticColorFormat<ColorType::TrueColor>::ToRuntime(),
    StaticColorFormat<ColorType::IndexedColor>::ToRuntime(),
    StaticColorFormat<ColorType::GreyscaleWithAlpha>::ToRuntime(),
    StaticColorFormat<ColorType::TruecolorWithAlpha>::ToRuntime(),
};