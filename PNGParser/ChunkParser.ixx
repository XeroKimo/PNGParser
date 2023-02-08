module;

#include <concepts>
#include <istream>
#include <array>

export module PNGParser:ChunkParser;
import :PlatformDetection;

template<size_t Count>
Bytes<Count> ReadBytes(std::istream& stream)
{
    using Byte = Bytes<Count>::value_type;
    Bytes<Count> bytes;
    for(Byte& b : bytes)
        b = static_cast<Byte>(stream.get());
    return bytes;
}

template<size_t Count>
Bytes<Count> ReadNativeBytes(std::istream& stream)
{
    using Byte = Bytes<Count>::value_type;
    Bytes<Count> bytes;

    if constexpr(SwapByteOrder)
    {
        auto begin = bytes.rbegin();
        auto end = bytes.rend();
        for(; begin != end; ++begin)
        {
            *begin = static_cast<Byte>(stream.get());
        }
    }
    else
    {
        for(Byte& b : bytes)
            b = static_cast<Byte>(stream.get());
    }

    return bytes;
};

template<class Ty>
    requires std::integral<Ty> || std::floating_point<Ty>
Ty ParseBytes(std::istream& stream)
{
    return std::bit_cast<Ty>(ReadNativeBytes<sizeof(Ty)>(stream));
};

export class ChunkDataInputStream
{
    std::istream* m_stream;
    std::uint32_t m_chunkSize = 0;
    std::uint32_t m_bytesRead = 0;

public:
    ChunkDataInputStream(std::istream& stream, std::uint32_t chunkSize) :
        m_stream(&stream),
        m_chunkSize(chunkSize)
    {
    }
    ChunkDataInputStream(const ChunkDataInputStream&) = delete;
    ChunkDataInputStream(ChunkDataInputStream&& other) noexcept :
        m_stream(other.m_stream),
        m_chunkSize(other.m_chunkSize),
        m_bytesRead(other.m_bytesRead)
    {
        other.m_stream = nullptr;
        other.m_chunkSize = 0;
        other.m_bytesRead = 0;
    }

    ~ChunkDataInputStream()
    {
        Skip(UnreadSize());
    }

public:
    template<class Ty>
        requires std::integral<Ty> || std::floating_point<Ty> || std::is_enum_v<Ty>
    Ty ReadNative()
    {
        return std::bit_cast<Ty>(ReadNative<sizeof(Ty)>());
    }

    template<size_t Count>
    Bytes<Count> ReadNative()
    {
        std::uint32_t bytesRead = m_bytesRead + Count;

        if(bytesRead > m_chunkSize)
            throw std::out_of_range("Reading memory outside of range");

        m_bytesRead = bytesRead;

        return ReadNativeBytes<Count>(*m_stream);
    }

    template<class Ty>
        requires std::integral<Ty> || std::floating_point<Ty> || std::is_enum_v<Ty>
    Ty Read()
    {
        return std::bit_cast<Ty>(Read<sizeof(Ty)>());
    }

    template<size_t Count>
    Bytes<Count> Read()
    {
        std::uint32_t bytesRead = m_bytesRead + Count;

        if(bytesRead > m_chunkSize)
            throw std::out_of_range("Reading memory outside of range");

        m_bytesRead = bytesRead;

        return ReadBytes<Count>(*m_stream);
    }

    bool HasUnreadData() const noexcept { return m_bytesRead < m_chunkSize; }

    void Skip(std::uint32_t count)
    {
        count = std::min(count, UnreadSize());
        m_stream->seekg(count, std::ios_base::cur);
        m_bytesRead = count;
    }

    std::uint32_t ChunkSize() const noexcept { return m_chunkSize; }
    std::uint32_t UnreadSize() const noexcept { return m_chunkSize - m_bytesRead; }
};
