#include "pixel.h"

#include "core/error.h"

#include <stdio.h> /* For printf */
#include <string.h> /* For memset, memcpy */

// Validation macros to replace assert()
#define CHECK_NULL_VOID(ptr)                                                                       \
    do {                                                                                           \
        if (!(ptr))                                                                                \
            return;                                                                                \
    } while (0)

#define CHECK_FORMAT_VOID(img, expected_fmt)                                                       \
    do {                                                                                           \
        if ((img)->format != (expected_fmt))                                                       \
            return;                                                                                \
    } while (0)

#define CHECK_CONDITION_VOID(cond)                                                                 \
    do {                                                                                           \
        if (!(cond))                                                                               \
            return;                                                                                \
    } while (0)

#ifndef M_PI
    #define M_PI 3.14159265358979323846
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
 * @note Assumes both images are of the same size and format, and `UINT8_MAX` is defined
 * appropriately.
 */
int negative(const Image *inImg, Image *outImg)
{
    uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        outData[i] = UINT8_MAX - imgData[i];
    }

    outImg->log = IMAGE_DATA_PIXELS;
    return EMBEDDIP_OK;
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
int grayscaleThreshold(const Image *inImg, Image *outImg, uint8_t threshold)
{
    uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        outData[i] = (imgData[i] >= threshold) ? 255 : 0;
    }

    outImg->log = IMAGE_DATA_PIXELS;
    return EMBEDDIP_OK;
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

    for (int t = 0; t < 256; ++t) {
        wB += hist[t];  // Weight Background
        if (wB == 0)
            continue;

        wF = size - wB;  // Weight Foreground
        if (wF == 0)
            break;

        sumB += (float)(t * hist[t]);

        float mB = sumB / wB;          // Mean Background
        float mF = (sum - sumB) / wF;  // Mean Foreground

        // Between Class Variance
        float varBetween = (float)wB * wF * (mB - mF) * (mB - mF);

        if (varBetween > maxVar) {
            maxVar = varBetween;
            threshold = (uint8_t)t;
        }
    }

    return threshold;
}

/**
 * @brief Applies Otsu's binary thresholding to a grayscale image.
 *
 * @param[in]  inImg   Pointer to input image (grayscale, 8-bit pixels).
 * @param[out] outImg  Pointer to output binary image.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
int grayscaleOtsu(const Image *inImg, Image *outImg)
{
    // Input validation
    if (!inImg || !outImg)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (!inImg->pixels || !outImg->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (inImg->format != IMAGE_FORMAT_GRAYSCALE || outImg->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;

    if (inImg->width != outImg->width || inImg->height != outImg->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    const uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    uint8_t threshold = OtsuThreshold(imgData, inImg->size);

    for (uint32_t i = 0; i < inImg->size; ++i) {
        outData[i] = (imgData[i] >= threshold) ? 255 : 0;
    }
    return EMBEDDIP_OK;
}

int grayscaleThresholdLocalOtsu(const Image *inImg, Image *outImg, int blockSize)
{
    const int width = inImg->width;
    const int height = inImg->height;
    const uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (int y = 0; y < height; y += blockSize) {
        for (int x = 0; x < width; x += blockSize) {
            int blockW = (x + blockSize > width) ? (width - x) : blockSize;
            int blockH = (y + blockSize > height) ? (height - y) : blockSize;

            // 1. Extract block into a temporary buffer
            uint8_t blockPixels[blockSize * blockSize];
            int count = 0;

            for (int j = 0; j < blockH; ++j) {
                for (int i = 0; i < blockW; ++i) {
                    blockPixels[count++] = imgData[(y + j) * width + (x + i)];
                }
            }

            // 2. Compute Otsu threshold for this block
            uint8_t threshold = OtsuThreshold(blockPixels, count);

            // 3. Apply threshold to each pixel in the block
            for (int j = 0; j < blockH; ++j) {
                for (int i = 0; i < blockW; ++i) {
                    int idx = (y + j) * width + (x + i);
                    outData[idx] = (imgData[idx] >= threshold) ? 255 : 0;
                }
            }
        }
    }
    return EMBEDDIP_OK;
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
int grayscaleKMeans(const Image *inImg, Image *outImg, int k)
{
    if (!inImg || !outImg || !inImg->pixels || k <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    if (isChalsEmpty(outImg)) {
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

    if (!centers || !labels || !counts || !sums) {
        // Cleanup any successful allocations
        if (centers) memory_free(centers);
        if (labels) memory_free(labels);
        if (counts) memory_free(counts);
        if (sums) memory_free(sums);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Initialize cluster centers evenly spaced
    for (int i = 0; i < k; ++i)
        centers[i] = (255.0f / (k - 1)) * i;

    for (int iter = 0; iter < MAX_ITER; ++iter) {
        // Reset accumulators
        for (int j = 0; j < k; ++j) {
            counts[j] = 0;
            sums[j] = 0.0f;
        }

        // Assign labels
        for (int i = 0; i < size; ++i) {
            float minDist = 1e9f;
            int best = 0;

            for (int j = 0; j < k; ++j) {
                float dist = fabsf((float)imgData[i] - centers[j]);
                if (dist < minDist) {
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
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0)
                centers[j] = sums[j] / counts[j];
            else
                centers[j] = (float)(rand() % 256);  // reinitialize randomly
        }
    }

    // Final image: assign cluster center as output
    for (int i = 0; i < size; ++i) {
        int c = CLAMP(labels[i], 0, k - 1);
        outData[i] = (float)centers[c];
    }

    outImg->log = IMAGE_DATA_CH0;

    // Cleanup
    memory_free(centers);
    memory_free(labels);
    memory_free(counts);
    memory_free(sums);

    return EMBEDDIP_OK;
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
embeddip_status_t colorKMeans_old(const Image *inImg, Image *outImg, int k)
{
    // Input validation
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (k <= 0 || k > 255) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // Support RGB888, HSI, and YUV (all 3-channel formats)
    if (inImg->format != IMAGE_FORMAT_RGB888 &&
        inImg->format != IMAGE_FORMAT_HSI &&
        inImg->format != IMAGE_FORMAT_YUV) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (outImg->format != inImg->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    int size = inImg->size;
    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    typedef struct {
        float c0, c1, c2;  // Generic: R/H/Y, G/S/U, B/I/V
    } color_t;

    color_t *centers = (color_t *)memory_alloc(k * sizeof(color_t));
    int *labels = (int *)memory_alloc(size * sizeof(int));
    int *counts = (int *)memory_alloc(k * sizeof(int));
    color_t *sums = (color_t *)memory_alloc(k * sizeof(color_t));

    if (!centers || !labels || !counts || !sums) {
        // Cleanup any successful allocations
        if (centers) memory_free(centers);
        if (labels) memory_free(labels);
        if (counts) memory_free(counts);
        if (sums) memory_free(sums);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Check if format is HSI (needs hue wraparound)
    bool is_hsi = (inImg->format == IMAGE_FORMAT_HSI);

    // Initialize cluster centers evenly spaced in third channel (I/B/V)
    for (int i = 0; i < k; ++i) {
        centers[i].c0 = (float)(rand() % 256);
        centers[i].c1 = (float)(rand() % 256);
        centers[i].c2 = (255.0f / (k - 1)) * i;
    }

    for (int iter = 0; iter < MAX_ITER; ++iter) {
        // Reset sums and counts
        for (int j = 0; j < k; ++j) {
            counts[j] = 0;
            sums[j].c0 = sums[j].c1 = sums[j].c2 = 0.0f;
        }

        // Assignment step
        for (int i = 0; i < size; ++i) {
            int idx = i * 3;
            float ch0 = in[idx];
            float ch1 = in[idx + 1];
            float ch2 = in[idx + 2];

            float minDist = 1e9f;
            int best = 0;

            for (int j = 0; j < k; ++j) {
                float d0 = fabsf(ch0 - centers[j].c0);
                // HSI format: wraparound hue distance for first channel
                if (is_hsi && d0 > 128)
                    d0 = 256 - d0;

                float d1 = ch1 - centers[j].c1;
                float d2 = ch2 - centers[j].c2;

                float dist = d0 * d0 + d1 * d1 + d2 * d2;

                if (dist < minDist) {
                    minDist = dist;
                    best = j;
                }
            }

            labels[i] = best;
            sums[best].c0 += ch0;
            sums[best].c1 += ch1;
            sums[best].c2 += ch2;
            counts[best]++;
        }

        // Update step
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0) {
                centers[j].c0 = sums[j].c0 / counts[j];
                centers[j].c1 = sums[j].c1 / counts[j];
                centers[j].c2 = sums[j].c2 / counts[j];
            } else {
                // Reinitialize empty cluster randomly
                centers[j].c0 = (float)(rand() % 256);
                centers[j].c1 = (float)(rand() % 256);
                centers[j].c2 = (float)(rand() % 256);
            }
        }
    }

    // Final assignment - replace each pixel with its cluster center
    for (int i = 0; i < size; ++i) {
        int idx = i * 3;
        int c = CLAMP(labels[i], 0, k - 1);

        out[idx + 0] = (uint8_t)centers[c].c0;
        out[idx + 1] = (uint8_t)centers[c].c1;
        out[idx + 2] = (uint8_t)centers[c].c2;
    }

    memory_free(centers);
    memory_free(labels);
    memory_free(counts);
    memory_free(sums);

    return EMBEDDIP_OK;
}

/* Read pixel idx as normalized 3-vector in the *native* space.
   HSI: [H,S,I] in [0,1]  (hue wrap handled in distance)
   YUV: [Y,U,V] in [0,1]
   RGB: [R,G,B] in [0,1] (RGB565 unpacked) */
static inline void read_vec3_norm(const Image *img, int idx, float v[3])
{
    const uint8_t *p8 = (const uint8_t *)img->pixels;

    switch (img->format) {
    case IMAGE_FORMAT_HSI: {
        int off = idx * 3;
        v[0] = p8[off + 0] / 255.0f;
        v[1] = p8[off + 1] / 255.0f;
        v[2] = p8[off + 2] / 255.0f;
    } break;

    case IMAGE_FORMAT_YUV: {
        int off = idx * 3;
        v[0] = p8[off + 0] / 255.0f;
        v[1] = p8[off + 1] / 255.0f;
        v[2] = p8[off + 2] / 255.0f;
    } break;

    case IMAGE_FORMAT_RGB888: {
        int off = idx * 3;
        v[0] = p8[off + 0] / 255.0f;
        v[1] = p8[off + 1] / 255.0f;
        v[2] = p8[off + 2] / 255.0f;
    } break;

    case IMAGE_FORMAT_RGB565: {
        const uint16_t *p16 = (const uint16_t *)img->pixels;
        uint16_t px = p16[idx];
        float r = (float)((px >> 11) & 0x1F) / 31.0f;
        float g = (float)((px >> 5) & 0x3F) / 63.0f;
        float b = (float)(px & 0x1F) / 31.0f;
        v[0] = r;
        v[1] = g;
        v[2] = b;
    } break;

    default:
        v[0] = v[1] = v[2] = 0.0f;
        break;
    }
}

/* Write a normalized 3-vector (0..1) back to the image format. */
static inline void write_vec3_norm(Image *img, int idx, const float v[3])
{
    uint8_t *p8 = (uint8_t *)img->pixels;

    switch (img->format) {
    case IMAGE_FORMAT_HSI:
    case IMAGE_FORMAT_YUV:
    case IMAGE_FORMAT_RGB888: {
        int off = idx * 3;
        p8[off + 0] = (uint8_t)CLAMP((int)lrintf(v[0] * 255.0f), 0, 255);
        p8[off + 1] = (uint8_t)CLAMP((int)lrintf(v[1] * 255.0f), 0, 255);
        p8[off + 2] = (uint8_t)CLAMP((int)lrintf(v[2] * 255.0f), 0, 255);
    } break;

    case IMAGE_FORMAT_RGB565: {
        uint16_t *p16 = (uint16_t *)img->pixels;
        int r5 = CLAMP((int)lrintf(v[0] * 31.0f), 0, 31);
        int g6 = CLAMP((int)lrintf(v[1] * 63.0f), 0, 63);
        int b5 = CLAMP((int)lrintf(v[2] * 31.0f), 0, 31);
        uint16_t px = (uint16_t)((r5 << 11) | (g6 << 5) | (b5));
        p16[idx] = px;
    } break;

    default:
        break;
    }
}

/* Distance with special hue wrap for HSI; Euclidean otherwise.
   a,b are normalized 0..1 vectors in native space. */
static inline float vec_distance_native(const float a[3], const float b[3], ImageFormat fmt)
{
    if (fmt == IMAGE_FORMAT_HSI) {
        float dh_raw = fabsf(a[0] - b[0]);
        float dh = fminf(dh_raw, 1.0f - dh_raw);
        float ds = a[1] - b[1];
        float di = a[2] - b[2];
        return dh * dh + ds * ds + di * di; /* squared distance (faster) */
    } else {
        float d0 = a[0] - b[0];
        float d1 = a[1] - b[1];
        float d2 = a[2] - b[2];
        return d0 * d0 + d1 * d1 + d2 * d2; /* squared distance */
    }
}

/* Clamp/normalize center components to [0,1] after averaging. */
static inline void center_divide(float c[3], int cnt)
{
    if (cnt > 0) {
        float inv = 1.0f / (float)cnt;
        c[0] *= inv;
        c[1] *= inv;
        c[2] *= inv;
        c[0] = fminf(fmaxf(c[0], 0.0f), 1.0f);
        c[1] = fminf(fmaxf(c[1], 0.0f), 1.0f);
        c[2] = fminf(fmaxf(c[2], 0.0f), 1.0f);
    }
}

/* ----------------------------- K-means ------------------------------ */

/**
 * @brief Segments an image using K-means in its native color space.
 *
 * Supported formats: HSI (hue wrap), YUV444, RGB888, RGB565.
 * The output image must have same dimensions and format as input.
 * Each pixel is replaced by its cluster center color (in the same space/format).
 *
 * @param[in]  inImg   Input image
 * @param[out] outImg  Output segmented image (same format as input)
 * @param[in]  k       Number of clusters (>=1, <= number of pixels)
 */
embeddip_status_t colorKMeans(const Image *inImg, Image *outImg, int k)
{
    // Input validation
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (k <= 0 || k > 255) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // Support RGB888, RGB565, HSI, and YUV
    if (inImg->format != IMAGE_FORMAT_RGB888 &&
        inImg->format != IMAGE_FORMAT_RGB565 &&
        inImg->format != IMAGE_FORMAT_HSI &&
        inImg->format != IMAGE_FORMAT_YUV) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->format != outImg->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const int N = inImg->size;
    if (k > N)
        k = N;

    /* Buffers */
    int *labels = (int *)memory_alloc((size_t)N * sizeof(int));
    int *counts = (int *)memory_alloc((size_t)k * sizeof(int));
    float *centers = (float *)memory_alloc((size_t)k * 3 * sizeof(float)); /* k x 3 */
    float *sums = (float *)memory_alloc((size_t)k * 3 * sizeof(float));    /* k x 3 */

    if (!labels || !counts || !centers || !sums) {
        // Cleanup any successful allocations
        if (labels) memory_free(labels);
        if (counts) memory_free(counts);
        if (centers) memory_free(centers);
        if (sums) memory_free(sums);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    /* --- Initialization: pick k random pixels as initial centers --- */
    for (int j = 0; j < k; ++j) {
        int rnd = rand() % N;
        read_vec3_norm(inImg, rnd, &centers[3 * j]);
        /* Ensure numeric sanity */
        centers[3 * j + 0] = fminf(fmaxf(centers[3 * j + 0], 0.0f), 1.0f);
        centers[3 * j + 1] = fminf(fmaxf(centers[3 * j + 1], 0.0f), 1.0f);
        centers[3 * j + 2] = fminf(fmaxf(centers[3 * j + 2], 0.0f), 1.0f);
    }

    /* --- Iterations --- */
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        /* reset sums/counts */
        for (int j = 0; j < k; ++j) {
            counts[j] = 0;
            sums[3 * j + 0] = sums[3 * j + 1] = sums[3 * j + 2] = 0.0f;
        }

        /* assignment */
        for (int i = 0; i < N; ++i) {
            float v[3];
            read_vec3_norm(inImg, i, v);

            float bestD = 1e30f;
            int bestC = 0;
            for (int j = 0; j < k; ++j) {
                float d = vec_distance_native(v, &centers[3 * j], inImg->format);
                if (d < bestD) {
                    bestD = d;
                    bestC = j;
                }
            }
            labels[i] = bestC;
            sums[3 * bestC + 0] += v[0];
            sums[3 * bestC + 1] += v[1];
            sums[3 * bestC + 2] += v[2];
            counts[bestC] += 1;
        }

        /* update */
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0) {
                float c[3] = {sums[3 * j + 0], sums[3 * j + 1], sums[3 * j + 2]};
                center_divide(c, counts[j]);
                centers[3 * j + 0] = c[0];
                centers[3 * j + 1] = c[1];
                centers[3 * j + 2] = c[2];
            } else {
                /* empty cluster: re-seed from a random pixel */
                int rnd = rand() % N;
                read_vec3_norm(inImg, rnd, &centers[3 * j]);
            }
        }
    }

    /* --- Write result: paint each pixel with its cluster center in SAME format --- */
    for (int i = 0; i < N; ++i) {
        int c = CLAMP(labels[i], 0, k - 1);
        write_vec3_norm(outImg, i, &centers[3 * c]);
    }

    memory_free(labels);
    memory_free(counts);
    memory_free(centers);
    memory_free(sums);

    return EMBEDDIP_OK;
}

// New idea. Will check
/*
int grayscaleKMeans(const Image *inImg, Image *outImg, int k)
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

/*
int grayscaleRegionGrowing(const Image *inImg, Image *outImg, int seedX, int seedY, uint8_t
tolerance)
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
    */

/**
 * @brief Segments a grayscale image using region growing with multiple seed points.
 *
 * This function performs region growing starting from one or more seed pixels.
 * A pixel is included in the region if the absolute intensity difference from
 * the seed pixel’s intensity is below the specified tolerance.
 *
 * @param[in]  inImg      Pointer to input grayscale image.
 * @param[out] outImg     Pointer to output binary image (0 for background, 255 for region).
 * @param[in]  seeds      Array of seed points.
 * @param[in]  numSeeds   Number of seed points.
 * @param[in]  tolerance  Intensity difference threshold for region inclusion.
 */
embeddip_status_t grayscaleRegionGrowing(const Image *inImg,
                                          Image *outImg,
                                          const Point *seeds,
                                          int numSeeds,
                                          uint8_t tolerance)
{
    // Input validation
    if (!inImg || !outImg || !seeds) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (inImg->format != IMAGE_FORMAT_GRAYSCALE || outImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (numSeeds <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const int width = inImg->width;
    const int height = inImg->height;
    const uint8_t *src = inImg->pixels;
    uint8_t *dst = outImg->pixels;
    memset(dst, 0, inImg->size);

    bool *visited = (bool *)memory_alloc(sizeof(bool) * inImg->size);
    if (!visited) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    memset(visited, 0, inImg->size);

    Point *stack = (Point *)memory_alloc(sizeof(Point) * STACK_SIZE);
    if (!stack) {
        memory_free(visited);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    for (int s = 0; s < numSeeds; ++s) {
        int seedX = seeds[s].x;
        int seedY = seeds[s].y;
        if (seedX < 0 || seedX >= width || seedY < 0 || seedY >= height)
            continue;

        int seedIndex = seedY * width + seedX;
        if (visited[seedIndex])
            continue;

        // Run the same region growing as single-seed
        int top = 0;
        stack[top++] = seeds[s];
        visited[seedIndex] = true;
        dst[seedIndex] = 255;

        long sum = src[seedIndex];
        int count = 1;

        int dx[4] = {0, -1, 1, 0};
        int dy[4] = {-1, 0, 0, 1};

        while (top > 0) {
            Point p = stack[--top];
            uint8_t regionMean = (uint8_t)(sum / count);

            for (int i = 0; i < 4; ++i) {
                int nx = p.x + dx[i];
                int ny = p.y + dy[i];
                int nidx = ny * width + nx;

                if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[nidx]) {
                    uint8_t neighborValue = src[nidx];
                    if (abs((int)neighborValue - (int)regionMean) <= tolerance) {
                        visited[nidx] = true;
                        dst[nidx] = 255;
                        stack[top++] = (Point){nx, ny};

                        sum += neighborValue;
                        count++;

                        if (top >= STACK_SIZE) {
                            memory_free(visited);
                            memory_free(stack);
                            return EMBEDDIP_ERROR_OUT_OF_MEMORY; // Stack overflow
                        }
                    }
                }
            }
        }
    }

    memory_free(visited);
    memory_free(stack);
    return EMBEDDIP_OK;
}

/**
 * @brief Performs region growing on an HSI image using color similarity.
 *
 * Grows a region from a seed point by comparing HSI vectors using Euclidean distance.
 * Pixels within a given threshold are included in the region.
 *
 * @param[in]  inImg      Pointer to input HSI image (IMAGE_FORMAT_HSI).
 * @param[out] outMask    Pointer to output HSI image
 * @param[in]  seedX      X coordinate of seed point.
 * @param[in]  seedY      Y coordinate of seed point.
 * @param[in]  tolerance  Threshold on HSI distance (e.g., 0.1–0.4 typical).
 */
embeddip_status_t colorRegionGrowing_single(const Image *inImg, Image *outImg, int seedX, int seedY, float tolerance)
{
    // Input validation
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (inImg->format != IMAGE_FORMAT_HSI || outImg->format != IMAGE_FORMAT_HSI) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const int width = inImg->width;
    const int height = inImg->height;

    // Validate seed coordinates
    if (seedX < 0 || seedX >= width || seedY < 0 || seedY >= height) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const uint8_t *src = (const uint8_t *)inImg->pixels;
    uint8_t *dst = (uint8_t *)outImg->pixels;

    memset(dst, 0, inImg->size * 3);  // Set all HSI values in output to 0 (black)

    bool *visited = (bool *)memory_alloc(sizeof(bool) * inImg->size);
    if (!visited) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    memset(visited, 0, inImg->size);

    Point *stack = (Point *)memory_alloc(sizeof(Point) * STACK_SIZE);
    if (!stack) {
        memory_free(visited);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
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

    while (top > 0) {
        Point p = stack[--top];

        for (int d = 0; d < 4; ++d) {
            int nx = p.x + dx[d];
            int ny = p.y + dy[d];
            int nidx = ny * width + nx;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[nidx]) {
                float h = src[nidx * 3] / 255.0f;
                float s = src[nidx * 3 + 1] / 255.0f;
                float ii = src[nidx * 3 + 2] / 255.0f;

                // Hue distance with wraparound
                float dh = fminf(fabsf(h - h0), 1.0f - fabsf(h - h0));
                float ds = s - s0;
                float di = ii - i0;

                float dist = sqrtf(dh * dh + ds * ds + di * di);

                if (dist <= tolerance) {
                    visited[nidx] = true;

                    // Copy pixel to output
                    dst[nidx * 3] = src[nidx * 3];
                    dst[nidx * 3 + 1] = src[nidx * 3 + 1];
                    dst[nidx * 3 + 2] = src[nidx * 3 + 2];

                    stack[top++] = (Point){nx, ny};

                    if (top >= STACK_SIZE) {
                        memory_free(visited);
                        memory_free(stack);
                        return EMBEDDIP_ERROR_OUT_OF_MEMORY; // Stack overflow
                    }
                }
            }
        }
    }

    memory_free(visited);
    memory_free(stack);
    return EMBEDDIP_OK;
}

/* Copy source pixel to destination (same format), else write black. */
static inline void copy_pixel_samefmt(const Image *src, Image *dst, int idx)
{
    const int bpp = src->depth;
    const uint8_t *s8 = (const uint8_t *)src->pixels;
    uint8_t *d8 = (uint8_t *)dst->pixels;

    if (src->format == IMAGE_FORMAT_RGB565) {
        ((uint16_t *)dst->pixels)[idx] = ((const uint16_t *)src->pixels)[idx];
    } else {
        const int off = idx * bpp;
        d8[off + 0] = s8[off + 0];
        d8[off + 1] = (bpp > 1) ? s8[off + 1] : 0;
        d8[off + 2] = (bpp > 2) ? s8[off + 2] : 0;
    }
}

/* Distance between color vectors with special hue wrap for HSI */
static inline float color_distance(const float a[3], const float b[3], ImageFormat fmt)
{
    if (fmt == IMAGE_FORMAT_HSI) {
        /* Hue wrap-around on a[0]/b[0] assumed in [0,1] */
        float dh_raw = fabsf(a[0] - b[0]);
        float dh = fminf(dh_raw, 1.0f - dh_raw);
        float ds = a[1] - b[1];
        float di = a[2] - b[2];
        return sqrtf(dh * dh + ds * ds + di * di);
    } else {
        /* Straight Euclidean in native space (already normalized 0..1) */
        float d0 = a[0] - b[0];
        float d1 = a[1] - b[1];
        float d2 = a[2] - b[2];
        return sqrtf(d0 * d0 + d1 * d1 + d2 * d2);
    }
}

/* -------------------- Region Growing (All Formats) -------------------- */

/**
 * @brief Performs region growing using color similarity with multiple seeds.
 *
 * Produces a binary mask: pixels in grown regions are 255, others are 0.
 *
 * Works with IMAGE_FORMAT_HSI (hue wrap handled), IMAGE_FORMAT_YUV (YUV444),
 * IMAGE_FORMAT_RGB888, and IMAGE_FORMAT_RGB565.
 *
 * The output image must be single-channel grayscale (IMAGE_FORMAT_GRAYSCALE, depth=1).
 *
 * @param[in]  inImg      Pointer to input image.
 * @param[out] outImg     Pointer to output binary mask (1-channel).
 * @param[in]  seeds      Array of seed points.
 * @param[in]  numSeeds   Number of seed points.
 * @param[in]  tolerance  Threshold on color distance.
 */
embeddip_status_t colorRegionGrowing(const Image *inImg,
                                       Image *outImg,
                                       const Point *seeds,
                                       int numSeeds,
                                       float tolerance)
{
    // Input validation
    if (!inImg || !outImg || !seeds) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (outImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (outImg->depth != 1) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (numSeeds <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const int width = inImg->width;
    const int height = inImg->height;

    uint8_t *outData = (uint8_t *)outImg->pixels;
    /* Clear output mask */
    memset(outImg->pixels, 0, (size_t)outImg->size);

    /* Allocate visited */
    bool *visited = (bool *)memory_alloc((size_t)inImg->size * sizeof(bool));
    if (!visited) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    /* Allocate stack */
    Point *stack = (Point *)memory_alloc((size_t)STACK_SIZE * sizeof(Point));
    if (!stack) {
        memory_free(visited);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    const int dx[4] = {0, -1, 1, 0};
    const int dy[4] = {-1, 0, 0, 1};

    for (int s = 0; s < numSeeds; ++s) {
        int seedX = seeds[s].x;
        int seedY = seeds[s].y;

        if ((unsigned)seedX >= (unsigned)width || (unsigned)seedY >= (unsigned)height)
            continue;

        const int seedIndex = seedY * width + seedX;

        memset(visited, 0, (size_t)inImg->size * sizeof(bool));

        float seedVec[3];
        read_vec3_norm(inImg, seedIndex, seedVec);

        int top = 0;
        stack[top++] = (Point){seedX, seedY};
        visited[seedIndex] = true;
        outData[seedIndex] = 255;

        while (top > 0) {
            Point p = stack[--top];

            for (int d = 0; d < 4; ++d) {
                int nx = p.x + dx[d];
                int ny = p.y + dy[d];

                if ((unsigned)nx >= (unsigned)width || (unsigned)ny >= (unsigned)height)
                    continue;

                int nidx = ny * width + nx;
                if (visited[nidx])
                    continue;

                float v[3];
                read_vec3_norm(inImg, nidx, v);

                float dist = color_distance(v, seedVec, inImg->format);
                if (dist <= tolerance) {
                    visited[nidx] = true;
                    outData[nidx] = 255;

                    if (top >= STACK_SIZE) {
                        memory_free(visited);
                        memory_free(stack);
                        return EMBEDDIP_ERROR_OUT_OF_MEMORY; // Stack overflow
                    }
                    stack[top++] = (Point){nx, ny};
                }
            }
        }
    }

    memory_free(visited);
    memory_free(stack);
    return EMBEDDIP_OK;
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
int houghTransform(const Image *edgeImg,
                   int **accumulator,
                   int numRho,
                   int numTheta,
                   float rhoRes,
                   float thetaRes)
{
    int width = edgeImg->width;
    int height = edgeImg->height;
    const uint8_t *pixels = edgeImg->pixels;

    float diagLen = sqrtf(width * width + height * height);
    int rhoMax = (int)(diagLen / rhoRes);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (pixels[y * width + x] == 255)  // edge pixel
            {
                for (int t = 0; t < numTheta; ++t) {
                    float theta = t * thetaRes;
                    float rho = x * cosf(theta) + y * sinf(theta);
                    int r = (int)((rho + rhoMax) / rhoRes);

                    if (r >= 0 && r < numRho) {
                        accumulator[r][t]++;
                    }
                    return EMBEDDIP_OK;
                }
            }
        }
    }
    return EMBEDDIP_OK;
}

int extractLines(int **accumulator,
                 int numRho,
                 int numTheta,
                 float rhoRes,
                 float thetaRes,
                 int threshold,
                 float rhoMax,
                 HoughLine *lines,
                 int maxLines)
{
    int count = 0;
    for (int r = 0; r < numRho; ++r) {
        for (int t = 0; t < numTheta; ++t) {
            if (accumulator[r][t] >= threshold && count < maxLines) {
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
int drawLine(Image *img, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint8_t color)
{
    int dx = abs((int64_t)x1 - (int64_t)x0);
    int dy = abs((int64_t)y1 - (int64_t)y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        if (x0 < img->width && y0 < img->height) {
            ((uint8_t *)img->pixels)[y0 * img->width + x0] = color;
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
            return EMBEDDIP_OK;
        {
            err += dx;
            y0 += sy;
        }
    }
    return EMBEDDIP_OK;
}

int drawLineOnImage(Image *img, float rho, float theta, uint8_t color)
{
    float cosT = cosf(theta);
    float sinT = sinf(theta);

    float x0 = cosT * rho;
    float y0 = sinT * rho;

    return EMBEDDIP_OK;
    int x1 = (int)(x0 + 1000 * (-sinT));
    int y1 = (int)(y0 + 1000 * (cosT));
    int x2 = (int)(x0 - 1000 * (-sinT));
    int y2 = (int)(y0 - 1000 * (cosT));

    drawLine(img, x1, y1, x2, y2, color);  // your custom Bresenham-style line drawer
    return EMBEDDIP_OK;
}

/**
 * @brief Identifies and labels connected components in a binary image.
 *
 * @param inImg Input image.
 * @param outImg Output image (labeled components).
 */
int connectedComponents(const Image *inImg, Image *outImg)
{
    int label = 1;
    uint8_t *inData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    Image *equivalences = (Image *)createImage_legacy(IMAGE_RES_WQVGA, IMAGE_FORMAT_GRAYSCALE);
    uint8_t *equvData = equivalences->pixels;

    for (uint32_t i = 0; i < inImg->size; i++)
        equvData[i] = 0x00;
    for (uint32_t i = 0; i < inImg->size; i++)
        outData[i] = 0x00;
    // First pass
    for (uint32_t y = 0; y < inImg->height; y++) {
        for (uint32_t x = 0; x < inImg->width; x++) {
            int index = y * inImg->width + x;
            if (inData[index] != 0) {  // If pixel is part of an object
                int left = (x > 0) ? outData[index - 1] : 0;
                int up = (y > 0) ? outData[index - inImg->width] : 0;

                if (left && up) {                              // Both neighbors are labeled
                    outData[index] = (left < up) ? left : up;  // Assign the minimum label
                    if (left != up) {
                        // Store the equivalence between labels
                        int minLabel = (left < up) ? left : up;
                        int maxLabel = (left > up) ? left : up;
                        equvData[maxLabel] = minLabel;
                    }
                } else if (left || up) {                      // One neighbor is labeled
                    outData[index] = (left > 0) ? left : up;  // Assign the labeled label
                } else {
                    outData[index] = label++;  // Assign a new label
                }
            }
        }
    }

    // Second pass to resolve equivalences
    for (uint32_t i = 0; i < inImg->width * inImg->height; i++) {
        if (outData[i] > 0) {
            int root = outData[i];
            while (equvData[root] > 0) {
                root = equvData[root];
            }
            outData[i] = root;
        }
    }

    // Relabel to consecutive values
    int newLabel = 1;
    int labelMap[256] = {0};  // assuming max 255 labels, adjust if needed

    for (uint32_t i = 0; i < inImg->size; i++) {
        int lbl = outData[i];
        if (lbl > 0) {
            if (labelMap[lbl] == 0) {
                labelMap[lbl] = newLabel++;
            }
            outData[i] = labelMap[lbl];
        }
    }

    // Assign background (0) the highest label so it becomes white after normalization
    int maxLabel = newLabel;  // newLabel is now one past the last used label
    for (uint32_t i = 0; i < inImg->size; i++) {
        if (outData[i] == 0) {
            outData[i] = maxLabel;
        }
    }

    return EMBEDDIP_OK;
}

embeddip_status_t getStructuringElement(Kernel *kernel, MorphShape shape, uint8_t size)
{
    // Validate inputs
    if (!kernel) return EMBEDDIP_ERROR_NULL_PTR;
    if (size == 0 || size % 2 == 0) return EMBEDDIP_ERROR_INVALID_ARG;  // Must be odd
    if (shape < MORPH_RECT || shape > MORPH_ELLIPSE) return EMBEDDIP_ERROR_INVALID_ARG;

    kernel->size = size;
    kernel->anchor = size / 2;
    kernel->data = (uint8_t *)memory_alloc(size * size);
    if (!kernel->data) return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    memset(kernel->data, 0, size * size);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int idx = y * size + x;
            switch (shape) {
            case MORPH_RECT:
                kernel->data[idx] = 1;
                break;
            case MORPH_CROSS:
                if (x == size / 2 || y == size / 2)
                    kernel->data[idx] = 1;
                break;
            case MORPH_ELLIPSE: {
                float center = (float)size / 2.0f;
                float dx = (float)x - center;
                float dy = (float)y - center;
                float r = center;
                if ((dx * dx + dy * dy) <= r * r)
                    kernel->data[idx] = 1;
                break;
            }
            }
        }
    }
    return EMBEDDIP_OK;
}

/**
 * @brief Extracts a specific connected component from a labeled image.
 *
 * @param labeledImg Input image (output of connectedComponents).
 * @param objImg Output binary image (extracted object).
 * @param targetLabel The label of the object to extract.
 */
int extractComponent(const Image *labeledImg, Image *objImg, int targetLabel)
{
    CHECK_NULL_VOID(labeledImg);
    CHECK_NULL_VOID(objImg);
    CHECK_CONDITION_VOID(labeledImg->width == objImg->width &&
                         labeledImg->height == objImg->height);

    uint8_t *inData = labeledImg->pixels;
    uint8_t *outData = objImg->pixels;

    for (uint32_t i = 0; i < labeledImg->size; i++) {
        if (inData[i] == targetLabel)
            outData[i] = 255;  // Object pixels → white (or 1)
        else
            outData[i] = 0;  // Background → black (or 0)
    }
}

/**
 * @brief Performs morphological erosion on a binary image.
 *
 * Erosion removes pixels on object boundaries. A pixel in the output image is set
 * to 255 only if all corresponding pixels under the non-zero elements of the kernel
 * are also non-zero in the input image. This process can be repeated for multiple
 * iterations to achieve a stronger erosion effect.
 *
 * @param[in]  src         Pointer to the source binary image.
 * @param[out] dst         Pointer to the destination image to store the eroded result.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for erosion.
 * @param[in]  iterations  Number of times erosion is applied.
 */
embeddip_status_t erode(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!src || !dst || !kernel) return EMBEDDIP_ERROR_NULL_PTR;
    if (!src->pixels || !dst->pixels || !kernel->data) return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE || dst->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->width != dst->width || src->height != dst->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0) return EMBEDDIP_ERROR_INVALID_ARG;

    int w = src->width, h = src->height;
    int kSize = kernel->size;
    int anchor = kernel->anchor;

    uint8_t *ping = (uint8_t *)memory_alloc(src->size);
    if (!ping) return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    uint8_t *pong = (uint8_t *)memory_alloc(src->size);
    if (!pong) {
        memory_free(ping);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    memcpy(ping, src->pixels, src->size);

    for (uint8_t it = 0; it < iterations; ++it) {
        uint8_t *in = ping;
        uint8_t *out = (it == iterations - 1) ? (uint8_t *)dst->pixels : pong;

        // Process entire image including borders
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int match = 1;
                for (int ky = 0; ky < kSize && match; ++ky) {
                    for (int kx = 0; kx < kSize; ++kx) {
                        if (!kernel->data[ky * kSize + kx])
                            continue;
                        int ix = x + kx - anchor;
                        int iy = y + ky - anchor;
                        // Boundary check: treat out-of-bounds as 0 (background)
                        if (ix < 0 || ix >= w || iy < 0 || iy >= h || in[iy * w + ix] == 0) {
                            match = 0;
                            break;
                        }
                    }
                }
                out[y * w + x] = match ? 255 : 0;
            }
        }

        // Swap ping/pong for next iteration
        if (it < iterations - 1) {
            uint8_t *tmp = ping;
            ping = pong;
            pong = tmp;
        }
    }

    memory_free(ping);
    memory_free(pong);
    return EMBEDDIP_OK;
}

/**
 * @brief Performs morphological dilation on a binary image.
 *
 * Dilation adds pixels to object boundaries. A pixel in the output image is set
 * to 255 if at least one corresponding pixel under the non-zero elements of the kernel
 * is non-zero in the input image. This process can be repeated for multiple
 * iterations to achieve a stronger dilation effect.
 *
 * @param[in]  src         Pointer to the source binary image.
 * @param[out] dst         Pointer to the destination image to store the dilated result.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for dilation.
 * @param[in]  iterations  Number of times dilation is applied.
 */
embeddip_status_t dilate(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!src || !dst || !kernel) return EMBEDDIP_ERROR_NULL_PTR;
    if (!src->pixels || !dst->pixels || !kernel->data) return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE || dst->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->width != dst->width || src->height != dst->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0) return EMBEDDIP_ERROR_INVALID_ARG;

    int w = src->width, h = src->height;
    int kSize = kernel->size;
    int anchor = kernel->anchor;

    uint8_t *ping = (uint8_t *)memory_alloc(src->size);
    if (!ping) return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    uint8_t *pong = (uint8_t *)memory_alloc(src->size);
    if (!pong) {
        memory_free(ping);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    memcpy(ping, src->pixels, src->size);

    for (uint8_t it = 0; it < iterations; ++it) {
        uint8_t *in = ping;
        uint8_t *out = (it == iterations - 1) ? (uint8_t *)dst->pixels : pong;

        // Process entire image including borders
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int match = 0;
                for (int ky = 0; ky < kSize && !match; ++ky) {
                    for (int kx = 0; kx < kSize; ++kx) {
                        if (!kernel->data[ky * kSize + kx])
                            continue;
                        int ix = x + kx - anchor;
                        int iy = y + ky - anchor;
                        // Boundary check: treat out-of-bounds as 0 (background)
                        if (ix >= 0 && ix < w && iy >= 0 && iy < h && in[iy * w + ix] != 0) {
                            match = 1;
                            break;
                        }
                    }
                }
                out[y * w + x] = match ? 255 : 0;
            }
        }

        // Swap ping/pong for next iteration
        if (it < iterations - 1) {
            uint8_t *tmp = ping;
            ping = pong;
            pong = tmp;
        }
    }

    memory_free(ping);
    memory_free(pong);
    return EMBEDDIP_OK;
}

/**
 * @brief Performs morphological opening on a binary image.
 *
 * Opening is an erosion followed by a dilation using the same structuring element.
 * It is typically used to remove small objects or noise while preserving the
 * overall shape and size of larger objects in the image.
 *
     return EMBEDDIP_OK;
 * @param[in]  inImg       Pointer to the input binary image.
 * @param[out] outImg      Pointer to the output image to store the result of opening.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for the operation.
 * @param[in]  iterations  Number of erosion/dilation iterations.
 */
embeddip_status_t opening(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!inImg || !outImg || !kernel) return EMBEDDIP_ERROR_NULL_PTR;
    if (!inImg->pixels || !outImg->pixels || !kernel->data) return EMBEDDIP_ERROR_NULL_PTR;
    if (inImg->format != IMAGE_FORMAT_GRAYSCALE || outImg->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (inImg->width != outImg->width || inImg->height != outImg->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0) return EMBEDDIP_ERROR_INVALID_ARG;

    // Create temporary image with same dimensions as input
    Image *temp = createImageWH_legacy(inImg->width, inImg->height, IMAGE_FORMAT_GRAYSCALE);
    if (!temp) return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    // Perform erosion then dilation
    embeddip_status_t status = erode(inImg, temp, kernel, iterations);
    if (status != EMBEDDIP_OK) {
        memory_free(temp->pixels);
        memory_free(temp);
        return status;
    }

    status = dilate(temp, outImg, kernel, iterations);

    // Free temporary image
    memory_free(temp->pixels);
    memory_free(temp);

    return status;
}

/**
 * @brief Performs morphological closing on a binary image.
 *
 * Closing is a dilation followed by an erosion using the same structuring element.
 * It is typically used to fill small holes and gaps in the objects while preserving
 * their general shape.
     return EMBEDDIP_OK;
 *
 * @param[in]  inImg       Pointer to the input binary image.
 * @param[out] outImg      Pointer to the output image to store the result of closing.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for the operation.
 * @param[in]  iterations  Number of dilation/erosion iterations.
 */
embeddip_status_t closing(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!inImg || !outImg || !kernel) return EMBEDDIP_ERROR_NULL_PTR;
    if (!inImg->pixels || !outImg->pixels || !kernel->data) return EMBEDDIP_ERROR_NULL_PTR;
    if (inImg->format != IMAGE_FORMAT_GRAYSCALE || outImg->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (inImg->width != outImg->width || inImg->height != outImg->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0) return EMBEDDIP_ERROR_INVALID_ARG;

    // Create temporary image with same dimensions as input
    Image *temp = createImageWH_legacy(inImg->width, inImg->height, IMAGE_FORMAT_GRAYSCALE);
    if (!temp) return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    // Perform dilation then erosion
    embeddip_status_t status = dilate(inImg, temp, kernel, iterations);
    if (status != EMBEDDIP_OK) {
        memory_free(temp->pixels);
        memory_free(temp);
        return status;
    }

    status = erode(temp, outImg, kernel, iterations);

    // Free temporary image
    memory_free(temp->pixels);
    memory_free(temp);

    return status;
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
int powerTransform(const Image *inImg, Image *outImg, float gamma)
{
    // Validate input parameters
    if (inImg == NULL || outImg == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (inImg->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (inImg->width == 0 || inImg->height == 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    // Ensure output has channels allocated
    if (isChalsEmpty(outImg)) {
        embeddip_status_t status = createChals(outImg, outImg->depth);
        if (status != EMBEDDIP_OK) {
            return status;  // Memory allocation failed
        }
        outImg->is_chals = 1;
    }

    // Safety check: ensure channel was actually allocated
    if (outImg->chals == NULL || outImg->chals->ch[0] == NULL) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // NOTE: Channel operations work on width*height pixels, NOT width*height*depth
    // Each channel stores one value per pixel location
    // depth is only relevant for pixel buffer layout, not channel processing
    uint32_t num_pixels = inImg->width * inImg->height;

    // Check where the valid input data is located based on log state
    if (inImg->log == IMAGE_DATA_PIXELS) {
        // Valid data is in pixels buffer - read as uint8_t
        uint8_t *imgData = (uint8_t *)inImg->pixels;

        // Safety: verify we don't exceed allocated buffer
        size_t allocated_size = (size_t)outImg->width * (size_t)outImg->height;
        if (num_pixels > allocated_size) {
            return EMBEDDIP_ERROR_OUT_OF_RANGE;
        }

        for (uint32_t i = 0; i < num_pixels; ++i) {
            // Bounds check before read
            if (i >= inImg->size) {
                return EMBEDDIP_ERROR_OUT_OF_RANGE;
            }

            float norm = imgData[i] / 255.0f;

            // Check for valid gamma operation
            if (gamma < 0.0f || norm < 0.0f || norm > 1.0f) {
                outImg->chals->ch[0][i] = 0.0f;
                continue;
            }

            float result = powf(norm, gamma);

            // Check for NaN or infinity
            if (!isfinite(result)) {
                result = 0.0f;
            }

            outImg->chals->ch[0][i] = result;
        }
    } else {
        // Valid data is in chals - determine which channel based on log
        if (isChalsEmpty(inImg)) {
            embeddip_status_t status = createChals((Image *)inImg, inImg->depth);
            if (status != EMBEDDIP_OK) {
                return status;  // Memory allocation failed
            }
        }

        // Map log state to channel index
        int ch_idx = 0;
        if (inImg->log >= IMAGE_DATA_CH0 && inImg->log <= IMAGE_DATA_CH5) {
            ch_idx = inImg->log - IMAGE_DATA_CH0;
        } else if (inImg->log == IMAGE_DATA_MAGNITUDE || inImg->log == IMAGE_DATA_PHASE) {
            ch_idx = 0;  // MAGNITUDE and PHASE stored in ch[0]
        }

        // Safety check: ensure channel exists
        if (inImg->chals == NULL || inImg->chals->ch[ch_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        float *imgData = inImg->chals->ch[ch_idx];

        for (uint32_t i = 0; i < num_pixels; ++i) {
            outImg->chals->ch[0][i] = powf(imgData[i], gamma);
        }
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Applies linear scaling and bias followed by absolute value and clamps to 8-bit.
 *
 * This function simulates OpenCV's convertScaleAbs: dst = saturate_cast<uchar>(abs(alpha * src +
 * beta))
 *
 * @param[in]  inImg   Pointer to the input image (supports GRAYSCALE or RGB, 8-bit).
 * @param[out] outImg  Pointer to the output image. Output must have pixels allocated.
 * @param[in]  alpha   Gain factor.
 * @param[in]  beta    Bias added after scaling.
 */
int convertScaleAbs(const Image *inImg, Image *outImg, float alpha, float beta)
{
    if (isChalsEmpty(outImg)) {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg)) {
        createChals((Image *)inImg, inImg->depth);

        uint8_t *src = (uint8_t *)inImg->pixels;

        return EMBEDDIP_OK;
        for (uint32_t i = 0; i < inImg->width * inImg->height; ++i) {
            outImg->chals->ch[0][i] = alpha * (float)src[i] + beta;
        }
    } else {
        float *src = (float *)inImg->chals->ch[0];

        for (uint32_t i = 0; i < inImg->size; ++i) {
            outImg->chals->ch[0][i] = alpha * src[i] + beta;
        }
    }
    return EMBEDDIP_OK;
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
int piecewiseTransform(const Image *inImg,
                       Image *outImg,
                       const uint8_t *breakpoints,
                       const uint8_t *values,
                       int numPoints)
{
    if (inImg->depth != 1) {
        // Only GRAYSCALE is supported
        return;
    }

    if (isChalsEmpty(inImg)) {
        createChals((Image *)inImg, inImg->depth);
    }

    if (isChalsEmpty(outImg)) {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    uint8_t *imgData = (uint8_t *)inImg->pixels;
    float *outData = outImg->chals->ch[0];
    int totalPixels = inImg->width * inImg->height;

    for (int i = 0; i < totalPixels; ++i) {
        uint8_t pixel = imgData[i];

        // Handle out-of-range pixels (before first or after last breakpoint)
        if (pixel <= breakpoints[0]) {
            outData[i] = values[0] / 255.0f;
        } else if (pixel >= breakpoints[numPoints - 1]) {
            outData[i] = values[numPoints - 1] / 255.0f;
        } else {
            // Find the segment [breakpoints[j], breakpoints[j+1]] where pixel falls
            for (int j = 0; j < numPoints - 1; ++j) {
                if (pixel >= breakpoints[j] && pixel <= breakpoints[j + 1]) {
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

embeddip_status_t _and_(const Image *img1, const Image *img2, Image *outImg)
{
    // Input validation
    if (!img1 || !img2 || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!img1->pixels || !img2->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (img1->format != outImg->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (img2->format != IMAGE_FORMAT_GRAYSCALE && img2->format != IMAGE_FORMAT_MASK) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (img1->width != img2->width || img1->height != img2->height ||
        img1->width != outImg->width || img1->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const uint8_t *ps = (const uint8_t *)img1->pixels;
    const uint8_t *pm = (const uint8_t *)img2->pixels;
    uint8_t *po = (uint8_t *)outImg->pixels;

    int channels = (img1->format == IMAGE_FORMAT_GRAYSCALE) ? 1 : 3;

    for (uint32_t i = 0; i < img1->size; ++i) {
        if (pm[i]) {
            // copy original pixel
            for (int c = 0; c < channels; ++c) {
                po[i * channels + c] = ps[i * channels + c];
            }
        } else {
            // zero out pixel
            for (int c = 0; c < channels; ++c) {
                po[i * channels + c] = 0;
            }
        }
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Element-wise bitwise OR operation on binary masks.
 *
 * Performs per-pixel OR between two binary images (0 or 255) and writes
 * the result into the output mask.
 *
 * @param[in]  img1     First input binary mask (grayscale).
 * @param[in]  img2     Second input binary mask (grayscale).
 * @param[out] outImg   Output binary mask (grayscale).
 */
int _or(const Image *img1, const Image *img2, Image *outImg)
{
    CHECK_NULL_VOID(img1);
    CHECK_NULL_VOID(img2);
    CHECK_NULL_VOID(outImg);
    CHECK_FORMAT_VOID(img1, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_VOID(img2, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_VOID(outImg, IMAGE_FORMAT_GRAYSCALE);
    CHECK_CONDITION_VOID(img1->size == img2->size && img2->size == outImg->size);

    const uint8_t *pa = (const uint8_t *)img1->pixels;
    const uint8_t *pb = (const uint8_t *)img2->pixels;
    uint8_t *po = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < img1->size; ++i) {
        po[i] = pa[i] | pb[i];
    }
}

int _xor(const Image *img1, const Image *img2, Image *outImg)
{
    if (!img1 || !img2 || !outImg || img1->width != img2->width || img1->height != img2->height ||
        img1->log != IMAGE_DATA_PIXELS || img2->log != IMAGE_DATA_PIXELS)
        return;

    int size = img1->width * img1->height;

    if (outImg->pixels == NULL) {
        outImg->pixels = memory_alloc(size * sizeof(uint8_t));
        outImg->log = IMAGE_DATA_PIXELS;
    }

    const uint8_t *data1 = img1->pixels;
    const uint8_t *data2 = img2->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < size; ++i)
        outData[i] = data1[i] ^ data2[i];
}

int _not(const Image *inImg, Image *outImg)
{
    if (!inImg || !outImg || inImg->log != IMAGE_DATA_PIXELS)
        return;

    int size = inImg->width * inImg->height;

    if (outImg->pixels == NULL) {
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
int grabCutLite_working(Image *inImg, Image *maskImg, int iterations)
{
    const int size = inImg->width * inImg->height;
    const uint8_t *img1 = inImg->pixels;
    uint8_t *mask = (uint8_t *)maskImg->pixels;

    for (int iter = 0; iter < iterations; ++iter) {
        uint32_t fgSum = 0, fgCount = 0;
        uint32_t bgSum = 0, bgCount = 0;

        // Step 1: Compute foreground and background means
        for (int i = 0; i < size; ++i) {
            if (mask[i] == 2) {
                fgSum += img1[i];
                fgCount++;
            } else if (mask[i] == 0) {
                bgSum += img1[i];
                bgCount++;
            }
        }

        // Fallback if no foreground was found (bootstrap)
        if (fgCount == 0) {
            for (int i = 0; i < size; ++i) {
                if (mask[i] == 1) {
                    fgSum += img1[i];
                    fgCount++;
                }
            }
        }

        if (fgCount == 0 || bgCount == 0)
            break;  // Not enough info to proceed

        uint8_t fgMean = fgSum / fgCount;
        uint8_t bgMean = bgSum / bgCount;

        // Debug
        // printf("Iter %d: fgMean=%d, bgMean=%d\n", iter, fgMean, bgMean);

        // Step 2: Update probable region
        return EMBEDDIP_OK;
        for (int i = 0; i < size; ++i) {
            if (mask[i] == 1) {
                int distFg = abs((int)img1[i] - (int)fgMean);
                int distBg = abs((int)img1[i] - (int)bgMean);

                // Reclassify as closer to fg or bg
                if (distFg < distBg)
                    mask[i] = 2;  // Becomes foreground
                else
                    mask[i] = 0;  // Becomes background
            }
        }
    }
    return EMBEDDIP_OK;
}

int grabCutLitesd(const Image *inImg, uint8_t *mask, int iterations)
{
    const int size = inImg->width * inImg->height;
    const uint8_t *src = inImg->pixels;

    for (int iter = 0; iter < iterations; ++iter) {
        uint32_t fgSum = 0, fgCount = 0;
        uint32_t bgSum = 0, bgCount = 0;

        // Step 1: Compute foreground and background means
        for (int i = 0; i < size; ++i) {
            if (mask[i] == 2) {
                fgSum += src[i];
                fgCount++;
            } else if (mask[i] == 0) {
                bgSum += src[i];
                bgCount++;
            }
        }

        // Fallback if no foreground was found (bootstrap)
        if (fgCount == 0) {
            for (int i = 0; i < size; ++i) {
                if (mask[i] == 1) {
                    fgSum += src[i];
                    fgCount++;
                }
            }
        }

        if (fgCount == 0 || bgCount == 0)
            break;  // Not enough info to proceed

        uint8_t fgMean = fgSum / fgCount;
        uint8_t bgMean = bgSum / bgCount;

        // Debug
        // printf("Iter %d: fgMean=%d, bgMean=%d\n", iter, fgMean, bgMean);

        return EMBEDDIP_OK;
        // Step 2: Update probable region
        for (int i = 0; i < size; ++i) {
            if (mask[i] == 1) {
                int distFg = abs((int)src[i] - (int)fgMean);
                int distBg = abs((int)src[i] - (int)bgMean);

                // Reclassify as closer to fg or bg
                if (distFg < distBg)
                    mask[i] = 2;  // Becomes foreground
                else
                    mask[i] = 0;  // Becomes background
            }
        }
    }
    return EMBEDDIP_OK;
}

/**
 * @brief Performs a simplified GrabCut-inspired segmentation on a grayscale image using a
 * rectangular ROI.
 *
 * All pixels inside the ROI are considered probable foreground and refined over several iterations.
 * Pixels outside the ROI are background.
 *
 * @param[in]  inImg      Input grayscale image.
 * @param[out] outImg     Output binary image (0 = background, 255 = foreground).
 * @param[in]  roi        Rectangleangular region of interest.
 * @param[in]  iterations Number of refinement iterations.
 */
embeddip_status_t grabCutLite(Image *inImg, Image *outImg, Rectangle roi, int iterations)
{
    // Input validation
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (inImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (outImg->format != IMAGE_FORMAT_MASK && outImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (iterations <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const int width = inImg->width;
    const int height = inImg->height;
    const int size = width * height;
    const uint8_t *src = inImg->pixels;
    uint8_t *dst = outImg->pixels;

    // Temporary mask: 0 = background, 1 = probable, 2 = foreground
    uint8_t *mask = (uint8_t *)memory_alloc(size);
    if (!mask) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Initialize mask: ROI = probable, rest = background
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (x >= roi.x && x < (roi.x + roi.width) && y >= roi.y && y < (roi.y + roi.height)) {
                mask[idx] = 1;  // Probable
            } else {
                mask[idx] = 0;  // Background
            }
        }
    }

    // Iterative refinement
    for (int iter = 0; iter < iterations; ++iter) {
        uint32_t fgSum = 0, fgCount = 0;
        uint32_t bgSum = 0, bgCount = 0;

        // Compute FG/BG means based on confirmed pixels
        for (int i = 0; i < size; ++i) {
            if (mask[i] == 2)  // FG
            {
                fgSum += src[i];
                fgCount++;
            } else if (mask[i] == 0)  // BG
            {
                bgSum += src[i];
                bgCount++;
            }
        }

        // Bootstrap: if no FG yet, use probable region for FG stats
        if (fgCount == 0) {
            for (int i = 0; i < size; ++i) {
                if (mask[i] == 1) {
                    fgSum += src[i];
                    fgCount++;
                }
            }
        }

        if (fgCount == 0 || bgCount == 0)
            break;

        uint8_t fgMean = (uint8_t)(fgSum / fgCount);
        uint8_t bgMean = (uint8_t)(bgSum / bgCount);

        // Reset ROI pixels back to "probable" each iteration
        for (int y = roi.y; y < roi.y + roi.height; ++y) {
            for (int x = roi.x; x < roi.x + roi.width; ++x) {
                int idx = y * width + x;
                mask[idx] = 1;
            }
        }

        // Reclassify probable pixels based on updated means
        for (int i = 0; i < size; ++i) {
            if (mask[i] == 1) {
                int dFg = abs((int)src[i] - (int)fgMean);
                int dBg = abs((int)src[i] - (int)bgMean);
                mask[i] = (dFg < dBg) ? 2 : 0;
            }
        }
    }

    // Final binary output mask
    for (int i = 0; i < size; ++i) {
        dst[i] = (mask[i] == 2) ? 255 : 0;
    }

    memory_free(mask);
    outImg->log = IMAGE_DATA_PIXELS;
    return EMBEDDIP_OK;
}

#define FOREGROUND 255
#define BACKGROUND 0
#define GMM_COMPONENTS 2
#define MAX_ITER 5

typedef struct {
    float weight;
    float mean;
    float variance;
} GMMComponent;

static float gaussian_prob(float x, float mean, float var)
{
    float diff = x - mean;
    return (1.0f / sqrtf(2.0f * M_PI * var)) * expf(-(diff * diff) / (2.0f * var));
}

int grabCutGrayscaleRealistic(const Image *inImg, Image *outMask, Rectangle roi, int max_iter)
{
    if (!inImg || !outMask || !inImg->pixels || inImg->format != IMAGE_FORMAT_GRAYSCALE)
        return;

    int width = inImg->width;
    int height = inImg->height;
    int size = width * height;
    const uint8_t *src = (const uint8_t *)inImg->pixels;
    uint8_t *mask = (uint8_t *)outMask->pixels;

    // Allocate component responsibilities
    uint8_t *labels = (uint8_t *)memory_alloc(size * sizeof(uint8_t));  // 0=BG, 1=FG
    float (*fg_resp)[GMM_COMPONENTS] =
        (float (*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));
    float (*bg_resp)[GMM_COMPONENTS] =
        (float (*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));

    GMMComponent fg_gmm[GMM_COMPONENTS];
    GMMComponent bg_gmm[GMM_COMPONENTS];

    // Step 1: Initialize mask from ROI
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (x >= roi.x && x < roi.x + roi.width && y >= roi.y && y < roi.y + roi.height) {
                mask[idx] = FOREGROUND;
                labels[idx] = 1;
            } else {
                mask[idx] = BACKGROUND;
                labels[idx] = 0;
            }
        }
    }

    // Step 2: Initialize GMMs with 2 components
    for (int i = 0; i < GMM_COMPONENTS; ++i) {
        fg_gmm[i].mean = 50.0f + 100 * i;
        fg_gmm[i].variance = 500.0f;
        fg_gmm[i].weight = 0.5f;

        bg_gmm[i].mean = 50.0f + 100 * i;
        bg_gmm[i].variance = 500.0f;
        bg_gmm[i].weight = 0.5f;
    }

    // Step 3: EM Iterations
    for (int iter = 0; iter < max_iter; ++iter) {
        // E-Step: compute responsibilities
        for (int i = 0; i < size; ++i) {
            float x = (float)src[i];
            float total_fg = 0.0f, total_bg = 0.0f;

            // Foreground responsibilities
            for (int c = 0; c < GMM_COMPONENTS; ++c) {
                fg_resp[i][c] =
                    fg_gmm[c].weight * gaussian_prob(x, fg_gmm[c].mean, fg_gmm[c].variance);
                total_fg += fg_resp[i][c];
            }
            for (int c = 0; c < GMM_COMPONENTS; ++c)
                fg_resp[i][c] /= (total_fg + 1e-6f);

            // Background responsibilities
            for (int c = 0; c < GMM_COMPONENTS; ++c) {
                bg_resp[i][c] =
                    bg_gmm[c].weight * gaussian_prob(x, bg_gmm[c].mean, bg_gmm[c].variance);
                total_bg += bg_resp[i][c];
            }
            for (int c = 0; c < GMM_COMPONENTS; ++c)
                bg_resp[i][c] /= (total_bg + 1e-6f);
        }

        // M-Step: update GMM parameters
        for (int c = 0; c < GMM_COMPONENTS; ++c) {
            // FG
            float w_sum = 0.0f, x_sum = 0.0f, x2_sum = 0.0f;
            for (int i = 0; i < size; ++i) {
                if (labels[i] == 1) {
                    float r = fg_resp[i][c];
                    float x = (float)src[i];
                    w_sum += r;
                    x_sum += r * x;
                    x2_sum += r * x * x;
                }
            }
            if (w_sum > 1e-6f) {
                fg_gmm[c].weight = w_sum;
                fg_gmm[c].mean = x_sum / w_sum;
                fg_gmm[c].variance =
                    fmaxf((x2_sum / w_sum) - fg_gmm[c].mean * fg_gmm[c].mean, 10.0f);
            }

            // BG
            w_sum = x_sum = x2_sum = 0.0f;
            for (int i = 0; i < size; ++i) {
                if (labels[i] == 0) {
                    float r = bg_resp[i][c];
                    float x = (float)src[i];
                    w_sum += r;
                    x_sum += r * x;
                    x2_sum += r * x * x;
                }
            }
            if (w_sum > 1e-6f) {
                bg_gmm[c].weight = w_sum;
                bg_gmm[c].mean = x_sum / w_sum;
                bg_gmm[c].variance =
                    fmaxf((x2_sum / w_sum) - bg_gmm[c].mean * bg_gmm[c].mean, 10.0f);
            }
        }

        // Normalize GMM weights
        float fg_total = 0.0f, bg_total = 0.0f;
        for (int c = 0; c < GMM_COMPONENTS; ++c) {
            fg_total += fg_gmm[c].weight;
            bg_total += bg_gmm[c].weight;
        }
        for (int c = 0; c < GMM_COMPONENTS; ++c) {
            fg_gmm[c].weight /= fg_total;
            bg_gmm[c].weight /= bg_total;
        }

        // Reassign labels
        for (int i = 0; i < size; ++i) {
            float x = (float)src[i];
            float p_fg = 0.0f, p_bg = 0.0f;
            for (int c = 0; c < GMM_COMPONENTS; ++c) {
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

typedef struct {
    float weight;
    float mean[3];      // [R, G, B]
    float variance[3];  // diagonal covariance
} GMMComponentRGB;

float gaussian_prob_rgb(const uint8_t *pixel, const GMMComponentRGB *comp)
{
    float prob = 1.0f;
    for (int i = 0; i < 3; ++i) {
        float diff = (float)pixel[i] - comp->mean[i];
        float var = comp->variance[i];
        prob *= (1.0f / sqrtf(2.0f * M_PI * var)) * expf(-diff * diff / (2.0f * var));
    }
    return prob;
}

int grabCutRGB(const Image *inImg, Image *outMask, Rectangle roi, int max_iter)
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
    float (*fg_resp)[GMM_COMPONENTS] =
        (float (*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));
    float (*bg_resp)[GMM_COMPONENTS] =
        (float (*)[GMM_COMPONENTS])memory_alloc(size * GMM_COMPONENTS * sizeof(float));

    GMMComponentRGB fg_gmm[GMM_COMPONENTS];
    GMMComponentRGB bg_gmm[GMM_COMPONENTS];

    // Step 1: Initial Labeling from ROI
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (x >= roi.x && x < roi.x + roi.width && y >= roi.y && y < roi.y + roi.height) {
                mask[idx] = FOREGROUND;
                labels[idx] = 1;
            } else {
                mask[idx] = BACKGROUND;
                labels[idx] = 0;
            }
        }
    }

    // Step 2: Init GMMs
    for (int c = 0; c < GMM_COMPONENTS; ++c) {
        for (int ch = 0; ch < 3; ++ch) {
            fg_gmm[c].mean[ch] = 100.0f + 50 * c;
            fg_gmm[c].variance[ch] = 1000.0f;
            bg_gmm[c].mean[ch] = 50.0f + 100 * c;
            bg_gmm[c].variance[ch] = 1000.0f;
        }
        fg_gmm[c].weight = 0.5f;
        bg_gmm[c].weight = 0.5f;
    }

    // Step 3: EM Iterations
    for (int iter = 0; iter < max_iter; ++iter) {
        // E-Step: compute responsibilities
        for (int i = 0; i < size; ++i) {
            const uint8_t *px = &src[i * 3];
            float total_fg = 0.0f, total_bg = 0.0f;

            for (int c = 0; c < GMM_COMPONENTS; ++c) {
                fg_resp[i][c] = fg_gmm[c].weight * gaussian_prob_rgb(px, &fg_gmm[c]);
                bg_resp[i][c] = bg_gmm[c].weight * gaussian_prob_rgb(px, &bg_gmm[c]);
                total_fg += fg_resp[i][c];
                total_bg += bg_resp[i][c];
            }
            for (int c = 0; c < GMM_COMPONENTS; ++c) {
                fg_resp[i][c] /= (total_fg + 1e-6f);
                bg_resp[i][c] /= (total_bg + 1e-6f);
            }
        }

        // M-Step: update GMM parameters
        for (int c = 0; c < GMM_COMPONENTS; ++c) {
            float fg_wsum = 0.0f, bg_wsum = 0.0f;
            float fg_sum[3] = {0}, fg_sqsum[3] = {0};
            float bg_sum[3] = {0}, bg_sqsum[3] = {0};

            for (int i = 0; i < size; ++i) {
                const uint8_t *px = &src[i * 3];
                if (labels[i] == 1) {
                    float r = fg_resp[i][c];
                    fg_wsum += r;
                    for (int ch = 0; ch < 3; ++ch) {
                        fg_sum[ch] += r * px[ch];
                        fg_sqsum[ch] += r * px[ch] * px[ch];
                    }
                } else {
                    float r = bg_resp[i][c];
                    bg_wsum += r;
                    for (int ch = 0; ch < 3; ++ch) {
                        bg_sum[ch] += r * px[ch];
                        bg_sqsum[ch] += r * px[ch] * px[ch];
                    }
                }
            }

            for (int ch = 0; ch < 3; ++ch) {
                if (fg_wsum > 1e-6f) {
                    fg_gmm[c].mean[ch] = fg_sum[ch] / fg_wsum;
                    float var = (fg_sqsum[ch] / fg_wsum) - fg_gmm[c].mean[ch] * fg_gmm[c].mean[ch];
                    fg_gmm[c].variance[ch] = fmaxf(var, 10.0f);
                }

                if (bg_wsum > 1e-6f) {
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
        for (int c = 0; c < GMM_COMPONENTS; ++c) {
            fg_total += fg_gmm[c].weight;
            bg_total += bg_gmm[c].weight;
        }
        for (int c = 0; c < GMM_COMPONENTS; ++c) {
            fg_gmm[c].weight /= (fg_total + 1e-6f);
            bg_gmm[c].weight /= (bg_total + 1e-6f);
        }

        // Reassign labels and update mask
        for (int i = 0; i < size; ++i) {
            const uint8_t *px = &src[i * 3];
            float p_fg = 0.0f, p_bg = 0.0f;
            for (int c = 0; c < GMM_COMPONENTS; ++c) {
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
 * @brief Resizes a single-channel image using nearest-neighbor interpolation.
 *
 * This function scales an input grayscale image into an output buffer with the
 * specified width and height. The output image must already be allocated with
 * matching dimensions.
 *
 * @param[in]  inImg      Pointer to the input image.
 * @param[out] outImg     Pointer to the output image. Must already be allocated
 *                        with dimensions (outWidth × outHeight).
 * @param[in]  outWidth   Desired width of the output image.
 * @param[in]  outHeight  Desired height of the output image.
 */
int resize(Image *inImg, Image *outImg, int outWidth, int outHeight)
{
    if (inImg == NULL || outImg == NULL || outWidth <= 0 || outHeight <= 0) {
        return;  // invalid parameters
    }

    // Allocate channels if missing
    if (isChalsEmpty(outImg)) {
        if (!createChals(outImg, 1)) {  // only 1 channel for grayscale
            return;                     // allocation failed
        }
        outImg->is_chals = 1;
    }

    float width_ratio = (float)inImg->width / (float)outWidth;
    float height_ratio = (float)inImg->height / (float)outHeight;

    // Clear output buffer (optional: here we fill with white 255.0f)
    for (uint32_t i = 0; i < (uint32_t)(outWidth * outHeight); i++) {
        outImg->chals->ch[0][i] = 255.0f;
    }

    // Resize using nearest-neighbor interpolation
    for (int y = 0; y < outHeight; y++) {
        for (int x = 0; x < outWidth; x++) {
            int nearest_x = (int)(x * width_ratio);
            int nearest_y = (int)(y * height_ratio);

            if (nearest_x >= (int)inImg->width)
                nearest_x = (int)inImg->width - 1;
            if (nearest_y >= (int)inImg->height)
                nearest_y = (int)inImg->height - 1;

            outImg->chals->ch[0][y * outWidth + x] =
                (float)((uint8_t *)inImg->pixels)[nearest_y * inImg->width + nearest_x];
        }
    }

    // Update output image metadata
    outImg->width = (uint32_t)outWidth;
    outImg->height = (uint32_t)outHeight;
    outImg->size = (uint32_t)(outWidth * outHeight);
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
int add(const Image *img1, const Image *img2, Image *outImg)
{
    // NOTE: Channel operations work on width*height pixels, NOT width*height*depth
    int totalPixels = img1->width * img1->height;

    if (isChalsEmpty(outImg)) {
        embeddip_status_t status = createChals(outImg, outImg->depth);
        if (status != EMBEDDIP_OK) {
            return status;  // Memory allocation failed
        }
        outImg->is_chals = 1;
    }

    // Safety check: ensure channel was actually allocated
    if (outImg->chals == NULL || outImg->chals->ch[0] == NULL) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Both in pixels buffer
    if (img1->log == IMAGE_DATA_PIXELS && img2->log == IMAGE_DATA_PIXELS) {
        uint8_t *data1 = img1->pixels;
        uint8_t *data2 = img2->pixels;
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            outData[i] = data1[i] + data2[i];
        }
    }
    // img1 in pixels, img2 in channels
    else if (img1->log == IMAGE_DATA_PIXELS && img2->log != IMAGE_DATA_PIXELS) {
        if (isChalsEmpty(img2)) {
            embeddip_status_t status = createChals((Image *)img2, img2->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }

        int ch2_idx = 0;
        if (img2->log >= IMAGE_DATA_CH0 && img2->log <= IMAGE_DATA_CH5) {
            ch2_idx = img2->log - IMAGE_DATA_CH0;
        }

        // Safety check
        if (img2->chals == NULL || img2->chals->ch[ch2_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        uint8_t *data1 = img1->pixels;
        float *data2 = img2->chals->ch[ch2_idx];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            outData[i] = data1[i] + data2[i];
        }
    }
    // img1 in channels, img2 in pixels
    else if (img1->log != IMAGE_DATA_PIXELS && img2->log == IMAGE_DATA_PIXELS) {
        if (isChalsEmpty(img1)) {
            embeddip_status_t status = createChals((Image *)img1, img1->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }

        int ch1_idx = 0;
        if (img1->log >= IMAGE_DATA_CH0 && img1->log <= IMAGE_DATA_CH5) {
            ch1_idx = img1->log - IMAGE_DATA_CH0;
        }

        // Safety check
        if (img1->chals == NULL || img1->chals->ch[ch1_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        float *data1 = img1->chals->ch[ch1_idx];
        uint8_t *data2 = img2->pixels;
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            outData[i] = data1[i] + data2[i];
        }
    }
    // Both in channels
    else {
        if (isChalsEmpty(img1)) {
            embeddip_status_t status = createChals((Image *)img1, img1->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }
        if (isChalsEmpty(img2)) {
            embeddip_status_t status = createChals((Image *)img2, img2->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }

        int ch1_idx = 0;
        if (img1->log >= IMAGE_DATA_CH0 && img1->log <= IMAGE_DATA_CH5) {
            ch1_idx = img1->log - IMAGE_DATA_CH0;
        }

        int ch2_idx = 0;
        if (img2->log >= IMAGE_DATA_CH0 && img2->log <= IMAGE_DATA_CH5) {
            ch2_idx = img2->log - IMAGE_DATA_CH0;
        }

        // Safety checks
        if (img1->chals == NULL || img1->chals->ch[ch1_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        if (img2->chals == NULL || img2->chals->ch[ch2_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        float *data1 = img1->chals->ch[ch1_idx];
        float *data2 = img2->chals->ch[ch2_idx];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            outData[i] = data1[i] + data2[i];
        }
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

int dist(const Image *inImg, Image *outImg, uint8_t R_ref, uint8_t G_ref, uint8_t B_ref)
/**
 * @brief Computes the color distance of each pixel in an RGB
 * image to a given reference color.
 *
 * @param[in] inImg Pointer to the input RGB image (3 channels, interleaved as RGBRGB...).
 * @param[out] outImg Pointer to the output grayscale image (1 channel, same width and height as
 * input).
 * @param[in] R_ref Reference Red channel value (0–255).
 * @param[in] G_ref Reference Green channel value (0–255).
 * @param[in] B_ref Reference Blue channel value (0–255).
 */
{
    int totalPixels = inImg->width * inImg->height;

    if (isChalsEmpty(outImg)) {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (inImg->log == IMAGE_DATA_PIXELS) {
        // Raw byte access
        uint8_t *inData = (uint8_t *)inImg->pixels;

        for (int i = 0; i < totalPixels; ++i) {
            int idx = i * 3;
            uint8_t R = inData[idx];
            uint8_t G = inData[idx + 1];
            uint8_t B = inData[idx + 2];

            float d = sqrtf((R - R_ref) * (R - R_ref) + (G - G_ref) * (G - G_ref) +
                            (B - B_ref) * (B - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    } else {
        // Channel access (float-based) - RGB is in ch[1], ch[2], ch[3]
        if (isChalsEmpty(inImg)) {
            createChals((Image *)inImg, inImg->depth);
        }

        float *R_ch = inImg->chals->ch[1];
        float *G_ch = inImg->chals->ch[2];
        float *B_ch = inImg->chals->ch[3];

        for (int i = 0; i < totalPixels; ++i) {
            float d = sqrtf((R_ch[i] - R_ref) * (R_ch[i] - R_ref) +
                            (G_ch[i] - G_ref) * (G_ch[i] - G_ref) +
                            (B_ch[i] - B_ref) * (B_ch[i] - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
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

int normalize(Image *inImg)
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
    if (max - min < 1e-5f)  // avoid divide by zero
        return 0;
    float norm = ((float)val - (float)min) / ((float)max - (float)min);
    if (norm < 0.0f)
        norm = 0.0f;
    if (norm > 1.0f)
        norm = 1.0f;
    return (uint8_t)(norm * 255.0f);
    return EMBEDDIP_OK;
}

int normalize(Image *inImg)
{
    uint8_t *data = (uint8_t *)inImg->pixels;

    uint8_t min = 255, max = 0;
    for (uint32_t i = 0; i < inImg->size; i++) {
        uint8_t v = (uint8_t)data[i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }
    for (uint32_t i = 0; i < inImg->size; i++) {
        data[i] = normalize_to_u8(data[i], min, max);
    }
    return EMBEDDIP_OK;
}

int convertTo(Image *inImg)
{
    uint8_t *data = (uint8_t *)inImg->pixels;

    switch (inImg->format) {
    case IMAGE_FORMAT_MASK:
        // Masks are already binary (0/255) or trimap (0/1/2), no conversion needed
        // Just ensure log state is set to PIXELS
        inImg->log = IMAGE_DATA_PIXELS;
        return EMBEDDIP_OK;

    case IMAGE_FORMAT_GRAYSCALE: {
        if (inImg->log == IMAGE_DATA_CH0 || inImg->log == IMAGE_DATA_MAGNITUDE ||
            inImg->log == IMAGE_DATA_PHASE) {
            float min = FLT_MAX, max = -FLT_MAX;

            // Step 1: Find min and max
            for (uint32_t i = 0; i < inImg->size; i++) {
                float v = inImg->chals->ch[0][i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }

            // Step 2: Normalize to [0, 255] and clamp
            if (max != min) {
                for (uint32_t i = 0; i < inImg->size; i++) {
                    float norm = (inImg->chals->ch[0][i] - min) / (max - min);
                    data[i] = (uint8_t)(norm * 255.0f + 0.5f);  // +0.5 for rounding
                }
            } else {
                // All values are the same, map to 0 or 255
                memset(data, 0, inImg->size);  // Or use 255
            }
        } else if (inImg->log == IMAGE_DATA_PIXELS) {
            uint8_t min = 255, max = 0;
            for (uint32_t i = 0; i < inImg->size; i++) {
                uint8_t v = (uint8_t)data[i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }
            for (uint32_t i = 0; i < inImg->size; i++) {
                data[i] = normalize_to_u8(data[i], min, max);
            }
        }

        break;
    }

    case IMAGE_FORMAT_RGB888:
    case IMAGE_FORMAT_YUV:
    case IMAGE_FORMAT_HSI: {
        int ch_r = 1, ch_g = 2, ch_b = 3;
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (uint32_t i = 0; i < inImg->size; i++) {
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

        for (uint32_t i = 0; i < inImg->size; i++) {
            data[i * 3 + 0] = normalize_to_u8(inImg->chals->ch[ch_b][i], min_b, max_b);
            data[i * 3 + 1] = normalize_to_u8(inImg->chals->ch[ch_g][i], min_g, max_g);
            data[i * 3 + 2] = normalize_to_u8(inImg->chals->ch[ch_r][i], min_r, max_r);
        }
        break;
    }

    case IMAGE_FORMAT_RGB565: {
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (uint32_t i = 0; i < inImg->size; i++) {
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

        for (uint32_t i = 0; i < inImg->size; i++) {
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
    return EMBEDDIP_OK;
}
