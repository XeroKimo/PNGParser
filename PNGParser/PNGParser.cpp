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
#include <tl/expected.hpp>
#include <memory>

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

tl::expected<void, std::unique_ptr<std::exception>> VerifySignature(std::istream& stream)
{
    auto signature = ReadBytes<PNGSignature.size()>(stream);
    if(signature != PNGSignature)
    {
        return tl::unexpected(std::make_unique<std::exception>("PNG signature could not be matched"));
    }
    return {};
}


class ChunkDecoder
{
    //void OrderingConstraintSignature(std::istream& stream) {  }

private:
    DecodedChunks m_chunks;
    //decltype(&OrderingConstraintSignature) m_parseChunkState = &FirstChunk;

private:
    ChunkDecoder() = default;
public:
    static tl::expected<ChunkDecoder, std::unique_ptr<std::exception>> Create(std::istream& stream)
    {
        ChunkDecoder d;

        auto orderingConstraint = [](ChunkType e) {};
        while(!stream.eof())
        {
            auto parsedChunk = d.ParseChunk(stream);

            if(!parsedChunk)
            {
                if(dynamic_cast<UnknownChunkError*>(parsedChunk.error().get()))
                {
                    //std::cout << parsedChunk.error()->what();
                }
                else
                {
                    return tl::unexpected(std::move(parsedChunk).error());
                }
            }
        }

        return d;
    }

public:
    DecodedChunks& Chunks() & noexcept { return m_chunks; }
    const DecodedChunks& Chunks() const& noexcept { return m_chunks; }
    DecodedChunks& Chunks() && noexcept { return m_chunks; }
    const DecodedChunks& Chunks() const&& noexcept { return m_chunks; }



private:
    //template<std::invocable<ChunkType> OrderingConstraintCheck>
    AnyError<ChunkType> ParseChunk(std::istream& stream)
    {
        std::uint32_t chunkSize;
        if(auto value = ParseBytes<std::uint32_t>(stream); value)
            chunkSize = std::move(value).value();
        else
            return tl::unexpected(std::move(value).error());


        ChunkType type;
        if(auto value = ReadBytes<4>(stream); value)
        {
            if(auto value2 = ChunkType::Create(std::move(value).value()); value2)
                type = std::move(value2).value();
            else
                return tl::unexpected(std::move(value2).error());
        }
        else
            return tl::unexpected(std::move(value).error());

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

        if(auto value = VisitParseChunkData(stream, chunkSize, type); !value)
            return tl::unexpected(std::move(value).error());

        std::uint32_t crc;
        if(auto value = ParseBytes<std::uint32_t>(stream); !value)
            return tl::unexpected(std::move(value).error());

        crcCleanUp.Disengage();

        return type;
    }

    //template<std::invocable<ChunkType> OrderingConstraintCheck>
    AnyError<void> VisitParseChunkData(std::istream& stream, std::uint32_t chunkSize, ChunkType type)
    {
        ChunkDataInputStream chunkStream{ stream, chunkSize };

        //fn(type);

        switch(type)
        {
            //case "IHDR"_ct:
            //    if(auto value = ParseChunkData<"IHDR"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "PLTE"_ct:
            //    if(auto value = ParseChunkData<"PLTE"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "IDAT"_ct:
            //    if(auto value = ParseChunkData<"IDAT"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "IEND"_ct:
            //    break;
            //case "cHRM"_ct:
            //    if(auto value = ParseChunkData<"cHRM"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "gAMA"_ct:
            //    if(auto value = ParseChunkData<"gAMA"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "iCCP"_ct:
            //    if(auto value = ParseChunkData<"iCCP"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "sBIT"_ct:
            //    if(auto value = ParseChunkData<"sBIT"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "sRGB"_ct:
            //    if(auto value = ParseChunkData<"sRGB"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "bKGD"_ct:
            //    if(auto value = ParseChunkData<"bKGD"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "hIST"_ct:
            //    if(auto value = ParseChunkData<"hIST"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "tRNS"_ct:
            //    if(auto value = ParseChunkData<"tRNS"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "pHYs"_ct:
            //    if(auto value = ParseChunkData<"pHYs"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "sPLT"_ct:
            //    if(auto value = ParseChunkData<"sPLT"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "tIME"_ct:
            //    if(auto value = ParseChunkData<"tIME"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "iTXt"_ct:
            //    if(auto value = ParseChunkData<"iTXt"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "tEXt"_ct:
            //    if(auto value = ParseChunkData<"tEXt"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            //case "zTXt"_ct:
            //    if(auto value = ParseChunkData<"zTXt"_ct>(chunkStream); !value)
            //        return tl::unexpected(std::move(value).error());
            //    break;
            default:
                return tl::unexpected(std::make_unique<UnknownChunkError>(type));
                break;
        }

        if(chunkStream.HasUnreadData())
        {
            return tl::unexpected(std::make_unique<std::logic_error>("Chunk data has not been fully parsed"));
        }

        return {};
    }

    template<ChunkType Ty>
    AnyError<void> ParseChunkData(ChunkDataInputStream& stream)
    {
        if constexpr(ChunkTraits<Ty>::multiple_allowed)
        {
            if(auto value = ChunkTraits<Ty>::Parse(stream, m_chunks); value)
                m_chunks.Get<Ty>().push_back(std::move(value).value());
            else
                return tl::unexpected(std::move(value).error());
        }
        else
        {
            if(auto value = ChunkTraits<Ty>::Parse(stream, m_chunks); value)
                m_chunks.Get<Ty>() = std::move(value).value();
            else
                return tl::unexpected(std::move(value).error());
        }

        return {};
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

AnyError<std::vector<Byte>> ConcatDataChunks(std::span<const ChunkData<"IDAT"_ct>> dataChunks)
{
    if(dataChunks.size() == 0)
        return tl::unexpected(std::make_unique<std::exception>("No data chunks found"));
    size_t sizeWritten = 0;
    size_t totalSize = std::accumulate(dataChunks.begin(), dataChunks.end(), size_t(0), [](size_t val, const ChunkData<"IDAT"_ct>& d) { return val + d.bytes.size(); });

    std::vector<Byte> dataBytes;
    dataBytes.resize(totalSize);

    for(const ChunkData<"IDAT"_ct>&data : dataChunks)
    {
        std::copy(data.bytes.begin(), data.bytes.end(), dataBytes.begin() + sizeWritten);
        sizeWritten += data.bytes.size();
    }

    return dataBytes;
}

AnyError<std::vector<Byte>> DecompressImage(std::vector<Byte> dataBytes, const ChunkData<"IHDR"_ct>& headerData)
{
    z_stream zstream = {};
    zstream.next_in = dataBytes.data();
    zstream.avail_in = dataBytes.size();

    if(inflateInit(&zstream) != Z_OK)
        return tl::unexpected(std::make_unique<std::exception>("zstream failed to initialize"));

    ScopeGuard endInflate = [&zstream]
    {
        if(inflateEnd(&zstream) != Z_OK)
        {
            std::terminate();
        }
    };

    std::vector<Byte> decompressedImage;
    if(auto value = DecompressedImageSize(headerData); value)
    {
        decompressedImage.resize(std::move(value).value());
    }
    else
    {
        return tl::unexpected(std::move(value).error());
    }

    zstream.next_out = &decompressedImage[0];
    zstream.avail_out = decompressedImage.size();

    if(auto cont = inflate(&zstream, Z_FULL_FLUSH); !(cont == Z_OK || cont == Z_STREAM_END))
    {
        return tl::unexpected(std::make_unique<std::exception>("unknown error"));
    }

    if(zstream.avail_in != 0)
        return tl::unexpected(std::make_unique<std::exception>("Not enough bytes to decompress"));
    if(zstream.avail_out != 0)
        return tl::unexpected(std::make_unique<std::exception>("size does not match"));


    return decompressedImage;
}

//TODO: Add strong type def of Image
using ReducedImages = std::vector<Filter0::Image>;
using ExplodedImages = std::vector<Filter0::Image>;
using DefilteredImages = std::vector<Image>;
using DeinterlacedImage = Image;

AnyError<DeinterlacedImage> DeinterlaceImage(DefilteredImages reducedImages, ChunkData<"IHDR"_ct> header)
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

    return tl::unexpected(std::make_unique<std::exception>("Unknown interlace method"));
}

AnyError<DefilteredImages> DefilterImage(ExplodedImages filteredImages, const ChunkData<"IHDR"_ct>& headerChunk)
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

    return tl::unexpected(std::make_unique<std::exception>("Unexpected filter type"));
}

AnyError<ReducedImages> GetReducedImages(std::vector<Byte> decompressedImage, const ChunkData<"IHDR"_ct>& headerChunk)
{
    ReducedImages deinterlacedImages;
    switch(headerChunk.interlaceMethod)
    {
        case InterlaceMethod::None:
        {
            if(auto value = headerChunk.ToImageInfo(); value)
            {
                ReducedImages::value_type reducedImage{ std::move(value).value() };
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
            else
            {
                return tl::unexpected(std::move(value).error());
            }
        }
        case InterlaceMethod::Adam7:
        {
            size_t imageOffset = 0;
            if(auto value = headerChunk.ToImageInfo(); value)
            {
                Adam7::ImageInfos imageInfos{ std::move(value).value() };
                for(size_t i = 0; i < 7; i++)
                {
                    ReducedImages::value_type reducedImage{ imageInfos.ToImageInfo(i).value() };
                    reducedImage.filterBytes.resize(imageInfos.heights[i]);

                    std::span reducedImageView = std::span(decompressedImage).subspan(imageOffset, Filter0::ImageSize(imageInfos.ToImageInfo(i).value()));
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
            else
            {
                return tl::unexpected(std::move(value).error());
            }
        }
    }
    return tl::unexpected(std::make_unique<std::exception>("Unknown filter type"));
}

ExplodedImages ExplodeImages(ReducedImages images, const ChunkData<"IHDR"_ct>& headerChunk)
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

DeinterlacedImage ConvertTo8BitDepth(DeinterlacedImage image, ChunkData<"IHDR"_ct> header)
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

DeinterlacedImage ColorImage(DeinterlacedImage image, ChunkData<"IHDR"_ct> header, const ChunkData<"PLTE"_ct>& palette)
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

AnyError<Image2> ParsePNG(std::istream& stream)
{
    tl::expected<void, std::unique_ptr<std::exception>> signatureVerifier = VerifySignature(stream);

    if(!signatureVerifier)
        return tl::unexpected(std::move(signatureVerifier).error());

    return ChunkDecoder::Create(stream).and_then([](ChunkDecoder decoder) -> AnyError<Image2>
        {
            DecodedChunks chunks = std::move(decoder).Chunks();

            AnyError<DeinterlacedImage> image = ConcatDataChunks(chunks.Get<"IDAT"_ct>())
                .and_then([&](std::vector<Byte> bytes)
                    {
                        return DecompressImage(std::move(bytes), chunks.Get<"IHDR"_ct>());
                    })
                .and_then([&](std::vector<Byte> bytes)
                    {
                        return GetReducedImages(std::move(bytes), chunks.Get<"IHDR"_ct>());
                    })
                .and_then([&](ReducedImages images) -> AnyError<ExplodedImages>
                    {
                        return ExplodeImages(std::move(images), chunks.Get<"IHDR"_ct>());
                    })
                .and_then([&](ExplodedImages images)
                    {
                        return DefilterImage(std::move(images), chunks.Get<"IHDR"_ct>());
                    })
                .and_then([&](DefilteredImages images)
                    {
                        return DeinterlaceImage(std::move(images), chunks.Get<"IHDR"_ct>());
                    })
                .and_then([&](DeinterlacedImage images) -> AnyError<DeinterlacedImage>
                    {
                        images = ConvertTo8BitDepth(std::move(images), chunks.Get<"IHDR"_ct>());
                        images = ColorImage(std::move(images), chunks.Get<"IHDR"_ct>(), chunks.Get<"PLTE"_ct>());
                        return images;
                    });
            if(!image)
                return tl::unexpected(std::move(image).error());

            DeinterlacedImage deinterlacedImage = std::move(image).value();
            Image2 i;
            i.width = deinterlacedImage.Width();
            i.height = deinterlacedImage.Height();
            i.imageBytes = std::move(deinterlacedImage.bytes);
            i.pitch = deinterlacedImage.ScanlineSize();
            i.bitDepth = deinterlacedImage.BitsPerPixel();
            return i;
        });
}