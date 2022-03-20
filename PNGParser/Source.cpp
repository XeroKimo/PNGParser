#include <fstream>
#include <iostream>
#include <array>
#include <span>
#include <algorithm>
#include <cstdint>
#include "PNGParser.h"

bool SignatureMatches(std::span<const std::uint8_t, 8> signature)
{
    return std::equal(signature.begin(), signature.end(), pngSignature.begin());
}

bool SignatureMatches(std::uint64_t signature)
{
    return std::memcmp(&signature, pngSignature.data(), pngSignature.size());
}

struct PNGChunkHeader
{
    std::uint32_t length;
    ChunkType type;
};

int main()
{
    std::fstream image{ "Korone_NotLikeThis.png", std::ios::binary | std::ios::in };
    if(image.is_open())
    {

        std::cout << "Opened\n";
        std::array<std::uint8_t, 8> signature = ReadBytes<8>(image);

        std::uint64_t sigTest = std::bit_cast<std::uint64_t>(signature);
        std::uint64_t sigTest2;
        std::memcpy(&sigTest2, FlipEndianness(signature).data(), signature.size());

        if(SignatureMatches(signature))
        {
            std::cout << "Signature matched\n";
        }

        if(SignatureMatches(sigTest))
        {
            std::cout << "Signature matched2\n";
        }

        if(SignatureMatches(sigTest2))
        {
            std::cout << "Signature matched3\n";
        }

        PNGChunkHeader header
        {
            .length = Parse<std::uint32_t>(image),
            .type = ChunkType(Parse<std::uint32_t>(image))
        };

        if(header.type == ChunkType::Image_Header)
        {
            std::cout << "Image Header\n";
            ChunkData<ChunkType::Image_Header> ih
            {
                .width = Parse<std::uint32_t>(image),
                .height = Parse<std::uint32_t>(image),
                .bitDepth = Parse<std::uint8_t>(image),
                .type = ColorType(Parse<std::uint8_t>(image)),
                .compressionMethod = Parse<std::uint8_t>(image),
                .filterMethod = Parse<std::uint8_t>(image),
                .interlaceMethod = Parse<std::uint8_t>(image),
            };
            int i = 0;
        }
    }

    return 0;
}
