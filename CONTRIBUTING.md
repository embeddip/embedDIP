# Contributing to embedDIP

Thank you for your interest in contributing to embedDIP! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Pull Request Process](#pull-request-process)
- [Reporting Issues](#reporting-issues)

---

## Code of Conduct

This project follows a professional and respectful code of conduct:

- **Be respectful**: Treat everyone with respect and kindness
- **Be constructive**: Provide helpful feedback and suggestions
- **Be collaborative**: Work together to improve the project
- **Be inclusive**: Welcome contributors of all backgrounds and skill levels

---

## Getting Started

### Prerequisites

- **Git**: Version control
- **CMake**: 3.15 or higher
- **C/C++ Compiler**:
  - HOST: GCC, Clang, or MSVC
  - STM32F7: ARM GCC toolchain (arm-none-eabi-gcc)
  - ESP32: ESP-IDF toolchain
- **clang-format**: For code formatting (optional but recommended)

### Fork and Clone

```bash
# Fork the repository on GitHub, then clone your fork
git clone https://github.com/YOUR_USERNAME/embedDIP.git
cd embedDIP

# Add upstream remote
git remote add upstream https://github.com/EmbedDIP/embedDIP.git

# Create a feature branch
git checkout -b feature/your-feature-name
```

---

## Development Setup

### Building for HOST (Development)

```bash
mkdir build && cd build
cmake .. \
    -DEMBEDDIP_TARGET_PLATFORM=HOST \
    -DEMBEDDIP_BUILD_TESTS=ON \
    -DEMBEDDIP_BUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Debug
make
```

### Running Tests

```bash
cd build
ctest --output-on-failure

# Or run individual test executables
./test_image
./test_color
./test_error
```

### Building for STM32F7

```bash
mkdir build-stm32 && cd build-stm32
cmake .. \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake \
    -DEMBEDDIP_TARGET_PLATFORM=STM32F7 \
    -DCMAKE_BUILD_TYPE=Release
ninja
```

---

## Coding Standards

### Code Style

embedDIP follows a consistent coding style enforced by `.clang-format`:

```bash
# Format all source files
find . -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \
    | xargs clang-format -i

# Check formatting (CI style)
clang-format --dry-run --Werror core/*.c imgproc/*.c
```

### Naming Conventions

#### C Code

- **Functions**: `snake_case`
  ```c
  embeddip_status_t create_image(int width, int height);
  ```

- **Types**: `PascalCase` with `_t` suffix for typedefs
  ```c
  typedef struct Image Image;
  typedef enum ImageFormat ImageFormat;
  ```

- **Macros/Constants**: `UPPER_SNAKE_CASE`
  ```c
  #define EMBEDDIP_VERSION_MAJOR 0
  #define IMAGE_MAX_CHANNELS 4
  ```

- **Enums**: `UPPER_SNAKE_CASE` with prefix
  ```c
  typedef enum {
      EMBEDDIP_OK = 0,
      EMBEDDIP_ERROR_NULL_PTR = -1,
      // ...
  } embeddip_status_t;
  ```

#### C++ Code

- **Classes/Structs**: `PascalCase`
  ```cpp
  class ImageWrapper { };
  ```

- **Methods**: `camelCase`
  ```cpp
  void processImage();
  ```

- **Member Variables**: `snake_case` with `m_` prefix (optional)
  ```cpp
  int m_width;
  int m_height;
  ```

### File Organization

```
embedDIP/
├── core/           # Core data structures and utilities
│   ├── error.c/h   # Error handling
│   ├── image.h     # Image structure
│   └── memory_manager.h
├── imgproc/        # Image processing algorithms
│   ├── color.c/h
│   ├── filter.c/h
│   └── histogram.c/h
├── device/         # Hardware abstraction layer
│   ├── camera/
│   ├── display/
│   └── serial/
├── board/          # Platform-specific implementations
│   ├── stm32f7/
│   ├── esp32/
│   └── host/
├── wrapper/        # C++ wrappers
│   ├── ImageWrapper.cpp/hpp
│   └── CameraWrapper.cpp/hpp
└── tests/          # Unit tests
    └── test_*.c
```

### Documentation

Use Doxygen-style comments for all public APIs:

```c
/**
 * @brief Create an image with specified resolution and format
 *
 * @param resolution Predefined resolution (VGA, QVGA, etc.)
 * @param format Pixel format (RGB565, RGB888, Grayscale, etc.)
 * @param[out] out_image Pointer to store created image (NULL on failure)
 * @return EMBEDDIP_OK on success, error code otherwise
 *
 * @note Caller is responsible for freeing the image with freeImage()
 *
 * Example:
 * @code
 * Image* img = NULL;
 * embeddip_status_t status = createImage(IMAGE_RES_VGA, IMAGE_FORMAT_RGB565, &img);
 * if (status == EMBEDDIP_OK) {
 *     // Use image...
 *     freeImage(&img);
 * }
 * @endcode
 */
embeddip_status_t createImage(
    ImageResolution resolution,
    ImageFormat format,
    Image **out_image
);
```

---

## Testing

### Writing Unit Tests

All new features must include unit tests. We use the Unity testing framework.

Create a test file in `tests/`:

```c
#include "unity.h"
#include "embedDIP.h"

void setUp(void) {
    // Setup code (runs before each test)
}

void tearDown(void) {
    // Cleanup code (runs after each test)
}

void test_my_new_feature(void) {
    Image* img = NULL;
    embeddip_status_t status = createImage(IMAGE_RES_VGA, IMAGE_FORMAT_RGB565, &img);

    TEST_ASSERT_EQUAL(EMBEDDIP_OK, status);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL(640, img->width);
    TEST_ASSERT_EQUAL(480, img->height);

    freeImage(&img);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_my_new_feature);
    return UNITY_END();
}
```

Add to `tests/CMakeLists.txt`:

```cmake
add_embeddip_test(test_my_feature test_my_feature.c)
```

### Test Coverage Goals

- **Core functions**: 100% coverage
- **Public APIs**: >90% coverage
- **Platform-specific code**: Best effort (hardware dependent)

---

## Pull Request Process

### Before Submitting

1. **Update from upstream**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Run tests**:
   ```bash
   cd build && ctest
   ```

3. **Format code**:
   ```bash
   clang-format -i **/*.c **/*.h **/*.cpp **/*.hpp
   ```

4. **Update documentation**:
   - Update README.md if adding features
   - Update CHANGELOG.md following [Keep a Changelog](https://keepachangelog.com/)
   - Add/update code examples if relevant

### Commit Messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Code style (formatting, whitespace)
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance tasks

**Examples**:
```
feat(imgproc): add bilateral filter implementation

Implements bilateral filtering for edge-preserving smoothing.
Includes unit tests and example usage.

Closes #123

---

fix(camera): correct OV5640 initialization sequence

The previous sequence missed the standby exit step,
causing intermittent capture failures on STM32F746.

Fixes #456
```

### PR Checklist

- [ ] Code follows style guidelines (clang-format applied)
- [ ] All tests pass locally
- [ ] New tests added for new features
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Commit messages follow conventions
- [ ] No merge conflicts with main branch

### Review Process

1. **Submit PR** with clear description
2. **CI checks** must pass (build, tests, formatting)
3. **Code review** by maintainers
4. **Address feedback** with new commits
5. **Approval and merge** by maintainer

---

## Reporting Issues

### Bug Reports

Use the GitHub issue tracker and include:

```markdown
**Description**
Clear description of the bug

**To Reproduce**
Steps to reproduce:
1. Build with '...'
2. Run '...'
3. See error

**Expected Behavior**
What you expected to happen

**Environment**
- Platform: [HOST / STM32F7 / ESP32]
- OS: [Linux / Windows / macOS]
- Compiler: [GCC 11.2 / ARM GCC 13.2 / etc.]
- embedDIP Version: [0.1.0]

**Additional Context**
Any other relevant information
```

### Feature Requests

```markdown
**Feature Description**
Clear description of the proposed feature

**Use Case**
Why is this feature needed? What problem does it solve?

**Proposed Implementation**
(Optional) How might this be implemented?

**Alternatives Considered**
(Optional) Other approaches you've considered
```

---

## License

By contributing to embedDIP, you agree that your contributions will be licensed under the MIT License.

---

## Questions?

- **GitHub Discussions**: For general questions and discussions
- **GitHub Issues**: For bugs and feature requests
- **Email**: Contact maintainers at [repository email]

Thank you for contributing to embedDIP! 🎉
