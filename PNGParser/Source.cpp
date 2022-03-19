#include <fstream>
#include <iostream>
#include <array>
#include <span>
#include <algorithm>
#include <cstdint>
#include "PNGParser.h"

static constexpr std::array<std::uint8_t, 8> pngSignature
{ 
    137 , 
    80  , 
    78  , 
    71  , 
    13  , 
    10  , 
    26  , 
    10   
};

bool SignatureMatches(std::span<const std::uint8_t, 8> signature)
{
    return std::equal(signature.begin(), signature.end(), pngSignature.begin());
}

constexpr std::uint32_t BytesToIntBigEndian(const std::uint8_t b4, const std::uint8_t b3, std::uint8_t b2, const std::uint8_t b1)
{
    uint32_t i4 = (b4 << 24);
    uint32_t i3 = (b3 << 16);
    uint32_t i2 = (b2 << 8);
    uint32_t i1 = (b1 << 0);
    return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}

constexpr std::uint32_t BytesToIntBigEndian(std::span<const std::uint8_t, 4> bytes)
{
    return BytesToIntBigEndian(bytes[0], bytes[1], bytes[2], bytes[3]);
}


constexpr std::uint32_t BytesToIntBigEndian(const std::array<std::uint8_t, 4>& bytes)
{
    return BytesToIntBigEndian(bytes[0], bytes[1], bytes[2], bytes[3]);
}

struct PNGChunkHeader
{
    std::uint32_t length;
    ChunkType type;
};


struct ImageHeader
{
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t bitDepth;
    ColorType type;
    std::uint8_t compressionMethod;
    std::uint8_t filterMethod;
    std::uint8_t interlaceMethod;
};

int main()
{
    std::fstream image{ "Korone_NotLikeThis.png", std::ios::binary | std::ios::in };
    if(image.is_open())
    {

        std::cout << "Opened\n";
        std::array<std::uint8_t, 8> signature = ReadBytes<8>(image);

        if(SignatureMatches(signature))
        {
            std::cout << "Signature matched\n";
        }

        PNGChunkHeader header
        {
            .length = Parse<std::uint32_t>(image),
            .type = ChunkType(Parse<std::uint32_t>(image))
        };

        if(header.type == ChunkType::Image_Header)
        {
            std::cout << "Image Header\n";
            ImageHeader ih
            {
                .width = BytesToIntBigEndian(ReadBytes<4>(image)),
                .height = BytesToIntBigEndian(ReadBytes<4>(image)),
                .bitDepth = ReadBytes<1>(image)[0],
                .type = ColorType(ReadBytes<1>(image)[0]),
                .compressionMethod = ReadBytes<1>(image)[0],
                .filterMethod = ReadBytes<1>(image)[0],
                .interlaceMethod = ReadBytes<1>(image)[0],
            };
            int i = 0;
        }
    }

    return 0;
}
