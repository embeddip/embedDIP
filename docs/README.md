# embedDIP Documentation

This directory contains the generated Doxygen documentation for the embedDIP library.

## Online Documentation

The latest documentation is automatically built and deployed to GitHub Pages:

**[View Documentation Online](https://embeddip.github.io/embedDIP/)**

## Building Documentation Locally

### Prerequisites

Install Doxygen and Graphviz:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Arch Linux
sudo pacman -S doxygen graphviz
```

### Generate Documentation

From the repository root:

```bash
doxygen docs/Doxyfile
```

The HTML documentation will be generated in `docs/html/`.

### View Documentation

Open the documentation in your browser:

```bash
# Linux
xdg-open docs/html/index.html

# macOS
open docs/html/index.html

# Windows
start docs/html/index.html
```

## Documentation Structure

- **HTML Output:** `docs/html/` - Main HTML documentation
- **Configuration:** `docs/Doxyfile` - Doxygen configuration file
- **Warnings Log:** `doxygen_warnings.log` - Build warnings (generated during build)

## Automatic Deployment

Documentation is automatically built and deployed via GitHub Actions:

- **Workflow:** `.github/workflows/doxygen-gh-pages.yml`
- **Trigger:** Push to `main` or `master` branch
- **Build:** Ubuntu runner with Doxygen 1.9.x and Graphviz
- **Deploy:** GitHub Pages

## Documentation Coverage

The documentation covers:

- **C++ Wrapper API** (namespace `embedDIP`)
  - Image processing wrapper (`Image` class)
  - Camera device wrapper (`Camera` class)
  - Display device wrapper (`Display` class)
  - Serial communication wrapper (`Serial` class)
  - Memory management wrapper (`Memory` class)

- **C API** (imgproc layer)
  - Color space conversions (`color.h`)
  - Filtering operations (`filter.h`)
  - Morphological operations (`morph.h`)
  - Thresholding (`thresh.h`)
  - Histogram operations (`histogram.h`)
  - FFT and frequency domain (`fft.h`)
  - Hough transform (`hough.h`)
  - Segmentation algorithms (`segmentation.h`)
  - And more...

- **Core Types** (`core/` layer)
  - Image structures
  - Error handling
  - Memory management

- **Device Drivers** (`device/` layer)
  - Camera abstraction
  - Display abstraction
  - Serial communication abstraction

## Cross-References

The documentation includes cross-references between:
- C++ wrapper methods → underlying C functions
- Related functions within the same module
- Example: `Image::erode()` → `::erode()`

## Contributing

When adding new functions or classes:

1. Use Doxygen-style comments (`/** ... */`)
2. Include `@brief` description
3. Document parameters with `@param[in]`, `@param[out]`, or `@param[in,out]`
4. Document return values with `@return`
5. Add notes with `@note` for important behavior
6. Add cross-references with `@see`

### Example:

```cpp
/**
 * @brief Applies Gaussian blur to image.
 * @param[in] src Source image
 * @param[out] dst Destination image
 * @param[in] kernelSize Blur kernel size (must be odd)
 * @return Status code
 * @note Both images must have matching dimensions
 * @see ::filter2D For general convolution
 */
embeddip_status_t gaussianBlur(const Image *src, Image *dst, int kernelSize);
```

## Troubleshooting

### Documentation not generating?

Check the warnings log:
```bash
cat doxygen_warnings.log
```

### Missing diagrams?

Ensure Graphviz is installed:
```bash
dot -V
```

### GitHub Pages not showing?

1. Go to repository Settings → Pages
2. Set Source to "GitHub Actions"
3. Check Actions tab for workflow status

## Version

Documentation version matches library version: **0.1.0**
