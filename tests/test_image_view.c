#include <assert.h>
#include <stdint.h>

#include <board/common.h>
#include <core/image.h>
#include <core/memory_manager.h>

int main(void)
{
    uint8_t pixels[16] = {0};
    ImageView view;
    Image image = {
        .width = 3u,
        .height = 2u,
        .pixels = pixels,
        .format = IMAGE_FORMAT_RGB888,
        .depth = IMAGE_DEPTH_U24,
    };

    assert(image_view_from_buffer(pixels, 3u, 2u, 9u, IMAGE_FORMAT_RGB888, IMAGE_DEPTH_U24,
                                  EMBEDDIP_MEMORY_REGION_DMA, EMBEDDIP_BUFFER_DMA_WRITE,
                                  &view) == EMBEDDIP_OK);
    assert(image_view_row(&view, 1u) == pixels + 9u);
    assert(image_view_from_buffer(pixels, 3u, 2u, 8u, IMAGE_FORMAT_RGB888, IMAGE_DEPTH_U24,
                                  EMBEDDIP_MEMORY_REGION_DMA, EMBEDDIP_BUFFER_DMA_WRITE,
                                  &view) == EMBEDDIP_ERROR_INVALID_SIZE);

    assert(image_view_from_buffer(pixels, 1u, 1u, 3u, IMAGE_FORMAT_RGB888, IMAGE_DEPTH_U24,
                                  EMBEDDIP_MEMORY_REGION_DEFAULT, 0u,
                                  NULL) == EMBEDDIP_ERROR_NULL_PTR);
    assert(image_view_from_buffer(NULL, 1u, 1u, 3u, IMAGE_FORMAT_RGB888, IMAGE_DEPTH_U24,
                                  EMBEDDIP_MEMORY_REGION_DEFAULT, 0u,
                                  &view) == EMBEDDIP_ERROR_NULL_PTR);
    assert(image_view_from_buffer(pixels, 0u, 1u, 3u, IMAGE_FORMAT_RGB888, IMAGE_DEPTH_U24,
                                  EMBEDDIP_MEMORY_REGION_DEFAULT, 0u,
                                  &view) == EMBEDDIP_ERROR_INVALID_SIZE);
    assert(image_view_from_buffer(pixels, 1u, 1u, 3u, IMAGE_FORMAT_RGB888, IMAGE_DEPTH_U8,
                                  EMBEDDIP_MEMORY_REGION_DEFAULT, 0u,
                                  &view) == EMBEDDIP_ERROR_INVALID_FORMAT);

    assert(image_view_from_image(&image, &view) == EMBEDDIP_OK);
    assert(view.pixels == pixels && view.row_stride_bytes == 9u);
    assert(view.region == EMBEDDIP_MEMORY_REGION_DEFAULT);
    assert(view.flags == (EMBEDDIP_BUFFER_CPU_READ | EMBEDDIP_BUFFER_CPU_WRITE));
    assert(image_view_from_image(NULL, &view) == EMBEDDIP_ERROR_NULL_PTR);
    assert(image_view_row(&view, 2u) == NULL);
    assert(image_view_row(NULL, 0u) == NULL);
    return 0;
}
