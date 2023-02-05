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
    const DecodedChunks& Chunks() const& noexcept { return m_chunks; }
    DecodedChunks& Chunks() && noexcept { return m_chunks; }
    const DecodedChunks& Chunks() const&& noexcept { return m_chunks; }



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

    for(const ChunkData<"IDAT">&data : dataChunks)
    {
        std::copy(data.bytes.begin(), data.bytes.end(), dataBytes.begin() + sizeWritten);
        sizeWritten += data.bytes.size();
    }

    return dataBytes;
}

/*std::vector<Byte>*/
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

using ReducedImages =       std::vector<Image<PixelRepresentation::Bit, FilterState::Filtered, InterlaceState::Interlaced>>;
using ExplodedImages =      std::vector<Image<PixelRepresentation::Byte, FilterState::Filtered, InterlaceState::Interlaced>>;
using DefilteredImages =    std::vector<Image<PixelRepresentation::Byte, FilterState::Defiltered, InterlaceState::Interlaced>>;
using DeinterlacedImage =   Image<PixelRepresentation::Byte, FilterState::Defiltered, InterlaceState::Deinterlaced>;

class ImageDefilterer
{
private:
    std::vector<Byte> m_imageBytes;


public:
    ImageDefilterer(ExplodedImages::value_type reducedImage)
    {
        m_imageBytes.resize(reducedImage.ImageSize());

        ScanlineFilterer scanlines{ reducedImage.BytesPerPixel(), reducedImage.ScanlineSize(), reducedImage.ScanlineSize() };
        for(size_t i = 0; i < reducedImage.height; i++)
        {
            auto filterByte = reducedImage.filterBytes[i];
            auto scanlineData = reducedImage.GetScanline(i);
            auto imageScanline = Scanline<PixelRepresentation::Byte, Byte>::FromImage(m_imageBytes, reducedImage.width, reducedImage.width, reducedImage.bitDepth, reducedImage.subpixelCount, i);

            scanlines.ApplyFilter(scanlineData.bytes, defilterFunctions[filterByte], imageScanline.bytes);
        }
    }

    std::vector<Byte> Get()&
    {
        return m_imageBytes;
    }

    std::vector<Byte> Get() const&
    {
        return m_imageBytes;
    }

    std::vector<Byte> Get()&&
    {
        return std::move(m_imageBytes);
    }

    std::vector<Byte> Get() const&&
    {
        return m_imageBytes;
    }

};

DeinterlacedImage DeinterlaceImage(DefilteredImages reducedImages, ChunkData<"IHDR"> header)
{
    size_t i = 6;
    DeinterlacedImage deinterlacedImage;

    deinterlacedImage.bitDepth = reducedImages[i].bitDepth;
    deinterlacedImage.subpixelCount = reducedImages[i].subpixelCount;
    deinterlacedImage.width = reducedImages[i].width;
    deinterlacedImage.height = reducedImages[i].height;
    deinterlacedImage.bytes = std::move(reducedImages[i].bytes);

    return deinterlacedImage;
    switch(header.interlaceMethod)
    {
    case InterlaceMethod::None:
    {
        DeinterlacedImage deinterlacedImage;

        deinterlacedImage.bitDepth = reducedImages[0].bitDepth;
        deinterlacedImage.subpixelCount = reducedImages[0].subpixelCount;
        deinterlacedImage.width = reducedImages[0].width;
        deinterlacedImage.height = reducedImages[0].height;
        deinterlacedImage.bytes = std::move(reducedImages[0].bytes);

        return deinterlacedImage;
    }
        break;
    case InterlaceMethod::Adam7:
    {
        DeinterlacedImage deinterlacedImage;
        deinterlacedImage.bitDepth = header.bitDepth;
        deinterlacedImage.subpixelCount = header.SubpixelPerPixel();
        deinterlacedImage.width = header.width;
        deinterlacedImage.height = header.height;
        deinterlacedImage.bytes.resize(header.ImageSize());

        size_t i = 2;
        //for(size_t i = 0; i < reducedImages.size(); i++)
        {
            DefilteredImages::value_type& currentImage = reducedImages[i];
            for(std::uint32_t y = 0; y < currentImage.height; y++)
            {
                Scanline<PixelRepresentation::Byte, Byte> writeScanline = deinterlacedImage.GetScanline(y * Adam7::rowIncrement[i] + Adam7::startingRow[i]);
                for(std::uint32_t x = 0; x < currentImage.width; x++)
                {
                    std::span<Byte> writeBytes = writeScanline.GetPixel(Adam7::startingCol[i] + x * Adam7::columnIncrement[i]);
                    std::span<Byte> readBytes = currentImage.GetPixel(x + y * currentImage.width);

                    std::copy(readBytes.begin(), readBytes.end(), writeBytes.begin());
                }
            }
        }

        return deinterlacedImage;
    }
        break;
    }
    
    throw std::exception("Unknown interlace method");
}

DefilteredImages DefilterImage(ExplodedImages filteredImages, const ChunkData<"IHDR">& headerChunk)
{
    DefilteredImages images;
    switch(headerChunk.filterMethod)
    {
    case 0:
        for(const auto& i : filteredImages)
        {
            DefilteredImages::value_type image;
            image.width = i.width;
            image.height = i.height;
            image.bitDepth = i.bitDepth;
            image.subpixelCount = i.subpixelCount;
            image.bytes = std::move(ImageDefilterer(i).Get());
            images.push_back(std::move(image));
        }
        return images;
        break;
    }

    throw std::exception("Unexpected filter type");
}

ReducedImages GetReducedImages(std::vector<Byte> decompressedImage, const ChunkData<"IHDR">& headerChunk)
{
    ReducedImages deinterlacedImages;
    switch(headerChunk.interlaceMethod)
    {
    case InterlaceMethod::None:
    {
        ReducedImages::value_type reducedImage;

        reducedImage.width = headerChunk.width;
        reducedImage.height = headerChunk.height;
        reducedImage.bitDepth = headerChunk.bitDepth;
        reducedImage.subpixelCount = headerChunk.SubpixelPerPixel();
        reducedImage.bytes.resize(headerChunk.ImageSize());
        reducedImage.filterBytes.resize(headerChunk.height);

        for(std::uint32_t i = 0; i < reducedImage.filterBytes.size(); i++)
        {
            auto writeScanline = Scanline<PixelRepresentation::Bit, Byte>::FromImage(reducedImage.bytes, headerChunk.width, headerChunk.width, headerChunk.bitDepth, headerChunk.SubpixelPerPixel(), i);
            auto readScanline = Scanline<PixelRepresentation::Bit, const Byte>::FromFilteredImage(decompressedImage, headerChunk.width, headerChunk.width, headerChunk.bitDepth, headerChunk.SubpixelPerPixel(), i);
            std::copy(readScanline.first.bytes.begin(), readScanline.first.bytes.end(), writeScanline.bytes.begin());
            reducedImage.filterBytes[i] = readScanline.second;
        }

        deinterlacedImages.push_back(reducedImage);
        return deinterlacedImages;
    }
    case InterlaceMethod::Adam7:
    {
        for(size_t i = 0; i < 7; i++)
        {
            ReducedImages::value_type  reducedImage;

            reducedImage.width = headerChunk.Adam7ReducedWidth(i);
            reducedImage.height = headerChunk.Adam7ReducedHeight(i);
            reducedImage.bitDepth = headerChunk.bitDepth;
            reducedImage.subpixelCount = headerChunk.SubpixelPerPixel();
            reducedImage.bytes.resize(headerChunk.Adam7ImageSize(i));
            reducedImage.filterBytes.resize(reducedImage.height);

            std::span reducedImageView = headerChunk.Adam7ReducedImageSpan(decompressedImage, i);
            for(std::uint32_t j = 0; j < reducedImage.height; j++)
            {
                auto writeScanline = Scanline<PixelRepresentation::Bit, Byte>::FromImage(reducedImage.bytes, reducedImage.width, reducedImage.width, headerChunk.bitDepth, headerChunk.SubpixelPerPixel(), j);
                auto readScanline = Scanline<PixelRepresentation::Bit, const Byte>::FromFilteredImage(reducedImageView, reducedImage.width, reducedImage.width, headerChunk.bitDepth, headerChunk.SubpixelPerPixel(), j);
                std::copy(readScanline.first.bytes.begin(), readScanline.first.bytes.end(), writeScanline.bytes.begin());
                reducedImage.filterBytes[j] = readScanline.second;
            }
            deinterlacedImages.push_back(reducedImage);
        }
        return deinterlacedImages;
    }
    }
    throw std::exception("Unknown filter type");
}

ExplodedImages ExplodeImages(ReducedImages images, const ChunkData<"IHDR">& headerChunk)
{
    ExplodedImages explodedImages;

    for(auto image : images)
    {
        ExplodedImages::value_type i;
        i.width = image.width;
        i.height = image.height;
        i.bitDepth = image.bitDepth;
        i.subpixelCount = image.subpixelCount;
        i.bytes = std::move(image.bytes);
        i.filterBytes = std::move(image.filterBytes);

        explodedImages.push_back(i);
    }

    return explodedImages;
}

Image2 ParsePNG(std::istream& stream)
{
    VerifySignature(stream);

    DecodedChunks chunks = ChunkDecoder{ stream }.Chunks();
    std::vector<Byte> decompressedImageData = DecompressImage(ConcatDataChunks(chunks.Get<"IDAT">()), chunks.Get<"IHDR">());
    ReducedImages view = GetReducedImages(std::move(decompressedImageData), chunks.Get<"IHDR">());
    ExplodedImages explodedImages = ExplodeImages(std::move(view), chunks.Get<"IHDR">());
    DefilteredImages defilteredImages = DefilterImage(std::move(explodedImages), chunks.Get<"IHDR">());
    DeinterlacedImage deinterlacedImage = DeinterlaceImage(std::move(defilteredImages), chunks.Get<"IHDR">());
    Image2 i;
    i.width = deinterlacedImage.width;
    i.height = deinterlacedImage.height;
    i.imageBytes = std::move(deinterlacedImage.bytes);
    i.pitch = deinterlacedImage.ScanlineSize();
    i.bitDepth = deinterlacedImage.bitDepth * deinterlacedImage.BytesPerPixel();
    return i;
}