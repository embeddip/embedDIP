/* Host reference (== on-device code): kNN recognition accuracy over feature
 * files. Reads float32 train/test matrices + int32 label vectors, runs
 * cv_knn_classify_f32, prints "correct total accuracy%".
 * Args: train.f32 trainlab.i32 test.f32 testlab.i32 n_train n_test dim k */
#include <stdio.h>
#include <stdlib.h>

#include "cv/knn.h"

static void *slurp(const char *path, size_t bytes)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "open %s\n", path); exit(2); }
    void *p = malloc(bytes);
    if (fread(p, 1, bytes, f) != bytes) { fprintf(stderr, "read %s\n", path); exit(2); }
    fclose(f);
    return p;
}

int main(int argc, char **argv)
{
    if (argc != 9) {
        fprintf(stderr, "usage: %s train.f32 trainlab.i32 test.f32 testlab.i32 "
                        "n_train n_test dim k\n", argv[0]);
        return 2;
    }
    size_t ntr = strtoul(argv[5], 0, 10), nte = strtoul(argv[6], 0, 10);
    size_t dim = strtoul(argv[7], 0, 10);
    int k = atoi(argv[8]);

    float *train = slurp(argv[1], ntr * dim * sizeof(float));
    int32_t *tlab = slurp(argv[2], ntr * sizeof(int32_t));
    float *test = slurp(argv[3], nte * dim * sizeof(float));
    int32_t *elab = slurp(argv[4], nte * sizeof(int32_t));

    size_t correct = 0;
    for (size_t i = 0; i < nte; ++i) {
        int32_t pred = -1;
        cv_knn_classify_f32(train, tlab, ntr, dim, test + i * dim, k, CV_KNN_L2, &pred, 0);
        correct += (pred == elab[i]);
    }
    printf("%zu %zu %.2f%%\n", correct, nte, 100.0 * (double)correct / (double)nte);
    return 0;
}
