// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

extern "C" {
#include "board/common.h"                /**< Board-level helpers and timing. */
#include "core/error.h"                  /**< Error handling and status codes. */
#include "core/image.h"                  /**< Image type and utilities. */
#include "core/memory_manager.h"         /**< Allocators and memory helpers. */
#include "device/serial/serial.h"        /**< Serial I/O abstraction. */
#include "imgproc/color.h"               /**< Color conversions and helpers. */
#include "imgproc/compress.h"            /**< JPEG compression helper. */
#include "imgproc/connectedcomponents.h" /**< Connected components labeling. */
#include "imgproc/drawing.h"             /**< Drawing primitives and shapes. */
#include "imgproc/fft.h"                 /**< Frequency-domain processing. */
#include "imgproc/filter.h"              /**< Spatial filtering and kernels. */
#include "imgproc/histogram.h"           /**< Histogram ops and equalization. */
#include "imgproc/hough.h"               /**< Hough transform for line/circle detection. */
#include "imgproc/imgwarp.h"             /**< Geometric transformations (warp, rotate, scale). */
#include "imgproc/misc.h"                /**< Miscellaneous image utilities. */
#include "imgproc/morph.h"               /**< Morphological operations (erode, dilate). */
#include "imgproc/pixel.h"               /**< Pixel operations (negative, etc). */
#include "imgproc/segmentation.h"        /**< Image segmentation algorithms. */
#include "imgproc/thresh.h"              /**< Thresholding operations. */
}

namespace embedDIP
{

/**
 * @brief Integer image coordinate used by wrapper APIs.
 */
struct Point {
    int x;  ///< X coordinate
    int y;  ///< Y coordinate
};

/**
 * @brief Hough line structure (alias for C Line type).
 */
using HoughLine = ::Line;

// C++ wrapper
class Kernel
{
  public:
    /**
     * @brief Creates an empty kernel wrapper.
     */
    Kernel() noexcept : kernel_(static_cast<::Kernel *>(memory_alloc(sizeof(::Kernel))))
    {
        if (kernel_) {
            kernel_->data = nullptr;
            kernel_->size = 0;
            kernel_->anchor = 0;
        }
    }

    /**
     * @brief Creates and initializes kernel wrapper in one step.
     * @param shape Structuring-element shape.
     * @param ksize Structuring-element size.
     */
    Kernel(MorphShape shape, uint8_t ksize) noexcept : Kernel()
    {
        if (kernel_) {
            ::getStructuringElement(kernel_, shape, ksize);
        }
    }

    /**
     * @brief Frees allocated kernel resources.
     */
    ~Kernel()
    {
        if (kernel_) {
            memory_free(kernel_->data);
            memory_free(kernel_);
        }
    }

    Kernel(const Kernel &) = delete;
    Kernel &operator=(const Kernel &) = delete;

    /**
     * @brief Move-constructs kernel ownership.
     * @param other Source kernel wrapper.
     */
    Kernel(Kernel &&other) noexcept : kernel_(other.kernel_)
    {
        other.kernel_ = nullptr;
    }

    /**
     * @brief Move-assigns kernel ownership.
     * @param other Source kernel wrapper.
     * @return Reference to this object.
     */
    Kernel &operator=(Kernel &&other) noexcept
    {
        if (this != &other) {
            if (kernel_) {
                memory_free(kernel_->data);
                memory_free(kernel_);
            }
            kernel_ = other.kernel_;
            other.kernel_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Returns underlying C kernel pointer.
     */
    ::Kernel *raw() noexcept
    {
        return kernel_;
    }
    /**
     * @brief Returns underlying C kernel pointer (const overload).
     */
    ::Kernel *raw() const noexcept
    {
        return kernel_;
    }

    /**
     * @brief Returns kernel edge length.
     */
    inline uint8_t size() const noexcept
    {
        return kernel_ ? kernel_->size : 0;
    }
    /**
     * @brief Returns kernel anchor index.
     */
    inline uint8_t anchor() const noexcept
    {
        return kernel_ ? kernel_->anchor : 0;
    }
    /**
     * @brief Returns flat kernel data pointer.
     */
    inline uint8_t *data() const noexcept
    {
        return kernel_ ? kernel_->data : nullptr;
    }

    /**
     * @brief Builds or rebuilds the structuring element (kernel) used in morphological operations.
     *
     * @param[in] shape  The morphological shape of the structuring element.
     * @param[in] ksize  The size of the kernel.
     *
     * @return Reference to the current Kernel object with the updated structuring element.
     */
    Kernel &getStructuringElement(MorphShape shape, uint8_t ksize) noexcept
    {
        if (!kernel_)
            return *this;

        // If C helper allocates, free old buffer to avoid leaks
        memory_free(kernel_->data);
        kernel_->data = nullptr;
        kernel_->size = 0;
        kernel_->anchor = 0;

        ::getStructuringElement(kernel_, shape, ksize);
        return *this;
    }

  private:
    ::Kernel *kernel_;
};

class Image
{
  public:
    // Constructors / destructor / move operations
    /**
     * @brief Creates an empty image wrapper with null internal pointer.
     */
    Image();  // default constructor declaration
    /**
     * @brief Creates an image with predefined resolution enum.
     * @param resolution Resolution preset.
     * @param format Pixel format.
     */
    Image(ImageResolution resolution, ImageFormat format);
    /**
     * @brief Creates an image using explicit width and height.
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @param format Pixel format.
     */
    Image(int width, int height, ImageFormat format);
    /**
     * @brief Releases owned image resources.
     */
    ~Image();
    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;
    /**
     * @brief Move-constructs image ownership.
     * @param other Source wrapper to move from.
     */
    Image(Image &&other) noexcept;
    /**
     * @brief Move-assigns image ownership.
     * @param other Source wrapper to move from.
     * @return Reference to this object.
     */
    Image &operator=(Image &&other) noexcept;

    // Raw access
    /**
     * @brief Returns underlying C image pointer.
     */
    ::Image *raw() noexcept;
    /**
     * @brief Returns underlying C image pointer.
     */
    ::Image *raw() const noexcept;

    /**
     * @brief Checks whether wrapper currently owns a valid C image pointer.
     */
    inline bool isValid() const noexcept
    {
        return image_ != nullptr;
    }

    /**
     * @brief Returns pixel buffer pointer.
     */
    inline void *pixels() const noexcept
    {
        return image_ ? image_->pixels : nullptr;
    }
    /**
     * @brief Returns total pixel-buffer size in bytes.
     */
    inline uint32_t size() const noexcept
    {
        return image_ ? image_->size : 0;
    }
    /**
     * @brief Returns image width in pixels.
     */
    inline uint32_t width() const noexcept
    {
        return image_ ? image_->width : 0;
    }
    /**
     * @brief Returns image height in pixels.
     */
    inline uint32_t height() const noexcept
    {
        return image_ ? image_->height : 0;
    }
    /**
     * @brief Returns image format.
     */
    inline ImageFormat format() const noexcept
    {
        return image_ ? image_->format : IMAGE_FORMAT_GRAYSCALE;
    }
    /**
     * @brief Returns image channel depth metadata.
     */
    inline ImageDepth depth() const noexcept
    {
        return image_ ? image_->depth : IMAGE_DEPTH_U8;
    }

    // Channel operations
    /**
     * @brief Allocates floating-point channels for this image.
     * @param numChals Number of channels to create.
     */
    void createChals(uint8_t numChals) noexcept;
    /**
     * @brief Checks whether channel storage is currently empty.
     * @return true if no channel buffers are allocated.
     */
    bool isChalsEmpty() const noexcept;

    // Pixel operations
    /**
     * @brief Compress this image into JPEG payload stored in output image.
     * @param[out] out Destination image buffer that will hold JPEG bytes.
     * @param[in] quality JPEG quality in range [1, 100].
     * @return 0 on success, -1 on error.
     * @see ::compress For underlying C implementation
     */
    int compressJPEG(Image &out, int quality = 75) const noexcept;

    /**
     * @brief Computes negative image transform.
     * @param[out] out Output image for inverted result
     * @see ::negative For underlying C implementation
     */
    void negative(Image &out) const noexcept;
    /**
     * @brief Applies fixed grayscale threshold.
     * @param[out] out Output binary image
     * @param[in] threshold Threshold value in [0, 255]
     * @see ::grayscaleThreshold For underlying C implementation
     */
    void grayscaleThreshold(Image &out, uint8_t threshold) const noexcept;
    /**
     * @brief Computes Otsu threshold from image pixels.
     * @return Optimal threshold value in [0, 255]
     * @see ::OtsuThreshold For underlying C implementation
     */
    uint8_t OtsuThreshold() const noexcept;
    /**
     * @brief Applies global Otsu thresholding.
     * @param[out] out Output binary image
     * @see ::grayscaleOtsu For underlying C implementation
     */
    void grayscaleOtsu(Image &out) const noexcept;
    /**
     * @brief Applies block-based local Otsu thresholding.
     * @param[out] out Output binary image
     * @param[in] blockSize Local block size in pixels
     * @see ::grayscaleThresholdLocalOtsu For underlying C implementation
     */
    void grayscaleThresholdLocalOtsuTo(Image &out, int blockSize) const noexcept;
    /**
     * @brief Segments grayscale image with K-means.
     * @param[out] out Output segmented image
     * @param[in] k Number of clusters
     * @see ::grayscaleKMeans For underlying C implementation
     */
    void grayscaleKMeans(Image &out, int k) const noexcept;

    /**
     * @brief Segments color image with K-means.
     * @param[out] out Output segmented image
     * @param[in] k Number of clusters
     * @see ::colorKMeans For underlying C implementation
     */
    void colorKMeans(Image &out, int k) const noexcept;

    /**
     * @brief Runs grayscale region-growing segmentation.
     * @param[out] out Output segmented image
     * @param[in] seeds Seed points array
     * @param[in] numSeeds Number of seed points
     * @param[in] tolerance Intensity tolerance for region membership
     * @see ::grayscaleRegionGrowing For underlying C implementation
     */
    void grayscaleRegionGrowing(Image &out,
                                const Point *seeds,
                                int numSeeds,
                                uint8_t tolerance) const noexcept;

    /**
     * @brief Runs color region-growing segmentation.
     * @param[out] out Output segmented image
     * @param[in] seeds Seed points array
     * @param[in] numSeeds Number of seed points
     * @param[in] tolerance Color-distance tolerance for region membership
     * @see ::colorRegionGrowing For underlying C implementation
     */
    void colorRegionGrowing(Image &out,
                            const Point *seeds,
                            int numSeeds,
                            float tolerance) const noexcept;
    // Hough Transform
    /**
     * @brief Computes Hough accumulator for a binary edge image.
     * @param[in] numRho Number of rho bins
     * @param[in] numTheta Number of theta bins
     * @param[in] rhoRes Rho resolution
     * @param[in] thetaRes Theta resolution
     * @return 2D accumulator array (rho x theta)
     * @see ::houghTransform For underlying C implementation
     */
    std::vector<std::vector<int>>
    houghAccumulator(int numRho, int numTheta, float rhoRes, float thetaRes) const;
    /**
     * @brief Extracts line candidates from a Hough accumulator.
     * @param[in] acc Hough accumulator array
     * @param[in] rhoRes Rho resolution
     * @param[in] thetaRes Theta resolution
     * @param[in] threshold Minimum votes for line detection
     * @param[in] rhoMax Maximum rho value
     * @param[out] lines Detected lines vector
     * @param[in] maxLines Maximum number of lines to extract
     * @return Number of extracted lines
     * @see ::extractLines For underlying C implementation
     */
    int extractHoughLines(const std::vector<std::vector<int>> &acc,
                          float rhoRes,
                          float thetaRes,
                          int threshold,
                          float rhoMax,
                          std::vector<HoughLine> &lines,
                          int maxLines = 100) const;

    // Drawing
    /**
     * @brief Draws a line segment between two points.
     * @param[in] x0 Start X coordinate
     * @param[in] y0 Start Y coordinate
     * @param[in] x1 End X coordinate
     * @param[in] y1 End Y coordinate
     * @param[in] color Line color value
     * @see ::drawLine For underlying C implementation
     */
    void drawLine(int x0, int y0, int x1, int y1, uint8_t color) const noexcept;
    /**
     * @brief Draws an infinite line defined by Hough parameters.
     * @param[in] rho Distance from origin
     * @param[in] theta Angle in radians
     * @param[in] color Line color value
     * @see ::drawLineOnImage For underlying C implementation
     */
    void drawHoughLine(float rho, float theta, uint8_t color) const noexcept;

    // Morphology
    /**
     * @brief Applies morphological erosion.
     * @param[out] out Destination image for eroded result
     * @param[in] kernel Structuring element defining erosion shape
     * @param[in] iterations Number of times to apply erosion
     * @see ::erode For underlying C implementation
     */
    void erode(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;
    /**
     * @brief Applies morphological dilation.
     * @param[out] out Destination image for dilated result
     * @param[in] kernel Structuring element defining dilation shape
     * @param[in] iterations Number of times to apply dilation
     * @see ::dilate For underlying C implementation
     */
    void dilate(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;
    /**
     * @brief Applies morphological opening (erode then dilate).
     * @param[out] out Destination image for opening result
     * @param[in] kernel Structuring element
     * @param[in] iterations Number of times to apply operation
     * @see ::opening For underlying C implementation
     */
    void opening(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;
    /**
     * @brief Applies morphological closing (dilate then erode).
     * @param[out] out Destination image for closing result
     * @param[in] kernel Structuring element
     * @param[in] iterations Number of times to apply operation
     * @see ::closing For underlying C implementation
     */
    void closing(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;

    // Advanced transforms
    /**
     * @brief Applies gamma (power-law) correction.
     * @param[out] out Output gamma-corrected image
     * @param[in] gamma Gamma exponent value
     * @see ::powerTransform For underlying C implementation
     */
    void powerTransform(Image &out, float gamma) const noexcept;
    /**
     * @brief Applies linear scale + offset followed by absolute value.
     * @param[out] out Output transformed image
     * @param[in] alpha Scale factor
     * @param[in] beta Offset value
     * @see ::convertScaleAbs For underlying C implementation
     */
    void convertScaleAbsTo(Image &out, float alpha, float beta) const noexcept;
    /**
     * @brief Applies piecewise intensity transform from LUT breakpoints.
     * @param[out] out Output transformed image
     * @param[in] breakpoints Input intensity breakpoints
     * @param[in] values Output intensity values for each segment
     * @see ::piecewiseTransform For underlying C implementation
     */
    void piecewiseTransform(Image &out,
                            const std::vector<uint8_t> &breakpoints,
                            const std::vector<uint8_t> &values) const noexcept;

    // histogram
    /**
     * @brief Computes histogram (256 bins) of this image.
     * @param[out] histogram Output vector filled with histogram bins
     * @return Status code from C layer
     * @see ::histForm For underlying C implementation
     */
    int histForm(std::vector<int> &histogram) const;

    /**
     * @brief Performs histogram equalization.
     * @param[out] out Output contrast-enhanced image
     * @return Status code from C layer
     * @see ::histEq For underlying C implementation
     */
    int histEq(Image &out) const;

    /**
     * @brief Performs histogram specification to a target histogram.
     * @param[out] out Output image with specified histogram
     * @param[in] targetHistogram Target distribution
     * @return Status code from C layer
     * @see ::histSpec For underlying C implementation
     */
    int histSpec(Image &out, const std::vector<int> &targetHistogram) const;

    /**
     * @brief Applies a square 2D convolution kernel.
     * @param[in] filter 2D kernel coefficients (row-major)
     * @param[out] out Output filtered image
     * @see ::filter2D For underlying C implementation
     */
    void filter2D(const std::vector<std::vector<float>> &filter, Image &out) const;

    /**
     * @brief Applies Difference-of-Gaussians filtering.
     * @param[out] outImg Output edge-detected image
     * @param[in] sigma1 First Gaussian standard deviation
     * @param[in] sigma2 Second Gaussian standard deviation
     * @see ::dogFilter For underlying C implementation
     */
    void dogFilter(Image &outImg, float sigma1, float sigma2) const;

    /**
     * @brief Applies Laplacian-of-Gaussian filtering.
     * @param[out] outImg Output edge-detected image
     * @param[in] sigma Gaussian standard deviation
     * @see ::logFilter For underlying C implementation
     */
    void logFilter(Image &outImg, float sigma) const;

    /**
     * @brief Splits RGB image into R, G and B band images.
     * @param[out] r Red channel output image
     * @param[out] g Green channel output image
     * @param[out] b Blue channel output image
     * @see ::rgbSplit For underlying C implementation
     */
    void rgbSplit(Image &r, Image &g, Image &b) const noexcept;

    /**
     * @brief Merges R, G and B band images into this image.
     * @param[in] r Red channel input image
     * @param[in] g Green channel input image
     * @param[in] b Blue channel input image
     * @see ::rgbMerge For underlying C implementation
     */
    void rgbMerge(const Image &r, const Image &g, const Image &b) noexcept;

    /**
     * @brief Applies separable convolution using raw C-array kernels.
     * @param[out] out Output filtered image
     * @param[in] sizeX Horizontal kernel size
     * @param[in] kernelX Horizontal kernel array
     * @param[in] sizeY Vertical kernel size
     * @param[in] kernelY Vertical kernel array
     * @param[in] delta Value added to filtered pixels
     * @see ::sepfilter2D For underlying C implementation
     */
    void filter2DSeparable(Image &out,
                           int sizeX,
                           float *kernelX,
                           int sizeY,
                           float *kernelY,
                           float delta) const;

    /**
     * @brief Applies separable convolution.
     * @param[out] out Output filtered image
     * @param[in] kernelX Horizontal kernel vector
     * @param[in] kernelY Vertical kernel vector
     * @param[in] delta Value added to filtered pixels
     * @see ::sepfilter2D For underlying C implementation
     */
    void sepFilter2D(Image &out,
                     const std::vector<float> &kernelX,
                     const std::vector<float> &kernelY,
                     float delta) const noexcept;

    /**
     * @brief Applies minimum filter.
     * @param[out] out Output filtered image
     * @param[in] kernelSize Filter kernel size
     * @see ::minFilter For underlying C implementation
     */
    void minFilter(Image &out, int kernelSize) const;

    /**
     * @brief Applies maximum filter.
     * @param[out] out Output filtered image
     * @param[in] kernelSize Filter kernel size
     * @see ::maxFilter For underlying C implementation
     */
    void maxFilter(Image &out, int kernelSize) const;

    /**
     * @brief Applies median filter.
     * @param[out] out Output filtered image
     * @param[in] kernelSize Filter kernel size
     * @see ::medianFilter For underlying C implementation
     */
    void medianFilter(Image &out, int kernelSize) const;

    // fundamentals
    /**
     * @brief Resizes image to requested dimensions.
     * @param[out] out Output resized image
     * @param[in] outWidth Target width in pixels
     * @param[in] outHeight Target height in pixels
     * @see ::resize For underlying C implementation
     */
    void resize(Image &out, int outWidth, int outHeight) const;

    /**
     * @brief Computes saturated pixel-wise addition.
     * @param[in] other Second input image
     * @param[out] out Output sum image
     * @see ::add For underlying C implementation
     */
    void add(const Image &other, Image &out) const;

    /**
     * @brief Computes RGB-distance map from a reference color.
     * @param[out] out Output distance map
     * @param[in] r Reference red channel value
     * @param[in] g Reference green channel value
     * @param[in] b Reference blue channel value
     * @see ::dist For underlying C implementation
     */
    void dist(Image &out, uint8_t r, uint8_t g, uint8_t b) const;

    /**
     * @brief Normalizes image intensity range in place.
     * @see ::normalize For underlying C implementation
     */
    void normalize();

    /**
     * @brief Converts pixel buffer to channel representation.
     * @see ::convertTo For underlying C implementation
     */
    void convertTo();

    /**
     * @brief Computes forward Fourier transform.
     * @param[out] out Output frequency-domain image with complex channels
     * @see ::fft For underlying C implementation
     */
    void fft(Image &out) const;

    /**
     * @brief Computes absolute value from complex channels.
     * @param[out] out Output magnitude image
     * @see ::_abs_ For underlying C implementation
     */
    void _abs_(Image &out) const;

    /**
     * @brief Computes phase from complex channels.
     * @param[out] out Output phase image in radians
     * @see ::_phase_ For underlying C implementation
     */
    void _phase_(Image &out) const;

    /**
     * @brief Applies natural logarithm in place.
     * @see ::_log_ For underlying C implementation
     */
    void _log_() const;

    /**
     * @brief Adds scalar value to image in place.
     * @param[in] value Scalar offset to add to all pixels
     * @see ::_add_ For underlying C implementation
     */
    void _add_(float value) const;

    /**
     * @brief Computes inverse Fourier transform.
     * @param[out] out Output spatial-domain image
     * @see ::ifft For underlying C implementation
     */
    void ifft(Image &out) const;

    /**
     * @brief Converts polar representation to complex channels.
     * @param[in] magnitude Magnitude image
     * @param[in] phase Phase image in radians
     * @see ::polarToCart For underlying C implementation
     */
    void polarToCart(const Image &magnitude, Image &phase) const;

    /**
     * @brief Multiplies two images element-wise.
     * @param[in] other Second input image
     * @param[out] out Output product image
     * @see ::multiply For underlying C implementation (if exists)
     */
    void multiply(const Image &other, Image &out) const;

    /**
     * @brief Moves DC component to center of spectrum.
     * @see ::fftshift For underlying C implementation
     */
    void fftshift();

    /**
     * @brief Builds frequency-domain mask in this image.
     * @param[in] type Filter type (lowpass, highpass, bandpass)
     * @param[in] cutoff1 Primary cutoff frequency in pixels
     * @param[in] cutoff2 Secondary cutoff frequency in pixels
     * @see ::getFilter For underlying C implementation
     */
    void createFrequencyMask(FrequencyFilterType type, float cutoff1, float cutoff2);

    /**
     * @brief Generates frequency filter coefficients (single-cutoff variant).
     * @param[in] type Filter type (lowpass, highpass)
     * @param[in] cutoff1 Cutoff frequency in pixels
     * @see ::getFilter For underlying C implementation
     */
    void getFilter(FrequencyFilterType type, float cutoff1);  // 2 args
    /**
     * @brief Generates frequency filter coefficients (dual-cutoff variant).
     * @param[in] type Filter type (bandpass, bandstop)
     * @param[in] cutoff1 Lower cutoff frequency in pixels
     * @param[in] cutoff2 Upper cutoff frequency in pixels
     * @see ::getFilter For underlying C implementation
     */
    void getFilter(FrequencyFilterType type, float cutoff1, float cutoff2);  // 3 args

    /**
     * @brief Applies frequency-domain mask/filter to this image.
     * @param[in] filterMask Filter mask image
     * @param[out] out Output filtered image
     * @see ::ffilter2D For underlying C implementation
     */
    void ffilter2D(const Image &filterMask, Image &out) const;

    /**
     * @brief Computes absolute difference between two images.
     * @param[in] other Second input image
     * @param[out] out Output difference image
     * @see ::difference For underlying C implementation
     */
    void difference(const Image &other, Image &out) const;

    /**
     * @brief Converts this image to a different format and stores it in the provided output image.
     * @param[out] out Reference to an Image object where the converted image will be stored
     * @param[in] code Conversion code defined in the ColorConversionCode enum
     * @see ::cvtColor For underlying C implementation
     */
    void cvtColor(Image &out, ColorConversionCode code) const;

    /**
     * @brief Computes element-wise bitwise AND.
     * @param[in] other Second input image
     * @param[out] out Output AND result image
     * @see ::_and_ For underlying C implementation
     */
    void bitwiseAnd(const Image &other, Image &out) const;
    /**
     * @brief Computes element-wise bitwise OR.
     * @param[in] other Second input image
     * @param[out] out Output OR result image
     * @see ::_or For underlying C implementation
     */
    void _or(const Image &other, Image &out) const;
    /**
     * @brief Computes element-wise bitwise AND.
     * @param[in] other Second input image
     * @param[out] out Output AND result image
     * @see ::_and_ For underlying C implementation
     */
    void _and_(const Image &other, Image &out) const;

    /**
     * @brief Computes element-wise bitwise XOR.
     * @param[in] other Second input image
     * @param[out] out Output XOR result image
     * @see ::_xor For underlying C implementation
     */
    void bitwiseXor(const Image &other, Image &out) const;
    /**
     * @brief Computes element-wise bitwise NOT.
     * @param[out] out Output NOT result image (inverted)
     * @see ::_not For underlying C implementation
     */
    void bitwiseNot(Image &out) const;

    /**
     * @brief Runs grayscale graph-cut GrabCut in ROI.
     * @param[out] maskImg Output segmentation mask.
     * @param[in] roi Region of interest.
     * @param[in] iterations Number of refinement iterations.
     * @return C-layer status code.
     */
    embeddip_status_t grabCut(Image &maskImg, Rectangle roi, int iterations) const;

    /**
     * @brief Runs simplified GrabCut in a rectangular ROI.
     * @param outImg Output segmentation image/mask.
     * @param roi Region of interest.
     * @param iterations Number of refinement iterations.
     */
    void grabCutLite(Image &outImg, Rectangle roi, int iterations) const;

    /**
     * @brief Thresholds image by hue range.
     * @param output Output binary mask image.
     * @param minHue Minimum hue.
     * @param maxHue Maximum hue.
     */
    void hueThreshold(Image &output, float minHue, float maxHue) const;

    /**
     * @brief Applies inclusive RGB range threshold.
     * @param mask Output binary mask.
     * @param lower Lower RGB bound.
     * @param upper Upper RGB bound.
     */
    void inRange(Image &mask, const uint8_t lower[3], const uint8_t upper[3]) const;

    /**
     * @brief Computes Gaussian-smoothed X and Y gradients.
     */
    void gaussianGradients(Image &outIx, Image &outIy, float sigma) const;

    /**
     * @brief Computes gradient magnitude into this image.
     */
    void gradientMagnitude(const Image &IxImg, const Image &IyImg);

    /**
     * @brief Computes gradient phase into this image.
     */
    void gradientPhase(const Image &IxImg, const Image &IyImg);

    /**
     * @brief Runs Canny edge detector.
     */
    void
    Canny(Image &outImg, double threshold1, double threshold2, int apertureSize, bool L2gradient);

    /**
     * @brief Labels connected components.
     */
    void connectedComponents(Image &outImg, uint32_t *labelCount = nullptr);

    /**
     * @brief Extracts one connected-component label as a mask.
     */
    void extractComponent(Image &objImg, int targetLabel);

  private:
    ::Image *image_;
};

}  // namespace embedDIP
