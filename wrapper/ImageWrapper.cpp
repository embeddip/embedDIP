// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "ImageWrapper.hpp"

#include "board/common.h"

namespace embedDIP
{

/**
 * @brief Creates image from predefined resolution and format.
 */
Image::Image(ImageResolution resolution, ImageFormat format) : image_(nullptr)
{
    embeddip_status_t status = createImage(resolution, format, &image_);
    // On failure, image_ remains nullptr - check with isValid() or raw()
    (void)status;  // Suppress unused variable warning in release builds
}

/**
 * @brief Creates image from explicit width, height and format.
 */
Image::Image(int width, int height, ImageFormat format) : image_(nullptr)
{
    embeddip_status_t status = createImageWH(width, height, format, &image_);
    // On failure, image_ remains nullptr - check with isValid() or raw()
    (void)status;  // Suppress unused variable warning in release builds
}

/**
 * @brief Destroys wrapper and frees owned image.
 */
Image::~Image()
{
    if (image_) {
        deleteImage(image_);
        image_ = nullptr;
    }
}

/**
 * @brief Creates empty wrapper with null image pointer.
 */
Image::Image()
{
    image_ = nullptr;  // Initially null until image is created
}

/**
 * @brief Move-constructs wrapper ownership.
 */
Image::Image(Image &&other) noexcept : image_(other.image_)
{
    other.image_ = nullptr;
}

/**
 * @brief Move-assigns wrapper ownership.
 */
Image &Image::operator=(Image &&other) noexcept
{
    if (this != &other) {
        if (image_)
            deleteImage(image_);
        image_ = other.image_;
        other.image_ = nullptr;
    }
    return *this;
}

/**
 * @brief Returns underlying mutable C image pointer.
 */
::Image *Image::raw() noexcept
{
    return image_;
}

/**
 * @brief Returns underlying mutable C image pointer (const overload).
 */
::Image *Image::raw() const noexcept
{
    return image_;
}

/**
 * @brief Allocates image channels through C API.
 */
void Image::createChals(uint8_t numChals) noexcept
{
    if (image_) {
        embeddip_status_t status = ::createChals(image_, numChals);
        if (status != EMBEDDIP_OK) {
            // Note: This is marked noexcept, so we can't throw
            // In a noexcept context, we silently fail (consider logging in production)
            // Alternative: Remove noexcept and throw exception
        }
    }
}

/**
 * @brief Checks whether channel buffers are allocated.
 */
bool Image::isChalsEmpty() const noexcept
{
    return image_ ? ::isChalsEmpty(image_) : true;
}

/**
 * @brief Compresses this image into JPEG format.
 */
int Image::compressJPEG(Image &out, int quality) const noexcept
{
    return ::compress(raw(), out.raw(), IMAGE_COMP_JPEG, quality);
}

/**
 * @brief Applies negative transform.
 */
void Image::negative(Image &out) const noexcept
{
    ::negative(raw(), out.raw());
}

/**
 * @brief Applies fixed grayscale threshold.
 */
void Image::grayscaleThreshold(Image &out, uint8_t threshold) const noexcept
{
    ::grayscaleThreshold(raw(), out.raw(), threshold);
}

/**
 * @brief Computes Otsu threshold from image pixels.
 */
uint8_t Image::OtsuThreshold() const noexcept
{
    if (!raw() || !raw()->pixels) {
        return 0;
    }
    return ::OtsuThreshold((const uint8_t *)raw()->pixels, raw()->size);
}

/**
 * @brief Applies global Otsu binarization.
 */
void Image::grayscaleOtsu(Image &out) const noexcept
{
    ::grayscaleOtsu(raw(), out.raw());
}

/**
 * @brief Applies local Otsu with block processing.
 */
void Image::grayscaleThresholdLocalOtsuTo(Image &out, int blockSize) const noexcept
{
    ::grayscaleThresholdLocalOtsu(raw(), out.raw(), blockSize);
}

/**
 * @brief Applies grayscale K-means segmentation.
 */
void Image::grayscaleKMeans(Image &out, int k) const noexcept
{
    ::grayscaleKMeans(raw(), out.raw(), k);
}

/**
 * @brief Applies color K-means segmentation.
 */
void Image::colorKMeans(Image &out, int k) const noexcept
{
    ::colorKMeans(raw(), out.raw(), k);
}

/**
 * @brief Runs grayscale region-growing from seed points.
 */
void Image::grayscaleRegionGrowing(Image &out,
                                   const Point *seeds,
                                   int numSeeds,
                                   uint8_t tolerance) const noexcept
{
    if (!raw() || !out.raw() || !seeds || numSeeds <= 0) {
        return;
    }

    // Convert to C Point array
    std::vector<::Point> cSeeds(numSeeds);
    for (int i = 0; i < numSeeds; ++i) {
        cSeeds[i].x = seeds[i].x;
        cSeeds[i].y = seeds[i].y;
    }

    ::grayscaleRegionGrowing(raw(), out.raw(), cSeeds.data(), numSeeds, tolerance);
}

/**
 * @brief Runs color region-growing from seed points.
 */
void Image::colorRegionGrowing(Image &out,
                               const Point *seeds,
                               int numSeeds,
                               float tolerance) const noexcept
{
    if (!raw() || !out.raw() || !seeds || numSeeds <= 0) {
        return;
    }

    // Convert to C Point array
    std::vector<::Point> cSeeds(numSeeds);
    for (int i = 0; i < numSeeds; ++i) {
        cSeeds[i].x = seeds[i].x;
        cSeeds[i].y = seeds[i].y;
    }

    ::colorRegionGrowing(raw(), out.raw(), cSeeds.data(), numSeeds, tolerance);
}

/**
 * @brief Computes Hough accumulator from edge image.
 */
std::vector<std::vector<int>>
Image::houghAccumulator(int numRho, int numTheta, float rhoRes, float thetaRes) const
{
    if (!raw() || numRho <= 0 || numTheta <= 0) {
        return {};
    }

    std::vector<std::vector<int>> acc(numRho, std::vector<int>(numTheta));
    std::vector<int *> rows(numRho);
    for (int i = 0; i < numRho; ++i)
        rows[i] = acc[i].data();
    ::houghTransform(raw(), rows.data(), numRho, numTheta, rhoRes, thetaRes);
    return acc;
}

/**
 * @brief Extracts line candidates from Hough accumulator.
 */
int Image::extractHoughLines(const std::vector<std::vector<int>> &acc,
                             float rhoRes,
                             float thetaRes,
                             int threshold,
                             float rhoMax,
                             std::vector<HoughLine> &lines,
                             int maxLines) const
{
    if (!raw() || maxLines <= 0 || acc.empty() || acc[0].empty()) {
        lines.clear();
        return 0;
    }

    int numRho = static_cast<int>(acc.size());
    int numTheta = numRho ? static_cast<int>(acc[0].size()) : 0;
    std::vector<int *> rows(numRho);
    for (int i = 0; i < numRho; ++i)
        rows[i] = const_cast<int *>(acc[i].data());

    std::vector<HoughLine> tmp(maxLines);
    int count = ::extractLines(rows.data(),
                               numRho,
                               numTheta,
                               rhoRes,
                               thetaRes,
                               threshold,
                               rhoMax,
                               reinterpret_cast<::Line *>(tmp.data()),
                               maxLines);
    if (count <= 0) {
        lines.clear();
        return 0;
    }

    if (count > maxLines) {
        count = maxLines;
    }

    lines.assign(tmp.begin(), tmp.begin() + count);
    return count;
}

/**
 * @brief Draws line segment between two points.
 */
void Image::drawLine(int x0, int y0, int x1, int y1, uint8_t color) const noexcept
{
    ::drawLine(raw(), x0, y0, x1, y1, color);
}

/**
 * @brief Draws line represented in Hough space.
 */
void Image::drawHoughLine(float rho, float theta, uint8_t color) const noexcept
{
    ::drawLineOnImage(raw(), rho, theta, color);
}

/**
 * @brief Applies morphological erosion to the image.
 *
 * Erosion removes pixels on object boundaries. This method stores the
 * eroded result in the provided output Image using the given structuring
 * element.
 *
 * @param[out] out         Reference to the output Image that will store
 * the eroded result.
 * @param[in]  kernel      Reference to the Kernel defining the
 * neighborhood for erosion.
 * @param[in]  iterations  Number of times erosion is applied.
 *
 */
void Image::erode(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
{
    ::erode(raw(), out.raw(), kernel.raw(), iterations);
}

/**
 * @brief Applies morphological dilation to the image.
 *
 * Dilation adds pixels to object boundaries. This method stores the
 * dilated result in the provided output Image using the given structuring
 * element.
 *
 * @param[out] out         Reference to the output Image that will store
 * the dilated result.
 * @param[in]  kernel      Reference to the Kernel defining the
 * neighborhood for dilation.
 * @param[in]  iterations  Number of times dilation is applied.
 *
 */
void Image::dilate(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
{
    ::dilate(raw(), out.raw(), kernel.raw(), iterations);
}

/**
 * @brief Applies morphological opening to the image.
 *
 * Opening is an erosion followed by a dilation using the same structuring
 * element. It is typically used to remove small objects or noise while
 * preserving larger shapes.
 *
 * @param[out] out         Reference to the output Image that will store
 * the result.
 * @param[in]  kernel      Reference to the Kernel defining the
 * neighborhood for the operation.
 * @param[in]  iterations  Number of erosion/dilation iterations.
 *
 */
void Image::opening(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
{
    ::opening(raw(), out.raw(), kernel.raw(), iterations);
}

/**
 * @brief Applies morphological closing to the image.
 *
 * Closing is a dilation followed by an erosion using the same structuring
 * element. It is typically used to fill small holes or gaps in objects
 * while preserving their shapes.
 *
 * @param[out] out         Reference to the output Image that will store
 * the result.
 * @param[in]  kernel      Reference to the Kernel defining the
 * neighborhood for the operation.
 * @param[in]  iterations  Number of dilation/erosion iterations.
 *
 */
void Image::closing(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept
{
    ::closing(raw(), out.raw(), kernel.raw(), iterations);
}

/**
 * @brief Applies power-law (gamma) transform.
 */
void Image::powerTransform(Image &out, float gamma) const noexcept
{
    ::powerTransform(raw(), out.raw(), gamma);
}

/**
 * @brief Applies linear scaling and absolute value conversion.
 */
void Image::convertScaleAbsTo(Image &out, float alpha, float beta) const noexcept
{
    ::convertScaleAbs(raw(), out.raw(), alpha, beta);
}

/**
 * @brief Applies piecewise transform from breakpoint/value LUT arrays.
 */
void Image::piecewiseTransform(Image &out,
                               const std::vector<uint8_t> &breakpoints,
                               const std::vector<uint8_t> &values) const noexcept
{
    if (breakpoints.size() == values.size() && breakpoints.size() > 0) {
        ::piecewiseTransform(
            raw(), out.raw(), breakpoints.data(), values.data(), breakpoints.size());
    }
}

/**
 * @brief Computes 256-bin histogram.
 */
int Image::histForm(std::vector<int> &histogram) const
{
    histogram.resize(256);
    return ::histForm(raw(), histogram.data());
}

/**
 * @brief Applies histogram equalization.
 */
int Image::histEq(Image &out) const
{
    return ::histEq(raw(), out.raw());
}

/**
 * @brief Applies histogram specification/matching.
 */
int Image::histSpec(Image &out, const std::vector<int> &targetHistogram) const
{
    return ::histSpec(raw(), out.raw(), targetHistogram.data());
}

/**
 * @brief Applies a 2D filter to the input image using the embedDIP core.
 *
 * This function creates a compatible C-style filter kernel and calls the
 * C filter2D function which handles channel processing and log marking.
 *
 * @param[in] filter 2D kernel as a vector of vectors.
 * @param[out] out Output image that will hold the result.
 */
void Image::filter2D(const std::vector<std::vector<float>> &filter, Image &out) const
{
    // Validate filter (non-empty and square)
    if (filter.empty() || filter.size() != filter[0].size()) {
        return;  // Silent failure - no exceptions in embedded systems
    }
    int size = static_cast<int>(filter.size());

    // Allocate a flat kernel buffer
    float *kernelFlat = new float[size * size];
    for (int i = 0; i < size; ++i)
        for (int j = 0; j < size; ++j)
            kernelFlat[i * size + j] = filter[i][j];

    // Call the C filter2D function which handles everything including log marking
    ::filter2D(raw(), out.raw(), kernelFlat, size);

    delete[] kernelFlat;
}

/**
 * @brief Applies Difference-of-Gaussians (DoG) filtering.
 *
 * @param[out] outImg Output image.
 * @param[in]  sigma1 Sigma for first Gaussian blur.
 * @param[in]  sigma2 Sigma for second Gaussian blur.
 */
void Image::dogFilter(Image &outImg, float sigma1, float sigma2) const
{
    ::dogFilter(raw(), outImg.raw(), sigma1, sigma2);
}

/**
 * @brief Applies Laplacian-of-Gaussian (LoG) filtering.
 *
 * @param[out] outImg Output image.
 * @param[in]  sigma  Sigma for Gaussian smoothing.
 */
void Image::logFilter(Image &outImg, float sigma) const
{
    ::logFilter(raw(), outImg.raw(), sigma);
}

/**
 * @brief Splits RGB image into R, G, B bands (C++ wrapper).
 *
 * Internally calls the C function \c rgbSplit() with raw pointers.
 *
 * @param[out] r Red band image.
 * @param[out] g Green band image.
 * @param[out] b Blue band image.
 */
void Image::rgbSplit(Image &r, Image &g, Image &b) const noexcept
{
    ::rgbSplit(raw(), r.raw(), g.raw(), b.raw());
}

/**
 * @brief Merges R, G, B bands into an RGB image (C++ wrapper).
 *
 * This function sets this image as the output and merges three grayscale
 * inputs.
 *
 * @param[in] r Red band image.
 * @param[in] g Green band image.
 * @param[in] b Blue band image.
 */
void Image::rgbMerge(const Image &r, const Image &g, const Image &b) noexcept
{
    ::rgbMerge(r.raw(), g.raw(), b.raw(), raw());
}

/**
 * @brief Applies separable 2D filtering with raw kernels.
 *
 * @param[out] out Output image.
 * @param[in]  sizeX Horizontal kernel length.
 * @param[in]  kernelX Horizontal kernel coefficients.
 * @param[in]  sizeY Vertical kernel length.
 * @param[in]  kernelY Vertical kernel coefficients.
 * @param[in]  delta Output offset.
 */
void Image::filter2DSeparable(Image &out,
                              int sizeX,
                              float *kernelX,
                              int sizeY,
                              float *kernelY,
                              float delta) const
{
    ::sepfilter2D(raw(), out.raw(), sizeX, kernelX, sizeY, kernelY, delta);
}

/**
 * @brief Applies separable 2D convolution using independent 1D kernels.
 *
 * This method performs a 2D convolution by first applying the vertical
 * kernel and then the horizontal kernel on the image data. It leverages
 * the underlying C implementation by converting the kernel vectors to raw
 * pointers.
 *
 * @param[out] out      Output image after convolution.
 * @param[in]  kernelX  1D horizontal convolution kernel.
 * @param[in]  kernelY  1D vertical convolution kernel.
 * @param[in]  delta    Optional offset applied after convolution.
 */
void Image::sepFilter2D(Image &out,
                        const std::vector<float> &kernelX,
                        const std::vector<float> &kernelY,
                        float delta) const noexcept
{
    ::sepfilter2D(this->raw(),
                  out.raw(),
                  static_cast<int>(kernelX.size()),
                  const_cast<float *>(kernelX.data()),
                  static_cast<int>(kernelY.size()),
                  const_cast<float *>(kernelY.data()),
                  delta);
}

/**
 * @brief Applies minimum filter with square neighborhood.
 *
 * @param[out] out Output image.
 * @param[in]  kernelSize Side length of the neighborhood window.
 */
void Image::minFilter(Image &out, int kernelSize) const
{
    ::minFilter(raw(), out.raw(), kernelSize);
}

/**
 * @brief Applies maximum filter with square neighborhood.
 *
 * @param[out] out Output image.
 * @param[in]  kernelSize Side length of the neighborhood window.
 */
void Image::maxFilter(Image &out, int kernelSize) const
{
    ::maxFilter(raw(), out.raw(), kernelSize);
}

/**
 * @brief Applies median filter with square neighborhood.
 *
 * @param[out] out Output image.
 * @param[in]  kernelSize Side length of the neighborhood window.
 */
void Image::medianFilter(Image &out, int kernelSize) const
{
    ::medianFilter(raw(), out.raw(), kernelSize);
}

/**
 * @brief Resizes image to requested output dimensions.
 */
void Image::resize(Image &out, int outWidth, int outHeight) const
{
    ::resize(raw(), out.raw(), outWidth, outHeight);
}

/**
 * @brief Computes saturated pixel-wise addition with another image.
 */
void Image::add(const Image &other, Image &out) const
{
    ::add(raw(), other.raw(), out.raw());
}

/**
 * @brief Computes color distance from this RGB image to a reference color.
 */
void Image::dist(Image &out, uint8_t r, uint8_t g, uint8_t b) const
{
    ::dist(raw(), out.raw(), r, g, b);
}

/**
 * @brief Converts pixel representation to channel representation.
 */
void Image::convertTo()
{
    ::convertTo(raw());
}

/**
 * @brief Computes forward Fourier transform.
 */
void Image::fft(Image &out) const
{
    ::fft(raw(), out.raw());
}

/**
 * @brief Computes absolute/magnitude image from complex channels.
 */
void Image::_abs_(Image &out) const
{
    ::_abs_(raw(), out.raw());
}

/**
 * @brief Computes phase image from complex channels.
 */
void Image::_phase_(Image &out) const
{
    ::_phase_(raw(), out.raw());
}

/**
 * @brief Applies logarithm transform in place.
 */
void Image::_log_() const
{
    ::_log_(raw());
}

/**
 * @brief Adds scalar value in place.
 */
void Image::_add_(float value) const
{
    ::_add_(raw(), value);
}

/**
 * @brief Normalizes intensity range in place.
 */
void Image::normalize()
{
    ::normalize(raw());
}

/**
 * @brief Shifts FFT quadrants to center low frequencies.
 */
void Image::fftshift()
{
    ::fftshift(raw());
}

/**
 * @brief Computes inverse Fourier transform.
 */
void Image::ifft(Image &out) const
{
    ::ifft(raw(), out.raw());
}

/**
 * @brief Converts polar components to complex/cartesian form.
 */
void Image::polarToCart(const Image &magnitude, Image &phase) const
{
    ::polarToCart(magnitude.raw(), phase.raw(), raw());
}

/**
 * @brief Applies frequency-domain filter mask.
 */
void Image::ffilter2D(const Image &filterMask, Image &out) const
{
    ::ffilter2D(raw(), filterMask.raw(), out.raw());
}

/**
 * @brief Computes pixel-wise difference between this image and another
 * image.
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

/**
 * @brief Generates frequency filter with dual-cutoff parameters.
 */
void Image::getFilter(FrequencyFilterType type, float cutoff1, float cutoff2)
{
    ::getFilter(raw(), type, cutoff1, cutoff2);
}

/**
 * @brief Generates frequency filter with single-cutoff parameter.
 */
void Image::getFilter(FrequencyFilterType type, float cutoff1)
{
    float dummyCutoff2 = 0.0f;  // fallback value (won't be used for low/high-pass)
    ::getFilter(raw(), type, cutoff1, dummyCutoff2);
}

/**
 * @brief Converts image format using color-conversion code.
 */
void Image::cvtColor(Image &out, ColorConversionCode code) const
{
    ::cvtColor(raw(), out.raw(), code);
}

/**
 * @brief Element-wise bitwise AND operation.
 */
void Image::bitwiseAnd(const Image &other, Image &out) const
{
    ::_and_(raw(), other.raw(), out.raw());
}

/**
 * @brief Element-wise bitwise OR operation.
 */
void Image::_or(const Image &other, Image &out) const
{
    ::_or(raw(), other.raw(), out.raw());
}

/**
 * @brief Element-wise bitwise AND operation.
 */
void Image::_and_(const Image &other, Image &out) const
{
    ::_and_(raw(), other.raw(), out.raw());
}

/**
 * @brief Element-wise bitwise XOR operation.
 */
void Image::bitwiseXor(const Image &other, Image &out) const
{
    ::_xor(raw(), other.raw(), out.raw());
}

/**
 * @brief Element-wise bitwise NOT operation (unary).
 */
void Image::bitwiseNot(Image &out) const
{
    _not(raw(), out.raw());
}

/**
 * @brief Runs grayscale graph-cut GrabCut.
 */
embeddip_status_t Image::grabCut(Image &maskImg, Rectangle roi, int iterations) const
{
    return ::grabCut(raw(), maskImg.raw(), roi, iterations);
}

/**
 * @brief Runs simplified GrabCut within ROI.
 */
void Image::grabCutLite(Image &outImg, Rectangle roi, int iterations) const
{
    ::grabCutLite(raw(), outImg.raw(), roi, iterations);
}

/**
 * @brief Thresholds image by hue interval.
 */
void Image::hueThreshold(Image &output, float minHue, float maxHue) const
{
    ::hueThreshold(raw(), output.raw(), minHue, maxHue);
}

/**
 * @brief Produces mask for inclusive RGB interval.
 */
void Image::inRange(Image &mask, const uint8_t lower[3], const uint8_t upper[3]) const
{
    ::inRange(raw(), mask.raw(), lower, upper);
}

/**
 * @brief Computes Gaussian-smoothed X/Y gradients.
 */
void Image::gaussianGradients(Image &outIx, Image &outIy, float sigma) const
{
    ::gaussianGradients(raw(), outIx.raw(), outIy.raw(), sigma);
}

/**
 * @brief Computes gradient magnitude into this image.
 */
void Image::gradientMagnitude(const Image &IxImg, const Image &IyImg)
{
    ::gradientMagnitude(IxImg.raw(), IyImg.raw(), raw());
}

/**
 * @brief Computes gradient phase into this image.
 */
void Image::gradientPhase(const Image &IxImg, const Image &IyImg)
{
    ::gradientPhase(IxImg.raw(), IyImg.raw(), raw());
}

/**
 * @brief Runs Canny edge detector.
 */
void Image::Canny(Image &outImg,
                  double threshold1,
                  double threshold2,
                  int apertureSize,
                  bool L2gradient)
{
    ::Canny(raw(), outImg.raw(), threshold1, threshold2, apertureSize, L2gradient);
}

/**
 * @brief Labels connected components.
 */
void Image::connectedComponents(Image &outImg, uint32_t *labelCount)
{
    ::connectedComponents(raw(), outImg.raw(), labelCount);
}

/**
 * @brief Extracts a selected connected-component label.
 */
void Image::extractComponent(Image &objImg, int targetLabel)
{
    ::extractComponent(raw(), objImg.raw(), targetLabel);
}

}  // namespace embedDIP
