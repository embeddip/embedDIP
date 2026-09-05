// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "embedDIP.hpp"
#include "wrapper/CvFeatureWrapper.hpp"

extern "C" {
#include "cv/hog.h"
#include "cv/image_gray.h"
}

int main()
{
    using embedDIP::CvFeatures;
    using embedDIP::Image;

    Image img(16, 16, IMAGE_FORMAT_GRAYSCALE);
    assert(img.isValid());

    // C++ validateGray parity with the C contract.
    ImageView view{};
    assert(CvFeatures::validateGray(img, view) == EMBEDDIP_OK);

    // Same view can be fetched via the Image::view helper.
    ImageView view2{};
    assert(img.view(&view2) == EMBEDDIP_OK);
    assert(view2.width == view.width && view2.height == view.height);
    assert(cv_gray_view_validate(&view2) == EMBEDDIP_OK);

    // hogSize parity with the C API.
    Rectangle roi{0, 0, 16, 16};
    CvHogConfig config{4u, 0.2f};
    std::size_t cppLen = 0u;
    std::size_t cLen = 0u;
    assert(CvFeatures::hogSize(roi, config, cppLen) == EMBEDDIP_OK);
    assert(cv_hog_descriptor_size(roi, &config, &cLen) == EMBEDDIP_OK);
    assert(cppLen == cLen);
    assert(cppLen == (4u - 1u) * (4u - 1u) * 36u);

    // linearTopK through the facade.
    const float weights[2 * 3] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    const float bias[2] = {0.0f, 0.0f};
    CvLinearClassifier model{weights, bias, 2u, 3u};
    const float descriptor[3] = {5.0f, 0.0f, 1.0f};
    CvClassScore scores[2]{};
    std::size_t count = 0u;
    assert(CvFeatures::linearTopK(model, descriptor, 3u, 2u, scores, 2u, count) ==
           EMBEDDIP_OK);
    assert(count == 2u);
    assert(scores[0].class_index == 0u); // score 5 > score 1
    assert(scores[1].class_index == 1u);

    return 0;
}
