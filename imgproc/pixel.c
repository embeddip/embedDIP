#include "pixel.h"
#include "assert.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif
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
uint8_t OtsuThreshold(const uint8_t *imgData, int size)
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

    uint8_t threshold = OtsuThreshold(imgData, inImg->size);

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
            uint8_t threshold = OtsuThreshold(blockPixels, count);

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

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    int size = inImg->size;
    const uint8_t *imgData = inImg->pixels;
    float *outData = outImg->chals->ch[0];

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
        outData[i] = (float)centers[c];
    }

    outImg->log = IMAGE_DATA_CH0;
}

/**
 * @brief Segments an HSI image using K-means clustering.
 *
 * This function applies K-means clustering to an HSI image by treating each pixel
 * as a 3D vector (Hue, Saturation, Intensity). It assigns each pixel to a cluster
 * and sets the output to the corresponding cluster center color.
 *
 * @param[in]  inImg   Pointer to input HSI image.
 * @param[out] outImg  Pointer to output segmented HSI image.
 * @param[in]  k       Number of clusters.
 */
void colorKMeans(const Image *inImg, Image *outImg, int k)
{
    if (!inImg || !outImg || !inImg->pixels || !outImg->pixels || k <= 0)
    {
        printf("Invalid input\n");
        return;
    }

    assert(inImg->format == IMAGE_FORMAT_HSI);
    assert(outImg->format == IMAGE_FORMAT_HSI);

    int size = inImg->size;
    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    typedef struct
    {
        float h, s, i;
    } hsi_t;

    hsi_t *centers = (hsi_t *)memory_alloc(k * sizeof(hsi_t));
    int *labels = (int *)memory_alloc(size * sizeof(int));
    int *counts = (int *)memory_alloc(k * sizeof(int));
    hsi_t *sums = (hsi_t *)memory_alloc(k * sizeof(hsi_t));

    if (!centers || !labels || !counts || !sums)
    {
        printf("Memory allocation failed\n");
        return;
    }

    // Initialize cluster centers evenly spaced in I channel
    for (int i = 0; i < k; ++i)
    {
        centers[i].h = (float)(rand() % 256);
        centers[i].s = (float)(rand() % 256);
        centers[i].i = (255.0f / (k - 1)) * i;
    }

    for (int iter = 0; iter < MAX_ITER; ++iter)
    {
        // Reset sums and counts
        for (int j = 0; j < k; ++j)
        {
            counts[j] = 0;
            sums[j].h = sums[j].s = sums[j].i = 0.0f;
        }

        // Assignment step
        for (int i = 0; i < size; ++i)
        {
            int idx = i * 3;
            float h = in[idx];
            float s = in[idx + 1];
            float v = in[idx + 2];

            float minDist = 1e9f;
            int best = 0;

            for (int j = 0; j < k; ++j)
            {
                float dh = fabsf(h - centers[j].h);
                if (dh > 128)
                    dh = 256 - dh; // wraparound hue distance
                float ds = s - centers[j].s;
                float di = v - centers[j].i;

                float dist = dh * dh + ds * ds + di * di;

                if (dist < minDist)
                {
                    minDist = dist;
                    best = j;
                }
            }

            labels[i] = best;
            sums[best].h += h;
            sums[best].s += s;
            sums[best].i += v;
            counts[best]++;
        }

        // Update step
        for (int j = 0; j < k; ++j)
        {
            if (counts[j] > 0)
            {
                centers[j].h = sums[j].h / counts[j];
                centers[j].s = sums[j].s / counts[j];
                centers[j].i = sums[j].i / counts[j];
            }
            else
            {
                centers[j].h = (float)(rand() % 256);
                centers[j].s = (float)(rand() % 256);
                centers[j].i = (float)(rand() % 256);
            }
        }
    }

    // Final assignment
    for (int i = 0; i < size; ++i)
    {
        int idx = i * 3;
        int c = CLAMP(labels[i], 0, k - 1);

        out[idx + 0] = (uint8_t)centers[c].h;
        out[idx + 1] = (uint8_t)centers[c].s;
        out[idx + 2] = (uint8_t)centers[c].i;
    }

    memory_free(centers);
    memory_free(labels);
    memory_free(counts);
    memory_free(sums);
}

// New idea. Will check
/*
void grayscaleKMeans(const Image *inImg, Image *outImg, int k)
{
    if (!inImg || !outImg || !inImg->pixels || !outImg->pixels || k <= 0)
    {
        printf("Invalid input\n");
        return;
    }

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    int N = inImg->size;
    const uint8_t *X = inImg->pixels;
    float *output = outImg->chals->ch[0];

    // Step 1: Declare cluster structures
    float *M = (float *)memory_alloc(k * sizeof(float));   // Cluster means M_1 to M_K
    float *S = (float *)memory_alloc(k * sizeof(float));   // Cluster sums S_k
    int *T = (int *)memory_alloc(k * sizeof(int));         // Cluster sample counts T_k
    int *C = (int *)memory_alloc(N * sizeof(int));         // Cluster labels for each sample

    if (!M || !S || !T || !C)
    {
        printf("Memory allocation failed\n");
        return;
    }

    // Step 2: Randomly select K initial means from data
    for (int i = 0; i < k; ++i)
    {
        int r = rand() % N;
        M[i] = (float)X[r];
    }

    // Step 3: Repeat until convergence (we use MAX_ITER as approximation)
    for (int iter = 0; iter < MAX_ITER; ++iter)
    {
        // Reset sums and counts
        for (int j = 0; j < k; ++j)
        {
            S[j] = 0.0f;
            T[j] = 0;
        }

        // Step 4: For each sample, assign to nearest cluster
        for (int i = 0; i < N; ++i)
        {
            float minDist = 1e9f;
            int best_k = 0;

            for (int j = 0; j < k; ++j)
            {
                float dist = fabsf((float)X[i] - M[j]);
                if (dist < minDist)
                {
                    minDist = dist;
                    best_k = j;
                }
            }

            // Step 5: Update cluster label
            C[i] = best_k;

            // Step 6: Update cluster sum and count
            S[best_k] += (float)X[i];
            T[best_k] += 1;
        }

        // Step 7: Recompute the cluster centroids
        for (int j = 0; j < k; ++j)
        {
            if (T[j] > 0)
                M[j] = S[j] / T[j];
            else
                M[j] = (float)(rand() % 256); // reinitialize if empty
        }
    }

    // Step 8: Output assignment (each pixel gets its centroid value)
    for (int i = 0; i < N; ++i)
    {
        int cluster = CLAMP(C[i], 0, k - 1);
        output[i] = M[cluster];
    }
}
*/

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
 * @brief Performs region growing on an HSI image using color similarity.
 *
 * Grows a region from a seed point by comparing HSI vectors using Euclidean distance.
 * Pixels within a given threshold are included in the region.
 *
 * @param[in]  inImg      Pointer to input HSI image (IMAGE_FORMAT_HSI).
 * @param[out] outMask    Pointer to output binary image (0 = background, 255 = region).
 * @param[in]  seedX      X coordinate of seed point.
 * @param[in]  seedY      Y coordinate of seed point.
 * @param[in]  tolerance  Threshold on HSI distance (e.g., 0.1–0.4 typical).
 */
void colorRegionGrowing(const Image *inImg, Image *outImg, int seedX, int seedY, float tolerance)
{
    assert(inImg && outImg);
    assert(inImg->format == IMAGE_FORMAT_HSI);
    assert(outImg->format == IMAGE_FORMAT_HSI);
    assert(inImg->size == outImg->size);

    const int width = inImg->width;
    const int height = inImg->height;
    const uint8_t *src = (const uint8_t *)inImg->pixels;
    uint8_t *dst = (uint8_t *)outImg->pixels;

    memset(dst, 0, inImg->size * 3); // Set all HSI values in output to 0 (black)

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
    float h0 = src[seedIndex * 3] / 255.0f;
    float s0 = src[seedIndex * 3 + 1] / 255.0f;
    float i0 = src[seedIndex * 3 + 2] / 255.0f;

    stack[top++] = (Point){seedX, seedY};
    visited[seedIndex] = true;

    // Copy seed pixel to output
    dst[seedIndex * 3] = src[seedIndex * 3];
    dst[seedIndex * 3 + 1] = src[seedIndex * 3 + 1];
    dst[seedIndex * 3 + 2] = src[seedIndex * 3 + 2];

    const int dx[4] = {0, -1, 1, 0};
    const int dy[4] = {-1, 0, 0, 1};

    while (top > 0)
    {
        Point p = stack[--top];

        for (int d = 0; d < 4; ++d)
        {
            int nx = p.x + dx[d];
            int ny = p.y + dy[d];
            int nidx = ny * width + nx;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[nidx])
            {
                float h = src[nidx * 3] / 255.0f;
                float s = src[nidx * 3 + 1] / 255.0f;
                float ii = src[nidx * 3 + 2] / 255.0f;

                // Hue distance with wraparound
                float dh = fminf(fabsf(h - h0), 1.0f - fabsf(h - h0));
                float ds = s - s0;
                float di = ii - i0;

                float dist = sqrtf(dh * dh + ds * ds + di * di);

                if (dist <= tolerance)
                {
                    visited[nidx] = true;

                    // Copy pixel to output
                    dst[nidx * 3] = src[nidx * 3];
                    dst[nidx * 3 + 1] = src[nidx * 3 + 1];
                    dst[nidx * 3 + 2] = src[nidx * 3 + 2];

                    stack[top++] = (Point){nx, ny};

                    if (top >= STACK_SIZE)
                    {
                        printf("Region stack overflow\n");
                        memory_free(visited);
                        memory_free(stack);
                        return;
                    }
                }
            }
        }
    }

    memory_free(visited);
    memory_free(stack);
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
 *
 * @note Input must have 8-bit depth. Output must have `chals` allocated.
 */
void powerTransform(const Image *inImg, Image *outImg, float gamma)
{
    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        createChals((Image *)inImg, inImg->depth);
        uint8_t *imgData = (uint8_t *)inImg->pixels;

        for (uint32_t i = 0; i < inImg->width * inImg->height * inImg->depth; ++i)
        {
            float norm = imgData[i] / 255.0f;
            outImg->chals->ch[0][i] = powf(norm, gamma);
        }
    }
    else
    {
        float *imgData = inImg->chals->ch[0];

        for (uint32_t i = 0; i < inImg->width * inImg->height * inImg->depth; ++i)
        {
            float norm = imgData[i] / 255.0f;
            outImg->chals->ch[0][i] = powf(norm, gamma);
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

void _and(const Image *img1, const Image *img2, Image *outImg)
{
    if (!img1 || !img2 || !outImg ||
        img1->width != img2->width || img1->height != img2->height ||
        img1->log != IMAGE_DATA_PIXELS || img2->log != IMAGE_DATA_PIXELS)
        return;

    int size = img1->width * img1->height;

    if (outImg->pixels == NULL)
    {
        outImg->pixels = memory_alloc(size * sizeof(uint8_t));
        outImg->log = IMAGE_DATA_PIXELS;
    }

    const uint8_t *data1 = img1->pixels;
    const uint8_t *data2 = img2->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < size; ++i)
        outData[i] = data1[i] & data2[i];
}

void _or(const Image *img1, const Image *img2, Image *outImg)
{
    if (!img1 || !img2 || !outImg ||
        img1->width != img2->width || img1->height != img2->height ||
        img1->log != IMAGE_DATA_PIXELS || img2->log != IMAGE_DATA_PIXELS)
        return;

    int size = img1->width * img1->height;

    if (outImg->pixels == NULL)
    {
        outImg->pixels = memory_alloc(size * sizeof(uint8_t));
        outImg->log = IMAGE_DATA_PIXELS;
    }

    const uint8_t *data1 = img1->pixels;
    const uint8_t *data2 = img2->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < size; ++i)
        outData[i] = data1[i] | data2[i];
}

void _xor(const Image *img1, const Image *img2, Image *outImg)
{
    if (!img1 || !img2 || !outImg ||
        img1->width != img2->width || img1->height != img2->height ||
        img1->log != IMAGE_DATA_PIXELS || img2->log != IMAGE_DATA_PIXELS)
        return;

    int size = img1->width * img1->height;

    if (outImg->pixels == NULL)
    {
        outImg->pixels = memory_alloc(size * sizeof(uint8_t));
        outImg->log = IMAGE_DATA_PIXELS;
    }

    const uint8_t *data1 = img1->pixels;
    const uint8_t *data2 = img2->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < size; ++i)
        outData[i] = data1[i] ^ data2[i];
}

void _not(const Image *inImg, Image *outImg)
{
    if (!inImg || !outImg || inImg->log != IMAGE_DATA_PIXELS)
        return;

    int size = inImg->width * inImg->height;

    if (outImg->pixels == NULL)
    {
        outImg->pixels = memory_alloc(size * sizeof(uint8_t));
        outImg->log = IMAGE_DATA_PIXELS;
    }

    const uint8_t *data = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < size; ++i)
        outData[i] = ~data[i];
}

/**
 * @brief Performs a simplified GrabCut-inspired segmentation on a grayscale image.
 *
 * It refines a binary mask using iterative classification based on foreground/background means.
 *
 * @param[in]  inImg     Input grayscale image.
 * @param[in,out] mask   Trimap mask: 0 = background, 1 = probable, 2 = foreground.
 * @param[in]  iterations Number of refinement iterations.
 */
void grabCutLite_working(Image *inImg, Image *maskImg, int iterations)
{
    const int size = inImg->width * inImg->height;
    const uint8_t *src = inImg->pixels;
    uint8_t *mask = (uint8_t *)maskImg->pixels;

    for (int iter = 0; iter < iterations; ++iter)
    {
        uint32_t fgSum = 0, fgCount = 0;
        uint32_t bgSum = 0, bgCount = 0;

        // Step 1: Compute foreground and background means
        for (int i = 0; i < size; ++i)
        {
            if (mask[i] == 2)
            {
                fgSum += src[i];
                fgCount++;
            }
            else if (mask[i] == 0)
            {
                bgSum += src[i];
                bgCount++;
            }
        }

        // Fallback if no foreground was found (bootstrap)
        if (fgCount == 0)
        {
            for (int i = 0; i < size; ++i)
            {
                if (mask[i] == 1)
                {
                    fgSum += src[i];
                    fgCount++;
                }
            }
        }

        if (fgCount == 0 || bgCount == 0)
            break; // Not enough info to proceed

        uint8_t fgMean = fgSum / fgCount;
        uint8_t bgMean = bgSum / bgCount;

        // Debug
        // printf("Iter %d: fgMean=%d, bgMean=%d\n", iter, fgMean, bgMean);

        // Step 2: Update probable region
        for (int i = 0; i < size; ++i)
        {
            if (mask[i] == 1)
            {
                int distFg = abs((int)src[i] - (int)fgMean);
                int distBg = abs((int)src[i] - (int)bgMean);

                // Reclassify as closer to fg or bg
                if (distFg < distBg)
                    mask[i] = 2; // Becomes foreground
                else
                    mask[i] = 0; // Becomes background
            }
        }
    }
}

void grabCutLitesd(const Image *inImg, uint8_t *mask, int iterations)
{
    const int size = inImg->width * inImg->height;
    const uint8_t *src = inImg->pixels;

    for (int iter = 0; iter < iterations; ++iter)
    {
        uint32_t fgSum = 0, fgCount = 0;
        uint32_t bgSum = 0, bgCount = 0;

        // Step 1: Compute foreground and background means
        for (int i = 0; i < size; ++i)
        {
            if (mask[i] == 2)
            {
                fgSum += src[i];
                fgCount++;
            }
            else if (mask[i] == 0)
            {
                bgSum += src[i];
                bgCount++;
            }
        }

        // Fallback if no foreground was found (bootstrap)
        if (fgCount == 0)
        {
            for (int i = 0; i < size; ++i)
            {
                if (mask[i] == 1)
                {
                    fgSum += src[i];
                    fgCount++;
                }
            }
        }

        if (fgCount == 0 || bgCount == 0)
            break; // Not enough info to proceed

        uint8_t fgMean = fgSum / fgCount;
        uint8_t bgMean = bgSum / bgCount;

        // Debug
        // printf("Iter %d: fgMean=%d, bgMean=%d\n", iter, fgMean, bgMean);

        // Step 2: Update probable region
        for (int i = 0; i < size; ++i)
        {
            if (mask[i] == 1)
            {
                int distFg = abs((int)src[i] - (int)fgMean);
                int distBg = abs((int)src[i] - (int)bgMean);

                // Reclassify as closer to fg or bg
                if (distFg < distBg)
                    mask[i] = 2; // Becomes foreground
                else
                    mask[i] = 0; // Becomes background
            }
        }
    }
}

/**
 * @brief Performs a simplified GrabCut-inspired segmentation on a grayscale image using a rectangular ROI.
 *
 * All pixels inside the ROI are considered probable foreground and refined over several iterations.
 * Pixels outside the ROI are background.
 *
 * @param[in]  inImg      Input grayscale image.
 * @param[out] outImg     Output binary image (0 = background, 255 = foreground).
 * @param[in]  roi        Rectangular region of interest.
 * @param[in]  iterations Number of refinement iterations.
 */
void grabCutLite(Image *inImg, Image *outImg, Rect roi, int iterations)
{

    const int width = inImg->width;
    const int height = inImg->height;
    const int size = width * height;
    const uint8_t *src = inImg->pixels;
    uint8_t *dst = outImg->pixels;

    // Temporary mask: 0 = background, 1 = probable, 2 = foreground
    uint8_t *mask = (uint8_t *)memory_alloc(size);
    if (!mask)
    {
        printf("Memory allocation failed\n");
        return;
    }

    // Initialize mask using ROI
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = y * width + x;
            if (x >= roi.x && x < (roi.x + roi.width) &&
                y >= roi.y && y < (roi.y + roi.height))
            {
                mask[idx] = 1; // Probable
            }
            else
            {
                mask[idx] = 0; // Background
            }
        }
    }

    // Iterative refinement
    for (int iter = 0; iter < iterations; ++iter)
    {
        uint32_t fgSum = 0, fgCount = 0;
        uint32_t bgSum = 0, bgCount = 0;

        // Compute means
        for (int i = 0; i < size; ++i)
        {
            if (mask[i] == 2)
            {
                fgSum += src[i];
                fgCount++;
            }
            else if (mask[i] == 0)
            {
                bgSum += src[i];
                bgCount++;
            }
        }

        // Bootstrap: if no fg yet, use probable
        if (fgCount == 0)
        {
            for (int i = 0; i < size; ++i)
            {
                if (mask[i] == 1)
                {
                    fgSum += src[i];
                    fgCount++;
                }
            }
        }

        if (fgCount == 0 || bgCount == 0)
            break;

        uint8_t fgMean = fgSum / fgCount;
        uint8_t bgMean = bgSum / bgCount;

        // Reclassify probable pixels
        for (int i = 0; i < size; ++i)
        {
            if (mask[i] == 1)
            {
                int dFg = abs((int)src[i] - (int)fgMean);
                int dBg = abs((int)src[i] - (int)bgMean);
                mask[i] = (dFg < dBg) ? 2 : 0;
            }
        }
    }

    // Final binary mask output
    for (int i = 0; i < size; ++i)
    {
        dst[i] = (mask[i] == 2) ? 255 : 0;
    }

    memory_free(mask);
    outImg->log = IMAGE_DATA_PIXELS;
}

#define FOREGROUND 255
#define BACKGROUND 0
#define GMM_COMPONENTS 2
#define MAX_ITER 5

typedef struct
{
    float weight;
    float mean;
    float variance;
} GMMComponent;

static float gaussian_prob(float x, float mean, float var)
{
    float diff = x - mean;
    return (1.0f / sqrtf(2.0f * M_PI * var)) * expf(-(diff * diff) / (2.0f * var));
}

void grabCutGrayscaleRealistic(const Image *inImg, Image *outMask, Rect roi, int max_iter)
{
    if (!inImg || !outMask || !inImg->pixels || inImg->format != IMAGE_FORMAT_GRAYSCALE)
        return;

    int width = inImg->width;
    int height = inImg->height;
    int size = width * height;
    const uint8_t *src = (const uint8_t *)inImg->pixels;
    uint8_t *mask = (uint8_t *)outMask->pixels;

    // Allocate component responsibilities
    uint8_t *labels = (uint8_t *)memory_alloc(size * sizeof(uint8_t)); // 0=BG, 1=FG
    float(*fg_resp)[GMM_COMPONENTS] = (float(*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));
    float(*bg_resp)[GMM_COMPONENTS] = (float(*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));

    GMMComponent fg_gmm[GMM_COMPONENTS];
    GMMComponent bg_gmm[GMM_COMPONENTS];

    // Step 1: Initialize mask from ROI
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = y * width + x;
            if (x >= roi.x && x < roi.x + roi.width && y >= roi.y && y < roi.y + roi.height)
            {
                mask[idx] = FOREGROUND;
                labels[idx] = 1;
            }
            else
            {
                mask[idx] = BACKGROUND;
                labels[idx] = 0;
            }
        }
    }

    // Step 2: Initialize GMMs with 2 components
    for (int i = 0; i < GMM_COMPONENTS; ++i)
    {
        fg_gmm[i].mean = 50.0f + 100 * i;
        fg_gmm[i].variance = 500.0f;
        fg_gmm[i].weight = 0.5f;

        bg_gmm[i].mean = 50.0f + 100 * i;
        bg_gmm[i].variance = 500.0f;
        bg_gmm[i].weight = 0.5f;
    }

    // Step 3: EM Iterations
    for (int iter = 0; iter < max_iter; ++iter)
    {
        // E-Step: compute responsibilities
        for (int i = 0; i < size; ++i)
        {
            float x = (float)src[i];
            float total_fg = 0.0f, total_bg = 0.0f;

            // Foreground responsibilities
            for (int c = 0; c < GMM_COMPONENTS; ++c)
            {
                fg_resp[i][c] = fg_gmm[c].weight * gaussian_prob(x, fg_gmm[c].mean, fg_gmm[c].variance);
                total_fg += fg_resp[i][c];
            }
            for (int c = 0; c < GMM_COMPONENTS; ++c)
                fg_resp[i][c] /= (total_fg + 1e-6f);

            // Background responsibilities
            for (int c = 0; c < GMM_COMPONENTS; ++c)
            {
                bg_resp[i][c] = bg_gmm[c].weight * gaussian_prob(x, bg_gmm[c].mean, bg_gmm[c].variance);
                total_bg += bg_resp[i][c];
            }
            for (int c = 0; c < GMM_COMPONENTS; ++c)
                bg_resp[i][c] /= (total_bg + 1e-6f);
        }

        // M-Step: update GMM parameters
        for (int c = 0; c < GMM_COMPONENTS; ++c)
        {
            // FG
            float w_sum = 0.0f, x_sum = 0.0f, x2_sum = 0.0f;
            for (int i = 0; i < size; ++i)
            {
                if (labels[i] == 1)
                {
                    float r = fg_resp[i][c];
                    float x = (float)src[i];
                    w_sum += r;
                    x_sum += r * x;
                    x2_sum += r * x * x;
                }
            }
            if (w_sum > 1e-6f)
            {
                fg_gmm[c].weight = w_sum;
                fg_gmm[c].mean = x_sum / w_sum;
                fg_gmm[c].variance = fmaxf((x2_sum / w_sum) - fg_gmm[c].mean * fg_gmm[c].mean, 10.0f);
            }

            // BG
            w_sum = x_sum = x2_sum = 0.0f;
            for (int i = 0; i < size; ++i)
            {
                if (labels[i] == 0)
                {
                    float r = bg_resp[i][c];
                    float x = (float)src[i];
                    w_sum += r;
                    x_sum += r * x;
                    x2_sum += r * x * x;
                }
            }
            if (w_sum > 1e-6f)
            {
                bg_gmm[c].weight = w_sum;
                bg_gmm[c].mean = x_sum / w_sum;
                bg_gmm[c].variance = fmaxf((x2_sum / w_sum) - bg_gmm[c].mean * bg_gmm[c].mean, 10.0f);
            }
        }

        // Normalize GMM weights
        float fg_total = 0.0f, bg_total = 0.0f;
        for (int c = 0; c < GMM_COMPONENTS; ++c)
        {
            fg_total += fg_gmm[c].weight;
            bg_total += bg_gmm[c].weight;
        }
        for (int c = 0; c < GMM_COMPONENTS; ++c)
        {
            fg_gmm[c].weight /= fg_total;
            bg_gmm[c].weight /= bg_total;
        }

        // Reassign labels
        for (int i = 0; i < size; ++i)
        {
            float x = (float)src[i];
            float p_fg = 0.0f, p_bg = 0.0f;
            for (int c = 0; c < GMM_COMPONENTS; ++c)
            {
                p_fg += fg_gmm[c].weight * gaussian_prob(x, fg_gmm[c].mean, fg_gmm[c].variance);
                p_bg += bg_gmm[c].weight * gaussian_prob(x, bg_gmm[c].mean, bg_gmm[c].variance);
            }
            labels[i] = (p_fg > p_bg) ? 1 : 0;
            mask[i] = labels[i] ? FOREGROUND : BACKGROUND;
        }
    }

    memory_free(labels);
    memory_free(fg_resp);
    memory_free(bg_resp);
}

typedef struct
{
    float weight;
    float mean[3];     // [R, G, B]
    float variance[3]; // diagonal covariance
} GMMComponentRGB;

float gaussian_prob_rgb(const uint8_t *pixel, const GMMComponentRGB *comp)
{
    float prob = 1.0f;
    for (int i = 0; i < 3; ++i)
    {
        float diff = (float)pixel[i] - comp->mean[i];
        float var = comp->variance[i];
        prob *= (1.0f / sqrtf(2.0f * M_PI * var)) * expf(-diff * diff / (2.0f * var));
    }
    return prob;
}

void grabCutRGB(const Image *inImg, Image *outMask, Rect roi, int max_iter)
{
    if (!inImg || !outMask || !inImg->pixels || inImg->format != IMAGE_FORMAT_RGB888)
        return;

    int width = inImg->width;
    int height = inImg->height;
    int size = width * height;
    const uint8_t *src = (const uint8_t *)inImg->pixels;
    uint8_t *mask = (uint8_t *)outMask->pixels;

    // Allocate label buffer (0 = BG, 1 = FG)
    uint8_t *labels = (uint8_t *)memory_alloc(size * sizeof(uint8_t));
    float(*fg_resp)[GMM_COMPONENTS] = (float(*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));
    float(*bg_resp)[GMM_COMPONENTS] = (float(*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));

    GMMComponentRGB fg_gmm[GMM_COMPONENTS];
    GMMComponentRGB bg_gmm[GMM_COMPONENTS];

    // Step 1: Initial Labeling from ROI
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = y * width + x;
            if (x >= roi.x && x < roi.x + roi.width && y >= roi.y && y < roi.y + roi.height)
            {
                mask[idx] = FOREGROUND;
                labels[idx] = 1;
            }
            else
            {
                mask[idx] = BACKGROUND;
                labels[idx] = 0;
            }
        }
    }

    // Step 2: Init GMMs
    for (int c = 0; c < GMM_COMPONENTS; ++c)
    {
        for (int ch = 0; ch < 3; ++ch)
        {
            fg_gmm[c].mean[ch] = 100.0f + 50 * c;
            fg_gmm[c].variance[ch] = 1000.0f;
            bg_gmm[c].mean[ch] = 50.0f + 100 * c;
            bg_gmm[c].variance[ch] = 1000.0f;
        }
        fg_gmm[c].weight = 0.5f;
        bg_gmm[c].weight = 0.5f;
    }

    // Step 3: EM Iterations
    for (int iter = 0; iter < max_iter; ++iter)
    {
        // E-Step: compute responsibilities
        for (int i = 0; i < size; ++i)
        {
            const uint8_t *px = &src[i * 3];
            float total_fg = 0.0f, total_bg = 0.0f;

            for (int c = 0; c < GMM_COMPONENTS; ++c)
            {
                fg_resp[i][c] = fg_gmm[c].weight * gaussian_prob_rgb(px, &fg_gmm[c]);
                bg_resp[i][c] = bg_gmm[c].weight * gaussian_prob_rgb(px, &bg_gmm[c]);
                total_fg += fg_resp[i][c];
                total_bg += bg_resp[i][c];
            }
            for (int c = 0; c < GMM_COMPONENTS; ++c)
            {
                fg_resp[i][c] /= (total_fg + 1e-6f);
                bg_resp[i][c] /= (total_bg + 1e-6f);
            }
        }

        // M-Step: update GMM parameters
        for (int c = 0; c < GMM_COMPONENTS; ++c)
        {
            float fg_wsum = 0.0f, bg_wsum = 0.0f;
            float fg_sum[3] = {0}, fg_sqsum[3] = {0};
            float bg_sum[3] = {0}, bg_sqsum[3] = {0};

            for (int i = 0; i < size; ++i)
            {
                const uint8_t *px = &src[i * 3];
                if (labels[i] == 1)
                {
                    float r = fg_resp[i][c];
                    fg_wsum += r;
                    for (int ch = 0; ch < 3; ++ch)
                    {
                        fg_sum[ch] += r * px[ch];
                        fg_sqsum[ch] += r * px[ch] * px[ch];
                    }
                }
                else
                {
                    float r = bg_resp[i][c];
                    bg_wsum += r;
                    for (int ch = 0; ch < 3; ++ch)
                    {
                        bg_sum[ch] += r * px[ch];
                        bg_sqsum[ch] += r * px[ch] * px[ch];
                    }
                }
            }

            for (int ch = 0; ch < 3; ++ch)
            {
                if (fg_wsum > 1e-6f)
                {
                    fg_gmm[c].mean[ch] = fg_sum[ch] / fg_wsum;
                    float var = (fg_sqsum[ch] / fg_wsum) - fg_gmm[c].mean[ch] * fg_gmm[c].mean[ch];
                    fg_gmm[c].variance[ch] = fmaxf(var, 10.0f);
                }

                if (bg_wsum > 1e-6f)
                {
                    bg_gmm[c].mean[ch] = bg_sum[ch] / bg_wsum;
                    float var = (bg_sqsum[ch] / bg_wsum) - bg_gmm[c].mean[ch] * bg_gmm[c].mean[ch];
                    bg_gmm[c].variance[ch] = fmaxf(var, 10.0f);
                }
            }

            fg_gmm[c].weight = fg_wsum;
            bg_gmm[c].weight = bg_wsum;
        }

        // Normalize weights
        float fg_total = 0.0f, bg_total = 0.0f;
        for (int c = 0; c < GMM_COMPONENTS; ++c)
        {
            fg_total += fg_gmm[c].weight;
            bg_total += bg_gmm[c].weight;
        }
        for (int c = 0; c < GMM_COMPONENTS; ++c)
        {
            fg_gmm[c].weight /= (fg_total + 1e-6f);
            bg_gmm[c].weight /= (bg_total + 1e-6f);
        }

        // Reassign labels and update mask
        for (int i = 0; i < size; ++i)
        {
            const uint8_t *px = &src[i * 3];
            float p_fg = 0.0f, p_bg = 0.0f;
            for (int c = 0; c < GMM_COMPONENTS; ++c)
            {
                p_fg += fg_gmm[c].weight * gaussian_prob_rgb(px, &fg_gmm[c]);
                p_bg += bg_gmm[c].weight * gaussian_prob_rgb(px, &bg_gmm[c]);
            }
            labels[i] = (p_fg > p_bg) ? 1 : 0;
            mask[i] = labels[i] ? FOREGROUND : BACKGROUND;
        }
    }

    memory_free(labels);
    memory_free(fg_resp);
    memory_free(bg_resp);
}

/**
 * @brief Resizes a single-channel image to a square output using nearest-neighbor interpolation.
 *
 * @param[in]  inImg        Pointer to the input image.
 * @param[out] outImg       Pointer to the output image. It must already be allocated to (fourier_size × fourier_size).
 * @param[in]  size Desired width and height of the output image.
 */
void resize(Image *inImg, Image *outImg, int size)
{

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    float width_ratio = (float)inImg->width / size;
    float height_ratio = (float)inImg->height / size;

    // Clear output buffer to white (0xFF)
    for (int y = 0; y < outImg->width * outImg->height; y++)
        ((float *)outImg->chals->ch[0])[y] = 255.0f;

    // Resize using nearest neighbor
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int nearest_x = (int)(x * width_ratio);
            int nearest_y = (int)(y * height_ratio);

            if (nearest_x >= inImg->width)
                nearest_x = inImg->width - 1;
            if (nearest_y >= inImg->height)
                nearest_y = inImg->height - 1;

            outImg->chals->ch[0][y * size + x] =
                (float)((uint8_t *)inImg->pixels)[nearest_y * inImg->width + nearest_x];
        }
    }

    outImg->width = size;
    outImg->height = size;
    outImg->size = size * size * inImg->depth;
}

/**
 * @brief Performs pixel-wise addition of two float-valued grayscale images.
 *
 * This function adds the corresponding float values from `img1` and `img2`, and stores
 * the result in `outImg->chals->ch[0][i]`.
 *
 * Assumes all images have the same resolution and float output channel allocated.
 *
 * @param[in]  img1   Pointer to the first image (float channel expected).
 * @param[in]  img2   Pointer to the second image (float channel expected).
 * @param[out] outImg Pointer to the output image (float channel).
 */
void add(const Image *img1, const Image *img2, Image *outImg)
{

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(img1) && isChalsEmpty(img2))
    {
        int totalPixels = img1->width * img1->height * img1->depth;
        ;

        uint8_t *data1 = img1->pixels;
        uint8_t *data2 = img2->pixels;
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
    else if (isChalsEmpty(img1) && !isChalsEmpty(img2))
    {
        int totalPixels = img1->width * img1->height * img1->depth;

        uint8_t *data1 = img1->pixels;
        float *data2 = img2->chals->ch[0];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
    else if (!isChalsEmpty(img1) && isChalsEmpty(img2))
    {
        int totalPixels = img1->width * img1->height * img1->depth;
        ;

        float *data1 = img1->chals->ch[0];
        uint8_t *data2 = img2->pixels;
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
    else
    {
        int totalPixels = img1->width * img1->height * img1->depth;
        ;

        float *data1 = img1->chals->ch[0];
        float *data2 = img2->chals->ch[0];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
}

void dist(const Image *inImg, Image *outImg, uint8_t R_ref, uint8_t G_ref, uint8_t B_ref)
/**
 * @brief Computes the color distance of each pixel in an RGB
 * image to a given reference color.
 *
 * @param[in] inImg Pointer to the input RGB image (3 channels, interleaved as RGBRGB...).
 * @param[out] outImg Pointer to the output grayscale image (1 channel, same width and height as input).
 * @param[in] R_ref Reference Red channel value (0–255).
 * @param[in] G_ref Reference Green channel value (0–255).
 * @param[in] B_ref Reference Blue channel value (0–255).
 */
{
    int totalPixels = inImg->width * inImg->height;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        // Raw byte access
        uint8_t *inData = (uint8_t *)inImg->pixels;

        for (int i = 0; i < totalPixels; ++i)
        {
            int idx = i * 3;
            uint8_t R = inData[idx];
            uint8_t G = inData[idx + 1];
            uint8_t B = inData[idx + 2];

            float d = sqrtf((R - R_ref) * (R - R_ref) +
                            (G - G_ref) * (G - G_ref) +
                            (B - B_ref) * (B - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    }
    else
    {
        // Channel access (float-based)
        float *R_ch = inImg->chals->ch[0];
        float *G_ch = inImg->chals->ch[1];
        float *B_ch = inImg->chals->ch[2];

        for (int i = 0; i < totalPixels; ++i)
        {
            float d = sqrtf((R_ch[i] - R_ref) * (R_ch[i] - R_ref) +
                            (G_ch[i] - G_ref) * (G_ch[i] - G_ref) +
                            (B_ch[i] - B_ref) * (B_ch[i] - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    }
}
/*
static inline uint8_t normalize_to_u8(float val, float min, float max)
{
    if (max - min < 1e-5f) // avoid divide by zero
        return 0;
    float norm = (val - min) / (max - min);
    if (norm < 0.0f)
        norm = 0.0f;
    if (norm > 1.0f)
        norm = 1.0f;
    return (uint8_t)(norm * 255.0f);
}

void normalize(Image *inImg)
{
    float *data = (float *)inImg->chals->ch[0];

    float min = FLT_MAX, max = -FLT_MAX;
    for (int i = 0; i < inImg->size; i++)
    {
        float v = inImg->chals->ch[0][i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }
    for (int i = 0; i < inImg->size; i++)
    {
        data[i] = normalize_to_u8(inImg->chals->ch[0][i], min, max);
    }
}
*/

static inline uint8_t normalize_to_u8(uint8_t val, uint8_t min, uint8_t max)
{
    if (max - min < 1e-5f) // avoid divide by zero
        return 0;
    float norm = ((float)val - (float)min) / ((float)max - (float)min);
    if (norm < 0.0f)
        norm = 0.0f;
    if (norm > 1.0f)
        norm = 1.0f;
    return (uint8_t)(norm * 255.0f);
}

void normalize(Image *inImg)
{
    uint8_t *data = (uint8_t *)inImg->pixels;

    uint8_t min = 255, max = 0;
    for (int i = 0; i < inImg->size; i++)
    {
        uint8_t v = (uint8_t)data[i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }
    for (int i = 0; i < inImg->size; i++)
    {
        data[i] = normalize_to_u8(data[i], min, max);
    }
}

void convertTo(Image *inImg)
{

    uint8_t *data = (uint8_t *)inImg->pixels;

    switch (inImg->format)
    {
    case IMAGE_FORMAT_GRAYSCALE:
    {
        if (inImg->log == IMAGE_DATA_CH0 || inImg->log == IMAGE_DATA_MAGNITUDE)
        {
            float min = FLT_MAX, max = -FLT_MAX;

            // Step 1: Find min and max
            for (int i = 0; i < inImg->size; i++)
            {
                float v = inImg->chals->ch[0][i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }

            // Step 2: Normalize to [0, 255] and clamp
            if (max != min)
            {
                for (int i = 0; i < inImg->size; i++)
                {
                    float norm = (inImg->chals->ch[0][i] - min) / (max - min);
                    data[i] = (uint8_t)(norm * 255.0f + 0.5f); // +0.5 for rounding
                }
            }
            else
            {
                // All values are the same, map to 0 or 255
                memset(data, 0, inImg->size); // Or use 255
            }
        }
        else if (inImg->log == IMAGE_DATA_PIXELS)
        {
            uint8_t min = 255, max = 0;
            for (int i = 0; i < inImg->size; i++)
            {
                uint8_t v = (uint8_t)data[i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }
            for (int i = 0; i < inImg->size; i++)
            {
                data[i] = normalize_to_u8(data[i], min, max);
            }
        }

        break;
    }

    case IMAGE_FORMAT_RGB888:
    case IMAGE_FORMAT_YUV:
    case IMAGE_FORMAT_HSI:
    {
        int ch_r = 1, ch_g = 2, ch_b = 3;
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (int i = 0; i < inImg->size; i++)
        {
            if (inImg->chals->ch[ch_r][i] < min_r)
                min_r = inImg->chals->ch[ch_r][i];
            if (inImg->chals->ch[ch_r][i] > max_r)
                max_r = inImg->chals->ch[ch_r][i];
            if (inImg->chals->ch[ch_g][i] < min_g)
                min_g = inImg->chals->ch[ch_g][i];
            if (inImg->chals->ch[ch_g][i] > max_g)
                max_g = inImg->chals->ch[ch_g][i];
            if (inImg->chals->ch[ch_b][i] < min_b)
                min_b = inImg->chals->ch[ch_b][i];
            if (inImg->chals->ch[ch_b][i] > max_b)
                max_b = inImg->chals->ch[ch_b][i];
        }

        for (int i = 0; i < inImg->size; i++)
        {
            data[i * 3 + 0] = normalize_to_u8(inImg->chals->ch[ch_b][i], min_b, max_b);
            data[i * 3 + 1] = normalize_to_u8(inImg->chals->ch[ch_g][i], min_g, max_g);
            data[i * 3 + 2] = normalize_to_u8(inImg->chals->ch[ch_r][i], min_r, max_r);
        }
        break;
    }

    case IMAGE_FORMAT_RGB565:
    {
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (int i = 0; i < inImg->size; i++)
        {
            if (inImg->chals->ch[1][i] < min_r)
                min_r = inImg->chals->ch[1][i];
            if (inImg->chals->ch[1][i] > max_r)
                max_r = inImg->chals->ch[1][i];
            if (inImg->chals->ch[2][i] < min_g)
                min_g = inImg->chals->ch[2][i];
            if (inImg->chals->ch[2][i] > max_g)
                max_g = inImg->chals->ch[2][i];
            if (inImg->chals->ch[3][i] < min_b)
                min_b = inImg->chals->ch[3][i];
            if (inImg->chals->ch[3][i] > max_b)
                max_b = inImg->chals->ch[3][i];
        }

        for (int i = 0; i < inImg->size; i++)
        {
            uint8_t r = normalize_to_u8(inImg->chals->ch[1][i], min_r, max_r);
            uint8_t g = normalize_to_u8(inImg->chals->ch[2][i], min_g, max_g);
            uint8_t b = normalize_to_u8(inImg->chals->ch[3][i], min_b, max_b);

            uint16_t rgb565 = (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
            data[i * 2 + 0] = (uint8_t)(rgb565 & 0xFF);
            data[i * 2 + 1] = (uint8_t)((rgb565 >> 8) & 0xFF);
        }
        break;
    }

    default:
        break;
    }

    inImg->log = IMAGE_DATA_PIXELS;
}