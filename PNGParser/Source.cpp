//#include "PNGParser.h"
#include <span>
#include <iostream>
#include <fstream>
#include <array>
#include <bit>

//#include "PNGParser.h"
import PNGParser;


//
//bool SignatureMatches(std::span<const std::uint8_t, 8> signature)
//{
//    return std::equal(signature.begin(), signature.end(), pngSignature.begin());
//}
//
//bool SignatureMatches(std::uint64_t signature)
//{
//    return std::memcmp(&signature, pngSignature.data(), pngSignature.size());
//}
//
//struct PNGChunkHeader
//{
//    std::uint32_t length;
//    ChunkType type;
//};
//
//void ParseImage(std::istream& image, bool flipEndianness)
//{
//    while(!image.eof())
//    {
//        PNGChunkHeader header
//        {
//            .length = Parse<std::uint32_t>(image, flipEndianness),
//            .type = ChunkType(Parse<std::uint32_t>(image, flipEndianness))
//        };
//
//        if(header.type == ChunkType::Image_Header)
//        {
//            std::cout << "Image Header\n";
//            ChunkData<ChunkType::Image_Header> ih
//            {
//                .width = Parse<std::uint32_t>(image, flipEndianness),
//                .height = Parse<std::uint32_t>(image, flipEndianness),
//                .bitDepth = Parse<std::uint8_t>(image, flipEndianness),
//                .type = ColorType(Parse<std::uint8_t>(image, flipEndianness)),
//                .compressionMethod = Parse<std::uint8_t>(image, flipEndianness),
//                .filterMethod = Parse<std::uint8_t>(image, flipEndianness),
//                .interlaceMethod = Parse<std::uint8_t>(image, flipEndianness),
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Palette)
//        {
//            std::cout << "Image Palette\n";
//            ChunkData<ChunkType::Palette> ih
//            {
//                .paletteEntries = static_cast<std::uint8_t>(header.length / 3),
//                .palette = [&]()
//                {
//                    std::array<ChunkData<ChunkType::Palette>::Color, ChunkData<ChunkType::Palette>::maxPaletteEntries> localPalette;
//                    for(size_t i = 0; i < ih.paletteEntries; i++)
//                    {
//                        localPalette[i].red = Parse<std::uint8_t>(image, flipEndianness);
//                        localPalette[i].green = Parse<std::uint8_t>(image, flipEndianness);
//                        localPalette[i].blue = Parse<std::uint8_t>(image, flipEndianness);
//                    }
//
//                    return localPalette;
//                }()
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Image_Data)
//        {
//            std::cout << "Image Data\n";
//
//            for(size_t i = 0; i < header.length; i++)
//            {
//                ReadBytes<8>(image);
//            }
//            ChunkData<ChunkType::Image_Data> ih
//            {
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Image_Trailer)
//        {
//            std::cout << "Image Trailer\n";
//            ChunkData<ChunkType::Image_Trailer> ih
//            {
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Chroma)
//        {
//            std::cout << "Chroma\n";
//            ChunkData<ChunkType::Chroma> ih
//            {
//                .whitePointX = Parse<std::uint32_t>(image, flipEndianness),
//                .whitePointY = Parse<std::uint32_t>(image, flipEndianness),
//                .redX = Parse<std::uint32_t>(image, flipEndianness),
//                .redY = Parse<std::uint32_t>(image, flipEndianness),
//                .greenX = Parse<std::uint32_t>(image, flipEndianness),
//                .greenY = Parse<std::uint32_t>(image, flipEndianness),
//                .blueX = Parse<std::uint32_t>(image, flipEndianness),
//                .blueY = Parse<std::uint32_t>(image, flipEndianness),
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Image_Gamma)
//        {
//            std::cout << "Image Gamma\n";
//            ChunkData<ChunkType::Image_Gamma> ih
//            {
//                .gamma = Parse<std::uint32_t>(image, flipEndianness)
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::ICC_Profile)
//        {
//            std::cout << "ICC_Profile\n";
//
//            for(size_t i = 0; i < header.length; i++)
//            {
//                ReadBytes<8>(image);
//            }
//            ChunkData<ChunkType::ICC_Profile> ih
//            {
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Significant_Bits)
//        {
//            std::cout << "Significant_Bits\n";
//
//            for(size_t i = 0; i < header.length; i++)
//            {
//                ReadBytes<8>(image);
//            }
//            ChunkData<ChunkType::Significant_Bits> ih
//            {
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::RGB_Color_Space)
//        {
//            std::cout << "RGB_Color_Space\n";
//
//            ChunkData<ChunkType::RGB_Color_Space> ih
//            {
//                .intent = RenderingIntent{Parse<std::uint8_t>(image, flipEndianness)}
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Transparency)
//        {
//            std::cout << "Transparency\n";
//
//            for(size_t i = 0; i < header.length; i++)
//            {
//                ReadBytes<8>(image);
//            }
//
//            ChunkData<ChunkType::Transparency> ih
//            {
//            };
//            int i = 0;
//        }
//
//        if(header.type == ChunkType::Physical_Pixel_Dimensions)
//        {
//            std::cout << "Physical_Pixel_Dimensions\n";
//
//            ChunkData<ChunkType::Physical_Pixel_Dimensions> ih
//            {
//                .pixelsPerUnitX = Parse<std::uint32_t>(image, flipEndianness),
//                .pixelsPerUnitY = Parse<std::uint32_t>(image, flipEndianness),
//                .type = UnitType{ Parse<std::uint8_t>(image, flipEndianness) }
//            };
//            int i = 0;
//        }
//
//
//        auto crc = Parse<std::uint32_t>(image, flipEndianness);
//    }
//}

int main()
{
    std::fstream image{ "Korone_NotLikeThis.png", std::ios::binary | std::ios::in };

    if(image.is_open())
    {
        ParsePNG(image);
        //std::cout << "Opened\n";

        //bool flipEndianess = false;
        //std::array<std::uint8_t, 8> signature = ReadBytes<8>(image);
        //if(!SignatureMatches(signature))
        //{
        //    signature = FlipEndianness(signature);
        //    flipEndianess = true;
        //}
        //if(!SignatureMatches(signature))
        //{
        //    std::cout << "Signature match failed\n";
        //    return 0;
        //}

        //ParseImage(image, flipEndianess);
    }

    return 0;
}
