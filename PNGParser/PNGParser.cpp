module;

#include <array>
#include <cstdint>
#include <algorithm>
#include <concepts>
#include <vector>
#include <bit>
#include <iostream>

module PNGParser;




static constexpr Bytes<8> ReferencePNGSignature = { 137, 80, 78, 71, 13, 10, 26, 10 };
static constexpr Bytes<8> PNGSignature = (SwapByteOrder) ? FlipEndianness(ReferencePNGSignature) : ReferencePNGSignature;

template<size_t Count>
Bytes<Count> ReadBytes(std::istream& stream)
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
    return std::bit_cast<Ty>(ReadBytes<sizeof(Ty)>(stream));
};



class ChunkInputStream
{
    std::istream* m_stream;
    std::uint32_t m_chunkSize = 0;
    std::uint32_t m_bytesRead = 0;

public:
    ChunkInputStream(std::istream& stream) :
        m_stream(&stream)
    {
        m_chunkSize = ParseBytes<std::uint32_t>(*m_stream);
    }

public:
    template<class Ty>
        requires std::integral<Ty> || std::floating_point<Ty>
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

    std::uint32_t ChunkSize() const noexcept { return m_chunkSize; }
};

std::unique_ptr<ChunkData::TypeErasedData> SkipChunk(ChunkInputStream& stream)
{
    while(stream.HasUnreadData())
        stream.Read<1>();

    return nullptr;
}

class PNGInputStream
{
    std::istream* m_stream;

public:
    PNGInputStream(std::istream& stream) :
        m_stream(&stream)
    {

    }

public:
    bool Reading() const noexcept { return !m_stream->eof(); }
    Chunk ParseChunk()
    {
        ChunkInputStream chunkStream{ *m_stream };
        ChunkData data;
        data.type = ChunkType(ParseBytes<std::uint32_t>(*m_stream));
        std::cout << data.type.ToString() << "\n";

        data.data = [&chunkStream](ChunkType type)
        {
            switch(type)
            {
            case "IHDR"_ct:
                return SkipChunk(chunkStream);
            case "PLTE"_ct:
                return SkipChunk(chunkStream);
            case "IDAT"_ct:
                return SkipChunk(chunkStream);
            case "IEND"_ct:
                return SkipChunk(chunkStream);
            case "cHRM"_ct:
                return SkipChunk(chunkStream);
            case "gAMA"_ct:
                return SkipChunk(chunkStream);
            case "iCCP"_ct:
                return SkipChunk(chunkStream);
            case "sBIT"_ct:
                return SkipChunk(chunkStream);
            case "sRGB"_ct:
                return SkipChunk(chunkStream);
            case "bKGD"_ct:
                return SkipChunk(chunkStream);
            case "hIST"_ct:
                return SkipChunk(chunkStream);
            case "tRNS"_ct:
                return SkipChunk(chunkStream);
            case "pHYs"_ct:
                return SkipChunk(chunkStream);
            case "sPLT"_ct:
                return SkipChunk(chunkStream);
            case "tIME"_ct:
                return SkipChunk(chunkStream);
            case "iTXt"_ct:
                return SkipChunk(chunkStream);
            case "tEXt"_ct:
                return SkipChunk(chunkStream);
            case "zTXt"_ct:
                return SkipChunk(chunkStream);
            default:
                return SkipChunk(chunkStream);
            }
        }(data.type);

        ParseBytes<std::uint32_t>(*m_stream);

        if(data.type == "IEND"_ct)
        {
            while(Reading())
            {
                m_stream->get();
            }
        }
        return Chunk{};
    }
};

void VerifySignature(std::istream& stream)
{
    auto signature = ReadBytes<PNGSignature.size()>(stream);
    if(signature != PNGSignature)
    {
        throw std::exception("PNG signature could not be matched");
    }
}

void ParsePNG(std::istream& stream)
{
    VerifySignature(stream);
    PNGInputStream pngStream{ stream };
    std::vector<Chunk> chunks;
    
    while(pngStream.Reading())
    {
        chunks.push_back(pngStream.ParseChunk());    
    }
}