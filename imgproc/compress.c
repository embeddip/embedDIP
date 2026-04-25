// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "imgproc/compress.h"

#include <stdlib.h>
#include <string.h>

#if !defined(EMBEDDIP_HAVE_LIBJPEG) && defined(ARDUINO_ARCH_ESP32) && defined(__has_include)
    #if __has_include("img_converters.h")
        #define EMBEDDIP_HAVE_ESP32_FMT2JPG 1
    #endif
#endif

#if defined(EMBEDDIP_HAVE_LIBJPEG)

    #include "jpeglib.h"

typedef struct {
    struct jpeg_destination_mgr pub;
    JOCTET *buffer;
    uint32_t capacity;
    JOCTET spill[64];
    int overflow;
} fixed_dest_mgr_t;

static void fixed_dest_init(j_compress_ptr cinfo)
{
    fixed_dest_mgr_t *dest = (fixed_dest_mgr_t *)cinfo->dest;
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = dest->capacity;
    dest->overflow = 0;
}

static boolean fixed_dest_empty(j_compress_ptr cinfo)
{
    fixed_dest_mgr_t *dest = (fixed_dest_mgr_t *)cinfo->dest;
    dest->overflow = 1;
    dest->pub.next_output_byte = dest->spill;
    dest->pub.free_in_buffer = sizeof(dest->spill);
    return TRUE;
}

static void fixed_dest_term(j_compress_ptr cinfo)
{
    (void)cinfo;
}

static void jpeg_fixed_dest(j_compress_ptr cinfo, uint8_t *out, uint32_t out_capacity)
{
    fixed_dest_mgr_t *dest = (fixed_dest_mgr_t *)cinfo->dest;
    if (dest == NULL) {
        cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)(
            (j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(fixed_dest_mgr_t));
        dest = (fixed_dest_mgr_t *)cinfo->dest;
    }

    dest->buffer = out;
    dest->capacity = out_capacity;
    dest->overflow = 0;
    dest->pub.init_destination = fixed_dest_init;
    dest->pub.empty_output_buffer = fixed_dest_empty;
    dest->pub.term_destination = fixed_dest_term;
}

int compress(Image *src, Image *dst, int format, int quality)
{
    if (!src || !dst || !src->pixels || !dst->pixels) {
        return -1;
    }

    if (format != IMAGE_COMP_JPEG) {
        return -1;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    uint32_t dst_capacity = dst->width * dst->height * dst->depth;
    if (dst_capacity == 0) {
        jpeg_destroy_compress(&cinfo);
        return -1;
    }
    jpeg_fixed_dest(&cinfo, (uint8_t *)dst->pixels, dst_capacity);

    cinfo.image_width = src->width;
    cinfo.image_height = src->height;

    static JSAMPLE *row_buffer = NULL;
    static uint32_t row_buffer_capacity = 0;
    int row_stride;

    if (src->format == IMAGE_FORMAT_RGB565) {
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);
        cinfo.dct_method = JDCT_IFAST;
        cinfo.optimize_coding = FALSE;

        jpeg_start_compress(&cinfo, TRUE);

        row_stride = src->width * 3;
        if ((uint32_t)row_stride > row_buffer_capacity) {
            JSAMPLE *new_row_buffer = (JSAMPLE *)realloc(row_buffer, row_stride);
            if (!new_row_buffer) {
                jpeg_destroy_compress(&cinfo);
                return -1;
            }
            row_buffer = new_row_buffer;
            row_buffer_capacity = (uint32_t)row_stride;
        }

        if (!row_buffer) {
            jpeg_destroy_compress(&cinfo);
            return -1;
        }

        uint16_t *src_pixels = (uint16_t *)src->pixels;
        while (cinfo.next_scanline < cinfo.image_height) {
            for (uint32_t x = 0; x < src->width; x++) {
                uint16_t pixel = src_pixels[cinfo.next_scanline * src->width + x];
                row_buffer[x * 3 + 0] = (uint8_t)(((pixel >> 11) & 0x1F) << 3);
                row_buffer[x * 3 + 1] = (uint8_t)(((pixel >> 5) & 0x3F) << 2);
                row_buffer[x * 3 + 2] = (uint8_t)((pixel & 0x1F) << 3);
            }
            JSAMPROW row_pointer = row_buffer;
            jpeg_write_scanlines(&cinfo, &row_pointer, 1);
        }
    } else if (src->format == IMAGE_FORMAT_RGB888) {
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);
        cinfo.dct_method = JDCT_IFAST;
        cinfo.optimize_coding = FALSE;

        jpeg_start_compress(&cinfo, TRUE);

        row_stride = src->width * 3;
        uint8_t *src_pixels = (uint8_t *)src->pixels;
        while (cinfo.next_scanline < cinfo.image_height) {
            JSAMPROW row_pointer = &src_pixels[cinfo.next_scanline * row_stride];
            jpeg_write_scanlines(&cinfo, &row_pointer, 1);
        }
    } else if (src->format == IMAGE_FORMAT_GRAYSCALE) {
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);
        cinfo.dct_method = JDCT_IFAST;
        cinfo.optimize_coding = FALSE;

        jpeg_start_compress(&cinfo, TRUE);

        row_stride = src->width;
        uint8_t *src_pixels = (uint8_t *)src->pixels;
        while (cinfo.next_scanline < cinfo.image_height) {
            JSAMPROW row_pointer = &src_pixels[cinfo.next_scanline * row_stride];
            jpeg_write_scanlines(&cinfo, &row_pointer, 1);
        }
    } else {
        jpeg_destroy_compress(&cinfo);
        return -1;
    }

    jpeg_finish_compress(&cinfo);

    fixed_dest_mgr_t *dest = (fixed_dest_mgr_t *)cinfo.dest;
    if (!dest || dest->overflow) {
        jpeg_destroy_compress(&cinfo);
        return -1;
    }

    dst->size = (uint32_t)(dst_capacity - dest->pub.free_in_buffer);
    jpeg_destroy_compress(&cinfo);
    return 0;
}

#elif defined(EMBEDDIP_HAVE_ESP32_FMT2JPG)

    #include "img_converters.h"

int compress(Image *src, Image *dst, int format, int quality)
{
    if (!src || !dst || !src->pixels || !dst->pixels) {
        return -1;
    }

    if (format != IMAGE_COMP_JPEG) {
        return -1;
    }

    if (src->width == 0 || src->height == 0 || src->width > 0xFFFFu || src->height > 0xFFFFu) {
        return -1;
    }

    pixformat_t src_format;
    switch (src->format) {
    case IMAGE_FORMAT_RGB565:
        src_format = PIXFORMAT_RGB565;
        break;
    case IMAGE_FORMAT_RGB888:
        src_format = PIXFORMAT_RGB888;
        break;
    case IMAGE_FORMAT_GRAYSCALE:
    case IMAGE_FORMAT_MASK:
        src_format = PIXFORMAT_GRAYSCALE;
        break;
    default:
        return -1;
    }

    uint8_t jpeg_quality;
    if (quality < 1) {
        jpeg_quality = 1;
    } else if (quality > 100) {
        jpeg_quality = 100;
    } else {
        jpeg_quality = (uint8_t)quality;
    }

    uint32_t src_len = src->width * src->height * src->depth;
    uint32_t dst_capacity = dst->width * dst->height * dst->depth;
    if (src_len == 0 || dst_capacity == 0) {
        return -1;
    }

    uint8_t *encoded = NULL;
    size_t encoded_len = 0;
    bool ok = fmt2jpg((uint8_t *)src->pixels,
                      (size_t)src_len,
                      (uint16_t)src->width,
                      (uint16_t)src->height,
                      src_format,
                      jpeg_quality,
                      &encoded,
                      &encoded_len);
    if (!ok || !encoded || encoded_len == 0) {
        if (encoded) {
            free(encoded);
        }
        return -1;
    }

    if (encoded_len > dst_capacity) {
        free(encoded);
        return -1;
    }

    memcpy(dst->pixels, encoded, encoded_len);
    dst->size = (uint32_t)encoded_len;
    free(encoded);
    return 0;
}

#else

int compress(Image *src, Image *dst, int format, int quality)
{
    (void)src;
    (void)dst;
    (void)format;
    (void)quality;
    return -1;
}

#endif
