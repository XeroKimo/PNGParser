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
#include <cassert>

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
        std::runtime_error(std::string("Could not parse chunk: ") + std::string(type.ToString()) + "\n"),
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

template<ChunkType Ty>
ChunkData Parse(ChunkDataInputStream& stream, std::span<const Chunk> chunks)
{
    return ChunkTraits<Ty>::Parse(stream, chunks);
}

ChunkData ParseChunkData(std::istream& stream, std::uint32_t chunkSize, ChunkType type, std::span<const Chunk> chunks)
{
    std::cout << type.ToString() << "\n";
    ChunkDataInputStream chunkStream{ stream, chunkSize };
    ChunkData data;

    switch(type)
    {
    case "IHDR"_ct:
        data = Parse<"IHDR">(chunkStream, chunks);
        break;
    case "PLTE"_ct:
        throw UnknownChunkError{ type };
        break;
    case "IDAT"_ct:
        //throw UnknownChunkError{ type };
        data = Parse<"IDAT">(chunkStream, chunks);
        break;
    case "IEND"_ct:
        break;
    case "cHRM"_ct:
        throw UnknownChunkError{ type };
        break;
    case "gAMA"_ct:
        data = Parse<"gAMA">(chunkStream, chunks);
        break;
    case "iCCP"_ct:
        throw UnknownChunkError{ type };
        break;
    case "sBIT"_ct:
        throw UnknownChunkError{ type };
        break;
    case "sRGB"_ct:
        data = Parse<"sRGB">(chunkStream, chunks);
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
    ChunkType type{ ReadBytes<4>(stream) };

    ScopeGuard endChunkCleanUp = [&]
    {
        if(type == "IEND")
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
    auto firstChunk = std::ranges::find_if(chunks, [](const Chunk& chunk) { return chunk.Type() == "IDAT"; });
 
    size_t totalSize = 0;
    for(auto begin = firstChunk; begin != chunks.end() && begin->Type() == "IDAT"; ++begin)
    {
        totalSize += begin->Data<"IDAT">().bytes.size();
    }

    std::vector<Byte> dataBytes;
    dataBytes.reserve(totalSize);

    for(auto begin = firstChunk; begin != chunks.end() && begin->Type() == "IDAT"; ++begin)
    {
        auto data = begin->Data<"IDAT">();
        std::copy(data.bytes.begin(), data.bytes.end(), std::back_inserter(dataBytes));
    }

    return dataBytes;
}

std::vector<Byte> DecompressImage(std::vector<Byte> dataBytes, std::span<const Chunk> chunks)
{
    z_stream zstream = {};
    zstream.next_in = dataBytes.data();
    zstream.avail_in = dataBytes.size();

    if(inflateInit(&zstream) != Z_OK)
        throw std::exception("zstream failed to initialize");

    ScopeGuard endInflate = [&zstream]
    {        
        if(inflateEnd(&zstream) != Z_OK)
        {
            throw std::exception("unknown error");
        }
    };

    std::vector<Byte> decompressedImage;
    decompressedImage.resize(chunks[0].Data<"IHDR">().FilteredImageSize());

    zstream.next_out = &decompressedImage[0];
    zstream.avail_out = decompressedImage.size();

    if(auto cont = inflate(&zstream, Z_FULL_FLUSH); !(cont == Z_OK || cont == Z_STREAM_END))
    {
        throw std::exception("unknown error");
    }

    assert(zstream.avail_in == 0);

    return decompressedImage;
}

Byte PaethPredictor(std::uint16_t a, std::uint16_t b, std::uint16_t c)
{
    std::uint16_t p = a + b - c;
    Byte pa = static_cast<Byte>(std::abs(p - a));
    Byte pb = static_cast<Byte>(std::abs(p - b));
    Byte pc = static_cast<Byte>(std::abs(p - c));

    if(pa <= pb && pa <= pc)
        return a;
    else if(pb <= pc)
        return b;
    else
        return c;
}

struct DefilteredImage
{
    std::vector<Byte> bytes;
};

class ImageDefilterer
{
private:
    std::uint32_t m_bytesPerPixel;
    std::vector<Byte> m_currentScanline;
    std::vector<Byte> m_previousScanline;
    std::vector<Byte> m_imageBytes;

public:
    ImageDefilterer(std::vector<Byte> decompressedImage, std::span<const Chunk> chunks)
    {
        const auto& headerData = chunks[0].Data<"IHDR">();
        m_bytesPerPixel = headerData.BytesPerPixel();

        m_currentScanline.resize(headerData.ScanlineSize() + m_bytesPerPixel);
        m_previousScanline.resize(headerData.ScanlineSize() + m_bytesPerPixel);

        m_imageBytes.resize(headerData.ImageSize());

        static constexpr std::array<decltype(&ImageDefilterer::NoFilter), 5> defilterFunctions
        {
            &ImageDefilterer::NoFilter,
            &ImageDefilterer::SubFilter,
            &ImageDefilterer::UpFilter,
            &ImageDefilterer::AverageFilter,
            &ImageDefilterer::PaethFilter
        };

        for(size_t i = 0; i < headerData.height; i++)
        {
            auto filterByte = decompressedImage[headerData.FilterByte(i)];
            auto scanlineData = headerData.ScanlineSubspanNoFilterByte(decompressedImage, i);
            std::copy(scanlineData.begin(), scanlineData.end(), m_currentScanline.begin() + m_bytesPerPixel);

            std::span<Byte> outputScanline{ m_imageBytes.data() + headerData.ScanlineSize() * i, headerData.ScanlineSize() };
            (this->*defilterFunctions[filterByte])(outputScanline);

            std::swap(m_currentScanline, m_previousScanline);
        }
    }

    DefilteredImage Get() &
    {
        return DefilteredImage(m_imageBytes);
    }

    DefilteredImage Get() const&
    {
        return DefilteredImage(m_imageBytes);
    }

    DefilteredImage Get() &&
    {
        return DefilteredImage(std::move(m_imageBytes));
    }

    DefilteredImage Get() const&&
    {
        return DefilteredImage(std::move(m_imageBytes));
    }

private:
    void NoFilter(std::span<Byte> outputScanline) noexcept
    {
        std::copy(m_currentScanline.begin() + m_bytesPerPixel, m_currentScanline.end(), outputScanline.begin());
    }
    void SubFilter(std::span<Byte> outputScanline) noexcept
    {
        for(size_t i = 0; i < outputScanline.size(); i++)
        {
            Byte x = X(i);
            Byte a = A(i);
            X(i) = x + a;
            outputScanline[i] = X(i);
        }
    }
    void UpFilter(std::span<Byte> outputScanline) noexcept
    {
        for(size_t i = 0; i < outputScanline.size(); i++)
        {
            Byte x = X(i);
            Byte b = B(i);
            X(i) = x + b;
            outputScanline[i] = X(i);
        }
    }
    void AverageFilter(std::span<Byte> outputScanline) noexcept
    {
        for(size_t i = 0; i < outputScanline.size(); i++)
        {
            Byte x = X(i);
            std::uint16_t a = A(i);
            std::uint16_t b = B(i);
            X(i) = x + static_cast<Byte>(((a + b) >> 1));
            outputScanline[i] = X(i);
        }
    }
    void PaethFilter(std::span<Byte> outputScanline) noexcept
    {
        for(size_t i = 0; i < outputScanline.size(); i++)
        {
            Byte x = X(i);
            Byte a = A(i);
            Byte b = B(i);
            Byte c = C(i);
            X(i) = x + PaethPredictor(a, b, c);
            outputScanline[i] = X(i);
        }
    }

private:
    Byte& X(size_t index) noexcept
    {
        return m_currentScanline[index + m_bytesPerPixel];
    }
    Byte& A(size_t index) noexcept
    {
        return m_currentScanline[index];
    }
    Byte& B(size_t index) noexcept
    {
        return m_previousScanline[index + m_bytesPerPixel];
    }
    Byte& C(size_t index) noexcept
    {
        return m_previousScanline[index];
    }

    const Byte& X(size_t index) const noexcept
    {
        return m_currentScanline[index + m_bytesPerPixel];
    }
    const Byte& A(size_t index) const noexcept
    {
        return m_currentScanline[index];
    }
    const Byte& B(size_t index) const noexcept
    {
        return m_previousScanline[index + m_bytesPerPixel];
    }
    const Byte& C(size_t index) const noexcept
    {
        return m_previousScanline[index];
    }
};

std::vector<Byte> DefilterImage(std::vector<Byte> decompressedImage, std::span<const Chunk> chunks)
{
    return ImageDefilterer{ decompressedImage, chunks }.Get().bytes;
}

std::vector<Byte> DeinterlaceImage(std::vector<Byte> defilteredImage, std::span<const Chunk> chunks)
{
    const auto& headerData = chunks[0].Data<"IHDR">();

    return defilteredImage;
}

Image ParsePNG(std::istream& stream)
{
    VerifySignature(stream);
    std::vector<Chunk> chunks = ParseChunks(stream);
    std::vector<Byte> decompressedImage = DecompressImage(ConcatDataChunks(chunks), chunks);

    Image i;
    i.imageBytes = DeinterlaceImage(DefilterImage(std::move(decompressedImage), chunks), chunks);
    const auto& headerData = chunks[0].Data<"IHDR">();
    i.width = headerData.width;
    i.height = headerData.height;
    i.pitch = headerData.ScanlineSize();
    i.bitDepth = headerData.bitDepth * headerData.BytesPerPixel();

    return i;
}