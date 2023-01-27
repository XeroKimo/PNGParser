module;

#include <cstdint>
#include <bit>
#include <string>
#include <string_view>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

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

export struct ChunkType
{
    std::uint32_t identifier;

    constexpr ChunkType() = default;
    constexpr ChunkType(std::uint32_t identifier) :
        identifier(identifier)
    {
        auto bytes = std::bit_cast<Bytes<4>>(identifier);

        for(auto byte : bytes)
        {
            if(!(IsUppercase(byte) || IsLowercase(byte)))
                throw std::exception("String must only contain Alphabetical characters");
        }
    }

    std::string ToString() const noexcept { return std::string{ reinterpret_cast<const char*>(ByteInterpretation().data()), sizeof(identifier) }; }
    std::string_view ToNativeString() const noexcept { return std::string_view{ reinterpret_cast<const char*>(identifier), sizeof(identifier) }; }

    constexpr bool IsCritical() const noexcept { return IsUppercase(ByteInterpretation()[0]); }
    constexpr bool IsPrivate() const noexcept { return IsUppercase(ByteInterpretation()[1]); }
    constexpr bool Copyable() const noexcept { return !IsUppercase(ByteInterpretation()[3]); }

    constexpr operator std::uint32_t() noexcept { return identifier; }
    constexpr operator const std::uint32_t() const noexcept { return identifier; }

private:
    constexpr Bytes<sizeof(identifier)> ByteInterpretation() const noexcept
    {
        return ToNativeRepresentation(NativeByteInterpretation());
    }

    constexpr Bytes<sizeof(identifier)> NativeByteInterpretation() const noexcept
    {
        return std::bit_cast<Bytes<sizeof(identifier)>>(identifier);
    }
};

export constexpr ChunkType operator""_ct(const char* str, std::size_t len)
{
    if(len != 4)
        throw std::exception("String length must be 4");

    using ChunkTypeBytes = Bytes<4>;

    ChunkTypeBytes bytes = [str]() -> ChunkTypeBytes
    {
        if constexpr(SwapByteOrder)
        {
            return
            {
                static_cast<Byte>(str[3]),
                static_cast<Byte>(str[2]),
                static_cast<Byte>(str[1]),
                static_cast<Byte>(str[0]),
            };
        }
        else
        {
            return
            {
                static_cast<Byte>(str[0]),
                static_cast<Byte>(str[1]),
                static_cast<Byte>(str[2]),
                static_cast<Byte>(str[3]),
            };
        }
    }();

    return ChunkType{ std::bit_cast<std::uint32_t>(bytes) };
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
struct ChunkTraits<"IHDR"_ct>
{
    static constexpr ChunkType identifier = "IHDR"_ct;
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
            throw std::runtime_error(identifier.ToString() + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

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
struct ChunkTraits<"gAMA"_ct>
{
    static constexpr ChunkType identifier = "gAMA"_ct;
    static constexpr std::string_view name = "Image Gamma";

    struct Data
    {
        std::uint32_t gamma;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static ChunkData Parse(ChunkDataInputStream& stream, std::span<const Chunk> chunks)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(identifier.ToString() + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.gamma = stream.Read<std::uint32_t>();

        return data;
    }
};

template<>
struct ChunkTraits<"sRGB"_ct>
{
    static constexpr ChunkType identifier = "sRGB"_ct;
    static constexpr std::string_view name = "Standard RGB Color Space";

    struct Data
    {
        Byte renderingIntent;
    };

    static constexpr size_t maxSize = sizeof(Data);

    static ChunkData Parse(ChunkDataInputStream& stream, std::span<const Chunk> chunks)
    {
        if(stream.ChunkSize() != maxSize)
            throw std::runtime_error(identifier.ToString() + " data exceeds the expected size\nGiven size: " + std::to_string(stream.ChunkSize()) + "\nExpected size: " + std::to_string(maxSize) + "\n");

        Data data;

        data.renderingIntent = stream.Read<Byte >();

        return data;
    }
};

template<>
struct ChunkTraits<"IDAT"_ct>
{
    static constexpr ChunkType identifier = "IDAT"_ct;
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