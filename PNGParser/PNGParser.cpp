module;

#include <array>
#include <cstdint>
#include <algorithm>
#include <concepts>
#include <vector>
#include <bit>
#include <iostream>
#include <span>
#include <zlib.h>

module PNGParser;

class UnknownChunkError : public std::runtime_error
{
    ChunkType m_type{};

public:
    using std::runtime_error::runtime_error;
    UnknownChunkError() :
        std::runtime_error("Could not parse unknown chunk\n")
    {

    }

    UnknownChunkError(ChunkType type) :
        std::runtime_error(std::string("Could not parse chunk: ") + type.ToString() + "\n"),
        m_type(type)
    {

    }

    UnknownChunkError(std::string errorMsg, ChunkType type) : 
        std::runtime_error(errorMsg),
        m_type(type)
    {

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

template<class Lambda>
class ScopeGuard
{
private:
    Lambda m_l;
    bool m_engaged = true;

public:
    ScopeGuard(Lambda l) :
        m_l(l)
    {

    }
    ScopeGuard(const ScopeGuard&) = default;
    ScopeGuard(ScopeGuard&&) noexcept = default;
    ~ScopeGuard()
    {
        if(m_engaged)
            m_l();
    };


    ScopeGuard& operator=(const ScopeGuard&) = default;
    ScopeGuard& operator=(ScopeGuard&&) noexcept = default;

public:
    void Disengage() { m_engaged = false; }
};

ChunkData ParseChunkData(std::istream& stream, std::uint32_t chunkSize, ChunkType type, std::span<const Chunk> chunks)
{
    std::cout << type.ToString() << "\n";
    ChunkDataInputStream chunkStream{ stream, chunkSize };
    ChunkData data;
    switch(type)
    {
    case "IHDR"_ct:
        data = ChunkTraits<"IHDR"_ct>::Parse(chunkStream, chunks);
        break;
    case "PLTE"_ct:
        throw UnknownChunkError{ type };
        break;
    case "IDAT"_ct:
        //throw UnknownChunkError{ type };
        data = ChunkTraits<"IDAT"_ct>::Parse(chunkStream, chunks);
        break;
    case "IEND"_ct:
        throw UnknownChunkError{ type };
        break;
    case "cHRM"_ct:
        throw UnknownChunkError{ type };
        break;
    case "gAMA"_ct:
        data = ChunkTraits<"gAMA"_ct>::Parse(chunkStream, chunks);
    case "iCCP"_ct:
        throw UnknownChunkError{ type };
        break;
    case "sBIT"_ct:
        throw UnknownChunkError{ type };
        break;
    case "sRGB"_ct:
        throw UnknownChunkError{ type };
        break;
    case "bKGD"_ct:
        throw UnknownChunkError{ type };
        break;
    case "hIST"_ct:
        throw UnknownChunkError{ type };
        break;
    case "tRNS"_ct:
        throw UnknownChunkError{ type };
        break;
    case "pHYs"_ct:
        throw UnknownChunkError{ type };
        break;
    case "sPLT"_ct:
        throw UnknownChunkError{ type };
        break;
    case "tIME"_ct:
        throw UnknownChunkError{ type };
        break;
    case "iTXt"_ct:
        throw UnknownChunkError{ type };
        break;
    case "tEXt"_ct:
        throw UnknownChunkError{ type };
        break;
    case "zTXt"_ct:
        throw UnknownChunkError{ type };
        break;
    default:
        throw UnknownChunkError{ type };
        break;
    }

    if(chunkStream.HasUnreadData())
    {
        throw std::logic_error("Chunk data has not been fully parsed");
    }

    return data;
}

Chunk ParseChunk(std::istream& stream, std::span<const Chunk> chunks)
{
    std::uint32_t chunkSize = ParseBytes<std::uint32_t>(stream);
    ChunkType type{ ParseBytes<std::uint32_t>(stream) };

    ScopeGuard endChunkCleanUp = [&]
    {
        if(type == "IEND"_ct)
        {
            while(!stream.eof())
            {
                stream.get();
            }
        }
    };
    ScopeGuard crcCleanUp = [&]
    {
        ParseBytes<std::uint32_t>(stream);
    };

    ChunkData data = ParseChunkData(stream, chunkSize, type, chunks);
    std::uint32_t crc = ParseBytes<std::uint32_t>(stream);

    crcCleanUp.Disengage();
    return Chunk{ chunkSize, type, std::move(data), crc };
}

std::vector<Chunk> ParseChunks(std::istream& stream)
{
    std::vector<Chunk> chunks;

    while(!stream.eof())
    {
        try
        {
            chunks.push_back(ParseChunk(stream, chunks));
        }
        catch(UnknownChunkError& e)
        {
            std::cout << e.what();
        }
    }

    return chunks;
}

std::vector<Byte> ConcatDataChunks(std::span<const Chunk> chunks)
{
    auto firstChunk = std::ranges::find_if(chunks, [](const Chunk& chunk) { return chunk.Type() == "IDAT"_ct; });
 
    size_t totalSize = 0;
    for(auto begin = firstChunk; begin != chunks.end() && begin->Type() == "IDAT"_ct; ++begin)
    {
        totalSize += begin->Data<ChunkTraits<"IDAT"_ct>::Data>().bytes.size();
    }

    std::vector<Byte> dataBytes;
    dataBytes.reserve(totalSize);

    for(auto begin = firstChunk; begin != chunks.end() && begin->Type() == "IDAT"_ct; ++begin)
    {
        auto data = begin->Data<ChunkTraits<"IDAT"_ct>::Data>();
        std::copy(data.bytes.begin(), data.bytes.end(), std::back_inserter(dataBytes));
    }

    return dataBytes;
}

std::vector<Byte> DecompressImage(std::vector<Byte> chunks)
{
    z_stream zstream = {};
    zstream.next_in = chunks.data();
    zstream.avail_in =  chunks.size();

    std::vector<Byte> decompressedImage;
    decompressedImage.resize(chunks.size());

    if(inflateInit(&zstream) != Z_OK)
        throw std::exception("zstream failed to initialize");

    ScopeGuard endInflate = [&zstream]
    {        
        if(inflateEnd(&zstream) != Z_OK)
        {
            throw std::exception("unknown error");
        }
    };

    for(int i = 0; zstream.avail_in > 0; i++)
    {
        zstream.next_out = &decompressedImage[chunks.size() * i];
        zstream.avail_out = chunks.size();

        if(auto cont = inflate(&zstream, Z_FULL_FLUSH); !(cont == Z_OK || cont == Z_STREAM_END))
        {
            throw std::exception("unknown error");
        }

        decompressedImage.resize(decompressedImage.size() + chunks.size());
    }

    return decompressedImage;
}

void ParsePNG(std::istream& stream)
{
    VerifySignature(stream);
    std::vector<Chunk> chunks = ParseChunks(stream);
    std::vector<Byte> decompressedImage = DecompressImage(ConcatDataChunks(chunks));


}