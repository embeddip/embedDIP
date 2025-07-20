#include "image.h"
#include "pixel.h"
#include "assert.h"

/**
 * @brief Applies negative transformation to an image.
 *
 * This function computes the negative of a given image by subtracting each pixel value
 * from the maximum intensity value (typically 255 for 8-bit grayscale images).
 *
 * @param[in]  inImg   Pointer to the input image structure (source image).
 * @param[out] outImg  Pointer to the output image structure (resulting negative image).
 *
 * @note Assumes both images are of the same size and format, and `MAX_INTENSITY` is defined appropriately.
 */
void negative(const Image *inImg, Image *outImg)
{
    uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < inImg->size; ++i)
    {
        outData[i] = MAX_INTENSITY - imgData[i];
    }
}

/**
 * @brief Applies binary thresholding to a grayscale image.
 *
 * This function sets each pixel to 255 if its value is greater than or equal to the threshold,
 * and to 0 otherwise. The output image will be a binary image (black and white).
 *
 * @param[in]  inImg      Pointer to the input image structure (grayscale image).
 * @param[out] outImg     Pointer to the output image structure (thresholded image).
 * @param[in]  threshold  Threshold value (in range 0–255).
 *
 * @note Assumes both images are of the same size and format.
 */
void grayscaleThreshold(const Image *inImg, Image *outImg, uint8_t threshold)
{
    uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < inImg->size; ++i)
    {
        outData[i] = (imgData[i] >= threshold) ? 255 : 0;
    }
}

/**
 * @brief Computes Otsu's threshold for a grayscale image.
 *
 * @param[in] imgData  Pointer to grayscale pixel data.
 * @param[in] size     Total number of pixels in the image.
 * @return uint8_t     Computed Otsu threshold value.
 */
uint8_t computeOtsuThreshold(const uint8_t *imgData, int size)
{
    int hist[256] = {0};
    for (int i = 0; i < size; ++i)
        hist[imgData[i]]++;

    float sum = 0.0f;
    for (int t = 0; t < 256; ++t)
        sum += t * hist[t];

    float sumB = 0.0f;
    int wB = 0;
    int wF = 0;
    float maxVar = 0.0f;
    uint8_t threshold = 0;

    for (int t = 0; t < 256; ++t)
    {
        wB += hist[t]; // Weight Background
        if (wB == 0)
            continue;

        wF = size - wB; // Weight Foreground
        if (wF == 0)
            break;

        sumB += (float)(t * hist[t]);

        float mB = sumB / wB;         // Mean Background
        float mF = (sum - sumB) / wF; // Mean Foreground

        // Between Class Variance
        float varBetween = (float)wB * wF * (mB - mF) * (mB - mF);

        if (varBetween > maxVar)
        {
            maxVar = varBetween;
            threshold = (uint8_t)t;
        }
    }

    return threshold;
}

/**
 * @brief Applies Otsu's binary thresholding to a grayscale image.
 *
 * @param[in]  inImg   Pointer to input image (grayscale, 8-bit).
 * @param[out] outImg  Pointer to output binary image.
 */
void grayscaleOtsu(const Image *inImg, Image *outImg)
{
    const uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    uint8_t threshold = computeOtsuThreshold(imgData, inImg->size);

    for (int i = 0; i < inImg->size; ++i)
    {
        outData[i] = (imgData[i] >= threshold) ? 255 : 0;
    }
}

void grayscaleThresholdLocalOtsu(const Image *inImg, Image *outImg, int blockSize)
{
    const int width = inImg->width;
    const int height = inImg->height;
    const uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (int y = 0; y < height; y += blockSize)
    {
        for (int x = 0; x < width; x += blockSize)
        {
            int blockW = (x + blockSize > width) ? (width - x) : blockSize;
            int blockH = (y + blockSize > height) ? (height - y) : blockSize;

            // 1. Extract block into a temporary buffer
            uint8_t blockPixels[blockSize * blockSize];
            int count = 0;

            for (int j = 0; j < blockH; ++j)
            {
                for (int i = 0; i < blockW; ++i)
                {
                    blockPixels[count++] = imgData[(y + j) * width + (x + i)];
                }
            }

            // 2. Compute Otsu threshold for this block
            uint8_t threshold = computeOtsuThreshold(blockPixels, count);

            // 3. Apply threshold to each pixel in the block
            count = 0;
            for (int j = 0; j < blockH; ++j)
            {
                for (int i = 0; i < blockW; ++i)
                {
                    int idx = (y + j) * width + (x + i);
                    outData[idx] = (imgData[idx] >= threshold) ? 255 : 0;
                }
            }
        }
    }
}

#define MAX_ITER 10
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/**
 * @brief Segments a grayscale image using K-means clustering.
 *
 * This function applies K-means clustering to segment a grayscale image into `k` clusters.
 * Each pixel in the output image is assigned the cluster center value it belongs to.
 *
 * @param[in]  inImg   Pointer to input grayscale image.
 * @param[out] outImg  Pointer to output segmented image.
 * @param[in]  k       Number of clusters (e.g., 2 for foreground/background).
 */
void grayscaleKMeans(const Image *inImg, Image *outImg, int k)
{
    if (!inImg || !outImg || !inImg->pixels || !outImg->pixels || k <= 0)
    {
        printf("Invalid input\n");
        return;
    }

    int size = inImg->size;
    const uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    float *centers = (float *)memory_alloc(k * sizeof(float));
    int *labels = (int *)memory_alloc(size * sizeof(int));
    int *counts = (int *)memory_alloc(k * sizeof(int));
    float *sums = (float *)memory_alloc(k * sizeof(float));

    if (!centers || !labels || !counts || !sums)
    {
        printf("Memory allocation failed\n");
    }

    // Initialize cluster centers evenly spaced
    for (int i = 0; i < k; ++i)
        centers[i] = (255.0f / (k - 1)) * i;

    for (int iter = 0; iter < MAX_ITER; ++iter)
    {
        // Reset accumulators
        for (int j = 0; j < k; ++j)
        {
            counts[j] = 0;
            sums[j] = 0.0f;
        }

        // Assign labels
        for (int i = 0; i < size; ++i)
        {
            float minDist = 1e9f;
            int best = 0;

            for (int j = 0; j < k; ++j)
            {
                float dist = fabsf((float)imgData[i] - centers[j]);
                if (dist < minDist)
                {
                    minDist = dist;
                    best = j;
                }
            }

            // Safety clamp
            best = CLAMP(best, 0, k - 1);
            labels[i] = best;
            sums[best] += (float)imgData[i];
            counts[best]++;
        }

        // Update centers, reinitialize empty clusters
        for (int j = 0; j < k; ++j)
        {
            if (counts[j] > 0)
                centers[j] = sums[j] / counts[j];
            else
                centers[j] = (float)(rand() % 256); // reinitialize randomly
        }
    }

    // Final image: assign cluster center as output
    for (int i = 0; i < size; ++i)
    {
        int c = CLAMP(labels[i], 0, k - 1);
        outData[i] = (uint8_t)centers[c];
    }
}

#define THRESHOLD 10
#define STACK_SIZE 65536

typedef struct
{
    int x;
    int y;
} Point;

/**
 * @brief Segments a grayscale image using region growing.
 *
 * This function performs region growing from a seed pixel. It includes pixels
 * in the region if the absolute intensity difference from the region mean is below a threshold.
 *
 * @param[in]  inImg     Pointer to input grayscale image.
 * @param[out] outImg    Pointer to output binary image (0 for background, 255 for region).
 * @param[in]  seedX     X coordinate of the seed point.
 * @param[in]  seedY     Y coordinate of the seed point.
 * @param[in]  tolerance Intensity difference threshold for region inclusion.
 */
void grayscaleRegionGrowing(const Image *inImg, Image *outImg, int seedX, int seedY, uint8_t tolerance)
{
    const int width = inImg->width;
    const int height = inImg->height;
    const uint8_t *src = inImg->pixels;
    uint8_t *dst = outImg->pixels;
    memset(dst, 0, inImg->size); // Initialize output to background

    bool *visited = (bool *)memory_alloc(sizeof(bool) * inImg->size);
    if (!visited)
    {
        printf("Memory allocation failed\n");
        return;
    }
    memset(visited, 0, inImg->size);

    Point *stack = (Point *)memory_alloc(sizeof(Point) * STACK_SIZE);
    int top = 0;

    int seedIndex = seedY * width + seedX;
    uint8_t regionMean = src[seedIndex];

    // Start from the seed
    stack[top++] = (Point){seedX, seedY};
    visited[seedIndex] = true;
    dst[seedIndex] = 255;

    // 4-connected neighbors
    int dx[4] = {0, -1, 1, 0};
    int dy[4] = {-1, 0, 0, 1};

    while (top > 0)
    {
        Point p = stack[--top];
        int idx = p.y * width + p.x;

        for (int i = 0; i < 4; ++i)
        {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];
            int nidx = ny * width + nx;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[nidx])
            {
                uint8_t neighborValue = src[nidx];
                if (abs((int)neighborValue - (int)regionMean) <= tolerance)
                {
                    visited[nidx] = true;
                    dst[nidx] = 255;
                    stack[top++] = (Point){nx, ny};

                    if (top >= STACK_SIZE)
                    {
                        printf("Region stack overflow\n");
                        return;
                    }
                }
            }
        }
    }
}

/**
 * @brief Detects straight lines in a binary edge image using the Hough Transform.
 *
 * This function computes the Hough accumulator space for all edge pixels in the input image.
 * Each edge pixel votes for all lines passing through it, parameterized by (rho, theta).
 *
 * @param[in]  edgeImg     Pointer to the binary edge image (output of Sobel/Canny).
 * @param[out] accumulator 2D accumulator array for Hough space (rho x theta).
 * @param[in]  numRho      Number of bins in the rho dimension.
 * @param[in]  numTheta    Number of bins in the theta dimension.
 * @param[in]  rhoRes      Rho resolution (e.g., 1.0).
 * @param[in]  thetaRes    Theta resolution in radians (e.g., CV_PI / 180).
 */
void houghTransform(const Image *edgeImg, int **accumulator, int numRho, int numTheta,
                    float rhoRes, float thetaRes)
{
    int width = edgeImg->width;
    int height = edgeImg->height;
    const uint8_t *pixels = edgeImg->pixels;

    float diagLen = sqrtf(width * width + height * height);
    int rhoMax = (int)(diagLen / rhoRes);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (pixels[y * width + x] == 255) // edge pixel
            {
                for (int t = 0; t < numTheta; ++t)
                {
                    float theta = t * thetaRes;
                    float rho = x * cosf(theta) + y * sinf(theta);
                    int r = (int)((rho + rhoMax) / rhoRes);

                    if (r >= 0 && r < numRho)
                    {
                        accumulator[r][t]++;
                    }
                }
            }
        }
    }
}

int extractLines(int **accumulator, int numRho, int numTheta, float rhoRes, float thetaRes,
                 int threshold, float rhoMax, HoughLine *lines, int maxLines)
{
    int count = 0;
    for (int r = 0; r < numRho; ++r)
    {
        for (int t = 0; t < numTheta; ++t)
        {
            if (accumulator[r][t] >= threshold && count < maxLines)
            {
                lines[count].rho = r * rhoRes - rhoMax;
                lines[count].theta = t * thetaRes;
                lines[count].votes = accumulator[r][t];
                count++;
            }
        }
    }
    return count;
}

/**
 * @brief Draws a line on a grayscale image using Bresenham's algorithm.
 *
 * @param[in,out] img   Pointer to the grayscale image to modify.
 * @param[in]     x0    Starting x coordinate.
 * @param[in]     y0    Starting y coordinate.
 * @param[in]     x1    Ending x coordinate.
 * @param[in]     y1    Ending y coordinate.
 * @param[in]     color Grayscale intensity (0–255) to draw the line.
 */
void drawLine(Image *img, int x0, int y0, int x1, int y1, uint8_t color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1)
    {
        if (x0 >= 0 && x0 < img->width && y0 >= 0 && y0 < img->height)
        {
            ((uint8_t *)img->pixels)[y0 * img->width + x0] = color;
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void drawLineOnImage(Image *img, float rho, float theta, uint8_t color)
{
    float cosT = cosf(theta);
    float sinT = sinf(theta);

    float x0 = cosT * rho;
    float y0 = sinT * rho;

    int x1 = (int)(x0 + 1000 * (-sinT));
    int y1 = (int)(y0 + 1000 * (cosT));
    int x2 = (int)(x0 - 1000 * (-sinT));
    int y2 = (int)(y0 - 1000 * (cosT));

    drawLine(img, x1, y1, x2, y2, color); // your custom Bresenham-style line drawer
}

/**
 * @brief Identifies and labels connected components in a binary image.
 *
 * @param inImg Input image.
 * @param outImg Output image (labeled components).
 */
void connectedComponents(const Image *inImg, Image *outImg)
{
    int label = 1;
    uint8_t *inData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    Image *equivalences = (Image *)createImage(IMAGE_RES_WQVGA,
                                               IMAGE_FORMAT_GRAYSCALE);
    uint8_t *equvData = equivalences->pixels;

    for (int i = 0; i < inImg->size; i++)
        equvData[i] = 0x00;
    for (int i = 0; i < inImg->size; i++)
        outData[i] = 0x00;
    // First pass
    for (int y = 0; y < inImg->height; y++)
    {
        for (int x = 0; x < inImg->width; x++)
        {
            int index = y * inImg->width + x;
            if (inData[index] != 0)
            { // If pixel is part of an object
                int left = (x > 0) ? outData[index - 1] : 0;
                int up = (y > 0) ? outData[index - inImg->width] : 0;

                if (left && up)
                {                                             // Both neighbors are labeled
                    outData[index] = (left < up) ? left : up; // Assign the minimum label
                    if (left != up)
                    {
                        // Store the equivalence between labels
                        int minLabel = (left < up) ? left : up;
                        int maxLabel = (left > up) ? left : up;
                        equvData[maxLabel] = minLabel;
                    }
                }
                else if (left || up)
                {                                            // One neighbor is labeled
                    outData[index] = (left > 0) ? left : up; // Assign the labeled label
                }
                else
                {
                    outData[index] = label++; // Assign a new label
                }
            }
        }
    }

    // Second pass to resolve equivalences
    for (int i = 0; i < inImg->width * inImg->height; i++)
    {
        if (outData[i] > 0)
        {
            int root = outData[i];
            while (equvData[root] > 0)
            {
                root = equvData[root];
            }
            outData[i] = root;
        }
    }
}

void getStructuringElement(Kernel *kernel, MorphShape shape, uint8_t size)
{
    kernel->size = size;
    kernel->anchor = size / 2;
    kernel->data = malloc(size * size);
    memset(kernel->data, 0, size * size);

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            int idx = y * size + x;
            switch (shape)
            {
            case MORPH_RECT:
                kernel->data[idx] = 1;
                break;
            case MORPH_CROSS:
                if (x == size / 2 || y == size / 2)
                    kernel->data[idx] = 1;
                break;
            case MORPH_ELLIPSE:
            {
                float dx = x - size / 2;
                float dy = y - size / 2;
                float r = size / 2.0f;
                if ((dx * dx + dy * dy) <= r * r)
                    kernel->data[idx] = 1;
                break;
            }
            }
        }
    }
}

void erode(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations)
{
    int w = src->width, h = src->height;
    int kSize = kernel->size;
    int anchor = kernel->anchor;

    uint8_t *ping = memory_alloc(src->size); // temp buffer
    uint8_t *pong = memory_alloc(src->size); // working buffer

    memcpy(ping, src->pixels, src->size);

    for (uint8_t it = 0; it < iterations; ++it)
    {
        uint8_t *in = ping;
        uint8_t *out = (it == iterations - 1) ? dst->pixels : pong;

        for (int y = anchor; y < h - anchor; ++y)
        {
            for (int x = anchor; x < w - anchor; ++x)
            {
                int match = 1;
                for (int ky = 0; ky < kSize && match; ++ky)
                {
                    for (int kx = 0; kx < kSize; ++kx)
                    {
                        if (!kernel->data[ky * kSize + kx])
                            continue;
                        int ix = x + kx - anchor;
                        int iy = y + ky - anchor;
                        if (in[iy * w + ix] == 0)
                        {
                            match = 0;
                            break;
                        }
                    }
                }
                out[y * w + x] = match ? 255 : 0;
            }
        }

        // Swap ping/pong for next iteration
        if (it < iterations - 1)
        {
            uint8_t *tmp = ping;
            ping = pong;
            pong = tmp;
        }
    }
}

void dilate(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations)
{
    int w = src->width, h = src->height;
    int kSize = kernel->size;
    int anchor = kernel->anchor;

    uint8_t *ping = memory_alloc(src->size); // temp buffer
    uint8_t *pong = memory_alloc(src->size); // working buffer

    memcpy(ping, src->pixels, src->size);

    for (uint8_t it = 0; it < iterations; ++it)
    {
        uint8_t *in = ping;
        uint8_t *out = (it == iterations - 1) ? dst->pixels : pong;

        for (int y = anchor; y < h - anchor; ++y)
        {
            for (int x = anchor; x < w - anchor; ++x)
            {
                int match = 0;
                for (int ky = 0; ky < kSize && !match; ++ky)
                {
                    for (int kx = 0; kx < kSize; ++kx)
                    {
                        if (!kernel->data[ky * kSize + kx])
                            continue;
                        int ix = x + kx - anchor;
                        int iy = y + ky - anchor;
                        if (in[iy * w + ix] != 0)
                        {
                            match = 1;
                            break;
                        }
                    }
                }
                out[y * w + x] = match ? 255 : 0;
            }
        }

        // Swap ping/pong for next iteration
        if (it < iterations - 1)
        {
            uint8_t *tmp = ping;
            ping = pong;
            pong = tmp;
        }
    }
}

void opening(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations)
{
    Image *temp = createImage(IMAGE_RES_WQVGA, IMAGE_FORMAT_GRAYSCALE);

    erode(inImg, temp, kernel, iterations);
    dilate(temp, outImg, kernel, iterations);
}

void closing(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations)
{
    Image *temp = createImage(IMAGE_RES_WQVGA, IMAGE_FORMAT_GRAYSCALE);

    dilate(inImg, temp, kernel, iterations);
    erode(temp, outImg, kernel, iterations);
}

#include <math.h>

/**
 * @brief Applies a power-law (gamma) transformation and stores the result in float channels.
 *
 * This function normalizes the pixel data to [0, 1], applies gamma correction,
 * and stores the result in `outImg->chals`. It ensures high-precision results
 * for further processing (e.g., Fourier or filtering).
 *
 * @param[in]  inImg   Pointer to the input image (supports GRAYSCALE or RGB).
 * @param[out] outImg  Pointer to the output image. Allocated `chals` field will be filled.
 * @param[in]  gamma   Gamma value (e.g., <1 brightens, >1 darkens).
 * @param[in]  c       Scaling constant (typically 1.0).
 *
 * @note Input must have 8-bit depth. Output must have `chals` allocated.
 */
void powerTransform(const Image *inImg, Image *outImg, float gamma, float c)
{
    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);
        uint8_t *imgData = (uint8_t *)inImg->pixels;

        for (uint32_t i = 0; i < inImg->width * inImg->height * inImg->depth; ++i)
        {
            float norm = imgData[i] / 255.0f;
            outImg->chals->ch[0][i] = c * powf(norm, gamma);
        }
    }
    else
    {
        float *imgData = (uint8_t *)inImg->chals->ch[0];

        for (uint32_t i = 0; i < inImg->width * inImg->height * inImg->depth; ++i)
        {
            float norm = imgData[i] / 255.0f;
            outImg->chals->ch[0][i] = c * powf(norm, gamma);
        }
    }
}

/**
 * @brief Applies linear scaling and bias followed by absolute value and clamps to 8-bit.
 *
 * This function simulates OpenCV's convertScaleAbs: dst = saturate_cast<uchar>(abs(alpha * src + beta))
 *
 * @param[in]  inImg   Pointer to the input image (supports GRAYSCALE or RGB, 8-bit).
 * @param[out] outImg  Pointer to the output image. Output must have pixels allocated.
 * @param[in]  alpha   Gain factor.
 * @param[in]  beta    Bias added after scaling.
 */
void convertScaleAbs(const Image *inImg, Image *outImg, float alpha, float beta)
{

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);

        uint8_t *src = (uint8_t *)inImg->pixels;

        for (uint32_t i = 0; i < inImg->width * inImg->height; ++i)
        {
            outImg->chals->ch[0][i] = alpha * (float)src[i] + beta;
        }
    }
    else
    {
        float *src = (float *)inImg->chals->ch[0];

        for (uint32_t i = 0; i < inImg->size; ++i)
        {
            outImg->chals->ch[0][i] = alpha * src[i] + beta;
        }
    }
}

/**
 * @brief Apply a piecewise linear transformation to an image.
 *
 * This function maps each pixel's intensity based on user-defined breakpoints
 * and corresponding output values, enabling custom contrast adjustment.
 * Assumes input is 8-bit grayscale and output `chals` is allocated.
 *
 * @param[in]  inImg        Input grayscale image.
 * @param[out] outImg       Output image after transformation.
 * @param[in]  breakpoints  Intensity breakpoints (must be sorted, in [0, 255]).
 * @param[in]  values       Corresponding output values (in [0, 1] or desired float range).
 * @param[in]  numPoints    Number of breakpoints (must be ≥2 and match values).
 */
void piecewiseTransform(const Image *inImg, Image *outImg,
                        const uint8_t *breakpoints, const uint8_t *values,
                        int numPoints)
{
    if (inImg->depth != 1)
    {
        // Only GRAYSCALE is supported
        return;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);
    }

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    uint8_t *imgData = (uint8_t *)inImg->pixels;
    float *outData = outImg->chals->ch[0];
    int totalPixels = inImg->width * inImg->height;

    for (int i = 0; i < totalPixels; ++i)
    {
        uint8_t pixel = imgData[i];

        // Handle out-of-range pixels (before first or after last breakpoint)
        if (pixel <= breakpoints[0])
        {
            outData[i] = values[0] / 255.0f;
        }
        else if (pixel >= breakpoints[numPoints - 1])
        {
            outData[i] = values[numPoints - 1] / 255.0f;
        }
        else
        {
            // Find the segment [breakpoints[j], breakpoints[j+1]] where pixel falls
            for (int j = 0; j < numPoints - 1; ++j)
            {
                if (pixel >= breakpoints[j] && pixel <= breakpoints[j + 1])
                {
                    float x0 = breakpoints[j];
                    float x1 = breakpoints[j + 1];
                    float y0 = values[j] / 255.0f;
                    float y1 = values[j + 1] / 255.0f;

                    // Linear interpolation
                    outData[i] = y0 + ((pixel - x0) * (y1 - y0)) / (x1 - x0);
                    break;
                }
            }
        }
    }
}
