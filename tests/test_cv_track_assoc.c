// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <core/image.h>
#include <cv/track_assoc.h>

static Rectangle R(int x,int y,int w,int h){ Rectangle r={x,y,w,h}; return r; }

static void test_iou(void){
    assert(cv_track_assoc_iou(R(0,0,10,10), R(0,0,10,10)) > 0.99f);
    assert(cv_track_assoc_iou(R(0,0,10,10), R(100,100,10,10)) == 0.0f);
    float half = cv_track_assoc_iou(R(0,0,10,10), R(5,0,10,10)); /* overlap 50/150 */
    assert(half > 0.32f && half < 0.34f);
}

static void test_null(void){
    assert(cv_track_assoc_init(NULL, 0.3f, 5) == EMBEDDIP_ERROR_NULL_PTR);
    CvTrackAssoc t; cv_track_assoc_init(&t,0.3f,5);
    assert(cv_track_assoc_step(NULL, NULL, 0) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_spawn_and_persist(void){
    CvTrackAssoc t; assert(cv_track_assoc_init(&t,0.3f,5)==EMBEDDIP_OK);
    CvDetection d = { R(50,50,20,20), 100 };
    assert(cv_track_assoc_step(&t,&d,1)==EMBEDDIP_OK);
    /* one track spawned with id 0 */
    int active=0, id=-1;
    for(size_t i=0;i<CV_TRACK_ASSOC_MAX;i++) if(t.tracks[i].active){active++; id=t.tracks[i].id;}
    assert(active==1 && id==0);
    /* move detection slightly; same track keeps its id */
    CvDetection d2 = { R(53,52,20,20), 100 };
    assert(cv_track_assoc_step(&t,&d2,1)==EMBEDDIP_OK);
    active=0; for(size_t i=0;i<CV_TRACK_ASSOC_MAX;i++) if(t.tracks[i].active){active++; assert(t.tracks[i].id==0);}
    assert(active==1);
}

static void test_miss_then_drop(void){
    CvTrackAssoc t; cv_track_assoc_init(&t,0.3f,2);
    CvDetection d = { R(10,10,20,20), 100 };
    cv_track_assoc_step(&t,&d,1);
    /* no detections for max_misses+1 frames -> track dropped */
    for(int k=0;k<3;k++) cv_track_assoc_step(&t,NULL,0);
    for(size_t i=0;i<CV_TRACK_ASSOC_MAX;i++) assert(!t.tracks[i].active);
}

int main(void){ test_iou(); test_null(); test_spawn_and_persist(); test_miss_then_drop(); return 0; }
