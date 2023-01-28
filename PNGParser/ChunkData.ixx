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

struct ChunkData
{
    struct TypeErasedData
    {
        virtual ~TypeErasedData() = default;
    };

    template<class Ty>
    struct Data : public TypeErasedData
    {
        Ty internalData;

        Data(const Ty& t) :
            internalData(t)
        {

        }

        Data(Ty&& t) noexcept(noexcept(std::move(std::declval<Ty>()))) :
            internalData(std::move(t))
        {

        }

    };

    std::unique_ptr<TypeErasedData> data;

    ChunkData() = default;

    template<class Ty>
    ChunkData(Ty&& value) :
        data(std::make_unique<Data<Ty>>(std::forward<Ty>(value)))
    {

    }
};

export template<ChunkType Ty>
struct ChunkTraits;

export class Chunk
{
    std::uint32_t m_length;
    ChunkType m_type;
    ChunkData m_data;
    std::uint32_t m_CRC;

public:
    Chunk(std::uint32_t length, ChunkType type, ChunkData data, std::uint32_t CRC) :
        m_length(length),
        m_type(type),
        m_data(std::move(data)),
        m_CRC(CRC)
    {

    }

public:
    std::uint32_t Length() const noexcept { return m_length; }
    ChunkType Type() const noexcept { return m_type; }

    template<class Type>
    Type& Data() 
    {
        if(typeid(*m_data.data) != typeid(ChunkData::template Data<Type>))
            throw std::bad_cast();

        return static_cast<ChunkData::Data<Type>&>(*m_data.data).internalData;
    };

    template<class Type>
    const Type& Data() const
    {
        if(typeid(*m_data.data) != typeid(ChunkData::template Data<Type>))
            throw std::bad_cast();

        return static_cast<const ChunkData::Data<Type>&>(*m_data.data).internalData;
    };

    template<ChunkType Ty>
    typename ChunkTraits<Ty>::Data& Data()
    {
        if(typeid(*m_data.data) != typeid(ChunkData::template Data<typename ChunkTraits<Ty>::Data>))
            throw std::bad_cast();

        return static_cast<ChunkData::Data<typename ChunkTraits<Ty>::Data>&>(*m_data.data).internalData;
    };

    template<ChunkType Ty>
    const typename ChunkTraits<Ty>::Data& Data() const
    {
        if(typeid(*m_data.data) != typeid(ChunkData::template Data<typename ChunkTraits<Ty>::Data>))
            throw std::bad_cast();

        return static_cast<const ChunkData::Data<typename ChunkTraits<Ty>::Data>&>(*m_data.data).internalData;
    };
};


export template<ChunkType Ty>
struct ChunkTraits;

template<>
struct ChunkTraits<"IHDR">
{
    static constexpr ChunkType identifier = "IHDR";
    static constexpr std::string_view name = "Image Header";

    struct Data
    {
        std::uint32_t width;
        std::uint32_t height;
        std::int8_t bitDepth;
        std::int8_t colorType;
        std::int8_t compressionMethod;
        std::int8_t filterMethod;
        std::int8_t interlaceMethod;

        std::uint32_t SubpixelPerPixel() const
        {
            switch(colorType)
            {
            case 0:
                return 1;
            case 2:
                return 3;
            case 3:
                return 1;
            case 4:
                return 2;
            case 6:
                return 4;
            }

            throw std::exception("Unexpected pixel type");
        }

        std::uint32_t BytesPerPixel() const 
        {
            return std::max(bitDepth / 8, 1) * SubpixelPerPixel();
        }

        std::uint32_t FilteredScanlineSize() const
        {
            static constexpr std::uint32_t filterByteCount = 1;
            return filterByteCount + (width * BytesPerPixel());
        }

        std::uint32_t FilteredImageSize() const
        {
            return FilteredScanlineSize() * height;
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
            return (width * height * BytesPerPixel());
        }
    };

    static constexpr size_t maxSize = 13;

    static ChunkData Parse(ChunkDataInputStream& stream, std::span<const Chunk> chunks)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(std::string(identifier.ToString()) + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.width = stream.Read<std::uint32_t>();
        data.height = stream.Read<std::uint32_t>();
        data.bitDepth = stream.Read<std::int8_t>();
        data.colorType = stream.Read<std::int8_t>();
        data.compressionMethod = stream.Read<std::int8_t>();
        data.filterMethod = stream.Read<std::int8_t>();
        data.interlaceMethod = stream.Read<std::int8_t>();

        return data;
    }
};

template<>
struct ChunkTraits<ConstString("gAMA")>
{
    static constexpr ChunkType identifier = "gAMA";
    static constexpr std::string_view name = "Image Gamma";

    struct Data
    {
        std::uint32_t gamma;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static ChunkData Parse(ChunkDataInputStream& stream, std::span<const Chunk> chunks)
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

    struct Data
    {
        Byte renderingIntent;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static ChunkData Parse(ChunkDataInputStream& stream, std::span<const Chunk> chunks)
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

    struct Data
    {
        std::vector<Byte> bytes;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static constexpr size_t maxSlidingWindowSize = 32768;

    static ChunkData Parse(ChunkDataInputStream& stream, std::span<const Chunk> chunks)
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