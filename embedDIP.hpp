/* ========================================================================== */
/*  File: embedDIP.hpp                                                        */
/*  Brief: C++ umbrella header for the EmbedDIP C++ API layer                 */
/*  SPDX-License-Identifier: MIT                                              */
/* ========================================================================== */
#pragma once

/**
 * @file embedDIP.hpp
 * @brief Umbrella header for the C++ layer of the EmbedDIP library.
 *
 * This header includes all high-level C++ wrappers for EmbedDIP’s core
 * components, such as image handling, camera control, memory management,
 * display, and serial communication.
 *
 * @note Include this file if you are writing C++ code and want the
 *       RAII-style, type-safe interface over the underlying C API.
 */

#include "wrapper/ImageWrapper.hpp"
#include "wrapper/CameraWrapper.hpp"
#include "wrapper/MemoryManager.hpp"
#include "wrapper/DisplayWrapper.hpp"
#include "wrapper/SerialWrapper.hpp"