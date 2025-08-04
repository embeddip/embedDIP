#include "ImageWrapper.hpp"
#include "board/common.h"
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

    void Image::grayscaleThreshold(Image &out, uint8_t threshold) const noexcept
    {
        ::grayscaleThreshold(raw(), out.raw(), threshold);
    }

    uint8_t Image::OtsuThreshold() const noexcept
    {
        return ::OtsuThreshold((const uint8_t *)raw()->pixels, raw()->size);
    }

    void Image::grayscaleOtsu(Image &out) const noexcept
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

    void Image::colorKMeans(Image &out, int k) const noexcept
    {
        ::colorKMeans(raw(), out.raw(), k);
    }
    void Image::grayscaleRegionGrowingTo(Image &out, int seedX, int seedY, uint8_t tolerance) const noexcept
    {
        ::grayscaleRegionGrowing(raw(), out.raw(), seedX, seedY, tolerance);
    }

    void Image::colorRegionGrowing(Image &out, int seedX, int seedY, float tolerance) const noexcept
    {
        ::colorRegionGrowing(raw(), out.raw(), seedX, seedY, tolerance);
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

    void Image::powerTransform(Image &out, float gamma) const noexcept
    {
        ::powerTransform(raw(), out.raw(), gamma);
    }

    void Image::convertScaleAbsTo(Image &out, float alpha, float beta) const noexcept
    {
        ::convertScaleAbs(raw(), out.raw(), alpha, beta);
    }

    void Image::piecewiseTransform(Image &out,
                                   const std::vector<uint8_t> &breakpoints,
                                   const std::vector<uint8_t> &values) const noexcept
    {
        ::piecewiseTransform(raw(), out.raw(),
                             breakpoints.data(), values.data(),
                             static_cast<int>(breakpoints.size()));
    }

    int Image::histForm(std::vector<int> &histogram) const
    {
        histogram.resize(256);
        return ::histForm(raw(), histogram.data());
    }

    int Image::histEq(Image &out) const
    {
        return ::histEq(raw(), out.raw());
    }

    int Image::histSpec(Image &out, const std::vector<int> &targetHistogram) const
    {
        return ::histSpec(raw(), out.raw(), targetHistogram.data());
    }

    /**
     * @brief Applies a 2D filter to the input image using the embedDIP core.
     *
     * This function creates a compatible C-style filter kernel and applies the 2D filter
     * to each channel individually using the underlying C function `filter2D_single_channel`.
     * The wrapper supports grayscale and RGB888 images.
     *
     * @param[in] filter 2D kernel as a vector of vectors.
     * @param[out] out Output image that will hold the result.
     */
    void Image::filter2D(const std::vector<std::vector<float>> &filter, Image &out) const
    {
        assert(!filter.empty() && filter.size() == filter[0].size()); // Ensure square filter
        int size = static_cast<int>(filter.size());

        // Allocate a flat kernel buffer
        float *kernelFlat = new float[size * size];
        for (int i = 0; i < size; ++i)
            for (int j = 0; j < size; ++j)
                kernelFlat[i * size + j] = filter[i][j];

        // Create and fill Filter2DContext
        Filter2DContext context;
        context.size = size;
        // context.kernel = reinterpret_cast<float(*)[size]>(kernelFlat); // not the best solution.
        context.kernel = kernelFlat; // not the best solution.

        // Dispatch filtering based on format
        if (raw()->format == IMAGE_FORMAT_GRAYSCALE)
        {
            filter2D_single_channel(raw(), out.raw(), 0, &context);
        }
        else if (raw()->format == IMAGE_FORMAT_RGB888)
        {
            filter2D_single_channel(raw(), out.raw(), 1, &context); // R
            filter2D_single_channel(raw(), out.raw(), 2, &context); // G
            filter2D_single_channel(raw(), out.raw(), 3, &context); // B
        }
        else
        {
            delete[] kernelFlat;
            // TODO
            // adad error handlng here.
            // throw std::runtime_error("Unsupported image format for filter2D");
        }

        delete[] kernelFlat;
    }

    void Image::dogFilter(Image &outImg, float sigma1, float sigma2) const
    {
        ::dogFilter(raw(), outImg.raw(), sigma1, sigma2);
    }

    void Image::logFilter(Image &outImg, float sigma) const
    {
        ::logFilter(raw(), outImg.raw(), sigma);
    }

    void Image::rgbSplit(Image &r, Image &g, Image &b) const noexcept
    /**
     * @brief Splits RGB image into R, G, B bands (C++ wrapper).
     *
     * Internally calls the C function \c rgbSplit() with raw pointers.
     *
     * @param[out] r Red band image.
     * @param[out] g Green band image.
     * @param[out] b Blue band image.
     */
    {
        ::rgbSplit(raw(), r.raw(), g.raw(), b.raw());
    }

    void Image::rgbMerge(const Image &r, const Image &g, const Image &b) noexcept
    /**
     * @brief Merges R, G, B bands into an RGB image (C++ wrapper).
     *
     * This function sets this image as the output and merges three grayscale inputs.
     *
     * @param[in] r Red band image.
     * @param[in] g Green band image.
     * @param[in] b Blue band image.
     */
    {
        ::rgbMerge(r.raw(), g.raw(), b.raw(), raw());
    }

    void Image::filter2DSeparable(Image &out, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta) const
    {
        ::filter2D_separable(raw(), out.raw(), sizeX, kernelX, sizeY, kernelY, delta);
    }

    void Image::sepFilter2D(Image &out,
                            const std::vector<float> &kernelX,
                            const std::vector<float> &kernelY,
                            float delta) const noexcept
    /**
     * @brief Applies separable 2D convolution using independent 1D kernels.
     *
     * This method performs a 2D convolution by first applying the vertical kernel and then
     * the horizontal kernel on the image data. It leverages the underlying C implementation
     * by converting the kernel vectors to raw pointers.
     *
     * @param[out] out      Output image after convolution.
     * @param[in]  kernelX  1D horizontal convolution kernel.
     * @param[in]  kernelY  1D vertical convolution kernel.
     * @param[in]  delta    Optional scaling factor applied after convolution.
     */
    {
        ::filter2D_separable(this->raw(), out.raw(),
                             static_cast<int>(kernelX.size()), const_cast<float *>(kernelX.data()),
                             static_cast<int>(kernelY.size()), const_cast<float *>(kernelY.data()),
                             delta);
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

    void Image::add(const Image &other, Image &out) const
    {
        ::add(raw(), other.raw(), out.raw());
    }

    void Image::dist(Image &out, uint8_t r, uint8_t g, uint8_t b) const
    {
        ::dist(raw(), out.raw(), r, g, b);
    }

    void Image::convertTo()
    {
        ::convertTo(raw());
    }

    // Fourier Transform
#ifdef STM32F7xx
    void Image::fft(Image &out) const
    {
        ::fft(raw(), out.raw());
    }

    void Image::_abs_(Image &out) const
    {
        ::_abs(raw(), out.raw());
    }

    void Image::_phase_(Image &out) const
    {
        ::_phase(raw(), out.raw());
    }

    void Image::_log_() const
    {
        ::logImage(raw());
    }

    void Image::_add_(float value) const
    {
        ::addScalar(raw(), value);
    }

    void Image::normalize()
    {
        ::normalize(raw());
    }

    void Image::fftshift()
    {
        ::fftshift(raw()->chals->ch[1], raw()->width, raw()->height);
    }

    void Image::ifft(Image &out) const
    {
        ::ifft(raw(), out.raw());
    }

    void Image::ifftshift()
    {
        ::fftshift(raw()->chals->ch[0], raw()->width, raw()->height);
    }

    void Image::polarToCart(const Image &magnitude, Image &phase) const
    {
        ::polarToCart(magnitude.raw(), phase.raw(), raw());
    }

    void Image::multiply(const Image &other, Image &out) const
    {
        ::multiply(raw(), other.raw(), out.raw());
    }

    void Image::ffilter2D(const Image &filterMask, Image &out) const
    {
        ::ffilter2D(raw(), filterMask.raw(), out.raw());
    }

    /**
     * @brief Computes pixel-wise difference between this image and another image.
     *
     * This method wraps the global `difference` function.
     *
     * @param[in]  other  The image to subtract from this image.
     * @param[out] out    Output image containing the difference result.
     */
    void Image::difference(const Image &other, Image &out) const
    {
        ::difference(raw(), other.raw(), out.raw());
    }

    void Image::createFrequencyMask(FrequencyFilterType type, float cutoff1, float cutoff2)
    {
        ::getMask(raw(), type, cutoff1, cutoff2);
    }

    void Image::getFilter(FrequencyFilterType type, float cutoff1, float cutoff2)
    {
        ::getFilter(raw(), type, cutoff1, cutoff2);
    }

    void Image::getFilter(FrequencyFilterType type, float cutoff1)
    {
        float dummyCutoff2 = 0.0f; // fallback value (won't be used for low/high-pass)
        ::getFilter(raw(), type, cutoff1, dummyCutoff2);
    }

#endif

    void Image::cvtColor(Image &out, ColorConversionCode code) const
    {
        ::cvtColor(raw(), out.raw(), code);
    }

    /**
     * @brief Element-wise bitwise AND operation.
     */
    void Image::bitwiseAnd(const Image &other, Image &out) const
    {
        _and(raw(), other.raw(), out.raw());
    }

    /**
     * @brief Element-wise bitwise OR operation.
     */
    void Image::bitwiseOr(const Image &other, Image &out) const
    {
        _or(raw(), other.raw(), out.raw());
    }

    /**
     * @brief Element-wise bitwise XOR operation.
     */
    void Image::bitwiseXor(const Image &other, Image &out) const
    {
        _xor(raw(), other.raw(), out.raw());
    }

    /**
     * @brief Element-wise bitwise NOT operation (unary).
     */
    void Image::bitwiseNot(Image &out) const
    {
        _not(raw(), out.raw());
    }

    void Image::grabCutLitesd(Image &maskImg, int iterations) const
    {
        ::grabCutLite_working(raw(), maskImg.raw(), iterations);
    }

    void Image::grabCutLite(Image &outImg, Rect roi, int iterations) const
    {
        ::grabCutLite(raw(), outImg.raw(), roi, iterations);
    }
    void Image::grabCutLite888(Image &outImg, Rect roi, int iterations) const
    {
        ::grabCutLite888(raw(), outImg.raw(), roi, iterations);
    }

    void Image::grabCutRGB(Image &outMask, Rect roi, int max_iter) const
    {
        ::grabCutRGB(raw(), outMask.raw(), roi, max_iter);
    }

    void Image::hueThreshold(Image &output, float minHue, float maxHue) const
    {
        ::hueThreshold(raw(), output.raw(), minHue, maxHue);
    }

} // namespace embedDIP
