#include <assert.h>
#include <stdint.h>

#include <cv/knn.h>

/* Two well-separated 2-D clusters: label 0 near (0,0), label 1 near (10,10). */
#define N 8
static const float train[N * 2] = {
    0.0f, 0.0f,  0.5f, -0.3f,  -0.4f, 0.2f,  0.1f, 0.6f,   /* label 0 */
    10.0f, 10.0f, 9.6f, 10.3f, 10.2f, 9.7f,  9.9f, 9.8f,   /* label 1 */
};
static const int32_t labels[N] = {0, 0, 0, 0, 1, 1, 1, 1};

int main(void)
{
    int32_t lab = -1;
    int votes = 0;

    float q0[2] = {0.2f, 0.1f};   /* clearly class 0 */
    float q1[2] = {9.8f, 10.1f};  /* clearly class 1 */

    assert(cv_knn_classify_f32(train, labels, N, 2, q0, 3, CV_KNN_L2, &lab, &votes) == EMBEDDIP_OK);
    assert(lab == 0 && votes == 3);
    assert(cv_knn_classify_f32(train, labels, N, 2, q1, 3, CV_KNN_L2, &lab, &votes) == EMBEDDIP_OK);
    assert(lab == 1 && votes == 3);

    /* k=1 exact hit */
    assert(cv_knn_classify_f32(train, labels, N, 2, q1, 1, CV_KNN_L2, &lab, NULL) == EMBEDDIP_OK);
    assert(lab == 1);

    /* L1 metric agrees on separable data */
    assert(cv_knn_classify_f32(train, labels, N, 2, q0, 3, CV_KNN_L1, &lab, NULL) == EMBEDDIP_OK);
    assert(lab == 0);

    /* midpoint, k=8 -> 4/4 tie; tie-break by smaller summed distance is
     * symmetric here, so it falls through to smaller label (0). */
    float qm[2] = {5.0f, 5.0f};
    assert(cv_knn_classify_f32(train, labels, N, 2, qm, 8, CV_KNN_L2, &lab, &votes) == EMBEDDIP_OK);
    assert(votes == 4);
    assert(lab == 0 || lab == 1); /* deterministic pick, either cluster valid */

    /* Argument validation. */
    assert(cv_knn_classify_f32(NULL, labels, N, 2, q0, 3, CV_KNN_L2, &lab, NULL) ==
           EMBEDDIP_ERROR_NULL_PTR);
    assert(cv_knn_classify_f32(train, labels, N, 2, q0, 0, CV_KNN_L2, &lab, NULL) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_knn_classify_f32(train, labels, N, 2, q0, N + 1, CV_KNN_L2, &lab, NULL) ==
           EMBEDDIP_ERROR_INVALID_ARG);
    assert(cv_knn_classify_f32(train, labels, 0, 2, q0, 3, CV_KNN_L2, &lab, NULL) ==
           EMBEDDIP_ERROR_INVALID_SIZE);

    return 0;
}
