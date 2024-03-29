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
#include <limits>
#include <variant>

module PNGParser;
import :ScopeGuard;

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


class ChunkDecoder
{
    //void OrderingConstraintSignature(std::istream& stream) {  }

private:
    DecodedChunks m_chunks;
    //decltype(&OrderingConstraintSignature) m_parseChunkState = &FirstChunk;

public:
    ChunkDecoder(std::istream& stream)
    {
        while(true)
        {
            try
            {
                if(ParseChunk(stream, [](ChunkType e) {}) == "IEND")
                    break;
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
        std::uint32_t chunkSize = ReadNativeBytes<std::uint32_t>(stream);
        ChunkType type{ ReadBytes<4>(stream) };

        ScopeGuard crcCleanUp = [&]
        {
            ReadNativeBytes<std::uint32_t>(stream);
        };

        VisitParseChunkData(stream, chunkSize, type, fn);
        std::uint32_t crc = ReadNativeBytes<std::uint32_t>(stream);

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
        case "PLTE"_ct:
            ParseChunkData<"PLTE">(chunkStream);
            break;
        case "IDAT"_ct:
            ParseChunkData<"IDAT">(chunkStream);
            break;
        case "IEND"_ct:
            break;
        case "cHRM"_ct:
            ParseChunkData<"cHRM">(chunkStream);
            break;
        case "gAMA"_ct:
            ParseChunkData<"gAMA">(chunkStream);
            break;
        case "iCCP"_ct:
            ParseChunkData<"iCCP">(chunkStream);
            break;
        case "sBIT"_ct:
            ParseChunkData<"sBIT">(chunkStream);
            break;
        case "sRGB"_ct:
            ParseChunkData<"sRGB">(chunkStream);
            break;
        case "bKGD"_ct:
            ParseChunkData<"bKGD">(chunkStream);
            break;
        case "hIST"_ct:
            ParseChunkData<"hIST">(chunkStream);
            break;
        case "tRNS"_ct:
            ParseChunkData<"tRNS">(chunkStream);
            break;
        case "pHYs"_ct:
            ParseChunkData<"pHYs">(chunkStream);
            break;
        case "sPLT"_ct:
            ParseChunkData<"sPLT">(chunkStream);
            break;
        case "tIME"_ct:
            ParseChunkData<"tIME">(chunkStream);
            break;
        case "iTXt"_ct:
            ParseChunkData<"iTXt">(chunkStream);
            break;
        case "tEXt"_ct:
            ParseChunkData<"tEXt">(chunkStream);
            break;
        case "zTXt"_ct:
            ParseChunkData<"zTXt">(chunkStream);
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
            m_chunks.Get<Ty>().push_back(ChunkTraits<Ty>::Parse(stream, m_chunks));
        }
        else
        {
            m_chunks.Get<Ty>() = ChunkTraits<Ty>::Parse(stream, m_chunks);
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
    if(dataChunks.size() == 0)
        throw std::exception("No data chunks found");
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

    std::vector<Byte> decompressedImage;
    decompressedImage.resize(DecompressedImageSize(headerData));

    zstream.next_out = &decompressedImage[0];
    zstream.avail_out = decompressedImage.size();

    if(auto cont = inflate(&zstream, Z_FULL_FLUSH); !(cont == Z_OK || cont == Z_STREAM_END))
    {
        throw std::exception("unknown error");
    }

    if(zstream.avail_in != 0)
        throw std::exception("Not enough bytes to decompress");
    if(zstream.avail_out != 0)
        throw std::exception("size does not match");


    return decompressedImage;
}

//TODO: Add strong type def of Image
using ReducedImages = std::vector<Filter0::Image>;
using ExplodedImages = std::vector<Filter0::Image>;
using DefilteredImages = std::vector<Image>;
using DeinterlacedImage = Image;

DeinterlacedImage DeinterlaceImage(DefilteredImages reducedImages, ChunkData<"IHDR"> header)
{
    switch(header.interlaceMethod)
    {
    case InterlaceMethod::None:
    {
        return { std::move(reducedImages[0]) };
    }
    break;
    case InterlaceMethod::Adam7:
    {
        DeinterlacedImage deinterlacedImage{ {reducedImages[0].imageInfo.pixelInfo, header.width, header.height } };

        for(size_t i = 0; i < reducedImages.size(); i++)
        {
            DefilteredImages::value_type& currentImage = reducedImages[i];
            for(std::int32_t y = 0; y < currentImage.imageInfo.height; y++)
            {
                Scanline<Byte> writeScanline = deinterlacedImage.GetScanline(y * Adam7::rowIncrement[i] + Adam7::startingRow[i]);
                for(std::int32_t x = 0; x < currentImage.imageInfo.width; x++)
                {
                    std::span<Byte> writeBytes = writeScanline.GetPixel(Adam7::startingCol[i] + x * Adam7::columnIncrement[i]);
                    std::span<Byte> readBytes = currentImage.GetPixel(x + y * currentImage.imageInfo.width);

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
        for(const auto& image : filteredImages)
        {
            Image defilteredImage{ image.image };

            Filter0::ScanlineFilterer scanlines{ image.image.BytesPerPixel(), image.image.ScanlineSize() };
            for(size_t i = 0; i < image.image.Height(); i++)
            {
                auto filterByte = image.filterBytes[i];
                auto filteredScanline = image.image.GetScanline(i);
                auto unfilteredScanline = defilteredImage.GetScanline(i);

                scanlines.ApplyFilter(filteredScanline.bytes, Filter0::defilterFunctions[filterByte], unfilteredScanline.bytes);
            }

            images.push_back(std::move(defilteredImage));
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
        ReducedImages::value_type reducedImage{ { headerChunk.ToImageInfo() } };
        reducedImage.filterBytes.resize(headerChunk.height);

        for(std::uint32_t i = 0; i < reducedImage.filterBytes.size(); i++)
        {

            auto writeScanline = Scanline<Byte>::FromImage(reducedImage.image.bytes, reducedImage.image.imageInfo, i);
            auto readScanline = Filter0::Scanline<const Byte>(decompressedImage, reducedImage.image.imageInfo, i);
            std::copy(readScanline.first.bytes.begin(), readScanline.first.bytes.end(), writeScanline.bytes.begin());
            reducedImage.filterBytes[i] = readScanline.second;
        }

        deinterlacedImages.push_back(reducedImage);
        return deinterlacedImages;
    }
    case InterlaceMethod::Adam7:
    {
        size_t imageOffset = 0;
        Adam7::ImageInfos imageInfos{ headerChunk.ToImageInfo() };
        for(size_t i = 0; i < 7; i++)
        {
            ReducedImages::value_type reducedImage{ imageInfos.ToImageInfo(i) };
            reducedImage.filterBytes.resize(imageInfos.heights[i]);

            std::span reducedImageView = std::span(decompressedImage).subspan(imageOffset, Filter0::ImageSize(imageInfos.ToImageInfo(i)));
            imageOffset += reducedImageView.size();

            for(std::int32_t j = 0; j < reducedImage.image.imageInfo.height; j++)
            {
                auto writeScanline = Scanline<Byte>::FromImage(reducedImage.image.bytes, reducedImage.image.imageInfo, j);
                auto readScanline = Filter0::Scanline(reducedImageView, reducedImage.image.imageInfo, j);
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

    if(headerChunk.bitDepth >= 8)
    {
        for(auto& image : images)
        {
            explodedImages.push_back(std::move(image));
        }
    }
    else
    {
        for(auto& image : images)
        {
            ExplodedImages::value_type i{ std::move(image.image.imageInfo), std::move(image.filterBytes) };
            i.image.imageInfo.pixelInfo.bitDepth = 8;

            i.image.bytes.resize(i.image.imageInfo.width * i.image.imageInfo.height * i.image.BytesPerPixel());

            for(std::int32_t j = 0; j < i.image.imageInfo.height; j++)
            {
                ExplodeScanline(image.image.GetScanline(j), i.image.GetScanline(j));
            }


            explodedImages.push_back(i);
        }
    }

    return explodedImages;
}

DeinterlacedImage ConvertTo8BitDepth(DeinterlacedImage image, ChunkData<"IHDR"> header)
{
    if(header.bitDepth > 8)
    {
        DeinterlacedImage copy = image;
        image.imageInfo.pixelInfo.bitDepth = 8;
        image.bytes.resize(image.ImageSize());
        for(size_t i = 0, j = 0; i < copy.bytes.size(); i += 2, j++)
        {
            Bytes<2> initialPixel = [&]() -> Bytes<2>
            {
                if constexpr(SwapByteOrder)
                {
                    return { copy.bytes[i + 1], copy.bytes[i] };
                }
                else
                {
                    return { copy.bytes[i], copy.bytes[i + 1] };
                }
            }();
            std::uint32_t product = std::bit_cast<std::uint16_t>(initialPixel) * std::numeric_limits<std::uint8_t>::max();
            std::uint32_t finalColor = product / std::numeric_limits<std::uint16_t>::max();

            image.bytes[j] = static_cast<Byte>(finalColor);
        }
    }

    return image;
}

DeinterlacedImage ColorImage(DeinterlacedImage image, ChunkData<"IHDR"> header, const ChunkData<"PLTE">& palette)
{
    if(header.colorType == ColorType::IndexedColor)
    {
        DeinterlacedImage copy = image;
        image.imageInfo.pixelInfo.subpixelCount = 4;
        image.bytes.resize(image.ImageSize());

        for(size_t i = 0, j = 0; i < copy.bytes.size(); i++, j += 4)
        {
            std::span<const Byte, 3> paletteColor = palette.colorPalette[copy.bytes[i]];
            std::copy(paletteColor.begin(), paletteColor.end(), image.bytes.begin() + j);
            image.bytes[j + 3] = 255;
        }
    }
    else if(header.colorType == ColorType::GreyScale)
    {
        if(header.bitDepth >= 8)
        {
            DeinterlacedImage copy = image;
            image.imageInfo.pixelInfo.subpixelCount = 4;
            image.bytes.resize(image.ImageSize());
            for(size_t i = 0, j = 0; i < copy.bytes.size(); i++)
            {
                for(size_t k = 0; k < 3; j++, k++)
                {
                    image.bytes[j] = copy.bytes[i];
                }
                image.bytes[j++] = 255;
            }
        }
        else
        {
            DeinterlacedImage copy = image;
            image.imageInfo.pixelInfo.subpixelCount = 4;
            image.bytes.resize(image.ImageSize());
            Byte colorIncrements = std::numeric_limits<Byte>::max() / (std::pow(2, header.bitDepth) - 1);
            for(size_t i = 0, j = 0; i < copy.bytes.size(); i++)
            {
                for(size_t k = 0; k < 3; j++, k++)
                {
                    image.bytes[j] = copy.bytes[i] * colorIncrements;
                }
                image.bytes[j++] = 255;
            }
        }
    }
    else if(header.colorType == ColorType::GreyscaleWithAlpha)
    {
        if(header.bitDepth >= 8)
        {
            DeinterlacedImage copy = image;
            image.imageInfo.pixelInfo.subpixelCount = 4;
            image.bytes.resize(image.ImageSize());
            for(size_t i = 0, j = 0; i < copy.bytes.size(); i += 2)
            {
                for(size_t k = 0; k < 3; j++, k++)
                {
                    image.bytes[j] = copy.bytes[i];
                }
                image.bytes[j++] = copy.bytes[i + 1];
            }
        }
    }
    return image;
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
    deinterlacedImage = ConvertTo8BitDepth(std::move(deinterlacedImage), chunks.Get<"IHDR">());
    deinterlacedImage = ColorImage(std::move(deinterlacedImage), chunks.Get<"IHDR">(), chunks.Get<"PLTE">());
    Image2 i;
    i.width = deinterlacedImage.Width();
    i.height = deinterlacedImage.Height();
    i.imageBytes = std::move(deinterlacedImage.bytes);
    i.pitch = deinterlacedImage.ScanlineSize();
    i.bitDepth = deinterlacedImage.BitsPerPixel();
    return i;
}