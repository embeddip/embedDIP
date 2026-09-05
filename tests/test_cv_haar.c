#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cv/haar.h>
#include <cv/integral.h>

int main(void)
{
    /* Integral of the 4x4 ramp 1..16 (inclusive sums). */
    uint32_t values[16] = {
        1u,  3u,  6u,  10u,
        6u,  14u, 24u, 36u,
        15u, 32u, 51u, 72u,
        28u, 60u, 96u, 136u,
    };
    CvIntegralU32 table = {
        .values = values,
        .width = 4u,
        .height = 4u,
        .row_stride_values = 4u,
    };
    int32_t response = 0;

    /* One positive rectangle over the full window: sum 136, weight 1.0. */
    CvHaarRect full_rect[1] = {
        {.x = 0, .y = 0, .width = 4u, .height = 4u, .weight_q8 = 256},
    };
    assert(cv_haar_feature_response(&table, 0, 0, full_rect, 1u, &response) ==
           EMBEDDIP_OK);
    assert(response == 136 * 256);

    /* Two-rectangle left-minus-right: left sum 14, right sum 22. */
    CvHaarRect lr_rects[2] = {
        {.x = 0, .y = 0, .width = 2u, .height = 2u, .weight_q8 = 256},
        {.x = 2, .y = 0, .width = 2u, .height = 2u, .weight_q8 = -256},
    };
    assert(cv_haar_feature_response(&table, 0, 0, lr_rects, 2u, &response) ==
           EMBEDDIP_OK);
    assert(response == (14 - 22) * 256);

    /* Rejections: count 0, count > max, null out, out-of-table coords. */
    assert(cv_haar_feature_response(&table, 0, 0, full_rect, 0u, &response) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_haar_feature_response(&table, 0, 0, full_rect, 4u, &response) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_haar_feature_response(&table, 0, 0, full_rect, 1u, NULL) ==
           EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_haar_feature_response(&table, 1, 0, full_rect, 1u, &response) ==
           EMBEDDIP_ERROR_OUT_OF_RANGE);
    assert(cv_haar_feature_response(&table, -1, 0, full_rect, 1u, &response) ==
           EMBEDDIP_ERROR_OUT_OF_RANGE);

    /*
     * Weak classifier: response_q8 = 136*256 = 34816.
     * threshold 30000 -> response >= threshold -> right_value.
     */
    CvHaarWeakClassifier weak_right = {
        .rectangle_count = 1u,
        .rectangles = {{.x = 0, .y = 0, .width = 4u, .height = 4u, .weight_q8 = 256}},
        .threshold_q8 = 30000,
        .left_value = 0,
        .right_value = 100,
    };
    CvHaarStage pass_stage = {.weak = &weak_right, .weak_count = 1u, .threshold = 50};
    CvHaarCascade pass_cascade = {
        .stages = &pass_stage,
        .stage_count = 1u,
        .window = {.x = 0, .y = 0, .width = 4, .height = 4},
    };
    bool detected = false;
    assert(cv_haar_cascade_eval(&table, 0, 0, &pass_cascade, &detected) == EMBEDDIP_OK);
    assert(detected == true);

    /* Same weak below its threshold selects left_value. */
    CvHaarWeakClassifier weak_left = weak_right;
    weak_left.threshold_q8 = 40000; /* 34816 < 40000 -> left_value 0 */
    CvHaarStage left_stage = {.weak = &weak_left, .weak_count = 1u, .threshold = 50};
    CvHaarCascade left_cascade = {
        .stages = &left_stage,
        .stage_count = 1u,
        .window = {.x = 0, .y = 0, .width = 4, .height = 4},
    };
    assert(cv_haar_cascade_eval(&table, 0, 0, &left_cascade, &detected) == EMBEDDIP_OK);
    assert(detected == false); /* weak sum 0 < stage threshold 50 */

    /* Cascade fails at a raised stage threshold. */
    CvHaarStage fail_stage = {.weak = &weak_right, .weak_count = 1u, .threshold = 200};
    CvHaarCascade fail_cascade = {
        .stages = &fail_stage,
        .stage_count = 1u,
        .window = {.x = 0, .y = 0, .width = 4, .height = 4},
    };
    assert(cv_haar_cascade_eval(&table, 0, 0, &fail_cascade, &detected) == EMBEDDIP_OK);
    assert(detected == false);

    /* Null argument rejections. */
    assert(cv_haar_cascade_eval(&table, 0, 0, &pass_cascade, NULL) ==
           EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_haar_cascade_eval(&table, 0, 0, NULL, &detected) ==
           EMBEDDIP_ERROR_NULL_PTR);

    return 0;
}
