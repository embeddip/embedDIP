#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <core/image.h>
#include <cv/tracker_kalman.h>

static void test_init_null(void)
{
    Rectangle box = {0, 0, 10, 10};
    assert(cv_kalman_init(NULL, box) == EMBEDDIP_ERROR_NULL_PTR);
}

static void test_init_invalid_size(void)
{
    CvKalmanState state;
    Rectangle bad_box = {0, 0, 0, 10};
    assert(cv_kalman_init(&state, bad_box) == EMBEDDIP_ERROR_INVALID_SIZE);
}

static void test_predict_without_init(void)
{
    CvKalmanState state;
    memset(&state, 0, sizeof(state));
    Rectangle out;
    assert(cv_kalman_predict(&state, &out) == EMBEDDIP_ERROR_NOT_INITIALIZED);
}

static void test_tracks_constant_velocity(void)
{
    CvKalmanState state;
    Rectangle initial = {0, 0, 10, 10};
    assert(cv_kalman_init(&state, initial) == EMBEDDIP_OK);

    /* Feed measurements moving +5px/frame in x, correcting the filter each time. */
    int32_t x = 0;
    for (int frame = 0; frame < 20; ++frame) {
        Rectangle out;
        assert(cv_kalman_predict(&state, &out) == EMBEDDIP_OK);
        x += 5;
        Rectangle measured = {x, 0, 10, 10};
        assert(cv_kalman_update(&state, measured) == EMBEDDIP_OK);
    }

    Rectangle final_out;
    assert(cv_kalman_predict(&state, &final_out) == EMBEDDIP_OK);
    /* After 20 frames of consistent +5px/frame motion, predicted x should be
     * close to the true trajectory (within 15px slack for filter lag). */
    assert(final_out.x > x - 15 && final_out.x < x + 30);
}

int main(void)
{
    test_init_null();
    test_init_invalid_size();
    test_predict_without_init();
    test_tracks_constant_velocity();
    return 0;
}
