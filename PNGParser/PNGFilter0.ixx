module;

#include <concepts>
#include <array>
#include <vector>
#include <span>
#include <algorithm>

export module PNGParser:PNGFilter0;
import :PlatformDetection;
import :Image;

namespace Filter0
{
    constexpr Byte filterByteCount = 1;

    constexpr std::size_t ScanlineSize(const ImageInfo& info) noexcept
    {
        return info.ScanlineSize() + filterByteCount;
    }

    constexpr std::size_t ImageSize(const ImageInfo& info) noexcept
    {
        return info.height * ScanlineSize(info);
    }

    template<class ByteTy>
        requires std::unsigned_integral<ByteTy> && (sizeof(ByteTy) == 1)
    std::pair<::Scanline<ByteTy>, Byte> Scanline(std::span<ByteTy> imageBytes, ImageInfo info, size_t scanlineIndex)
    {
        std::span scanline = imageBytes.subspan(scanlineIndex * ScanlineSize(info), ScanlineSize(info));
        return { ::Scanline<ByteTy>(info.pixelInfo, info.width, scanline.subspan(filterByteCount, info.ScanlineSize())), scanline[0] };
    }

    struct Image
    {
        ::Image image;
        std::vector<Byte> filterBytes;
    };


    template<class BinaryOp>
        requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
    constexpr Byte NoFilter(Byte x, Byte a, Byte b, Byte c) noexcept
    {
        return x;
    }

    template<class BinaryOp>
        requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
    constexpr Byte SubFilter(Byte x, Byte a, Byte b, Byte c) noexcept
    {
        return BinaryOp{}(x, a);
    }

    template<class BinaryOp>
        requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
    constexpr Byte UpFilter(Byte x, Byte a, Byte b, Byte c) noexcept
    {
        return BinaryOp{}(x, b);
    }

    template<class BinaryOp>
        requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
    constexpr Byte AverageFilter(Byte x, Byte a, Byte b, Byte c) noexcept
    {
        return BinaryOp{}(x, (a + b) / 2);
    }

    constexpr Byte PaethPredictor(std::int16_t a, std::int16_t b, std::int16_t c) noexcept
    {
        auto abs = [](std::int16_t v) { return (v > 0) ? v : -v; };

        std::int16_t p = a + b - c;
        std::int16_t pa = abs(p - a);
        std::int16_t pb = abs(p - b);
        std::int16_t pc = abs(p - c);

        if(pa <= pb && pa <= pc)
            return static_cast<Byte>(a);
        else if(pb <= pc)
            return static_cast<Byte>(b);
        else
            return static_cast<Byte>(c);
    }

    template<class BinaryOp>
        requires std::is_invocable_r_v<Byte, BinaryOp, Byte, Byte>
    constexpr Byte PaethFilter(Byte x, Byte a, Byte b, Byte c) noexcept
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
        std::vector<Byte> m_currentScanline;
        std::vector<Byte> m_previousScanline;

    public:
        /// <summary>
        /// Scanline filterer only works when subpixel takes up a byte
        /// </summary>
        /// <param name="bytesPerPixel">How many bytes are there per pixel</param>
        /// <param name="scanlinePitch">How many bytes there are per scanline without the filter byte</param>
        ScanlineFilterer(std::uint8_t bytesPerPixel, std::uint32_t scanlinePitch) :
            m_bytesPerPixel(bytesPerPixel),
            m_currentScanline(scanlinePitch + bytesPerPixel),
            m_previousScanline(m_currentScanline)
        {

        }

        template<std::invocable<Byte, Byte, Byte, Byte> FilterFunc>
        void ApplyFilter(std::span<const Byte> scanlineBytes, FilterFunc&& filter) noexcept
        {
            std::swap(m_previousScanline, m_currentScanline);
            std::copy(scanlineBytes.begin(), scanlineBytes.end(), XBytes().begin());

            auto extractValues = [values = std::make_tuple(XBytes(), ABytes(), BBytes(), CBytes())](size_t i)
            {
                return std::make_tuple(std::get<0>(values)[i], std::get<1>(values)[i], std::get<2>(values)[i], std::get<3>(values)[i]);
            };

            for(size_t i = 0; i < ScanlineSize(); i++)
            {
                auto [x, a, b, c] = extractValues(i);
                X(i) = filter(x, a, b, c);
            }
        }

        template<std::invocable<Byte, Byte, Byte, Byte> FilterFunc>
        void ApplyFilter(std::span<const Byte> scanlineBytes, FilterFunc&& filter, std::span<Byte> filteredBytes) noexcept
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
            return std::span(m_currentScanline.begin() + m_bytesPerPixel, ScanlineSize());
        }

        std::span<Byte> ABytes() noexcept
        {
            return std::span(m_currentScanline.begin(), ScanlineSize());
        }

        std::span<Byte> BBytes() noexcept
        {
            return std::span(m_previousScanline.begin() + m_bytesPerPixel, ScanlineSize());
        }

        std::span<Byte> CBytes() noexcept
        {
            return std::span(m_previousScanline.begin(), ScanlineSize());
        }

        std::span<const Byte> XBytes() const noexcept
        {
            return std::span(m_currentScanline.begin() + m_bytesPerPixel, ScanlineSize());
        }

        std::span<const Byte> ABytes() const noexcept
        {
            return std::span(m_currentScanline.begin(), ScanlineSize());
        }

        std::span<const Byte> BBytes() const noexcept
        {
            return std::span(m_previousScanline.begin() + m_bytesPerPixel, ScanlineSize());
        }

        std::span<const Byte> CBytes() const noexcept
        {
            return std::span(m_previousScanline.begin(), ScanlineSize());
        }

        Byte& X(size_t i) noexcept
        {
            return XBytes()[i];
        }

        std::size_t ScanlineSize() const noexcept
        {
            return m_currentScanline.size() - m_bytesPerPixel;
        }
    };
}