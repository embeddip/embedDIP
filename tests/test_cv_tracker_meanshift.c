// SPDX-License-Identifier: MIT
#include <assert.h>
#include <string.h>
#include <core/image.h>
#include <cv/tracker_meanshift.h>

int main(void){
    enum { W=128, H=128 };
    static uint8_t px[W*H];
    CvMeanShiftState st; memset(&st,0,sizeof st);
    memset(px,0,sizeof px);
    for(int y=40;y<60;y++) for(int x=40;x<60;x++) px[y*W+x]=200;
    ImageView f={px,W,H,W,IMAGE_FORMAT_GRAYSCALE,IMAGE_DEPTH_U8,0,0};
    Rectangle roi={40,40,20,20};
    assert(cv_meanshift_init(NULL,&f,roi)==EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_meanshift_init(&st,&f,roi)==EMBEDDIP_OK);
    Rectangle out;
    /* shift block by +8,+8 */
    memset(px,0,sizeof px);
    for(int y=48;y<68;y++) for(int x=48;x<68;x++) px[y*W+x]=200;
    assert(cv_meanshift_update(&st,&f,&out)==EMBEDDIP_OK);
    int cx=out.x+out.width/2, cy=out.y+out.height/2;
    assert(cx>=44 && cx<=64 && cy>=44 && cy<=64);   /* tracked toward (58,58) */

    /* ROI partially outside the frame: geometry must clamp to fit. */
    CvMeanShiftState st2; memset(&st2,0,sizeof st2);
    Rectangle partial_roi={120,120,20,20};
    assert(cv_meanshift_init(&st2,&f,partial_roi)==EMBEDDIP_OK);
    assert(st2.box_width<=W-120 && st2.box_height<=H-120);
    assert(st2.center_x - st2.box_width/2 >= 0 && st2.center_x + st2.box_width/2 <= W);
    assert(st2.center_y - st2.box_height/2 >= 0 && st2.center_y + st2.box_height/2 <= H);

    /* ROI fully outside the frame: init must fail. */
    CvMeanShiftState st3; memset(&st3,0,sizeof st3);
    Rectangle outside_roi={200,200,20,20};
    assert(cv_meanshift_init(&st3,&f,outside_roi)==EMBEDDIP_ERROR_INVALID_SIZE);

    /* update() must reject an unsupported format even if init'd on a supported one. */
    static uint8_t px888[W*H*3];
    ImageView f_rgb888={px888,W,H,W*3,IMAGE_FORMAT_RGB888,IMAGE_DEPTH_U8,0,0};
    Rectangle out2;
    assert(cv_meanshift_update(&st,&f_rgb888,&out2)==EMBEDDIP_ERROR_INVALID_FORMAT);

    return 0;
}
