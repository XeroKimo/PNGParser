module;

#include <concepts>
#include <array>
#include <vector>
#include <span>
#include <algorithm>

export module PNGParser:PNGFilter0;
import :PlatformDetection;

template<class BinaryOp>
    requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
Byte NoFilter(Byte x, Byte a, Byte b, Byte c)
{
    return x;
}

template<class BinaryOp>
    requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
Byte SubFilter(Byte x, Byte a, Byte b, Byte c)
{
    return BinaryOp{}(x, a);
}

template<class BinaryOp>
    requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
Byte UpFilter(Byte x, Byte a, Byte b, Byte c)
{
    return BinaryOp{}(x, b);
}

template<class BinaryOp>
    requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
Byte AverageFilter(Byte x, Byte a, Byte b, Byte c)
{
    return BinaryOp{}(x, (a + b) / 2);
}

Byte PaethPredictor(Byte a, Byte b, Byte c)
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

template<class BinaryOp>
    requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
Byte PaethFilter(Byte x, Byte a, Byte b, Byte c)
{
    return BinaryOp{}(x, PaethPredictor(a, b, c));
}

using FilterFunctionSignature = decltype(&NoFilter<std::plus<Byte>>);

constexpr size_t numFilterFunctions = 5;

constexpr std::array<FilterFunctionSignature, numFilterFunctions> defilterFunctions
{
    NoFilter<std::plus<Byte>>,
    SubFilter<std::plus<Byte>>,
    UpFilter<std::plus<Byte>>,
    AverageFilter<std::plus<Byte>>,
    PaethFilter<std::plus<Byte>>,
};

constexpr std::array<FilterFunctionSignature, numFilterFunctions> filterFunctions
{
    NoFilter<std::minus<Byte>>,
    SubFilter<std::minus<Byte>>,
    UpFilter<std::minus<Byte>>,
    AverageFilter<std::minus<Byte>>,
    PaethFilter<std::minus<Byte>>,
};

class ScanlineFilterer
{
private:
    std::uint8_t m_bytesPerPixel;
    std::uint32_t m_imagePitch;
    std::vector<Byte> m_currentScanline;
    std::vector<Byte> m_previousScanline;

public:
    /// <summary>
    /// Scanline filterer only works when subpixel takes up a byte
    /// </summary>
    /// <param name="bytesPerPixel">How many bytes are there per pixel</param>
    /// <param name="scanlinePitch">How many bytes there are per scanline</param>
    /// <param name="imagePitch">How many bytes are actually used by the image</param>
    ScanlineFilterer(std::uint8_t bytesPerPixel, std::uint32_t scanlinePitch, std::uint32_t imagePitch) :
        m_bytesPerPixel(bytesPerPixel),
        m_imagePitch(imagePitch),
        m_currentScanline(scanlinePitch + bytesPerPixel),
        m_previousScanline(m_currentScanline)
    {
        
    }

    template<std::invocable<Byte, Byte, Byte, Byte> FilterFunc>
    void ApplyFilter(std::span<const Byte> scanlineBytes, FilterFunc&& filter)
    {
        std::swap(m_previousScanline, m_currentScanline);
        std::copy(scanlineBytes.begin(), scanlineBytes.end(), XBytes().begin());

        auto extractValues = [values = std::make_tuple(XBytes(), ABytes(), BBytes(), CBytes())](size_t i)
        {
            return std::make_tuple(std::get<0>(values)[i], std::get<1>(values)[i], std::get<2>(values)[i], std::get<3>(values)[i]);
        };
        for(size_t i = 0; i < m_imagePitch; i++)
        {
            auto [x, a, b, c] = extractValues(i);
            X(i) = filter(x, a, b, c);
        }
    }

    template<std::invocable<Byte, Byte, Byte, Byte> FilterFunc>
    void ApplyFilter(std::span<const Byte> scanlineBytes, FilterFunc&& filter, std::span<Byte> filteredBytes)
    {
        ApplyFilter(scanlineBytes, std::forward<FilterFunc>(filter));
        CopyBackCurrentScanline(filteredBytes);
    }

    void CopyBackCurrentScanline(std::span<Byte> bytes) const noexcept
    {
        std::copy(GetCurrentScanline().begin(), GetCurrentScanline().end(), bytes.begin());
    }

    std::span<const Byte> GetCurrentScanline() const noexcept
    {
        return XBytes();
    }

private:
    std::span<Byte> XBytes() noexcept
    {
        return std::span(m_currentScanline.begin() + m_bytesPerPixel, m_imagePitch);
    }

    std::span<Byte> ABytes() noexcept
    {
        return std::span(m_currentScanline.begin(), m_imagePitch);
    }

    std::span<Byte> BBytes() noexcept
    {
        return std::span(m_previousScanline.begin() + m_bytesPerPixel, m_imagePitch);
    }

    std::span<Byte> CBytes() noexcept
    {
        return std::span(m_previousScanline.begin(), m_imagePitch);
    }

    std::span<const Byte> XBytes() const noexcept
    {
        return std::span(m_currentScanline.begin() + m_bytesPerPixel, m_imagePitch);
    }

    std::span<const Byte> ABytes() const noexcept
    {
        return std::span(m_currentScanline.begin(), m_imagePitch);
    }

    std::span<const Byte> BBytes() const noexcept
    {
        return std::span(m_previousScanline.begin() + m_bytesPerPixel, m_imagePitch);
    }

    std::span<const Byte> CBytes() const noexcept
    {
        return std::span(m_previousScanline.begin(), m_imagePitch);
    }

    Byte& X(size_t i)
    {
        return XBytes()[i];
    }
};