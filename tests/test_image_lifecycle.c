#include <assert.h>
#include <stdint.h>

#include <board/common.h>
#include <core/memory_manager.h>

int main(void)
{
    Image *image = 0;

    memory_init(0);
    assert(createImageWH(3, 2, IMAGE_FORMAT_RGB888, &image) == EMBEDDIP_OK);
    assert(image != 0);
    assert(image->width == 3u && image->height == 2u);
    assert(image->size == 6u && image->depth == IMAGE_DEPTH_U24);
    assert(image->pixels != 0);
    ((uint8_t *)image->pixels)[17] = 0xA5u;
    assert(((uint8_t *)image->pixels)[17] == 0xA5u);
    deleteImage(image);
    return 0;
}
