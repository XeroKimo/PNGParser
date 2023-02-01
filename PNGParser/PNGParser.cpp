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
#include <tuple>
#include <optional>
#include <numeric>

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

void VerifySignature(std::istream& stream)
{
    auto signature = ReadBytes<PNGSignature.size()>(stream);
    if(signature != PNGSignature)
    {
        throw std::exception("PNG signature could not be matched");
    }
}

template<ChunkType Ty, bool Optional, bool Multiple>
struct ChunkContainerImpl;

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, false, false>
{
    using type = ChunkTraits<Ty>::Data;
};

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, false, true>
{
    using type = std::vector<typename ChunkTraits<Ty>::Data>;
};

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, true, false>
{
    using type = std::optional<typename ChunkTraits<Ty>::Data>;
};

template<ChunkType Ty>
struct ChunkContainerImpl<Ty, true, true>
{
    using type = std::vector<typename ChunkTraits<Ty>::Data>;
};

template<ChunkType Ty>
using ChunkContainer = ChunkContainerImpl<Ty, ChunkTraits<Ty>::is_optional, ChunkTraits<Ty>::multiple_allowed>::type;

using StandardChunks = std::tuple<
    ChunkContainer<"IHDR">,
    ChunkContainer<"IDAT">>;

struct DecodedChunks
{
    StandardChunks standardChunks;

    template<ChunkType Ty>
    ChunkContainer<Ty>& Get() noexcept
    {
        return std::get<ChunkContainer<Ty>>(standardChunks);
    }

    template<ChunkType Ty>
    const ChunkContainer<Ty>& Get() const noexcept
    {
        return std::get<ChunkContainer<Ty>>(standardChunks);
    }
};


class ChunkDecoder
{
    //void OrderingConstraintSignature(std::istream& stream) {  }

private:
    DecodedChunks m_chunks;
    //decltype(&OrderingConstraintSignature) m_parseChunkState = &FirstChunk;

public:
    ChunkDecoder(std::istream& stream)
    {
        while(!stream.eof())
        {
            try
            {
                //(this->*m_parseChunkState)(stream);
                ParseChunk(stream, [](ChunkType e) {});
            }
            catch(const UnknownChunkError& e)
            {
                std::cout << e.what();
            }
        }
    }

    DecodedChunks& Chunks() & noexcept { return m_chunks; }
    const DecodedChunks& Chunks() const & noexcept { return m_chunks; }
    DecodedChunks& Chunks() && noexcept { return m_chunks; }
    const DecodedChunks& Chunks() const && noexcept { return m_chunks; }



private:
    template<std::invocable<ChunkType> OrderingConstraintCheck>
    ChunkType ParseChunk(std::istream& stream, OrderingConstraintCheck fn)
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

        VisitParseChunkData(stream, chunkSize, type, fn);
        std::uint32_t crc = ParseBytes<std::uint32_t>(stream);

        crcCleanUp.Disengage();

        return type;
    }

    template<std::invocable<ChunkType> OrderingConstraintCheck>
    void VisitParseChunkData(std::istream& stream, std::uint32_t chunkSize, ChunkType type, OrderingConstraintCheck fn)
    {
        ChunkDataInputStream chunkStream{ stream, chunkSize };

        fn(type);

        switch(type)
        {
        case "IHDR"_ct:
            ParseChunkData<"IHDR">(chunkStream);
            break;
        case "IDAT"_ct:
            ParseChunkData<"IDAT">(chunkStream);
            break;
        case "IEND"_ct:
            break;
        default:
            throw UnknownChunkError{ type };
            break;
        }

        if(chunkStream.HasUnreadData())
        {
            throw std::logic_error("Chunk data has not been fully parsed");
        }
    }

    template<ChunkType Ty>
    void ParseChunkData(ChunkDataInputStream& stream)
    {
        if constexpr(ChunkTraits<Ty>::multiple_allowed)
        {
            m_chunks.Get<Ty>().push_back(ChunkTraits<Ty>::Parse(stream));
        }
        else
        {
            m_chunks.Get<Ty>() = ChunkTraits<Ty>::Parse(stream);
        }
    }

//Ordering constraint functions, no clue if it's important
//private:
//    void FirstChunk(std::istream& stream)
//    {
//        auto orderingConstraint = [](ChunkType type)
//        {
//            if(type != "IHDR")
//            {
//                throw std::exception("Header chunk not found");
//            }
//        };
//        ParseChunk(stream, orderingConstraint);
//
//        ChangeState(BeforePLTE);
//    }
//
//    void BeforePLTE(std::istream& stream)
//    {
//        auto orderingConstraint = [](ChunkType type)
//        {
//            if(type != "IHDR")
//            {
//                throw std::exception("Header chunk not found");
//            }
//        };
//        ChunkType type = ParseChunk(stream, orderingConstraint);
//
//        if(type == "IDAT")
//            ChangeState(DuringIDAT);
//        else if(type == "bKGD" || type == "hIST" || type == "tRNS")
//            ChangeState(BeforeIDAT);
//    }
//
//    void BeforeIDAT(std::istream& stream)
//    {
//        auto orderingConstraint = [](ChunkType type)
//        {
//            if(type != "IHDR")
//            {
//                throw std::exception("Header chunk not found");
//            }
//        };
//        ChunkType type = ParseChunk(stream, orderingConstraint);
//
//        if(type == "IDAT")
//            ChangeState(DuringIDAT);
//    }
//
//    void DuringIDAT(std::istream& stream)
//    {
//        auto orderingConstraint = [](ChunkType type)
//        {
//        };
//
//        if(ParseChunk(stream, orderingConstraint) != "IDAT")
//        {
//            m_parseChunkState = PostIDAT;
//        }
//    }
//
//    void PostIDAT(std::istream& stream)
//    {
//        std::uint32_t chunkSize = ParseBytes<std::uint32_t>(stream);
//        ChunkType type{ ReadBytes<4>(stream) };
//
//        if(type != "IHDR")
//        {
//            throw std::exception("Header chunk not found");
//        }
//    }
//
//    void ChangeState(decltype(&OrderingConstraintSignature) stateFunction)
//    {
//        m_parseChunkState = stateFunction;
//    }
};

std::vector<Byte> ConcatDataChunks(std::span<const ChunkData<"IDAT">> dataChunks)
{
    size_t sizeWritten = 0;
    size_t totalSize = std::accumulate(dataChunks.begin(), dataChunks.end(), size_t(0), [](size_t val, const ChunkData<"IDAT">& d) { return val + d.bytes.size(); });

    std::vector<Byte> dataBytes;
    dataBytes.resize(totalSize);

    for(const ChunkData<"IDAT">& data : dataChunks)
    {
        std::copy(data.bytes.begin(), data.bytes.end(), dataBytes.begin() + sizeWritten);
        sizeWritten += data.bytes.size();
    }

    return dataBytes;
}

std::vector<Byte> DecompressImage(std::vector<Byte> dataBytes, const ChunkData<"IHDR">& headerData)
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

    auto imageSize = headerData.FilteredImageSize();
    std::vector<Byte> decompressedImage;
    decompressedImage.resize(headerData.FilteredImageSize());

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
    ImageDefilterer(std::span<Byte> decompressedImage, const ChunkData<"IHDR">& headerData)
    {
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
    ImageDefilterer(FilteredReducedImageView reducedImage)
    {
        m_bytesPerPixel = reducedImage.header->BytesPerPixel();

        m_currentScanline.resize(reducedImage.ScanlineSize() + m_bytesPerPixel);
        m_previousScanline.resize(reducedImage.ScanlineSize() + m_bytesPerPixel);

        m_imageBytes.resize(reducedImage.ImageSize());

        static constexpr std::array<decltype(&ImageDefilterer::NoFilter), 5> defilterFunctions
        {
            &ImageDefilterer::NoFilter,
            &ImageDefilterer::SubFilter,
            &ImageDefilterer::UpFilter,
            &ImageDefilterer::AverageFilter,
            &ImageDefilterer::PaethFilter
        };

        for(size_t i = 0; i < reducedImage.height; i++)
        {
            auto filterByte = reducedImage.FilterByte(i);
            auto scanlineData = reducedImage.Scanline(i);
            std::copy(scanlineData.begin(), scanlineData.end(), m_currentScanline.begin() + m_bytesPerPixel);

            std::span<Byte> outputScanline{ m_imageBytes.data() + reducedImage.ScanlineSize() * i, reducedImage.ScanlineSize() };
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

//std::vector<Byte> DeinterlaceImage(std::vector<Byte> decompressedImage, const ChunkData<"IHDR">& headerChunk)
//{
//    switch(headerChunk.interlaceMethod)
//    {
//
//    }
//}

std::vector<Byte> DefilterImage(std::vector<Byte> decompressedImage, const ChunkData<"IHDR">& headerChunk)
{
    switch(headerChunk.interlaceMethod)
    {
    case InterlaceMethod::None:
        return ImageDefilterer(decompressedImage, headerChunk).Get().bytes;
    case InterlaceMethod::Adam7:
        return ImageDefilterer(decompressedImage, headerChunk).Get().bytes;
        break;
    }

    throw std::exception("Unknown filter type");


}

static constexpr size_t filteredIm = 0;

ReducedImage DeinterlaceImage(std::span<ReducedImage, Adam7::passCount> reducedImages)
{
    const ChunkData<"IHDR">* header = reducedImages[0].header;
    ReducedImage deinterlacedImage;
    deinterlacedImage.header = reducedImages[0].header;
    deinterlacedImage.width = header->width;
    deinterlacedImage.height = header->height;
    deinterlacedImage.bytes.resize(header->ImageSize());

    size_t i = 6;
    for(size_t i = 0; i < reducedImages.size(); i++)
    {
        ReducedImage& currentImage = reducedImages[i];
        for(std::uint32_t y = 0; y < currentImage.height; y++)
        {
            std::span<Byte> writeScanline = deinterlacedImage.Scanline(y * Adam7::rowIncrement[i] + Adam7::startingRow[i]);
            for(std::uint32_t x = 0; x < currentImage.width; x++)
            {
                std::span<Byte> writeBytes = writeScanline.subspan((Adam7::startingCol[i] + x * Adam7::columnIncrement[i]) * header->BytesPerPixel(), header->BytesPerPixel());
                std::span<Byte> readBytes = currentImage.GetPixel(x + y * currentImage.width);

                std::copy(readBytes.begin(), readBytes.end(), writeBytes.begin());
            }
        }
    }

    return deinterlacedImage;
}

ReducedImage DefilterImage(std::vector<FilteredReducedImageView>& filteredImages, const ChunkData<"IHDR">& headerChunk)
{
    switch(headerChunk.interlaceMethod)
    {
    case InterlaceMethod::None:
    {
        ReducedImage reducedImages;
        reducedImages.header = filteredImages[0].header;
        reducedImages.width = filteredImages[0].width;
        reducedImages.height = filteredImages[0].height;
        reducedImages.bytes = ImageDefilterer(filteredImages[0]).Get().bytes;
        return reducedImages;
    }
    case InterlaceMethod::Adam7:
    {
        std::array<ReducedImage, Adam7::passCount> reducedImages;
        for(size_t i = 0; i < Adam7::passCount; i++)
        {
            reducedImages[i].header = filteredImages[i].header;
            reducedImages[i].width = filteredImages[i].width;
            reducedImages[i].height = filteredImages[i].height;
            reducedImages[i].bytes = ImageDefilterer(filteredImages[i]).Get().bytes;
        }
        return DeinterlaceImage(reducedImages);
    }
    }

    throw std::exception("Unknown filter type");


}

std::vector<FilteredReducedImageView> GetReducedImages(std::span<Byte> decompressedImage, const ChunkData<"IHDR">& headerChunk)
{
    std::vector<FilteredReducedImageView> deinterlacedImages;
    switch(headerChunk.interlaceMethod)
    {
    case InterlaceMethod::None:
    {
        FilteredReducedImageView reducedImage;
        reducedImage.header = &headerChunk;
        reducedImage.height = headerChunk.height;
        reducedImage.width = headerChunk.width;
        reducedImage.bytes = decompressedImage;

        deinterlacedImages.push_back(reducedImage);
        return deinterlacedImages;
    }
    case InterlaceMethod::Adam7:
    {
        for(size_t i = 0; i < 7; i++)
        {
            FilteredReducedImageView reducedImage;
            reducedImage.header = &headerChunk;
            reducedImage.height = headerChunk.Adam7ReducedHeight(i);
            reducedImage.width = headerChunk.Adam7ReducedWidth(i);
            reducedImage.bytes = headerChunk.Adam7ReducedImageSpan(decompressedImage, i);

            deinterlacedImages.push_back(reducedImage);
        }
        return deinterlacedImages;
    }
    }
    throw std::exception("Unknown filter type");
}

Image ParsePNG(std::istream& stream)
{
    VerifySignature(stream);

    DecodedChunks chunks = ChunkDecoder{ stream }.Chunks();
    std::vector<Byte> decompressedImage = DecompressImage(ConcatDataChunks(chunks.Get<"IDAT">()), chunks.Get<"IHDR">());
    std::vector<FilteredReducedImageView> view = GetReducedImages(decompressedImage, chunks.Get<"IHDR">());

    ReducedImage image = DefilterImage(view, chunks.Get<"IHDR">());
    Image i{};
    i.imageBytes = std::move(image.bytes);
    i.width = image.width;//headerData.Adam7ReducedWidth(filteredIm);
    i.height = image.height;//headerData.Adam7ReducedHeight(filteredIm);
    i.pitch = image.ScanlineSize();
    i.bitDepth = image.header->bitDepth * image.header->BytesPerPixel();

    return i;
}