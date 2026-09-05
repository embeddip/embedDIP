add_executable(embeddip_test_image_lifecycle test_image_lifecycle.c)
target_link_libraries(embeddip_test_image_lifecycle PRIVATE embedDIP)
add_test(NAME embeddip.image_lifecycle COMMAND embeddip_test_image_lifecycle)

add_executable(embeddip_test_image_view test_image_view.c)
target_link_libraries(embeddip_test_image_view PRIVATE embedDIP)
add_test(NAME embeddip.image_view COMMAND embeddip_test_image_view)

add_executable(embeddip_test_cv_image_gray test_cv_image_gray.c)
target_link_libraries(embeddip_test_cv_image_gray PRIVATE embedDIP)
add_test(NAME embeddip.cv_image_gray COMMAND embeddip_test_cv_image_gray)

add_executable(embeddip_test_cv_integral test_cv_integral.c)
target_link_libraries(embeddip_test_cv_integral PRIVATE embedDIP)
add_test(NAME embeddip.cv_integral COMMAND embeddip_test_cv_integral)

add_executable(embeddip_test_cv_haar test_cv_haar.c)
target_link_libraries(embeddip_test_cv_haar PRIVATE embedDIP)
add_test(NAME embeddip.cv_haar COMMAND embeddip_test_cv_haar)

add_executable(embeddip_test_cv_hog test_cv_hog.c)
target_link_libraries(embeddip_test_cv_hog PRIVATE embedDIP)
add_test(NAME embeddip.cv_hog COMMAND embeddip_test_cv_hog)

add_executable(embeddip_test_cv_linear_classifier test_cv_linear_classifier.c)
target_link_libraries(embeddip_test_cv_linear_classifier PRIVATE embedDIP)
add_test(NAME embeddip.cv_linear_classifier COMMAND embeddip_test_cv_linear_classifier)

add_executable(embeddip_test_cv_cpp_wrapper test_cv_cpp_wrapper.cpp)
target_link_libraries(embeddip_test_cv_cpp_wrapper PRIVATE embedDIP)
add_test(NAME embeddip.cv_cpp_wrapper COMMAND embeddip_test_cv_cpp_wrapper)

add_executable(embeddip_test_cv_detect test_cv_detect.c)
target_link_libraries(embeddip_test_cv_detect PRIVATE embedDIP)
add_test(NAME embeddip.cv_detect COMMAND embeddip_test_cv_detect)

add_executable(embeddip_test_cv_nn test_cv_nn.c)
target_link_libraries(embeddip_test_cv_nn PRIVATE embedDIP)
add_test(NAME embeddip.cv_nn COMMAND embeddip_test_cv_nn)

add_executable(embeddip_test_cv_tracker_kalman test_cv_tracker_kalman.c)
target_link_libraries(embeddip_test_cv_tracker_kalman PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_kalman COMMAND embeddip_test_cv_tracker_kalman)

add_executable(embeddip_test_cv_tracker_template test_cv_tracker_template.c)
target_link_libraries(embeddip_test_cv_tracker_template PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_template COMMAND embeddip_test_cv_tracker_template)

add_executable(embeddip_test_cv_track_hist test_cv_track_hist.c)
target_link_libraries(embeddip_test_cv_track_hist PRIVATE embedDIP)
add_test(NAME embeddip.cv_track_hist COMMAND embeddip_test_cv_track_hist)

add_executable(embeddip_test_cv_tracker_particle test_cv_tracker_particle.c)
target_link_libraries(embeddip_test_cv_tracker_particle PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_particle COMMAND embeddip_test_cv_tracker_particle)

add_executable(embeddip_test_cv_tracker_meanshift test_cv_tracker_meanshift.c)
target_link_libraries(embeddip_test_cv_tracker_meanshift PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_meanshift COMMAND embeddip_test_cv_tracker_meanshift)

add_executable(embeddip_test_cv_tracker_kcf test_cv_tracker_kcf.c)
target_link_libraries(embeddip_test_cv_tracker_kcf PRIVATE embedDIP)
add_test(NAME embeddip.cv_tracker_kcf COMMAND embeddip_test_cv_tracker_kcf)

add_executable(embeddip_test_cv_track_assoc test_cv_track_assoc.c)
target_link_libraries(embeddip_test_cv_track_assoc PRIVATE embedDIP)
add_test(NAME embeddip.cv_track_assoc COMMAND embeddip_test_cv_track_assoc)

add_executable(embeddip_test_runtime test_runtime.c)
target_link_libraries(embeddip_test_runtime PRIVATE embedDIP)
add_test(NAME embeddip.runtime COMMAND embeddip_test_runtime)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME embeddip.model_manifest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_model_manifest.py)
endif()

if(EMBEDDIP_TARGET_BOARD STREQUAL "HOST")
    add_executable(embeddip_test_memory_regions test_memory_regions.c)
    target_link_libraries(embeddip_test_memory_regions PRIVATE embedDIP)
    add_test(NAME embeddip.memory_regions COMMAND embeddip_test_memory_regions)

    add_executable(embeddip_test_stm32n6_memory test_stm32n6_memory.c)
    target_include_directories(embeddip_test_stm32n6_memory PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/stm32n6
        ${CMAKE_SOURCE_DIR}/board/stm32n6
        ${CMAKE_SOURCE_DIR})
    add_test(NAME embeddip.stm32n6_memory COMMAND embeddip_test_stm32n6_memory)

    add_executable(embeddip_test_stm32h7s_memory test_stm32h7s_memory.c)
    target_include_directories(embeddip_test_stm32h7s_memory PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fakes/stm32h7s
        ${CMAKE_SOURCE_DIR}/board/stm32h7s
        ${CMAKE_SOURCE_DIR})
    add_test(NAME embeddip.stm32h7s_memory COMMAND embeddip_test_stm32h7s_memory)
endif()

add_test(NAME embeddip.target_matrix
    COMMAND ${CMAKE_COMMAND}
        -DEMBEDDIP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DEMBEDDIP_BINARY_DIR=${CMAKE_BINARY_DIR}/matrix
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_target_matrix.cmake)

add_test(NAME embeddip.stm32n6_profile_missing
    COMMAND ${CMAKE_COMMAND}
        -DEMBEDDIP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DEMBEDDIP_BINARY_DIR=${CMAKE_BINARY_DIR}/n6-missing
        -DEMBEDDIP_STM32CUBE_N6_ROOT=/path/that/does/not/exist
        -DEMBEDDIP_EXPECT_MISSING_SDK=ON
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_stm32n6_profile.cmake)

if(IS_DIRECTORY "${EMBEDDIP_STM32CUBE_N6_ROOT}")
    add_test(NAME embeddip.stm32n6_profile
        COMMAND ${CMAKE_COMMAND}
            -DEMBEDDIP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DEMBEDDIP_BINARY_DIR=${CMAKE_BINARY_DIR}/n6
            "-DEMBEDDIP_STM32CUBE_N6_ROOT=${EMBEDDIP_STM32CUBE_N6_ROOT}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_stm32n6_profile.cmake)
endif()

add_test(NAME embeddip.stm32h7s_profile_missing
    COMMAND ${CMAKE_COMMAND}
        -DEMBEDDIP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DEMBEDDIP_BINARY_DIR=${CMAKE_BINARY_DIR}/h7s-missing
        -DEMBEDDIP_STM32CUBE_H7RS_ROOT=/path/that/does/not/exist
        -DEMBEDDIP_EXPECT_MISSING_SDK=ON
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_stm32h7s_profile.cmake)

if(IS_DIRECTORY "${EMBEDDIP_STM32CUBE_H7RS_ROOT}")
    add_test(NAME embeddip.stm32h7s_profile
        COMMAND ${CMAKE_COMMAND}
            -DEMBEDDIP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DEMBEDDIP_BINARY_DIR=${CMAKE_BINARY_DIR}/h7s
            "-DEMBEDDIP_STM32CUBE_H7RS_ROOT=${EMBEDDIP_STM32CUBE_H7RS_ROOT}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_stm32h7s_profile.cmake)
endif()
