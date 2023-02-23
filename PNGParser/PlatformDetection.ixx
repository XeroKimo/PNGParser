module;

#include <bit>
#include <array>
#include <algorithm>
#include "tl/expected.hpp"
#include <memory>
#include <stdexcept>

export module PNGParser:PlatformDetection;

constexpr bool IsPlatformNetworkByteOrder = std::endian::native == std::endian::big;
constexpr bool SwapByteOrder = !IsPlatformNetworkByteOrder;

export using Byte = std::uint8_t;

export enum class PNGError : int
{
    Unknown_Error,
    Stream_Initialization_Failure,
    Size_Mismatch,
    Insufficient_Size,
    Out_Of_Range_Access,
    String_Not_Null_Terminated,
    Unsupported_Color_Format,
    Chunk_Expected_Size_Exceeded,
    Chunk_Not_Fully_Parsed,
    No_Chunks_Found,
    Unknown_Signature,
    Unknown_Interlace_Method,
    Unknown_Filter_Type,
    Unknown_Parse_Error,
    Unknown_Chunk
};

template<class Ty>
using AnyError = tl::expected<Ty, PNGError>;

export template<size_t Count>
using Bytes = std::array<Byte, Count>;

export template<size_t Count>
constexpr Bytes<Count> FlipEndianness(Bytes<Count> bytes)
{
    Bytes<Count> newBytes;
    std::reverse_copy(bytes.begin(), bytes.end(), newBytes.begin());
    return newBytes;
}

export template<size_t Count>
constexpr Bytes<Count> ToNativeRepresentation(Bytes<Count> bytes)
{
    if constexpr(SwapByteOrder)
        return FlipEndianness(std::move(bytes));
    else
        return bytes;
}

export inline constexpr Bytes<8> PNGSignature = Bytes<8>{ 137, 80, 78, 71, 13, 10, 26, 10 };