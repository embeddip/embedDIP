#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <cv/linear_classifier.h>

int main(void)
{
    /* Two classes, three-element descriptors. */
    const float weights[2 * 3] = {
        1.0f, 0.0f, 0.0f, /* class 0 keys on element 0 */
        0.0f, 0.0f, 1.0f, /* class 1 keys on element 2 */
    };
    const float bias[2] = {0.0f, 10.0f};
    const float descriptor[3] = {5.0f, 0.0f, 1.0f};
    /* score0 = 5, score1 = 1 + 10 = 11 -> class 1 wins. */
    CvLinearClassifier model = {
        .weights = weights,
        .bias = bias,
        .class_count = 2u,
        .descriptor_length = 3u,
    };
    CvClassScore scores[2] = {0};
    size_t count = 0u;

    assert(cv_linear_classifier_topk(&model, descriptor, 3u, 2u, scores, 2u, &count) ==
           EMBEDDIP_OK);
    assert(count == 2u);
    assert(scores[0].class_index == 1u);
    assert(scores[0].score == 11.0f);
    assert(scores[1].class_index == 0u);
    assert(scores[1].score == 5.0f);

    /* top-1 returns only the winner. */
    assert(cv_linear_classifier_topk(&model, descriptor, 3u, 1u, scores, 2u, &count) ==
           EMBEDDIP_OK);
    assert(count == 1u);
    assert(scores[0].class_index == 1u);

    /* Tie breaks toward the lower class index. */
    const float tie_weights[2 * 3] = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    CvLinearClassifier tie_model = {
        .weights = tie_weights,
        .bias = NULL,
        .class_count = 2u,
        .descriptor_length = 3u,
    };
    const float tie_desc[3] = {7.0f, 0.0f, 0.0f};
    assert(cv_linear_classifier_topk(&tie_model, tie_desc, 3u, 2u, scores, 2u, &count) ==
           EMBEDDIP_OK);
    assert(count == 2u);
    assert(scores[0].class_index == 0u); /* equal scores: lower index first */
    assert(scores[1].class_index == 1u);

    /* Rejections. */
    assert(cv_linear_classifier_topk(&model, descriptor, 2u, 2u, scores, 2u, &count) ==
           EMBEDDIP_ERROR_INVALID_SIZE); /* length mismatch */
    assert(cv_linear_classifier_topk(&model, descriptor, 3u, 0u, scores, 2u, &count) ==
           EMBEDDIP_ERROR_INVALID_ARG); /* top_k == 0 */
    assert(cv_linear_classifier_topk(&model, descriptor, 3u, 2u, scores, 1u, &count) ==
           EMBEDDIP_ERROR_INVALID_SIZE); /* capacity < result_count */
    assert(cv_linear_classifier_topk(NULL, descriptor, 3u, 2u, scores, 2u, &count) ==
           EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_linear_classifier_topk(&model, NULL, 3u, 2u, scores, 2u, &count) ==
           EMBEDDIP_ERROR_NULL_PTR);

    CvLinearClassifier null_weights = model;
    null_weights.weights = NULL;
    assert(cv_linear_classifier_topk(&null_weights, descriptor, 3u, 2u, scores, 2u,
                                     &count) == EMBEDDIP_ERROR_NULL_PTR);

    return 0;
}
