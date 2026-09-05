// SPDX-License-Identifier: MIT
#include <assert.h>
#include <math.h>
#include <string.h>
#include <core/image.h>
#include <cv/track_hist.h>

int main(void){
    /* 8x8 grayscale, left half 0, right half 255 */
    uint8_t g[64];
    for(int y=0;y<8;y++) for(int x=0;x<8;x++) g[y*8+x] = (x<4)?0:255;
    ImageView gv = {g,8,8,8,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    Rectangle full = {0,0,8,8};
    float h1[CV_HIST_MAX_BINS], h2[CV_HIST_MAX_BINS]; uint32_t nb=0;
    assert(cv_hist_build(&gv, full, h1, &nb)==EMBEDDIP_OK && nb==CV_HIST_GRAY_BINS);
    float sum=0; for(uint32_t i=0;i<nb;i++) sum+=h1[i];
    assert(fabsf(sum-1.0f) < 1e-4f);                 /* normalized */
    /* identical histogram -> similarity ~1 */
    memcpy(h2,h1,sizeof(float)*nb);
    assert(cv_hist_bhattacharyya(h1,h2,nb) > 0.99f);
    /* all-black roi -> disjoint from h1 -> low similarity */
    uint8_t b[64]; memset(b,0,64);
    ImageView bv = {b,8,8,8,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    cv_hist_build(&bv, full, h2, &nb);
    assert(cv_hist_bhattacharyya(h1,h2,nb) < 0.8f);
    /* RGB565 path reports 96 bins */
    uint16_t c[16]; for(int i=0;i<16;i++) c[i]=0xF800; /* pure red */
    ImageView cv = {(uint8_t*)c,4,4,8,IMAGE_FORMAT_RGB565,IMAGE_DEPTH_U16,0,0};
    Rectangle r4={0,0,4,4};
    assert(cv_hist_build(&cv, r4, h1, &nb)==EMBEDDIP_OK && nb==CV_HIST_COLOR_BINS);
    assert(cv_hist_build(NULL, r4, h1, &nb)==EMBEDDIP_ERROR_NULL_PTR);
    return 0;
}
