module;

#include <cstdint>
#include <string_view>
#include <memory>
#include <bit>
#include <array>
#include <algorithm>

export module PNGParser;
export import <istream>;

export void ParsePNG(std::istream& stream);


export struct ChunkType
{
    std::uint32_t identifier;

    std::string_view ToString() const noexcept { return std::string_view{ reinterpret_cast<const char*>(&identifier), sizeof(decltype(identifier)) }; }

    constexpr operator std::uint32_t() noexcept { return identifier; }
    constexpr operator const std::uint32_t() const noexcept { return identifier; }
};

template<size_t Count>
using Bytes = std::array<std::uint8_t, Count>;

constexpr bool IsPlatformNetworkByteOrder = std::endian::native == std::endian::big;
constexpr bool SwapByteOrder = !IsPlatformNetworkByteOrder;



template<size_t Count>
constexpr Bytes<Count> FlipEndianness(const Bytes<Count>& bytes)
{
    Bytes<Count> newBytes;
    std::reverse_copy(bytes.begin(), bytes.end(), newBytes.begin());
    return newBytes;
}

constexpr ChunkType operator""_ct(const char* str, std::size_t len)
{
    if(len != 4)
        throw std::exception("String length must be 4");

    using ChunkTypeBytes = Bytes<4>;
    using Bytes_Type = ChunkTypeBytes::value_type;
    const ChunkTypeBytes bytes
    {
        static_cast<Bytes_Type>(str[0]),
        static_cast<Bytes_Type>(str[1]),
        static_cast<Bytes_Type>(str[2]),
        static_cast<Bytes_Type>(str[3]),
    };

    if constexpr(SwapByteOrder)
    {
        return ChunkType{ std::bit_cast<std::uint32_t>(FlipEndianness(bytes)) };
    }
    else
    {
        return ChunkType{ std::bit_cast<std::uint32_t>(bytes) };
    }
}

template<class Ty>
struct ChunkTraits;

template<ChunkType Ty>
struct ChunkTypeToTraits;

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
    };

    ChunkType type;
    std::unique_ptr<TypeErasedData> data;
};

export class Chunk
{
    std::uint32_t m_length;
    ChunkData m_data;
    std::uint32_t m_CRC;

public:
    std::uint32_t Length() const noexcept { return m_length; }
    ChunkType Type() const noexcept { return m_data.type; }

    template<class Type>
    Type& Data() { };

    //template<ChunkType Type>
    //Type& Data() { };
};

export struct HeaderChunk
{
    std::uint32_t width;
    std::uint32_t height;
    std::int8_t bitDepth;
    std::int8_t colorType;
    std::int8_t compressionMethod;
    std::int8_t filterMethod;
    std::int8_t interlaceMethod;

};

template<>
struct ChunkTraits<HeaderChunk>
{
    using value_type = HeaderChunk;
    static constexpr ChunkType identifier = "IHDR"_ct;
};

template<>
struct ChunkTypeToTraits<ChunkTraits<HeaderChunk>::identifier>
{
    using traits = ChunkTraits<HeaderChunk>;
};