#include <assert.h>
#include <stdint.h>

#include <cv/image_gray.h>

int main(void)
{
    uint8_t pixels[10] = {1u, 2u, 3u, 0xaau, 0xbbu, 4u, 5u, 6u, 0xccu, 0xddu};
    uint8_t pixel = 0u;
    ImageView view = {
        .pixels = pixels,
        .width = 3u,
        .height = 2u,
        .row_stride_bytes = 5u,
        .format = IMAGE_FORMAT_GRAYSCALE,
        .depth = IMAGE_DEPTH_U8,
        .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
        .flags = 0u,
    };

    assert(cv_gray_view_validate(&view) == EMBEDDIP_OK);
    assert(cv_gray_pixel_u8(&view, 0u, 0u, &pixel) == EMBEDDIP_OK);
    assert(pixel == 1u);
    assert(cv_gray_pixel_u8(&view, 2u, 1u, &pixel) == EMBEDDIP_OK);
    assert(pixel == 6u);

    view.format = IMAGE_FORMAT_MASK;
    assert(cv_gray_view_validate(&view) == EMBEDDIP_OK);
    view.format = IMAGE_FORMAT_GRAYSCALE;

    assert(cv_gray_pixel_u8(&view, view.width, 0u, &pixel) == EMBEDDIP_ERROR_OUT_OF_RANGE);
    assert(cv_gray_pixel_u8(&view, 0u, view.height, &pixel) == EMBEDDIP_ERROR_OUT_OF_RANGE);
    assert(cv_gray_pixel_u8(&view, 0u, 0u, NULL) == EMBEDDIP_ERROR_NULL_PTR);

    view.pixels = NULL;
    assert(cv_gray_view_validate(&view) == EMBEDDIP_ERROR_NULL_PTR);
    view.pixels = pixels;

    view.width = 0u;
    assert(cv_gray_view_validate(&view) == EMBEDDIP_ERROR_INVALID_SIZE);
    view.width = 3u;

    view.height = 0u;
    assert(cv_gray_view_validate(&view) == EMBEDDIP_ERROR_INVALID_SIZE);
    view.height = 2u;

    view.row_stride_bytes = 2u;
    assert(cv_gray_view_validate(&view) == EMBEDDIP_ERROR_INVALID_SIZE);
    view.row_stride_bytes = 5u;

    view.format = IMAGE_FORMAT_RGB888;
    assert(cv_gray_view_validate(&view) == EMBEDDIP_ERROR_INVALID_FORMAT);
    view.format = IMAGE_FORMAT_GRAYSCALE;

    view.depth = IMAGE_DEPTH_F32;
    assert(cv_gray_view_validate(&view) == EMBEDDIP_ERROR_INVALID_DEPTH);

    return 0;
}
