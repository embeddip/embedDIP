#include "ImageWrapper.hpp"

namespace embedDIP
{

    Image::Image(ImageResolution resolution, ImageFormat format)
        : image_(createImage(resolution, format)) {}

    Image::Image(int width, int height, ImageFormat format)
        : image_(createImageWH(width, height, format)) {}

    Image::~Image()
    {
        if (image_)
        {
            deleteImage(image_);
            image_ = nullptr;
        }
    }

    Image::Image(Image &&other) noexcept
        : image_(other.image_)
    {
        other.image_ = nullptr;
    }

    Image &Image::operator=(Image &&other) noexcept
    {
        if (this != &other)
        {
            if (image_)
                deleteImage(image_);
            image_ = other.image_;
            other.image_ = nullptr;
        }
        return *this;
    }

    ::Image *Image::raw() noexcept
    {
        return image_;
    }

    ::Image *Image::raw() const noexcept
    {
        return image_;
    }

    void Image::createChals(uint8_t numChals) noexcept
    {
        if (image_)
            ::createChals(image_, numChals);
    }

    bool Image::isChalsEmpty() const noexcept
    {
        return image_ ? ::isChalsEmpty(image_) : true;
    }

    void Image::negative(Image &out) const noexcept
    {
        ::negative(raw(), out.raw());
    }

    void Image::grayscaleThresholdTo(Image &out, uint8_t threshold) const noexcept
    {
        ::grayscaleThreshold(raw(), out.raw(), threshold);
    }

    uint8_t Image::computeOtsuThreshold() const noexcept
    {
        return ::computeOtsuThreshold((const uint8_t *)raw()->pixels, raw()->size);
    }

    void Image::grayscaleOtsuTo(Image &out) const noexcept
    {
        ::grayscaleOtsu(raw(), out.raw());
    }

    void Image::grayscaleThresholdLocalOtsuTo(Image &out, int blockSize) const noexcept
    {
        ::grayscaleThresholdLocalOtsu(raw(), out.raw(), blockSize);
    }

    void Image::grayscaleKMeansTo(Image &out, int k) const noexcept
    {
        ::grayscaleKMeans(raw(), out.raw(), k);
    }

    void Image::grayscaleRegionGrowingTo(Image &out, int seedX, int seedY, uint8_t tolerance) const noexcept
    {
        ::grayscaleRegionGrowing(raw(), out.raw(), seedX, seedY, tolerance);
    }

    std::vector<std::vector<int>> Image::houghAccumulator(int numRho, int numTheta,
                                                          float rhoRes, float thetaRes) const
    {
        std::vector<std::vector<int>> acc(numRho, std::vector<int>(numTheta));
        std::vector<int *> rows(numRho);
        for (int i = 0; i < numRho; ++i)
            rows[i] = acc[i].data();
        ::houghTransform(raw(), rows.data(), numRho, numTheta, rhoRes, thetaRes);
        return acc;
    }

    int Image::extractHoughLines(const std::vector<std::vector<int>> &acc,
                                 float rhoRes, float thetaRes,
                                 int threshold, float rhoMax,
                                 std::vector<HoughLine> &lines,
                                 int maxLines) const
    {
        int numRho = static_cast<int>(acc.size());
        int numTheta = numRho ? static_cast<int>(acc[0].size()) : 0;
        std::vector<int *> rows(numRho);
        for (int i = 0; i < numRho; ++i)
            rows[i] = const_cast<int *>(acc[i].data());

        std::vector<HoughLine> tmp(maxLines);
        int count = ::extractLines(rows.data(), numRho, numTheta,
                                   rhoRes, thetaRes,
                                   threshold, rhoMax,
                                   tmp.data(), maxLines);
        lines.assign(tmp.begin(), tmp.begin() + count);
        return count;
    }

    void Image::drawLine(int x0, int y0, int x1, int y1, uint8_t color) const noexcept
    {
        ::drawLine(raw(), x0, y0, x1, y1, color);
    }

    void Image::drawHoughLine(float rho, float theta, uint8_t color) const noexcept
    {
        ::drawLineOnImage(raw(), rho, theta, color);
    }

    void Image::getStructuringElement(Kernel &kernel, MorphShape shape, uint8_t size) const noexcept
    {
        ::getStructuringElement(&kernel, shape, size);
    }

    void Image::erode(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
    {
        ::erode(raw(), out.raw(), &kernel, iterations);
    }

    void Image::dilate(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
    {
        ::dilate(raw(), out.raw(), &kernel, iterations);
    }

    void Image::opening(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
    {
        ::opening(raw(), out.raw(), &kernel, iterations);
    }

    void Image::closing(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
    {
        ::closing(raw(), out.raw(), &kernel, iterations);
    }

    void Image::powerTransformTo(Image &out, float gamma, float c) const noexcept
    {
        ::powerTransform(raw(), out.raw(), gamma, c);
    }

    void Image::convertScaleAbsTo(Image &out, float alpha, float beta) const noexcept
    {
        ::convertScaleAbs(raw(), out.raw(), alpha, beta);
    }

    void Image::piecewiseTransformTo(Image &out,
                                     const std::vector<uint8_t> &breakpoints,
                                     const std::vector<uint8_t> &values) const noexcept
    {
        ::piecewiseTransform(raw(), out.raw(),
                             breakpoints.data(), values.data(),
                             static_cast<int>(breakpoints.size()));
    }

    int Image::histFormTo(std::vector<int> &histogram) const
    {
        histogram.resize(256);
        return ::histForm(raw(), histogram.data());
    }

    int Image::histEqTo(Image &out) const
    {
        return ::histEq(raw(), out.raw());
    }

    int Image::histSpecTo(Image &out, const std::vector<int> &targetHistogram) const
    {
        return ::histSpec(raw(), out.raw(), targetHistogram.data());
    }

    void Image::filter2DChannel(int ch_idx, const Filter2DContext &ctx, Image &out) const
    {
        ::filter2D_single_channel(raw(), out.raw(), ch_idx, (void *)&ctx);
    }

    void Image::filter2DSeparable(Image &out, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta) const
    {
        ::filter2D_separable(raw(), out.raw(), sizeX, kernelX, sizeY, kernelY, delta);
    }

    void Image::minFilter(Image &out, int kernelSize) const
    {
        ::minFilter(raw(), out.raw(), kernelSize);
    }

    void Image::maxFilter(Image &out, int kernelSize) const
    {
        ::maxFilter(raw(), out.raw(), kernelSize);
    }

    void Image::medianFilter(Image &out, int kernelSize) const
    {
        ::medianFilter(raw(), out.raw(), kernelSize);
    }

    void Image::resizeTo(Image &out, int size) const
    {
        ::resize(raw(), out.raw(), size);
    }

    void Image::addTo(const Image &other, Image &out) const
    {
        ::add(raw(), other.raw(), out.raw());
    }

    void Image::distanceTo(Image &out, uint8_t r, uint8_t g, uint8_t b) const
    {
        ::dist(raw(), out.raw(), r, g, b);
    }

    void Image::normalize()
    {
        ::normalize(raw());
    }

    void Image::convertTo()
    {
        ::convertTo(raw());
    }

    // Fourier Transform

    void Image::fft(Image &out) const
    {
        ::fft(raw(), out.raw());
    }

    void Image::_abs(Image &out) const
    {
        ::_abs(raw(), out.raw());
    }

    void Image::_phase(Image &out) const
    {
        ::_phase(raw(), out.raw());
    }

    void Image::_log() const
    {
        ::logImage(raw());
    }

    void Image::_add(float value) const
    {
        ::addScalar(raw(), value);
    }

    void Image::ifft(Image &out) const
    {
        ::ifft(raw(), out.raw());
    }

    void Image::polarToCart(const Image &magnitude, Image &phase) const
    {
        ::polarToCart(magnitude.raw(), phase.raw(), raw());
    }

    void Image::multiplyWith(const Image &other, Image &out) const
    {
        ::multiply(raw(), other.raw(), out.raw());
    }

    void Image::createFrequencyMask(FrequencyFilterType type, float cutoff1, float cutoff2)
    {
        ::getMask(raw(), type, cutoff1, cutoff2);
    }

    void Image::fftshift()
    {
        ::fftshift(raw()->chals->ch[1], raw()->width, raw()->height);
    }

    void Image::ifftshift()
    {
        ::fftshift(raw()->chals->ch[1], raw()->width, raw()->height);
    }

    void Image::getFilter(FrequencyFilterType type, float cutoff1, float cutoff2)
    {
        ::getFilter(raw(), type, cutoff1, cutoff2);
    }

    void Image::multiply(const Image &img2, Image &outImg) const
    {
        ::multiply(raw(), img2.raw(), outImg.raw());
    }

} // namespace embedDIP
