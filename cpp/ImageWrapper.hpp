// ImageWrapper.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <cmath>

extern "C"
{
#include "image.h"
#include "common.h"
#include "memory_manager.h"
#include "pixel.h"
#include "histogram.h"
#include "fft.h"
#include "color.h"
#include "filter.h"
}

namespace embedDIP
{

    class Image
    {
    public:
        // Constructors / destructor / move operations
        Image(ImageResolution resolution, ImageFormat format);
        Image(int width, int height, ImageFormat format);
        ~Image();
        Image(const Image &) = delete;
        Image &operator=(const Image &) = delete;
        Image(Image &&other) noexcept;
        Image &operator=(Image &&other) noexcept;

        // Raw access
        ::Image *raw() noexcept;
        ::Image *raw() const noexcept;
        inline void *pixels() const noexcept { return image_->pixels; }
        inline uint32_t size() const noexcept { return image_->size; }
        inline uint32_t width() const noexcept { return image_->width; }
        inline uint32_t height() const noexcept { return image_->height; }
        inline ImageFormat format() const noexcept { return image_->format; }
        inline ImageDepth depth() const noexcept { return image_->depth; }

        // Channel operations
        void createChals(uint8_t numChals) noexcept;
        bool isChalsEmpty() const noexcept;

        // Pixel operations
        void negative(Image &out) const noexcept;
        void grayscaleThresholdTo(Image &out, uint8_t threshold) const noexcept;
        uint8_t computeOtsuThreshold() const noexcept;
        void grayscaleOtsuTo(Image &out) const noexcept;
        void grayscaleThresholdLocalOtsuTo(Image &out, int blockSize) const noexcept;
        void grayscaleKMeansTo(Image &out, int k) const noexcept;
        void grayscaleRegionGrowingTo(Image &out, int seedX, int seedY, uint8_t tolerance) const noexcept;

        // Hough Transform
        std::vector<std::vector<int>> houghAccumulator(int numRho, int numTheta,
                                                       float rhoRes, float thetaRes) const;
        int extractHoughLines(const std::vector<std::vector<int>> &acc,
                              float rhoRes, float thetaRes,
                              int threshold, float rhoMax,
                              std::vector<HoughLine> &lines,
                              int maxLines = 100) const;

        // Drawing
        void drawLine(int x0, int y0, int x1, int y1, uint8_t color) const noexcept;
        void drawHoughLine(float rho, float theta, uint8_t color) const noexcept;

        // Morphology
        void getStructuringElement(Kernel &kernel, MorphShape shape, uint8_t size) const noexcept;
        void erode(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;
        void dilate(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;
        void opening(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;
        void closing(Image &out, const Kernel &kernel, uint8_t iterations) const noexcept;

        // Advanced transforms
        void powerTransformTo(Image &out, float gamma, float c) const noexcept;
        void convertScaleAbsTo(Image &out, float alpha, float beta) const noexcept;
        void piecewiseTransformTo(Image &out,
                                  const std::vector<uint8_t> &breakpoints,
                                  const std::vector<uint8_t> &values) const noexcept;

        // histogram
        int histFormTo(std::vector<int> &histogram) const;

        int histEqTo(Image &out) const;

        int histSpecTo(Image &out, const std::vector<int> &targetHistogram) const;

        void filter2DChannel(int ch_idx, const Filter2DContext &ctx, Image &out) const;

        void filter2DSeparable(Image &out, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta) const;

        void minFilter(Image &out, int kernelSize) const;

        void maxFilter(Image &out, int kernelSize) const;

        void medianFilter(Image &out, int kernelSize) const;

        // fundamentals
        void resizeTo(Image &out, int size) const;

        void addTo(const Image &other, Image &out) const;

        void distanceTo(Image &out, uint8_t r, uint8_t g, uint8_t b) const;

        void normalize();

        void convertTo();

        void fft(Image &out) const;

        void _abs(Image &out) const;

        void _phase(Image &out) const;

        void _log() const;

        void _add(float value) const;

        void ifft(Image &out) const;

        void polarToCart(const Image &magnitude, Image &phase) const;

        void multiplyWith(const Image &other, Image &out) const;

        void createFrequencyMask(FrequencyFilterType type, float cutoff1, float cutoff2);

        void fftshift();

        void ifftshift();

        void getFilter(FrequencyFilterType type, float cutoff1, float cutoff2 = 0.0f);

        void multiply(const Image &img2, Image &outImg) const;

        void cvtColor(Image &out, ColorConversionCode code) const;

    private:
        ::Image *image_;
    };

} // namespace embedDIP